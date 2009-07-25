/*
Author : Paul Fox
Date:  July 2009

Much of the working code derived from the readelf.c utility
and reading the DWARF spec, copied verbatim below.

	readelf -wf <exename>

and this code should agree, but readelf is more generic targetting
arbitrary cpus.

This file is both a kernel driver subroutine for stack
walking an application with/without frame pointers, but
compiled with DWARF eh_frame support, and a standalone
tool.

*/

/**********************************************************************/
/*   Taken from the Dwarf spec...				      */
/*
http://refspecs.freestandards.org/dwarf/dwarf-2.0.0.pdf

Revision: 2.0.0                                Page 62                                  July 27, 1993

6.4.2 Call Frame Instructions
Each call frame instruction is defined to take 0 or more operands. Some of the operands may be
encoded as part of the opcode (see section 7.23). The instructions are as follows:

 1. DW_CFA_advance_loc takes a single argument that represents a constant delta. The
    required action is to create a new table row with a location value that is computed by taking
    the current entry\u2019s location value and adding (delta * code_alignment_factor). All
    other values in the new row are initially identical to the current row.
 2. DW_CFA_offset takes two arguments: an unsigned LEB128 constant representing a
    factored offset and a register number. The required action is to change the rule for the
    register indicated by the register number to be an offset(N) rule with a value of (N =
    factored offset * data_alignment_factor).
 3. DW_CFA_restore takes a single argument that represents a register number. The
    required action is to change the rule for the indicated register to the rule assigned it by the
    initial_instructions in the CIE.
 4. DW_CFA_set_loc takes a single argument that represents an address. The required
    action is to create a new table row using the specified address as the location. All other
    values in the new row are initially identical to the current row. The new location value
    should always be greater than the current one.
 5. DW_CFA_advance_loc1 takes a single ubyte argument that represents a constant delta.
    This instruction is identical to DW_CFA_advance_loc except for the encoding and size
    of the delta argument.
 6. DW_CFA_advance_loc2 takes a single uhalf argument that represents a constant delta.
    This instruction is identical to DW_CFA_advance_loc except for the encoding and size
    of the delta argument.
 7. DW_CFA_advance_loc4 takes a single uword argument that represents a constant delta.
    This instruction is identical to DW_CFA_advance_loc except for the encoding and size
    of the delta argument.
 8. DW_CFA_offset_extended takes two unsigned LEB128 arguments representing a
    register number and a factored offset. This instruction is identical to DW_CFA_offset
    except for the encoding and size of the register argument.
 9. DW_CFA_restore_extended takes a single unsigned LEB128 argument that
    represents a register number. This instruction is identical to DW_CFA_restore except
    for the encoding and size of the register argument.
10. DW_CFA_undefined takes a single unsigned LEB128 argument that represents a register
    number. The required action is to set the rule for the specified register to \u2018\u2018undefined.\u2019\u2019
11. DW_CFA_same_value takes a single unsigned LEB128 argument that represents a
    register number. The required action is to set the rule for the specified register to \u2018\u2018same
    value.\u2019\u2019
12. DW_CFA_register takes two unsigned LEB128 arguments representing register
    numbers. The required action is to set the rule for the first register to be the second register.
13. DW_CFA_remember_state
14. DW_CFA_restore_state
    These      instructions    define      a   stack    of   information.       Encountering     the
    DW_CFA_remember_state instruction means to save the rules for every register on the
    current row on the stack. Encountering the DW_CFA_restore_state instruction
    means to pop the set of rules off the stack and place them in the current row. (This
    operation is useful for compilers that move epilogue code into the body of a function.)
15. DW_CFA_def_cfa takes two unsigned LEB128 arguments representing a register number
    and an offset. The required action is to define the current CFA rule to use the provided
    register and offset.
16. DW_CFA_def_cfa_register takes a single unsigned LEB128 argument representing
    a register number. The required action is to define the current CFA rule to use the provided
    register (but to keep the old offset).
17. DW_CFA_def_cfa_offset takes a single unsigned LEB128 argument representing an
    offset. The required action is to define the current CFA rule to use the provided offset (but
    to keep the old register).
18. DW_CFA_nop has no arguments and no required actions. It is used as padding to make the
    FDE an appropriate size.

6.4.3 Call Frame Instruction Usage
To determine the virtual unwind rule set for a given location (L1), one searches through the FDE
headers looking at the initial_location and address_range values to see if L1 is
contained in the FDE. If so, then:
  1.  Initialize a register set by reading the initial_instructions field of the associated
      CIE.
  2. Read and process the FDE\u2019s instruction sequence until a DW_CFA_advance_loc,
      DW_CFA_set_loc, or the end of the instruction stream is encountered.
  3.  If a DW_CFA_advance_loc or DW_CFA_set_loc instruction was encountered, then
      compute a new location value (L2). If L1 >= L2 then process the instruction and go back
      to step 2.
  4.  The end of the instruction stream can be thought of as a
      DW_CFA_set_loc( initial_location + address_range )
      instruction. Unless the FDE is ill-formed, L1 should be less than L2 at this point.
The rules in the register set now apply to location L1.

*/
/**********************************************************************/

#if defined(_KERNEL)
#include "dtrace_linux.h"
#else
# define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
//#include <dwarf.h> /* Might not have dwarf.h, but we dont need it */
#include <elf.h>
#include <libelf.h>
#endif

/**********************************************************************/
/*   Kernel mode definitions.					      */
/**********************************************************************/
#if defined(_KERNEL)
#	define	printf if (1) printk
#else
#	define	printk printf
#endif

#define DW_EH_PE_FORMAT_MASK    0x0f    /* format of the encoded value */
#if !defined(DW_EH_PE_absptr)
/* Pointer-encoding formats: */
#define DW_EH_PE_omit           0xff
#define DW_EH_PE_ptr            0x00    /* pointer-sized unsigned value */
#define DW_EH_PE_uleb128        0x01    /* unsigned LE base-128 value */
#define DW_EH_PE_udata2         0x02    /* unsigned 16-bit value */
#define DW_EH_PE_udata4         0x03    /* unsigned 32-bit value */
#define DW_EH_PE_udata8         0x04    /* unsigned 64-bit value */
#define DW_EH_PE_sleb128        0x09    /* signed LE base-128 value */
#define DW_EH_PE_sdata2         0x0a    /* signed 16-bit value */
#define DW_EH_PE_sdata4         0x0b    /* signed 32-bit value */
#define DW_EH_PE_sdata8         0x0c    /* signed 64-bit value */

/* Pointer-encoding application: */
#define DW_EH_PE_absptr         0x00    /* absolute value */
#define DW_EH_PE_pcrel          0x10    /* rel. to addr. of encoded value */
#define DW_EH_PE_textrel        0x20    /* text-relative (GCC-specific???) */
#define DW_EH_PE_datarel        0x30    /* data-relative */
#endif

/**********************************************************************/
/*   CFA definitions.						      */
/**********************************************************************/
#if !defined(DW_CFA_advance_loc)
#define DW_CFA_advance_loc      0x40
#define DW_CFA_offset           0x80

#define DW_CFA_nop              0x00
#define DW_CFA_set_loc          0x01
#define DW_CFA_advance_loc1     0x02
#define DW_CFA_advance_loc2     0x03
#define DW_CFA_advance_loc4     0x04
#define DW_CFA_def_cfa          0x0c
#define DW_CFA_def_cfa_register 0x0d
#define DW_CFA_def_cfa_offset   0x0e

/**********************************************************************/
/*   For the fda_encoding/pc_begin/pc_end code.			      */
/**********************************************************************/
#define DW_EH_PE_signed		0x08

#endif


typedef struct dwarf_eh_frame_hdr {
        unsigned char version;
        unsigned char eh_frame_ptr_enc;
        unsigned char fde_count_enc;
        unsigned char table_enc;
        } dwarf_eh_frame_hdr;

#if !defined(_KERNEL)
static int do_phdr;
static int do_sec;
static int do_self;

static void dump_mem(char *start, char *mem, char *memend);
#endif

typedef struct dw_info_t {
	int	data_factor;
	int	code_factor;
	int	fde_encoding;
	unsigned long pc_begin;
	unsigned long pc_end;
	Elf64_Shdr	*eh_frame_sec;
	Elf64_Shdr	*eh_frame_hdr_sec;
	Elf64_Shdr	eh_frame_s;
	Elf64_Shdr	eh_frame_hdr_s;
        char    *eh_frame_data;
        char    *eh_frame_hdr_data;
} dw_info_t;

#define LEB(cp) get_leb128(&cp, 0)
#define SLEB(cp) get_leb128(&cp, 1)
/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
static char *dump_ptr(char *ptr, char *);
void elferr(char *);
static int size_of_encoded_value(int encoding);
static unsigned long get_encoded_value(char **fp, int encoding);

static unsigned long
get_encoded_value(char **fp, int encoding)
{
	unsigned long n;

	if (encoding & DW_EH_PE_signed) {
		n = *(int32_t *) *fp;
	} else {
		n = *(uint32_t *) *fp;
	}
	*fp += 4;

	return n;
}
unsigned long
get_leb128(char **fp, int sign)
{	unsigned shift = 0;
	unsigned char	val;
	unsigned long result = 0;
	char	*bp = *fp;

	do {
		val = *bp++;
		result |= ((unsigned long) (val & 0x7f)) << shift;
		shift += 7;
		}
	while (val & 0x80);
	if (sign && shift < 64 && val & 0x40)
		result |= -1L << shift;

	*fp = bp;

	return result;
}
#if !defined(_KERNEL)
/**********************************************************************/
/*   Userland code using the -lelf library.			      */
/**********************************************************************/
static int
do_dwarf_elf(char *filename, dw_info_t *dw)
{	int	fd;
        Elf     *elf;
        Elf64_Ehdr *ehdr;
	int	i;

        if ((fd = open(filename, O_RDONLY)) < 0) {
                perror(filename);
                exit(1);
        }

        if (elf_version(EV_CURRENT) == EV_NONE) {
                printf("elf_version fails\n");
                exit(1);
        }
        elf = elf_begin(fd, ELF_C_READ, (Elf *)0);
        if (elf == 0)
                elferr("elf_begin");

        ehdr = elf64_getehdr(elf);
        if (ehdr == NULL)
                elferr("elf64_getehdr");

#if 0
        phdr = elf64_getphdr(elf);
        if (phdr == NULL)
                elferr("elf64_getphdr");

        for (i = 0; i < ehdr->e_phnum; i++) {
		if (do_phdr)
	                printf("%d: type=%x offset=%lx size=%lx vaddr=%p\n", 
				i, phdr->p_type, phdr->p_offset, phdr->p_memsz,
				phdr->p_vaddr);
                if (phdr->p_type == PT_GNU_EH_FRAME)
                        p_eh = phdr;
                if (phdr->p_type == PT_LOAD)
                        p_text = phdr;
                phdr++;
        }
        if (p_eh == NULL) {
                fprintf(stderr, "Couldnt locate .eh_frame section\n");
                exit(1);
        }
#endif
        /***********************************************/
        /*   Find the .eh_frame section.               */
        /***********************************************/
        Elf_Scn *scn = elf_getscn(elf, ehdr->e_shstrndx);
        Elf_Data *edata;
        if ((edata = elf_getdata(scn, NULL)) == NULL) {
                elferr("elf_getdata");
        }
        char *data = edata->d_buf;
        for (i = 0; i < ehdr->e_shnum; i++) {
                Elf_Scn *scn = elf_getscn(elf, i);
                Elf64_Shdr *shdr = elf64_getshdr(scn);
                char *name = data + shdr->sh_name;
		if (do_sec)
	                printf("  sec %d: %-20s size=%ld\n", i, name, shdr->sh_size);
                if (strcmp(name, ".eh_frame") == 0) {
			dw->eh_frame_sec = shdr;
                        if ((edata = elf_getdata(scn, NULL)) == NULL)
                                elferr("elf_getdata");
                        dw->eh_frame_data = edata->d_buf;
		}
                if (strcmp(name, ".eh_frame_hdr") == 0) {
			dw->eh_frame_hdr_sec = shdr;
                        if ((edata = elf_getdata(scn, NULL)) == NULL)
                                elferr("elf_getdata");
                        dw->eh_frame_hdr_data = edata->d_buf;
		}
        }
	if (dw->eh_frame_sec == NULL) {
                fprintf(stderr, "Could not locate .eh_frame section\n");
		exit(1);
	}

	return 0;
}
#endif

/**********************************************************************/
/*   Get  ready to look for the .eh_frame / .eh_frame_hdr when given  */
/*   a pointer to an in-memory ELF header.			      */
/**********************************************************************/
int
do_dwarf_phdr(char *ptr, dw_info_t *dw)
{
        Elf64_Ehdr *ehdr;
        Elf64_Phdr *phdr;
	int	i;
	int	found = 0;

	/***********************************************/
	/*   Check for ELF magic header.	       */
	/***********************************************/
	if (ptr[0] != 0x7f || ptr[1] != 'E' || ptr[2] != 'L' || ptr[3] != 'F')
		return -1;

printk("do_dwarf_phdr: %d\n", __LINE__);
int do_phdr = 1;
        ehdr = (Elf64_Ehdr *) ptr;
        phdr = (Elf64_Phdr *) ((char *) ehdr + ehdr->e_phoff);

	memset(dw, 0, sizeof *dw);

        for (i = 0; i < ehdr->e_phnum; i++) {
		if (do_phdr)
			printf("%d: type=%x offset=%lx size=%lx vaddr=%p\n", 
				i, phdr->p_type, (long) phdr->p_offset, 
				(long) phdr->p_memsz,
				(void *) phdr->p_vaddr);
		if (phdr->p_type == PT_GNU_EH_FRAME) {
			char	*addr;

			dw->eh_frame_hdr_sec = &dw->eh_frame_hdr_s;
			dw->eh_frame_hdr_s.sh_addr = phdr->p_vaddr;
			dw->eh_frame_hdr_s.sh_offset = phdr->p_offset;
			dw->eh_frame_hdr_s.sh_size = phdr->p_memsz;
                        dw->eh_frame_hdr_data = (char *) phdr->p_vaddr;

			addr = (char *) phdr->p_vaddr + phdr->p_memsz;
#if defined(_KERNEL)
			if (ehdr->e_type == ET_DYN) {
				addr = ptr + (long) addr;
			}
#endif
			if ((long) addr & 0x7)
				addr = (char *) (((long) addr | 7) + 1);
			dw->eh_frame_sec = &dw->eh_frame_s;
			dw->eh_frame_s.sh_addr = (long) addr;
			dw->eh_frame_s.sh_offset = (long) addr;
			dw->eh_frame_s.sh_size = phdr->p_memsz; //  + 0x2277c;
                        dw->eh_frame_data = addr;

			found = 1;
		}
                phdr++;
        }

	if (!found)
		return -1;

printk("do_dwarf_phdr: %d\n", __LINE__);
	/***********************************************/
	/*   Need   to   compute   the  size  of  the  */
	/*   .eh_frame section. The strtab may not be  */
	/*   loaded  into memory so we cannot rely on  */
	/*   having  a  section  table  to  determine  */
	/*   this.				       */
	/***********************************************/
	char *fp_start = (char *) ehdr + (int) dw->eh_frame_hdr_data;
	if (ehdr->e_type == ET_EXEC) {
		fp_start = dw->eh_frame_hdr_data;
	}
	char *fp = fp_start;

printk("do_dwarf_phdr: %d ehdr=%p eh_frame_hdr_data=%p fp=%p\n", __LINE__, ehdr, dw->eh_frame_hdr_data, fp);
	printf("EH.version: %02x\n", *fp++);
	printf("EH.eh_frame_ptr_enc: %02x\n", *fp++);
	printf("EH.fde_fount_enc: %02x\n", *fp++);
	printf("EH.table_enc: %02x\n", *fp++);
	printf("EH.eh_frame_ptr: %02x\n", *(uint32_t *) fp); fp += 4;
	int fde_count = *(uint32_t *) fp; fp += 4;
	printf("EH.fde_count: %02x\n", fde_count);
	unsigned addr = 0;
	for (i = 0; i < fde_count; i++) {
		char *fp1 = fp + i * 2 * 4;
		unsigned addr1 = *(uint32_t *) (fp1 + 4);
//printk("%d addr1: %x\n", i, addr1);
		addr = addr > addr1 ? addr : addr1;
	}
	/***********************************************/
	/*   Bug  alert:  we  found  the last FDE/CIE  */
	/*   entry  but  not its size, so we may fail  */
	/*   to  map  on  the  last  entry  unless we  */
	/*   compute the size here.		       */
	/***********************************************/
	printf("max addr=%x\n", addr);

	dw->eh_frame_s.sh_size += addr;

/*
	Elf64_Shdr *strsec = (char *) ehdr + ehdr->e_shoff + ehdr->e_shstrndx * sizeof(Elf64_Shdr);
	char *strtab = (char *) ehdr + strsec->sh_addr;
printk("strtab=%p\n", strtab);
return 0;
        for (i = 0; i < ehdr->e_shnum; i++) {
		Elf64_Shdr *scn = (char *) ehdr + ehdr->e_shoff + i * sizeof(Elf64_Shdr);
		printk("sec %d: %p %p\n", i, strtab + 25, *(long *) scn);
		break;
	}
*/
	return 0;
}

int
dwarf_read_pointer(void **addr, void *dst, int fmt)
{       char    *src = *addr;

        switch (fmt & DW_EH_PE_FORMAT_MASK) {
          case DW_EH_PE_sdata4:
                *(int *) dst = *(int *) src;
                *addr += 4;
                return 0;

          default:
                printf("fmt %02x not handled\n", fmt);
                return -1;
        }
}
/**********************************************************************/
/*   Find  where  the  return address is for a given function on the  */
/*   stack.							      */
/**********************************************************************/
int
dw_find_ret_addr(dw_info_t *dw, unsigned long pc, int *cfa_offsetp)
{	int	i;
        char *fp = dw->eh_frame_data;
	char	*fp_start;
	int	cie;
	int	len;
	int	version;
	char *eh_frame_end = dw->eh_frame_data + dw->eh_frame_sec->sh_size;
//	printf("=== .eh_frame\n");


	char	*aug = NULL;

printk("dw_find_ret_addr: here....1\npc=%p fp=%p end=%p size=%x\n", pc, fp, eh_frame_end, dw->eh_frame_sec->sh_size);
	/***********************************************/
	/*   Walk  the  series of CIE/FDE entries til  */
	/*   we  find  one  that  matches  the target  */
	/*   program counter.			       */
	/***********************************************/
	while (fp < eh_frame_end) {
		char	*cp, *cpend;
		int	cfa_offset;

	        len = *(uint32_t *) fp; fp += 4;

		if (len == 0) {
printf("  Length is zero -- end\n");
continue;
			return 0;
		}
		fp_start = fp;

		cie = *(uint32_t *) fp; fp += 4;
		if (cie == 0) {
		        printf("\nCIE length=%08x\n", len);
			version = *fp++;
		        printf("  Version:              %02x\n", version);
			aug = fp;
		        printf("  Augmentation:         \"");
			for (i = 0; fp[i]; i++)
				printf("%c", fp[i]);
			printf("\"\n");
			fp += strlen(fp) + 1;

			dw->code_factor = LEB(fp);
		        printf("  Code alignment factor: %d\n", dw->code_factor);
			dw->data_factor = SLEB(fp);
		        printf("  Data alignment factor: %d\n", dw->data_factor);
			if (strchr(aug, 'z')) {
				int ret_addr = version == 1 ? (int) *fp++ : (int) LEB(fp);
				int	aug_len;
				char	*a, *p;

				printf("  Return address reg:    0x%02x\n", ret_addr);
				aug_len = LEB(fp);
				if (aug_len) {
					printf("  Augmentation Length:   len=0x%02x ", aug_len);
					for (i = 0; i < aug_len; i++) {
						printf("%02x ", fp[i] & 0xff);
					}
					printf("\n");
				}
				a = fp;
				fp += aug_len;
				for (p = aug; ; p++) {
					int	ok = 1;
//printf("aug %x %c\n", *p, *p ? *p : 'x');
					switch (*p) {
					  case 'L':
					  	a++;
					  	break;
					  case 'P':
					  	a += 1 + size_of_encoded_value(*a);
					  	break;
					  case 'R':
printf("R encoding %x (kernel)\n", *a);
					  	dw->fde_encoding = *a++;
					  	break;
					  case 'z':
					  	break;
					  default:
					  	ok = 0;
						break;
					  }
					if (!ok)
						break;
				}
			fp = fp_start + len;
			continue;
			}
		} else {
			dw->pc_begin = get_encoded_value(&fp, dw->fde_encoding);
			dw->pc_end = get_encoded_value(&fp, dw->fde_encoding);
//printf("pc_begin=%p vad=%p %p\n", pc_begin, p_eh->p_vaddr, fp-eh_frame_data);
			if ((dw->fde_encoding & 0x70) == DW_EH_PE_pcrel) {
//printk("pc_begin1=%p pc_begin2=%p\n", dw->pc_begin, dw->eh_frame_sec->sh_addr);
//printk("pc_begin3=%p looking 4 %p\n", fp - dw->eh_frame_data - 8, pc);
				dw->pc_begin += dw->eh_frame_sec->sh_addr;
				dw->pc_begin += fp - dw->eh_frame_data - 8*0;
			}
if(0)
		        printf("FDE len=%x cie=%04x pc=%lx..%lx tpc=%lx\n", 
				len, cie,
				dw->pc_begin, dw->pc_begin + dw->pc_end, pc);
//printf("fde_encoding=%d\n", fde_encoding);
			if (*aug == 'z') {
				int aug_len = *fp++;
				if (aug_len) {
					printf("  Augmentation Length: 0x%02x ", aug_len);
					for (i = 0; i < aug_len; i++)
						printf("%02x ", fp[i] & 0xff);
					printf("\n");
				}
				fp += aug_len;
			}
		}
//		dump_mem(eh_frame_data, fp, fp +32);

		/***********************************************/
		/*   If  the  target PC is not in this block,  */
		/*   then  we  can skip over it without doing  */
		/*   any interpretation.		       */
		/***********************************************/
		cfa_offset = 0;
		if (pc < dw->pc_begin) {
fp = fp_start + len;
continue;
			/***********************************************/
			/*   Something  wrong  ..  we  didnt find the  */
			/*   block.				       */
			/***********************************************/
			printf("block not found pc=%p begin=%p cfa_offset=%d\n", 
				(void *) pc, 
				(void *) dw->pc_begin,
				cfa_offset);
			*cfa_offsetp = 0;
			return 0;
		}
		/***********************************************/
		/*   Not reached the right block yet.	       */
		/***********************************************/
		if (pc >= dw->pc_begin + dw->pc_end) {
//printf("skip pc=%p pc_end=%p (diff=%p)\n", (void *) pc, (void *) (dw->pc_begin + dw->pc_end), (char *) pc - (char *) (dw->pc_begin + dw->pc_end));
			fp = fp_start + len;
			continue;
		}

	        cp = fp; 
		cpend = fp_start + len;

	        while (cp < cpend) {
			int	a, b;
			int	offset;
			int	op;
			int	opa;
			char	msgbuf[128];
			char	*cp1;
			char	*cp_start = cp;

	                printf("%04lx: ", cp - fp);
			op = *cp++;
			opa = op & 0x3f;
			if (op & 0xc0)
				op &= 0xc0;

	                switch (op) {
	                  case DW_CFA_advance_loc:
			  	/***********************************************/
			  	/*   As  we  advance the PC, we may jump past  */
			  	/*   the  target  instruction. But we need to  */
			  	/*   grab   the  DW_CFA_def_cfa_offset  which  */
			  	/*   appears  here.  So, we can skip out once  */
			  	/*   we get past the target area.	       */
			  	/***********************************************/
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_advance_loc %d to %08lx",
					opa * dw->code_factor,
					dw->pc_begin + opa * dw->code_factor);
				dw->pc_begin += opa * dw->code_factor;
				if (pc < dw->pc_begin) {
printf("found you, you little baby\n");
					*cfa_offsetp = cfa_offset;
					return 1;
				}
				break;

	                  case DW_CFA_advance_loc4:
			  	offset = *(int32_t *) cp; cp += 4;
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_advance_loc4 %d to %08lx",
					offset * dw->code_factor,
					dw->pc_begin + offset * dw->code_factor);
				dw->pc_begin += offset * dw->code_factor;
				break;

	                  case DW_CFA_def_cfa:
				a = LEB(cp);
				b = LEB(cp);
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_def_cfa: r%d ofs %d", a, b);
	                        break;
	                  case DW_CFA_def_cfa_offset:
				a = LEB(cp);
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_def_cfa_offset: %d", a);
				cfa_offset = a;
	                        break;
	                  case DW_CFA_def_cfa_register:
				a = LEB(cp);
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_def_cfa_register: r%d", a);
	                        break;
	                  case DW_CFA_offset:
				a = LEB(cp);
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_offset: r%d at cfa%+d", opa, a * dw->data_factor);
//				cfa_offset = a * dw->data_factor;
	                        break;
	                  case DW_CFA_set_loc:
	                        snprintf(msgbuf, sizeof msgbuf, "DW_CFA_set_loc: ");
	                        cp = dump_ptr(cp, msgbuf + strlen(msgbuf));
	                        break;
			  case DW_CFA_nop:
	                        snprintf(msgbuf, sizeof msgbuf, "DW_CFA_nop");
			  	break;
			  default:
			  	printf("unsupported DW entry %x\n", op);
				return 0;
	                  }
			for (a = 0, cp1 = cp_start; a < 4; cp1++, a++) {
				if (cp1 < cp) {
					printf("%02x ", *cp1 & 0xff);
				} else {
					printf("   ");
				}
			}
	                printf("%s\n", msgbuf);
	        }
		fp = cp;
printf("got it....\n");
*cfa_offsetp = cfa_offset;
return 1;
	}
	/***********************************************/
	/*   Nope - not here at all.		       */
	/***********************************************/
printf("not found at all....\n");
printk("here....1 fp=%p end=%p size=%x\n", fp, eh_frame_end, dw->eh_frame_sec->sh_size);
	return 0;
}
static char *
dump_ptr(char *ptr, char *msgbuf)
{
        int     i;

        for (i = 0; i < 8; i++) {
                snprintf(msgbuf, sizeof msgbuf, "%02x", *ptr++ & 0xff);
		msgbuf += 2;
        }
        return ptr;
}
static int
size_of_encoded_value(int encoding)
{
	switch (encoding & 0x7) {
	  default:
	  case 0:     return sizeof(void *);
	  case 2:     return 2;
	  case 3:     return 4;
	  case 4:     return 8;
	}
}
/**********************************************************************/
/*   Following  is  the  main  code  for  the  userland  utility for  */
/*   dumping/debugging the dwarf stack walk code.		      */
/**********************************************************************/
#if !defined(_KERNEL)
int 
do_switches(int argc, char **argv)
{	int	i;
	char	*cp;

	for (i = 1; i < argc; i++) {
		cp = argv[i];
		if (*cp++ != '-')
			return i;
		if (strcmp(cp, "self") == 0) {
			do_self = 1;
			continue;
		}

		while (*cp) {
			switch (*cp++) {
			  case 'p':
			  	do_phdr = 1;
				break;
			  case 's':
			  	do_sec = 1;
				break;
			}
		}
	}
	return i;
}
/**********************************************************************/
/*   Main entry for the program.				      */
/**********************************************************************/
int 
main(int argc, char **argv)
{       int     i;
        char    *cp, *cpend;
        dwarf_eh_frame_hdr *dhp;
	int	arg_index;
	dw_info_t	dw;

	arg_index = do_switches(argc, argv);

	memset(&dw, 0, sizeof dw);

	if (do_self) {
		char	buf[BUFSIZ];
		FILE	*fp;
		sprintf(buf, "/proc/%d/maps", getpid());
		char	*addr;

		fp = fopen(buf, "r");
		while (fgets(buf, sizeof buf, fp)) {
			unsigned long lo, hi;
			sscanf(buf, "%lx-%lx", &lo, &hi);
			do_dwarf_phdr((char *) lo, &dw);
			break;
		}
		if (arg_index >= argc) {
			printf("Usage: eh -self <addr>\n");
			exit(1);
		}
		sscanf(argv[arg_index], "%p", &addr);
		printf("Searching for %p\n", addr);
		int	cfa_offset = 0;
		int ret = dw_find_ret_addr(&dw, (unsigned long) addr, &cfa_offset);
		printf("dw_find_ret_addr: returns %d, cfa=%d\n", ret, cfa_offset);
		exit(0);

	} else {
		do_dwarf_elf(argv[arg_index], &dw);
	}

	if (dw.eh_frame_hdr_sec) {
//	dump_mem(mem, mem, mem + p_eh->p_memsz);

	        dhp = (dwarf_eh_frame_hdr *) dw.eh_frame_hdr_data;
	        printf("DWARF.version:          %02x\n", dhp->version);
	        printf("DWARF.eh_frame_ptr_enc: %02x\n", dhp->eh_frame_ptr_enc);
	        printf("DWARF.fde_count_enc:    %02x\n", dhp->fde_count_enc);
	        printf("DWARF.table_enc:        %02x\n", dhp->table_enc);

	        int frame_ptr_offset = 0;
	        char *ptr = (char *) (dhp + 1);
	        dwarf_read_pointer((void **) &ptr, &frame_ptr_offset, dhp->eh_frame_ptr_enc);
	        printf("frame_ptr: %08x\n", frame_ptr_offset);

	        int fde_count = 0;
	        dwarf_read_pointer((void **) &ptr, &fde_count, dhp->eh_frame_ptr_enc);
	        printf("fde_count: %08x\n", fde_count);

	        if (dhp->table_enc != (DW_EH_PE_datarel | DW_EH_PE_sdata4)) {
	                printf("Unexpected table_enc - giving up 0x%x\n", dhp->table_enc);
	                exit(1);
	        }
	/*
	        char    *table;
	        int table_len;


	        table_len = (fde_count * 8) / sizeof(void *); // int32 or int64
	        table = p_eh->p_vaddr + ptr - (char *) ehdr - p_eh->p_offset;
	*/
	        typedef struct ftable_t {
	                int32_t         t_ip;
	                int32_t         t_frame_offset;
	                } ftable_t;
	        ftable_t *table = (ftable_t *) ptr;
	        printf("table=%p\n", table);
	        for (i = 0; i < fde_count; i++) {
	                printf(" %d: %04x %04x\n", i, table[i].t_ip, table[i].t_frame_offset);
	        }
	}

        char *fp = dw.eh_frame_data;
	char *eh_frame_end = dw.eh_frame_data + dw.eh_frame_sec->sh_size;
	printf("=== .eh_frame\n");


	char	*aug = NULL;
	while (fp < eh_frame_end) {
	        int len = *(uint32_t *) fp; fp += 4;
		if (len == 0) {
			printf("  Length is zero -- end\n");
			break;
		}
		char	*fp_start = fp;

		int cie = *(uint32_t *) fp; fp += 4;
		if (cie == 0) {
		        printf("\nCIE length=%08x\n", len);
			int	version = *fp++;
		        printf("  Version:              %02x\n", version);
			aug = fp;
		        printf("  Augmentation:         \"");
			while (*fp) {
				printf("%c", *fp++);
			}
			printf("\"\n");
			fp++;
			dw.code_factor = LEB(fp);
		        printf("  Code alignment factor: %d\n", dw.code_factor);
			dw.data_factor = SLEB(fp);
		        printf("  Data alignment factor: %d\n", dw.data_factor);
			if (strchr(aug, 'z')) {
				int ret_addr = version == 1 ? (int) *fp++ : (int) LEB(fp);
			        printf("  Return address reg:    0x%02x\n", ret_addr);
				int aug_len = LEB(fp);
				if (aug_len) {
					printf("  Augmentation Length:   len=0x%02x ", aug_len);
					for (i = 0; i < aug_len; i++) {
						printf("%02x ", fp[i]);
					}
					printf("\n");
				}
				char	*a = fp;
				fp += aug_len;
				char *p;
				for (p = aug; ; p++) {
					int	ok = 1;
//printf("aug %x %c\n", *p, *p ? *p : 'x');
					switch (*p) {
					  case 'L':
					  	a++;
					  	break;
					  case 'P':
					  	a += 1 + size_of_encoded_value(*a);
					  	break;
					  case 'R':
printf("R encoding %x\n", *a);
					  	dw.fde_encoding = *a++;
					  	break;
					  case 'z':
					  	break;
					  default:
					  	ok = 0;
						break;
					  }
					if (!ok)
						break;
				}
			}
		} else {
			dw.pc_begin = get_encoded_value(&fp, dw.fde_encoding);
			dw.pc_end = get_encoded_value(&fp, dw.fde_encoding);
//printf("pc_begin=%p vad=%p %p\n", pc_begin, p_eh->p_vaddr, fp-eh_frame_data);
//printf("encoding=%x\n", dw.fde_encoding);
			if ((dw.fde_encoding & 0x70) == DW_EH_PE_pcrel) {
printf("+= %p\n", dw.eh_frame_sec->sh_addr);
				dw.pc_begin += dw.eh_frame_sec->sh_addr;
printf("+= %p - %p\n", fp, dw.eh_frame_data);
				dw.pc_begin += fp - dw.eh_frame_data - 8;
			}
		        printf("\nFDE length=%08x ptr=%04x pc=%08lx..%08lx\n", 
				len, cie,
				dw.pc_begin, dw.pc_begin + dw.pc_end);
//printf("fde_encoding=%d\n", fde_encoding);
			if (strchr(aug, 'z')) {
				int aug_len = *fp++;
				if (aug_len) {
					printf("  Augmentation Length: 0x%02x ", aug_len);
					for (i = fp[-1]; i-- > 0; )
						printf("%02x ", *fp++);
					printf("\n");
				}
			}
		}
//		dump_mem(eh_frame_data, fp, fp +32);

	        cp = fp; cpend = fp_start + len;
	        while (cp < cpend) {
			int	a, b;
			int	offset;
			int	op;
			int	opa;
			char	msgbuf[128];
			char	*cp1;
			char	*cp_start = cp;

	                printf("%04lx: ", cp - fp);
			op = *cp++;
			opa = op & 0x3f;
			if (op & 0xc0)
				op &= 0xc0;

	                switch (op) {
	                  case DW_CFA_advance_loc:
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_advance_loc %d to %08lx",
					opa * dw.code_factor,
					dw.pc_begin + opa * dw.code_factor);
				dw.pc_begin += opa * dw.code_factor;
				break;

	                  case DW_CFA_advance_loc4:
			  	offset = *(int32_t *) cp; cp += 4;
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_advance_loc4 %d to %08lx",
					offset * dw.code_factor,
					dw.pc_begin + offset * dw.code_factor);
				dw.pc_begin += offset * dw.code_factor;
				break;

	                  case DW_CFA_def_cfa:
				a = LEB(cp);
				b = LEB(cp);
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_def_cfa: r%d ofs %d", a, b);
	                        break;
	                  case DW_CFA_def_cfa_offset:
				a = LEB(cp);
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_def_cfa_offset: %d", a);
	                        break;
	                  case DW_CFA_def_cfa_register:
				a = LEB(cp);
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_def_cfa_register: r%d", a);
	                        break;
	                  case DW_CFA_offset:
				a = LEB(cp);
				snprintf(msgbuf, sizeof msgbuf, "DW_CFA_offset: r%d at cfa%+d", opa, a * dw.data_factor);
	                        break;
	                  case DW_CFA_set_loc:
	                        snprintf(msgbuf, sizeof msgbuf, "DW_CFA_set_loc: ");
	                        cp = dump_ptr(cp, msgbuf + strlen(msgbuf));
	                        break;
			  case DW_CFA_nop:
	                        snprintf(msgbuf, sizeof msgbuf, "DW_CFA_nop");
			  	break;
			  default:
			  	fprintf(stderr, "unsupported DW entry %x\n", op);
				exit(1);
	                  }
			for (a = 0, cp1 = cp_start; a < 4; cp1++, a++) {
				if (cp1 < cp)
					printf("%02x ", *cp1 & 0xff);
				else
					printf("   ");
			}
	                printf("%s\n", msgbuf);
	        }
		fp = cp;
	}
}
static void
dump_mem(char *start, char *mem, char *memend)
{	char	*cp;
	int	i;

        for (cp = mem; cp < memend; cp += 16) {
                printf("%04lx: ", cp - start);
                for (i = 0; i < 16 && cp + i < memend; i++) {
                        printf("%02x ", cp[i] & 0xff);
                }
                printf("\n");
        }
}
void
elferr(char *str)
{
        fprintf(stderr, "%s: %s\n", str, elf_errmsg(elf_errno()));
        exit(1);
}
#endif /* !defined(_KERNEL) */

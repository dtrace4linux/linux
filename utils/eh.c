/*
Author : Paul Fox
Date:  July 2009

This is crap experimental code to understand DWARF debug records.

Much of the working code derived from the readelf.c utility
and reading the DWARF spec, copied verbatim below.

	readelf -wf <exename>

and this code should agree, but readelf is more generic targetting
arbitrary cpus.

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

# define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dwarf.h>
#include <elf.h>
#include <libelf.h>

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

typedef struct dwarf_eh_frame_hdr {
        unsigned char version;
        unsigned char eh_frame_ptr_enc;
        unsigned char fde_count_enc;
        unsigned char table_enc;
        } dwarf_eh_frame_hdr;

static int do_phdr;
static int do_sec;
static int do_self;

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

#define LEB(cp) get_leb128(&cp, 0);
#define SLEB(cp) get_leb128(&cp, 1);
/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
static void dump_mem(char *start, char *mem, char *memend);
static char *dump_ptr(char *ptr, char *);
void elferr(char *);
static int size_of_encoded_value(int encoding);

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
/**********************************************************************/
/*   Get  ready to look for the .eh_frame / .eh_frame_hdr when given  */
/*   a pointer to an in-memory ELF header.			      */
/**********************************************************************/
static int
do_dwarf_phdr(char *ptr, dw_info_t *dw)
{
        Elf64_Ehdr *ehdr;
        Elf64_Phdr *phdr;
        Elf64_Shdr *shdr;
        Elf64_Shdr *str_sec;
	char	*strtab;
	int	i;

        ehdr = (Elf64_Ehdr *) ptr;
        phdr = (Elf64_Phdr *) ((char *) ehdr + ehdr->e_phoff);
        if (phdr == NULL)
                elferr("elf64_getphdr");

        for (i = 0; i < ehdr->e_phnum; i++) {
		if (do_phdr)
			printf("%d: type=%x offset=%lx size=%lx vaddr=%p\n", 
				i, phdr->p_type, phdr->p_offset, phdr->p_memsz,
				(void *) phdr->p_vaddr);
		if (phdr->p_type == PT_GNU_EH_FRAME) {
			dw->eh_frame_hdr_sec = &dw->eh_frame_hdr_s;
			dw->eh_frame_hdr_s.sh_addr = phdr->p_vaddr;
			dw->eh_frame_hdr_s.sh_offset = phdr->p_offset;
			dw->eh_frame_hdr_s.sh_size = phdr->p_memsz;
                        dw->eh_frame_hdr_data = phdr->p_vaddr;

			char *addr = phdr->p_vaddr + phdr->p_memsz;
			if ((int) addr & 0x7)
				addr = (char *) (((long) addr | 7) + 1);
			dw->eh_frame_sec = &dw->eh_frame_s;
			dw->eh_frame_s.sh_addr = addr;
			dw->eh_frame_s.sh_offset = addr;
			dw->eh_frame_s.sh_size = phdr->p_memsz  + 0x1f8;
                        dw->eh_frame_data = addr;
			}
                phdr++;
        }
	/***********************************************/
	/*   Now find the .eh_frame section.	       */
	/***********************************************/
# if 0
	shdr = (Elf64_Shdr *) ((char *) ehdr + ehdr->e_shoff);
	str_sec = shdr + ehdr->e_shstrndx;
	strtab = str_sec->sh_offset;
	for (i = 0; i < ehdr->e_shnum; i++) {
		printf("%d: sec '%s'\n", i, strtab + shdr[i].sh_name);
	}
# endif
	return 0;
}

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
dw_find_ret_addr(dw_info_t *dw, unsigned long pc)
{	int	i;
        char *fp = dw->eh_frame_data;
	char *eh_frame_end = dw->eh_frame_data + dw->eh_frame_sec->sh_size;
//	printf("=== .eh_frame\n");


	char	*aug = NULL;

	/***********************************************/
	/*   Walk  the  series of CIE/FDE entries til  */
	/*   we  find  one  that  matches  the target  */
	/*   program counter.			       */
	/***********************************************/
	while (fp < eh_frame_end) {
	        int len = *(uint32_t *) fp; fp += 4;
		if (len == 0) {
//			printf("  Length is zero -- end\n");
			return 0;
		}
		char	*fp_start = fp;

		int cie = *(uint32_t *) fp; fp += 4;
		if (cie == 0) {
		        printf("\nCIE length=%08x\n", len);
			int	version = *fp++;
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
				printf("  Return address reg:    0x%02x\n", ret_addr);
				int aug_len = LEB(fp);
				if (aug_len) {
					printf("  Augmentation Length:   len=0x%02x ", aug_len);
					for (i = 0; i < aug_len; i++) {
						printf("%02x ", fp[i]);
					}
					printf("\n");
				}
				fp += aug_len;
				char *p;
				char	*a = fp;
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
//printf("R encoding %x\n", *a);
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
			dw->pc_begin = *(int32_t *) fp; fp += 4;
			dw->pc_end = *(uint32_t *) fp; fp += 4;
//printf("pc_begin=%p vad=%p %p\n", pc_begin, p_eh->p_vaddr, fp-eh_frame_data);
			if ((dw->fde_encoding & 0x70) == DW_EH_PE_pcrel) {
				dw->pc_begin += dw->eh_frame_sec->sh_addr;
				dw->pc_begin += fp - dw->eh_frame_data - 8;
			}
		        printf("\nFDE length=%08x ptr=%04x pc=%08lx..%08lx\n", 
				len, cie,
				dw->pc_begin, dw->pc_begin + dw->pc_end);
//printf("fde_encoding=%d\n", fde_encoding);
			if (*aug == 'z') {
				int aug_len = *fp++;
				if (aug_len) {
					printf("  Augmentation Length: 0x%02x ", aug_len);
					for (i = 0; i < aug_len; i++)
						printf("%02x ", fp[i]);
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
		if (pc < dw->pc_begin) {
			/***********************************************/
			/*   Something  wrong  ..  we  didnt find the  */
			/*   block.				       */
			/***********************************************/
			printf("block not found pc=%p begin=%p\n", pc, dw->pc_begin);
			return 0;
		}
		/***********************************************/
		/*   Not reached the right block yet.	       */
		/***********************************************/
		if (pc >= dw->pc_begin + dw->pc_end) {
printf("skip %p %p\n", pc, dw->pc_begin + dw->pc_end);
			fp = fp_start + len;
			continue;
		}

	        char *cp = fp; 
		char *cpend = fp_start + len;
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
				sprintf(msgbuf, "DW_CFA_advance_loc %d to %08lx",
					opa * dw->code_factor,
					dw->pc_begin + opa * dw->code_factor);
				dw->pc_begin += opa * dw->code_factor;
				break;

	                  case DW_CFA_advance_loc4:
			  	offset = *(int32_t *) cp; cp += 4;
				sprintf(msgbuf, "DW_CFA_advance_loc4 %d to %08lx",
					offset * dw->code_factor,
					dw->pc_begin + offset * dw->code_factor);
				dw->pc_begin += offset * dw->code_factor;
				break;

	                  case DW_CFA_def_cfa:
				a = LEB(cp);
				b = LEB(cp);
				sprintf(msgbuf, "DW_CFA_def_cfa: r%d ofs %d", a, b);
	                        break;
	                  case DW_CFA_def_cfa_offset:
				a = LEB(cp);
				sprintf(msgbuf, "DW_CFA_def_cfa_offset: %d", a);
	                        break;
	                  case DW_CFA_def_cfa_register:
				a = LEB(cp);
				sprintf(msgbuf, "DW_CFA_def_cfa_register: r%d", a);
	                        break;
	                  case DW_CFA_offset:
				a = LEB(cp);
				sprintf(msgbuf, "DW_CFA_offset: r%d at cfa%+d", opa, a * dw->data_factor);
	                        break;
	                  case DW_CFA_set_loc:
	                        sprintf(msgbuf, "DW_CFA_set_loc: ");
	                        cp = dump_ptr(cp, msgbuf + strlen(msgbuf));
	                        break;
			  case DW_CFA_nop:
	                        sprintf(msgbuf, "DW_CFA_nop");
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
printf("got it....\n");
return 1;
	}
	/***********************************************/
	/*   Nope - not here at all.		       */
	/***********************************************/
printf("not found at all....\n");
	return 0;
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
		int ret = dw_find_ret_addr(&dw, addr);
		printf("dw_find_ret_addr: returns %d\n", ret);
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
						printf("%02x ", *fp++);
					}
					printf("\n");
				}
				char *p;
				char	*a = fp;
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
//printf("R encoding %x\n", *a);
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
			dw.pc_begin = *(int32_t *) fp; fp += 4;
			dw.pc_end = *(uint32_t *) fp; fp += 4;
//printf("pc_begin=%p vad=%p %p\n", pc_begin, p_eh->p_vaddr, fp-eh_frame_data);
			if ((dw.fde_encoding & 0x70) == DW_EH_PE_pcrel) {
				dw.pc_begin += dw.eh_frame_sec->sh_addr;
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
				sprintf(msgbuf, "DW_CFA_advance_loc %d to %08lx",
					opa * dw.code_factor,
					dw.pc_begin + opa * dw.code_factor);
				dw.pc_begin += opa * dw.code_factor;
				break;

	                  case DW_CFA_advance_loc4:
			  	offset = *(int32_t *) cp; cp += 4;
				sprintf(msgbuf, "DW_CFA_advance_loc4 %d to %08lx",
					offset * dw.code_factor,
					dw.pc_begin + offset * dw.code_factor);
				dw.pc_begin += offset * dw.code_factor;
				break;

	                  case DW_CFA_def_cfa:
				a = LEB(cp);
				b = LEB(cp);
				sprintf(msgbuf, "DW_CFA_def_cfa: r%d ofs %d", a, b);
	                        break;
	                  case DW_CFA_def_cfa_offset:
				a = LEB(cp);
				sprintf(msgbuf, "DW_CFA_def_cfa_offset: %d", a);
	                        break;
	                  case DW_CFA_def_cfa_register:
				a = LEB(cp);
				sprintf(msgbuf, "DW_CFA_def_cfa_register: r%d", a);
	                        break;
	                  case DW_CFA_offset:
				a = LEB(cp);
				sprintf(msgbuf, "DW_CFA_offset: r%d at cfa%+d", opa, a * dw.data_factor);
	                        break;
	                  case DW_CFA_set_loc:
	                        sprintf(msgbuf, "DW_CFA_set_loc: ");
	                        cp = dump_ptr(cp, msgbuf + strlen(msgbuf));
	                        break;
			  case DW_CFA_nop:
	                        sprintf(msgbuf, "DW_CFA_nop");
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
static char *
dump_ptr(char *ptr, char *msgbuf)
{
        int     i;

        for (i = 0; i < 8; i++) {
                sprintf(msgbuf, "%02x", *ptr++ & 0xff);
		msgbuf += 2;
        }
        return ptr;
}
void
elferr(char *str)
{
        fprintf(stderr, "%s: %s\n", str, elf_errmsg(elf_errno()));
        exit(1);
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

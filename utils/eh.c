/*
Author : Paul Fox
Date:  July 2009

This is crap experimental code to understand DWARF debug records.

*/
# define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
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

static void dump_mem(char *mem, char *memend);
static char *dump_ptr(char *ptr);
void elferr(void);
int
get_leb128(char *fp)
{
	if (*fp & 0x40)
		return (-1 & ~0x7f) | *fp;
	return *fp;
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
int main(int argc, char **argv)
{       int     fd, i;
        Elf     *elf;
        char    *cp, *cpend, *mem;
        Elf64_Phdr *phdr;
        Elf64_Phdr *p_text = NULL;
        Elf64_Phdr *p_eh = NULL;
        Elf64_Ehdr *ehdr;
        dwarf_eh_frame_hdr *dhp;


        if ((fd = open(argv[1], O_RDONLY)) < 0) {
                perror(argv[1]);
                exit(1);
        }

        if (elf_version(EV_CURRENT) == EV_NONE) {
                printf("elf_version fails\n");
                exit(1);
        }
        elf = elf_begin(fd, ELF_C_READ, (Elf *)0);
        if (elf == 0)
                elferr();

        ehdr = elf64_getehdr(elf);
        if (ehdr == NULL)
                elferr();

        phdr = elf64_getphdr(elf);
        if (phdr == NULL)
                elferr();

        for (i = 0; i < ehdr->e_phnum; i++) {
                printf("%d: type=%x offset=%lx size=%lx\n", i, phdr->p_type, phdr->p_offset, phdr->p_memsz);
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

        /***********************************************/
        /*   Find the .eh_frame section.               */
        /***********************************************/
        Elf_Scn *scn = elf_getscn(elf, ehdr->e_shstrndx);
        Elf_Data *edata;
        if ((edata = elf_getdata(scn, NULL)) == NULL) {
                elferr();
        }
        char *data = edata->d_buf;
        unsigned long eh_frame_addr;
        unsigned long eh_frame_size;
        char    *eh_frame_data = NULL;
        for (i = 0; i < ehdr->e_shnum; i++) {
                Elf_Scn *scn = elf_getscn(elf, i);
                Elf64_Shdr *shdr = elf64_getshdr(scn);
                char *name = data + shdr->sh_name;
                printf("  sec %d: %s\n", i, name);
                if (strcmp(name, ".eh_frame") == 0) {
                        eh_frame_addr = shdr->sh_offset;
                        eh_frame_size = shdr->sh_size;
                        if ((edata = elf_getdata(scn, NULL)) == NULL)
                                elferr();
                        eh_frame_data = edata->d_buf;
                        }
        }

        mem = malloc(p_eh->p_memsz);
        lseek(fd, p_eh->p_offset, SEEK_SET);
        if (read(fd, mem, p_eh->p_memsz) != p_eh->p_memsz) {
                fprintf(stderr, "Failed to read %ld bytes from file\n", p_eh->p_memsz);
                exit(1);
        }

	dump_mem(mem, mem + p_eh->p_memsz);

        dhp = (dwarf_eh_frame_hdr *) mem;
        printf("DWARF.version:          %02x\n", dhp->version);
        printf("DWARF.eh_frame_ptr_enc: %02x\n", dhp->eh_frame_ptr_enc);
        printf("DWARF.fde_count_enc:    %02x\n", dhp->fde_count_enc);
        printf("DWARF.table_enc:        %02x\n", dhp->table_enc);

        int frame_ptr_offset;
        char *ptr = dhp + 1;
        dwarf_read_pointer(&ptr, &frame_ptr_offset, dhp->eh_frame_ptr_enc);
        printf("frame_ptr: %08x\n", frame_ptr_offset);
        char *frame_ptr = (ptr - 4) + frame_ptr_offset;

        int fde_count;
        dwarf_read_pointer(&ptr, &fde_count, dhp->eh_frame_ptr_enc);
        printf("fde_count: %08x\n", fde_count);

        if (dhp->table_enc != (DW_EH_PE_datarel | DW_EH_PE_sdata4)) {
                printf("Unexpected table_enc - giving up\n");
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
        ftable_t *table = ptr;
        printf("table=%p\n", table);
        for (i = 0; i < fde_count; i++) {
                printf(" %d: %04x %04x\n", i, table[i].t_ip, table[i].t_frame_offset);
        }

        printf("=== .eh_frame\n");
        char *fp = eh_frame_data;
        int len = *(uint32_t *) fp; fp += 4;
        printf("  Length: %08x\n", len);
        printf("  CIE: %08x\n", *(uint32_t *) fp); fp+= 4;
        printf("  Version: %02x\n", *fp++);
	char *aug = fp;
        printf("  Augmentation string: '");
	while (*fp) {
		printf("%c", *fp++);
	}
	printf("'\n");
	fp++;
        printf("  Code alignment factor: %d\n", get_leb128(fp));
	fp++;
	int data_factor = get_leb128(fp);
        printf("  Data alignment factor: %d\n", data_factor);
	fp++;
        printf("  Return address reg: 0x%02x\n", *fp++);
	if (strchr(aug, 'z')) {
		printf("Augmentation Length: 0x%02x ", *fp++);
		for (i = fp[-1]; i-- > 0; )
			printf("%02x ", *fp++);
		printf("\n");
	}
	dump_mem(fp, fp +32);

#define LEB(cp) get_leb128(cp); cp++

        cp = fp; cpend = cp + 32;
        while (cp < cpend) {
		int	a, b;
		int	op;
		int	opa;

                printf("%04x: %02x: ", cp - fp, *cp & 0xff);
		op = *cp++;
		opa = op & 0x3f;
		if (op & 0xc0)
			op &= 0xc0;

                switch (op) {
                  case DW_CFA_def_cfa:
			a = LEB(cp);
			b = LEB(cp);
			printf("DW_CFA_def_cfa: r%d ofs %d", a, b);
                        break;
                  case DW_CFA_offset:
			a = LEB(cp);
			printf("DW_CFA_offset: r%d at cfa%+d", opa, a * data_factor);
                        break;
                  case DW_CFA_set_loc:
                        printf("DW_CFA_set_loc: ");
                        cp = dump_ptr(cp);
                        break;
		  case DW_CFA_nop:
                        printf("DW_CFA_nop");
		  	break;
                  }
                printf("\n");
        }
}
static void
dump_mem(char *mem, char *memend)
{	char	*cp;
	int	i;

        for (cp = mem; cp < memend; cp += 16) {
                printf("%04x: ", cp - mem);
                for (i = 0; i < 16 && cp + i < memend; i++) {
                        printf("%02x ", cp[i] & 0xff);
                }
                printf("\n");
        }
}
static char *
dump_ptr(char *ptr)
{
        int     i;

        for (i = 0; i < 8; i++) {
                printf("%02x", *ptr++ & 0xff);
        }
        return ptr;
}
void
elferr()
{
        fprintf(stderr, "%s\n", elf_errmsg(elf_errno()));
        exit(1);
}

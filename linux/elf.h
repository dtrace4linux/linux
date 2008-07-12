/**********************************************************************/
/*   Linux  and Sun elf include files are complicated and different,  */
/*   having  to  both support 32 and 64 bit includes and provide the  */
/*   gelf.h wrapper.						      */
/*   								      */
/*   Solaris doesnt seem to allow for a 32-bit binary which can read  */
/*   64-bit  ELF  files. We dont care about this on Linux, but we do  */
/*   want to build dtrace for 32-bit kernels.			      */
/**********************************************************************/

# if !defined(ELF_H)
# define ELF_H 1

# include <sys/elf.h>
# include <link.h>

typedef struct {
  uint64_t a_type;
  union {
      uint64_t a_val;
    } a_un;
} Elf64_auxv_t;

typedef struct
{
  Elf64_Word l_name;		/* Name (string table index) */
  Elf64_Word l_time_stamp;	/* Timestamp */
  Elf64_Word l_checksum;	/* Checksum */
  Elf64_Word l_version;		/* Interface version */
  Elf64_Word l_flags;		/* Flags */
} Elf64_Lib;

typedef uint16_t Elf64_Section;
typedef Elf64_Syminfo GElf_Syminfo;
typedef Elf64_auxv_t GElf_auxv_t;

# endif /* !defined(ELF_H) */

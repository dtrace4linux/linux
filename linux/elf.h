# if !defined(ELF_H)
# define	ELF_H	1

# define  EM_AMD64        62
# define  SHT_SUNW_dof            0x6ffffff4
# define  SHN_SUNW_IGNORE 0xff3f
# define PF_SUNW_FAILURE 0x00100000      /* mapping absent due to failure */

# include <sys/elf_amd64.h>
# include "/usr/include/elf.h"
# endif

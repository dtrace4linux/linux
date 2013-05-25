/**********************************************************************/
/*   Simple wrapper to shield from some Linux isms.		      */
/**********************************************************************/

# if !defined(LINUX_H_INCLUDE)
# define LINUX_H_INCLUDE

#include <stdint.h>
#include <build/port.h>

# if HAVE_LIB_LIBDW
#	define	dwarf_dealloc(a, b, c)
#	define	dwarf_finish(a, b) dwarf_end(a)
#	define	dwarf_next_cu_header(dw, hdrlen, vers, off, addrsz, nxthdr, err) \
		dwarf_nextcu(dw, off, nxthdr, addrsz, hdrlen, off, addrsz)

# endif

# endif


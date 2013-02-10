/**********************************************************************/
/*   Trampoline to correct C code.				      */
/**********************************************************************/
# if defined(__i386)
# include "../i386/Pisadep.c"
# elif defined(__amd64)
# include "../amd64/Pisadep.c"
# elif defined(__arm__)
# include "../arm/Pisadep.c"
# else
#	error "pisdaep.c: cannot compile for this CPU"
# endif

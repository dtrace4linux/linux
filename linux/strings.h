/**********************************************************************/
/*   Wrapper to correct for brokenness on some glibc systems.	      */
/**********************************************************************/
//# undef __USE_BSD /* Avoid bzero() being defined on glibc 2.7 */
# include <string.h>


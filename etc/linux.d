/**********************************************************************/
/*   Definitions for the Linux platform.			      */
/*   $Header: Last edited: 06-Aug-2010 1.1 $ 			      */
/**********************************************************************/

/**********************************************************************/
/*   Normally  this  would  be in the CTF ELF section of the kernel,  */
/*   but  we  cannot  do that, but we can do it here instead. Allows  */
/*   use of "uid" in probe functions.				      */
/**********************************************************************/
typedef int uid_t;


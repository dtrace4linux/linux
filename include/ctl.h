/**********************************************************************/
/*   Driver  for  allowing direct access to process memory - similar  */
/*   to /proc/pid/mem, but bypassing various restrictions which mean  */
/*   that  dtrace  can  have trouble ptrace() attaching to a process  */
/*   which is being debugged.					      */
/**********************************************************************/

#if !defined(_CTL_H_INCLUDE)
# define _CTL_H_INCLUDE

/**********************************************************************/
/*   ioctl values and structures for /dev/dtrace_ctl		      */
/**********************************************************************/
#define	CTLIOC	(('c' << 24) | ('t' << 16) | ('l' << 8))
#define CTLIOC_RDMEM	(CTLIOC | 1)
#define CTLIOC_WRMEM	(CTLIOC | 2)
typedef struct ctl_mem_t {
	int	c_pid;
	void	*c_src;
	void	*c_dst;
	int	c_len;
	} ctl_mem_t;

#endif /* _CTL_H_INCLUDE */

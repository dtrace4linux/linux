/**********************************************************************/
/*   Functions which are not there in the older kernels.	      */
/*   Open Source						      */
/*   Author: P D Fox						      */
/**********************************************************************/

#include "dtrace_linux.h"

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
# define toupper(x) ((x) >= 'a' && (x) <= 'z' ? (x) - 0x20 : (x))
# define tolower(x) ((x) >= 'A' && (x) <= 'Z' ? (x) + 0x20 : (x))

int
strcasecmp(char *s1, char *s2)
{
	while (*s1 && *s2) {
		int	ch1 = *s1++;
		int	ch2 = *s2++;
		int	n = toupper(ch1) - toupper(ch2);
		if (n)
			return n < 0 ? -1 : 1;
		}
	if (*s1)
		return 1;
	if (*s2)
		return -1;
	return 0;
}
int
find_task_by_vpid(int n)
{
	return find_task_by_pid(n);
}
int
mutex_is_locked(struct mutex *mp)
{
	return atomic_read(&mp->count) != 1;
}
void
smp_call_function_single(int cpuid, int (*func)(void *), void *info, int wait)
{
        local_irq_disable();
        (*func)(info);
        local_irq_enable();
}
# endif

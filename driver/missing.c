/**********************************************************************/
/*   Functions which are not there in the older kernels.	      */
/*   Open Source						      */
/*   Author: P D Fox						      */
/* $Header: Last edited: 26-Jul-2012 1.6 $ 			      */
/**********************************************************************/

#include "dtrace_linux.h"

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 21)
# define toupper(x) ((x) >= 'a' && (x) <= 'z' ? (x) - 0x20 : (x))
# define tolower(x) ((x) >= 'A' && (x) <= 'Z' ? (x) + 0x20 : (x))

int
strcasecmp(const char *s1, const char *s2)
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
# endif

/**********************************************************************/
/*   Make  sure  we  never use the kernels string functions here, in  */
/*   case we are probing them.					      */
/**********************************************************************/
#if !defined(__HAVE_ARCH_STRLEN)
size_t
strlen(const char *str)
{
	const char *str1 = str;

	while (*str1)
		str1++;
	return str1 - str;
}
#endif

#if !defined(__HAVE_ARCH_STRCMP)
int
strncmp(const char *s1, const char *s2, size_t len)
{
	while (len-- > 0) {
		int	c1 = *s1++;
		int	c2 = *s2++;
		if (c1 == c2 && c1 == 0)
			return 0;
		if (c1 < c2)
			return -1;
		if (c1 > c2)
			return 1;
	}
	return 0;
}
#endif

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
struct task_struct *
find_task_by_vpid(int n)
{
# if defined(PIDTYPE_PID)
	return find_task_by_pid_type(PIDTYPE_PID, n);
# else
	return find_task_by_pid(n);
# endif
}
# endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 16)
/**********************************************************************/
/*   Simple  bubble  sort  replacement  for  old  kernels. Called by  */
/*   fasttrap_isa.c						      */
/**********************************************************************/
void sort(void *base, size_t num, size_t size,
          int (*cmp)(const void *, const void *),
          void (*swap)(void *, void *, int))
{	int	i, j;

	for (i = 0; i < num; i++) {
		for (j = i + 1; j < num; j++) {
			int r = cmp(&base[i * size], &base[j * size]);
			if (r > 0)
				swap(&base[i * size], &base[j * size], size);
		}
	}
}
#endif

# if defined(FUNC_SMP_CALL_FUNCTION_SINGLE_MISSING)
/**********************************************************************/
/*   If only one cpu, then it must be us who gets called.	      */
/**********************************************************************/
int
smp_call_function_single(int cpuid, void (*func)(void *), void *info, int wait)
{
        local_irq_disable();
        (*func)(info);
        local_irq_enable();
	return 0;
}
# endif

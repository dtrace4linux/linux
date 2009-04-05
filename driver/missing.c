/**********************************************************************/
/*   Functions which are not there in the older kernels.	      */
/*   Open Source						      */
/*   Author: P D Fox						      */
/* $Header: Last edited: 06-Apr-2009 1.2 $ 			      */
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
size_t
strlen(const char *str)
{
	const char *str1 = str;

	while (*str1)
		str1++;
	return str1 - str;
}
int
strncmp(const char *s1, const char *s2, size_t len)
{
	while (len-- > 0) {
		int ch = *s2++ - *s1++;
		if (ch)
			return ch;
	}
	return 0;
}
# endif

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
int
find_task_by_vpid(int n)
{
	return find_task_by_pid(n);
}
# endif

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
int
mutex_is_locked(struct mutex *mp)
{
	return atomic_read(&mp->count) != 1;
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

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
void
smp_call_function_single(int cpuid, int (*func)(void *), void *info, int wait)
{
        local_irq_disable();
        (*func)(info);
        local_irq_enable();
}
# endif

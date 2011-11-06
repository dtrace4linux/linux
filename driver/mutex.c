#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>

#define MAX_ITER (500 * 1000 * 1000)
static int	stop_mutex;

unsigned long cnt_mtx1;
unsigned long cnt_mtx2;

void
dmutex_init(mutex_t *mp)
{
	mp->count = 0;
}
void
dmutex_enter(mutex_t *mp)
{	unsigned long cnt = 0;
	int	cookie;

	if (stop_mutex)
		return;

	cookie = dtrace_interrupt_disable();
	while (dtrace_casptr(&mp->count, 0, 1) == 1) {
		if (mp->cpu == smp_processor_id())
			cnt_mtx1++;
		if (cnt++ == MAX_ITER) {
			cnt_mtx2++;
			printk("dmutex_enter: looping\n");
			stop_mutex = 1;
			dtrace_panic("dmutex_enter");
			break;
		}
	}
	mp->cpu = smp_processor_id();
	dtrace_interrupt_enable(cookie);
}

void
dmutex_exit(mutex_t *mp)
{	unsigned long cnt = 0;
	int	cookie;

	if (stop_mutex)
		return;

	cookie = dtrace_interrupt_disable();
	while (dtrace_casptr(&mp->count, 1, 0) == 0) {
		if (cnt++ == MAX_ITER) {
			printk("dmutex_exit: looping\n");
			stop_mutex = 1;
			dtrace_panic("dmutex_exit");
			break;
		}
	}
	dtrace_interrupt_enable(cookie);
}

int
dmutex_is_locked(mutex_t *mp)
{
	return mp->count;
}


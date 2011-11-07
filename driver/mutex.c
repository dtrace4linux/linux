#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>

unsigned long cnt_mtx1;
unsigned long cnt_mtx2;

void
dmutex_init(mutex_t *mp)
{
	memset(mp, 0, sizeof mp);
	sema_init(&mp->m_sem, 1);
	mp->m_initted = TRUE;
}
void
dmutex_enter(mutex_t *mp)
{
	if (!mp->m_initted)
		dmutex_init(mp);
	down(&mp->m_sem);
	preempt_disable();
}

void
dmutex_exit(mutex_t *mp)
{
	preempt_enable();
	up(&mp->m_sem);
}

int
dmutex_is_locked(mutex_t *mp)
{
# if defined(HAVE_SEMAPHORE_ATOMIC_COUNT)
	return atomic_read(&mp->m_sem.count) == 0;
# else
	return mp->m_sem.count == 0;
#endif	
}


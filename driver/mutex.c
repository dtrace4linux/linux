/**********************************************************************/
/*                                                                    */
/*  File:          mutex.c                                            */
/*  Author:        P. D. Fox                                          */
/*  Created:       13 Nov 2011                                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  Description:  Mutex implementations                               */
/*   We  support  two  styles  of mutexes: those called from the top  */
/*   half  of the kernel, and run with interrupts enabled, and those  */
/*   called from probe context.					      */
/*   								      */
/*   Upper  half  mutexes  are  accessed via mutex_enter/mutex_exit.  */
/*   Lower half via dmutex_enter/dmutex_exit.			      */
/*   								      */
/*   The  same  mutex  may be called by either function, but must be  */
/*   limited to the appropriate half of the kernel.		      */
/*   								      */
/*   Without  this,  we  may  end  up  causing kernel diagnostics or  */
/*   deadlock.   mutex_enter   will  grab  the  mutex,  and  disable  */
/*   interrupts during the grab, but re-enable after the grab.	      */
/*   								      */
/*   dmutex_enter will leave interrupts disabled.		      */
/*   								      */
/*   We  need  to avoid the spin locks and semaphores/mutexes of the  */
/*   kernel,   because   they   do   preemption   advice,   and  the  */
/*   WARN_ON/BUG_ON  macros in the kernel can fire, e.g. on kmalloc,  */
/*   when allocating memory if the irqs_disabled() function disagree  */
/*   with the allocation flags.					      */
/*--------------------------------------------------------------------*/
/*  $Header: Last edited: 18-Nov-2011 1.2 $ 			      */
/**********************************************************************/

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <dtrace_proto.h>

unsigned long cnt_mtx1;
unsigned long cnt_mtx2;
unsigned long cnt_mtx3;

void
dmutex_init(mutex_t *mp)
{
	memset(mp, 0, sizeof *mp);
	sema_init(&mp->m_sem, 1);
	mp->m_initted = TRUE;
}

void
mutex_enter_common(mutex_t *mp, int dflag)
{       unsigned long flags;
        unsigned long cnt;

        if (!mp->m_initted)
                dmutex_init(mp);

        /***********************************************/
        /*   Check for recursive mutex.                */
        /***********************************************/
        if (mp->m_count && mp->m_cpu == smp_processor_id()) {
dtrace_printf("%p mutex recursive, dflag=%d %d\n", mp, dflag, mp->m_type);
                mp->m_level++;
                return;
                }

        if (dflag)
                flags = dtrace_interrupt_disable();
	else
		flags = dtrace_interrupt_get();

        for (cnt = 0; dtrace_casptr(&mp->m_count, 0, (void *) 1) == (void *) 1; ) {
                if (cnt++ > 10 * 1000 * 1000) {
			dtrace_printf("mutex_enter: breaking out of stuck loop cpu=%\n", smp_processor_id());
                        dtrace_interrupt_enable(flags);
                        cnt_mtx3++;
			break;
		}
	}
	mp->m_flags = flags;
	mp->m_cpu = smp_processor_id();
	mp->m_level = 1;
	mp->m_type = dflag;
}
void
dmutex_enter(mutex_t *mp)
{
	cnt_mtx1++;
	mutex_enter_common(mp, TRUE);
}
void
mutex_enter(mutex_t *mp)
{
mutex_t imp = *mp;

	if (mp->m_count && mp->m_type) {
		dtrace_printf("%p mutex...fail in mutex_enter count=%d type=%d\n", mp, mp->m_count, mp->m_type);
	}

	cnt_mtx2++;
	mutex_enter_common(mp, FALSE);
	if (irqs_disabled()) {
		dtrace_printf("%p: mutex_enter with irqs disabled fl:%lx level:%d cpu:%d\n", 
			mp, mp->m_flags, mp->m_level, mp->m_cpu);
		dtrace_printf("orig: init=%d fl:%lx cpu:%d\n", imp.m_initted, imp.m_flags, imp.m_cpu);
	}
}

void
dmutex_exit(mutex_t *mp)
{
	if (--mp->m_level)
		return;

	dtrace_interrupt_enable(mp->m_flags);
	mp->m_count = 0;
}

void
mutex_exit(mutex_t *mp)
{
	if (--mp->m_level)
		return;

	mp->m_count = 0;
}

int
dmutex_is_locked(mutex_t *mp)
{
	return mp->m_count != NULL;

/*# if defined(HAVE_SEMAPHORE_ATOMIC_COUNT)
	return atomic_read(&mp->m_sem.count) == 0;
# else
	return mp->m_sem.count == 0;
#endif	
*/
}

/**********************************************************************/
/*   Utility for debugging - print the state of the mutex.	      */
/**********************************************************************/
void
mutex_dump(mutex_t *mp)
{
	dtrace_printf("mutex: %p initted=%d count=%p flags=%lx cpu=%d type=%d level=%d\n",
		mp, mp->m_initted, mp->m_count, mp->m_flags, mp->m_cpu, mp->m_type, 
		mp->m_level);
}

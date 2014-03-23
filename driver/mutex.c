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
/*  $Header: Last edited: 04-Dec-2011 1.3 $ 			      */
/**********************************************************************/

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <dtrace_proto.h>

unsigned long cnt_mtx1;
unsigned long cnt_mtx2;
unsigned long cnt_mtx3;

static const int disable_ints;

void
dmutex_init(mutex_t *mp)
{
	/***********************************************/
	/*   Linux  changed  from MUTEX to SEMAPHORE.  */
	/*   We  want  a very low cost mutex - mainly  */
	/*   so  we  can avoid reentrancy issues when  */
	/*   placing  probes  - we dont do many mutex  */
	/*   operations,  but  they  do  have to work  */
	/*   reliably when doing xcalls.	       */
	/***********************************************/
#if defined(DEFINE_SEMAPHORE)
static DEFINE_SEMAPHORE(null_sema);
#else
static DECLARE_MUTEX(null_sema);
#endif

	memset(mp, 0, sizeof *mp);
	mp->m_sem = null_sema;
//	sema_init(&mp->m_sem, 1);
	mp->m_initted = TRUE;
}

/**********************************************************************/
/*   Do the work of acquiring and blocking on a mutex. "mutex_enter"  */
/*   is  for  normal  upper-layer  code,  e.g.  the ioctl(), whereas  */
/*   "dmutex_enter" is for interrupt level code.		      */
/*   								      */
/*   We  could  probably  coalesce  the functions and use the kernel  */
/*   irqs_disabled()  and  hard_irq_count()  functions, but we dont,  */
/*   for now.							      */
/**********************************************************************/
void
mutex_enter_common(mutex_t *mp, int dflag)
{       unsigned long flags;
	unsigned int  cnt;

	if (!mp->m_initted) {
		/***********************************************/
		/*   Special  debug:  detect  a dynamic mutex  */
		/*   being  used  (one  coming from a kmalloc  */
		/*   type block of memory), vs the statically  */
		/*   defined ones).			       */
		/***********************************************/
		if (mp->m_initted != 2) {
			dtrace_printf("initting a mutex\n");
			dump_stack();
		}
	    dmutex_init(mp);
	    }

	/***********************************************/
	/*   Check  for  recursive  mutex.  Theres  a  */
	/*   number of scenarios.		       */
	/*   					       */
	/*   Non-intr followed by an intr: we have to  */
	/*   allow the intr.			       */
	/*   					       */
	/*   Non-intr  followed  by  non-intr: normal  */
	/*   recursive mutex.			       */
	/*   					       */
	/*   Intr   followed  by  an  intr:  shouldnt  */
	/*   happen.				       */
	/*   					       */
	/*   We  mustnt allow us to be put on another  */
	/*   cpu,  else  we  will lose track of which  */
	/*   cpu has the mutex.			       */
	/*   					       */
	/*   Now  that  the mutex code is working, we  */
	/*   mustnt  allow  recursive  mutexes.  This  */
	/*   causes  problems  for  two  dtrace  user  */
	/*   space  apps  running  at  the same time.  */
	/*   Turn  off  for  now.  Later  on,  we can  */
	/*   delete the code below.		       */
	/***********************************************/
	if (0 && mp->m_count && mp->m_cpu == smp_processor_id()) {
		static int x;
		if (x++ < 4 || (x < 1000000 && (x % 5000) == 0))
		    dtrace_printf("%p mutex recursive, dflag=%d %d [%d]\n", mp, dflag, mp->m_type, x);
		mp->m_level++;
		return;
	}

	/***********************************************/
	/*   We  grab  the  current interrupt state -  */
	/*   purely   to   help  with  debugging  and  */
	/*   tracing.  We  run  without  touching the  */
	/*   interrupts, in the normal case.	       */
	/***********************************************/
	if (disable_ints && dflag) {
		/***********************************************/
		/*   This  is  not  normally  executed  - its  */
		/*   compiled  out.  We  do  this  for  debug  */
		/*   scenarios when the const disable_ints is  */
		/*   hand set to non-zero.		       */
		/***********************************************/
		flags = dtrace_interrupt_disable();
	} else {
		flags = dtrace_interrupt_get();
	}

	for (cnt = 0; dtrace_casptr(&mp->m_count, 0, (void *) 1) == (void *) 1; ) {
		/***********************************************/
		/*   We  are  waiting  for  the lock. Someone  */
		/*   else  has  it.  Someone  else  might  be  */
		/*   waiting  for us (xcall), so occasionally  */
		/*   empty the xcall queue for us.	       */
		/***********************************************/
		if ((cnt++ % 100) == 0)
			xcall_slave2();

		/***********************************************/
		/*   If  we  are running in the upper half of  */
		/*   the   kernel,   periodically   let   the  */
		/*   scheduler  run,  to  avoid deadlock when  */
		/*   running N+1 copies of dtrace on an N CPU  */
		/*   system.				       */
		/***********************************************/
		if (/*!dflag &&*/ (cnt % 2000) == 0)
			schedule();

		/***********************************************/
		/*   If  we  start locking up the kernel, let  */
		/*   user  know  something  bad is happening.  */
		/*   Probably  pointless  if mutex is working  */
		/*   correctly.				       */
		/***********************************************/
		if ((cnt % (500 * 1000 * 1000)) == 0) {
			dtrace_printf("mutex_enter: taking a long time to grab lock mtx3=%llu\n", cnt_mtx3);
			cnt_mtx3++;
		}
	}
//preempt_disable();
	mp->m_flags = flags;
	mp->m_cpu = smp_processor_id();
	mp->m_level = 1;
	mp->m_type = dflag;
}

/**********************************************************************/
/*   Enter from interrupt context, interrupts might be disabled.      */
/**********************************************************************/
void
dmutex_enter(mutex_t *mp)
{
	cnt_mtx1++;
	mutex_enter_common(mp, TRUE);
}

/**********************************************************************/
/*   Enter  from  the  upper-level  of  the  kernel, with interrupts  */
/*   enabled.							      */
/**********************************************************************/
void
mutex_enter(mutex_t *mp)
{
	mutex_t imp = *mp;
/*static int f;
if (f++ == 70) {
	int c = smp_processor_id();
	unsigned long x = 0;
	for (x = 0; x < 4000000000UL; x++) {
		if (c != smp_processor_id())
			break;
	}
	dtrace_printf("FIRST CPU SW: %d x=%lu\n", c, x);

}*/
	/***********************************************/
	/*   Try  and  detect a nested call from this  */
	/*   cpu whilst the mutex is held.	       */
	/***********************************************/
	if (mp->m_count && mp->m_type && mp->m_cpu == smp_processor_id()) {
		dtrace_printf("%p mutex...fail in mutex_enter count=%d type=%d\n", mp, mp->m_count, mp->m_type);
	}

	cnt_mtx2++;
	mutex_enter_common(mp, FALSE);
	if (disable_ints && irqs_disabled()) {
		dtrace_printf("%p: mutex_enter with irqs disabled fl:%lx level:%d cpu:%d\n",
		    mp, mp->m_flags, mp->m_level, mp->m_cpu);
		dtrace_printf("orig: init=%d fl:%lx cpu:%d\n", imp.m_initted, imp.m_flags, imp.m_cpu);
	}
}

/**********************************************************************/
/*   Release mutex, called by interrupt context.		      */
/**********************************************************************/
void
dmutex_exit(mutex_t *mp)
{	unsigned long fl = mp->m_flags;

	if (--mp->m_level)
	    return;

	/*
	if (mp->m_cpu != smp_processor_id())
		dtrace_printf("dmutex_exit:: cross cpu %d count=%d\n", mp->m_cpu, mp->m_count);
	*/

	mp->m_count = 0;
	if (disable_ints)
		dtrace_interrupt_enable(fl);
//preempt_enable_no_resched();
}

/**********************************************************************/
/*   Release mutex, called by upper-half of kernel.		      */
/**********************************************************************/
void
mutex_exit(mutex_t *mp)
{
	if (--mp->m_level)
	    return;


	/*
	if (mp->m_cpu != smp_processor_id()) {
	static int xx;
		dtrace_printf("mutex_exit:: cross cpu %d\n", mp->m_cpu);
	if (xx++ < 5)
		dump_stack();
	}
	preempt_enable_no_resched();
	*/

	mp->m_count = 0;
}

/**********************************************************************/
/*   Used by the assertion code MUTEX_LOCKED in dtrace.c	      */
/**********************************************************************/
int
dmutex_is_locked(mutex_t *mp)
{
	return mp->m_count != NULL;
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

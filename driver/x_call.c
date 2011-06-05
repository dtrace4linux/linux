/**********************************************************************/
/*   Code to implement cross-cpu calling for Dtrace.		      */
/*   Author: Paul D. Fox					      */
/*   Date: May 2011						      */
/**********************************************************************/
#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"

#include <linux/cpumask.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sys.h>
#include <linux/thread_info.h>
#include <linux/smp.h>
#include <asm/smp.h>
#include <linux/vmalloc.h>
#include <asm/current.h>
#include <asm/desc.h>
#include <asm/hardirq.h>

/**********************************************************************/
/*   Set to true to enable debugging.				      */
/**********************************************************************/
int	xcall_debug = 0;

extern int nr_cpus;
extern int driver_initted;
extern int ipi_vector;

/**********************************************************************/
/*   Let us flip between the new and old code easily.		      */
/**********************************************************************/
# define	XCALL_MODE	XCALL_ORIG
# define	XCALL_ORIG	0
# define	XCALL_NEW	1

/**********************************************************************/
/*   Spinlock  to avoid dtrace_xcall having issues whilst queuing up  */
/*   a request.							      */
/**********************************************************************/
spinlock_t xcall_spinlock;

#define smt_pause() asm("pause\n")

/**********************************************************************/
/*   Code  to implement inter-cpu synchronisation. Sometimes, dtrace  */
/*   needs to ensure the other CPUs are in sync, e.g. because we are  */
/*   about  to  affect  global  state  for a user dtrace process. We  */
/*   cannot  do directly what Solaris does because Linux wont let us  */
/*   use smp_call_function() and friends from interrupt context. The  */
/*   timer interrupt will cause this to happen.			      */
/*   								      */
/*   So,  instead  we  rely on being able to fire a timer on another  */
/*   cpu.							      */
/*   								      */
/*   The  code  below is similar in style to the XC code in Solaris,  */
/*   and we keep some stats so we can see if this is "good-enough".   */
/**********************************************************************/
# define XC_SYNC_OP	1
# define XC_DONE	2
static struct xcalls {
	dtrace_xcall_t	xc_func;
	void		*xc_arg;
	volatile int	xc_ack;
	volatile int	xc_wait;
	volatile char	xc_state;
	} xcalls[NCPU];
static int xcall_levels[NCPU];

unsigned long cnt_xcall1;
unsigned long cnt_xcall2;
unsigned long cnt_xcall3;
unsigned long cnt_xcall4;
unsigned long cnt_xcall5;	/* Max ack_wait/spin loop */
unsigned long cnt_ipi1;

void orig_dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg);
void dtrace_xcall1(processorid_t cpu, dtrace_xcall_t func, void *arg);

/**********************************************************************/
/*   Switch the interface from the orig dtrace_xcall code to the new  */
/*   code.  As  of 20110520, I cannot decide which one is worse. The  */
/*   orig  code  cannot  come  from a timer interrupt (hr_timer code  */
/*   comes  here),  but  the  new  code  is  suffering  from dropped  */
/*   interprocessor  interrupts  signifying  I  dont  know what I am  */
/*   doing :-(							      */
/**********************************************************************/
void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	/***********************************************/
	/*   If  we  arent ready for xcalls yet, then  */
	/*   do  it  the old race-condition way. This  */
	/*   will happen during/after driver load til  */
	/*   we  get  the  symbol patchups. Its not a  */
	/*   problem for this scenario, and we should  */
	/*   resolve     the    dtrace_attach()    vs  */
	/*   dtrace_linux_init()  relative startup so  */
	/*   we can kill the older code.	       */
	/***********************************************/
	if (!driver_initted || XCALL_MODE == XCALL_ORIG) {
		orig_dtrace_xcall(cpu, func, arg);
		return;
	}

#if XCALL_MODE == XCALL_NEW
	dtrace_xcall1(cpu, func, arg);
#endif
}

/**********************************************************************/
/*   This  code  is similar to the Solaris original - except we rely  */
/*   on  smp_call_function, and that wont work from an interrupt. We  */
/*   temporarily  use this during driver initialisation and/or if we  */
/*   decide at compile time to use this.			      */
/**********************************************************************/
void
orig_dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
//printk("orig_dtrace_xcall %lu\n", cnt_xcall1);
	cnt_xcall1++;

	if (cpu == DTRACE_CPUALL) {
		cnt_xcall2++;
		/***********************************************/
		/*   Avoid  calling  local_irq_disable, since  */
		/*   we   will  likely  be  called  from  the  */
		/*   hrtimer callback.			       */
		/***********************************************/
		preempt_disable();
		SMP_CALL_FUNCTION(func, arg, TRUE);
		func(arg);
		preempt_enable();
	} else {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 26)
		/***********************************************/
		/*   20090710   Special  case  where  we  are  */
		/*   trying    to   call   ourselves,   since  */
		/*   smp_call_function_single  doesnt like us  */
		/*   doing    this.    Patch    provided   by  */
		/*   Nicolas.Williams@sun.com		       */
		/***********************************************/
		int me = get_cpu();

		put_cpu();

		if (me == cpu) {
			local_irq_disable();
			func(arg);
			local_irq_enable();
			return;
		}
		SMP_CALL_FUNCTION_SINGLE(cpu, func, arg, TRUE);
#else
		SMP_CALL_FUNCTION_SINGLE(cpu, func, arg, TRUE);
#endif
	}
}

# if XCALL_MODE == XCALL_NEW
/**********************************************************************/
/*   Wait  for  the other cpu to get to an appropriate point. We put  */
/*   in  counters  to  avoid  deadlocking ourselves - these shouldnt  */
/*   happen under normal circumstances, but they can happen, e.g. if  */
/*   other  cpu  is  in  a big disabled-interrupt mode. We dont like  */
/*   breaking the locking protocol, but remember the xcalls are rare  */
/*   -  typically  as  dtrace  is  starting up or shutting down (see  */
/*   /proc/dtrace/stats  to  see  the  numbers  relative  to  actual  */
/*   probes).							      */
/**********************************************************************/
static void
ack_wait(int c, char *msg)
{	unsigned long cnt = 0;
	int	cnt1 = 0;
	volatile struct xcalls *xc = &xcalls[c];

	while (xc->xc_ack == 0) {
		/***********************************************/
		/*   Be HT friendly.			       */
		/***********************************************/
		smt_pause();

		/***********************************************/
		/*   Keep track of the max.		       */
		/***********************************************/
		if (cnt > cnt_xcall5)
			cnt_xcall5 = cnt;

		/***********************************************/
		/*   On  my  Dual Core 2.26GHz system, we are  */
		/*   seeing counters in the range of hundreds  */
		/*   to  maybe  2,000,000  for  more  extreme  */
		/*   cases.  (This  is  inside  a VM). During  */
		/*   debugging,  we  found  problems with the  */
		/*   two  cores  not  seeing  each  other  --  */
		/*   possibly because I wasnt doing the right  */
		/*   things to ensure memory barriers were in  */
		/*   place.				       */
		/*   					       */
		/*   We  dont  want  to  wait forever because  */
		/*   that  will  crash/hang your machine, but  */
		/*   we  do  need to give up if its taken far  */
		/*   too long.				       */
		/***********************************************/
		if (cnt++ == 50 * 1000 * 1000UL) {
			cnt = 0;
			cnt_xcall4++;

			/***********************************************/
			/*   Repost the IPI if it didnt respond quick  */
			/*   enough.  This  is a dirty hack - because  */
			/*   we shouldnt lose the interrupts.	       */
			/***********************************************/
			if (1) {
			struct cpumask mask;
			cpumask_clear(&mask);
			cpumask_set_cpu(c, &mask);
			apic->send_IPI_mask(&mask, ipi_vector);
			}

			if (xcall_debug) {
				set_console_on(1);
				printk("%s cpu=%d xcall %staking too long! c=%d ack=%d [xcall1=%lu] %p\n", 
					msg, smp_processor_id(), 
					cnt1 ? "STILL " : "",
					c, xc->xc_ack, cnt_xcall1, &xc->xc_ack);
				//dump_stack();
				set_console_on(0);
			}

			if (cnt1++ > 6) {
				dtrace_linux_panic();
				break;
			}
		}
	}
	if (xcall_debug && xc->xc_ack && cnt1) {
		set_console_on(1);
		printk("we recovered=%d!\n", cnt1);
		set_console_on(0);
	}
	xc->xc_ack = 0;

	if (xcall_debug) {
		printk("[%d] ack_wait %s finished c=%d cnt=%lu (max=%lu)\n", smp_processor_id(), msg, c, cnt, cnt_xcall5);
	}
}
/**********************************************************************/
/*   Linux  version  of  the  cpu  cross call code. We need to avoid  */
/*   smp_call_function()  as  it  is  not callable from an interrupt  */
/*   routine.  Instead,  we utilise our own private interrupt vector  */
/*   to send an IPI to the other cpus to validate we are synced up.   */
/*   								      */
/*   Why  do  we  do this? Because we dont know where the other cpus  */
/*   are  -  traditional  mutexes  cannot guard against people being  */
/*   inside locked regions without expensive locking protocols.	      */
/*   								      */
/*   On Solaris, the kernel supports arbitrary nested cross-calling,  */
/*   but  on  linux, the restrictions are more severe. We attempt to  */
/*   emulate what Solaris does.					      */
/*   								      */
/*   The  target functions that can be called are minimalist - so we  */
/*   dont  need  to  worry about lots of deadlock complexity for the  */
/*   client functions.						      */
/**********************************************************************/
void dtrace_xcall2(processorid_t cpu, dtrace_xcall_t func, void *arg);
void
dtrace_xcall1(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
static int in_xcall;
	unsigned long flags;

	/***********************************************/
	/*   We disable interrupts whilst xcall is in  */
	/*   progress.    This   prevents   a   timer  */
	/*   interrupt  (eg  profile  provider)  from  */
	/*   trying to cause problems.		       */
	/*   					       */
	/*   But  we also need to prevent another CPU  */
	/*   taking  an  xcall, so we use a spin lock  */
	/*   to keep them out.			       */
	/*   					       */
	/*   Clashing  xcalls are rare - but the rate  */
	/*   can   be  increased  by  judicious  test  */
	/*   scenarios,  and  if  we  clash, we would  */
	/*   hang  the CPU. ack_wait() takes steps to  */
	/*   avoid  deadlock,  but  we really want to  */
	/*   avoid that kicking in.		       */
	/*   					       */
	/*   So  we  spin lock. But we must allow the  */
	/*   other  CPU  to  take an interrupt whilst  */
	/*   waiting, else, we would deadlock against  */
	/*   the  other CPU as it waits for responses  */
	/*   from the other cpus, at the same time as  */
	/*   we have interrupts disabled.	       */
	/***********************************************/
unsigned long zz = 0;
	preempt_disable();
	while (0 && in_xcall) {
zz++;
		local_irq_save(flags);

		/***********************************************/
		/*   We  temporarily  let  the  cpu  take  an  */
		/*   interrupt.  This is possibly bad - could  */
		/*   we have a nested xcall stack? Maybe, but  */
		/*   I *think* this is rare and not a runaway  */
		/*   problem.				       */
		/*   					       */
		/*   My  testing  isnt showing a problem, but  */
		/*   may  need  to try 4 or 8 core cpu to see  */
		/*   if we can force a problem.		       */
		/***********************************************/
		asm("sti");

		local_irq_restore(flags);
	}
if (zz > 1000000) {
printk("we looped too much %d\n", smp_processor_id());
dump_stack();
}
	/***********************************************/
	/*   Critical section.			       */
	/***********************************************/
	local_irq_save(flags);
	in_xcall = 1;
	dtrace_xcall2(cpu, func, arg);
	in_xcall = 0;
	local_irq_restore(flags);
	preempt_enable();
}
void
dtrace_xcall2(processorid_t cpu, dtrace_xcall_t func, void *arg)
{	int	c;
	int	cpu_id = smp_processor_id();
	struct cpumask mask;

	/***********************************************/
	/*   If  we had an internal panic, stop doing  */
	/*   xcalls.   Shouldnt  happen,  but  useful  */
	/*   during debugging so we can diagnose what  */
	/*   caused the panic.			       */
	/***********************************************/
	if (dtrace_shutdown)
		return;

	/***********************************************/
	/*   Special case - just 'us'.		       */
	/***********************************************/
	cnt_xcall1++;
	if (cpu_id == cpu) {
		local_irq_disable();
//printk("[%d] sole cnt=%lu\n", smp_processor_id(), cnt_xcall1);
		func(arg);
		local_irq_enable();
		return;
	}

	preempt_disable();
//	local_irq_disable();

	cnt_xcall2++;
	if (xcall_levels[cpu_id]++)
		cnt_xcall3++;
	/***********************************************/
	/*   Set  up  the  rendezvous  with the other  */
	/*   targetted cpus.			       */
	/***********************************************/
	cpumask_clear(&mask);
	for (c = 0; c < nr_cpus; c++) {
		struct xcalls *xc = &xcalls[c];
		if ((cpu & (1 << c)) == 0 || c == cpu_id) {
			xc->xc_ack = 1;
			continue;
		}

		xc->xc_func = func;
		xc->xc_arg = arg;
		xc->xc_ack = 0;
		xc->xc_wait = 1;
		xc->xc_state = XC_SYNC_OP;
		cpumask_set_cpu(c, &mask);
	}

	smp_mb();

	/***********************************************/
	/*   Now tell the other cpus to do some work.  */
	/***********************************************/
	apic->send_IPI_mask(&mask, ipi_vector);
/*while (native_apic_mem_read(APIC_ICR) & APIC_ICR_BUSY) ;
unsigned long cfg = mask.bits[0] << 4;
native_apic_mem_write(APIC_ICR2, cfg);
cfg = APIC_DM_FIXED | ipi_vector | apic->dest_logical;
native_apic_mem_write(APIC_ICR, cfg);
*/

	/***********************************************/
	/*   Set  up  timers  for the other cpus - we  */
	/*   have  to  let them decide when is a good  */
	/*   time  to  catch  the interrupt, to avoid  */
	/*   irq issues in smp_call_function().	       */
	/***********************************************/
	if (cpu != DTRACE_CPUALL)
		cpu = 1 << cpu;
DELAY(500000);
	/***********************************************/
	/*   Check for ourselves.		       */
	/***********************************************/
	if (cpu & (1 << cpu_id)) {
		func(arg);
	}
printk("[%d] getting ready.... (%ld) mask=%x func=%p\n", smp_processor_id(), cnt_xcall1, mask, func);
	for (c = 0; c < nr_cpus; c++) {
		if (c == cpu_id)
			continue;

		ack_wait(c, "wait1");
	}

	smp_mb();
	/***********************************************/
	/*   All cpus are done - let them proceed.     */
	/***********************************************/
	for (c = 0; c < nr_cpus; c++) {
		struct xcalls *xc = &xcalls[c];
		xc->xc_state = XC_DONE;
		xc->xc_wait = 0;
	}
	smp_mb();
#if 0
	/***********************************************/
	/*   Code  to  handle VMs sync, as documented  */
	/*   by Adam Leventhal.			       */
	/***********************************************/
	for (c = 0; c < nr_cpus; c++) {
		if (c == cpu_id)
			continue;
		ack_wait(c, "wait2");
	}
#endif

	xcall_levels[cpu_id]--;

	preempt_enable();
}
#endif

/**********************************************************************/
/*   This  is the IPI interrupt handler - we got invoked, so we must  */
/*   have something to do.					      */
/**********************************************************************/
void 
xcall_slave(void)
{	volatile struct xcalls *xc = &xcalls[smp_processor_id()];

	ack_APIC_irq();

	cnt_ipi1++;

	if (xcall_debug) {
		printk("%p timer %d (%lu) func=%p\n", xc, smp_processor_id(), cnt_xcall1, xc->xc_func);
		if (xc->xc_ack) printk("why is the ack set?\n");
	}

	(*xc->xc_func)(xc->xc_arg);
	xc->xc_ack = 1;
	smp_mb();

	/***********************************************/
	/*   Wait to be continued.		       */
	/***********************************************/
	if (xcall_debug) {
		printk("%p timer %d waiting for xc_wait (%lu) addr=%p\n", xc, smp_processor_id(), cnt_xcall1, &xc->xc_ack);
	}
	while (xc->xc_wait) {
		smt_pause();
	}

	if (xcall_debug) {
		printk("%p timer %d -- waiting finished (%lu)\n", xc, smp_processor_id(), cnt_xcall1);
	}
}

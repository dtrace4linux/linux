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


extern void dtrace_sync_func(void);
int 	in_xcall;
char	nmi_masks[NCPU];

/**********************************************************************/
/*   Set to true to enable debugging.				      */
/**********************************************************************/
int	xcall_debug = 0;

extern int nr_cpus;
extern int driver_initted;
extern int ipi_vector;

/**********************************************************************/
/*   Let  us  flip between the new and old code easily. The old code  */
/*   will  deadlock  because the kernel smp_call_function calls dont  */
/*   allow for interrupts to be disabled.			      */
/**********************************************************************/
# define	XCALL_MODE	XCALL_NEW
# define	XCALL_ORIG	0
# define	XCALL_NEW	1

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
/*   The  xcalls array is our main work queue. Each cpu can invoke a  */
/*   call to every other cpu.					      */
/**********************************************************************/
# define XC_IDLE	0	/* Owned by us. */
# define XC_WORKING     1	/* Waiting for the target cpu to finish. */
static struct xcalls {
	dtrace_xcall_t	xc_func;
	void		*xc_arg;
	volatile int	xc_state;
	} xcalls[NCPU][NCPU] __cacheline_aligned;
static int xcall_levels[NCPU];

unsigned long cnt_xcall0;
unsigned long cnt_xcall1;
unsigned long cnt_xcall2;
unsigned long cnt_xcall3;
unsigned long cnt_xcall4;
unsigned long cnt_xcall5;	/* Max ack_wait/spin loop */
unsigned long long cnt_xcall6;
unsigned long long cnt_xcall7;
unsigned long cnt_xcall8;
unsigned long cnt_ipi1;
unsigned long cnt_nmi1;
unsigned long cnt_nmi2;

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
void orig_dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg);
void dtrace_xcall1(processorid_t cpu, dtrace_xcall_t func, void *arg);
static void dump_xcalls(void);
static void send_ipi_interrupt(cpumask_t *mask, int vector);
void xcall_slave2(void);

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
//dtrace_printf("orig_dtrace_xcall %lu\n", cnt_xcall1);
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
int
ack_wait(int c, int attempts)
{
	unsigned long cnt = 0;
	int	cnt1 = 0;
	volatile struct xcalls *xc = &xcalls[smp_processor_id()][c];

	/***********************************************/
	/*   Avoid holding on to a stale cache line.   */
	/***********************************************/
	while (dtrace_cas32((void *) &xc->xc_state, XC_WORKING, XC_WORKING) != XC_IDLE) {
		if (attempts-- <= 0)
			return 0;

		barrier();

		/***********************************************/
		/*   Be HT friendly.			       */
		/***********************************************/
//		smt_pause();

		cnt_xcall6++;
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
//		if (cnt++ == 50 * 1000 * 1000UL) {
		if (cnt++ == 1 * 1000 * 1000UL) {
			cnt = 0;
			cnt_xcall4++;

			if (cnt1 == 0) {
				/***********************************************/
				/*   Looks like we are having trouble getting  */
				/*   the interrupt, so try for an NMI.	       */
				/***********************************************/
				cpumask_t mask;
				cpus_clear(mask);
				cpu_set(c, mask);
//				nmi_masks[c] = 1;
//				send_ipi_interrupt(&mask, 2); //NMI_VECTOR);
			}

			if (1) {
//				set_console_on(1);
				dtrace_printf("ack_wait cpu=%d xcall %staking too long! c=%d [xcall1=%lu]\n", 
					smp_processor_id(), 
					cnt1 ? "STILL " : "",
					c, cnt_xcall1);
				//dump_stack();
//				set_console_on(0);
			}

			if (cnt1++ > 3) {
				dump_xcalls();
				dtrace_linux_panic("xcall taking too long");
				break;
			}
		}
	}

	if (xcall_debug) {
		dtrace_printf("[%d] ack_wait finished c=%d cnt=%lu (max=%lu)\n", smp_processor_id(), c, cnt, cnt_xcall5);
	}
	return 1;
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

	/***********************************************/
	/*   We  dont disable interrupts whilst doing  */
	/*   an xcall. We may have two cpus trying to  */
	/*   do  this at the same time, so we need to  */
	/*   run in a lockless mode.		       */
	/*   					       */
	/*   We  do run with preempt_disable to avoid  */
	/*   any  chance  of rescheduling the calling  */
	/*   process  whilst we wait for long periods  */
	/*   of  time.  (Note  that  disable  is  not  */
	/*   nested;  does  this  matter  if  we have  */
	/*   multiple  cpus  coming  in  at  the same  */
	/*   time?				       */
	/***********************************************/
//	preempt_disable();

	/***********************************************/
	/*   Just  track re-entrancy events - we will  */
	/*   be lockless in dtrace_xcall2.	       */
	/***********************************************/
	if (in_xcall) {
		cnt_xcall0++; 
		if (cnt_xcall0 < 10 || (cnt_xcall0 % 50) == 0)
			dtrace_printf("x_call: re-entrant call in progress (%d).\n", cnt_xcall0); 
//dump_all_stacks();
	}
	in_xcall = 1;
	dtrace_xcall2(cpu, func, arg);
	in_xcall = 0;
//	preempt_enable();
}
void
dtrace_xcall2(processorid_t cpu, dtrace_xcall_t func, void *arg)
{	int	c;
	int	cpu_id = smp_processor_id();
	int	cpus_todo = 0;
# if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 18)
typedef struct cpumask cpumask_t;
//#define cpu_set(c, mask) cpumask_set_cpu(c, &(mask))
//#define cpus_clear(mask) cpumask_clear(&mask)
# endif
	cpumask_t mask;

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
//dtrace_printf("[%d] sole cnt=%lu\n", smp_processor_id(), cnt_xcall1);
		func(arg);
		local_irq_enable();
		return;
	}

	/***********************************************/
	/*   Set  up  the  cpu  mask  to  do just the  */
	/*   relevant cpu.			       */
	/***********************************************/
	if (cpu != DTRACE_CPUALL) {
//dtrace_printf("just me %d %d\n", cpu_id, cpu);
		cpu = 1 << cpu;
	}


//dtrace_printf("xcall %d f=%p\n", cpu_id, func);
	cnt_xcall2++;
	if (xcall_levels[cpu_id]++)
		cnt_xcall3++;
	/***********************************************/
	/*   Set  up  the  rendezvous  with the other  */
	/*   targetted  cpus.  We use a nearly square  */
	/*   NCPU*NCPU matrix to allow for any cpu to  */
	/*   wait  for  any  other. We have two slots  */
	/*   per  cpu  -  because  we  may  be  in an  */
	/*   interrupt.				       */
	/*   					       */
	/*   The  interrupt  slave  will  service all  */
	/*   queued  calls  -  sometimes  it  will be  */
	/*   lucky and see multiple, especially if we  */
	/*   are heavily loaded.		       */
	/***********************************************/
	cpus_clear(mask);
	for (c = 0; c < nr_cpus; c++) {
		struct xcalls *xc = &xcalls[cpu_id][c];
		unsigned int cnt;

		/***********************************************/
		/*   Dont  set  ourselves  - we dont want our  */
		/*   cpu  to  be  taking an IPI interrupt and  */
		/*   doing   the   work   twice.   We  inline  */
		/*   ourselves below.			       */
		/***********************************************/
		if ((cpu & (1 << c)) == 0 || c == cpu_id) {
			continue;
		}

		/***********************************************/
		/*   Is  this  safe?  We want to avoid an IPI  */
		/*   call  if the other cpu is idle/not doing  */
		/*   dtrace  work.  If  thats the case and we  */
		/*   are  calling  dtrace_sync,  then  we can  */
		/*   avoid the xcall.			       */
		/***********************************************/
		if ((void *) func == (void *) dtrace_sync_func &&
		    cpu_core[c].cpuc_probe_level == 0) {
			cpu &= ~(1 << c);
			cnt_xcall7++;
			continue;
		}
//dtrace_printf("xcall %p\n", func);

		xc->xc_func = func;
		xc->xc_arg = arg;
		/***********************************************/
		/*   Spinlock  in  case  the  interrupt hasnt  */
		/*   fired.  This  should  be  very rare, and  */
		/*   when it happens, we would be hanging for  */
		/*   100m  iterations  (about  1s). We reduce  */
		/*   the   chance  of  a  hit  by  using  the  */
		/*   NCPU*NCPU*2 array approach. These things  */
		/*   happen  when buffers are full or user is  */
		/*   ^C-ing dtrace.			       */
		/***********************************************/
		for (cnt = 0; dtrace_cas32((void *) &xc->xc_state, XC_WORKING, XC_WORKING) == XC_WORKING; cnt++) {
			/***********************************************/
			/*   Avoid noise for tiny windows.	       */
			/***********************************************/
			if ((cnt == 0 && xcall_debug) || !(xcall_debug && cnt == 50)) {
				dtrace_printf("[%d] cpu%d in wrong state (state=%d)\n",
					smp_processor_id(), c, xc->xc_state);
			}
			if (cnt == 100 * 1000 * 1000) {
				dtrace_printf("[%d] cpu%d - busting lock\n",
					smp_processor_id(), c);
				break;
			}
		}
		if ((cnt && xcall_debug) || (!xcall_debug && cnt > 50)) {
			dtrace_printf("[%d] cpu%d in wrong state (state=%d) %u cycles\n",
				smp_processor_id(), c, xc->xc_state, cnt);
		}
		/***********************************************/
		/*   As  soon  as  we set xc_state and BEFORE  */
		/*   the  apic  call,  the  cpu  may  see the  */
		/*   change  since  it  may  be taking an IPI  */
		/*   interrupt  for  someone else. We need to  */
		/*   be  careful  with  barriers  (I  think -  */
		/*   although    the   clflush/wmb   may   be  */
		/*   redundant).			       */
		/***********************************************/
		xc->xc_state = XC_WORKING;
//		clflush(&xc->xc_state);
//		smp_wmb();
		cpu_set(c, mask);
		cpus_todo++;
	}

	smp_mb();

	/***********************************************/
	/*   Now tell the other cpus to do some work.  */
	/***********************************************/
	if (cpus_todo)
		send_ipi_interrupt(&mask, ipi_vector);

	/***********************************************/
	/*   Check for ourselves.		       */
	/***********************************************/
	if (cpu & (1 << cpu_id)) {
		func(arg);
	}

	if (xcall_debug)
		dtrace_printf("[%d] getting ready.... (%ld) mask=%x func=%p\n", smp_processor_id(), cnt_xcall1, *(int *) &mask, func);

	/***********************************************/
	/*   Wait for the cpus we invoked the IPI on.  */
	/*   Cycle  thru  the  cpus,  to avoid mutual  */
	/*   deadlock  between one cpu trying to call  */
	/*   us whilst we are calling them.	       */
	/***********************************************/
	while (cpus_todo > 0) {
//static int first = 1;
		for (c = 0; c < nr_cpus && cpus_todo > 0; c++) {
			xcall_slave2();
			if (c == cpu_id || (cpu & (1 << c)) == 0)
				continue;

			/***********************************************/
			/*   Wait  a  little  while  for  this cpu to  */
			/*   respond before going on to the next one.  */
			/***********************************************/
			if (ack_wait(c, 1000)) {
				cpus_todo--;
				cpu &= ~(1 << c);
			}
		}
/*if (cpus_todo > 0 && first) { 
cnt_xcall8++;
void dump_all_stacks(void); first = FALSE; dtrace_printf("xcall deadlock:\n"); }
*/
	}

//	smp_mb();

	xcall_levels[cpu_id]--;
}
#endif

/**********************************************************************/
/*   Used for debugging.					      */
/**********************************************************************/
static void
dump_xcalls(void)
{
/*
	int i;
	for (i = 0; i < nr_cpus; i++) {
		dtrace_printf("  cpu%d:  state=%d/%s\n", i,
			xcalls[i].xc_state,
			xcalls[i].xc_state == XC_IDLE ? "idle" : 
			xcalls[i].xc_state == XC_WORKING ? "work" : 
				"??");
	}
*/
}
/**********************************************************************/
/*   Send interrupt request to target cpus.			      */
/**********************************************************************/
static void
send_ipi_interrupt(cpumask_t *mask, int vector)
{
# if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 18)
	/***********************************************/
	/*   Theres  'flat' and theres 'cluster'. The  */
	/*   cluster  functions  handle  more  than 8  */
	/*   cpus. The flat does not - since the APIC  */
	/*   only has room for an 8-bit cpu mask.      */
	/***********************************************/
	static void (*send_IPI_mask)(cpumask_t, int);
	if (send_IPI_mask == NULL)
	        send_IPI_mask = get_proc_addr("cluster_send_IPI_mask");
	if (send_IPI_mask == NULL) dtrace_printf("HELP ON send_ipi_interrupt!\n"); else
	        send_IPI_mask(*mask, vector);
# elif LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 28)
	send_IPI_mask(*mask, vector);
# else
	apic->send_IPI_mask(mask, vector);
# endif
}
/**********************************************************************/
/*   This  is the IPI interrupt handler - we got invoked, so we must  */
/*   have something to do.					      */
/**********************************************************************/
void 
xcall_slave(void)
{
	cnt_ipi1++;

	xcall_slave2();

	smp_mb();

	ack_APIC_irq();
}
void 
xcall_slave2(void)
{	int	i;

	/***********************************************/
	/*   Check  each slot for this cpu - one from  */
	/*   each  of  the  other  cpus  and  one for  */
	/*   interrupt mode and non-interrupt mode in  */
	/*   each cpu.				       */
	/***********************************************/
	for (i = 0; i < nr_cpus; i++) {
		struct xcalls *xc = &xcalls[i][smp_processor_id()];
		if (xc->xc_state == XC_WORKING) {
			(*xc->xc_func)(xc->xc_arg);
			xc->xc_state = XC_IDLE;
		}
	}
}


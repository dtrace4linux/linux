/**********************************************************************/
/*   Code to implement cross-cpu calling for Dtrace.		      */
/*   Author: Paul D. Fox					      */
/*   Date: May 2011						      */
/*   $Header: Last edited: 26-Jul-2012 1.1 $ 			      */
/*   								      */
/*   We  need  to  sync  across  CPUs using the IPI interface. Linux  */
/*   provides  smp_call_function_single and friends to achieve this.  */
/*   But, we cannot call the function with interrupts disabled as we  */
/*   may deadlock.						      */
/*   								      */
/*   This  code  can be called from an interrupt with irqs disabled.  */
/*   So we would deadlock.					      */
/*   								      */
/*   Solaris  doesnt  have this restriction. So, ideally, we use our  */
/*   own non-IRQ sensitive version - lock free xcall.		      */
/*   								      */
/*   Things  get  complicated because of issues with APIC references  */
/*   across the kernel releases, and because of Xen.		      */
/**********************************************************************/

/**********************************************************************/
/*   Bit of madness/strangeness here. If we are not GPL, then "apic"  */
/*   (symbol)  isnt accessible to us. We dont care, but theres a bug  */
/*   in  gcc/ld  and  <asm/ipi.h> because of static/inline functions  */
/*   refer  to "apic" even tho we dont use it. So, we #define it out  */
/*   of  harms way, and then, to avoid an undefined symbol reference  */
/*   (which  we  didnt want), we define our own "hello_apic". I told  */
/*   you it was strange.					      */
/**********************************************************************/

#define apic hello_apic
#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"

#include <linux/cpumask.h>
# if defined(__i386) || defined(__amd64)
#	include <asm/smp.h>
#	include <asm/ipi.h>
# endif


# if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 28)
typedef struct apic_ops apic_t;
#else
typedef struct apic apic_t;
#endif

apic_t *hello_apic; /* Define this because apic.h is broken when facing a */
			/* non-GPL driver. We get an undefined, so define it. */
			/* We use dynamic lookup instead.		*/
apic_t *x_apic;

extern void dtrace_sync_func(void);
int 	in_xcall = -1;
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
/*   The  xcalls array is our main work queue. Each cpu can invoke a  */
/*   call to every other cpu.					      */
/**********************************************************************/
# define XC_IDLE	0	/* Owned by us. */
# define XC_WORKING     1	/* Waiting for the target cpu to finish. */
#if NCPU > 256
#	warning "NCPU is large - your module may not load (x_call.c)"
#endif

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

void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	if (cpu == DTRACE_CPUALL)
		smp_call_function(func, arg, 1);
	else
		smp_call_function_single(cpu, func, arg, 1);
}



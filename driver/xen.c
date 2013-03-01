/**********************************************************************/
/*   Xen  interface  code  for  x_call.c  support.  To be honest, it  */
/*   mostly  doesnt  work.  I  think Xen is broken and the Linux/Xen  */
/*   support  really  doesnt  cater for multiple IPI interrupts. Xen  */
/*   tries  to  use an event interface and control the IRQ mappings,  */
/*   but  this  is  unfriendly to DTrace - we need to get at a lower  */
/*   layer  than the kernel support, if we want to allow a chance of  */
/*   probing the Linux/Xen code.				      */
/*   								      */
/*   The  following  reference  suggests  the  IPI code under Xen is  */
/*   possibly broken.						      */
/*   								      */
/*   http://www.mentby.com/Group/linux-kernel/bisected-perf-top-causing-soft-lockups-under-xen.html				      */
/*   								      */
/*   I am going to have to revisit Xen adoption at a later stage.     */
/*   								      */
/*   Paul Fox, Nov 2012.					      */
/**********************************************************************/

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"

#include <linux/cpumask.h>

extern int nr_cpus;

#if defined(CONFIG_PARAVIRT) && defined(__HYPERVISOR_set_trap_table) && \
	defined(__HYPERVISOR_event_channel_op)
#	define	DO_XEN	1
#	include <xen/interface/callback.h>
# else
#	define	DO_XEN	0
#endif

/**********************************************************************/
/*   See if we are inside a Xen guest.				      */
/**********************************************************************/
int
dtrace_is_xen(void)
{	static void **xen_start_info;
static int first_time = TRUE;

//	return xen_domain();
	if (first_time && xen_start_info == NULL) {
		xen_start_info = get_proc_addr("xen_start_info");
		first_time = FALSE;
	}

	if (xen_start_info && *xen_start_info)
		return TRUE;
	return FALSE;
}

int
dtrace_xen_hypercall(int call, void *a, void *b, void *c)
{
#if DO_XEN
	static char *hypercall_page;
	static int	(*hp)(void *, void *, void *);
	int	ret;

	if (!dtrace_is_xen())
		return 0;

	/***********************************************/
	/*   hypercall_page  is  GPL protected and is  */
	/*   an  assembly level parameter coming from  */
	/*   <hypercall.h>,  so  work around that. We  */
	/*   need  to  be low level to get to the Xen  */
	/*   API  (allow  as  much  dtrace probing as  */
	/*   possible),  so  we  have  to do what the  */
	/*   kernel is doing (approximately).	       */
	/***********************************************/
	if (hypercall_page == NULL) 
		hypercall_page = get_proc_addr("hypercall_page");
	if (hypercall_page == NULL) 
		return 0;

	hp = (void *) (hypercall_page + call * 32);
	ret = hp(a, b, c);
	return ret;
#else
	return 0;
#endif /* DO_XEN */
}

#if DO_XEN
static struct xen_cpu_info_t {
	int	xen_port;
	} xen_cpu_info[NCPU];
#endif

void
xen_xcall_init(void)
{
#if DO_XEN
	int	c;

#define	EVTCHNOP_bind_virq        1
	struct evtchn_bind_virq {
	        /* IN parameters. */
	        uint32_t virq;
	        uint32_t vcpu;
	        /* OUT parameters. */
	        uint32_t port;
	} bind_virq;

	struct evtchn_bind_ipi {
	        uint32_t vcpu;
	        /* OUT parameters. */
	        uint32_t port;
	} bind_ipi;
	int	ret;

	/***********************************************/
	/*   Experiment to see if we can leverage the  */
	/*   callback   code   to   flush   out   IPI  */
	/*   callbacks. (It didnt work).	       */
	/***********************************************/
#if 0
extern int hypcall();
static  struct callback_register callback = {
                .type = CALLBACKTYPE_event,
                .address = XEN_CALLBACK(__KERNEL_CS, hypcall),
                .flags = CALLBACKF_mask_events,
        };
	dtrace_xen_hypercall(__HYPERVISOR_callback_op,
			(void *) CALLBACKOP_register, &callback, 0);
#endif

	/***********************************************/
	/*   Attempt  to allocate a VIRQ for each CPU  */
	/*   and    assign    to   an   event   port.  */
	/*   Unfortunately, Xen documentation is very  */
	/*   poor   in  this  area  and  the  initial  */
	/*   bind_irq possibly fails, and we dont get  */
	/*   any indication of IPI interrupts.	       */
	/***********************************************/
	for (c = 0; c < nr_cpus; c++) {
		int	port;
		int	virq = 64+c;

		bind_virq.virq = virq;
		bind_virq.vcpu = c;
		port = dtrace_xen_hypercall(__HYPERVISOR_event_channel_op,
			(void *) EVTCHNOP_bind_virq, &bind_virq, 0);
		printk("xen: [%d] bind_virq ret==%d port=%d\n", c, port, bind_virq.port);
		if (port == 0) {
			port = bind_virq.port;
		}

#define EVTCHNOP_bind_ipi         7
		bind_ipi.vcpu = c;
		ret = dtrace_xen_hypercall(__HYPERVISOR_event_channel_op,
			(void *) EVTCHNOP_bind_ipi, &bind_ipi, 0);
		printk("xen: [%d] bind_ipi=%d\n", c, ret);

		xen_cpu_info[c].xen_port = bind_ipi.port;
	}
#endif /* DO_XEN */

}

/**********************************************************************/
/*   Attempt  to  cleanup  and deregister the port channels for each  */
/*   cpu, so we can unload the driver.				      */
/**********************************************************************/
void
xen_xcall_fini(void)
{
#if DO_XEN
	int	c;

#define	EVTCHNOP_close            3
struct evtchn_close {
	uint32_t port;
} close;

	for (c = 0; c < nr_cpus; c++) {
		close.port = xen_cpu_info[c].xen_port;

		dtrace_xen_hypercall(__HYPERVISOR_event_channel_op,
			(void *) EVTCHNOP_close, &close, 0);
	}
#endif
}

/**********************************************************************/
/*   Send  an  event to the target cpus (we should be checking which  */
/*   cpus), but this doesnt seem to work.			      */
/**********************************************************************/
int
xen_send_ipi(cpumask_t *mask, int vector)
{
#if DO_XEN
	int	c;
	int	ret;
#define	EVTCHNOP_send             4

	for (c = 0; c < nr_cpus; c++) {
		struct evtchn_send {
		        uint32_t port;
		} send = { .port = xen_cpu_info[c].xen_port };

		if (c == smp_processor_id())
			continue;

		ret = dtrace_xen_hypercall(__HYPERVISOR_event_channel_op,
			(void *) EVTCHNOP_send, &send, 0);
	}
#endif
	return 0;
}

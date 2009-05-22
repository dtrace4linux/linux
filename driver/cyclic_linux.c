/**********************************************************************/
/*   Cyclic timers -- hi-res repeated timers. Sun have a big complex  */
/*   and  sophisticated  cyclic.c  implementation  with lots of nice  */
/*   comments describing whats going on.			      */
/*   								      */
/*   We  could  port it and then try and map it down to Linux, or we  */
/*   could  do  what  FreeBSD  does  --  a simplified version of the  */
/*   cyclic.c code.						      */
/*   								      */
/*   Or  we  could  try  and  map  direct  to hrtimer.c in the Linux  */
/*   kernel.							      */
/*   								      */
/*   Or we could do nothing; if we do nothing, then a running dtrace  */
/*   will die sooner or later because it will believe we are hanging  */
/*   the system.						      */
/*   								      */
/*   Paul Fox June 2008						      */
/*   								      */
/*   $Header: Last edited: 22-Apr-2009 1.2 $ 			      */
/**********************************************************************/

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <sys/cyclic_impl.h>

# define	CYCLIC_SUN		0
# define	CYCLIC_LINUX		1
# define	CYCLIC_OLD_TIMER	2

/**********************************************************************/
/*   Need to rewrite this for older kernels without hrtimers?	      */
/**********************************************************************/
#if !defined(HRTIMER_STATE_INACTIVE)
# define MODE CYCLIC_OLD_TIMER
#else
# define MODE CYCLIC_LINUX
#endif

# if MODE == CYCLIC_SUN
// needed if we go the Sun route..

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
static void cbe_enable(cyb_arg_t);
static void cbe_disable(cyb_arg_t);
static void cbe_reprogram(cyb_arg_t, hrtime_t);
static void cbe_xcall(cyb_arg_t, cpu_t *, cyc_func_t, void *);

static void 
cbe_enable(cyb_arg_t a)
{
}
static void 
cbe_disable(cyb_arg_t a)
{
}
static void 
cbe_reprogram(cyb_arg_t a, hrtime_t t)
{
}
static void 
cbe_xcall(cyb_arg_t a, cpu_t *c, cyc_func_t f, void *p)
{
}

static cyc_backend_t be = {
        .cyb_enable    = cbe_enable,
        .cyb_disable   = cbe_disable,
        .cyb_reprogram = cbe_reprogram,
        .cyb_xcall     = cbe_xcall,
	};

/**********************************************************************/
/*   Called when dtrace is loaded.				      */
/**********************************************************************/
int
init_cyclic()
{
	/***********************************************/
	/*   Second argument - hrtime_t doesnt appear  */
	/*   to be used.			       */
	/***********************************************/
        cyclic_init(&be, 1);
	
	return TRUE;
}

# endif

# if MODE == CYCLIC_LINUX
// Here to create our own stubs.

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/

/**********************************************************************/
/*   hrtimer_cancel function which is marked as GPL.		      */
/**********************************************************************/
static int (*fn_hrtimer_init)(struct hrtimer *timer, clockid_t which_clock,
                         enum hrtimer_mode mode);
static int (*fn_hrtimer_cancel)(struct hrtimer *);
static int (*fn_hrtimer_start)(struct hrtimer *timer, ktime_t tim,
                         const enum hrtimer_mode mode);

struct c_timer {
	struct hrtimer	c_htp;	/* Must be first item in structure */
	cyc_handler_t	c_hdlr;
	cyc_time_t	c_time;
	};

int
init_cyclic()
{
	/***********************************************/
	/*   Get the functions we need - some kernels  */
	/*   wont export these (maybe I am wrong), so  */
	/*   we    can    detect   at   runtime.   We  */
	/*   could/should  do  something else if this  */
	/*   happens,   but   its   not  a  supported  */
	/*   scenario  since  we  can  use one of the  */
	/*   other MODEs.			       */
	/***********************************************/
	fn_hrtimer_cancel = get_proc_addr("hrtimer_cancel");
	fn_hrtimer_init   = get_proc_addr("hrtimer_init");
	fn_hrtimer_start  = get_proc_addr("hrtimer_start");
	fn_hrtimer_start  = get_proc_addr("hrtimer_start");

	if (fn_hrtimer_start == NULL) {
		printk(KERN_WARNING "dtracedrv: Cannot locate hrtimer in this kernel\n");
		return FALSE;
	}
	return TRUE;
}
static enum hrtimer_restart
be_callback(struct hrtimer *ptr)
{	struct c_timer *cp = (struct c_timer *) ptr;
	ktime_t kt;

	kt.tv64 = cp->c_time.cyt_interval;
	/***********************************************/
	/*   Invoke the callback.		       */
	/***********************************************/
	cp->c_hdlr.cyh_func(cp->c_hdlr.cyh_arg);

	/***********************************************/
	/*   Bit   annoying   this   --   in  2.6.28,  */
	/*   "expires" gets renamed to "_expires" and  */
	/*   "_softexpires".   The  API  we  want  is  */
	/*   inlined,  but relies on a GPL exportable  */
	/*   symbol,  so  we  cannot  use  it (or use  */
	/*   /proc/kallsyms  lookups),  so its easier  */
	/*   to just inline what they expect.	       */
	/***********************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
	ptr->_expires = ktime_add_ns(ptr->_expires, kt.tv64);
	ptr->_softexpires = ktime_add_ns(ptr->_softexpires, kt.tv64);
#else
	ptr->expires = ktime_add_ns(ptr->expires, kt.tv64);
#endif
	return HRTIMER_RESTART;
}
cyclic_id_t 
cyclic_add(cyc_handler_t *hdrl, cyc_time_t *t)
{	struct c_timer *cp = (struct c_timer *) kzalloc(sizeof *cp, GFP_KERNEL);
	ktime_t kt;

	if (cp == NULL) {
		printk("dtracedrv:cyclic_add: Cannot alloc memory\n");
		return 0;
	}

	kt.tv64 = t->cyt_interval;
	cp->c_hdlr = *hdrl;
	cp->c_time = *t;

	fn_hrtimer_init(&cp->c_htp, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
/*	cp->c_htp.cb_mode = HRTIMER_CB_SOFTIRQ;*/
	cp->c_htp.function = be_callback;

	fn_hrtimer_start(&cp->c_htp, kt, HRTIMER_MODE_REL);

	return (cyclic_id_t) cp;
}
cyclic_id_t
cyclic_add_omni(cyc_omni_handler_t *omni)
{
	TODO();
	return 0;
}
void 
cyclic_remove(cyclic_id_t id)
{	struct c_timer *ctp = (struct c_timer *) id;

	if (id == 0)
		return;

	fn_hrtimer_cancel(&ctp->c_htp);
	kfree(ctp);
}

# endif

# if MODE == CYCLIC_OLD_TIMER
/**********************************************************************/
/*   This is for non-hrtimer aware kernels.			      */
/**********************************************************************/
struct c_timer {
        struct timer_list c_timer;      /* Must be first item in structure */
        cyc_handler_t   c_hdlr;
        cyc_time_t      c_time;
        };

int
init_cyclic()
{
	return TRUE;
}
static void
be_callback(struct timer_list *ptr)
{	struct c_timer *cp = (struct c_timer *) ptr;
	struct timespec ts;
	unsigned long j;

	/***********************************************/
	/*   Invoke the callback.                      */
	/***********************************************/
	cp->c_hdlr.cyh_func(cp->c_hdlr.cyh_arg);

	/***********************************************/
	/*   Refire the timer.                         */
	/***********************************************/
	ts.tv_sec = cp->c_time.cyt_interval / (1000 * 1000 * 1000);
	ts.tv_nsec = cp->c_time.cyt_interval % (1000 * 1000 * 1000);
	j = timespec_to_jiffies(&ts);
	cp->c_timer.expires = jiffies + j;
	add_timer(&cp->c_timer);
}

void
cyclic_init(cyc_backend_t *be, hrtime_t resolution)
{
}
cyclic_id_t
cyclic_add(cyc_handler_t *hdrl, cyc_time_t *t)
{	struct c_timer *cp = (struct c_timer *) kzalloc(sizeof *cp, GFP_KERNEL);
	struct timespec ts;
	unsigned long j;

	ts.tv_sec = t->cyt_interval / (1000 * 1000 * 1000);
	ts.tv_nsec = t->cyt_interval % (1000 * 1000 * 1000);
	j = timespec_to_jiffies(&ts);

	init_timer(&cp->c_timer);
	cp->c_timer.function = be_callback;
	cp->c_timer.data = (void *) cp;
	cp->c_timer.expires = jiffies + j;
	cp->c_hdlr = *hdrl;
	cp->c_time = *t;
	add_timer(&cp->c_timer);
	return (cyclic_id_t) cp;
}
cyclic_id_t
cyclic_add_omni(cyc_omni_handler_t *omni)
{
        TODO();
        return 0;
}
void
cyclic_remove(cyclic_id_t id)
{       struct c_timer *cp = (struct c_timer *) id;

	if (id) {
		del_timer(&cp->c_timer);
		kfree(cp);
	}
}
# endif


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
/**********************************************************************/

#include "dtrace_linux.h"
#include <sys/cyclic_impl.h>

# define	CYCLIC_SUN	0
# define	CYCLIC_LINUX	1
# define	CYCLIC_DUMMY	2

/**********************************************************************/
/*   Need to rewrite this for older kernels without hrtimers?	      */
/**********************************************************************/
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 19)
# define MODE CYCLIC_DUMMY
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
void
init_cyclic()
{
	/***********************************************/
	/*   Second argument - hrtime_t doesnt appear  */
	/*   to be used.			       */
	/***********************************************/
        cyclic_init(&be, 1);
	
}

# endif

# if MODE == CYCLIC_LINUX
// Here to create our own stubs.

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
void *fbt_get_hrtimer_cancel(void);
void *fbt_get_hrtimer_start(void);
void *fbt_get_hrtimer_init(void);

/**********************************************************************/
/*   hrtimer_cancel function which is marked as GPL.		      */
/**********************************************************************/
static int (*fn_hrtimer_init)(struct hrtimer *timer, clockid_t which_clock,
                         enum hrtimer_mode mode);
static int (*fn_hrtimer_cancel)(struct hrtimer *);
static int (*fn_hrtimer_start)(struct hrtimer *timer, ktime_t tim,
                         const enum hrtimer_mode mode);

struct c_timer {
	struct hrtimer	c_htp;
	cyc_handler_t	c_hdlr;
	cyc_time_t	c_time;
	};

void
cyclic_init(cyc_backend_t *be, hrtime_t resolution)
{
}
static void
be_callback(struct c_timer *cp)
{	ktime_t kt;

	kt.tv64 = cp->c_time.cyt_interval;

	cp->c_hdlr.cyh_func(cp->c_hdlr.cyh_arg);
	fn_hrtimer_start(&cp->c_htp, kt, HRTIMER_MODE_REL);
}
cyclic_id_t 
cyclic_add(cyc_handler_t *hdrl, cyc_time_t *t)
{	struct c_timer *cp = (struct c_timer *) kzalloc(sizeof *cp, GFP_KERNEL);
	ktime_t kt;

	fn_hrtimer_cancel = fbt_get_hrtimer_cancel();
	fn_hrtimer_init   = fbt_get_hrtimer_init();
	fn_hrtimer_start  = fbt_get_hrtimer_start();

	kt.tv64 = t->cyt_interval;

	if (cp == NULL)
		return 0;

	cp->c_hdlr = *hdrl;
	cp->c_time = *t;

	fn_hrtimer_init(&cp->c_htp, CLOCK_REALTIME, HRTIMER_MODE_REL);

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

# if MODE == CYCLIC_DUMMY
// Here to create our own stubs.

void
cyclic_init(cyc_backend_t *be, hrtime_t resolution)
{
}
cyclic_id_t 
cyclic_add(cyc_handler_t *hdrl, cyc_time_t *t)
{
	TODO();
	return 0;
}
cyclic_id_t
cyclic_add_omni(cyc_omni_handler_t *omni)
{
	TODO();
	return 0;
}
void 
cyclic_remove(cyclic_id_t id)
{
	TODO();
}

# endif


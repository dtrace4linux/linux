/**********************************************************************/
/*   Emulation  of  the Solaris taskq interface. A taskq corresponds  */
/*   to  a Linux work_queue interface. Sometimes we need to schedule  */
/*   something  to happen outside of an interrupt and in the context  */
/*   of  a  process  -  not  any  process  -  but a process which is  */
/*   independent of all other processes.			      */
/*   								      */
/*   This  is needed by the garbage collection code for fasttrap. We  */
/*   want  to remove the probes when the original dtrace owner dies,  */
/*   but  he may be taking time, so, to avoid mutex deadlock, dtrace  */
/*   uses  the  taskq mechanism to periodically retry, since its not  */
/*   time critical.						      */
/*   								      */
/*   (Of  course  the  implication  is  that  a  pathological dtrace  */
/*   come-and-go  could  use  up  a  lot of memory and not allow the  */
/*   garbage collector to kick in).				      */
/*   								      */
/*   Unfortunately,  the  workqueue interface is GPL, but even if we  */
/*   state  this  file  is  GPL,  doesnt really help. We have to use  */
/*   function  pointers to get around this, which means things could  */
/*   be   brittle   over  kernel  releases.  __alloc_workqueue_key()  */
/*   significantly  changed  parameter  ordering  so  we  need to be  */
/*   careful to handle this.					      */
/*   								      */
/*   Author: Paul D Fox						      */
/*   Date: July 2012						      */
/*   								      */
/*   This  is GPL code - its only a piece of glue and doesnt need to  */
/*   come under the CDDL.					      */
/**********************************************************************/

#define __alloc_workqueue_key local__alloc_workqueue_key
#define lockdep_init_map local_lockdep_init_map

#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include <dtrace_proto.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <taskq.h>

//typedef struct taskq taskq_t;

/**********************************************************************/
/*   Handle   older   kernels   which   do   not  separate  out  the  */
/*   work/delayed-work structs.					      */
/**********************************************************************/
#if !defined(INIT_DELAYED_WORK)
	/***********************************************/
	/*   On  older  kernel, INIT_WORK takes three  */
	/*   args  and the last is the callback (void  */
	/*   *)ptr, so be explicit and pass it in.     */
	/***********************************************/
#	define	INIT_DELAYED_WORK(a, b) INIT_WORK(a, b, a)
#	define	delayed_work	work_struct
#	define	cancel_delayed_work_sync	cancel_delayed_work
#else
#undef __INIT_DELAYED_WORK
#	define __INIT_DELAYED_WORK(_work, _func, _tflags)		\
	do {								\
		INIT_WORK(&(_work)->work, (_func));			\
		__setup_timer(&(_work)->timer, delayed_work_timer_fn_ptr,	\
			      (unsigned long)(_work),			\
			      (_tflags) | TIMER_IRQSAFE);		\
	} while (0)
#endif

/**********************************************************************/
/*   Kernels >= 2.6.37 changed the interface. Work around GPL issue.  */
/**********************************************************************/
# if !defined(WR_MEM_RECLAIM) /* Introduced in 2.6.37 */
struct workqueue_struct *
__create_workqueue_key(const char *name, int singlethread,
                       int freezeable, int rt, struct lock_class_key *key,
                       const char *lock_name)
{	static void *(*func)(const char *name, int singlethread,
                       int freezeable, int rt, struct lock_class_key *key,
                       const char *lock_name);
	if (func == NULL)
		func = get_proc_addr("__create_workqueue_key");
	if (func == NULL)
		return NULL;

	return func(name, singlethread, freezeable, rt, key, lock_name);
}
# endif

/**********************************************************************/
/*   Following   is   horrible.   For   older  kernels,  eg  centos5  */
/*   2.6.18-128.el5,  the  create_workqueue  expands  into a call to  */
/*   this  GPL  function.  So we get the macro to call locally, then  */
/*   indirect to the real function.				      */
/**********************************************************************/
#if !defined(__create_workqueue)
struct workqueue_struct *__create_workqueue(const char *name, int singlethread)
{	static struct workqueue_struct *(*func)(const char *name, int singlethread);

	if (func == NULL)
		func = get_proc_addr("__create_workqueue");
	return func(name, singlethread);
}
#endif

/**********************************************************************/
/*   Work  queue  data structure, used by dtrace.c for dtrace_taskq,  */
/*   and for timeout() implementation.				      */
/**********************************************************************/
typedef struct my_work_t {
	struct delayed_work t_work;
	task_func_t	*t_func;
	void		*t_arg;
	timeout_id_t	t_id;
	struct my_work_t *t_next;
	} my_work_t;
/**********************************************************************/
/*   List of timers - should only be 1 or 2, not lots.		      */
/**********************************************************************/
static my_work_t *callo;


static int taskq_enabled;

/**********************************************************************/
/*   Hold on to the dtrace_taskq, from dtrace.c, so we can reuse for  */
/*   the timeout() functions.					      */
/**********************************************************************/
static taskq_t *global_tq;

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
taskqid_t taskq_dispatch2(taskq_t *tq, task_func_t func, void *arg, uint_t flags, unsigned long delay);

/**********************************************************************/
/*   GPL prototypes.						      */
/**********************************************************************/
static int (*queue_delayed_work_ptr)(struct workqueue_struct *wq, struct delayed_work *work, unsigned long delay);
static void (*destroy_workqueue_ptr)(struct workqueue_struct *wq);
static void (*flush_workqueue_ptr)(struct workqueue_struct *wq);
static void (*delayed_work_timer_fn_ptr)(unsigned long __data);

/**********************************************************************/
/*   Very  ugly  ..  different  number  of  args  for  3.3 and above  */
/*   kernels.  All  we  want  is to invoke the real function, but we  */
/*   cannot link if we try (because we arent GPL).		      */
/**********************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
# define	WQ_ARGS , ...
#else
# define	WQ_ARGS
#endif

struct workqueue_struct *(*__alloc_workqueue_key_ptr)(const char *fmt,
                                               unsigned int flags,
                                               int max_active,
                                               struct lock_class_key *key,
                                               const char *lock_name WQ_ARGS);

void (*lockdep_init_map_ptr)(struct lockdep_map *lock, const char *name,
                             struct lock_class_key *key, int subclass);

/**********************************************************************/
/*   Wrapper for functions we cannot directly link against.	      */
/**********************************************************************/
void dummy_trampoline(void)
{
	/***********************************************/
	/*   We  embed  the real function here, since  */
	/*   we   dont   want   to  touch  the  stack  */
	/*   arguments  or bother with 32/64 compiler  */
	/*   calling conventions. I would like to say  */
	/*   how   clever   I   am,   but  this  aint  */
	/*   clever...its just heavy, man ! :-)	       */
	/***********************************************/
# if defined(__i386) || defined(__amd64)
	asm(FUNCTION(local__alloc_workqueue_key));
	asm("jmp *__alloc_workqueue_key_ptr\n");

	asm(FUNCTION(local_lockdep_init_map));
	asm("jmp *lockdep_init_map_ptr\n");
# elif defined(__arm__)
	asm(FUNCTION(local__alloc_workqueue_key));
	asm("ldr	ip, .L3x\n");
	asm("ldr	ip, [ip, #0]\n");
	asm("bx ip\n");
	asm(".L3x:	.word __alloc_workqueue_key_ptr\n");

	asm(FUNCTION(local_lockdep_init_map));
	asm("ldr	ip, .L4x\n");
	asm("ldr	ip, [ip, #0]\n");
	asm("bx ip\n");
	asm(".L4x:	.word local_lockdep_init_map\n");
# else
	# error "missing trampoline definition for this CPU"
# endif

}
static void
taskq_callback(struct work_struct *work)
{	my_work_t *mw = (my_work_t *) work;

	(*mw->t_func)(mw->t_arg);
}
taskq_t *                                                                    
taskq_create(const char *name, int nthreads, pri_t pri, int minalloc,        
    int maxalloc, uint_t flags)    
{	taskq_t *tq;

	if (__alloc_workqueue_key_ptr == NULL) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
		__alloc_workqueue_key_ptr = get_proc_addr("__alloc_workqueue_key");
		printk("__alloc_workqueue_key_ptr=%p\n", __alloc_workqueue_key_ptr);
		/***********************************************/
		/*   If,  for  some reason, we cannot get the  */
		/*   pointers,  dont let us do anything. This  */
		/*   will  stop  fasttrap/pid  provider  from  */
		/*   working  properly  (garbage collection),  */
		/*   but at least dtrace will load.	       */
		/***********************************************/
		if (__alloc_workqueue_key_ptr == NULL) {
			printk("taskq_create failed because of lack of pointers\n");
			return 0;
			}
#endif
		queue_delayed_work_ptr = get_proc_addr("queue_delayed_work");
		destroy_workqueue_ptr = get_proc_addr("destroy_workqueue");
		flush_workqueue_ptr = get_proc_addr("flush_workqueue");
		delayed_work_timer_fn_ptr = get_proc_addr("delayed_work_timer_fn");
		lockdep_init_map_ptr = get_proc_addr("lockdep_init_map");

		printk("queue_delayed_work_ptr=%p\n", queue_delayed_work_ptr);
		printk("destroy_workqueue_ptr=%p\n", destroy_workqueue_ptr);
		printk("flush_workqueue_ptr=%p\n", flush_workqueue_ptr);
		printk("delayed_work_timer_fn_ptr=%p\n", delayed_work_timer_fn_ptr);
#if !defined(lockdep_init_map)
		/***********************************************/
		/*   This is a macro if CONFIG_LOCKDEP is not  */
		/*   set.				       */
		/***********************************************/
		printk("lockdep_init_map_ptr=%p\n", lockdep_init_map_ptr);
#endif
	}

	tq = (taskq_t *) create_workqueue(name);
	global_tq = tq;
	taskq_enabled = TRUE;
	return tq;
}

taskqid_t                                                                    
taskq_dispatch(taskq_t *tq, task_func_t func, void *arg, uint_t flags)       
{
	return taskq_dispatch2(tq, func, arg, flags, 1);
}

taskqid_t                                                                    
taskq_dispatch2(taskq_t *tq, task_func_t func, void *arg, uint_t flags, unsigned long delay)
{	my_work_t *work;

	if (!taskq_enabled)
		return 0;

	work = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL & ~__GFP_WAIT);
	if (work == NULL) {
		printk("taskq_dispatch: couldnt alloc work buffer\n");
		return -1;
	}
	INIT_DELAYED_WORK( &work->t_work, taskq_callback);
	work->t_func = func;
	work->t_arg = arg;
	queue_delayed_work_ptr((struct workqueue_struct *) tq, 
		&work->t_work, delay);
	return (taskqid_t) work;
}
void
taskq_destroy(taskq_t *tq)                                                   
{	struct workqueue_struct *wq = (struct workqueue_struct *) tq;

	if (!taskq_enabled || wq == NULL)
		return;
	if (flush_workqueue_ptr)
		flush_workqueue_ptr(wq);
	if (destroy_workqueue_ptr)
		destroy_workqueue_ptr(wq);
} 
/**********************************************************************/
/*   Old-fashioned  style  timer callbacks. Called by fasttrap.c and  */
/*   dtrace_unregister.  We  need  to  ensure  timer  callbacks  are  */
/*   executed  in  the  context  of  a  process which doesnt care or  */
/*   couldnt  be  locking  dtrace  - avoiding deadlock. This is only  */
/*   used  for fasttrap garbage collection but we mustnt fire whilst  */
/*   the  tear  down occurs, else mutex_enter will deadlock or call  */
/*   schedule() from an invalid context.			      */
/**********************************************************************/

timeout_id_t
timeout(void (*func)(void *), void *arg, unsigned long ticks)
{static unsigned long id;
	my_work_t *wp;

printk("timeout(%p, %lu)\n", func, ticks);
	id++;

	wp = (my_work_t *) taskq_dispatch2(global_tq, func, arg, 0, ticks);
	if (wp == NULL) {
		printk("timeout: couldnt allocate my_work_t (taskq_enabled=%d)\n", taskq_enabled);
		return 0;
	}
	wp->t_id = (timeout_id_t) id;
	wp->t_next = callo;
	callo = wp;
	return (timeout_id_t) id;
}
void
untimeout(timeout_id_t id)
{	my_work_t *wp;

printk("untimeout: %p\n", id);
	if (callo && callo->t_id == id) {
		wp = callo;
		callo = wp->t_next;
	} else {
		my_work_t *wp0 = NULL;
		for (wp = callo; wp; wp0 = wp, wp = wp->t_next) {
			if (wp->t_id == id) {
				wp0->t_next = wp->t_next;
				break;
				}
		}
		if (wp == NULL) {
			printk("untimeout(%p) - not found\n", id);
			return;
		}
	}

	cancel_delayed_work_sync(&wp->t_work);
}

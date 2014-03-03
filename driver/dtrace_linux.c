/**********************************************************************/
/*   This file contains much of the glue between the Solaris code in  */
/*   dtrace.c  and  the linux kernel. We emulate much of the missing  */
/*   functionality, or map into the kernel.			      */
/*   								      */
/*   Date: April 2008						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/*   								      */
/*   $Header: Last edited: 04-Feb-2013 1.14 $ 			      */
/**********************************************************************/

#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include "proc_compat.h"

#include <linux/cpumask.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sys.h>
#include <linux/thread_info.h>
#include <linux/profile.h>
#include <linux/vmalloc.h>
#include <asm/tlbflush.h>
#include <asm/current.h>
# if defined(__i386) || defined(__amd64)
#	include <asm/desc.h>
# endif
#include <sys/rwlock.h>
#include <sys/privregs.h>
//#include <asm/pgtable.h>
//#include <asm/pgalloc.h>

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_DESCRIPTION("DTRACEDRV Driver");

# define	TRACE_ALLOC	0

/**********************************************************************/
/*   Turn on HERE() macro tracing.				      */
/**********************************************************************/
int dtrace_here;
module_param(dtrace_here, int, 0);
int dtrace_mem_alloc;
module_param(dtrace_mem_alloc, int, 0);
int dtrace_unhandled;
module_param(dtrace_unhandled, int, 0);
int fbt_name_opcodes;
module_param(fbt_name_opcodes, int, 0);
int grab_panic;
module_param(grab_panic, int, 0);
char *arg_kallsyms_lookup_name; /* Done as a string, because kernel doesnt */
				/* like 0xfffffff12345678 as a number. */
module_param(arg_kallsyms_lookup_name, charp, 0);

extern char dtrace_buf[];
extern const int log_bufsiz;
extern int dbuf_i;
extern int dtrace_safe;

/**********************************************************************/
/*   TRUE when we have called dtrace_linux_init(). After that point,  */
/*   xcalls  are  valid,  but  until  then,  we need to wait for the  */
/*   externally loaded symbol patchups.				      */
/**********************************************************************/
int driver_initted;

/**********************************************************************/
/*   Stuff we stash away from /proc/kallsyms.			      */
/**********************************************************************/
enum {
	OFFSET_kallsyms_lookup_name,
	OFFSET_modules,
	OFFSET_sys_call_table,
	OFFSET_access_process_vm,
	OFFSET_syscall_call,
	OFFSET_print_modules,
	OFFSET_task_exit_notifier,
	OFFSET_xtime,
	OFFSET_ia32_sys_call_table,
	OFFSET_END_SYMS,
	};
static struct map {
	char		*m_name;
	unsigned long	*m_ptr;
	} syms[] = {
{"kallsyms_lookup_name",   NULL},
{"modules",                NULL},
{"sys_call_table",         NULL},
{"access_process_vm",      NULL},
{"syscall_call",           NULL}, /* Backup for i386 2.6.23 kernel to help */
			 	  /* find the sys_call_table.		  */
{"print_modules",          NULL}, /* Backup for i386 2.6.23 kernel to help */
			 	  /* find the modules table. 		  */
{"task_exit_notifier",     NULL},
{"xtime",     		   NULL}, /* Needed for dtrace_gethrtime, if 2.6.9 */
{"ia32_sys_call_table",    NULL}, /* On 64b kernel, the 32b syscall table. */
{"END_SYMS",               NULL}, /* This is a sentinel so we know we are done. */
	{0}
	};
static unsigned long (*xkallsyms_lookup_name)(char *);
static void *xmodules;
static void **xsys_call_table;
static void **xia32_sys_call_table;
static void (*fn_sysrq_showregs_othercpus)(void *);
int (*kernel_text_address_fn)(unsigned long);
char *(*dentry_path_fn)(struct dentry *, char *, int);
static struct module *(*fn__module_text_address)(unsigned long);
void *(*fn_pid_task)(void *, int);
void *(*fn_find_get_pid)(int);

/**********************************************************************/
/*   Stats    counters   for   ad   hoc   debugging;   exposed   via  */
/*   /proc/dtrace/stats.					      */
/**********************************************************************/
unsigned long dcnt[MAX_DCNT];

/**********************************************************************/
/*   The  security  profiles, loaded via /dev/dtrace. See comment in  */
/*   dtrace_linux.h for details on how this works.		      */
/**********************************************************************/
# define	MAX_SEC_LIST	64
int		di_cnt;
dsec_item_t	di_list[MAX_SEC_LIST];

/**********************************************************************/
/*   The  kernel  can be compiled with a lot of potential CPUs, e.g.  */
/*   64  is  not  untypical,  but  we  only have a dual core cpu. We  */
/*   allocate  buffers  for each cpu - which can mushroom the memory  */
/*   needed  (4MB+  per  core), and can OOM for small systems or put  */
/*   other systems into mortal danger as we eat all the memory.	      */
/**********************************************************************/
cpu_t		*cpu_list;
cpu_core_t	*cpu_core;
cpu_t		*cpu_table;
cred_t		*cpu_cred;
int	nr_cpus = 1;
DEFINE_MUTEX(mod_lock);

/**********************************************************************/
/*   Set  to  true  by  debug code that wants to immediately disable  */
/*   probes so we can diagnose what was happening.		      */
/**********************************************************************/
int dtrace_shutdown;

/**********************************************************************/
/*   We  need  one  of  these for every process on the system. Linux  */
/*   doesnt  provide a way to find a process being created, and even  */
/*   if  it did, we need to allocate memory when that happens, which  */
/*   is not viable. So we preallocate all the space we need up front  */
/*   during driver init. This isnt nice, since that max_pid variable  */
/*   can change, but typically doesnt.				      */
/**********************************************************************/
sol_proc_t	*shadow_procs;

DEFINE_MUTEX(cpu_lock);
int	panic_quiesce;
sol_proc_t	*curthread;

dtrace_vtime_state_t dtrace_vtime_active = 0;
dtrace_cacheid_t dtrace_predcache_id = DTRACE_CACHEIDNONE + 1;

/**********************************************************************/
/*   For dtrace_gethrtime.					      */
/**********************************************************************/
static int tsc_max_delta;
static struct timespec *xtime_cache_ptr;
ktime_t (*ktime_get_ptr)(void);
struct timespec (*__current_kernel_time_ptr)(void);
static u64 (*native_sched_clock_ptr)(void);

/**********************************************************************/
/*   Ensure we are at the head of the chains. Unfortunately, kprobes  */
/*   puts  itself  at  the front of the chain and the notifier calls  */
/*   wont  let  us  go  first  -  which  we  need  in order to avoid  */
/*   re-entrancy   issues.   We   have   to   work  around  that  in  */
/*   dtrace_linux_init().					      */
/**********************************************************************/
# define NOTIFIER_MAX_PRIO 0x7fffffff

/**********************************************************************/
/*   Stuff for INT3 interception.				      */
/**********************************************************************/

static int (*fn_profile_event_register)(enum profile_type type, struct notifier_block *n);
static int (*fn_profile_event_unregister)(enum profile_type type, struct notifier_block *n);

/**********************************************************************/
/*   Notifier for invalid instruction trap.			      */
/**********************************************************************/
# if 0
static int proc_notifier_trap_illop(struct notifier_block *, unsigned long, void *);
static struct notifier_block n_trap_illop = {
	.notifier_call = proc_notifier_trap_illop,
	.priority = NOTIFIER_MAX_PRIO, // notify us first - before kprobes
	};
# endif

/**********************************************************************/
/*   Process exiting notifier.					      */
/**********************************************************************/
static int proc_exit_notifier(struct notifier_block *, unsigned long, void *);
static struct notifier_block n_exit = {
	.notifier_call = proc_exit_notifier,
	.priority = NOTIFIER_MAX_PRIO, // notify us first - before kprobes
	};

/**********************************************************************/
/*   Used when we want to intercept panics for final autopsy of what  */
/*   we did wrong.						      */
/**********************************************************************/
#if 0
int dtrace_kernel_panic(struct notifier_block *this, unsigned long event, void *ptr);
static struct notifier_block panic_notifier = {
        .notifier_call = dtrace_kernel_panic,
};
#endif

/**********************************************************************/
/*   Externs.							      */
/**********************************************************************/
extern unsigned long long cnt_timer_add;
extern unsigned long long cnt_timer_remove;
extern unsigned long long cnt_syscall1;
extern unsigned long long cnt_syscall2;
extern unsigned long long cnt_syscall3;
extern unsigned long cnt_xcall0;
extern unsigned long cnt_xcall1;
extern unsigned long cnt_xcall2;
extern unsigned long cnt_xcall3;
extern unsigned long cnt_xcall4;
extern unsigned long cnt_xcall5;
extern unsigned long long cnt_xcall6;
extern unsigned long long cnt_xcall7;
extern unsigned long cnt_xcall8;
extern unsigned long cnt_nmi1;
extern unsigned long cnt_nmi2;
extern unsigned long long cnt_timer1;
extern unsigned long long cnt_timer2;
extern unsigned long long cnt_timer3;

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
void ctf_setup(void);
static void * par_lookup(void *ptr);
# define	cas32 dtrace_cas32
uint32_t dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new);
int	ctf_init(void);
void	ctf_exit(void);
int	ctl_init(void);
void	ctl_exit(void);
int	dcpc_init(void);
void	dcpc_exit(void);
int	dtrace_profile_init(void);
int	dtrace_profile_fini(void);
int	fasttrap_init(void);
void	fasttrap_exit(void);
int	fbt_init(void);
int	fbt_init2(void);
void	fbt_exit(void);
int	instr_init(void);
void	instr_exit(void);
int	intr_init(void);
void	intr_exit(void);
int	dtrace_prcom_init(void);
void	dtrace_prcom_exit(void);
int	sdt_init(void);
void	sdt_exit(void);
int	signal_init(void);
void	signal_fini(void);
int	systrace_init(void);
void	systrace_exit(void);
void	io_prov_init(void);
void	xcall_init(void);
void	xcall_fini(void);
//static void print_pte(pte_t *pte, int level);

/**********************************************************************/
/*   Avoid   problems   with  old  kernels  which  have  conflicting  */
/*   definitions.						      */
/**********************************************************************/
void
dtrace_clflush(void *ptr)
{
# if defined(__arm__)
	local_flush_tlb_kernel_page(ptr);
# else
        __asm__("clflush %0\n" : "+m" (*(char *)ptr));
# endif
}

/**********************************************************************/
/*   Return  the credentials of the current process. Solaris assumes  */
/*   we  are  embedded inside the proc/user struct, but in Linux, we  */
/*   have   different   linages.   Earlier   kernels   had  explicit  */
/*   uid/euid/...   fields,   and   from  2.6.29,  a  separate  cred  */
/*   structure. We need to hide that from the kernel.		      */
/*   								      */
/*   In  addition,  because  we  are  encapsulating,  we  need to be  */
/*   careful  of  SMP  systems - we cannot use a static, so we use a  */
/*   per-cpu   array  so  that  any  CPU  wont  disturb  the  cached  */
/*   credentials we are picking up.				      */
/**********************************************************************/
cred_t *
CRED()
{
	cred_t	*cr;
	/* FIXME: This is a hack */
#if 0
	cr = &cpu_cred[cpu_get_id()];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	cr->cr_uid = current->cred->uid;
	cr->cr_gid = current->cred->gid;
#else
	cr->cr_uid = current->uid;
	cr->cr_gid = current->gid;
#endif
//printk("get cred end %d %d\n", cr->cr_uid, cr->cr_gid);
#else
	cr = &cpu_cred[0];
#endif

	return cr;
}
# if 0
/**********************************************************************/
/*   Placeholder  code  --  we  need  to  avoid a linear tomax/xamot  */
/*   buffer...not just yet....					      */
/**********************************************************************/
typedef struct page_array_t {
	int	pa_num;
	caddr_t pa_array[1];
	} page_array_t;
/**********************************************************************/
/*   Allocate  large  buffer because kmalloc/vmalloc dont like multi  */
/*   megabyte allocs.						      */
/**********************************************************************/
page_array_t *
array_alloc(int size)
{	int	n = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	int	i;
	page_array_t *pap = kmalloc(sizeof *pap + (n - 1) * sizeof(caddr_t), GFP_KERNEL);

	if (pap == NULL)
		return NULL;

	pap->pa_num = n;
	for (i = 0; i < n; i++) {
		if ((pap->pa_array[i] == vmalloc(PAGE_SIZE)) == NULL) {
			while (--i >= 0)
				vfree(pap->pa_array[i]);
			kfree(pap);
			return NULL;
			}
		}
	return pap;
}
/**********************************************************************/
/*   Free up the page array.					      */
/**********************************************************************/
void
array_free(page_array_t *pap)
{	int	i = pap->pa_num;;

	while (--i >= 0)
		vfree(pap->pa_array[i]);
	kfree(pap);
}
# endif
void
atomic_add_64(uint64_t *p, int n)
{
	*p += n;
}
/**********************************************************************/
/*   We cannot call do_gettimeofday, or ktime_get_ts or any of their  */
/*   inferiors  because they will attempt to lock on a mutex, and we  */
/*   end  up  not being able to trace fbt::kernel:nr_active. We have  */
/*   to emulate a stable clock reading ourselves. 		      */
/*   								      */
/*   Later  Linux  kernels  hide  the  xtime_cache  data  inside the  */
/*   'timekeeper'     struct     (kernel/time/timekeeping.c).     So  */
/*   dtrace_linux_init() will attempt to find something.	      */
/*   								      */
/*   Brendan Giggs gives us this test case to prove if the mechanism  */
/*   is working properly.					      */
/*   								      */
/* dtrace -n 'syscall:::entry { self->ts = timestamp; }
    syscall:::return /self->ts/ { @["ns"] = quantize(timestamp - self->ts); }' */
/**********************************************************************/
hrtime_t
dtrace_gethrtime()
{
	struct timespec ts;

	/*
	void (*ktime_get_ts)() = get_proc_addr("ktime_get_ts");
	if (ktime_get_ts == NULL) return 0;
	ktime_get_ts(&ts);
	*/

	/***********************************************/
	/*   This seems to be the lowest level access  */
	/*   to tsc and return nsec.		       */
	/***********************************************/
	if (native_sched_clock_ptr) {
		return (*native_sched_clock_ptr)();
	}
	/***********************************************/
	/*   Later  kernels  use this to allow access  */
	/*   to the timespec in timekeeper.xtime.      */
	/***********************************************/
	if (__current_kernel_time_ptr) {
		ts = __current_kernel_time_ptr();
		return (hrtime_t) ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
	}

	/***********************************************/
	/*   TODO:  This  needs  to  be fixed: we are  */
	/*   reading  the  clock  which  needs a lock  */
	/*   around  the  access,  since  this  is  a  */
	/*   multiword  item,  and  we  may pick up a  */
	/*   partial update.			       */
	/***********************************************/
	if (!xtime_cache_ptr) {
		/***********************************************/
		/*   Most   likely   an  old  kernel,  so  do  */
		/*   something  reasonable.  We  cannot  call  */
		/*   do_gettimeofday()  which  we would like,  */
		/*   since  we may be in an interrupt and may  */
		/*   deadlock  whilst  it  waits for xtime to  */
		/*   settle down. We dont need hires accuracy  */
		/*   (or  do  we?), but we must not block the  */
		/*   kernel.				       */
		/***********************************************/
#if 0
		struct timeval tv;
		do_gettimeofday(&tv);
		return (hrtime_t) tv.tv_sec * 1000 * 1000 * 1000 + tv.tv_usec * 1000;
#else
		/***********************************************/
		/*   This  is  very  poor  - the Brendan Gigg  */
		/*   test  will  fail.  Let me know if we get  */
		/*   here.  It  wont stop dtrace from working  */
		/*   but it will be ugly.		       */
		/***********************************************/
		return 0;
#endif
	}

	ts = *xtime_cache_ptr;
	return (hrtime_t) ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;

}
uint64_t
dtrace_gethrestime()
{
	return dtrace_gethrtime();
}
void
vcmn_err(int ce, const char *fmt, va_list adx)
{
        char buf[256];

        switch (ce) {
        case CE_CONT:
                snprintf(buf, sizeof(buf), "Solaris(cont): %s\n", fmt);
                break;
        case CE_NOTE:
                snprintf(buf, sizeof(buf), "Solaris: NOTICE: %s\n", fmt);
                break;
        case CE_WARN:
                snprintf(buf, sizeof(buf), "Solaris: WARNING: %s\n", fmt);
                break;
        case CE_PANIC:
                snprintf(buf, sizeof(buf), "Solaris(panic): %s\n", fmt);
                break;
        case CE_IGNORE:
                break;
        default:
                panic("Solaris: unknown severity level");
        }
        if (ce == CE_PANIC)
                panic("%s", buf);
        if (ce != CE_IGNORE)
                printk(buf, adx);
}

void
cmn_err(int type, const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vcmn_err(type, fmt, ap);
        va_end(ap);
}
/**********************************************************************/
/*   We dont really call this, but Solaris would.		      */
/**********************************************************************/
void
debug_enter(char *arg)
{
	printk("%s(%d): %s\n", __FILE__, __LINE__, __func__);
}
/**********************************************************************/
/*   When  logging  HERE()  calls, dont bloat/slow us down with full  */
/*   path names, we only want to know which file its in.	      */
/**********************************************************************/
char *
dtrace_basename(char *name)
{	char	*cp = name + strlen(name);

	while (cp > name) {
		if (cp[-1] == '/')
			return cp;
		cp--;
	}
	return name;
}
/**********************************************************************/
/*   Avoid reentrancy issues, by defining our own bzero.	      */
/**********************************************************************/
void
dtrace_bzero(void *dst, int len)
{	char *d = dst;

	while (len-- > 0)
		*d++ = 0;
}

void
dtrace_dump_mem(char *cp, int len)
{	char	buf[128];
	int	i;

	while (len > 0) {
		sprintf(buf, "%p: ", cp);
		for (i = 0; i < 16 && len-- > 0; i++) {
			sprintf(buf + strlen(buf), "%02x ", *cp++ & 0xff);
			}
		strcat(buf, "\n");
		printk("%s", buf);
		}
}
/**********************************************************************/
/*   Dump memory in 32-bit chunks.				      */
/**********************************************************************/
void
dtrace_dump_mem32(int *cp, int len)
{	char	buf[128];
	int	i;

	while (len > 0) {
		sprintf(buf, "%p: ", cp);
		for (i = 0; i < 8 && len-- > 0; i++) {
			sprintf(buf + strlen(buf), "%08x ", *cp++);
			}
		strcat(buf, "\n");
		printk("%s", buf);
		}
}
void
dtrace_dump_mem64(unsigned long *cp, int len)
{	char	buf[128];
	int	i;

	while (len > 0) {
		sprintf(buf, "%p: ", cp);
		for (i = 0; i < 4 && len-- > 0; i++) {
			sprintf(buf + strlen(buf), "%p ", (void *) *cp++);
			}
		strcat(buf, "\n");
		dtrace_printf("%s", buf);
		}
}
/**********************************************************************/
/*   Return  the  datamodel (64b/32b) mode of the underlying binary.  */
/*   Linux  doesnt  seem  to mark a proc as 64/32, but relies on the  */
/*   class  vtable  for  the  underlying  executable  file format to  */
/*   handle  this in an OO way. Needed in fasttrap to work out which  */
/*   disassembler to use when computing instruction sizes.	      */
/**********************************************************************/
int
dtrace_data_model(proc_t *p)
{
# if defined(__i386)
	return DATAMODEL_ILP32;
# elif defined(__arm__)
	return DATAMODEL_ILP32;
# else
	/***********************************************/
	/*   Could  be  32 or 64 bit. We will use the  */
	/*   ELFCLASS  to  determine what this really  */
	/*   is.				       */
	/***********************************************/
	struct mm_struct *mm;
	struct vm_area_struct *vma;

	if ((mm = p->p_task->mm) == NULL)
		return DATAMODEL_LP64;
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		char	buf[EI_CLASS + 1];
		long	flags;

		if ((vma->vm_flags & VM_EXEC) == 0)
			continue;

		/***********************************************/
		/*   This  is  ugly - access_process_vm wants  */
		/*   to be called with interrupts enabled. We  */
		/*   are  going  to  be  coming  from an INT3  */
		/*   interrupt  (fasttrap  trace),  with ints  */
		/*   disabled.  In  theory  we can enable the  */
		/*   interrupts,  but  we  dont  to avoid the  */
		/*   interrupt  routine  triggering  a nested  */
		/*   exception. So, we open a small window to  */
		/*   let  the  interrupt  through.  It doesnt  */
		/*   matter  that  this  happens,  unless the  */
		/*   kernel is heavily probed and we do start  */
		/*   getting  nested  interrupts. This should  */
		/*   be ok, in the worst case.		       */
		/***********************************************/
		
		flags = dtrace_interrupt_get();
		asm("sti\n");
		uread(p, (void *) buf, (size_t) EI_CLASS + 1, (uintptr_t) vma->vm_start);
		dtrace_interrupt_enable(flags);

		if (*buf != 0x7f ||
		    buf[1] != 'E' ||
		    buf[2] != 'L' || 
		    buf[3] != 'F')
			continue;
		if (buf[EI_CLASS] == ELFCLASS64) {
			return DATAMODEL_LP64;
		}
		return DATAMODEL_ILP32;
	}
	return DATAMODEL_LP64;
# endif
}

/**********************************************************************/
/*   This  gets  called  once we have been told what missing symbols  */
/*   are  available,  so we can do initiatisation. load.pl loads the  */
/*   kernel  and  then  when  we  are  happy,  we  can  complete the  */
/*   initialisation.						      */
/**********************************************************************/
static void
dtrace_linux_init(void)
{	hrtime_t	t, t1;

	if (driver_initted)
		return;
	driver_initted = TRUE;

	/***********************************************/
	/*   Let  timers  grab  any symbols they need  */
	/*   (eg hrtimer).			       */
	/***********************************************/
	init_cyclic();

	/***********************************************/
	/*   Useful for debugging.		       */
	/***********************************************/
	fn_sysrq_showregs_othercpus = get_proc_addr("sysrq_showregs_othercpus");

	/***********************************************/
	/*   Used by prfind.			       */
	/***********************************************/
	fn_pid_task = get_proc_addr("pid_task");
	fn_find_get_pid = get_proc_addr("find_get_pid");

	/***********************************************/
	/*   Initialise the interrupt vectors.	       */
	/***********************************************/
	
	/***********************************************/
	/*   Needed   for  validating  module  symbol  */
	/*   probes.				       */
	/***********************************************/
	fn__module_text_address = get_proc_addr("__module_text_address");

	/***********************************************/
	/*   Needed  for  stack  walking  to validate  */
	/*   addresses on the stack.		       */
	/***********************************************/
	kernel_text_address_fn = get_proc_addr("kernel_text_address");

	/***********************************************/
	/*   Needed by sdt probes.		       */
	/***********************************************/
	dentry_path_fn = get_proc_addr("dentry_path");

	/***********************************************/
	/*   Register proc exit hook.		       */
	/***********************************************/
	fn_profile_event_register = 
		(int (*)(enum profile_type type, struct notifier_block *n))
			get_proc_addr("profile_event_register");
	fn_profile_event_unregister = 
		(int (*)(enum profile_type type, struct notifier_block *n))
			get_proc_addr("profile_event_unregister");

	/***********************************************/
	/*   We  need to intercept proc death in case  */
	/*   we are tracing it.			       */
	/***********************************************/
	if (fn_profile_event_register) {
		fn_profile_event_register(PROFILE_TASK_EXIT, &n_exit);
	}

	/***********************************************/
	/*   Get  the pointer to one of the key (lock  */
	/*   free)  areas  where  time may be stored.  */
	/*   This changed across the kernels.	       */
	/***********************************************/
	xtime_cache_ptr = (struct timespec *) get_proc_addr("xtime_cache");
	if (xtime_cache_ptr == NULL)
		xtime_cache_ptr = (struct timespec *) get_proc_addr("xtime");
	__current_kernel_time_ptr = (struct timespec (*)(void)) get_proc_addr("__current_kernel_time");
	native_sched_clock_ptr = get_proc_addr("native_sched_clock");

	/***********************************************/
	/*   Dirty    code:   in   3.4   and   above,  */
	/*   xtime_cache  is  hiding inside a private  */
	/*   struct.  We  replicate the first part of  */
	/*   the  struct here to get to the location.  */
	/*   Without   this,  DIF_VAR_TIMESTAMP  wont  */
	/*   return a useful value.		       */
	/***********************************************/
	if (xtime_cache_ptr == NULL) {
		char *p = get_proc_addr("timekeeper");
		if (p) {
			/***********************************************/
			/*   See kernel/time/timekeeping.c	       */
			/***********************************************/
			struct timekeeper_hack {
				void *clock;
				u32 mult;
				int shift;
				u64 cycle_interval;
				u64 xtime_interval;
				s64 xtime_remainder;
				u32 raw_interval;
				u64 xtime_nsec;
				s64 ntp_error;
				int ntp_error_shift;
				};
			xtime_cache_ptr = (struct timespec *) (p + sizeof(struct timekeeper_hack));
		}
	}
# if defined(__arm__)
	ktime_get_ptr = (ktime_t (*)(void)) get_proc_addr("ktime_get");
	# define rdtscll(t) t = ktime_get_ptr().tv64
	# define __flush_tlb_all() local_flush_tlb_all()
	# define _PAGE_NX 0
	# define _PAGE_RW 0
# endif
	preempt_disable();
	rdtscll(t);
	(void) dtrace_gethrtime();
	rdtscll(t1);
	tsc_max_delta = t1 - t;
	preempt_enable();

	/***********************************************/
	/*   Let  us  grab  the  panics  if we are in  */
	/*   debug mode.			       */
	/***********************************************/
#if HAVE_ATOMIC_NOTIFIER_CHAIN_REGISTER && 0
        if (grab_panic)
               atomic_notifier_chain_register(&panic_notifier_list, &panic_notifier);
#endif
}
/**********************************************************************/
/*   Cleanup notifications before we get unloaded.		      */
/**********************************************************************/
static int
dtrace_linux_fini(void)
{	int	ret = 1;

	/***********************************************/
	/*   If kernel doesnt have this enabled, then  */
	/*   we  wont  have  set  up the notifier and  */
	/*   cannot    detect    new    procs   being  */
	/*   created/exiting.			       */
	/***********************************************/
	if (fn_profile_event_unregister == NULL &&
	    fn_profile_event_register == NULL)
	    	return 1;

	if (fn_profile_event_unregister) {
		int pret = (*fn_profile_event_unregister)(PROFILE_TASK_EXIT, &n_exit);
		if (pret) {
			printk("profile_event_unregister=%d\n", pret);
			return -1;
		}
	} else {
		printk(KERN_WARNING "dtracedrv: Cannot call profile_event_unregister\n");
		ret = 0;
	}

	/***********************************************/
	/*   Stop  the IPI interrupt from firing (and  */
	/*   hanging  dtrace_xcall)  as we unload the  */
	/*   driver.				       */
	/***********************************************/
	driver_initted = FALSE;

#if HAVE_ATOMIC_NOTIFIER_CHAIN_REGISTER && 0
        if (grab_panic)
               atomic_notifier_chain_register(&panic_notifier_list, &panic_notifier);
#endif
	return ret;
}
/**********************************************************************/
/*   Utility function to dump stacks for all cpus.		      */
/**********************************************************************/
volatile int dumping;

static void dump_cpu_stack(void)
{
	while (dumping)
		;
	dumping = 1;
	printk("This is CPU#%d\n", smp_processor_id());
	dump_stack();
	dumping = 0;
}
void
dump_all_stacks(void)
{
static void (*arch_trigger_all_cpu_backtrace)(void);
	/***********************************************/
	/*   arch_trigger_all_cpu_backtrace     works  */
	/*   using NMI and will work even if the CPUs  */
	/*   have  interrupts  disabled. Our IPI call  */
	/*   wont work.				       */
	/***********************************************/
	if (arch_trigger_all_cpu_backtrace == NULL)
		arch_trigger_all_cpu_backtrace = get_proc_addr("arch_trigger_all_cpu_backtrace");
	if (0 && arch_trigger_all_cpu_backtrace)
		arch_trigger_all_cpu_backtrace();
	else {
//		set_console_on(0);
//		dump_stack();
		SMP_CALL_FUNCTION((void (*)(void *)) dump_cpu_stack, 0, FALSE);
	}
}
/**********************************************************************/
/*   Call   here  to  disarm  dtrace  so  we  can  debug  unexpected  */
/*   scenarios. In production dtrace, nothing calls this, but we may  */
/*   put temporary enablers in, for example the GPF handler. We dont  */
/*   panic, but we set dtrace_shutdown() to avoid dtrace causing any  */
/*   more additional damage.					      */
/**********************************************************************/
void
dtrace_linux_panic(const char *fmt, ...)
{	va_list	ap;

	if (dtrace_shutdown)
		return;

	dtrace_shutdown = TRUE;
	set_console_on(1);
	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
	printk("[cpu%d] dtrace_linux_panic called. Dumping cpu stacks\n", smp_processor_id());
	dump_all_stacks();
	printk("dtrace_linux_panic: finished\n");
}
/**********************************************************************/
/*   CPU specific - used by profiles.c to handle amount of frames in  */
/*   the clock ticks.						      */
/**********************************************************************/
int
dtrace_mach_aframes(void)
{
	return 1;
}

/**********************************************************************/
/*   Avoid  calling  real  memcpy,  since  we  will  call  this from  */
/*   interrupt context.						      */
/**********************************************************************/
void
dtrace_memcpy(void *dst, const void *src, int len)
{	char	*d = dst;
	const char	*s = src;

	while (len-- > 0)
		*d++ = *s++;
}
/**********************************************************************/
/*   Private  implementation  of  memchr() so we can avoid using the  */
/*   public one, so we could probe it if we want.		      */
/**********************************************************************/
char *
dtrace_memchr(const char *buf, int c, int len)
{
	while (len-- > 0) {
		if (*buf++ == c)
			return (char *) buf - 1;
		}
	return NULL;
}
# if 0
/**********************************************************************/
/*   Debug code for printing a gate.				      */
/**********************************************************************/
static void 
dtrace_print_gate(struct gate_struct *g)
{
	dtrace_dump_mem64(g, 2);
	printk("offset_low=%x seg=%x ist=%d type=%d dpl=%d p=%d\n",
		g->offset_low, g->segment, g->ist, g->type, g->dpl, g->p);
	printk("offset_mid=%x offset_hi=%x\n", g->offset_middle, g->offset_high);
}
# endif
# if 1
/**********************************************************************/
/*   For debugging only.					      */
/**********************************************************************/
void
dtrace_print_regs(struct pt_regs *regs)
{
	printk("Regs @ %p..%p CPU:%d\n", regs, regs+1, cpu_get_id());
# if defined(__amd64)
        printk("r15:%p r14:%p r13:%p r12:%p\n",
		(void *) regs->r15,
	        (void *) regs->r14,
	        (void *) regs->r13,
	        (void *) regs->r12);
        printk("rbp:%p rbx:%p r11:%p r10:%p\n",
	        (void *) regs->r_rbp,
	        (void *) regs->r_rbx,
	        (void *) regs->r11,
	        (void *) regs->r10);
        printk("r9:%p r8:%p rax:%p rcx:%p\n",
	        (void *) regs->r9,
	        (void *) regs->r8,
	        (void *) regs->r_rax,
	        (void *) regs->r_rcx);
        printk("rdx:%p rsi:%p rdi:%p orig_rax:%p\n",
	        (void *) regs->r_rdx,
	        (void *) regs->r_rsi,
	        (void *) regs->r_rdi,
	        (void *) regs->r_trapno);
        printk("rip:%p cs:%p eflags:%p %s%s\n",
	        (void *) regs->r_pc,
	        (void *) regs->cs,
	        (void *) regs->r_rfl,
		regs->r_rfl & X86_EFLAGS_TF ? "TF " : "",
		regs->r_rfl & X86_EFLAGS_IF ? "IF " : "");
        printk("rsp:%p ss:%p ",
	        (void *) regs->r_sp,
	        (void *) regs->r_ss);
        printk(" %p %p\n",
		((void **) regs->r_sp)[0],
		((void **) regs->r_sp)[1]);
# endif

}
# endif
/**********************************************************************/
/*   Make this public so that xcall can short-circuit the call if we  */
/*   are safe.							      */
/**********************************************************************/
/*static*/ void
dtrace_sync_func(void)
{
}

/**********************************************************************/
/*   Synchronise  the cpus. This really means make sure they are not  */
/*   in  a critical section. The source of so much heartache for me.  */
/*   But,  allow us to make life really bad by ramping up the repeat  */
/*   count.							      */
/**********************************************************************/
void
dtrace_sync(void)
{	int	i;
	/***********************************************/
	/*   Set  this  to 100 or 200 to see how well  */
	/*   the code in x_call.c works. You will see  */
	/*   occasional reentrancy issues (not sure I  */
	/*   fully  understand  why, but you can work  */
	/*   it out when two cpus are trying to cross  */
	/*   call each other and timer interrupts are  */
	/*   firing  and  doing  xcalls also. Not too  */
	/*   bad  -  but  enough to maybe lock one or  */
	/*   more cpus for a short while.	       */
	/***********************************************/
	int	repeat = 1;

	for (i = 0; i < repeat; i++) {
	        dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
	}
}
void
dtrace_vtime_enable(void)
{
	dtrace_vtime_state_t state, nstate = 0;

	do {
		state = dtrace_vtime_active;

		switch (state) {
		case DTRACE_VTIME_INACTIVE:
			nstate = DTRACE_VTIME_ACTIVE;
			break;

		case DTRACE_VTIME_INACTIVE_TNF:
			nstate = DTRACE_VTIME_ACTIVE_TNF;
			break;

		case DTRACE_VTIME_ACTIVE:
		case DTRACE_VTIME_ACTIVE_TNF:
			panic("DTrace virtual time already enabled");
			/*NOTREACHED*/
		}

	} while	(cas32((uint32_t *)&dtrace_vtime_active,
	    state, nstate) != state);
}

void
dtrace_vtime_disable(void)
{
	dtrace_vtime_state_t state, nstate = 0;

	do {
		state = dtrace_vtime_active;

		switch (state) {
		case DTRACE_VTIME_ACTIVE:
			nstate = DTRACE_VTIME_INACTIVE;
			break;

		case DTRACE_VTIME_ACTIVE_TNF:
			nstate = DTRACE_VTIME_INACTIVE_TNF;
			break;

		case DTRACE_VTIME_INACTIVE:
		case DTRACE_VTIME_INACTIVE_TNF:
			panic("DTrace virtual time already disabled");
			/*NOTREACHED*/
		}

	} while	(cas32((uint32_t *)&dtrace_vtime_active,
	    state, nstate) != state);
}


/**********************************************************************/
/*   Needed by systrace.c					      */
/**********************************************************************/
void *
fbt_get_sys_call_table(void)
{
	return xsys_call_table;
}
void *
fbt_get_ia32_sys_call_table(void)
{
	return xia32_sys_call_table;
}
void *
fbt_get_access_process_vm(void)
{
	return (void *) syms[OFFSET_access_process_vm].m_ptr;
}
int
fulword(const void *addr, uintptr_t *valuep)
{
	if (!validate_ptr((void *) addr))
		return 1;

	*valuep = dtrace_fulword((void *) addr);
	return 0;
}
int
fuword8(const void *addr, unsigned char *valuep)
{
	if (!validate_ptr((void *) addr))
		return 1;

	*valuep = dtrace_fuword8((void *) addr);
	return 0;
}
int
fuword32(const void *addr, uint32_t *valuep)
{
	if (!validate_ptr((void *) addr))
		return 1;

	*valuep = dtrace_fuword32((void *) addr);
	return 0;
}
struct module *
get_module(int n)
{	struct module *modp;
	struct list_head *head = (struct list_head *) xmodules;

//printk("get_module(%d) head=%p\n", n, head);
	if (head == NULL)
		return NULL;

	list_for_each_entry(modp, head, list) {
		if (n-- == 0) {
			return modp;
		}
	}
	return NULL;
}
/**********************************************************************/
/*   Interface kallsyms_lookup_name.				      */
/**********************************************************************/
void *
get_proc_addr(char *name)
{	void	*ptr;
	struct map *mp;
static int count;

	if (xkallsyms_lookup_name == NULL) {
		if (1 || dtrace_here)
			printk("get_proc_addr: No value for xkallsyms_lookup_name (%s)\n", name);
		return 0;
		}

	ptr = (void *) (*xkallsyms_lookup_name)(name);
	if (ptr) {
		if (dtrace_here)
			printk("get_proc_addr(%s) := %p\n", name, ptr);
		return ptr;
	}
	/***********************************************/
	/*   Some    symbols   may   originate   from  */
	/*   /boot/System.map, so try these out.       */
	/***********************************************/
	for (mp = syms; mp->m_name; mp++) {
		if (strcmp(name, mp->m_name) == 0) {
			if (dtrace_here)
				printk("get_proc_addr(%s, internal) := %p\n", name, mp->m_ptr);
			return mp->m_ptr;
		}
	}
	if (count < 100) {
		count++;
		printk("dtrace_linux.c:get_proc_addr: Failed to find '%s' (warn=%d)\n", name, count);
	}
	return NULL;
}
/**********************************************************************/
/*   Convert an address to a function name (if possible).	      */
/**********************************************************************/
int
get_proc_name(unsigned long addr, char *buf)
{
	unsigned long symsize = 0;
	unsigned long offset;
	char	*modname = NULL;
static	const char *(*my_kallsyms_lookup)(unsigned long addr,
                        unsigned long *symbolsize,
                        unsigned long *offset,
                        char **modname, char *namebuf);

	*buf = '\0';
	if (my_kallsyms_lookup == NULL)
		my_kallsyms_lookup = get_proc_addr("kallsyms_lookup");
	if (my_kallsyms_lookup == NULL)
		return 0;
	my_kallsyms_lookup(addr, &symsize, &offset, &modname, buf);
	return symsize;
}
int
sulword(const void *addr, ulong_t value)
{
	if (!validate_ptr((void *) addr))
		return -1;

	*(ulong_t *) addr = value;
	return 0;
}
int
suword32(const void *addr, uint32_t value)
{
	if (!validate_ptr((void *) addr))
		return -1;

	*(uint32_t *) addr = value;
	return 0;
}
/**********************************************************************/
/*   Given  a symbol we got from a module, is the symbol pointing to  */
/*   a  valid  address? We might have jettisoned the .init sections,  */
/*   so avoid GPF if symbol is not in the .text section.	      */
/*   								      */
/*   Complexity  here  for older pre-2.6.19 kernels since the module  */
/*   private area changed.					      */
/**********************************************************************/
int
instr_in_text_seg(struct module *mp, char *name, Elf_Sym *sym)
{
	struct module *mp1;

	/***********************************************/
	/*   Make   sure  we  have  the  prerequisite  */
	/*   function.				       */
	/***********************************************/
	if (fn__module_text_address == NULL)
		return 0;

	/***********************************************/
	/*   Map addr to a module - should be the one  */
	/*   we are looking at.			       */
	/***********************************************/
	mp1 = fn__module_text_address(sym->st_value);
//printk("fn__module_text_address=%p %p %p\n", fn__module_text_address, sym->st_value, mp1);
//printk("mp=%p %p\n", mp, mp1);

	return mp == mp1;
}
/**********************************************************************/
/*   When  walking  stacks, need to see if this is a kernel pointer,  */
/*   to avoid garbage on the stack.				      */
/**********************************************************************/
extern caddr_t ktext;
extern caddr_t ketext;
int
is_kernel_text(unsigned long p)
{static int first_time = TRUE;
static caddr_t stext;
static caddr_t etext;
	
//printk("ktext=%p..%p %p\n", ktext, ketext, p);
	if (ktext <= (caddr_t) p && (caddr_t) p < ketext)
		return 1;
	if (first_time) {
		first_time = FALSE;
		stext = get_proc_addr("_stext");
		etext = get_proc_addr("_etext");
	}
	if (stext <= (caddr_t) p && (caddr_t) p < etext)
		return 1;

	if (kernel_text_address_fn) {
		return kernel_text_address_fn(p);
	}
	return 0;
}

/**********************************************************************/
/*   Function  for  validating  we  dont  have  IRQs  disabled  on a  */
/*   sleeping kalloc.						      */
/**********************************************************************/
void
km_check(unsigned long flags)
{
	if (irqs_disabled() && flags == KM_NOSLEEP) {
		dtrace_printf("sleeping with ints disabled\n");
		dump_stack();
	}
}

# define	VMALLOC_SIZE	(100 * 1024)
void *
kmem_alloc(size_t size, int flags)
{	void *ptr;

//km_check(flags);
	if (size > VMALLOC_SIZE) {
		ptr = vmalloc(size);
	} else {
		ptr = kmalloc(size, flags);
	}
	if (TRACE_ALLOC || dtrace_mem_alloc)
		dtrace_printf("kmem_alloc(%d) := %p ret=%p\n", (int) size, ptr, __builtin_return_address(0));
	return ptr;
}
void *
kmem_zalloc(size_t size, int flags)
{	void *ptr;

//km_check(flags);
	if (size > VMALLOC_SIZE) {
		ptr = vmalloc(size);
		if (ptr)
			bzero(ptr, size);
	} else {
		ptr = kzalloc(size, flags);
	}
	if (TRACE_ALLOC || dtrace_mem_alloc)
		dtrace_printf("kmem_zalloc(%d) := %p\n", (int) size, ptr);
	return ptr;
}
void
kmem_free(void *ptr, int size)
{
	if (TRACE_ALLOC || dtrace_mem_alloc)
		dtrace_printf("kmem_free(%p, size=%d)\n", ptr, size);
	if (size > VMALLOC_SIZE)
		vfree(ptr);
	else
		kfree(ptr);
}

int
lx_get_curthread_id()
{
	return get_current()->pid;
}

/**********************************************************************/
/*   Test if a pointer is vaid in kernel space.			      */
/**********************************************************************/
# if defined(__i386)
#define __validate_ptr(ptr, ret)      \
 __asm__ __volatile__(      	      \
  "  mov $1, %0\n" 		      \
  "0: mov (%1), %1\n"                 \
  "2:\n"       			      \
  ".section .fixup,\"ax\"\n"          \
  "3: mov $0, %0\n"    	              \
  " jmp 2b\n"     		      \
  ".previous\n"      		      \
  ".section __ex_table,\"a\"\n"       \
  " .align 4\n"     		      \
  " .long 0b,3b\n"     		      \
  ".previous"      		      \
  : "=a" (ret) 		              \
  : "c" (ptr) 	                      \
  )
# elif defined(__amd64)
#define __validate_ptr(ptr, ret)      \
 __asm__ __volatile__(      	      \
  "  mov $1, %0\n" 		      \
  "0: mov (%1), %1\n"                 \
  "2:\n"       			      \
  ".section .fixup,\"ax\"\n"          \
  "3: mov $0, %0\n"    	              \
  " jmp 2b\n"     		      \
  ".previous\n"      		      \
  ".section __ex_table,\"a\"\n"       \
  " .align 8\n"     		      \
  " .quad 0b,3b\n"     		      \
  ".previous"      		      \
  : "=a" (ret) 		      	      \
  : "c" (ptr) 	                      \
  )
# elif defined(__arm__)
#define __validate_ptr(ptr, ret)      ret = 0
# endif

/**********************************************************************/
/*   Solaris  rdmsr  routine  only takes one arg, so we need to wrap  */
/*   the call. Called from fasttrap_isa.c. Not needed for ARM.	      */
/**********************************************************************/
# if defined(__i386) || defined(__amd64)
u64
lx_rdmsr(int x)
{	u32 val1, val2;

	rdmsr(x, val1, val2);
	return ((u64) val2 << 32) | val1;
}
# endif

int
validate_ptr(const void *ptr)
{	int	ret;

	__validate_ptr(ptr, ret);
//	printk("validate: ptr=%p ret=%d\n", ptr, ret);

	return ret;
}

/**********************************************************************/
/*   Used  for  probing  memory,  especially dtrace_casptr, to avoid  */
/*   paniccing kernel when we are debugging the page table stuff.     */
/**********************************************************************/
int
mem_is_writable(volatile char *addr)
{	int	ret = 1;

	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);

	*addr = *addr;

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_BADADDR))
		ret = 0;
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT | CPU_DTRACE_BADADDR);
	return ret;
}

# if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 11)
	/***********************************************/
	/*   Note  sure  which  version of the kernel  */
	/*   this  came in on, but we need it for AS4  */
	/*   - 2.6.9 plus lots of patches. 	       */
	/***********************************************/
	typedef struct { unsigned long pud; } pud_t;
# endif
typedef struct page_perms_t {
	int	pp_valid;
	unsigned long pp_addr;
	pgd_t	pp_pgd;
	pud_t	pp_pud;
	pmd_t	pp_pmd;
	pte_t	pp_pte;
	} page_perms_t;
/**********************************************************************/
/*   Set  a memory page to be writable, so we can patch it. Save the  */
/*   info so we can undo the changes afterwards. This is for x86-64,  */
/*   since  it  has  a 4 level page table. Every part of the walk to  */
/*   the  final PTE needs to be writable, not just the entry itself.  */
/*   Reference:							      */
/*   http://www.intel.com/design/processor/applnots/317080.pdf	      */
/**********************************************************************/
static int
mem_set_perms(unsigned long addr, page_perms_t *pp, unsigned long and_perms, unsigned long or_perms)
{
# if defined(__arm__)
	printk("mem_set_perms: please implement(arm)\n");

	return 0;
# else
/**********************************************************************/
/*   Following  code  is Xen/paravirt safe. Xen wont let us directly  */
/*   touch  the  page  table,  so  we  use  the  appropriate wrapper  */
/*   functions, and this handles paravirt or native hardware.	      */
/**********************************************************************/
static int first_time = TRUE;
static pte_t *(*lookup_address)(void *, int *);
        pte_t *kpte;
        pte_t old_pte;
        pte_t new_pte;
        pgprot_t new_prot;
        int level;
        unsigned long pfn;

        if (first_time) {
               lookup_address = get_proc_addr("lookup_address");
               first_time = FALSE;
        }

        addr = (unsigned long) addr & ~(PAGESIZE-1);
        kpte = lookup_address((void *) addr, &level);
        old_pte = *kpte;
        new_prot = pte_pgprot(old_pte);
        pgprot_val(new_prot) |= _PAGE_RW;
	pgprot_val(new_prot) &= ~_PAGE_NX;

        pfn = pte_pfn(old_pte);

        /***********************************************/
        /*   We  want  pfn_pte()  but that causes GPL  */
        /*   linking  issues,  so  inline  the actual  */
        /*   function.                                 */
        /***********************************************/
//      new_pte = pfn_pte(pfn, new_prot);
        new_pte = __pte(((phys_addr_t)pfn << PAGE_SHIFT) | pgprot_val(new_prot));


#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
	*(pte_t *) &pp->pp_pte = new_pte; /* Horror for <= 2.6.24 kernels */
#else
	/***********************************************/
	/*   This   is  for  Xen  kernels  which  may  */
	/*   virtualise the PTE tables.		       */
	/***********************************************/
        set_pte_atomic(kpte, new_pte);
#endif

        return 1;

//#if defined(__i386) && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
//	typedef struct { unsigned long long pte; } pte_t;
//#	undef pte_none
//#	define pte_none(p) (p).pte == 0
//#endif
//	int	ret = 1;
//	/***********************************************/
//	/*   Dont   think  we  need  this  for  older  */
//	/*   kernels  where  everything  is writable,  */
//	/*   e.g. sys_call_table.		       */
//	/***********************************************/
//# if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16)
//	pgd_t *pgd = pgd_offset(current->mm, addr);
//	pud_t *pud;
//	pmd_t *pmd;
//	pte_t *pte;
//	unsigned long *perms1;
//#define dump_tree FALSE
//
//	pp->pp_valid = FALSE;
//	pp->pp_addr = addr;
//
//	if (pgd_none(*pgd)) {
////		printk("yy: pgd=%lx\n", *pgd);
//		return 0;
//	}
//	pud = pud_offset(pgd, addr);
//	if (pud_none(*pud)) {
////		printk("yy: pud=%lx\n", *pud);
//		return 0;
//	}
//	pmd = pmd_offset(pud, addr);
//	if (pmd_none(*pmd)) {
////		printk("yy: pmd=%lx\n", *pmd);
//		return 0;
//	}
//
//	/***********************************************/
//	/*   20091223  Soumendu  Sekhar Satapathy: If  */
//	/*   large  memory  system,  then use the pmd  */
//	/*   directly.				       */
//	/***********************************************/
//	if (pmd_large(*pmd)) {
//		pte = (pte_t *) pmd;
//	} else {
//		pte = pte_offset_kernel(pmd, addr);
//	}
//	if (pte_none(*pte)) {
////printk("none pte\n");
//		return 0;
//	}
//
//# if dump_tree
//	{void print_pte(pte_t *pte, int level);
//	printk("yy -- begin\n");
//	print_pte((pte_t *) pgd, 0);
//	print_pte((pte_t *) pmd, 1);
//	print_pte((pte_t *) pud, 2);
//	print_pte(pte, 3);
//	printk("yy -- end\n");
//	}
//# endif
//
//	pp->pp_valid = TRUE;
//	pp->pp_pgd = *pgd;
//	pp->pp_pud = *pud;
//	pp->pp_pmd = *pmd;
//	*(pte_t *) &pp->pp_pte = *pte; /* Horror for <= 2.6.24 kernels */
//
//	/***********************************************/
//	/*   Avoid  touching/flushing  page  table if  */
//	/*   this is a no-op.			       */
//	/***********************************************/
//# if defined(__i386) && !defined(CONFIG_X86_PAE)
//	perms1 = &pmd->pud.pgd.pgd;
//# else
//	perms1 = &pmd->pmd;
//# endif
//	/***********************************************/
//	/*   Ensure we flush the page table, else our  */
//	/*   vmplayer/centos-2.6.18-64b   will  crash  */
//	/*   when doing syscall tracing.	       */
//	/***********************************************/
//	dtrace_clflush(&pp->pp_pte);
//
//	if (((*perms1 & and_perms) | or_perms) != *perms1 ||
//	    ((pte->pte & and_perms) | or_perms) != pte->pte) {
//		*perms1 = (*perms1 & and_perms) | or_perms;
//
//		/***********************************************/
//		/*   Make  page  executable. Ideally we would  */
//		/*   pass in the and+or perms to set.	       */
//		/***********************************************/
//		pte->pte = (pte->pte & and_perms) | or_perms;
//
//		/***********************************************/
//		/*   clflush only on >=2.6.28 kernels.	       */
//		/***********************************************/
//		dtrace_clflush(pmd);
//		dtrace_clflush(pte);
//		ret = 2;
//	}
//# endif
//	return ret;
#endif
}
/**********************************************************************/
/*   Invoked  by  pid  provider (fastrap_isa.c) when poking into the  */
/*   stack  page  for an emulated instruction. We might cross a page  */
/*   boundary, so handle this.					      */
/**********************************************************************/
void
set_page_prot(unsigned long addr, int len, long and_prot, long or_prot)
{	page_perms_t perms;
	int	ret = 0;

	addr &= ~(PAGESIZE - 1);
	while (len > 0) {
		int	r;
		len -= PAGESIZE;
		r = mem_set_perms(addr, &perms, and_prot, or_prot);
		if (r > ret)
			ret = r;
	}
	/***********************************************/
	/*   If  we  changed something update the TLB  */
	/*   state,  else  we  can get a segmentation  */
	/*   violation in the pid provider as we poke  */
	/*   the  emulated instruction into the stack  */
	/*   area.  Try  and  avoid full tlb flush if  */
	/*   the  page(s)  are already in the correct  */
	/*   state.				       */
	/***********************************************/
	if (ret == 2)
		__flush_tlb_all();
}
/**********************************************************************/
/*   Undo  the  patching of the page table entries after making them  */
/*   writable.							      */
/**********************************************************************/
# if 0 /* not used yet ... */
static int
mem_unset_writable(page_perms_t *pp)
{	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	if (!pp->pp_valid)
		return FALSE;

	pgd = pgd_offset_k(pp->pp_addr);
	pud = pud_offset(pgd, pp->pp_addr);
	pmd = pmd_offset(pud, pp->pp_addr);
	pte = pte_offset_kernel(pmd, pp->pp_addr);

	*pgd = pp->pp_pgd;
	*pud = pp->pp_pud;
	*pmd = pp->pp_pmd;
	*pte = pp->pp_pte;

	dtrace_clflush(pmd);
	dtrace_clflush(pte);
	return TRUE;
}
# endif

/**********************************************************************/
/*   i386:							      */
/*   Set  an  address  to be writeable. Need this because kernel may  */
/*   have  R/O sections. Using linux kernel apis is painful, because  */
/*   they  keep  changing  or  do  the wrong thing. (Or, I just dont  */
/*   understand  them  enough,  which  is  more  likely). This 'just  */
/*   works'.							      */
/*   								      */
/*   __amd64:							      */
/*   In  2.6.24.4  and  related kernels, x86-64 isnt page protecting  */
/*   the sys_call_table. Dont know why -- maybe its a bug and we may  */
/*   have to revisit this later if they turn it back on. 	      */
/**********************************************************************/
int
memory_set_rw(void *addr, int num_pages, int is_kernel_addr)
{	int	i;

#if defined(__i386xxx)
	/***********************************************/
	/*   Use  the 64b mechanism, for commonality.  */
	/*   We  have problems with set_memory_rw and  */
	/*   ensuring fbt + syscall work together.     */
	/***********************************************/
	int level;
	pte_t *pte;
static int first_time = TRUE;

static pte_t *(*lookup_address)(void *, int *);
static int (*set_memory_rw)(unsigned long, int);

	if (first_time) {
		set_memory_rw = get_proc_addr("set_memory_rw");
		lookup_address = get_proc_addr("lookup_address");
		first_time = FALSE;
	}

	/***********************************************/
	/*   Cant     use     set_memory_rw     since  */
	/*   pageattr.c/static_protections, will stop  */
	/*   us  making  the range [__start_rodata ..  */
	/*   __end_rodata] from being made writable.   */
	/*   Will panic if you do syscall:::	       */
	/***********************************************/
	if (set_memory_rw) {
		int	ret;
		addr = (void *) ((unsigned long) addr & PAGE_MASK);
		ret = set_memory_rw((unsigned long) addr, num_pages);
printk("set_memory_rw(%p, %d) := %d\n", addr, num_pages, ret);
		return 1;
		}

	if (lookup_address == NULL) {
		printk("dtrace:systrace.c: sorry - cannot locate lookup_address()\n");
		return 0;
		}

	for (i = 0; i <= num_pages; i++) {
		pte = lookup_address(addr, &level);
		if ((pte_val(*pte) & _PAGE_RW) == 0) {
# if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
			pte->pte_low |= _PAGE_RW;
# else
			pte->pte |= _PAGE_RW;
# endif

			/***********************************************/
			/*   If  we  touch  the page mappings, ensure  */
			/*   cpu  and  cpu caches know what happened,  */
			/*   else we may have random GPFs as we go to  */
			/*   do   a  write.  If  this  function  isnt  */
			/*   available,   we   are   pretty  much  in  */
			/*   trouble.				       */
			/*   20100726   This   function  calls  other  */
			/*   functions  which  we  may be probing. We  */
			/*   may   have   a  random  race  condition,  */
			/*   dependent on the order of symbols in the  */
			/*   kernel,  and  crash. I have seen this on  */
			/*   2.6.24/i386  kernel.  Need  to make this  */
			/*   safe as the probes are armed.	       */
			/***********************************************/
			__flush_tlb_all();
		}
		addr += PAGE_SIZE;
	}
# else
	page_perms_t perms;

	for (i = 0; i <= num_pages; i++) {
		mem_set_perms((unsigned long) addr, &perms, ~_PAGE_NX, _PAGE_RW);
		addr += PAGE_SIZE;
	}
# endif
	return 1;
}

/**********************************************************************/
/*   Called from fbt_linux.c. Dont let us register a probe point for  */
/*   something  on the notifier chain because if we trigger, we will  */
/*   panic/hang/reset the kernel due to reentrancy.		      */
/*   								      */
/*   Since  our entry may be towards the end of the list, we need to  */
/*   scan  from  the front of the list, but we dont have that, so we  */
/*   need to dig into the kernel to find it.			      */
/**********************************************************************/
int
on_notifier_list(uint8_t *ptr)
{	struct notifier_block *np;
#if !defined(ATOMIC_NOTIFIER_HEAD)
#       define atomic_notifier_head notifier_block
#       define atomic_head(x) x
# else
#       define atomic_head(x) (x)->head
# endif
	static struct atomic_notifier_head *die_chain;
	static struct blocking_notifier_head *task_exit_notifier;
	int	do_print = 0; // set to 1 for debug
static int first_time = TRUE;

return 0;

	if (first_time) {
		first_time = FALSE;
		if (die_chain == NULL)
			die_chain = (struct atomic_notifier_head *) get_proc_addr("die_chain");
		if (task_exit_notifier == NULL)
			task_exit_notifier = (struct blocking_notifier_head *) get_proc_addr("task_exit_notifier");
	}

	if (die_chain) {
		for (np = atomic_head(die_chain); np; np = np->next) {
			if (do_print) 
				printk("illop-chain: %p\n", np->notifier_call);
			if ((uint8_t *) np->notifier_call == ptr)
				return 1;
		}
	}

	if (task_exit_notifier) {
		for (np = atomic_head(task_exit_notifier); np; np = np->next) {
			if (do_print)
				printk("exit-chain: %p\n", np->notifier_call);
			if ((uint8_t *) np->notifier_call == ptr)
				return 1;
		}
	}

	return 0;
}
/**********************************************************************/
/*   Parallel  allocator  to  avoid touching kernel data structures.  */
/*   This  is presently used to create a shadow "struct module" - it  */
/*   used  to  be  used  for  processes  and  threads,  but  we  use  */
/*   shadow_procs for this purpose now.				      */
/**********************************************************************/
static struct par_alloc_t *hd_par;
static DEFINE_MUTEX(par_mutex);

void *
par_alloc(int domain, void *ptr, int size, int *init)
{	par_alloc_t *p;

#if 0
/* Whilst this is NULL, we dont get dynamic module probing.
We need to redo process par_alloc so we dont need dynamic memory from
probe context.

Need to FIX!
*/
return NULL;
#endif

	mutex_enter(&par_mutex);
	for (p = hd_par; p; p = p->pa_next) {
		if (p->pa_ptr == ptr && p->pa_domain == domain) {
			if (init)
				*init = FALSE;
			mutex_exit(&par_mutex);
			return p;
		}
	}
	mutex_exit(&par_mutex);

	if (init)
		*init = TRUE;

	if ((p = kmalloc(size + sizeof(*p), GFP_ATOMIC)) == NULL)
		return NULL;
	dtrace_bzero(p+1, size);
	p->pa_domain = domain;
	p->pa_ptr = ptr;
	mutex_enter(&par_mutex);
	p->pa_next = hd_par;
	hd_par = p;
	mutex_exit(&par_mutex);

	return p;
}
/**********************************************************************/
/*   Find  thread  without allocating a shadow struct. Needed during  */
/*   proc exit.							      */
/**********************************************************************/
proc_t *
par_find_thread(struct task_struct *t)
{	par_alloc_t *p = par_lookup(t);

	if (p == NULL)
		return NULL;

	return (proc_t *) (p + 1);
}
/**********************************************************************/
/*   Free the parallel pointer.					      */
/**********************************************************************/
void
par_free(int domain, void *ptr)
{	par_alloc_t *p = (par_alloc_t *) ptr;
	par_alloc_t *p1;

#if 0
return;
#endif
	mutex_enter(&par_mutex);
	if (hd_par == p && hd_par->pa_domain == domain) {
		hd_par = hd_par->pa_next;
		mutex_exit(&par_mutex);
		kfree(ptr);
		return;
		}
	for (p1 = hd_par; p1 && p1->pa_next != p; p1 = p1->pa_next) {
//		printk("p1=%p\n", p1);
		}
	if (p1 == NULL) {
		mutex_exit(&par_mutex);
		printk("where did p1 go?\n");
		return;
	}
	if (p1->pa_next == p && p1->pa_domain == domain)
		p1->pa_next = p->pa_next;
	mutex_exit(&par_mutex);
	kfree(ptr);
}
/**********************************************************************/
/*   Map  pointer to the shadowed area. Dont create if its not there  */
/*   already.							      */
/**********************************************************************/
static void *
par_lookup(void *ptr)
{	par_alloc_t *p;
	
	mutex_enter(&par_mutex);
	for (p = hd_par; p; p = p->pa_next) {
		if (p->pa_ptr == ptr) {
			mutex_exit(&par_mutex);
			return p;
		}
	}
	mutex_exit(&par_mutex);
	return NULL;
}
/**********************************************************************/
/*   We want curthread to point to something -- but we cannot modify  */
/*   the Linux kernel to add stuff to the proc/thread structures, so  */
/*   we will create a shadow data structure on demand. This means we  */
/*   may  need  to  do  this often (i.e. we need to be fast), but in  */
/*   addition,  we  may  need  to do garbage collection or intercept  */
/*   thread/proc death in the main kernel.			      */
/**********************************************************************/
# undef task_struct
void	*par_setup_thread1(struct task_struct *);
void
par_setup_thread()
{
	/***********************************************/
	/*   Setup   global   vars   for   the  probe  */
	/*   functions  in  case user is using one of  */
	/*   them.				       */
	/***********************************************/
	ctf_setup();

	/***********************************************/
	/*   May rewrite/drop this later on...	       */
	/***********************************************/
	par_setup_thread1(get_current());
}
void *
par_setup_thread1(struct task_struct *tp)
{	sol_proc_t	*solp;

//	solp = cpu_core[cpu_get_id()].cpuc_proc;
	solp = &shadow_procs[tp->pid];

	curthread = solp;
	curthread->pid = tp->pid;
	curthread->p_pid = tp->pid;
	curthread->p_task = tp;
	/***********************************************/
	/*   2.6.24.4    kernel    has   parent   and  */
	/*   real_parent,  but RH FC8 (2.6.24.4 also)  */
	/*   doesnt have real_parent.		       */
	/***********************************************/
	if (tp->parent) {
		curthread->p_ppid = tp->parent->pid;
		curthread->ppid = tp->parent->pid;
	}
	return curthread;
}
/**********************************************************************/
/*   Lookup a proc without allocating a shadow structure.	      */
/**********************************************************************/
void *
par_setup_thread2()
{
	return par_find_thread(get_current());
}
/**********************************************************************/
/*   For debugging...						      */
/**********************************************************************/
#if defined(dump_tree)
void
print_pte(pte_t *pte, int level)
{
	/***********************************************/
	/*   Workaround for older kernels.	       */
	/***********************************************/
# if !defined(_PAGE_PAT)
# define _PAGE_PAT 0
# define _PAGE_PAT_LARGE 0
# endif
	printk("pte: %p level=%d %p %s%s%s%s%s%s%s%s%s\n",
		pte,
		level, (long *) pte_val(*pte), 
		pte_val(*pte) & _PAGE_NX ? "NX " : "",
		pte_val(*pte) & _PAGE_RW ? "RW " : "RO ",
		pte_val(*pte) & _PAGE_USER ? "USER " : "KERNEL ",
		pte_val(*pte) & _PAGE_GLOBAL ? "GLOBAL " : "",
		pte_val(*pte) & _PAGE_PRESENT ? "Present " : "",
		pte_val(*pte) & _PAGE_PWT ? "Writethru " : "",
		pte_val(*pte) & _PAGE_PSE ? "4/2MB " : "",
		pte_val(*pte) & _PAGE_PAT ? "PAT " : "",
		pte_val(*pte) & _PAGE_PAT_LARGE ? "PAT_LARGE " : ""

		);
}
#endif

/**********************************************************************/
/*   Call on proc exit, so we can detach ourselves from the proc. We  */
/*   may  have  a  USDT  enabled app dying, so we need to remove the  */
/*   probes. Also need to garbage collect the shadow proc structure,  */
/*   so we dont leak memory.					      */
/**********************************************************************/
static int 
proc_exit_notifier(struct notifier_block *n, unsigned long code, void *ptr)
{
	sol_proc_t sol_proc;

//printk("proc_exit_notifier: code=%lu ptr=%p\n", code, ptr);
	/***********************************************/
	/*   See  if  we know this proc - if so, need  */
	/*   to let fasttrap retire the probes.	       */
	/***********************************************/
	if (dtrace_fasttrap_exit_ptr == NULL)
		return 0;

	/***********************************************/
	/*   Set up a fake proc_t for this process.    */
	/***********************************************/
	memset(&sol_proc, 0, sizeof sol_proc);
	sol_proc.p_pid = current->pid;
	curthread = &sol_proc;

	mutex_init(&sol_proc.p_lock);
	mutex_enter(&sol_proc.p_lock);

	dtrace_fasttrap_exit_ptr(&sol_proc);

	mutex_exit(&sol_proc.p_lock);

	return 0;
}

/**********************************************************************/
/*   Handle illegal instruction trap for fbt/sdt.		      */
/**********************************************************************/
# if 0
static int proc_notifier_trap_illop(struct notifier_block *n, unsigned long code, void *ptr)
{	struct die_args *args = (struct die_args *) ptr;

	if (dtrace_here) {
		printk("proc_notifier_trap_illop called! %s err:%ld trap:%d sig:%d PC:%p CPU:%d\n", 
			args->str, args->err, args->trapnr, args->signr,
			(void *) args->regs->r_pc, 
			smp_processor_id());
	}
	return NOTIFY_KERNEL;
}
# endif
/**********************************************************************/
/*   Lookup process by proc id.					      */
/**********************************************************************/
proc_t *
prfind(int p)
{	struct task_struct *tp;
	struct pid *pid;

	if (fn_find_get_pid == NULL) {
		dtrace_printf("prfind:find_get_pid is null (p=%d)\n", p);
		return NULL;
	}

	if (fn_pid_task == NULL) {
		dtrace_printf("prfind:pid_task is null (p=%d)\n", p);
		return NULL;
	}

	if ((pid = fn_find_get_pid(p)) == NULL) {
//		dtrace_printf("prfind:find_get_pid couldnt locate pid %d\n", p);
		return NULL;
	}
	/***********************************************/
	/*   Rework  this - the first arg is a struct  */
	/*   pid *, not a pid.			       */
	/***********************************************/
	tp = fn_pid_task(pid, PIDTYPE_PID);
	if (!tp)
		return (proc_t *) NULL;
HERE();
	return par_setup_thread1(tp);
}
/**********************************************************************/
/*   Reader/writer lock - allow any readers, but only one writer.     */
/**********************************************************************/
void
rw_enter(krwlock_t *p, krw_t type)
{
	if (type == RW_READER) {
		while (1) {
			mutex_enter(&p->k_mutex);
			if (p->k_writer == 0) {
				p->k_readers++;
				mutex_exit(&p->k_mutex);
				return;
			}
			mutex_exit(&p->k_mutex);
		}
	}
	/***********************************************/
	/*   RW_WRITER				       */
	/***********************************************/
	while (1) {
		mutex_enter(&p->k_mutex);
		if (p->k_readers == 0) {
			p->k_writer = 1;
			mutex_exit(&p->k_mutex);
			return;
		}
		mutex_exit(&p->k_mutex);
	}
}
void
rw_exit(krwlock_t *p)
{
	mutex_enter(&p->k_mutex);
	if (p->k_writer)
		p->k_writer = 0;
	else
		p->k_readers--;
	mutex_exit(&p->k_mutex);
}
/**********************************************************************/
/*   Allow us to see stuff from /dev/fbt.			      */
/**********************************************************************/
ssize_t
syms_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{	int	n;
	int	ncopied = 0;
	struct map *mp;
	char __user *bp = buf;

	if (*off)
		return 0;

	for (mp = syms; len > 0 && mp->m_name; mp++) {
		n = snprintf(bp, len, "%s=%p\n", mp->m_name, mp->m_ptr);
		bp += n;
		len -= n;
	}
	ncopied = bp - buf;
	*off += ncopied;
	return ncopied;
}
/**********************************************************************/
/*   Called  from  /dev/fbt  driver  to allow us to populate address  */
/*   entries.  Shouldnt  be in the /dev/fbt, and will migrate to its  */
/*   own dtrace_ctl driver at a later date.			      */
/**********************************************************************/
ssize_t 
syms_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *pos)
{
	int	n;
	int	orig_count = count;
	const char	*bufend = buf + count;
	char	*cp;
	char	*symend;
	struct map *mp;

//printk("write: '%*.*s'\n", count, count, buf);
	/***********************************************/
	/*   Allow  for 'nm -p' format so we can just  */
	/*   do:				       */
	/*   grep XXX /proc/kallsyms > /dev/fbt	       */
	/***********************************************/
	while (buf < bufend) {
		count = bufend - buf;
		if ((cp = dtrace_memchr(buf, ' ', count)) == NULL ||
		     cp + 3 >= bufend ||
		     cp[1] == ' ' ||
		     cp[2] != ' ') {
			return -EIO;
		}

		if ((cp = dtrace_memchr(buf, '\n', count)) == NULL) {
			return -EIO;
		}
		symend = cp--;
		while (cp > buf && cp[-1] != ' ')
			cp--;
		n = symend - cp;
//printk("sym='%*.*s'\n", n, n, cp);
		for (mp = syms; mp->m_name; mp++) {
			if (strlen(mp->m_name) == n &&
			    memcmp(mp->m_name, cp, n) == 0)
			    	break;
		}
		if (mp->m_name != NULL) {
			mp->m_ptr = (unsigned long *) simple_strtoul(buf, NULL, 16);
			if (dtrace_here)
				printk("fbt: got %s=%p\n", mp->m_name, mp->m_ptr);
		}
		buf = symend + 1;
	}


	xkallsyms_lookup_name 	= (unsigned long (*)(char *)) syms[OFFSET_kallsyms_lookup_name].m_ptr;
	xmodules 		= (void *) syms[OFFSET_modules].m_ptr;
	xsys_call_table 	= (void **) syms[OFFSET_sys_call_table].m_ptr;
	xia32_sys_call_table 	= (void **) syms[OFFSET_ia32_sys_call_table].m_ptr;

	/***********************************************/
	/*   On 2.6.23.1 kernel I have, in i386 mode,  */
	/*   we  have no exported sys_call_table, and  */
	/*   we   need   it.   Instead,   we  do  get  */
	/*   syscall_call  (code  in  entry.S) and we  */
	/*   can use that to find the syscall table.   */
	/*   					       */
	/*   We are looking at an instruction:	       */
	/*   					       */
	/*   call *sys_call_table(,%eax,4)	       */
	/***********************************************/
	{unsigned char *ptr = (unsigned char *) syms[OFFSET_syscall_call].m_ptr;
	if (ptr &&
	    syms[OFFSET_sys_call_table].m_ptr == NULL &&
	    ptr[0] == 0xff && ptr[1] == 0x14 && ptr[2] == 0x85) {
		syms[OFFSET_sys_call_table].m_ptr = (unsigned long *) *((long *) (ptr + 3));
		printk("dtrace: sys_call_table located at %p\n",
			syms[OFFSET_sys_call_table].m_ptr);
		xsys_call_table = (void **) syms[OFFSET_sys_call_table].m_ptr;
		}
	}
	/***********************************************/
	/*   Handle modules[] missing in 2.6.23.1      */
	/***********************************************/
	{unsigned char *ptr = (unsigned char *) syms[OFFSET_print_modules].m_ptr;
//if (ptr) {printk("print_modules* %02x %02x %02x\n", ptr[16], ptr[17], ptr[22]); }
	if (ptr &&
	    syms[OFFSET_modules].m_ptr == NULL &&
	    ptr[16] == 0x8b && ptr[17] == 0x15 && ptr[22] == 0x8d) {
		syms[OFFSET_modules].m_ptr = (unsigned long *) *((long *) 
			(ptr + 18));
		xmodules = syms[OFFSET_modules].m_ptr;
		printk("dtrace: modules[] located at %p\n",
			syms[OFFSET_modules].m_ptr);
		}
	}

	/***********************************************/
	/*   Dump out the table (for debugging).       */
	/***********************************************/
# if 0
	{int i;
	for (i = 0; i <= 14; i++) {
		printk("[%d] %s %p\n", i, syms[i].m_name, syms[i].m_ptr);
	}
	}
# endif

	if (syms[OFFSET_END_SYMS].m_ptr) {
		/***********************************************/
		/*   Init any patching code.		       */
		/***********************************************/
		dtrace_linux_init();

		xcall_init();
  		dtrace_profile_init();
		dtrace_prcom_init();
		dcpc_init();
		sdt_init();
		ctl_init();
		/***********************************************/
		/*   Initialise the io provider.	       */
		/***********************************************/
		io_prov_init();
		instr_init();

		/***********************************************/
		/*   Try  and  keep  this at the end, so that  */
		/*   the fbt_invop handler is at the front of  */
		/*   the  callback  list.  This  is a speedup  */
		/*   optimisation  since  fbt  is  likely the  */
		/*   most   common   provider   to  call  for  */
		/*   breakpoint traps.			       */
		/***********************************************/
		fbt_init2();

		/***********************************************/
		/*   Update the IDT so we get first chance at  */
		/*   the breakpoint and pagefault interrupts.  */
		/*   We   do   this   after   all  the  other  */
		/*   providers,  so  that memory allocated by  */
		/*   the  providers  can  now  be mapped into  */
		/*   each    processes    page    map   table  */
		/*   (vmalloc_sync_all).		       */
		/***********************************************/
		intr_init();

		/***********************************************/
		/*   Intercept  do_notify_resume  so  we  can  */
		/*   tell  a  process  is getting delivered a  */
		/*   signal  -  needed by fasttrap to avoid a  */
		/*   sigreturn   returning   to  the  scratch  */
		/*   buffer.				       */
		/***********************************************/
		signal_init();

	}
	return orig_count;
}

/**********************************************************************/
/*   Deliver signal to a proc.					      */
/**********************************************************************/
int
tsignal(proc_t *p, int sig)
{
	send_sig(sig, p->p_task, 0);
	return 0;
}
/**********************************************************************/
/*   Read from a procs memory.					      */
/**********************************************************************/
int 
uread(proc_t *p, void *addr, size_t len, uintptr_t dest)
{	int (*func)(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write) = 
		fbt_get_access_process_vm();
	int	ret;

	ret = func(p->p_task, (unsigned long) dest, (void *) addr, len, 0);
//printk("uread %p %p %d %p -- func=%p ret=%d\n", p, addr, (int) len, (void *) dest, func, ret);
	return ret == len ? 0 : -1;
}
int 
uwrite(proc_t *p, void *src, size_t len, uintptr_t addr)
{	int (*func)(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write) = 
		fbt_get_access_process_vm();
	int	ret;

	ret = func(p->p_task, (unsigned long) addr, (void *) src, len, 1);
//printk("uwrite %p %p %d src=%p %02x -- func=%p ret=%d\n", p, (void *) addr, (int) len, src, *(unsigned char *) src, func, ret);
	return ret == len ? 0 : -1;
}
/**********************************************************************/
/*   Need to implement this or use the unr code from FreeBSD.	      */
/**********************************************************************/
#define	SEQ_MAGIC (('m' << 24) | ('s' << 16) | ('e' << 8) | 'q')
typedef struct seq_t {
	mutex_t seq_mutex;
	long	seq_magic;
	int	seq_id;
	} seq_t;

void *
vmem_alloc(vmem_t *hdr, size_t s, int flags)
{	seq_t *seqp = (seq_t *) hdr;
	void	*ret;

	if (TRACE_ALLOC || dtrace_mem_alloc)
		dtrace_printf("vmem_alloc(size=%d)\n", (int) s);
	if (seqp->seq_magic != SEQ_MAGIC) {
		static int once = TRUE;
		if (once) {
			once = FALSE;
			dtrace_printf("%p: vmem_alloc: bad magic\n", seqp);
		}
	}

	mutex_enter(&seqp->seq_mutex);
	ret = (void *) (long) ++seqp->seq_id;
	mutex_exit(&seqp->seq_mutex);
	return ret;
}

void *
vmem_create(const char *name, void *base, size_t size, size_t quantum,
        vmem_alloc_t *afunc, vmem_free_t *ffunc, vmem_t *source,
        size_t qcache_max, int vmflag)
{	seq_t *seqp = kmalloc(sizeof *seqp, GFP_KERNEL);

	if (TRACE_ALLOC || dtrace_here)
		dtrace_printf("vmem_create(size=%d)\n", (int) size);

	mutex_init(&seqp->seq_mutex);
	seqp->seq_id = 0;
	seqp->seq_magic = SEQ_MAGIC;

	dtrace_printf("vmem_create(%s) %p\n", name, seqp);
	
	return seqp;
}

void
vmem_free(vmem_t *hdr, void *ptr, size_t size)
{	seq_t *seqp = (seq_t *) hdr;

	if (TRACE_ALLOC || dtrace_here)
		dtrace_printf("vmem_free(ptr=%p, size=%d)\n", ptr, (int) size);
	if (seqp->seq_magic != SEQ_MAGIC) {
		dtrace_printf("%p: vmem_free: bad magic\n", seqp);
	}

}
void 
vmem_destroy(vmem_t *hdr)
{	seq_t *seqp = (seq_t *) hdr;

	if (seqp->seq_magic != SEQ_MAGIC) {
		dtrace_printf("%p: vmem_destroy: bad magic\n", seqp);
	}
	kfree(hdr);
}
/**********************************************************************/
/*   Memory  barrier  used  by  atomic  cas instructions. We need to  */
/*   implement  this if we are to be reliable in an SMP environment.  */
/*   systrace.c calls us   					      */
/**********************************************************************/
void
membar_enter(void)
{
}
void
membar_producer(void)
{
}

int
priv_policy_choice(const cred_t *a, int priv, int allzone)
{
//printk("priv_policy_choice %p %x %d := trying...\n", a, priv, allzone);
	return priv_policy_only(a, priv, allzone);
}
/**********************************************************************/
/*   Control  the access restrictions of processes (users) so we can  */
/*   effect the privilege policies.				      */
/**********************************************************************/
int
priv_policy_only(const cred_t *a, int priv, int allzone)
{	dsec_item_t	*dp;
	dsec_item_t	*dpend;

	/***********************************************/
	/*   Let  root  have  access,  else we cannot  */
	/*   really initialise the driver.	       */
	/***********************************************/
	if (KUIDT_VALUE(a->cr_uid) == 0)
		return 1;

	dpend = &di_list[di_cnt];
//printk("priv_policy_only %p %x %d := trying...\n", a, priv, allzone);
	for (dp = di_list; dp < dpend; dp++) {
		switch (dp->di_type) {
		  case DIT_UID:
		  	if (dp->di_id != KUIDT_VALUE(a->cr_uid))
				continue;
			break;
		  case DIT_GID:
		  	if (dp->di_id != KGIDT_VALUE(a->cr_gid))
				continue;
			break;
		  case DIT_ALL:
		  	break;
		  }
		if ((dp->di_priv & priv) == 0)
			continue;
//printk("priv_policy_only %p (%d,%d) %x %d := true\n", a, a->cr_uid, a->cr_gid, priv, allzone);
		return 1;
	}
//printk("priv_policy_only %p (%d,%d) %x %d := false\n", a, a->cr_uid, a->cr_gid, priv, allzone);
        return 0;
}
/**********************************************************************/
/*   Module   interface   for   /dev/dtrace_helper.  I  really  want  */
/*   /dev/dtrace/helper,  but havent figured out the kernel calls to  */
/*   get this working yet.					      */
/**********************************************************************/
static int
helper_open(struct inode *inode, struct file *file)
{	int	ret = 0;

	return -ret;
}
static int
helper_release(struct inode *inode, struct file *file)
{
	/***********************************************/
	/*   Dont do anything for the helper.	       */
	/***********************************************/
	return 0;
}
static ssize_t
helper_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
	return -EIO;
}
/*
static int 
helper_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{	int len;

	// place holder for something useful later on.
	len = sprintf(page, "hello");
	return 0;
}
*/
/**********************************************************************/
/*   Invoked  by  drti.c  -- the USDT .o file linked into apps which  */
/*   provide user space dtrace probes.				      */
/**********************************************************************/
static int 
helper_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{	int	ret;
	int	rv = 0;

	ret = dtrace_ioctl_helper(file, cmd, arg, 0, NULL, &rv);
//HERE();
//if (dtrace_here && ret) printk("ioctl-returns: ret=%d rv=%d\n", ret, rv);
        return ret ? -ret : rv;
}
#ifdef HAVE_UNLOCKED_IOCTL
static long
helper_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return helper_ioctl(NULL, file, cmd, arg);
}
#endif
#ifdef HAVE_COMPAT_IOCTL
static long 
helper_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return helper_ioctl(NULL, file, cmd, arg);
}
#endif

/**********************************************************************/
/*   Module interface for /dev/dtrace - the main driver.	      */
/**********************************************************************/
static int
dtracedrv_open(struct inode *inode, struct file *file)
{	int	ret;

	ret = dtrace_open(file, 0, 0, CRED());

	return -ret;
}
static int
dtracedrv_release(struct inode *inode, struct file *file)
{
//HERE();
	dtrace_close(file, 0, 0, NULL);
	return 0;
}
/**********************************************************************/
/*   Read some vars/debug from the driver.			      */
/**********************************************************************/
static ssize_t
dtracedrv_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{	int	n = 0;
	int	j;

	if (*off)
		return 0;

	j = (dbuf_i + 1) % log_bufsiz;
	while (len > 0 && j != dbuf_i) {
		if (dtrace_buf[j]) {
			*buf++ = dtrace_buf[j];
			len--;
			n++;
		}
		j = (j + 1) % log_bufsiz;
	}
	*off += n;
	return n;
}
/**********************************************************************/
/*   Allow us to change driver parms.				      */
/**********************************************************************/

/**********************************************************************/
/*   Parse a security line, line this:				      */
/*   								      */
/*   clear                              			      */
/*   uid 1 priv_user priv_kernel				      */
/*   gid 23 priv_proc						      */
/*   all priv_owner						      */
/**********************************************************************/
static int
parse_sec(dsec_item_t *dp, const char *cp, const char *cpend)
{	const char	*word;

# define	skip_white() while (cp < cpend && *cp == ' ') cp++;

	skip_white();
	if (*cp == '#')
		return FALSE;

	word = cp;
	while (cp < cpend && *cp != ' ') {
		if (*cp == '\n')
			break;
		cp++;
	}
	/***********************************************/
	/*   Lets us clear the array.		       */
	/***********************************************/
	if (cp - word >= 5 && strncmp(word, "clear", 5) == 0)
		return -1;

	skip_white();
	if (strncmp(word, "uid", 3) == 0)
		dp->di_type = DIT_UID;
	else if (strncmp(word, "gid", 3) == 0)
		dp->di_type = DIT_GID;
	else if (strncmp(word, "all", 3) == 0)
		dp->di_type = DIT_ALL;
	else
		return FALSE;

	if (dp->di_type == DIT_UID || dp->di_type == DIT_GID) {
		word = cp;
		dp->di_id = simple_strtoul(cp, NULL, 0);
		while (cp < cpend && *cp != ' ') {
			if (*cp == '\n')
				return FALSE;
			cp++;
		}
	}

	dp->di_priv |= DTRACE_PRIV_NONE; /* 0x0000 */
	while (cp < cpend && *cp != '\n') {
		skip_white();
		word = cp;
		if (cp >= cpend || *cp == '\n' || *cp == '#')
			break;

		if (strncmp(cp, "priv_kernel", 11) == 0)
			dp->di_priv |= DTRACE_PRIV_KERNEL;
		else if (strncmp(cp, "priv_user", 9) == 0)
			dp->di_priv |= DTRACE_PRIV_USER;
		else if (strncmp(cp, "priv_proc", 9) == 0)
			dp->di_priv |= DTRACE_PRIV_PROC;
		else if (strncmp(cp, "priv_owner", 10) == 0)
			dp->di_priv |= DTRACE_PRIV_OWNER;
		else {
			return FALSE;
		}

		while (cp < cpend && *cp != ' ' && *cp != '\n')
			cp++;
	}
	return TRUE;
}

/**********************************************************************/
/*   Parse  data written to /dev/dtrace. This is either debug stuff,  */
/*   or the security regime.					      */
/**********************************************************************/
static ssize_t 
dtracedrv_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *pos)
{	const char	*bpend = buf + count;
	const char	*cp = buf;
	int	len;

	/***********************************************/
	/*   If  we  arent root, dont accept or trust  */
	/*   anything  since  they  can  subvert  the  */
	/*   security model.			       */
	/***********************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	if (KUIDT_VALUE(current->cred->uid) != 0 && 
	    KUIDT_VALUE(current->cred->euid) != 0)
#else
	if (current->uid != 0 && current->euid != 0)
#endif
		return count;

	while (cp && cp < bpend) {
		while (cp < bpend && *cp == ' ')
			cp++;
		if (cp >= bpend)
			continue;
		if (*cp == '\n') {
			cp++;
			continue;
		}
		len = bpend - cp;
		if (len >= 6 && strncmp(cp, "here=", 5) == 0) {
		    	dtrace_here = simple_strtoul(cp + 5, NULL, 0);
		} else if (len >= 6 && strncmp(cp, "dtrace_safe=", 5) == 0) {
		    	dtrace_safe = simple_strtoul(cp + 5, NULL, 0);
		} else if (di_cnt < MAX_SEC_LIST) {
			int	ret = parse_sec(&di_list[di_cnt], cp, bpend);
			if (ret < 0)
				di_cnt = 0;
			else if (ret)
				di_cnt++;
		}
		if ((cp = strchr(cp, '\n')) == NULL)
			break;
		cp++;
	}
	return count;
}

/**
 * "proc/dtrace"
 */

/** "proc/dtrace/debug" */
static int proc_dtrace_debug_show(struct seq_file *seq, void *v)
{
	seq_printf(seq,
		"here=%d\n"
		"cpuid=%d\n",
		dtrace_here,
		cpu_get_id());
	return 0;
}
static int proc_dtrace_debug_single_open(struct inode *inode, struct file *file)
{
	return single_open(file, &proc_dtrace_debug_show, NULL);
}
static struct file_operations proc_dtrace_debug = {
	.owner   = THIS_MODULE,
	.open    = proc_dtrace_debug_single_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

/** "proc/dtrace/security" */
static int proc_dtrace_security_show(struct seq_file *seq, void *v)
{
	char	tmpbuf[128];
	int	i;

	/***********************************************/
	/*   Dump out the security table.	       */
	/***********************************************/
	for (i = 0; i < di_cnt && di_list[i].di_type; i++) {
		char	*tp;
		char	*tpend = tmpbuf + sizeof tmpbuf;
		strcpy(tmpbuf, 
			di_list[i].di_type == DIT_UID ? "uid " :
			di_list[i].di_type == DIT_GID ? "gid " :
			di_list[i].di_type == DIT_ALL ? "all " : "sec?? ");
		tp = tmpbuf + strlen(tmpbuf);

		if (di_list[i].di_type != DIT_ALL) {
			snprintf(tp, tpend - tp,
				" %u", di_list[i].di_id);
		}
		if (di_list[i].di_priv & DTRACE_PRIV_USER)
			strcat(tp, " priv_user");
		if (di_list[i].di_priv & DTRACE_PRIV_KERNEL)
			strcat(tp, " priv_kernel");
		if (di_list[i].di_priv & DTRACE_PRIV_PROC)
			strcat(tp, " priv_proc");
		if (di_list[i].di_priv & DTRACE_PRIV_OWNER)
			strcat(tp, " priv_owner");

		seq_printf(seq, "%s\n", tmpbuf);
	}
	return 0;
}

static int proc_dtrace_security_single_open(struct inode *inode, struct file *file)
{
	return single_open(file, &proc_dtrace_security_show, NULL);
}
static struct file_operations proc_dtrace_security = {
	.owner   = THIS_MODULE,
	.open    = proc_dtrace_security_single_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

/** "proc/dtrace/stats" */
static int proc_dtrace_stats_show(struct seq_file *seq, void *v)
{	int	i;
	extern unsigned long cnt_0x7f;
	extern unsigned long cnt_gpf1;
	extern unsigned long cnt_gpf2;
	extern unsigned long long cnt_int1_1;
	extern unsigned long long cnt_int3_1;
	extern unsigned long long cnt_int3_2;
	extern unsigned long long cnt_int3_3;
	extern unsigned long cnt_ipi1;
	extern unsigned long long cnt_probe_recursion;
	extern unsigned long cnt_probes;
	extern unsigned long long cnt_probe_noint;
	extern unsigned long long cnt_probe_safe;
	extern unsigned long cnt_mtx1;
	extern unsigned long cnt_mtx2;
	extern unsigned long cnt_mtx3;
	extern unsigned long long cnt_pf1;
	extern unsigned long long cnt_pf2;
	extern unsigned long cnt_snp1;
	extern unsigned long cnt_snp2;
# define TYPE_LONG 0
# define TYPE_INT  1
# define TYPE_LONG_LONG 2

#define	LONG_LONG(var, name) {TYPE_LONG_LONG, (unsigned long *) &var, name}
	static struct map {
		int	type;
		unsigned long *ptr;
		char	*name;
		} stats[] = {
		{TYPE_INT, (unsigned long *) &dtrace_safe, "dtrace_safe"},
		{TYPE_LONG_LONG, &cnt_probes, "probes"},
		{TYPE_LONG, (unsigned long *) &cnt_probe_recursion, "probe_recursion"},
		LONG_LONG(cnt_probe_noint, "probe_noint"),
		LONG_LONG(cnt_probe_safe, "probe_safe"),
		LONG_LONG(cnt_int1_1, "int1"),
		LONG_LONG(cnt_int3_1, "int3_1"),
		LONG_LONG(cnt_int3_2, "int3_2(ours)"),
		LONG_LONG(cnt_int3_3, "int3_3(reentr)"),
		LONG_LONG(cnt_0x7f, "int_0x7f"),
		LONG_LONG(cnt_gpf1, "gpf1"),
		LONG_LONG(cnt_gpf2, "gpf2"),
		{TYPE_LONG, &cnt_ipi1, "ipi1"},
		{TYPE_LONG, &cnt_mtx1, "mtx1"},
		{TYPE_LONG, &cnt_mtx2, "mtx2"},
		{TYPE_LONG, &cnt_mtx3, "mtx3"},
		{TYPE_LONG, &cnt_nmi1, "nmi1"},
		{TYPE_LONG, &cnt_nmi2, "nmi2"},
		LONG_LONG(cnt_pf1, "pf1"),
		LONG_LONG(cnt_pf2, "pf2"),
		{TYPE_LONG, &cnt_snp1, "snp1"},
		{TYPE_LONG, &cnt_snp2, "snp2"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_syscall1, "syscall1"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_syscall2, "syscall2"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_syscall3, "syscall3"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_timer1, "timer1"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_timer2, "timer2(rec)"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_timer3, "timer3(defer-cancel)"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_timer_add, "timer_add"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_timer_remove, "timer_remove"},
		{TYPE_LONG, &cnt_xcall0, "xcall0"},
		{TYPE_LONG, &cnt_xcall1, "xcall1"},
		{TYPE_LONG, &cnt_xcall2, "xcall2"},
		{TYPE_LONG, &cnt_xcall3, "xcall3(reentrant)"},
		{TYPE_LONG, &cnt_xcall4, "xcall4(delay)"},
		{TYPE_LONG, &cnt_xcall5, "xcall5(spinlock)"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_xcall6, "xcall6(ack_waits)"},
		{TYPE_LONG_LONG, (unsigned long *) &cnt_xcall7, "xcall7(fast)"},
		{TYPE_LONG, (unsigned long *) &cnt_xcall8, "xcall8"},
		{TYPE_INT, (unsigned long *) &dtrace_shutdown, "shutdown"},
		{0}
		};

	for (i = 0; i < MAX_DCNT; i++) {
		if (dcnt[i] == 0)
			continue;
		seq_printf(seq, "dcnt%d=%lu\n", i, dcnt[i]);
	}

	for (i = 0; stats[i].name; i++) {
		if (stats[i].type == TYPE_LONG_LONG)
			seq_printf(seq, "%s=%llu\n", stats[i].name, *(unsigned long long *) stats[i].ptr);
		else if (stats[i].type == TYPE_LONG)
			seq_printf(seq, "%s=%lu\n", stats[i].name, *stats[i].ptr);
		else
			seq_printf(seq, "%s=%d\n", stats[i].name, *(int *) stats[i].ptr);
	}

	return 0;
}

static int proc_dtrace_stats_single_open(struct inode *inode, struct file *file)
{
	return single_open(file, &proc_dtrace_stats_show, NULL);
}
static struct file_operations proc_dtrace_stats = {
	.owner   = THIS_MODULE,
	.open    = proc_dtrace_stats_single_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

/**********************************************************************/
/*   Special hack for debugging.				      */
/**********************************************************************/
static ssize_t proc_dtrace_trace_write_proc(struct file *fp, const char __user *buffer, size_t count, loff_t *off)
{
	int	n = count > 32 ? 32 : count;

	dtrace_printf("proc_dtrace_trace_write_proc: %*.*s\n", n, n, buffer);
	if (strncmp(buffer, "stack", 5) == 0)
		dump_all_stacks();

	return count;
}

/**********************************************************************/
/*   Code for /proc/dtrace/trace				      */
/**********************************************************************/

static void *proc_dtrace_trace_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos < log_bufsiz)
		return pos;
	return NULL;
}
static void *proc_dtrace_trace_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >= log_bufsiz / 1024)
		return NULL;

	return pos;
}
static void proc_dtrace_trace_seq_stop(struct seq_file *seq, void *v)
{
}
static int proc_dtrace_trace_seq_show(struct seq_file *seq, void *v)
{	char	*cp;
	extern int dbuf_i;
	loff_t	*pos = v;
	int	n = (int) (long) *pos;
	int	n1 = 0;

	if (n >= log_bufsiz)
		return 0;

	/***********************************************/
	/*   Skip  any  nulls  since  we may not have  */
	/*   filled the buffer yet.		       */
	/***********************************************/
	cp = dtrace_buf + dbuf_i;
	for (n1 = 0; n1 < log_bufsiz && *cp == '\0'; n1++) {
		if (++cp >= &dtrace_buf[log_bufsiz])
			cp = dtrace_buf;
	}

	n = (cp - dtrace_buf + n * 1024) % log_bufsiz;
	cp = dtrace_buf + n;

	/***********************************************/
	/*   Now  return  the  next  block of text up  */
	/*   until the circular index.		       */
	/***********************************************/
	for (n1 = 0; cp != &dtrace_buf[dbuf_i] && n1 < 1024; ) {
		if (cp >= &dtrace_buf[log_bufsiz])
			cp = dtrace_buf;
		if (*cp && seq_write(seq, cp, 1))
			break;
		cp++;
		//(*pos)++;
		n1++;
	}
	return 0;
}
struct seq_operations proc_dtrace_trace_show_ops = {
	.start = proc_dtrace_trace_seq_start,
	.next = proc_dtrace_trace_seq_next,
	.stop = proc_dtrace_trace_seq_stop,
	.show = proc_dtrace_trace_seq_show,
	};
static int proc_dtrace_trace_single_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &proc_dtrace_trace_show_ops);
}
static struct file_operations proc_dtrace_trace = {
	.owner   = THIS_MODULE,
	.open    = proc_dtrace_trace_single_open,
	.read    = seq_read, // dtracedrv_read() ?
	.llseek  = seq_lseek,
	.release = seq_release,
	.write   = proc_dtrace_trace_write_proc
};

/** /proc */

static int dtracedrv_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{	int	ret;
	int	rv = 0;

	/***********************************************/
	/*   Make    sure   ioctl   has   the   users  */
	/*   credentials since many parts of the code  */
	/*   will  check if we have DTRACE_CRV access  */
	/*   (we default to root==all).		       */
	/***********************************************/
	ret = dtrace_ioctl(file, cmd, arg, 0, CRED(), &rv);
//HERE();
//if (dtrace_here && ret) printk("ioctl-returns: ret=%d rv=%d\n", ret, rv);
        return ret ? -ret : rv;
}
#ifdef HAVE_UNLOCKED_IOCTL
static long dtracedrv_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return dtracedrv_ioctl(NULL, file, cmd, arg);
}
#endif

#ifdef HAVE_COMPAT_IOCTL
static long dtracedrv_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return dtracedrv_ioctl(NULL, file, cmd, arg);
}
#endif

static const struct file_operations dtracedrv_fops = {
	.owner = THIS_MODULE,
        .read = dtracedrv_read,
        .write = dtracedrv_write,
#ifdef HAVE_OLD_IOCTL
        .ioctl = dtracedrv_ioctl,
#endif
#ifdef  HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl = dtracedrv_unlocked_ioctl,
#endif
#ifdef HAVE_COMPAT_IOCTL
        .compat_ioctl = dtracedrv_compat_ioctl,
#endif
        .open = dtracedrv_open,
        .release = dtracedrv_release,
};

static struct miscdevice dtracedrv_dev = {
        MISC_DYNAMIC_MINOR,
        "dtrace",
        &dtracedrv_fops
};
static const struct file_operations helper_fops = {
	.owner = THIS_MODULE,
        .read = helper_read,
#ifdef HAVE_OLD_IOCTL
        .ioctl = helper_ioctl,
#endif
#ifdef  HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl = helper_unlocked_ioctl,
#endif
#ifdef  HAVE_COMPAT_IOCTL
        .compat_ioctl = helper_compat_ioctl,
#endif
        .open = helper_open,
        .release = helper_release,
};
static struct miscdevice helper_dev = {
        MISC_DYNAMIC_MINOR,
        "dtrace_helper",
        &helper_fops
};

/**********************************************************************/
/*   This is where we start after loading the driver.		      */
/**********************************************************************/

static int __init dtracedrv_init(void)
{	int	i, ret;

	/***********************************************/
	/*   Create the parent directory.	       */
	/***********************************************/
static struct proc_dir_entry *dir;
	dir = proc_mkdir("dtrace", NULL);
	if (!dir) {
		printk("Cannot create /proc/dtrace\n");
		return -1;
	}

	/***********************************************/
	/*   Early bootstrap get_proc_addr(). We pass  */
	/*   in  a  string  because  the module param  */
	/*   functions   wont   handle  addresses  or  */
	/*   values greater than 31-bits (yes, 31).    */
	/***********************************************/
	if (arg_kallsyms_lookup_name)
		xkallsyms_lookup_name = (unsigned long (*)(char *)) simple_strtoul(arg_kallsyms_lookup_name, NULL, 0);

	/***********************************************/
	/*   Initialise   the   cpu_list   which  the  */
	/*   dtrace_buffer_alloc  code  wants when we  */
	/*   go into a GO state.		       */
	/*   Only  allocate  space  for the number of  */
	/*   online   cpus.   If   number   of   cpus  */
	/*   increases, we wont be able to do per-cpu  */
	/*   for them - we need to realloc this array  */
	/*   and/or force out the clients.	       */
	/***********************************************/
	nr_cpus = num_online_cpus();
	cpu_table = (cpu_t *) kzalloc(sizeof *cpu_table * nr_cpus, GFP_KERNEL);
	cpu_core = (cpu_core_t *) kzalloc(sizeof *cpu_core * nr_cpus, GFP_KERNEL);
	cpu_list = (cpu_t *) kzalloc(sizeof *cpu_list * nr_cpus, GFP_KERNEL);
	cpu_cred = (cred_t *) kzalloc(sizeof *cpu_cred * nr_cpus, GFP_KERNEL);
	for (i = 0; i < nr_cpus; i++) {
		cpu_table[i].cpu_id = i;
		cpu_list[i].cpuid = i;
		cpu_list[i].cpu_next = &cpu_list[i+1];
		/***********************************************/
		/*   BUG/TODO:  We should adapt the onln list  */
		/*   to handle actual online cpus.	       */
		/***********************************************/
		cpu_list[i].cpu_next_onln = &cpu_list[i+1];
		mutex_init(&cpu_list[i].cpu_ft_lock.k_mutex);
	}
	cpu_list[nr_cpus-1].cpu_next = cpu_list;
	for (i = 0; i < nr_cpus; i++) {
		mutex_init(&cpu_core[i].cpuc_pid_lock);
	}
	/***********************************************/
	/*   Initialise  the  shadow  procs.  We dont  */
	/*   cope  with pid_max changing on us. So be  */
	/*   careful.				       */
	/***********************************************/
	shadow_procs = (sol_proc_t *) vmalloc(sizeof(sol_proc_t) * PID_MAX_DEFAULT);
	memset(shadow_procs, 0, sizeof(sol_proc_t) * PID_MAX_DEFAULT);
	for (i = 0; i < PID_MAX_DEFAULT; i++) {
		mutex_init(&shadow_procs[i].p_lock);
		mutex_init(&shadow_procs[i].p_crlock);
		}

	/***********************************************/
	/*   Create /proc/dtrace subentries.	       */
	/***********************************************/
	proc_create("debug", S_IFREG | S_IRUGO | S_IWUGO, dir, &proc_dtrace_debug);
	proc_create("security", S_IFREG | S_IRUGO | S_IWUGO, dir, &proc_dtrace_security);
	proc_create("stats", S_IFREG | S_IRUGO | S_IWUGO, dir, &proc_dtrace_stats);
	proc_create("trace", S_IFREG | S_IRUGO | S_IWUGO, dir, &proc_dtrace_trace);

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "dtracedrv loaded: /dev/dtrace available, dtrace_here=%d nr_cpus=%d\n",
		dtrace_here, nr_cpus);

	dtrace_attach(NULL, 0);

	ctf_init();
	fasttrap_init();
	systrace_init();
	/***********************************************/
	/*   Arm  the  fbt provider early, because it  */
	/*   receives  the  symbol table updates, but  */
	/*   dont do the invop hook yet.	       */
	/***********************************************/
	fbt_init();
/*	dtrace_prcom_init();
	sdt_init();
	ctl_init();*/

	/***********************************************/
	/*   We  cannot  call  this  til  we  get the  */
	/*   /proc/kallsyms  entry.  So we defer this  */
	/*   til   we   get   this   and   call  from  */
	/*   syms_write().			       */
	/***********************************************/
	/* dtrace_linux_init(); */

	/***********************************************/
	/*   Create the /dev entry points.	       */
	/***********************************************/
	ret = misc_register(&dtracedrv_dev);
	if (ret) {
		printk(KERN_WARNING "dtracedrv: Unable to register /dev/dtrace\n");
		return ret;
		}
	ret = misc_register(&helper_dev);
	if (ret) {
		printk(KERN_WARNING "dtracedrv: Unable to register /dev/dtrace_helper\n");
		return ret;
		}

	return 0;
}
static void __exit dtracedrv_exit(void)
{
	if (!dtrace_linux_fini()) {
		printk("dtrace: dtracedrv_exit: Cannot exit - since cannot unhook notifier chains.\n");
		return;
	}

	signal_fini();
	intr_exit();
	dcpc_exit();
	ctl_exit();
	sdt_exit();
	dtrace_profile_fini();
	dtrace_prcom_exit();
	systrace_exit();
	instr_exit();
	fbt_exit();
	fasttrap_exit();
	ctf_exit();

	if (dtrace_attached()) {
		if (dtrace_detach(NULL, 0) == DDI_FAILURE) {
			printk("dtrace_detach failure\n");
		}
	}

	kfree(cpu_cred);
	kfree(cpu_table);
	kfree(cpu_core);
	kfree(cpu_list);
	vfree(shadow_procs);

	printk(KERN_WARNING "dtracedrv driver unloaded.\n");

	remove_proc_entry("dtrace/debug", 0);
	remove_proc_entry("dtrace/security", 0);
	remove_proc_entry("dtrace/stats", 0);
	remove_proc_entry("dtrace/trace", 0);
	remove_proc_entry("dtrace", 0);
	misc_deregister(&helper_dev);
	misc_deregister(&dtracedrv_dev);

	xcall_fini();
}
module_init(dtracedrv_init);
module_exit(dtracedrv_exit);


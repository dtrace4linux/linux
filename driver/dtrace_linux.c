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
/*   $Header: Last edited: 14-May-2011 1.10 $ 			      */
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
#include <asm/tlbflush.h>
#include <asm/current.h>
#include <asm/desc.h>
#include <sys/rwlock.h>
#include <sys/privregs.h>
//#include <asm/pgtable.h>
//#include <asm/pgalloc.h>

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_DESCRIPTION("DTRACEDRV Driver");

# if !defined(CONFIG_NR_CPUS)
#	define	CONFIG_NR_CPUS	1
# endif

# define	TRACE_ALLOC	0

# define	NOTIFY_KERNEL	1

#if !defined(X86_PLATFORM_IPI_VECTOR)
#	define	X86_PLATFORM_IPI_VECTOR GENERIC_INTERRUPT_VECTOR
#endif

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
int dtrace_printk;
module_param(dtrace_printk, int, 0);

/**********************************************************************/
/*   dtrace_printf() buffer and state.				      */
/**********************************************************************/
#define	LOG_BUFSIZ (64 * 1024)
char	dtrace_buf[LOG_BUFSIZ];
int	dbuf_i;
int	dtrace_printf_disable;

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
	OFFSET_kallsyms_op,
	OFFSET_kallsyms_lookup_name,
	OFFSET_modules,
	OFFSET_sys_call_table,
	OFFSET_access_process_vm,
	OFFSET_syscall_call,
	OFFSET_print_modules,
	OFFSET_die_chain,
	OFFSET_idt_descr,
	OFFSET_idt_table,
	OFFSET_task_exit_notifier,
	OFFSET_xtime,
	OFFSET_kernel_text_address,
	OFFSET_ia32_sys_call_table,
	OFFSET_vmalloc_exec,
	OFFSET_add_timer_on,
	OFFSET_END_SYMS,
	};
static struct map {
	char		*m_name;
	unsigned long	*m_ptr;
	} syms[] = {
{"kallsyms_op",            NULL},
{"kallsyms_lookup_name",   NULL},
{"modules",                NULL},
{"sys_call_table",         NULL},
{"access_process_vm",      NULL},
{"syscall_call",           NULL}, /* Backup for i386 2.6.23 kernel to help */
			 	  /* find the sys_call_table.		  */
{"print_modules",          NULL}, /* Backup for i386 2.6.23 kernel to help */
			 	  /* find the modules table. 		  */
{"die_chain",              NULL}, /* In case no unregister_die_notifier */
{"idt_descr",              NULL}, /* For int3 vector patching (maybe).	*/
{"idt_table",              NULL}, /* For int3 vector patching.		*/
{"task_exit_notifier",     NULL},
{"xtime",     		   NULL}, /* Needed for dtrace_gethrtime, if 2.6.9 */
{"kernel_text_address",    NULL}, /* Used for stack walking when no dump_trace available */
{"ia32_sys_call_table",    NULL}, /* On 64b kernel, the 32b syscall table. */
{"vmalloc_exec",    	   NULL}, /* Needed for syscall trampolines. */
{"add_timer_on",    	   NULL}, /* Needed for dtrace_xcall. */
{"END_SYMS",               NULL}, /* This is a sentinel so we know we are done. */
	{0}
	};
static unsigned long (*xkallsyms_lookup_name)(char *);
static void *xmodules;
static void **xsys_call_table;
static void **xia32_sys_call_table;
static void (*fn_add_timer_on)(struct timer_list *timer, int cpu);
static void (*fn_sysrq_showregs_othercpus)(void *);
int (*kernel_text_address_fn)(unsigned long);
char *(*dentry_path_fn)(struct dentry *, char *, int);
static struct module *(*fn__module_text_address)(unsigned long);

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
MUTEX_DEFINE(mod_lock);

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

/**********************************************************************/
/*   Spinlock  to avoid dtrace_xcall having issues whilst queuing up  */
/*   a request.							      */
/**********************************************************************/
extern spinlock_t xcall_spinlock;

/**********************************************************************/
/*   We need this to be in an executable page. kzalloc doesnt return  */
/*   us  one  of  these, and havent yet fixed this so we can make it  */
/*   executable, so for now, this will do.			      */
/**********************************************************************/
static cpu_core_t	cpu_core_exec[NCPU];
# define THIS_CPU() &cpu_core_exec[cpu_get_id()]
//# define THIS_CPU() &cpu_core[cpu_get_id()]

static MUTEX_DEFINE(dtrace_provider_lock);	/* provider state lock */
MUTEX_DEFINE(cpu_lock);
int	panic_quiesce;
sol_proc_t	*curthread;

dtrace_vtime_state_t dtrace_vtime_active = 0;
dtrace_cacheid_t dtrace_predcache_id = DTRACE_CACHEIDNONE + 1;

/**********************************************************************/
/*   For dtrace_gethrtime.					      */
/**********************************************************************/
static int tsc_max_delta;
static struct timespec *xtime_cache_ptr;

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
/*   Externs.							      */
/**********************************************************************/
extern unsigned long cnt_xcall1;
extern unsigned long cnt_xcall2;
extern unsigned long cnt_xcall3;
extern unsigned long cnt_xcall4;
extern unsigned long cnt_xcall5;

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
void ctf_setup(void);
int dtrace_double_fault(void);
int dtrace_int1(void);
int dtrace_int3(void);
int dtrace_int11(void);
int dtrace_int13(void);
int dtrace_page_fault(void);
int dtrace_int_ipi(void);
static void * par_lookup(void *ptr);
# define	cas32 dtrace_cas32
uint32_t dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new);
int	ctf_init(void);
void	ctf_exit(void);
int	ctl_init(void);
void	ctl_exit(void);
int	dtrace_profile_init(void);
int	dtrace_profile_fini(void);
int	fasttrap_init(void);
void	fasttrap_exit(void);
int	fbt_init(void);
void	fbt_exit(void);
int	instr_init(void);
void	instr_exit(void);
int	sdt_init(void);
void	sdt_exit(void);
int	systrace_init(void);
void	systrace_exit(void);
void	io_prov_init(void);
static void print_pte(pte_t *pte, int level);

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
void clflush(void *ptr);
# endif

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
{	cred_t	*cr = &cpu_cred[cpu_get_id()];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	cr->cr_uid = current->cred->uid;
	cr->cr_gid = current->cred->gid;
#else
	cr->cr_uid = current->uid;
	cr->cr_gid = current->gid;
#endif
//printk("get cred end %d %d\n", cr->cr_uid, cr->cr_gid);

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
cpu_core_t *
cpu_get_this(void)
{
	return THIS_CPU();
}
/**********************************************************************/
/*   We cannot call do_gettimeofday, or ktime_get_ts or any of their  */
/*   inferiors  because they will attempt to lock on a mutex, and we  */
/*   end  up  not being able to trace fbt::kernel:nr_active. We have  */
/*   to emulate a stable clock reading ourselves. 		      */
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
/*   handle this in an OO way.					      */
/**********************************************************************/
int
dtrace_data_model(proc_t *p)
{
return DATAMODEL_LP64;
	return p->p_model;
}
/**********************************************************************/
/*   Set  an entry in the idt_table (IDT) so we can have first grabs  */
/*   at interrupts (INT1 + INT3).				      */
/**********************************************************************/
void *kernel_int1_handler;
void *kernel_int3_handler;
void *kernel_int11_handler;
void *kernel_int13_handler;
void *kernel_double_fault_handler;
void *kernel_page_fault_handler;
int ipi_vector = 0xea; // very temp hack - need to find a new interrupt

/**********************************************************************/
/*   Kernel independent gate definitions.			      */
/**********************************************************************/
# define CPU_GATE_INTERRUPT 	0xE
# define GATE_DEBUG_STACK	0

/**********************************************************************/
/*   x86-64 gate structure.					      */
/**********************************************************************/
struct gate64 {
        u16 offset_low;
        u16 segment;
        unsigned ist : 3, zero0 : 5, type : 5, dpl : 2, p : 1;
        u16 offset_middle;
        u32 offset_high;
        u32 zero1;
} __attribute__((packed));

struct gate32 {
        u16		base0;
        u16		segment;

	unsigned char	zero;
	unsigned char	flags;
	u16		base1;
} __attribute__((packed));

# if defined(__amd64)
typedef struct gate64 gate_t;

# elif defined(__i386)
typedef struct gate32 gate_t;

# else
#   error "Dont know how to handle GATEs on this cpu"
# endif

# if 0
void
set_idt_entry(int intr, unsigned long func)
{
gate_desc *idt_table = get_proc_addr("idt_table");
gate_desc s;
//printk("patch idt %p vec %d func %p\n", idt_table, intr, func);
pack_gate(&s, GATE_INTERRUPT, func, 3, DEBUG_STACK, __KERNEL_CS);

memory_set_rw(idt_table, 1, TRUE);
write_idt_entry(idt_table, intr, &s);
return;
}
# else
void
set_idt_entry(int intr, unsigned long func)
{

	gate_t *idt_table = get_proc_addr("idt_table");
static	gate_t s;
	int	type = CPU_GATE_INTERRUPT;
	int	dpl = 3;
	int	seg = __KERNEL_CS;

	dtrace_bzero(&s, sizeof s);

#if defined(__amd64)
	s.offset_low = PTR_LOW(func);
	s.segment = seg;
        s.ist = GATE_DEBUG_STACK;
        s.p = 1;
        s.dpl = dpl;
        s.zero0 = 0;
        s.zero1 = 0;
        s.type = type;
        s.offset_middle = PTR_MIDDLE(func);
        s.offset_high = PTR_HIGH(func);

#elif defined(__i386)
	s.segment = seg;
	s.base0 = (u16) (func & 0xffff);
	s.base1 = (u16) ((func & 0xffff0000) >> 16);
	s.flags =	0x80 |		// present
			(dpl << 5) |
			type;		// CPU_GATE_INTERRUPT

#else
#  error "set_idt_entry: please help me"
#endif

	if (dtrace_here)
		printk("set_idt_entry %p %p sz=%d %lx %lx\n", 
			&idt_table[intr], &s, (int) sizeof s, 
			((long *) &idt_table[intr])[0], 
			((long *) &idt_table[intr])[1]);

	idt_table[intr] = s;

}
#endif

/**********************************************************************/
/*   'ipl'  function inside a probe - let us know if interrupts were  */
/*   enabled or not.						      */
/**********************************************************************/
int
dtrace_getipl(void)
{	cpu_core_t *this_cpu = THIS_CPU();

	return this_cpu->cpuc_regs->r_rfl & X86_EFLAGS_IF ? 0 : 1;
}
/**********************************************************************/
/*   Interrupt  handler  for a double fault. (We may not enable this  */
/*   if we dont see a need).					      */
/**********************************************************************/
int 
dtrace_double_fault_handler(int type, struct pt_regs *regs)
{
static int cnt;

dtrace_printf("double-fault[%lu]: CPU:%d PC:%p\n", cnt++, smp_processor_id(), (void *) regs->r_pc-1);
	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Handle  a  single step trap - hopefully ours, as we step past a  */
/*   probe, but, if not, it belongs to someone else.		      */
/**********************************************************************/
int 
dtrace_int1_handler(int type, struct pt_regs *regs)
{	cpu_core_t *this_cpu = THIS_CPU();
	cpu_trap_t	*tp;

dtrace_printf("int1 PC:%p regs:%p CPU:%d\n", (void *) regs->r_pc-1, regs, smp_processor_id());
	/***********************************************/
	/*   Did  we expect this? If not, back to the  */
	/*   kernel.				       */
	/***********************************************/
	if (this_cpu->cpuc_mode != CPUC_MODE_INT1)
		return NOTIFY_KERNEL;

	/***********************************************/
	/*   Yup - its ours, so we can finish off the  */
	/*   probe.				       */
	/***********************************************/
	tp = &this_cpu->cpuc_trap[0];

	/***********************************************/
	/*   We  may  need to repeat the single step,  */
	/*   e.g. for REP prefix instructions.	       */
	/***********************************************/
	if (cpu_adjust(this_cpu, tp, regs) == FALSE)
		this_cpu->cpuc_mode = CPUC_MODE_IDLE;

	/***********************************************/
	/*   Dont let the kernel know this happened.   */
	/***********************************************/
//printk("int1-end: CPU:%d PC:%p\n", smp_processor_id(), (void *) regs->r_pc-1);
	return NOTIFY_DONE;
}
/**********************************************************************/
/*   Called  from  intr_<cpu>.S -- see if this is a probe for us. If  */
/*   not,  let  kernel  use  the normal notifier chain so kprobes or  */
/*   user land debuggers can have a go.				      */
/**********************************************************************/
static unsigned long cnt_int3_1;
static unsigned long cnt_int3_2;
int dtrace_int_disable;
int 
dtrace_int3_handler(int type, struct pt_regs *regs)
{	cpu_core_t *this_cpu = THIS_CPU();
	cpu_trap_t	*tp;
	int	ret;
static unsigned long cnt;

dtrace_printf("#%lu INT3 PC:%p REGS:%p CPU:%d mode:%d\n", cnt_int3_1++, regs->r_pc-1, regs, cpu_get_id(), this_cpu->cpuc_mode);
preempt_disable();

	/***********************************************/
	/*   Are  we idle, or already single stepping  */
	/*   a probe?				       */
	/***********************************************/
	if (dtrace_int_disable)
		goto switch_off;

	if (this_cpu->cpuc_mode == CPUC_MODE_IDLE) {
		/***********************************************/
		/*   Is this one of our probes?		       */
		/***********************************************/
		tp = &this_cpu->cpuc_trap[0];
		tp->ct_tinfo.t_doprobe = TRUE;
		/***********************************************/
		/*   Protect  ourselves  in case dtrace_probe  */
		/*   causes a re-entrancy.		       */
		/***********************************************/
		this_cpu->cpuc_mode = CPUC_MODE_INT1;
		this_cpu->cpuc_regs_old = this_cpu->cpuc_regs;
		this_cpu->cpuc_regs = regs;

		/***********************************************/
		/*   Now try for a probe.		       */
		/***********************************************/
//dtrace_printf("CPU:%d ... calling invop\n", cpu_get_id());
		ret = dtrace_invop(regs->r_pc - 1, (uintptr_t *) regs, 
			regs->r_rax, &tp->ct_tinfo);
		if (ret) {
			cnt_int3_2++;
			/***********************************************/
			/*   Let  us  know  on  return  we are single  */
			/*   stepping.				       */
			/***********************************************/
			this_cpu->cpuc_mode = CPUC_MODE_INT1;
			/***********************************************/
			/*   Copy  instruction  in its entirety so we  */
			/*   can step over it.			       */
			/***********************************************/
			cpu_copy_instr(this_cpu, tp, regs);
preempt_enable_no_resched();
dtrace_printf("INT3 %p called CPU:%d good finish flags:%x\n", regs->r_pc-1, cpu_get_id(), regs->r_rfl);
			this_cpu->cpuc_regs = this_cpu->cpuc_regs_old;
			return NOTIFY_DONE;
		}
		this_cpu->cpuc_mode = CPUC_MODE_IDLE;

		/***********************************************/
		/*   Not  fbt/sdt,  so lets see if its a USDT  */
		/*   (user space probe).		       */
		/***********************************************/
preempt_enable_no_resched();
		if (dtrace_user_probe(3, regs, (caddr_t) regs->r_pc, smp_processor_id())) {
			this_cpu->cpuc_regs = this_cpu->cpuc_regs_old;
			HERE();
			return NOTIFY_DONE;
		}

		/***********************************************/
		/*   Not ours, so let the kernel have it.      */
		/***********************************************/
dtrace_printf("INT3 %p called CPU:%d hand over\n", regs->r_pc-1, cpu_get_id());
		this_cpu->cpuc_regs = this_cpu->cpuc_regs_old;
		return NOTIFY_KERNEL;
	}

	/***********************************************/
	/*   If   we   get  here,  we  hit  a  nested  */
	/*   breakpoint,  maybe  a  page  fault hit a  */
	/*   probe.  Sort  of  bad  news  really - it  */
	/*   means   the   core   of  dtrace  touched  */
	/*   something  that we shouldnt have. We can  */
	/*   handle  that  by disabling the probe and  */
	/*   logging  the  reason  so we can at least  */
	/*   find evidence of this.		       */
	/*   We  need to find the underlying probe so  */
	/*   we can unpatch it.			       */
	/***********************************************/
dtrace_printf("recursive-int[%lu]: CPU:%d PC:%p\n", cnt++, smp_processor_id(), (void *) regs->r_pc-1);
dtrace_printf_disable = 1;

switch_off:
	tp = &this_cpu->cpuc_trap[1];
	tp->ct_tinfo.t_doprobe = FALSE;
	ret = dtrace_invop(regs->r_pc - 1, (uintptr_t *) regs, 
		regs->r_rax, &tp->ct_tinfo);
preempt_enable_no_resched();
	if (ret) {
		((unsigned char *) regs->r_pc)[-1] = tp->ct_tinfo.t_opcode;
		regs->r_pc--;
//preempt_enable();
		return NOTIFY_DONE;
	}

	/***********************************************/
	/*   Not ours, so let the kernel have it.      */
	/***********************************************/
	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Segment not present interrupt. Not sure if we really care about  */
/*   these, but lets see what happens.				      */
/**********************************************************************/
static unsigned long cnt_snp1;
static unsigned long cnt_snp2;
int 
dtrace_int11_handler(int type, struct pt_regs *regs)
{
	cnt_snp1++;
	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_NOFAULT)) {
		cnt_snp2++;
		/***********************************************/
		/*   Bad user/D script - set the flag so that  */
		/*   the   invoking   code   can   know  what  */
		/*   happened,  and  propagate  back  to user  */
		/*   space, dismissing the interrupt here.     */
		/***********************************************/
        	DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
	        cpu_core[CPU->cpu_id].cpuc_dtrace_illval = read_cr2_register();
		return NOTIFY_DONE;
		}

	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Detect  us  causing a GPF, before we ripple into a double-fault  */
/*   and  a hung kernel. If we trace something incorrectly, this can  */
/*   happen.  If it does, just disable as much of us as we can so we  */
/*   can diagnose. If its not our GPF, then let the kernel have it.   */
/*   								      */
/*   Additionally, dtrace.c will tell this code that we are about to  */
/*   do  a dangerous probe, e.g. "copyinstr()" from an unvalidatable  */
/*   address.   This   is   allowed   to  trigger  a  GPF,  but  the  */
/*   DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT)  avoids  this  become  a  */
/*   user  core  dump  or  kernel  panic.  Honor  this  bit  setting  */
/*   appropriately for a safe journey through dtrace.		      */
/**********************************************************************/
static unsigned long cnt_gpf1;
static unsigned long cnt_gpf2;
int 
dtrace_int13_handler(int type, struct pt_regs *regs)
{	cpu_core_t *this_cpu = THIS_CPU();

	cnt_gpf1++;

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_NOFAULT)) {
		cnt_gpf2++;
		/***********************************************/
		/*   Bad user/D script - set the flag so that  */
		/*   the   invoking   code   can   know  what  */
		/*   happened,  and  propagate  back  to user  */
		/*   space, dismissing the interrupt here.     */
		/***********************************************/
        	DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
	        cpu_core[CPU->cpu_id].cpuc_dtrace_illval = read_cr2_register();
		regs->r_pc += dtrace_instr_size((uchar_t *) regs->r_pc);
//printk("int13 - gpf - %p\n", read_cr2_register());
		return NOTIFY_DONE;
		}

	/***********************************************/
	/*   If we are idle, it couldnt have been us.  */
	/***********************************************/
	if (this_cpu->cpuc_mode == CPUC_MODE_IDLE)
		return NOTIFY_KERNEL;

	/***********************************************/
	/*   This could be due to a page fault whilst  */
	/*   single stepping. We need to let the real  */
	/*   page fault handler have a chance (but we  */
	/*   prey it wont fire a probe, but it can).   */
	/***********************************************/
	this_cpu->cpuc_regs_old = this_cpu->cpuc_regs;
	this_cpu->cpuc_regs = regs;
	
	dtrace_printf("INT13:GPF %p called\n", regs->r_pc-1);
	dtrace_printf_disable = 1;
	dtrace_int_disable = TRUE;

	this_cpu->cpuc_regs = this_cpu->cpuc_regs_old;
	return NOTIFY_DONE;
}
/**********************************************************************/
/*   Handle  a  page  fault  - if its us being faulted, else we will  */
/*   pass  on  to  the kernel to handle. We dont care, except we can  */
/*   have  issues  if  a  fault fires whilst we are trying to single  */
/*   step the kernel.						      */
/**********************************************************************/
unsigned long cnt_pf1;
unsigned long cnt_pf2;
int 
dtrace_int_page_fault_handler(int type, struct pt_regs *regs)
{
//dtrace_printf("PGF %p called pf%d\n", regs->r_pc-1, cnt_pf1);
	cnt_pf1++;
	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_NOFAULT)) {
		cnt_pf2++;
/*
if (0) {
set_console_on(1);
//dump_stack();
printk("dtrace cpu#%d PGF %p err=%p cr2:%p %02x %02x %02x %02x\n", 
	cpu_get_id(), regs->r_pc, regs->r_trapno, read_cr2_register(), 
	((unsigned char *) regs->r_pc)[0],
	((unsigned char *) regs->r_pc)[1],
	((unsigned char *) regs->r_pc)[2],
	((unsigned char *) regs->r_pc)[3]
	);
}
*/
		/***********************************************/
		/*   Bad user/D script - set the flag so that  */
		/*   the   invoking   code   can   know  what  */
		/*   happened,  and  propagate  back  to user  */
		/*   space, dismissing the interrupt here.     */
		/***********************************************/
        	DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
	        cpu_core[CPU->cpu_id].cpuc_dtrace_illval = read_cr2_register();

		/***********************************************/
		/*   Skip the offending instruction, which is  */
		/*   probably just a MOV instruction.	       */
		/***********************************************/
		regs->r_pc += dtrace_instr_size((uchar_t *) regs->r_pc);
		return NOTIFY_DONE;
		}

	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Intercept  SMP  IPI  inter-cpu  interrupts  so we can implement  */
/*   xcall.							      */
/**********************************************************************/
extern unsigned long cnt_ipi1;
/**********************************************************************/
/*   Saved copies of idt_table[n] for when we get unloaded.	      */
/**********************************************************************/
gate_t saved_double_fault;
gate_t saved_int1;
gate_t saved_int3;
gate_t saved_int11;
gate_t saved_int13;
gate_t saved_page_fault;
gate_t saved_ipi;

/**********************************************************************/
/*   This  gets  called  once we have been told what missing symbols  */
/*   are  available,  so we can do initiatisation. load.pl loads the  */
/*   kernel  and  then  when  we  are  happy,  we  can  complete the  */
/*   initialisation.						      */
/**********************************************************************/
static void
dtrace_linux_init(void)
{	hrtime_t	t, t1;
	gate_t *idt_table;

	if (driver_initted)
		return;
	driver_initted = TRUE;

	/***********************************************/
	/*   Let  timers  grab  any symbols they need  */
	/*   (eg hrtimer).			       */
	/***********************************************/
	init_cyclic();

	/***********************************************/
	/*   Needed  for dtrace_xcall emulation since  */
	/*   we cannot do inter-cpu IPI calls.	       */
	/***********************************************/
	fn_add_timer_on = get_proc_addr("add_timer_on");
	fn_sysrq_showregs_othercpus = get_proc_addr("sysrq_showregs_othercpus");

	/***********************************************/
	/*   Needed by assembler trap handler.	       */
	/***********************************************/
	kernel_int1_handler = get_proc_addr("debug");
	kernel_int3_handler = get_proc_addr("int3");
	kernel_int11_handler = get_proc_addr("segment_not_present");
	kernel_int13_handler = get_proc_addr("general_protection");
	kernel_double_fault_handler = get_proc_addr("double_fault");
	kernel_page_fault_handler = get_proc_addr("page_fault");

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
	/*   Compute     tsc_max_delta     so    that  */
	/*   dtrace_gethrtime  doesnt hang around for  */
	/*   too long. Similar to Solaris code.	       */
	/***********************************************/
	xtime_cache_ptr = (struct timespec *) get_proc_addr("xtime_cache");
	if (xtime_cache_ptr == NULL)
		xtime_cache_ptr = (struct timespec *) get_proc_addr("xtime");
	rdtscll(t);
	(void) dtrace_gethrtime();
	rdtscll(t1);
	tsc_max_delta = t1 - t;

	/***********************************************/
	/*   Following gives us a direct patch to the  */
	/*   INT3  interrupt  vector, so we can avoid  */
	/*   recursive  issues  in  kprobes and other  */
	/*   parts  of  the  kernel  which get called  */
	/*   before the notifier callback is invoked.  */
	/***********************************************/
	idt_table = get_proc_addr("idt_table");
	if (idt_table == NULL) {
		printk("dtrace: idt_table: not found - cannot patch INT3 handler\n");
	} else {
		saved_double_fault = idt_table[8];
		saved_int1 = idt_table[1];
		saved_int3 = idt_table[3];
		saved_int11 = idt_table[11];
		saved_int13 = idt_table[13];
		saved_page_fault = idt_table[14];
		saved_ipi = idt_table[ipi_vector];
#if 0 && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
		set_idt_entry(8, (unsigned long) dtrace_double_fault);
#endif
		set_idt_entry(1, (unsigned long) dtrace_int1); // single-step
		set_idt_entry(3, (unsigned long) dtrace_int3); // breakpoint
		set_idt_entry(11, (unsigned long) dtrace_int11); //segment_not_present
		set_idt_entry(13, (unsigned long) dtrace_int13); //GPF
		set_idt_entry(14, (unsigned long) dtrace_page_fault);

		/***********************************************/
		/*   ipi  vector  needed by xcall code if our  */
		/*   'new'  code  is  used.  Turn off for now  */
		/*   since  we havent got the API to allocate  */
		/*   a  free irq. And we dont seem to need it  */
		/*   in xcall.c.			       */
		/***********************************************/
/*
{int *first_v = get_proc_addr("first_system_vector");
set_bit(ipi_vector, used_vectors);
if (*first_v > ipi_vector)
	*first_v = ipi_vector;
}
*/
		if (ipi_vector)
			set_idt_entry(ipi_vector, (unsigned long) dtrace_int_ipi);
	}

	/***********************************************/
	/*   Initialise the io provider.	       */
	/***********************************************/
	io_prov_init();
}
/**********************************************************************/
/*   Cleanup notifications before we get unloaded.		      */
/**********************************************************************/
static int
dtrace_linux_fini(void)
{	int	ret = 1;
	gate_t *idt_table;

	if (fn_profile_event_unregister) {
		(*fn_profile_event_unregister)(PROFILE_TASK_EXIT, &n_exit);
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

	/***********************************************/
	/*   Lose  the  grab  on the interrupt vector  */
	/*   table.				       */
	/***********************************************/
	idt_table = get_proc_addr("idt_table");
	if (idt_table) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
//		idt_table[8] = saved_double_fault;
#endif
		idt_table[1] = saved_int1;
		idt_table[3] = saved_int3;
		idt_table[11] = saved_int11;
		idt_table[13] = saved_int13;
		idt_table[14] = saved_page_fault;
		if (ipi_vector)
	 		idt_table[ipi_vector] = saved_ipi;
	}

	return ret;
}
/**********************************************************************/
/*   Utility function to dump stacks for all cpus.		      */
/**********************************************************************/
static void dump_cpu_stack(void)
{
	printk("This is CPU#%d\n", smp_processor_id());
	dump_stack();
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
	if (arch_trigger_all_cpu_backtrace)
		arch_trigger_all_cpu_backtrace();
	else {
		set_console_on(0);
		dump_stack();
		smp_call_function_single(smp_processor_id() == 0 ? 1 : 0, dump_cpu_stack, 0, FALSE);
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
dtrace_linux_panic(void)
{
	if (dtrace_shutdown)
		return;

	dtrace_shutdown = TRUE;
	set_console_on(1);
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
/*   Make    this    a   function,   since   on   earlier   kernels,  */
/*   mutex_is_locked() is an inline complex function which cannot be  */
/*   used   in   an   expression  context  (ASSERT(MUTEX_HELD())  in  */
/*   dtrace.c)							      */
/**********************************************************************/
int
dtrace_mutex_is_locked(mutex_t *mp)
{
	return mutex_is_locked(mp);
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
/*   Internal logging mechanism for dtrace. Avoid calls to printk if  */
/*   we are in dangerous territory).				      */
/**********************************************************************/

void
dtrace_printf(char *fmt, ...)
{	short	ch;
	va_list	ap;
	unsigned long n;
	char	*cp;
	short	i;
	short	l_mode;
static	char	tmp[40];
	short	zero;
	short	width;
static char digits[] = "0123456789abcdef";
# define ADDCH(ch) {dtrace_buf[dbuf_i] = ch; dbuf_i = (dbuf_i + 1) % LOG_BUFSIZ;}

# if 0
	/***********************************************/
	/*   Temp: dont wrap buffer - because we want  */
	/*   to see first entries.		       */
	/***********************************************/
	if (dbuf_i >= LOG_BUFSIZ - 2048)
		return;
# endif
	if (dtrace_printf_disable)
		return;

	va_start(ap, fmt);
	if (dtrace_printk) {
		vprintk(fmt, ap);
		va_end(ap);
		return;
	}
	while ((ch = *fmt++) != '\0') {
		if (ch != '%') {
			ADDCH(ch);
			continue;
		}
		zero = ' ';
		width = -1;

		if ((ch = *fmt++) == '\0')
			break;
		if (ch == '0')
			zero = '0';
		while (ch >= '0' && ch <= '9') {
			if (width < 0)
				width = ch - '0';
			else
				width = 10 * width + ch - '0';
			if ((ch = *fmt++) == '\0')
				break;
		}
		l_mode = FALSE;
		if (ch == 'l') {
			l_mode = TRUE;
			if ((ch = *fmt++) == '\0')
				break;
		}
		switch (ch) {
		  case 'c':
		  	n = (char) va_arg(ap, int);
			ADDCH(n);
		  	break;
		  case 'd':
		  case 'u':
		  	n = va_arg(ap, int);
			if (ch == 'd' && (long) n < 0) {
				ADDCH('-');
				n = -n;
			}
			for (i = 0; i < 40; i++) {
				tmp[i] = '0' + (n % 10);
				n /= 10;
				if (n == 0)
					break;
			}
			while (i >= 0)
				ADDCH(tmp[i--]);
		  	break;
		  case 'p':
#if defined(__i386)
		  	width = 8;
#else
		  	width = 16;
#endif
			zero = '0';
			l_mode = TRUE;
			// fallthru...
		  case 'x':
			if (l_mode) {
			  	n = va_arg(ap, unsigned long);
			} else {
			  	n = va_arg(ap, unsigned int);
			}
			for (i = 0; ; ) {
				tmp[i++] = digits[(n & 0xf)];
				n >>= 4;
				if (n == 0)
					break;
			}
			width -= i;
			while (width-- > 0)
				ADDCH(zero);
			while (--i >= 0)
				ADDCH(tmp[i]);
		  	break;
		  case 's':
		  	cp = va_arg(ap, char *);
			while (*cp) {
				ADDCH(*cp++);
			}
		  	break;
		  }
	}
	va_end(ap);
}

static void
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
	int	repeat = 1;

/*static int levels[NCPU];
static int lev;
if (lev++) {
printk("%d - nested invocation to dtrace_sync\n", smp_processor_id());
}
if (levels[smp_processor_id()]++) {
printk("%d starting -- nested\n", smp_processor_id());
dump_stack();
}*/
	for (i = 0; i < repeat; i++) {
	        dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
	}
/*levels[smp_processor_id()]--;
lev--;*/
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
		if (dtrace_here)
			printk("get_proc_addr: No value for xkallsyms_lookup_name\n");
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
{
	int (*func)(unsigned long) = (int (*)(unsigned long)) syms[OFFSET_kernel_text_address].m_ptr;

	if (ktext <= (caddr_t) p && (caddr_t) p < ketext)
		return 1;

	if (func) {
		return func(p);
	}
	return 0;
}

# define	VMALLOC_SIZE	(100 * 1024)
void *
kmem_alloc(size_t size, int flags)
{	void *ptr;

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
# else
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
# endif

/**********************************************************************/
/*   Solaris  rdmsr  routine  only takes one arg, so we need to wrap  */
/*   the call.							      */
/**********************************************************************/
u64
lx_rdmsr(int x)
{	u32 val1, val2;

	rdmsr(x, val1, val2);
	return ((u64) val2 << 32) | val1;
}
int
validate_ptr(const void *ptr)
{	int	ret;

	__validate_ptr(ptr, ret);
//	printk("validate: ptr=%p ret=%d\n", ptr, ret);

	return ret;
}
# if defined(__amd64)
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
mem_set_writable(unsigned long addr, page_perms_t *pp, int perms)
{
	/***********************************************/
	/*   Dont   think  we  need  this  for  older  */
	/*   kernels  where  everything  is writable,  */
	/*   e.g. sys_call_table.		       */
	/***********************************************/
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16)
//	pgd_t *pgd = pgd_offset_k(addr);
	pgd_t *pgd = pgd_offset(current->mm, addr);
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	int dump_tree = FALSE;

	pp->pp_valid = FALSE;
	pp->pp_addr = addr;

	if (pgd_none(*pgd)) {
//		printk("yy: pgd=%lx\n", *pgd);
		return 0;
	}
	pud = pud_offset(pgd, addr);
	if (pud_none(*pud)) {
//		printk("yy: pud=%lx\n", *pud);
		return 0;
	}
	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd)) {
//		printk("yy: pmd=%lx\n", *pmd);
		return 0;
	}
	/***********************************************/
	/*   20091223  Soumendu  Sekhar Satapathy: If  */
	/*   large  memory  system,  then use the pmd  */
	/*   directly.				       */
	/***********************************************/
	if (pmd_large(*pmd)) {
		pte = (pte_t *) pmd;
	} else {
		pte = pte_offset_kernel(pmd, addr);
	}
	if (pte_none(*pte))
		return 0;

	if (dump_tree) {
		printk("yy -- begin\n");
		print_pte((pte_t *) pgd, 0);
		print_pte((pte_t *) pmd, 1);
		print_pte((pte_t *) pud, 2);
		print_pte(pte, 3);
		printk("yy -- end\n");
	}

	pp->pp_valid = TRUE;
	pp->pp_pgd = *pgd;
	pp->pp_pud = *pud;
	pp->pp_pmd = *pmd;
	pp->pp_pte = *pte;

	/***********************************************/
	/*   We only need to set these two, for now.   */
	/***********************************************/
	pmd->pmd |= perms;
	pte->pte |= perms;

	clflush(pmd);
	clflush(pte);
# endif
	return 1;
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

	clflush(pmd);
	clflush(pte);
	return TRUE;
}
# endif
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
{
#if defined(__i386)
	int level;
	pte_t *pte;

static pte_t *(*lookup_address)(void *, int *);

	if (lookup_address == NULL)
		lookup_address = get_proc_addr("lookup_address");
	if (lookup_address == NULL) {
		printk("dtrace:systrace.c: sorry - cannot locate lookup_address()\n");
		return 0;
		}

	pte = lookup_address(addr, &level);
	if ((pte_val(*pte) & _PAGE_RW) == 0) {
# if defined(__i386) && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
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
# else
	page_perms_t perms;

	mem_set_writable((unsigned long) addr, &perms, _PAGE_RW);
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
/*   Parallel allocator to avoid touching kernel data structures. We  */
/*   need  to  make  this  a  hash  table,  and provide free/garbage  */
/*   collection semantics.					      */
/**********************************************************************/
static struct par_alloc_t *hd_par;

void *
par_alloc(int domain, void *ptr, int size, int *init)
{	par_alloc_t *p;
	
	for (p = hd_par; p; p = p->pa_next) {
		if (p->pa_ptr == ptr && p->pa_domain == domain) {
			if (init)
				*init = FALSE;
			return p;
		}
	}

	if (init)
		*init = TRUE;

	if ((p = kmalloc(size + sizeof(*p), GFP_ATOMIC)) == NULL)
		return NULL;
	p->pa_domain = domain;
	p->pa_ptr = ptr;
	p->pa_next = hd_par;
	dtrace_bzero(p+1, size);
//printk("par_alloc %d -> %p\n", domain, p);
	hd_par = p;

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

	if (hd_par == p && hd_par->pa_domain == domain) {
		hd_par = hd_par->pa_next;
		kfree(ptr);
		return;
		}
	for (p1 = hd_par; p1 && p1->pa_next != p; p1 = p1->pa_next) {
//		printk("p1=%p\n", p1);
		}
	if (p1 == NULL) {
		printk("where did p1 go?\n");
		return;
	}
	if (p1->pa_next == p && p1->pa_domain == domain)
		p1->pa_next = p->pa_next;
	kfree(ptr);
}
/**********************************************************************/
/*   Map  pointer to the shadowed area. Dont create if its not there  */
/*   already.							      */
/**********************************************************************/
static void *
par_lookup(void *ptr)
{	par_alloc_t *p;
	
	for (p = hd_par; p; p = p->pa_next) {
		if (p->pa_ptr == ptr)
			return p;
		}
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
static void
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
/**********************************************************************/
/*   Call on proc exit, so we can detach ourselves from the proc. We  */
/*   may  have  a  USDT  enabled app dying, so we need to remove the  */
/*   probes. Also need to garbage collect the shadow proc structure,  */
/*   so we dont leak memory.					      */
/**********************************************************************/
static int 
proc_exit_notifier(struct notifier_block *n, unsigned long code, void *ptr)
{
	struct task_struct *task = (struct task_struct *) ptr;
	proc_t *p;

//printk("proc_exit_notifier: code=%lu ptr=%p\n", code, ptr);
	/***********************************************/
	/*   See  if  we know this proc - if so, need  */
	/*   to let fasttrap retire the probes.	       */
	/***********************************************/
	if (dtrace_fasttrap_exit_ptr &&
	    (p = par_find_thread(task)) != NULL) {
HERE();
		dtrace_fasttrap_exit_ptr(p);
HERE();
	}

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
extern void *(*fn_pid_task)(void *, int);

proc_t *
prfind(int p)
{	struct task_struct *tp;

	/***********************************************/
	/*   Rework  this - the first arg is a struct  */
	/*   pid *, not a pid.			       */
	/***********************************************/
	tp = fn_pid_task(p, PIDTYPE_PID);
	if (!tp)
		return (proc_t *) NULL;
HERE();
	return par_setup_thread1(tp);
}
void
rw_enter(krwlock_t *p, krw_t type)
{
	TODO();
}
void
rw_exit(krwlock_t *p)
{
	TODO();
}
/**********************************************************************/
/*   Utility  routine  for debugging, mostly not needed. Turn on all  */
/*   writes  to the console - may be needed when debugging low level  */
/*   interrupts which crash the box.   				      */
/**********************************************************************/
void
set_console_on(int flag)
{	int	mode = flag ? 7 : 0;
	int *console_printk = get_proc_addr("console_printk");

	if (console_printk)
		console_printk[0] = mode;
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
		hunt_init();
		dtrace_linux_init();

	}
	return orig_count;
}
/**********************************************************************/
/*   Handle invalid instruction exception, needed by FBT/SDT as they  */
/*   patch  code, with an illegal instruction (i386==F0, amd64==CC),  */
/*   and we get to call the handler (in dtrace_subr.c).		      */
/**********************************************************************/
/*
dotraplinkage void
trap_invalid_instr()
{
}
*/

void
trap(struct pt_regs *rp, caddr_t addr, processorid_t cpu)
{
TODO();
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
typedef struct seq_t {
	mutex_t seq_mutex;
	int	seq_id;
	} seq_t;

void *
vmem_alloc(vmem_t *hdr, size_t s, int flags)
{	seq_t *seqp = (seq_t *) hdr;
	void	*ret;

	if (TRACE_ALLOC || dtrace_mem_alloc)
		dtrace_printf("vmem_alloc(size=%d)\n", (int) s);

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

	return seqp;
}

void
vmem_free(vmem_t *hdr, void *ptr, size_t size)
{
	if (TRACE_ALLOC || dtrace_here)
		dtrace_printf("vmem_free(ptr=%p, size=%d)\n", ptr, (int) size);

}
void 
vmem_destroy(vmem_t *hdr)
{
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
	if (a->cr_uid == 0)
		return 1;

	dpend = &di_list[di_cnt];
//printk("priv_policy_only %p %x %d := trying...\n", a, priv, allzone);
	for (dp = di_list; dp < dpend; dp++) {
		switch (dp->di_type) {
		  case DIT_UID:
		  	if (dp->di_id != a->cr_uid)
				continue;
			break;
		  case DIT_GID:
		  	if (dp->di_id != a->cr_gid)
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

//HERE();
	ret = dtrace_open(file, 0, 0, CRED());
HERE();

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

	j = (dbuf_i + 1) % LOG_BUFSIZ;
	while (len > 0 && j != dbuf_i) {
		if (dtrace_buf[j]) {
			*buf++ = dtrace_buf[j];
			len--;
			n++;
		}
		j = (j + 1) % LOG_BUFSIZ;
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
	if (current->cred->uid != 0 && current->cred->euid != 0)
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
static int proc_calc_metrics(char *page, char **start, off_t off,
				 int count, int *eof, int len)
{
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;
}
static int proc_dtrace_debug_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{	int len;

	len = snprintf(page, count, 
		"here=%d\n"
		"cpuid=%d\n",
		dtrace_here,
		cpu_get_id());
	return proc_calc_metrics(page, start, off, count, eof, len);
}
static int proc_dtrace_security_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{	int len = count;
	char	tmpbuf[128];
	int	i;
	int	n = 0;
	int	size;
	char	*buf = page;

	/***********************************************/
	/*   Dump out the security table.	       */
	/***********************************************/
	for (i = 0; i < di_cnt && di_list[i].di_type && len > 0; i++) {
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

		size = snprintf(buf, len, "%s\n", tmpbuf);
		buf += size;
		len -= size;
		n += size;
	}
	return proc_calc_metrics(page, start, off, count, eof, n);
}
static int proc_dtrace_stats_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{	int	i, size;
	int	n = 0;
	char	*buf = page;
	extern unsigned long cnt_probe_recursion;
	extern unsigned long cnt_probes;
# define TYPE_LONG 0
# define TYPE_INT  1
	static struct map {
		int	type;
		unsigned long *ptr;
		char	*name;
		} stats[] = {
		{TYPE_LONG, &cnt_probes, "probes"},
		{TYPE_LONG, &cnt_probe_recursion, "probe_recursion"},
		{TYPE_LONG, &cnt_int3_1, "int3_1"},
		{TYPE_LONG, &cnt_int3_2, "int3_2"},
		{TYPE_LONG, &cnt_gpf1, "gpf1"},
		{TYPE_LONG, &cnt_gpf2, "gpf2"},
		{TYPE_LONG, &cnt_ipi1, "ipi1"},
		{TYPE_LONG, &cnt_pf1, "pf1"},
		{TYPE_LONG, &cnt_pf2, "pf2"},
		{TYPE_LONG, &cnt_snp1, "snp1"},
		{TYPE_LONG, &cnt_snp2, "snp2"},
		{TYPE_LONG, &cnt_xcall1, "xcall1"},
		{TYPE_LONG, &cnt_xcall2, "xcall2"},
		{TYPE_LONG, &cnt_xcall3, "xcall3(reentrant)"},
		{TYPE_LONG, &cnt_xcall4, "xcall4(delay)"},
		{TYPE_LONG, &cnt_xcall5, "xcall5(spinlock)"},
		{TYPE_INT, &dtrace_shutdown, "shutdown"},
		{0}
		};

	for (i = 0; i < MAX_DCNT && n < count; i++) {
		if (dcnt[i] == 0)
			continue;
		size = snprintf(buf, count - n, "dcnt%d=%lu\n", i, dcnt[i]);
		buf += size;
		n += size;
		}

	for (i = 0; stats[i].name; i++) {
		if (stats[i].type == TYPE_LONG)
			size = snprintf(buf, count - n, "%s=%lu\n", stats[i].name, *stats[i].ptr);
		else
			size = snprintf(buf, count - n, "%s=%d\n", stats[i].name, *(int *) stats[i].ptr);
		buf += size;
		n += size;
	}

	return proc_calc_metrics(page, start, off, count, eof, n);
}
static int proc_dtrace_trace_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{	int	j;
	int	pos = 0;
	int	n = 0;
	char	*buf = page;

	if (off >= LOG_BUFSIZ) {
		*eof = 1;
		return 0;
	}

	j = (dbuf_i + 1) % LOG_BUFSIZ;
	while (pos < off + count && n < count && j != dbuf_i) {
		if (dtrace_buf[j] && pos++ >= off) {
			*buf++ = dtrace_buf[j];
			n++;
		}
		j = (j + 1) % LOG_BUFSIZ;
	}
	if (j == dbuf_i)
		*eof = 1;
	*start = page;
	return n;
}

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
	struct proc_dir_entry *ent;

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
	/*   Lock  for  avoid  reentrancy problems in  */
	/*   dtrace_xcall.			       */
	/***********************************************/
	spin_lock_init(&xcall_spinlock);

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
		mutex_init(&cpu_list[i].cpu_ft_lock);
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
	ent = create_proc_entry("debug", S_IFREG | S_IRUGO | S_IWUGO, dir);
	ent->read_proc = proc_dtrace_debug_read_proc;

	ent = create_proc_entry("security", S_IFREG | S_IRUGO | S_IWUGO, dir);
	ent->read_proc = proc_dtrace_security_read_proc;

	ent = create_proc_entry("stats", S_IFREG | S_IRUGO | S_IWUGO, dir);
	ent->read_proc = proc_dtrace_stats_read_proc;

	ent = create_proc_entry("trace", S_IFREG | S_IRUGO | S_IWUGO, dir);
	ent->read_proc = proc_dtrace_trace_read_proc;

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "dtracedrv loaded: /dev/dtrace available, dtrace_here=%d nr_cpus=%d\n",
		dtrace_here, nr_cpus);

	dtrace_attach(NULL, 0);

	ctf_init();
	fasttrap_init();
	systrace_init();
	fbt_init();
	instr_init();
	dtrace_profile_init();
	sdt_init();
	ctl_init();

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

	ctl_exit();
	sdt_exit();
	dtrace_profile_fini();
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
}
module_init(dtracedrv_init);
module_exit(dtracedrv_exit);


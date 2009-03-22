/**********************************************************************/
/*   This file contains much of the glue between the Solaris code in  */
/*   dtrace.c  and  the linux kernel. We emulate much of the missing  */
/*   functionality, or map into the kernel.			      */
/*   								      */
/*   Date: April 2008						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
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
#include <linux/vmalloc.h>
#include <asm/tlbflush.h>
#include <asm/current.h>
#include <sys/rwlock.h>
#include <sys/privregs.h>
//#include <asm/pgtable.h>
//#include <asm/pgalloc.h>

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE("CDDL");
MODULE_DESCRIPTION("DTRACEDRV Driver");

# if !defined(CONFIG_NR_CPUS)
#	define	CONFIG_NR_CPUS	1
# endif

# define	TRACE_ALLOC	0

/**********************************************************************/
/*   Turn on HERE() macro tracing.				      */
/**********************************************************************/
int dtrace_here;
module_param(dtrace_here, int, 0);
int dtrace_mem_alloc;
module_param(dtrace_mem_alloc, int, 0);

static char *invop_msgs[] = {
	"DTRACE_INVOP_zero",
	"DTRACE_INVOP_PUSHL_EBP",
	"DTRACE_INVOP_POPL_EBP",
	"DTRACE_INVOP_LEAVE",
	"DTRACE_INVOP_NOP",
	"DTRACE_INVOP_RET",
	"DTRACE_INVOP_PUSHL_EDI",
	"DTRACE_INVOP_TEST_EAX_EAX",
	"DTRACE_INVOP_SUBL_ESP_nn",
	"DTRACE_INVOP_PUSHL_ESI",
	"DTRACE_INVOP_PUSHL_EBX",
	"DTRACE_INVOP_RET_IMM16",
	"DTRACE_INVOP_MOVL_nnn_EAX",
	};

/**********************************************************************/
/*   Stuff we stash away from /proc/kallsyms.			      */
/**********************************************************************/
static struct map {
	char		*m_name;
	unsigned long	*m_ptr;
	} syms[] = {
/* 0 */	{"kallsyms_op",            NULL},
/* 1 */	{"kallsyms_num_syms",      NULL},
/* 2 */	{"kallsyms_addresses",     NULL},
/* 3 */	{"kallsyms_expand_symbol", NULL}, /* No longer needed. */
/* 4 */	{"get_symbol_offset",      NULL}, /* No longer needed. */
/* 5 */	{"kallsyms_lookup_name",   NULL},
/* 6 */	{"modules",                NULL},
/* 7 */	{"__symbol_get",           NULL},
/* 8 */	{"sys_call_table",         NULL},
/* 9 */	{"**  reserved",           NULL},
/* 10 */{"hrtimer_cancel",         NULL},
/* 11 */{"hrtimer_start",          NULL},
/* 12 */{"hrtimer_init",           NULL},
/* 13 */{"access_process_vm",      NULL},
/* 14 */{"syscall_call",           NULL}, /* Backup for i386 2.6.23 kernel to help */
				 	  /* find the sys_call_table.		  */
/* 15 */{"print_modules",          NULL}, /* Backup for i386 2.6.23 kernel to help */
				 	  /* find the modules table. 		  */
/* 16 */{"kernel_text_address",    NULL},
/* 17 */{"_text",		   NULL}, /* Start of kernel code.	*/
/* 18 */{"_etext",		   NULL}, /* End of kernel code.	*/
	{0}
	};
static int xkallsyms_num_syms;
static long *xkallsyms_addresses;
static unsigned long (*xkallsyms_lookup_name)(char *);
static void *xmodules;
static void *(*x__symbol_get)(const char *);
static void **xsys_call_table;

uintptr_t	_userlimit = 0x7fffffff;
uintptr_t kernelbase = 0; //_stext;

/**********************************************************************/
/*   The  kernel  can be compiled with a lot of potential CPUs, e.g.  */
/*   64  is  not  untypical,  but  we  only have a dual core cpu. We  */
/*   allocate  buffers  for each cpu - which can mushroom the memory  */
/*   needed  (4MB+  per  core), and can OOM for small systems or put  */
/*   other systems into mortal danger as we eat all the memory.	      */
/**********************************************************************/
cpu_t	*cpu_list;
cpu_core_t *cpu_core;
cpu_t *cpu_table;
int	nr_cpus = 1;
DEFINE_MUTEX(mod_lock);

static DEFINE_MUTEX(dtrace_provider_lock);	/* provider state lock */
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

/**********************************************************************/
/*   Stuff for INT3 interception.				      */
/**********************************************************************/
static int proc_notifier_int3(struct notifier_block *, unsigned long, void *);
static struct notifier_block n_int3 = {
	.notifier_call = proc_notifier_int3,
	};
static	int (*fn_register_die_notifier)(struct notifier_block *);

static int (*fn_profile_event_register)(enum profile_type type, struct notifier_block *n);
static int (*fn_profile_event_unregister)(enum profile_type type, struct notifier_block *n);

/**********************************************************************/
/*   Notifier for invalid instruction trap.			      */
/**********************************************************************/
static int proc_notifier_trap_illop(struct notifier_block *, unsigned long, void *);
static struct notifier_block n_trap_illop = {
	.notifier_call = proc_notifier_trap_illop,
	};

/**********************************************************************/
/*   Process exiting notifier.					      */
/**********************************************************************/
static int proc_exit_notifier(struct notifier_block *, unsigned long, void *);
static struct notifier_block n_exit = {
	.notifier_call = proc_exit_notifier,
	};

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
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
int	sdt_init(void);
void	sdt_exit(void);
int	systrace_init(void);
void	systrace_exit(void);
static void print_pte(pte_t *pte, int level);

cred_t *
CRED()
{	static cred_t c;

	return &c;
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
//	if (!xtime_cache_ptr)
//		return 0;

	ts = *xtime_cache_ptr;
	return (hrtime_t) ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;

/*
	struct timeval tv;
	do_gettimeofday(&tv);
	return (hrtime_t) tv.tv_sec * 1000 * 1000 * 1000 + tv.tv_usec * 1000;
*/
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
void
debug_enter(char *arg)
{
	printk("%s(%d): %s\n", __FILE__, __LINE__, __func__);
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
/*   Register notifications for ourselves here.			      */
/**********************************************************************/
static void
dtrace_linux_init(void)
{
	hrtime_t	t, t1;

	if (fn_register_die_notifier)
		return;

	/***********************************************/
	/*   Register 0xcc INT3 breakpoint trap.       */
	/***********************************************/
	fn_register_die_notifier = get_proc_addr("register_die_notifier");
	if (fn_register_die_notifier == NULL) {
		printk("dtrace: register_die_notifier is NULL : FIX ME !\n");
	} else {
		(*fn_register_die_notifier)(&n_int3);
	}

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
	/*   We  need  to  intercept  invalid  opcode  */
	/*   exceptions for fbt/sdt.		       */
	/***********************************************/
	if (fn_register_die_notifier) {
		(*fn_register_die_notifier)(&n_trap_illop);
	}
	/***********************************************/
	/*   Compute     tsc_max_delta     so    that  */
	/*   dtrace_gethrtime  doesnt hang around for  */
	/*   too long. Similar to Solaris code.	       */
	/***********************************************/
	xtime_cache_ptr = (struct timespec *) get_proc_addr("xtime_cache");
	rdtscll(t);
	(void) dtrace_gethrtime();
	rdtscll(t1);
	tsc_max_delta = t1 - t;
}
/**********************************************************************/
/*   Cleanup notifications before we get unloaded.		      */
/**********************************************************************/
static void
dtrace_linux_fini(void)
{
	int (*fn_unregister_die_notifier)(struct notifier_block *) = 
		get_proc_addr("unregister_die_notifier");
	if (fn_unregister_die_notifier) {
		(*fn_unregister_die_notifier)(&n_int3);
	}
	if (fn_unregister_die_notifier) {
		(*fn_unregister_die_notifier)(&n_trap_illop);
	}
	if (fn_profile_event_unregister) {
		(*fn_profile_event_unregister)(PROFILE_TASK_EXIT, &n_exit);
	}
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
static void
dtrace_sync_func(void)
{
}

void
dtrace_sync(void)
{
        dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
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
/*ARGSUSED*/
void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	if (cpu == DTRACE_CPUALL) {
		/***********************************************/
		/*   Dont   call  smp_call_function  as  this  */
		/*   doesnt work.			       */
		/***********************************************/
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 26)
		on_each_cpu(func, arg, 0, TRUE);
#else
		on_each_cpu(func, arg, TRUE);
#endif
	} else {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 26)
		smp_call_function_single(cpu, func, arg, 0, TRUE);
#else
		smp_call_function_single(cpu, func, arg, TRUE);
#endif
	}
}

/**********************************************************************/
/*   Needed by systrace.c					      */
/**********************************************************************/
void *
fbt_get_sys_call_table(void)
{
	return xsys_call_table;
}
/**********************************************************************/
/*   Needed in cyclic.c						      */
/**********************************************************************/
void *
fbt_get_hrtimer_cancel(void)
{
	return (void *) syms[10].m_ptr;
}
void *
fbt_get_hrtimer_init(void)
{
	return (void *) syms[12].m_ptr;
}
void *
fbt_get_hrtimer_start(void)
{
	return (void *) syms[11].m_ptr;
}
void *
fbt_get_kernel_text_address(void)
{
	return (void *) syms[16].m_ptr;
}
void *
fbt_get_access_process_vm(void)
{
	return (void *) syms[13].m_ptr;
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
{
	if (xkallsyms_lookup_name == NULL) {
		printk("get_proc_addr: No value for xkallsyms_lookup_name\n");
		return 0;
		}

	return (void *) (*xkallsyms_lookup_name)(name);
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
		printk("kmem_alloc(%d) := %p ret=%p\n", (int) size, ptr, __builtin_return_address(0));
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
		printk("kmem_zalloc(%d) := %p\n", (int) size, ptr);
	return ptr;
}
void
kmem_free(void *ptr, int size)
{
	if (TRACE_ALLOC || dtrace_mem_alloc)
		printk("kmem_free(%p, size=%d)\n", ptr, size);
	if (size > VMALLOC_SIZE)
		vfree(ptr);
	else
		kfree(ptr);
}

int
lx_get_curthread_id()
{
	return 0;
}

/**********************************************************************/
/*   Test if a pointer is vaid in kernel space.			      */
/**********************************************************************/
# if defined(__i386)
#define __validate_ptr(ptr, ret)        \
 __asm__ __volatile__(      	      \
  "  mov $1, %0\n" 		      \
  "0: mov (%1), %1\n"                \
  "2:\n"       			      \
  ".section .fixup,\"ax\"\n"          \
  "3: mov $0, %0\n"    	              \
  " jmp 2b\n"     		      \
  ".previous\n"      		      \
  ".section __ex_table,\"a\"\n"       \
  " .align 4\n"     		      \
  " .long 0b,3b\n"     		      \
  ".previous"      		      \
  : "=&a" (ret) 		      \
  : "c" (ptr) 	                      \
  )
# else
#define __validate_ptr(ptr, ret)        \
 __asm__ __volatile__(      	      \
  "  mov $1, %0\n" 		      \
  "0: mov (%1), %1\n"                \
  "2:\n"       			      \
  ".section .fixup,\"ax\"\n"          \
  "3: mov $0, %0\n"    	              \
  " jmp 2b\n"     		      \
  ".previous\n"      		      \
  ".section __ex_table,\"a\"\n"       \
  " .align 8\n"     		      \
  " .quad 0b,3b\n"     		      \
  ".previous"      		      \
  : "=&a" (ret) 		      \
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
mem_set_writable(unsigned long addr, page_perms_t *pp)
{
	pgd_t *pgd = pgd_offset_k(addr);
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
	pte = pte_offset_kernel(pmd, addr);
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
	pmd->pmd |= _PAGE_RW;
	pte->pte |= _PAGE_RW;

	clflush(pmd);
	clflush(pte);
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
//static void (*flush_tlb_all)(void);

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
		/*   cpu  and  cpu  caches  no what happened,  */
		/*   else we may have random GPFs as we go to  */
		/*   do   a  write.  If  this  function  isnt  */
		/*   available,   we   are   pretty  much  in  */
		/*   trouble.				       */
		/***********************************************/
		__flush_tlb_all();
	}
# else
	page_perms_t perms;

	mem_set_writable((unsigned long) addr, &perms);
# endif
	return 1;
}

/**********************************************************************/
/*   MUTEX_NOT_HELD  macro  calls  mutex_count. Linux doesnt seem to  */
/*   have an assertion equivalent?				      */
/**********************************************************************/
int
mutex_count(mutex_t *mp)
{
	return atomic_read(&mp->count);
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
int cnt;
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
	int	do_print = die_chain == NULL;

	if (die_chain == NULL)
		die_chain = (struct atomic_notifier_head *) get_proc_addr("die_chain");
	if (task_exit_notifier == NULL)
		task_exit_notifier = (struct blocking_notifier_head *) get_proc_addr("task_exit_notifier");

	for (np = atomic_head(die_chain); np; np = np->next) {
		if (do_print) 
			printk("illop-chain: %p\n", np->notifier_call);
		if ((uint8_t *) np->notifier_call == ptr)
			return 1;
	}
	for (np = atomic_head(task_exit_notifier); np; np = np->next) {
		if (do_print)
			printk("exit-chain: %p\n", np->notifier_call);
		if ((uint8_t *) np->notifier_call == ptr)
			return 1;
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
par_alloc(void *ptr, int size, int *init)
{	par_alloc_t *p;
	int do_init = FALSE;
	
	for (p = hd_par; p; p = p->pa_next) {
		if (p->pa_ptr == ptr)
			return p;
		}

	do_init = TRUE;
	if (init)
		*init = do_init;

	if ((p = kmalloc(size + sizeof(*p), GFP_KERNEL)) == NULL)
		return NULL;
	p->pa_ptr = ptr;
	p->pa_next = hd_par;
	memset(p+1, 0, size);
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
par_free(void *ptr)
{	par_alloc_t *p = (par_alloc_t *) ptr;
	par_alloc_t *p1;

	if (hd_par == p) {
		hd_par = hd_par->pa_next;
		kfree(ptr);
		return;
		}
	for (p1 = hd_par; p1->pa_next != p; p1 = p1->pa_next)
		;
	if (p1->pa_next == p)
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
	par_setup_thread1(get_current());
}
void *
par_setup_thread1(struct task_struct *tp)
{	int	init = TRUE;
//	par_alloc_t *p = par_alloc(tp, sizeof *curthread, &init);
static par_alloc_t *static_p;
	par_alloc_t *p;
	sol_proc_t	*solp;

	if (static_p == NULL) {
		static_p = par_alloc(tp, sizeof *curthread, &init);
	}
	p = static_p;

	if (p == NULL)
		return NULL;

	solp = (sol_proc_t *) (p + 1);
	if (init) {
		mutex_init(&solp->p_lock);
		mutex_init(&solp->p_crlock);
	}

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
/*   Handle  an  INT3 (0xCC) single step trap, for USDT and let rest  */
/*   of  the  engine  know  something  fired. This happens for *all*  */
/*   breakpoint  traps,  so  we have a go and decide if its ours. If  */
/*   not,  we  let the rest of the kernel manage it, e.g. for gdb or  */
/*   strace. We should be able to run gdb on a proc which has active  */
/*   probes,  since  the  bkpt  address  determines  who has it. (Of  */
/*   course,  dont  try  and  plant  a  breakpoint  on  a  USDT trap  */
/*   instruction  as your user space app may never see it, but thats  */
/*   being  a  little silly I think, unless of course you are single  */
/*   stepping (stepi) through foreign code.			      */
/**********************************************************************/
static int proc_notifier_int3(struct notifier_block *n, unsigned long code, void *ptr)
{	struct die_args *args = (struct die_args *) ptr;
	struct pt_regs *regs = args->regs;
	int	ret;

	if (dtrace_here) {
		printk("proc_notifier_int3 INT3 called PC:%p CPU:%d\n", 
			(void *) regs->r_pc, 
			smp_processor_id());
	}

	ret = dtrace_invop(regs->r_pc - 1, 
		(uintptr_t *) regs, 
		regs->r_rax);

	if (dtrace_here) {
		printk("ret=%d %s\n", ret, ret == 0 ? "nothing" :
			ret < 0 || ret >= sizeof(invop_msgs) / sizeof invop_msgs[0] ? "??" : invop_msgs[ret]);
	}
	if (ret) {
		/***********************************************/
		/*   Emulate  a  single instruction - the one  */
		/*   we  patched  over.  Note we patched over  */
		/*   just  the first byte, so the 'ret' tells  */
		/*   us  what the missing byte is, but we can  */
		/*   use  the  rest of the instruction stream  */
		/*   to emulate the instruction.	       */
		/***********************************************/
		dtrace_cpu_emulate(ret, 0, regs);

		HERE();
		return NOTIFY_STOP;
	}

	/***********************************************/
	/*   Not FBT/SDT, so try for USDT...	       */
	/***********************************************/
	if (dtrace_user_probe(3, args->regs, 
		(caddr_t) args->regs->r_pc, 
		smp_processor_id())) {
		HERE();
		return NOTIFY_STOP;
	}

	return NOTIFY_OK;
}
/**********************************************************************/
/*   Handle illegal instruction trap for fbt/sdt.		      */
/**********************************************************************/
static int proc_notifier_trap_illop(struct notifier_block *n, unsigned long code, void *ptr)
{	struct die_args *args = (struct die_args *) ptr;

	if (dtrace_here) {
		printk("proc_notifier_trap_illop called! %s err:%ld trap:%d sig:%d PC:%p CPU:%d\n", 
			args->str, args->err, args->trapnr, args->signr,
			(void *) args->regs->r_pc, 
			smp_processor_id());
	}
	return NOTIFY_OK;

//# if defined(__amd64)
//	ret = dtrace_invop(regs->r_pc - 1, 
//# else
//	ret = dtrace_invop(regs->r_pc, 
//# endif
//		(uintptr_t *) regs, 
//		regs->r_rax);
//HERE();
//	if (dtrace_here) {
//		printk("ret=%d %s\n", ret, ret == DTRACE_INVOP_PUSHL_EBP ? "DTRACE_INVOP_PUSHL_EBP" : 
//			ret == DTRACE_INVOP_POPL_EBP ? "DTRACE_INVOP_POPL_EBP" :
//			ret == DTRACE_INVOP_LEAVE ? "DTRACE_INVOP_LEAVE" :
//			ret == DTRACE_INVOP_NOP ? "DTRACE_INVOP_NOP" :
//			ret == DTRACE_INVOP_RET ? "DTRACE_INVOP_RET" : "??");
//	}
//	if (ret) {
//		/***********************************************/
//		/*   Emulate  a  single instruction - the one  */
//		/*   we  patched  over.  Note we patched over  */
//		/*   just  the first byte, so the 'ret' tells  */
//		/*   us  what the missing byte is, but we can  */
//		/*   use  the  rest of the instruction stream  */
//		/*   to emulate the instruction.	       */
//		/***********************************************/
//		dtrace_cpu_emulate(ret, regs);
//
//		HERE();
//		return NOTIFY_STOP;
//	}
//	return NOTIFY_OK;
}
/**********************************************************************/
/*   Lookup process by proc id.					      */
/**********************************************************************/
proc_t *
prfind(int p)
{
	struct task_struct *tp = find_task_by_vpid(p);

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
	console_printk[0] = mode;
}
/**********************************************************************/
/*   Get a static symbol.					      */
/**********************************************************************/
void *
sym_get_static(char *name)
{
	if (strcmp(name, "_text") == 0)
		return (void *) syms[17].m_ptr;
	if (strcmp(name, "_etext") == 0)
		return (void *) syms[18].m_ptr;
	return NULL;
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
			if (1 || dtrace_here)
				printk("fbt: got %s=%p\n", mp->m_name, mp->m_ptr);
		}
		buf = symend + 1;
	}

	if (syms[1].m_ptr)
		xkallsyms_num_syms = *(int *) syms[1].m_ptr;
	xkallsyms_addresses 	= (long *) syms[2].m_ptr;
	xkallsyms_lookup_name 	= (unsigned long (*)(char *)) syms[5].m_ptr;
	xmodules 		= (void *) syms[6].m_ptr;
	x__symbol_get		= (void *(*)(const char *)) syms[7].m_ptr;
	xsys_call_table 	= (void **) syms[8].m_ptr;

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
# define OFFSET_modules		6
# define OFFSET_sys_call_table	8
# define OFFSET_syscall_call	14
# define OFFSET_print_modules	15
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

	if (xkallsyms_lookup_name) {

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
	struct mutex seq_mutex;
	int	seq_id;
	} seq_t;

void *
vmem_alloc(vmem_t *hdr, size_t s, int flags)
{	seq_t *seqp = (seq_t *) hdr;
	void	*ret;

	if (TRACE_ALLOC || dtrace_mem_alloc)
		printk("vmem_alloc(size=%d)\n", (int) s);

	mutex_lock(&seqp->seq_mutex);
	ret = (void *) (long) ++seqp->seq_id;
	mutex_unlock(&seqp->seq_mutex);
	return ret;
}

void *
vmem_create(const char *name, void *base, size_t size, size_t quantum,
        vmem_alloc_t *afunc, vmem_free_t *ffunc, vmem_t *source,
        size_t qcache_max, int vmflag)
{	seq_t *seqp = kmalloc(sizeof *seqp, GFP_KERNEL);

	if (TRACE_ALLOC || dtrace_here)
		printk("vmem_create(size=%d)\n", (int) size);

	mutex_init(&seqp->seq_mutex);
	seqp->seq_id = 0;

	return seqp;
}

void
vmem_free(vmem_t *hdr, void *ptr, size_t size)
{
	if (TRACE_ALLOC || dtrace_here)
		printk("vmem_free(ptr=%p, size=%d)\n", ptr, (int) size);

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

/**********************************************************************/
/*   TODO:  this  will  enable  dtrace_canload  and friends to let D  */
/*   programs  peek  around  in  kernel  space.  We probably want to  */
/*   parameter  or  create an ACL list loaded at run-time to do what  */
/*   solaris has internally.					      */
/**********************************************************************/
int
priv_policy_only(const cred_t *a, int b, int c)
{
        return 1;
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
/**********************************************************************/
/*   Module interface for /dev/dtrace - the main driver.	      */
/**********************************************************************/
static int
dtracedrv_open(struct inode *inode, struct file *file)
{	int	ret;

//HERE();
	ret = dtrace_open(file, 0, 0, NULL);
HERE();

	return -ret;
# if 0
	/*
	 * Ask all providers to provide their probes.
	 */
	mutex_enter(&dtrace_provider_lock);
	dtrace_probe_provide(NULL);
	mutex_exit(&dtrace_provider_lock);

	return 0;
# endif
}
static int
dtracedrv_release(struct inode *inode, struct file *file)
{
//HERE();
	dtrace_close(file, 0, 0, NULL);
	return 0;
}
static ssize_t
dtracedrv_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
/*
long *sys_call_table = get_proc_addr("sys_call_table");
printk("sys_call_table=%p %lx\n", sys_call_table, sys_call_table[0]);
long x = *sys_call_table;
page_perms_t perms;

mem_set_writable((unsigned long) sys_call_table, &perms);
sys_call_table[0] = x;
mem_unset_writable(&perms);
printk("Line %d\n", __LINE__);
*/
return -EIO;

}
# if 0
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
# endif
# if 0
static int dtracedrv_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{	int len;

	// place holder for something useful later on.
	len = sprintf(page, "hello");
	return proc_calc_metrics(page, start, off, count, eof, len);
}
# endif
static int dtracedrv_ioctl(struct inode *inode, struct file *file,
                     unsigned int cmd, unsigned long arg)
{	int	ret;
	int	rv = 0;

	ret = dtrace_ioctl(file, cmd, arg, 0, NULL, &rv);
//HERE();
//if (dtrace_here && ret) printk("ioctl-returns: ret=%d rv=%d\n", ret, rv);
        return ret ? -ret : rv;
}
static const struct file_operations dtracedrv_fops = {
        .read = dtracedrv_read,
        .ioctl = dtracedrv_ioctl,
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
        .ioctl = helper_ioctl,
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
static struct proc_dir_entry *dir;

	/***********************************************/
	/*   Create the parent directory.	       */
	/***********************************************/
	if (!dir) {
		dir = proc_mkdir("dtrace", NULL);
		if (!dir) {
			printk("Cannot create /proc/dtrace\n");
			return -1;
		}
	}

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
	for (i = 0; i < nr_cpus; i++) {
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

# if 0
	struct proc_dir_entry *ent;
//	create_proc_read_entry("dtracedrv", 0, NULL, dtracedrv_read_proc, NULL);
	ent = create_proc_entry("dtrace", S_IFREG | S_IRUGO | S_IWUGO, dir);
	ent->read_proc = dtracedrv_read_proc;

	ent = create_proc_entry("helper", S_IFREG | S_IRUGO | S_IWUGO, dir);
	ent->read_proc = dtracedrv_helper_read_proc;
# endif
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

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "dtracedrv loaded: /dev/dtrace available, dtrace_here=%d\n",
		dtrace_here);

	dtrace_attach(NULL, 0);

	ctf_init();
	fasttrap_init();
	fbt_init();
	systrace_init();
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

	return 0;
}
static void __exit dtracedrv_exit(void)
{
	if (dtrace_attached()) {
		dtrace_detach(NULL, 0);
		}

	misc_deregister(&helper_dev);

	dtrace_linux_fini();
	ctl_exit();
	sdt_exit();
	dtrace_profile_fini();
	systrace_exit();
	fbt_exit();
	ctf_exit();
	fasttrap_exit();

	printk(KERN_WARNING "dtracedrv driver unloaded.\n");
	remove_proc_entry("dtrace/dtrace", 0);
	remove_proc_entry("dtrace/helper", 0);
	remove_proc_entry("dtrace", 0);
	misc_deregister(&dtracedrv_dev);
}
module_init(dtracedrv_init);
module_exit(dtracedrv_exit);


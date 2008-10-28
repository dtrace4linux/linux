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
#include <linux/uaccess.h>
#include <linux/sys.h>
#include <linux/thread_info.h>
#include <linux/smp.h>
#include <asm/current.h>

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE("CDDL");
MODULE_DESCRIPTION("DTRACEDRV Driver");

# if !defined(CONFIG_NR_CPUS)
#	define	CONFIG_NR_CPUS	1
# endif

/**********************************************************************/
/*   Turn on HERE() macro tracing.				      */
/**********************************************************************/
int dtrace_here;
module_param(dtrace_here, int, 0);

uintptr_t	_userlimit = 0x7fffffff;
uintptr_t kernelbase = 0; //_stext;
cpu_t	*cpu_list;
cpu_core_t cpu_core[CONFIG_NR_CPUS];
cpu_t cpu_table[NCPU];
EXPORT_SYMBOL(cpu_list);
EXPORT_SYMBOL(cpu_core);
EXPORT_SYMBOL(cpu_table);
DEFINE_MUTEX(mod_lock);

static DEFINE_MUTEX(dtrace_provider_lock);	/* provider state lock */
DEFINE_MUTEX(cpu_lock);
int	panic_quiesce;
sol_proc_t	*curthread;

dtrace_vtime_state_t dtrace_vtime_active = 0;
dtrace_cacheid_t dtrace_predcache_id = DTRACE_CACHEIDNONE + 1;

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
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

cred_t *
CRED()
{	static cred_t c;

	return &c;
}
void
atomic_add_64(uint64_t *p, int n)
{
	*p += n;
}
hrtime_t
dtrace_gethrtime()
{	struct timeval tv;

	do_gettimeofday(&tv);
	return (hrtime_t) tv.tv_sec * 1000 * 1000 * 1000 + tv.tv_usec * 1000;
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
                panic(buf);
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
dump_mem(char *cp, int len)
{	char	buf[128];
	int	i;

	while (len > 0) {
		sprintf(buf, "%p", cp);
		for (i = 0; i < 16 && len-- > 0; i++) {
			sprintf(buf + strlen(buf), "%02x ", *cp++ & 0xff);
			}
		strcat(buf, "\n");
		printk(buf);
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
		on_each_cpu(func, arg, 0, TRUE);
	} else {
		smp_call_function_single(cpu, func, arg, 0, TRUE);
	}
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
fuword32(const void *addr, uint32_t *valuep)
{
	if (!validate_ptr((void *) addr))
		return 1;

	*valuep = dtrace_fuword32((void *) addr);
	return 0;
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
# if 0
void *
kmem_alloc(size_t size, int flags)
{
	return kmalloc(size, flags);
}
void *
kmem_zalloc(size_t size, int flags)
{	void *ptr = kmalloc(size, flags);

	if (ptr)
		memset(ptr, 0, size);
	return ptr;
}
void
kmem_free(void *ptr, int size)
{
	kfree(ptr);
}
# endif

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

	p = kmalloc(size + sizeof(*p), GFP_KERNEL);
	p->pa_ptr = ptr;
	p->pa_next = hd_par;
	memset(p+1, 0, size);
	hd_par = p;

	return p;
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
void *
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
{	int	init;
	par_alloc_t *p = par_alloc(tp, sizeof *curthread, &init);
	sol_proc_t	*solp = (sol_proc_t *) (p + 1);

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
	par_alloc_t *p = par_lookup(get_current());
	sol_proc_t	*solp;

	if (p == NULL)
		return NULL;

	solp = (sol_proc_t *) (p + 1);
	return solp;
}
/**********************************************************************/
/*   Lookup process by proc id.					      */
/**********************************************************************/
proc_t *
prfind(int p)
{
	struct task_struct *tp = find_task_by_pid(p);

	if (!tp)
		return tp;
HERE();
	return par_setup_thread1(tp);
}
/**********************************************************************/
/*   Read from a procs memory.					      */
/**********************************************************************/
int 
uread(proc_t *p, void *addr, size_t len, uintptr_t dest)
{	int (*func)(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write) = 
		fbt_get_access_process_vm();

	return func(p, (unsigned long) addr, (void *) dest, len, 0);
}
int 
uwrite(proc_t *p, void *addr, size_t len, uintptr_t src)
{	int (*func)(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write) = 
		fbt_get_access_process_vm();

	return func(p, (unsigned long) addr, (void *) src, len, 1);
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

	mutex_init(&seqp->seq_mutex);
	seqp->seq_id = 0;

	return seqp;
}

void
vmem_free(vmem_t *hdr, void *ptr, size_t s)
{

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
priv_policy_only(const cred_t *a, int b, int c)
{
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
static int
helper_read(ctf_file_t *fp, int fd)
{
	return -EIO;
}
static int 
helper_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{	int len;

	// place holder for something useful later on.
	len = sprintf(page, "hello");
	return 0;
}
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
if (dtrace_here && ret) printk("ioctl-returns: ret=%d rv=%d\n", ret, rv);
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
static int
dtracedrv_read(ctf_file_t *fp, int fd)
{
//HERE();
	return -EIO;
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
static int dtracedrv_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{	int len;

	// place holder for something useful later on.
	len = sprintf(page, "hello");
	return proc_calc_metrics(page, start, off, count, eof, len);
}
static int dtracedrv_ioctl(struct inode *inode, struct file *file,
                     unsigned int cmd, unsigned long arg)
{	int	ret;
	int	rv = 0;

	ret = dtrace_ioctl(file, cmd, arg, 0, NULL, &rv);
//HERE();
if (dtrace_here && ret) printk("ioctl-returns: ret=%d rv=%d\n", ret, rv);
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
	/***********************************************/
	cpu_list = (cpu_t *) kzalloc(sizeof *cpu_list * NR_CPUS, GFP_KERNEL);
	for (i = 0; i < NR_CPUS; i++) {
		cpu_list[i].cpuid = i;
		cpu_list[i].cpu_next = &cpu_list[i+1];
		/***********************************************/
		/*   BUG/TODO:  We should adapt the onln list  */
		/*   to handle actual online cpus.	       */
		/***********************************************/
		cpu_list[i].cpu_next_onln = &cpu_list[i+1];
		mutex_init(&cpu_list[i].cpu_ft_lock);
		}
	cpu_list[NR_CPUS-1].cpu_next = cpu_list;

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
	return 0;
}
static void __exit dtracedrv_exit(void)
{
	if (dtrace_attached()) {
		dtrace_detach(NULL, 0);
		}

	misc_deregister(&helper_dev);

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


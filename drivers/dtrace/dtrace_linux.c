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
#include <asm/current.h>

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE("CDDL");
MODULE_DESCRIPTION("DTRACEDRV Driver");

# if !defined(CONFIG_NR_CPUS)
#	define	CONFIG_NR_CPUS	1
# endif

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
int	fasttrap_init(void);
void	fasttrap_exit(void);
int	fbt_init(void);
void	fbt_exit(void);
int	systrace_init(void);
void	systrace_exit(void);

cred_t *
CRED()
{	static cred_t c;

	return &c;
}
hrtime_t
dtrace_gethrtime()
{	struct timeval tv;

	do_gettimeofday(&tv);
	return tv.tv_sec * 1000 * 1000 * 1000 + tv.tv_usec * 1000;
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
static int
dtrace_xcall_func(dtrace_xcall_t func, void *arg)
{
        (*func)(arg);

        return (0);
}
/*ARGSUSED*/
void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	kpreempt_disable();
	if (cpu == DTRACE_CPUALL) {
		smp_call_function(func, arg, 0, TRUE);
	} else {
		smp_call_function_single(cpu, func, arg, 0, TRUE);
	}
	kpreempt_enable();
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

/**********************************************************************/
/*   Test if a pointer is vaid in kernel space.			      */
/**********************************************************************/
# if __i386
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

int
validate_ptr(void *ptr)
{	int	ret;

	__validate_ptr(ptr, ret);
//	printk("validate: ptr=%p ret=%d\n", ptr, ret);

	return ret;
}

/**********************************************************************/
/*   Parallel allocator to avoid touching kernel data structures. We  */
/*   need  to  make  this  a  hash  table,  and provide free/garbage  */
/*   collection semantics.					      */
/**********************************************************************/
static struct par_alloc_t *hd_par;

void *
par_alloc(void *ptr, int size)
{	par_alloc_t *p;

	for (p = hd_par; p; p = p->pa_next) {
		if (p->pa_ptr == ptr)
			return p;
		}

	p = kmalloc(size + sizeof(*p), GFP_KERNEL);
	p->pa_ptr = ptr;
	p->pa_next = hd_par;
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
/*   Module interface to the kernel.				      */
/**********************************************************************/
static int
dtracedrv_open(struct inode *inode, struct file *file)
{	int	ret;

HERE();
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
HERE();
	dtrace_close(file, 0, 0, NULL);
	return 0;
}
static int
dtracedrv_read(ctf_file_t *fp, int fd)
{
HERE();
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
	len = sprintf(page, "hello");
//	printk(KERN_WARNING "pgfault: %s", page);
	return proc_calc_metrics(page, start, off, count, eof, len);
}
static int dtracedrv_helper_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{	int len;
	len = sprintf(page, "hello");
//	printk(KERN_WARNING "pgfault: %s", page);
	return proc_calc_metrics(page, start, off, count, eof, len);
}
static int dtracedrv_ioctl(struct inode *inode, struct file *file,
                     unsigned int cmd, unsigned long arg)
{	int	ret;
	int	rv = 0;

	ret = dtrace_ioctl(file, cmd, arg, 0, NULL, &rv);
HERE();
printk("ioctl-returns: ret=%d rv=%d\n", ret, rv);
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
		printk(KERN_WARNING "dtracedrv: Unable to register misc device\n");
		return ret;
		}

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "dtracedrv loaded: /dev/dtrace now available\n");

	dtrace_attach(NULL, 0);

	ctf_init();
	fasttrap_init();
	fbt_init();
	systrace_init();
	return 0;
}
static void __exit dtracedrv_exit(void)
{
	if (dtrace_attached()) {
		dtrace_detach(NULL, 0);
		}

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


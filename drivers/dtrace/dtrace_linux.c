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
#include <sys/dtrace.h>
#include <linux/cpumask.h>

#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE("CDDL");
MODULE_DESCRIPTION("DTRACEDRV Driver");

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
void dtrace_probe_provide(dtrace_probedesc_t *desc);
int dtrace_detach(void);
int dtrace_ioctl_helper(int cmd, intptr_t arg, int *rv);
int dtrace_ioctl(dev_t dev, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);
int dtrace_attach(dev_info_t *devi, int cmd);
int dtrace_open(dev_t *devp, int flag, int otyp, cred_t *cred_p);
int	ctf_init(void);
void	ctf_exit(void);
int	fasttrap_init(void);
void	fasttrap_exit(void);
int	fbt_init(void);
void	fbt_exit(void);

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
debug_enter(int arg)
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
	dtrace_vtime_state_t state, nstate;

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
	dtrace_vtime_state_t state, nstate;

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
/*   Need to implement this or use the unr code from FreeBSD.	      */
/**********************************************************************/
static int vm_id;

void *
vmem_alloc(vmem_t *hdr, size_t s, int flags)
{
	return ++vm_id;
	
	printk("vmem_alloc: hdr=%x\n", hdr);
	return kmem_cache_alloc(hdr, GFP_KERNEL);
}

void *
vmem_create(const char *name, void *base, size_t size, size_t quantum,
        vmem_alloc_t *afunc, vmem_free_t *ffunc, vmem_t *source,
        size_t qcache_max, int vmflag)
{
	return NULL;
}

void
vmem_free(vmem_t *hdr, void *ptr, size_t s)
{
	return;

	kmem_cache_free(hdr, ptr);
}
void 
vmem_destroy(vmem_t *hdr)
{
}
int
priv_policy_only(const cred_t *a, int b, int c)
{
        return 0;
}
/*
void
dtrace_copy(uintptr_t src, uintptr_t dest, size_t size)
{
	memcpy(dest, src, size);
}
void
dtrace_copystr(uintptr_t uaddr, uintptr_t kaddr, size_t size)
{
	strlcpy(uaddr, kaddr, size);
}

*/
/**********************************************************************/
/*   Module interface to the kernel.				      */
/**********************************************************************/
static int
dtracedrv_open(struct inode *inode, struct file *file)
{	int	ret;

TODO();
	ret = dtrace_open(inode, 0, 0, NULL);
TODO();

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
dtracedrv_read(ctf_file_t *fp, int fd)
{
TODO();
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
	int	rv;

	ret = dtrace_ioctl(0, cmd, arg, 0, NULL, &rv);
        return -ret;
}
static const struct file_operations dtracedrv_fops = {
        .read = dtracedrv_read,
        .ioctl = dtracedrv_ioctl,
        .open = dtracedrv_open,
};

static struct miscdevice dtracedrv_dev = {
        MISC_DYNAMIC_MINOR,
        "dtrace",
        &dtracedrv_fops
};
static int __init dtracedrv_init(void)
{	int	ret;
static struct proc_dir_entry *dir;
	struct proc_dir_entry *ent;

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

# if 0
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
	return 0;
}
static void __exit dtracedrv_exit(void)
{
	dtrace_detach();

	ctf_exit();
	fasttrap_exit();
	fbt_exit();

	printk(KERN_WARNING "dtracedrv driver unloaded.\n");
	remove_proc_entry("dtrace/dtrace", 0);
	remove_proc_entry("dtrace/helper", 0);
	remove_proc_entry("dtrace", 0);
	misc_deregister(&dtracedrv_dev);
}
module_init(dtracedrv_init);
module_exit(dtracedrv_exit);


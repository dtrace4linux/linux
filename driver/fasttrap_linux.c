/**********************************************************************/
/*   Interface   between   Linux   /dev/fasttrap  and  the  code  in  */
/*   fasttrap.c.  The  ioctl()  interface  is where the PID provider  */
/*   comes into being.						      */
/*   								      */
/*   Date: April 2008						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/*   $Header: Last edited: 11-Jul-2012 1.1 $			      */
/**********************************************************************/

#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <linux/cpumask.h>

#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_DESCRIPTION("DTRACE/FASTTRAP Driver");

int fasttrap_ioctl(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);

/**********************************************************************/
/*   Module interface to the kernel.				      */
/**********************************************************************/
static int
fasttrap_open(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t
fasttrap_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
TODO();
	return -EIO;
}
static int 
fasttrap_linux_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{	int	ret;
	int	rv = 0;

	ret = fasttrap_ioctl(file, cmd, arg, 0, CRED(), &rv);
        return ret ? -ret : rv;
}
#ifdef HAVE_UNLOCKED_IOCTL
static long fasttrap_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return fasttrap_linux_ioctl(NULL, file, cmd, arg);
}
#endif

#ifdef HAVE_COMPAT_IOCTL
static long fasttrap_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return fasttrap_linux_ioctl(NULL, file, cmd, arg);
}
#endif
static const struct file_operations fasttrap_fops = {
	.read = fasttrap_read,
#ifdef HAVE_OLD_IOCTL
  	.ioctl = fasttrap_linux_ioctl,
#endif
#ifdef  HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl = fasttrap_unlocked_ioctl,
#endif
#ifdef HAVE_COMPAT_IOCTL
        .compat_ioctl = fasttrap_compat_ioctl,
#endif
	.open = fasttrap_open,
};

static struct miscdevice fasttrap_dev = {
        MISC_DYNAMIC_MINOR,
        "fasttrap",
        &fasttrap_fops
};

static int initted;

int fasttrap_init(void)
{	int	ret;

	ret = misc_register(&fasttrap_dev);
	if (ret) {
		printk(KERN_WARNING "fasttrap: Unable to register misc device\n");
		return ret;
		}

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
//	printk(KERN_WARNING "fasttrap loaded: /dev/fasttrap now available\n");

	fasttrap_attach();

	initted = TRUE;

	return 0;
}
void fasttrap_exit(void)
{
	if (initted) {
#if linux
		/***********************************************/
		/*   Rip  out  the  fasttrap helpers from all  */
		/*   processes in the system.		       */
		/***********************************************/
		void dtrace_helper_remove_all(void);
		dtrace_helper_remove_all();
#endif
		fasttrap_detach();
		misc_deregister(&fasttrap_dev);
	}
//	printk(KERN_WARNING "fasttrap driver unloaded.\n");
}


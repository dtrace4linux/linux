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
MODULE_LICENSE("CDDL");
MODULE_DESCRIPTION("DTRACE/FASTTRAP Driver");

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
#if 0
static int 
fasttrap_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
TODO();
	return -EIO;
}
#endif
static const struct file_operations fasttrap_fops = {
	.read = fasttrap_read,
//	.ioctl = fasttrap_ioctl,
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
# if 0
	struct proc_dir_entry *ent;
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

	ent = create_proc_entry("fasttrap", S_IFREG | S_IRUGO, dir);
	ent->read_proc = fasttrap_read_proc;
# endif

	ret = misc_register(&fasttrap_dev);
	if (ret) {
		printk(KERN_WARNING "fasttrap: Unable to register misc device\n");
		return ret;
		}

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "fasttrap loaded: /dev/fasttrap now available\n");

	fasttrap_attach();

	initted = TRUE;

	return 0;
}
void fasttrap_exit(void)
{
	if (initted) {
		fasttrap_detach();
		misc_deregister(&fasttrap_dev);
	}
	printk(KERN_WARNING "fasttrap driver unloaded.\n");
}


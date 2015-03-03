/**********************************************************************/
/*   Driver  to  implement simple reading of kernel. Needed on older  */
/*   unixes  where  /proc/kcore  is  broken.  Some  things  are  not  */
/*   exported by the kallsyms device.				      */
/*   								      */
/*   This  driver  is for use by tools/kcore.c - its not designed to  */
/*   be for generic use. Use /proc/kcore on a working kernel instead  */
/*   for that.							      */
/*   								      */
/*   Author: Paul Fox						      */
/*   Date: March 2015						      */
/**********************************************************************/
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

/**********************************************************************/
/*   Module interface to the kernel.				      */
/**********************************************************************/
static int
kmem_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t
kmem_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
	int	n;
	loff_t off2 = *off;

	/***********************************************/
	/*   Dirty  hack. RH5.6 cannot lseek() to the  */
	/*   kernel  addresses,  so we just seek to a  */
	/*   32b truncated version of the address.     */
	/***********************************************/
	off2 |= 0xffffffff00000000UL;
//printk("off2=%lx\n", off2);
	n = copy_to_user(buf, (char *) off2, len);
//printk("n=%d\n", n);
	if (n)
		return -EFAULT;
	*off += len;

	return len;
}

static struct file_operations proc_kmem = {
	.owner   = THIS_MODULE,
	.open    = kmem_open,
	.read    = kmem_read,
};

static int __init kmem_init(void)
{
	proc_create("dtrace_kmem", S_IFREG | 0400, NULL, &proc_kmem);

	return 0;
}

void
kmem_exit(void)
{
	remove_proc_entry("dtrace_kmem", 0);
}

module_init(kmem_init);
module_exit(kmem_exit);


/**********************************************************************/
/*   Simple  hello-world type driver that creates /proc/hello-world,  */
/*   and lets us use as a target for SDT dtrace probes.		      */
/**********************************************************************/
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <sys/sdt.h>

static int num_opens;

/**********************************************************************/
/*   Module interface to the kernel.				      */
/**********************************************************************/
static int
hworld_open(struct inode *inode, struct file *file)
{
	DTRACE_SDT_PROBE1(hworld, open_module, open1, entry, num_opens);
	num_opens++;

	DTRACE_SDT_PROBE1(hworld, open_module, open2, entry, num_opens);
	return 0;
}

static ssize_t
hworld_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{	static int x;
	int	n;
	char	*str;

	DTRACE_SDT_PROBE1(hworld, read_module, read, entry, num_opens);

	if (*off)
		return 0;

	if (x++ & 1)
		str = "Echoes....<ping>!\n";
	else
		str = "Time....<ring>!\n";

	n = strlen(str) > len ? len : strlen(str);
	memcpy(buf, str, n);
	*off += n;

	return n;
}


static struct file_operations proc_hworld = {
	.owner   = THIS_MODULE,
	.open    = hworld_open,
	.read    = hworld_read,
};

static int __init hworld_init(void)
{
	proc_create("hello-world", S_IFREG | S_IRUGO | S_IWUGO, NULL, &proc_hworld);

	return 0;
}

void
hworld_exit(void)
{
	remove_proc_entry("hello-world", 0);
}

module_init(hworld_init);
module_exit(hworld_exit);


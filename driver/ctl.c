/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/* This code is likely to be removed in a future release - it doesnt
work, and needs more work to complete it, and isnt needed at
present.
*/

/**********************************************************************/
/*   This  file  contains  a thin /proc/$pid/ctl interface (procfs).  */
/*   This  interface is 'legacy' at the moment (Feb 2009), since the  */
/*   experiment  to  make  it  workable  didnt  pan out - its a good  */
/*   start,  but  not  complete to use as a Solaris compatible /proc  */
/*   interface for Linux.					      */
/*   								      */
/*   $Header: Last edited: 10-Jul-2010 1.2 $ 			      */
/**********************************************************************/

#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <dtrace_proto.h>
# undef task_struct
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <sys/modctl.h>
#include <sys/dtrace.h>
#include <sys/stack.h>
#include <linux/ptrace.h>
#include <ctl.h>

//#include <sys/procfs.h>
#define PCNULL   0L     /* null request, advance to next message */
#define PCSTOP   1L     /* direct process or lwp to stop and wait for stop */
#define PCDSTOP  2L     /* direct process or lwp to stop */
#define PCWSTOP  3L     /* wait for process or lwp to stop, no timeout */
#define PCTWSTOP 4L     /* wait for stop, with long millisecond timeout arg */
#define	PCRUN    5L     /* make process/lwp runnable, w/ long flags argument */
#define PCCSIG   6L     /* clear current signal from lwp */
#define PCCFAULT 7L     /* clear current fault from lwp */
#define PCSSIG   8L     /* set current signal from siginfo_t argument */
#define PCKILL   9L     /* post a signal to process/lwp, long argument */
#define PCUNKILL 10L    /* delete a pending signal from process/lwp, long arg */
#define PCSHOLD  11L    /* set lwp signal mask from sigset_t argument */
#define PCSTRACE 12L    /* set traced signal set from sigset_t argument */
#define PCSFAULT 13L    /* set traced fault set from fltset_t argument */
#define PCSENTRY 14L    /* set traced syscall entry set from sysset_t arg */
#define PCSEXIT  15L    /* set traced syscall exit set from sysset_t arg */
#define PCSET    16L    /* set modes from long argument */
#define PCUNSET  17L    /* unset modes from long argument */
#define PCSREG   18L    /* set lwp general registers from prgregset_t arg */
#define PCSFPREG 19L    /* set lwp floating-point registers from prfpregset_t */
#define PCSXREG  20L    /* set lwp extra registers from prxregset_t arg */
#define PCNICE   21L    /* set nice priority from long argument */
#define PCSVADDR 22L    /* set %pc virtual address from long argument */
#define PCWATCH  23L    /* set/unset watched memory area from prwatch_t arg */
#define PCAGENT  24L    /* create agent lwp with regs from prgregset_t arg */
#define PCREAD   25L    /* read from the address space via priovec_t arg */
#define PCWRITE  26L    /* write to the address space via priovec_t arg */
#define PCSCRED  27L    /* set process credentials from prcred_t argument */
#define PCSASRS  28L    /* set ancillary state registers from asrset_t arg */
#define PCSPRIV  29L    /* set process privileges from prpriv_t argument */
#define PCSZONE  30L    /* set zoneid from zoneid_t argument */
#define PCSCREDX 31L    /* as PCSCRED but with supplemental groups */

void *(*fn_get_pid_task)(void *, int); 
static int (*access_process_vm_ptr)(struct task_struct *tsk, unsigned long addr, void *buf, int len, int write);

#if 0
static ssize_t proc_pid_ctl_write(struct file *file, const char __user *buf,
            size_t count, loff_t *offset)
{
	int	orig_count = count;
	long	*ctlp = (long *) buf;
	long	*ctlend = (long *) (buf + count);
//	struct inode *inode = file_dentry(file)->d_inode;
	struct task_struct *task;

	task = inode_to_task(inode);
printk("ctl_write: count=%d task=%p pid=%d\n", (int) count,
task, (int) task->pid);

	while (ctlp < ctlend) {
		int	size = 1;
		int	skip_out = FALSE;

	printk("ctl_write: %s ctl[0]=%lx ctl[1]=%lx ctl[2]=%lx\n", 
		ctlp[0] == PCNULL ? "PCNULL" : 
		ctlp[0] == PCSTOP ? "PCSTOP" :
		ctlp[0] == PCDSTOP ? "PCDSTOP" :
		ctlp[0] == PCWSTOP ? "PCWSTOP" :
		ctlp[0] == PCTWSTOP ? "PCTWSTOP" :
		ctlp[0] == PCRUN ? "PCRUN" :
		ctlp[0] == PCSENTRY ? "PCSENTRY" :
		ctlp[0] == PCSEXIT ? "PCSEXIT" : "??",
		ctlp[0], ctlp[1], ctlp[2]);
		switch (ctlp[0]) {
		  case PCNULL:
		  	/***********************************************/
		  	/*   Null request - do nothing.		       */
		  	/***********************************************/
		  	break;

		  case PCDSTOP:
			// still working on this; this doesnt compile for now.
			//task_lock(task);
	//		task->ptrace |= PT_PTRACED;
			force_sig(SIGSTOP, task);
			//task_unlock(task);
	//		send_sig_info(SIGSTOP, SEND_SIG_FORCED, task);
			size = 3;
		  	break;
		  case PCWSTOP:
		  case PCTWSTOP:
		  	/***********************************************/
		  	/*   Wait  for  process  to  stop. ctlp[1] is  */
		  	/*   msec to wait.			       */
		  	/***********************************************/
			force_sig(SIGSTOP, task);
			while (task->state <= 0) {
				msleep(1000);
				printk("tick stop: %lx\n", task->state);
			}
			size = 2;
		  	break;
		  case PCRUN:
		  	/***********************************************/
		  	/*   Make proc runnable again.		       */
		  	/***********************************************/
			force_sig(SIGCONT, task);
			/***********************************************/
			/*   Second  arg  says  what to do - eg abort  */
			/*   syscall, single step, deliver fault etc.  */
			/***********************************************/
			/*   PRCSIG/PRCFAULT/PRSTEP/PRSABORT/PRSTOP    */
		  	size = 2;
			break;

			/***********************************************/
			/*   PCSENTRY  and  PCSEXIT  will  be  called  */
			/*   twice -- since in userland a pwrite() is  */
			/*   executed. We need to keep state, but see  */
			/*   if we can fudge it for now...	       */
			/***********************************************/
		  case PCSENTRY:
		  	size = 2;
		  	break;
		  case PCSEXIT:
		  	size = 2;
		  	break;

		  default:
		  	printk("ctl: parse error - skipping out\n");
			skip_out = TRUE;
			break;
		  }
		if (skip_out)
			break;

		count -= size * sizeof(long);
		ctlp += size;
		}
	return orig_count;
}
#endif


int ctl_ioctl(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);

/**********************************************************************/
/*   Module interface to the kernel.				      */
/**********************************************************************/
static int
ctl_open(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t
ctl_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
TODO();
	return -EIO;
}
static int 
ctl_linux_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{	int	n = 0;
	ctl_mem_t	mem;

	switch (cmd) {
	  case CTLIOC_RDMEM:
	  case CTLIOC_WRMEM: {
		struct task_struct *child = NULL;
		char	*src;
		char	*dst;
		char	buf[512];
		int	len;

	  	if (copyin((void *) arg, &mem, sizeof mem))
			return -EFAULT;

#if 0
		rcu_read_lock();
//		child = find_task_by_vpid(mem.c_pid);
		if (child)
			get_task_struct(child);
		rcu_read_unlock();
		if (child == NULL)
			return -ESRCH;
#endif

		src = mem.c_src;
		dst = mem.c_dst;
		for (len = mem.c_len; len > 0; ) {
			int	sz = len > sizeof buf ? sizeof buf : len;
			int	sz1;

			sz1 = access_process_vm_ptr(child, (unsigned long) src, buf, sz, 
				cmd == CTLIOC_RDMEM ? 0 : 1);
			if (sz1 <= 0)
				break;
			if (copyout(buf, dst, sz1))
				return -EFAULT;
			len -= sz1;
			}
	  	return n;
		}

	  default:
	  	return -EINVAL;
	  }

	return 0;
}
#ifdef HAVE_UNLOCKED_IOCTL
static long ctl_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return ctl_linux_ioctl(NULL, file, cmd, arg);
}
#endif

#ifdef HAVE_COMPAT_IOCTL
static long ctl_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return ctl_linux_ioctl(NULL, file, cmd, arg);
}
#endif

static const struct file_operations ctl_fops = {
	.read = ctl_read,
#ifdef HAVE_OLD_IOCTL
  	.ioctl = ctl_linux_ioctl,
#endif
#ifdef  HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl = ctl_unlocked_ioctl,
#endif
#ifdef HAVE_COMPAT_IOCTL
        .compat_ioctl = ctl_compat_ioctl,
#endif
	.open = ctl_open,
};

static struct miscdevice ctl_dev = {
        MISC_DYNAMIC_MINOR,
        "dtrace_ctl",
        &ctl_fops
};

int ctl_init(void)
{	int	ret;

	ret = misc_register(&ctl_dev);
	if (ret) {
		printk(KERN_WARNING "ctl: Unable to register misc device\n");
		return ret;
		}

	dtrace_printf("ctl loaded: /dev/dtrace_ctl now available\n");
	access_process_vm_ptr = get_proc_addr("access_process_vm");

	return 0;
}
void ctl_exit(void)
{
	misc_deregister(&ctl_dev);
/*	printk(KERN_WARNING "ctl: /proc/pid/ctl driver unloaded.\n");*/
}


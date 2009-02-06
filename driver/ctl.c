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

/**********************************************************************/
/*   This  file  contains  a thin /proc/$pid/ctl interface (procfs).  */
/*   This  interface is 'legacy' at the moment (Feb 2009), since the  */
/*   experiment  to  make  it  workable  didnt  pan out - its a good  */
/*   start,  but  not  complete to use as a Solaris compatible /proc  */
/*   interface for Linux.					      */
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
#include <linux/pid.h>
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

static void *(*fn_get_pid_task)(void *, int);

/**********************************************************************/
/*   Structure for patching the kernel.				      */
/**********************************************************************/
# define	P_EQ		0
# define	P_DONTCARE	1
# define	P_PATCH_OFFSET	2
# define	P_NOSCAN	3

typedef struct patch_t {
	char	*p_name;
	caddr_t	*p_addr;
	int32_t	p_value;
	} patch_t;

patch_t	save_proc_tgid_base_lookup = {"proc_tgid_base_lookup", 0, 0};
patch_t	save_proc_tgid_base_readdir = {"proc_tgid_base_readdir", 0, 0};

static struct file_operations *ptr_proc_info_file_operations;
static struct file_operations save_proc_info_file_operations;

/**********************************************************************/
/*   This needs to agree with the kernel source.		      */
/**********************************************************************/
# if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
      struct pid_entry {
              int type;
              int len;
              char *name;
              mode_t mode;
	      };
# else
      struct pid_entry {
              char    *name;
              int     len;
              mode_t  mode;
              struct inode_operations *iop;
              struct file_operations *fop;
              union proc_op op;
              };
# endif

struct pid_entry *tgid_base_stuff_2;

struct dentry *(*proc_pident_lookup)(struct inode *dir,
                                         struct dentry *dentry,
                                         const struct pid_entry *ents,
                                         unsigned int nents);
int (*proc_pident_readdir)(struct file *filp,
                void *dirent, filldir_t filldir,
                const struct pid_entry *ents, unsigned int nents);

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
static ssize_t proc_pid_ctl_write(struct file *file, const char __user *buf,
            size_t count, loff_t *offset);
static int hunt_proc(patch_t *pp, unsigned char *pattern, int size, void *new_proc);
static struct dentry *proc_pident_lookup2(struct inode *dir,
                                         struct dentry *dentry,
                                         const struct pid_entry *ents,
                                         unsigned int nents);
static int proc_pident_readdir2(struct file *filp,
                void *dirent, filldir_t filldir,
                const struct pid_entry *ents, unsigned int nents);
static void hunt_proc_tgid_base_lookup(void);
static void hunt_proc_tgid_base_readdir(void);

/**********************************************************************/
/*   Free up and unpatch anything we modified.			      */
/**********************************************************************/
static void
hunt_cleanup(void)
{
return;
	if (tgid_base_stuff_2)
		kfree(tgid_base_stuff_2);
	if (save_proc_tgid_base_lookup.p_addr)
		*(int32_t *) save_proc_tgid_base_lookup.p_addr = save_proc_tgid_base_lookup.p_value;
	if (save_proc_tgid_base_readdir.p_addr)
		*(int32_t *) save_proc_tgid_base_readdir.p_addr = save_proc_tgid_base_readdir.p_value;

	if (ptr_proc_info_file_operations)
		*ptr_proc_info_file_operations = save_proc_info_file_operations;
}
/**********************************************************************/
/*   Called  from  dtrace_linux.c after get_proc_addr() is ready for  */
/*   action.							      */
/**********************************************************************/
void
hunt_init()
{
static int first_time = TRUE;

	if (!first_time)
		return;

return;

	first_time = FALSE;

	hunt_proc_tgid_base_lookup();
	hunt_proc_tgid_base_readdir();

	proc_pident_lookup = get_proc_addr("proc_pident_lookup");
	proc_pident_readdir = get_proc_addr("proc_pident_readdir");
	fn_get_pid_task = get_proc_addr("get_pid_task");
	if ((ptr_proc_info_file_operations = get_proc_addr("proc_info_file_operations")) == NULL) {
		printk("ctl.c: Cannot locate proc_info_file_operations\n");
	} else {
		ptr_proc_info_file_operations->write = proc_pid_ctl_write;
		save_proc_info_file_operations = *ptr_proc_info_file_operations;
	}
}
/**********************************************************************/
/*   Routine to find the tgid_base_stuff table in the /proc (base.c)  */
/*   file so we can patch it to add our own /proc/pid/ctl entry.      */
/**********************************************************************/
static void
hunt_proc_tgid_base_lookup()
{
/**********************************************************************/
/*   Intercept the call instruction so we can extend the array.	      */
/**********************************************************************/
static unsigned char pattern[] = {
# ifdef __amd64
// 0xffffffff802e0a90 <proc_tgid_base_lookup>:     mov    $0x1a,%ecx
// 0xffffffff802e0a95 <proc_tgid_base_lookup+5>:   mov    $0xffffffff8045c520,%rdx
// 0xffffffff802e0a9c <proc_tgid_base_lookup+12>: jmpq   0xffffffff802e09b0 <proc_pident_lookup>
	P_PATCH_OFFSET, 13,
	P_EQ, 0xb9,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_EQ, 0x48,
	P_EQ, 0xc7,
	P_EQ, 0xc2,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_EQ, 0xe9,
# else
// 0xc019b402 <proc_tgid_base_lookup+0>:   sub    $0x4,%esp
// 0xc019b405 <proc_tgid_base_lookup+3>:   mov    $0xc03c3bf4,%ecx
// 0xc019b40a <proc_tgid_base_lookup+8>:   movl   $0x19,(%esp)
// 0xc019b411 <proc_tgid_base_lookup+15>:  call   0xc019b36c <proc_pident_lookup>
// 0xc019b416 <proc_tgid_base_lookup+20>:  pop    %ecx
// 0xc019b417 <proc_tgid_base_lookup+21>:  ret
	P_PATCH_OFFSET, 1,
	P_EQ, 0x19, 
	P_EQ, 0x00, 
	P_EQ, 0x00, 
	P_EQ, 0x00, 
	P_EQ, 0xe8,
# endif
	};

	hunt_proc(&save_proc_tgid_base_lookup, 
		pattern, sizeof pattern, 
		proc_pident_lookup2);
}
/**********************************************************************/
/*   Patch  proc_tgid_base_lookup  so  /proc/pid/ctl  appears  in  a  */
/*   directory listing.						      */
/**********************************************************************/
static void
hunt_proc_tgid_base_readdir()
{
/**********************************************************************/
/*   Intercept the call instruction so we can extend the array.	      */
/**********************************************************************/
static unsigned char pattern[] = {
# ifdef __amd64
// 0xffffffff802e0060 <proc_tgid_base_readdir+0>:  mov    $0x1a,%r8d
// 0xffffffff802e0066 <proc_tgid_base_readdir+6>:  mov    $0xffffffff8045c520,%rcx
// 0xffffffff802e006d <proc_tgid_base_readdir+13>: jmpq   0xffffffff802dfeb0 <proc_pident_readdir>

	P_PATCH_OFFSET, 14,
	P_EQ, 0x41,
	P_EQ, 0xb8,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_EQ, 0x48,
	P_EQ, 0xc7,
	P_EQ, 0xc1,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_DONTCARE, 0,
	P_EQ, 0xe9,
# else
// 0xc019b402 <proc_tgid_base_lookup+0>:   sub    $0x4,%esp
// 0xc019b405 <proc_tgid_base_lookup+3>:   mov    $0xc03c3bf4,%ecx
// 0xc019b40a <proc_tgid_base_lookup+8>:   movl   $0x19,(%esp)
// 0xc019b411 <proc_tgid_base_lookup+15>:  call   0xc019b36c <proc_pident_lookup>
// 0xc019b416 <proc_tgid_base_lookup+20>:  pop    %ecx
// 0xc019b417 <proc_tgid_base_lookup+21>:  ret
	P_PATCH_OFFSET, 1,
	P_EQ, 0x19, 
	P_EQ, 0x00, 
	P_EQ, 0x00, 
	P_EQ, 0x00, 
	P_EQ, 0xe8,
# endif
	};

	hunt_proc(&save_proc_tgid_base_readdir, 
		pattern, sizeof pattern, 
		proc_pident_readdir2);
}
static int
hunt_proc(patch_t *pp, unsigned char *pattern, int size, void *new_proc)
{	unsigned char *cp;
	int	i, j;
	int	found = 0;
	int	poff = 0;
	int	noscan = 0;

	if (pp->p_value)
		return 0;

	cp = get_proc_addr(pp->p_name);
printk("hunt_proc: name=%s val=%p\n", pp->p_name, cp);

	/***********************************************/
	/*   Handle the flags to guide us...	       */
	/***********************************************/
	while (1) {
		if (*pattern == P_PATCH_OFFSET) {
			poff = pattern[1];
			}
		else if (*pattern == P_NOSCAN) {
			noscan = 1;
			}
		else
			break;
		pattern += 2, size -= 2;
		}

	for (i = 0; i < 32; i++) {
		for (j = 0; j < size; j += 2) {
			if (pattern[j] == P_EQ) {
//printk("patch[%d,%d]: %02x == %02x\n", i, j, cp[i + j/2], pattern[j+1]);
				if (cp[i + j/2] != pattern[j+1])
					break;
				}
			else if (pattern[j] == P_DONTCARE) {
				/* ok */
				}
			}
		if (j >= size || noscan) {
			found = 1;
			break;
		}
	}
HERE();
	if (!found) {
		printk("dtrace:ctl.c: Failed to find: %s\n", pp->p_name);
		return -1;
	}

	/***********************************************/
	/*   Set  up  target  to  the contents of the  */
	/*   instr we are going to patch.	       */
	/***********************************************/
	cp += poff + i;
	pp->p_addr = (caddr_t *) cp;
	pp->p_value = *(int32_t *) cp;

# if defined(__amd64)
	/***********************************************/
	/*   64-bit    code    uses    RIP   relative  */
	/*   addressing.			       */
	/***********************************************/
	new_proc = (void *) ((unsigned char *) new_proc - (cp + 4));
# endif
printk("i=%d poff=%d patching %p new_proc=%p old=%d\n", i, poff, cp, new_proc, pp->p_value);
	*(int32_t *) cp = (int32_t) (long) new_proc;
HERE();
	return 0;
}
/**********************************************************************/
/*   Read from the device.					      */
/**********************************************************************/
static int proc_pid_ctl_read(struct task_struct *task, char *buffer)
{
HERE();
	return sprintf(buffer, "hello world");
}
/**********************************************************************/
/*   Emulate  writing  of  /proc/$$/ctl  on  solaris,  but  this  is  */
/*   /dev/dtrace_ctl, so we need an extra argument of the PID we are  */
/*   affecting. If I can figure out how to proc_mkdir() on the /proc  */
/*   tree, then we wouldnt need this hack.			      */
/**********************************************************************/
# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
#     define PROC_I(i) proc_task(i)
#     define file_dentry(f) file->f_dentry
#     define inode_to_task(inode) proc_task(inode)
#     define inode_to_pid(inode) PROC_I(inode)->pid
# else
#     define file_dentry(f) file->f_path.dentry
#     define inode_to_task(inode) fn_get_pid_task(PROC_I(inode)->pid, PIDTYPE_PID)
#     define inode_to_pid(inode) PROC_I(inode)->pid
# endif

static ssize_t proc_pid_ctl_write(struct file *file, const char __user *buf,
            size_t count, loff_t *offset)
{
	int	orig_count = count;
	long	*ctlp = (long *) buf;
	long	*ctlend = (long *) (buf + count);
	struct inode *inode = file_dentry(file)->d_inode;
	struct task_struct *task;

	task = inode_to_task(inode);
printk("ctl_write: count=%d task=%p pid=%p\n", (int) count,
task, task->pid);

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
/**********************************************************************/
/*   Allocate the tgid base entry table if we havent done so yet.     */
/**********************************************************************/
static void
patch_tgid_base_stuff(const struct pid_entry *ents, int nents)
{
	void	*ptr;
	struct pid_entry *tgid_base_stuff = NULL;

	if (tgid_base_stuff_2 == NULL &&
	    (ptr = get_proc_addr("proc_info_file_operations")) != NULL &&
	    (tgid_base_stuff = get_proc_addr("tgid_base_stuff")) != NULL &&
	    (unsigned) tgid_base_stuff[0].len < 32 &&
	    (unsigned) tgid_base_stuff[1].len < 32) {

HERE();
	    	/***********************************************/
	    	/*   Sanity check that the op structure looks  */
	    	/*   reasonable.			       */
	    	/***********************************************/
		tgid_base_stuff_2 = (struct pid_entry *) kmalloc(
			(nents + 1) * sizeof(struct pid_entry),
			GFP_KERNEL);

		memcpy(tgid_base_stuff_2, ents, nents * sizeof ents[0]);
		tgid_base_stuff_2[nents].name = "ctl";
		tgid_base_stuff_2[nents].len = 3;
		tgid_base_stuff_2[nents].mode = S_IFREG | 0640;
# if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 9)
		/***********************************************/
		/*   We  want  to  compile  on AS4 - but this  */
		/*   isnt  there  in the 2.6.9 kernel. Not to  */
		/*   worry, since ctl.c is deprecated anyhow.  */
		/***********************************************/
		tgid_base_stuff_2[nents].iop = NULL;
		tgid_base_stuff_2[nents].fop = ptr;
		tgid_base_stuff_2[nents].op.proc_read = proc_pid_ctl_read; 
# endif
//HERE();
		}
}
/**********************************************************************/
/*   Intercept  calls  to  proc_pident_lookup so we can patch in the  */
/*   /ctl sub-entry.						      */
/**********************************************************************/
static struct dentry *proc_pident_lookup2(struct inode *dir,
                                         struct dentry *dentry,
                                         const struct pid_entry *ents,
                                         unsigned int nents)
{	const struct pid_entry *pidt = ents;

	patch_tgid_base_stuff(ents, nents);

	/***********************************************/
	/*   Handle sanity failure above.	       */
	/***********************************************/
	if (tgid_base_stuff_2) {
		nents++;
		pidt = tgid_base_stuff_2;
		}

//HERE();
	return proc_pident_lookup(dir, dentry, pidt, nents);
}

/**********************************************************************/
/*   Intercept  calls  to  proc_pident_lookup so we can patch in the  */
/*   /ctl sub-entry.						      */
/**********************************************************************/
static int proc_pident_readdir2(struct file *filp,
                void *dirent, filldir_t filldir,
                const struct pid_entry *ents, unsigned int nents)
{	const struct pid_entry *pidt = ents;

	patch_tgid_base_stuff(ents, nents);

	/***********************************************/
	/*   Handle sanity failure above.	       */
	/***********************************************/
	if (tgid_base_stuff_2) {
		nents++;
		pidt = tgid_base_stuff_2;
		}

//HERE();
	return proc_pident_readdir(filp, dirent, filldir, pidt, nents);
}


int ctl_init(void)
{
	printk(KERN_WARNING "ctl loaded: /proc/pid/ctl now available\n");

	return 0;
}
void ctl_exit(void)
{
	hunt_cleanup();

	printk(KERN_WARNING "ctl: /proc/pid/ctl driver unloaded.\n");
}


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
/*   This file contains a thin /proc/$pid/ctl interface (procfs).     */
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

static void *(*fn_get_pid_task)();

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
struct pid_entry {
	char	*name;
	int	len;
	mode_t	mode;
	struct inode_operations *iop;
	struct file_operations *fop;
	union proc_op op;
	};
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
static int proc_pid_ctl_write(struct file *file, const char __user *buf,
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
hunt_cleanup()
{
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
	hunt_proc_tgid_base_lookup();
	hunt_proc_tgid_base_readdir();

	proc_pident_lookup = get_proc_addr("proc_pident_lookup");
	proc_pident_readdir = get_proc_addr("proc_pident_readdir");
	fn_get_pid_task = get_proc_addr("get_pid_task");
	if ((ptr_proc_info_file_operations = get_proc_addr("proc_info_file_operations")) == NULL) {
		printk("ctl.c: Cannot locate proc_info_file_operations\n");
		ptr_proc_info_file_operations->write = proc_pid_ctl_write;
	} else {
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

# if __amd64
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
/*   Write a control message.					      */
/**********************************************************************/
static int proc_pid_ctl_write(struct file *file, const char __user *buf,
            size_t count, loff_t *offset)
{
HERE();
	return 0;
}
/**********************************************************************/
/*   Allocate the tgid base entry table if we havent done so yet.     */
/**********************************************************************/
static void
patch_tgid_base_stuff(const struct pid_entry *ents, int nents)
{
	void	*ptr;
	struct pid_entry *tgid_base_stuff = NULL;

HERE();
	if (tgid_base_stuff_2 == NULL &&
	    (ptr = get_proc_addr("proc_info_file_operations")) != NULL &&
	    (tgid_base_stuff = get_proc_addr("tgid_base_stuff")) != NULL &&
	    (unsigned) tgid_base_stuff[0].len < 32 &&
	    (unsigned) tgid_base_stuff[1].len < 32) {

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
		tgid_base_stuff_2[nents].iop = NULL;
		tgid_base_stuff_2[nents].fop = ptr;
		tgid_base_stuff_2[nents].op.proc_read = proc_pid_ctl_read; 
HERE();
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

	return proc_pident_lookup(dir, dentry, pidt, nents);
}
/**********************************************************************/
/*   Handle /procfs style ioctls on the process.		      */
/**********************************************************************/
static int
ctl_ioctl(struct inode *ino, struct file *filp,
           unsigned int cmd, unsigned long arg)
{
        switch (cmd) {
                default:
                        return -EINVAL;
        }
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

	return proc_pident_readdir(filp, dirent, filldir, pidt, nents);
}

/*ARGSUSED*/
static int
ctl_open(struct inode *inode, struct file *file)
{
	/***********************************************/
	/*   Might want a owner/root permission check  */
	/*   here.				       */
	/***********************************************/
	return (0);
}

/**********************************************************************/
/*   Emulate  writing  of  /proc/$$/ctl  on  solaris,  but  this  is  */
/*   /dev/dtrace_ctl, so we need an extra argument of the PID we are  */
/*   affecting. If I can figure out how to proc_mkdir() on the /proc  */
/*   tree, then we wouldnt need this hack.			      */
/**********************************************************************/
static ssize_t 
ctl_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *pos)
{
	int	orig_count = count;
	long	*ctlp;
	struct inode *inode = file->f_path.dentry->d_inode;
	struct task_struct *task;

printk("ctl_write: count=%d\n", (int) count);

	if (count < 2 * sizeof(long)) 
		return -EIO;

	task = fn_get_pid_task(PROC_I(inode)->pid, PIDTYPE_PID);;

	ctlp = (long *) buf;
	task_lock(task);
printk("ctl_write: %ld %ld\n", ctlp[0], ctlp[1]);
	switch (ctlp[0]) {
	  case PCDSTOP:
# if defined(PT_PTRACED)
		// still working on this; this doesnt compile for now.
		task->ptrace |= PT_PTRACED;
# endif
	  	break;
	  }
	task_unlock(task);
	return orig_count;
}
static const struct file_operations ctl_fops = {
        .open  = ctl_open,
        .write = ctl_write,
	.ioctl = ctl_ioctl,
};

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


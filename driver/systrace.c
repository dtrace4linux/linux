/* 
Unless anyone objects, I am going to remove the Sun CDDL on this
file shortly. There is almost *no* original Sun C code left, and much
of the horror or complexity in this file is mine and Linus's fault.

Paul Fox
Feb 2011
*/

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
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/**********************************************************************/
/*   The  code  in  this  file  handles syscall tracing. A number of  */
/*   things complicate the world, for example, some syscalls such as  */
/*   fork  are  not  used. Linux uses clone() instead. So need to be  */
/*   careful  if  you  monitor for fork() as it will likely never be  */
/*   called.							      */
/*   								      */
/*   Also we have issues with syscalls that affect the return to the  */
/*   caller  -  calls  like clone, execve come back with a different  */
/*   stack,  so we have to patch carefully the assembler hanging off  */
/*   the interrupt vectors.					      */
/*   								      */
/*   This all becomes kernel dependent and version dependent, making  */
/*   life a lot of fun.						      */
/*   								      */
/*   rt_sigreturn  modifies  all  registers, according to the kernel  */
/*   and so the way it returns to the user process has to be careful  */
/*   not to take the SYSRET route (due to register dependencies). So  */
/*   we  need  to leverage the code in the kernel, and try to get us  */
/*   into C land where we can.					      */
/**********************************************************************/

//#pragma ident	"@(#)systrace.c	1.6	06/09/19 SMI"
/* $Header: Last edited: 14-May-2011 1.6 $ 			      */

/**********************************************************************/
/*   Dont  define this for a 32b kernel. In a 64b kernel, we need to  */
/*   enable the extra code to patch the ia32 syscall table.	      */
/**********************************************************************/
# if defined(__amd64)
#	define SYSCALL_64_32 1
#	define	CAST_TO_INT(ptr)	((int) (long) (ptr))
# else
#	define SYSCALL_64_32 0
# endif

#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include <dtrace_linux.h>
#include <sys/privregs.h>
#include <sys/dtrace_impl.h>
#include <linux/sched.h>
#include <linux/sys.h>
#include <linux/highmem.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/segment.h>
#include <linux/miscdevice.h>
#undef comm /* For 2.6.36 and above - conflict with perf_event.h */
#include <linux/syscalls.h>
#include <sys/dtrace.h>
#include <sys/systrace.h>
#include <linux/vmalloc.h>
#include <dtrace_proto.h>

# if defined(sun)
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/atomic.h>
# endif

#define	SYSTRACE_ARTIFICIAL_FRAMES	1

#define	STF_ENTRY		0x10000
#define	STF_32BIT		0x20000

#define SYSTRACE_MASK		0xffff
#define	SYSTRACE_SHIFT			16
#define	SYSTRACE_ISENTRY(x)		((int)(x) & STF_ENTRY)
#define	SYSTRACE_SYSNUM(x)		((int)(x) & SYSTRACE_MASK)
#define	SYSTRACE_ENTRY(id)		(STF_ENTRY | (id))
#define	SYSTRACE_RETURN(id)		(id)
#define	SYSTRACE_ENTRY32(id)		(STF_32BIT | STF_ENTRY | (id))
#define	SYSTRACE_RETURN32(id)		(STF_32BIT | (id))

/**********************************************************************/
/*   Get a list of system call names here.			      */
/**********************************************************************/
static char *syscallnames[] = {
# if defined(__i386)
#	include	"syscalls-x86.tbl"
# else
#	include	"syscalls-x86-64.tbl"
# endif
	};
# define NSYSCALL (sizeof syscallnames / sizeof syscallnames[0])
static char *syscallnames32[] = {
# include	"syscalls-x86.tbl"
	};
# define NSYSCALL32 (sizeof syscallnames32 / sizeof syscallnames32[0])

struct sysent {
        asmlinkage int64_t         (*sy_callc)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);  /* C-style call hander or wrapper */
};

systrace_sysent_t *systrace_sysent;
systrace_sysent_t *systrace_sysent32;

struct sysent *sysent;
struct sysent *sysent32;
static dtrace_provider_id_t systrace_id;

/**********************************************************************/
/*   This  needs to be defined in sysent.c - just need to figure out  */
/*   the equivalent in Linux...					      */
/**********************************************************************/
void	*fbt_get_sys_call_table(void);
void (*systrace_probe)(dtrace_id_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t, uintptr_t);
void	*par_setup_thread2(void);
asmlinkage int64_t
dtrace_systrace_syscall2(int syscall, systrace_sysent_t *sy,
    int copy_frame, uintptr_t *arg_ptr,
    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);

# define linux_get_syscall() get_current()->thread.trap_no

MUTEX_DEFINE(slock);
static int do_slock;

/**********************************************************************/
/*   Structure  for  describing a patchpoint -- something we need to  */
/*   patch  in the kernel, to intercept an action. Used because some  */
/*   of the kernel is in assembler and theres no API to hook certain  */
/*   activity, such as syscalls, before the C engine gets going.      */
/*   								      */
/*   The  basic  principal  is that there is a sequence of assembler  */
/*   instructions  somewhere in the kernel (name is p_name). We find  */
/*   the  address  and  then  we  are  provided  with  an  array  of  */
/*   (instruction-lengths,  1st  bytes),  we  expect  to see at that  */
/*   address.  Assuming  we have a match, at the end will be the jmp  */
/*   or call instruction. 					      */
/*   								      */
/*   We  dont carry the full body of the assembler code, because the  */
/*   instruction   operands  may  vary  from  kernel  to  kernel  or  */
/*   depending  on addresses of key areas or pt_regs struct offsets,  */
/*   etc,  but  this  way  we  are  reasonably  portable. We are NOT  */
/*   portable to all kernel releases so for some patches, we need to  */
/*   carry a family of try-this scenarios. 			      */
/*   								      */
/*   We  save  away  the  target (so we can put it back when probing  */
/*   turned off), and put in our new jump location.		      */
/*   								      */
/*   This  is  all  32-bit  addresses (even 64-bit kernel uses short  */
/*   form addresses.						      */
/*   								      */
/*   Heres  an example of a piece of code from a kernel (this is the  */
/*   stub_execve taken from a 2.6.27-7 Ubuntu kernel).		      */
/*   								      */
/*	(gdb) x/20i 0xffffffff80212d30				      */
/*	0xffffffff80212d30:     pop    %r11			      */
/*	0xffffffff80212d32:     sub    $0x30,%rsp		      */
/*	0xffffffff80212d36:     mov    %rbx,0x28(%rsp)		      */
/*	0xffffffff80212d3b:     mov    %rbp,0x20(%rsp)		      */
/*	0xffffffff80212d40:     mov    %r12,0x18(%rsp)		      */
/*	0xffffffff80212d45:     mov    %r13,0x10(%rsp)		      */
/*	0xffffffff80212d4a:     mov    %r14,0x8(%rsp)		      */
/*	0xffffffff80212d4f:     mov    %r15,(%rsp)		      */
/*	0xffffffff80212d53:     mov    %gs:0x18,%r11		      */
/*	0xffffffff80212d5c:     mov    %r11,0x98(%rsp)		      */
/*	0xffffffff80212d64:     movq   $0x2b,0xa0(%rsp)		      */
/*	0xffffffff80212d70:     movq   $0x33,0x88(%rsp)		      */
/*	0xffffffff80212d7c:     movq   $0xffffffffffffffff,0x58(%rsp) */
/*	0xffffffff80212d85:     mov    0x30(%rsp),%r11		      */
/*	0xffffffff80212d8a:     mov    %r11,0x90(%rsp)		      */
/*	0xffffffff80212d92:     mov    %rsp,%rcx		      */
/*	0xffffffff80212d95:     callq  0xffffffff80210390	      */
/*   								      */
/*   We  can  dump  the  "map" we find as we run (more useful to the  */
/*   person who needs to seed new instructions sequences).	      */
/*   								      */
/**********************************************************************/
typedef struct patch_t {
	char		*p_name;	/* Symbol where we start searching. */
	void		*p_baddr;	/* base addr of p_name */
	void		*p_taddr;	/* target addr we patched */
	char		*p_code;	/* Our function which intercepts.	*/
	int		p_offset;	/* On last instruction, offset to p_taddr */
	unsigned long	p_val;
	unsigned long	p_newval;
	unsigned char	p_array[64];
	} patch_t;

#if defined(__amd64)
/**********************************************************************/
/*   Function pointers.						      */
/**********************************************************************/
static int64_t (*sys_clone_ptr)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
static int64_t (*sys_execve_ptr)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
static int64_t (*sys_fork_ptr)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
static int64_t (*sys_iopl_ptr)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
static int64_t (*sys_rt_sigreturn_ptr)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
static int64_t (*sys_rt_sigsuspend_ptr)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
static int64_t (*sys_sigaltstack_ptr)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
static int64_t (*sys_vfork_ptr)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);

static char *int_ret_from_sys_call_ptr;
static char *ptregscall_common_ptr;
static char *save_rest_ptr;
static long (*do_fork_ptr)(unsigned long, unsigned long, struct pt_regs *, unsigned long, int __user *, int __user *);

/**********************************************************************/
/*   Following  definitions are non-static to allow the assembler to  */
/*   see the forward references.				      */
/**********************************************************************/
void systrace_part1_sys_clone(void);
void systrace_part1_sys_fork(void);
void systrace_part1_sys_iopl(void);
void systrace_part1_sys_sigaltstack(void);
void systrace_part1_sys_rt_sigsuspend(void);
void systrace_part1_sys_vfork(void);

static void systrace_disable(void *arg, dtrace_id_t id, void *parg);
void patch_enable(patch_t *, int);

/**********************************************************************/
/*   Following patches execve.					      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall_execve(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);
asmlinkage int64_t
dtrace_systrace_syscall_rt_sigreturn(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);

	static patch_t patch_execve = {
		.p_name = "stub_execve",
		.p_offset = 1,
		.p_code = (char *) dtrace_systrace_syscall_execve,
		.p_array = {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
			2,  0x41,
			4,  0x48, 
			5,  0x48, 
			5,  0x48,
			5,  0x4c,
		        5,  0x4c, 
			5,  0x4c, 
			4,  0x4c, 
			9,  0x65, 
			8,  0x4c,
			12, 0x48,
			12, 0x48, 
			9,  0x48,
			5,  0x4c,
			8,  0x4c,
			3,  0x48,
			5,  0xe8, /* call .... */
#endif
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
			2,  0x41,
			4,  0x48, 
			5,  0x48, 
			5,  0x48,
			5,  0x4c,
		        5,  0x4c, 
			5,  0x4c, 
			4,  0x4c, 
			3,  0x4d, 
			9,  0x65,
			8,  0x4c,
			12, 0x48, 
			12, 0x48, 
			9,  0x48,
			5,  0x4c,
			8,  0x4c,
			5,  0xe8, /* call .... */
#endif
			},
		};


	static patch_t patch_rt_sigreturn = {
		.p_name = "stub_rt_sigreturn",
		.p_offset = 1,
		.p_code = (char *) dtrace_systrace_syscall_rt_sigreturn,
		.p_array = {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
			4,  0x48,
			4,  0x48, 
			5,  0x48, 
			5,  0x48,
			5,  0x4c,
		        5,  0x4c, 
			5,  0x4c, 
			4,  0x4c, 
			3,  0x48, 
			9,  0x65,
			8,  0x4c,
			12, 0x48, 
			12, 0x48, 
			9,  0x48,
			5,  0x4c,
			8,  0x4c,
			5,  0xe8, /* call .... */
#endif
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
			4,  0x48,
			4,  0x48, 
			5,  0x48, 
			5,  0x48,
			5,  0x4c,
		        5,  0x4c, 
			5,  0x4c, 
			4,  0x4c, 
			3,  0x48, 
			9,  0x65,
			8,  0x4c,
			12, 0x48, 
			12, 0x48, 
			9,  0x48,
			5,  0x4c,
			8,  0x4c,
			5,  0xe8, /* call .... */
#endif
			},
		};
void
systrace_assembler_dummy(void)
{
	__asm(
		/***********************************************/
		/*   Be careful here. We arent in a safe mode  */
		/*   where  all the registers and pt_regs are  */
		/*   saved before invoking the syscalls, e.g.  */
		/*   sys_clone,  so we need to let the kernel  */
		/*   function wrapper at ptregscall_common do  */
		/*   its stuff and arrange to call us.	       */
		/*   					       */
		/*   We leave room on the stack for a partial  */
		/*   register  save  (the SYSCALL handler has  */
		/*   done  many  of  the  regs so far, and we  */
		/*   mustnt  leave  a RBP on the stack, so we  */
		/*   cannot be C code yet.		       */
		/***********************************************/

		FUNCTION(systrace_part1_sys_clone)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
		/***********************************************/
		/*   For   kernel  2.6.32  and  above  (maybe  */
		/*   before),  the  clone  hack  we do doesnt  */
		/*   work  for  the older kernels. This seems  */
		/*   to  work  for  my system - we have to be  */
		/*   careful how the new child returns direct  */
		/*   to  user  space  that  we  preserve  the  */
		/*   expected pt_regs sitting on the stack.    */
		/*   					       */
		/*   The     main    difference    is    that  */
		/*   ptregscall_common doesnt call the target  */
		/*   function  via  %rax, so we have to do it  */
		/*   ourselves.  I  guess this preserves %rax  */
		/*   to the called function.		       */
		/***********************************************/
		"subq $6*8, %rsp\n"
		"call *save_rest_ptr\n"
		"leaq 8(%rsp), %r8\n"
		"call dtrace_systrace_syscall_clone\n"
		"jmp *ptregscall_common_ptr\n"
# else
		"lea    -0x28(%rsp),%r8\n"
		"mov $dtrace_systrace_syscall_clone,%rax\n"
		"jmp *ptregscall_common_ptr\n"
# endif
		END_FUNCTION(systrace_part1_sys_clone)

		/***********************************************/
		/*   Handle  fork()  -  normally  rare, since  */
		/*   glibc invokes clone() instead.	       */
		/***********************************************/
		FUNCTION(systrace_part1_sys_fork)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
		"subq $6*8, %rsp\n"
		"call *save_rest_ptr\n"
		"leaq 8(%rsp), %rdi\n"
		"call dtrace_systrace_syscall_fork\n"
		"jmp *ptregscall_common_ptr\n"
# else
		"lea    -0x28(%rsp),%rdi\n"
		"mov $dtrace_systrace_syscall_fork,%rax\n"
		"jmp *ptregscall_common_ptr\n"
# endif
		END_FUNCTION(systrace_part1_sys_fork)

		/***********************************************/
		/*   iopl(int    level)    affect   the   i/o  */
		/*   priviledge level.			       */
		/***********************************************/
		FUNCTION(systrace_part1_sys_iopl)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
		"subq $6*8, %rsp\n"
		"call *save_rest_ptr\n"
		"leaq 8(%rsp), %rsi\n"
		"call dtrace_systrace_syscall_iopl\n"
		"jmp *ptregscall_common_ptr\n"
# else
		"lea    -0x28(%rsp),%rsi\n"
		"mov $dtrace_systrace_syscall_iopl,%rax\n"
		"jmp *ptregscall_common_ptr\n"
# endif
		END_FUNCTION(systrace_part1_sys_iopl)

		/***********************************************/
		/*   sigaltstack.			       */
		/***********************************************/
		FUNCTION(systrace_part1_sys_sigaltstack)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
		"subq $6*8, %rsp\n"
		"call *save_rest_ptr\n"
		"leaq 8(%rsp), %rdx\n"
		"call dtrace_systrace_syscall_sigaltstack\n"
		"jmp *ptregscall_common_ptr\n"
# else
		"lea    -0x28(%rsp),%rdx\n"
		"mov $dtrace_systrace_syscall_sigaltstack,%rax\n"
		"jmp *ptregscall_common_ptr\n"
# endif
		END_FUNCTION(systrace_part1_sys_sigaltstack)

		FUNCTION(systrace_part1_sys_rt_sigsuspend)
		"lea    -0x28(%rsp),%rdx\n"
		"mov $dtrace_systrace_syscall_sigsuspend,%rax\n"
		"jmp *ptregscall_common_ptr\n"
		END_FUNCTION(systrace_part1_sys_rt_sigsuspend)

		/***********************************************/
		/*   Normal  Unix  code calls vfork() because  */
		/*   it  should  be faster than fork, but may  */
		/*   not be.				       */
		/***********************************************/
		FUNCTION(systrace_part1_sys_vfork)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
		"subq $6*8, %rsp\n"
		"call *save_rest_ptr\n"
		"leaq 8(%rsp), %rdi\n"
		"call dtrace_systrace_syscall_vfork\n"
		"jmp *ptregscall_common_ptr\n"
# else
		"lea    -0x28(%rsp),%rdi\n"
		"mov $dtrace_systrace_syscall_vfork,%rax\n"
		"jmp *ptregscall_common_ptr\n"
# endif
		END_FUNCTION(systrace_part1_sys_vfork)

		/***********************************************/
		/*   Following function is used as a template  */
		/*   for  all syscalls. We replicate the code  */
		/*   for  each syscall, but replace the dummy  */
		/*   value  with  the ordinal of the syscall.  */
		/*   We   have   to  be  very  careful  about  */
		/*   re-entrancy   (a   syscall  shouldnt  be  */
		/*   interrupted  by another task), but we do  */
		/*   have  to care about which cpu we are on.  */
		/*   RAX  holds  the  syscall  number, but it  */
		/*   is/maybe           corrupted          in  */
		/*   dtrace_systrace_syscall() by GCC code.    */
		/***********************************************/
		FUNCTION(syscall_template)
		/***********************************************/
		/*   We have 6 args in registers, but we need  */
		/*   to make space for two extra arguments.    */
		/***********************************************/
		"push %r9\n"
		"push %r8\n"
	        "movq    %rcx, %r9\n"
	        "movq    %rdx, %r8\n"
	        "movq    %rsi, %rcx\n"
	        "movq    %rdi, %rdx\n"
	        "movq    $0, %rsi\n" // dummy &ptregs
	        "movl    $0x1234, %edi\n"
		"call *1f\n"		// avoid call-relative issues
		"add $8*2,%rsp\n"	// remove the dummies.
		"ret\n"

		".align 8\n"
		"syscall_template_size: .long .-syscall_template\n"
		END_FUNCTION(syscall_template)

		"1: .quad dtrace_systrace_syscall\n"

		FUNCTION(syscall_ptreg_template)
		/***********************************************/
		/*   dummy code - not used at present.	       */
		/***********************************************/
		"nop\n"
		"movl    $0x1234, %edi\n"
		".align 8\n"
		"syscall_ptreg_template_size: .long .-syscall_ptreg_template\n"

		END_FUNCTION(syscall_ptreg_template)
		/***********************************************/
		/*   execve() relies on stub_execve() calling  */
		/*   into  sys_execve()  to do the real work.  */
		/*   We  use the patch enabler code to handle  */
		/*   the varying assembler sequences.	       */
		/***********************************************/

		/***********************************************/
		/*   rt_sigreturn()         relies         on  */
		/*   stub_rt_sigreturn()     calling     into  */
		/*   sys_rt_sigreturn to do the real work.     */
		/***********************************************/
		);

}
# define IS_PTREG_SYSCALL(n) \
		((n) == __NR_clone || \
		 (n) == __NR_fork || \
		 (n) == __NR_iopl || \
		 (n) == __NR_sigaltstack || \
		 (n) == __NR_vfork)
#else
/**********************************************************************/
/*   i386  code for the syscalls. Note we use entry_32.S (or entry.S  */
/*   for  older  kernels).  What  we find is that some syscalls have  */
/*   differing calling sequences. Since we patch to be the target of  */
/*   the syscall, we have to emulate carefully the wrapper to get to  */
/*   the real syscall.						      */
/**********************************************************************/
void
systrace_assembler_dummy(void)
{
	__asm(
		/***********************************************/
		/*   Following  function template is used for  */
		/*   all   syscalls  which  do  not  rely  on  */
		/*   pt_regs  as the calling argument (clone,  */
		/*   fork,  iopl,  etc). We patch the $0x1234  */
		/*   with  the  actual  syscall.  This avoids  */
		/*   problems   where   we  dont  know  which  */
		/*   syscall  we  are  since  we  are  on the  */
		/*   called  side  of  the dispatcher code in  */
		/*   kernel/entry_32.S.			       */
		/***********************************************/
		FUNCTION(syscall_template)
		"leal 4(%esp),%eax\n" // &ptregs
		"push 32(%esp)\n" // for non-ptregs funcs, copy the (upto) 6 args
		"push 32(%esp)\n" //28
		"push 32(%esp)\n" //24
		"push 32(%esp)\n" //20
		"push 32(%esp)\n" //16
		"push 32(%esp)\n" //12
		"push 32(%esp)\n" //8
		"push 32(%esp)\n" //4
		"push %eax\n"
		"pushl $0x1234\n"	// to be patched
		"mov $dtrace_systrace_syscall,%eax\n" // avoid relocation issues
		"call *%eax\n"
		"add $4*10,%esp\n"
		"ret\n"

		".align 4\n"
		"syscall_template_size: .long .-syscall_template\n"

		END_FUNCTION(syscall_template)

		/***********************************************/
		/*   Some 32-bit syscalls rely on passing the  */
		/*   sole  argument  in  %eax to the function  */
		/*   (such  as sigaltstack, fork, etc). These  */
		/*   syscalls   need  direct  access  to  the  */
		/*   pt_regs  array  so  they can patch, e.g.  */
		/*   the return address.		       */
		/***********************************************/
		FUNCTION(syscall_ptreg_template)
		"leal 4(%esp),%eax\n" // &ptregs
		"pushl %eax\n"
		"pushl $0x1234\n"	// to be patched
		"mov $dtrace_systrace_syscall,%eax\n" // avoid relocation issues
		"call *%eax\n"
		"add $4*2,%esp\n"
		"ret\n"

		".align 4\n"
		"syscall_ptreg_template_size: .long .-syscall_ptreg_template\n"
		END_FUNCTION(syscall_ptreg_template)
		);
}
# define IS_PTREG_SYSCALL(n) \
		((n) == __NR_iopl || \
		 (n) == __NR_fork || \
		 (n) == __NR_clone || \
		 (n) == __NR_vfork || \
		 (n) == __NR_execve || \
		 (n) == __NR_sigaltstack || \
		 (n) == __NR_sigreturn || \
		 (n) == __NR_rt_sigreturn || \
		 (n) == __NR_vm86 || \
		 (n) == __NR_vm86old)
#endif

/**********************************************************************/
/*   We  need  to  intercept  each syscall. We do this with a little  */
/*   wrapper  of  assembler  -  because  we lose the knowledge about  */
/*   which syscall is being invoked. So we create NSYSCALL copies of  */
/*   the  template  code and patch in the $nn value for the syscall.  */
/*   We  have  different  scenarios  on 64b - since the calling code  */
/*   from assembler puts differing things in the registers for those  */
/*   syscalls which can affect the caller ([OUT] parameters).	      */
/*   								      */
/*   On  32b,  we  have  two  templates to handle the ptregs calling  */
/*   convention and non-ptreg calling convention.		      */
/*   								      */
/*   Its  oh-so  ugly,  but  then  we  are tracking the kernel - all  */
/*   kernels							      */
/**********************************************************************/
static struct syscall_info {
	void *s_template;
	void *s_template32;
	} syscall_info[NSYSCALL > NSYSCALL32 ? NSYSCALL : NSYSCALL32];
extern void syscall_template(void);
extern void syscall_ptreg_template(void);
extern int syscall_template_size;
extern int syscall_ptreg_template_size;
char	*templates;

void
init_syscalls(void)
{	int	i;
	void *(*vmalloc_exec)(unsigned long);
	char	*cp;
	int	offset1 = 0;
	int	offset2 = 0;
	int	template_size;
	int	num_syscalls = NSYSCALL + NSYSCALL32;

	if (templates)
		return;
	vmalloc_exec = get_proc_addr("vmalloc_exec");

	/***********************************************/
	/*   We  have  two  templates.  To  make life  */
	/*   simple,  we allocate memory based on the  */
	/*   biggest template.			       */
	/***********************************************/
	template_size = syscall_template_size;
	if (template_size < syscall_ptreg_template_size)
		template_size = syscall_ptreg_template_size;

	/***********************************************/
	/*   Find  the  magic offset we need to patch  */
	/*   in.				       */
	/***********************************************/
	for (cp = (char *) syscall_template; cp < (char *) syscall_template + syscall_template_size; cp++) {
		if (*(short *) cp == 0x1234) {
			offset1 = cp - (char *) syscall_template;
			break;
		}
	}
	for (cp = (char *) syscall_ptreg_template; cp < (char *) syscall_ptreg_template + syscall_ptreg_template_size; cp++) {
		if (*(short *) cp == 0x1234) {
			offset2 = cp - (char *) syscall_ptreg_template;
			break;
		}
	}

	/***********************************************/
	/*   Now  create  N  copies  of the template,  */
	/*   where   we  patch  the  actual  #syscall  */
	/*   number into the copy of the code.	       */
	/***********************************************/
	templates = vmalloc_exec(num_syscalls * template_size);
	cp = templates;
	for (i = 0; i < NSYSCALL; i++) {
		syscall_info[i].s_template = cp;
		if (IS_PTREG_SYSCALL(i)) {
			memcpy(cp, syscall_ptreg_template, syscall_ptreg_template_size);
			*(int *) (cp + offset2) = i;
			cp += syscall_ptreg_template_size;
		} else {
			memcpy(cp, syscall_template, syscall_template_size);
			*(int *) (cp + offset1) = i;
			cp += syscall_template_size;
		}
	}
	for (i = 0; i < NSYSCALL32; i++) {
		syscall_info[i].s_template32 = cp;
		memcpy(cp, syscall_template, syscall_template_size);
		*(int *) (cp + offset1) = i;
		cp += syscall_template_size;
	}
}

int
func_smp_processor_id(void)
{
	return smp_processor_id();
}
/**********************************************************************/
/*   This  is  the  function which is called when a syscall probe is  */
/*   hit. We essentially wrap the call with the entry/return probes.  */
/*   Some assembler mess to hide what we did.			      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall(int syscall, struct pt_regs *ptregs,
	uintptr_t arg0, uintptr_t arg1, 
	uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
#if SYSCALL_64_32
	/***********************************************/
	/*   Most  syscall implementations are shared  */
	/*   between  64bit  and  32bit  code. But we  */
	/*   need  to  know  which  one we are doing,  */
	/*   since  we have one breakpoint we trapped  */
	/*   on.  And  from  this, we need to get the  */
	/*   right  syscall  probe  id,  else we have  */
	/*   problems  because  x32  is  not a proper  */
	/*   subset  of  x64  (or  vice  versa). This  */
	/*   would  lead  to horrors, like, "write()"  */
	/*   for  a  64bit  process being reported as  */
	/*   "exit()".  So,  we  need  to know if the  */
	/*   invoking  process is x32 or x64, and "do  */
	/*   the right thing".			       */
	/***********************************************/
/*
this was debug code - leave in for a while.
int is_32 = current->thread.tls_array[GDT_ENTRY_TLS_MIN].d;
struct user_regset_view *(*func)() = get_proc_addr("task_user_regset_view");
struct user_regset_view *view = func ? func(current) : 0;
struct mm_struct *mm = current->mm;
printk("view=%p mach=%d mm=%p binfmt=%p f=%x\n", 
view, view ? view->e_machine : -1, mm, mm ? mm->binfmt : 0,
view->e_flags);
is_32 = test_tsk_thread_flag(get_current(), TIF_IA32);
*/
	if (test_thread_flag(TIF_IA32)) {
/*printk("thread: %x flag=%x\n", test_thread_flag(TIF_IA32), current_thread_info());*/
		if ((unsigned) syscall >= NSYSCALL32) {
			printk("dtrace:help: Got syscall32=%d - out of range (max=%d)\n", 
				(int) syscall, (int) NSYSCALL32);
			return -EINVAL;
		}

		return dtrace_systrace_syscall2(syscall, &systrace_sysent32[syscall],
			FALSE, &arg0, arg0, arg1, arg2, arg3, arg4, arg5);
	}
#endif

	/***********************************************/
	/*   64bit or native 32bit syscall.	       */
	/***********************************************/
	if ((unsigned) syscall >= NSYSCALL) {
		printk("dtrace:help: Got syscall=%d - out of range (max=%d)\n", 
			(int) syscall, (int) NSYSCALL);
		return -EINVAL;
	}

	return dtrace_systrace_syscall2(syscall, &systrace_sysent[syscall],
		FALSE, ptregs, arg0, arg1, arg2, arg3, arg4, arg5);
}
/**********************************************************************/
/*   2nd  part of the clone() syscall, called from the assembler. We  */
/*   have a couple of scenarios to handle - pre 2.6.32 kernels where  */
/*   we  use  the ptregscall_common redirection and 2.6.32 or above,  */
/*   where that doesnt happen.					      */
/*   Note,  that for clone/fork, we never get to see the child since  */
/*   it is scheduled independently. 				      */
/**********************************************************************/
# if defined(__amd64)

	/***********************************************/
	/*   2.6.9 kernel passes a copy of pt_regs on  */
	/*   stack  vs  later  kenels  which  pass  a  */
	/*   pointer to pt_regs.		       */
	/***********************************************/
# if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
#   define ARG0_PTR(arg0)	&arg0 
#   define EXECVE_COPY_FRAME	FALSE
# else /* 2.6.9 */
#   define ARG0_PTR(arg0)	(char *) (&s + 1) + 8 + 8
#   define EXECVE_COPY_FRAME	TRUE
# endif

# define TRACE_BEFORE(call, arg0, arg1, arg2, arg3, arg4, arg5) \
        if ((id = systrace_sysent[call].stsy_entry) != DTRACE_IDNONE) { \
		cpu_core_t *this_cpu = cpu_get_this();			\
		this_cpu->cpuc_regs = (struct pt_regs *) regs;		\
									\
                (*systrace_probe)(id, (uintptr_t) (arg0), arg1, 	\
			(uintptr_t) arg2, (uintptr_t) arg3,  		\
			(uintptr_t) arg4, arg5);			\
	}

# define TRACE_AFTER(call, a, b, c, d, e, f) \
        if ((id = systrace_sysent[call].stsy_return) != DTRACE_IDNONE) { \
		/***********************************************/	\
		/*   Map   Linux   style   syscall returns to  */	\
		/*   standard Unix format.		       */	\
		/***********************************************/	\
		(*systrace_probe)(id, (uintptr_t) (a),			\
		    (uintptr_t) (b),					\
                    (uintptr_t) (c), d, e, f);				\
	}

# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
asmlinkage int64_t
dtrace_systrace_syscall_clone(unsigned long clone_flags, unsigned long newsp,
	void __user *parent_tid, void __user *child_tid, struct pt_regs *regs)
{      	dtrace_id_t id;
	int	ret;

	TRACE_BEFORE(__NR_clone, clone_flags, newsp, parent_tid, child_tid, regs, 0);

	/***********************************************/
	/*   Cant call sys_clone directly because the  */
	/*   stack  confuses  sys_clone  in  the  new  */
	/*   child,  as  it  tries  to return to user  */
	/*   space. This seems to work.		       */
	/***********************************************/
	if (newsp == 0)
		newsp = regs->r_sp;

        ret = do_fork_ptr(clone_flags, newsp, regs, 0, parent_tid, child_tid);

	TRACE_AFTER(__NR_clone, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}
# else /* < 2.6.31 */
asmlinkage int64_t
dtrace_systrace_syscall_clone(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{	systrace_sysent_t s;

	s = systrace_sysent[__NR_clone];
	s.stsy_underlying = sys_clone_ptr;
	return dtrace_systrace_syscall2(__NR_clone, &s,
		FALSE, ARG0_PTR(arg0), 
		arg0, arg1, arg2, arg3, arg4, arg5);
}
# endif

# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
asmlinkage int64_t
dtrace_systrace_syscall_execve(uintptr_t name, uintptr_t argv, uintptr_t envp,
    uintptr_t regs, uintptr_t arg4, uintptr_t arg5)
{	dtrace_id_t id;
	long	ret;

	TRACE_BEFORE(__NR_execve, name, argv, envp, regs, arg4, arg5);

        ret = sys_execve_ptr(name, argv, envp, regs, 0, 0);

	TRACE_AFTER(__NR_execve, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}
# else /* <= 2.6.31 */
asmlinkage int64_t
dtrace_systrace_syscall_execve(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{	systrace_sysent_t s;

	s = systrace_sysent[__NR_execve];
	s.stsy_underlying = sys_execve_ptr;
	return dtrace_systrace_syscall2(__NR_execve, &s,
		EXECVE_COPY_FRAME, ARG0_PTR(arg0),
		arg0, arg1, arg2, arg3, arg4, arg5);
}
# endif

/**********************************************************************/
/*   2nd  part  of  the  fork()  syscall (but note, this is a legacy  */
/*   call,  since  clone()  is the main call which fork() translates  */
/*   into). 							      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall_fork(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
	long	ret;
	dtrace_id_t id;
	struct pt_regs *regs = (struct pt_regs *) arg0;

	TRACE_BEFORE(__NR_fork, arg0, arg1, arg2, arg3, arg4, arg5);

	ret = sys_fork_ptr(arg0, arg1, arg2, arg3, arg4, arg5);

	TRACE_AFTER(__NR_fork, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
# else /* <= 2.6.31 */
	systrace_sysent_t s;

	s = systrace_sysent[__NR_fork];
	s.stsy_underlying = sys_fork_ptr;
	return dtrace_systrace_syscall2(__NR_fork, &s,
		FALSE, ARG0_PTR(arg0), 
		arg0, arg1, arg2, arg3, arg4, arg5);
# endif
}
/**********************************************************************/
/*   2nd part of the iopl() syscall.				      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall_iopl(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
	long	ret;
	dtrace_id_t id;
	struct pt_regs *regs = (struct pt_regs *) arg1;

	TRACE_BEFORE(__NR_iopl, arg0, arg1, arg2, arg3, arg4, arg5);

	ret = sys_iopl_ptr(arg0, arg1, arg2, arg3, arg4, arg5);

	TRACE_AFTER(__NR_iopl, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
# else /* <= 2.6.31 */
	systrace_sysent_t s;

	s = systrace_sysent[__NR_iopl];
	s.stsy_underlying = sys_iopl_ptr;
	return dtrace_systrace_syscall2(__NR_iopl, &s,
		FALSE, ARG0_PTR(arg0), 
		arg0, arg1, arg2, arg3, arg4, arg5);
# endif
}
/**********************************************************************/
/*   2nd  part  of  the  sig_rt_sigreturn()  syscall.  This  syscall  */
/*   appears  to  be  used  by "xinit" (i.e. if we dont emulate this  */
/*   properly, a raw "xinit" may fail to launch or hang).	      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall_rt_sigreturn(uintptr_t regs, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
	long	ret;
	dtrace_id_t id;

	TRACE_BEFORE(__NR_rt_sigreturn, regs, arg1, arg2, arg3, arg4, arg5);

	ret = sys_rt_sigreturn_ptr(regs, arg1, arg2, arg3, arg4, arg5);

	TRACE_AFTER(__NR_rt_sigreturn, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;

# else /* <= 2.6.31 */
	systrace_sysent_t s;

	s = systrace_sysent[__NR_rt_sigreturn];
	s.stsy_underlying = sys_rt_sigreturn_ptr;
	return dtrace_systrace_syscall2(__NR_rt_sigreturn, &s,
		FALSE, ARG0_PTR(regs), 
		regs, arg1, arg2, arg3, arg4, arg5);
# endif
}
/**********************************************************************/
/*   2nd part of the sig_rt_sigsuspend() syscall.		      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall_rt_sigsuspend(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{	systrace_sysent_t s;

	s = systrace_sysent[__NR_rt_sigsuspend];
	s.stsy_underlying = sys_rt_sigsuspend_ptr;
	return dtrace_systrace_syscall2(__NR_rt_sigsuspend, &s,
		FALSE, ARG0_PTR(arg0), 
		arg0, arg1, arg2, arg3, arg4, arg5);
}
/**********************************************************************/
/*   2nd part of the sigaltstack() syscall.				      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall_sigaltstack(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
	long	ret;
	dtrace_id_t id;
	struct pt_regs *regs = (struct pt_regs *) arg0;

	TRACE_BEFORE(__NR_sigaltstack, arg0, arg1, arg2, arg3, arg4, arg5);

	ret = sys_sigaltstack_ptr(arg0, arg1, arg2, arg3, arg4, arg5);

	TRACE_AFTER(__NR_sigaltstack, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
# else
	systrace_sysent_t s;

	s = systrace_sysent[__NR_sigaltstack];
	s.stsy_underlying = sys_sigaltstack_ptr;
	return dtrace_systrace_syscall2(__NR_sigaltstack, &s,
		FALSE, ARG0_PTR(arg0), 
		arg0, arg1, arg2, arg3, arg4, arg5);
# endif
}
/**********************************************************************/
/*   2nd part of the sigaltstack() syscall.				      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall_sigsuspend(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{	systrace_sysent_t s;

	s = systrace_sysent[__NR_rt_sigsuspend];
	s.stsy_underlying = sys_rt_sigsuspend_ptr;
	return dtrace_systrace_syscall2(__NR_rt_sigsuspend, &s,
		FALSE, ARG0_PTR(arg0), 
		arg0, arg1, arg2, arg3, arg4, arg5);
}
/**********************************************************************/
/*   2nd part of the vfork() syscall.				      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall_vfork(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
	long	ret;
	dtrace_id_t id;
	struct pt_regs *regs = (struct pt_regs *) arg0;

	TRACE_BEFORE(__NR_vfork, arg0, arg1, arg2, arg3, arg4, arg5);

	ret = sys_vfork_ptr(arg0, arg1, arg2, arg3, arg4, arg5);

	TRACE_AFTER(__NR_vfork, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
# else /* <= 2.6.31 */
	systrace_sysent_t s;

	s = systrace_sysent[__NR_vfork];
	s.stsy_underlying = sys_vfork_ptr;
	return dtrace_systrace_syscall2(__NR_vfork, &s,
		FALSE, ARG0_PTR(arg0), 
		arg0, arg1, arg2, arg3, arg4, arg5);
# endif
}
# endif /* defined(__amd64) */

/**********************************************************************/
/*   This  handles  the  syscall  traps,  but  may  be called by the  */
/*   specialised  handlers,  like  stub_clone(),  where  we know the  */
/*   syscall already, and dont/cannot find it on the stack.	      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall2(int syscall, systrace_sysent_t *sy,
    int copy_frame, uintptr_t *arg0_ptr,
    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{	dtrace_id_t id;
	intptr_t	rval;
	struct pt_regs *pregs = (struct pt_regs *) arg0_ptr;

	/***********************************************/
	/*   May  want  to single thread this code if  */
	/*   we are debugging.			       */
	/***********************************************/
	if (do_slock) {
		mutex_enter(&slock);
	}

	if (dtrace_here) {
		printk("syscall=%d %s current=%p syscall=%ld\n", syscall, 
			syscall >= 0 && syscall < NSYSCALL ? syscallnames[syscall] : "dont-know", 
			get_current(), linux_get_syscall());
	}

        if ((id = sy->stsy_entry) != DTRACE_IDNONE) {
		cpu_core_t *this_cpu = cpu_get_this();
		this_cpu->cpuc_regs = pregs;

                (*systrace_probe)(id, arg0, arg1, arg2, arg3, arg4, arg5);
	}

        /*
         * We want to explicitly allow DTrace consumers to stop a process
         * before it actually executes the meat of the syscall.
         */
# if defined(TODOxxx)
        {proc_t *p = ttoproc(curthread);
        mutex_enter(&p->p_lock);
        if (curthread->t_dtrace_stop && !curthread->t_lwp->lwp_nostop) {
                curthread->t_dtrace_stop = 0;
                stop(PR_REQUESTED, 0);
        }
        mutex_exit(&p->p_lock);
	}
# else
	{
	sol_proc_t *solp = par_setup_thread2();
        if (solp && solp->t_dtrace_stop) {
                curthread->t_dtrace_stop = 0;
		send_sig(SIGSTOP, current, 0);
	}
	}
# endif

	/***********************************************/
	/*   This  is  the  magic that calls the real  */
	/*   syscall...we need to be careful, because  */
	/*   some  syscalls,  such as sigreturn, need  */
	/*   the  real frame so that registers can be  */
	/*   modified.				       */
	/***********************************************/
#if defined(__i386)
		__asm(
			/***********************************************/
			/*   Move  the  stack  pt_regs  to  be in the  */
			/*   right place for the underlying syscall.   */
			/*   					       */
			/*   We    are    copying   pt_regs   because  */
			/*   sys_fork(),  for  instance  is called as  */
			/*   sys_fork(struct pt_regs regs)	       */
			/***********************************************/
			"push 60(%%esi)\n"

			"push 56(%%esi)\n"
			"push 52(%%esi)\n"
			"push 48(%%esi)\n"
			"push 44(%%esi)\n"
			"push 40(%%esi)\n"
			"push 36(%%esi)\n"
			"push 32(%%esi)\n"
			"push 28(%%esi)\n"
			"push 24(%%esi)\n"
			"push 20(%%esi)\n"
			"push 16(%%esi)\n"
			"push 12(%%esi)\n"
			"push 8(%%esi)\n"
			"push 4(%%esi)\n"
			"push 0(%%esi)\n"

			"call *%%edi\n"

			/***********************************************/
			/*   Copy the output pt_regs back to the home  */
			/*   location. This matters for sys_clone()    */
			/***********************************************/
			"pop 0(%%esi)\n"
			"pop 4(%%esi)\n"
			"pop 8(%%esi)\n"
			"pop 12(%%esi)\n"
			"pop 16(%%esi)\n"
			"pop 20(%%esi)\n"
			"pop 24(%%esi)\n"
			"pop 28(%%esi)\n"
			"pop 32(%%esi)\n"
			"pop 36(%%esi)\n"
			"pop 40(%%esi)\n"
			"pop 44(%%esi)\n"
			"pop 48(%%esi)\n"
			"pop 52(%%esi)\n"
			"pop 56(%%esi)\n"
			"pop 60(%%esi)\n"
		
	                : "=a" (rval)
	                : "S" (pregs), "D" (sy->stsy_underlying)
			);
# else

	/***********************************************/
	/*   x86-64  handler  here. Some kernels will  */
	/*   call  by  value,  the  args for pt_regs,  */
	/*   e.g.  exexcve()  on  2.6.9. Others wont.  */
	/*   When  we  call  by value, the stack must  */
	/*   contain  the pt_regs where the underlyer  */
	/*   expects,  else  it  will  panic  us.  We  */
	/*   arrange  to  memcpy()  the original call  */
	/*   frame  to  here,  and  then  put it back  */
	/*   afterwards,  since  the  underlyer maybe  */
	/*   setting  up  the user space register set  */
	/*   on clone/exec/fork/etc.		       */
	/*   					       */
	/*   In  theory,  always  copying  the  frame  */
	/*   should  be safe (assuming we do not walk  */
	/*   off  the  end  of the valid kernel stack  */
	/*   area).  For now we will pass in from the  */
	/*   caller what we expected.		       */
	/***********************************************/
//printk("syscall arg0=%p %p %p %p %p %p\n", arg0, arg1, arg2, arg3, arg4, arg5);
	if (copy_frame) {
		__asm(
			// Move the stack pt_regs to be in the right
			// place for the underlying syscall.
			"mov %%rcx,%%r15\n"
			"push 0xa0(%%r15)\n"
			"push 0x98(%%r15)\n"
			"push 0x90(%%r15)\n"
			"push 0x88(%%r15)\n"
			"push 0x80(%%r15)\n"
			"push 0x78(%%r15)\n"
			"push 0x70(%%r15)\n"
			"push 0x68(%%r15)\n"
			"push 0x60(%%r15)\n"
			"push 0x58(%%r15)\n"
			"push 0x50(%%r15)\n"
			"push 0x48(%%r15)\n"
			"push 0x40(%%r15)\n"
			"push 0x38(%%r15)\n"
			"push 0x30(%%r15)\n"
			"push 0x28(%%r15)\n"
			"push 0x20(%%r15)\n"
			"push 0x18(%%r15)\n"
			"push 0x10(%%r15)\n"
			"push 0x08(%%r15)\n"
			"push 0x00(%%r15)\n"

			"mov 0x68(%%r15),%%rsi\n"
			"mov 0x70(%%r15),%%rdi\n"
			"mov 0x60(%%r15),%%rdx\n"
			"mov %%rsp,%%rcx\n"

			"call *%%rax\n"

			// Copy the output pt_regs back to the home location
			"pop 0(%%r15)\n"
			"pop 0x08(%%r15)\n"
			"pop 0x10(%%r15)\n"
			"pop 0x18(%%r15)\n"
			"pop 0x20(%%r15)\n"
			"pop 0x28(%%r15)\n"
			"pop 0x30(%%r15)\n"
			"pop 0x38(%%r15)\n"
			"pop 0x40(%%r15)\n"
			"pop 0x48(%%r15)\n"
			"pop 0x50(%%r15)\n"
			"pop 0x58(%%r15)\n"
			"pop 0x60(%%r15)\n"
			"pop 0x68(%%r15)\n"
			"pop 0x70(%%r15)\n"
			"pop 0x78(%%r15)\n"
			"pop 0x80(%%r15)\n"
			"pop 0x88(%%r15)\n"
			"pop 0x90(%%r15)\n"
			"pop 0x98(%%r15)\n"
			"pop 0xa0(%%r15)\n"
		
	                : "=a" (rval)
	                : "c" (pregs), "a" (sy->stsy_underlying)
			);
		}
	else
	        rval = (*sy->stsy_underlying)(arg0, arg1, arg2, arg3, arg4, arg5);
# endif

//HERE();
//printk("syscall returns %d\n", rval);
# if defined(TODOxxx)
        if (ttolwp(curthread)->lwp_errno != 0)
                rval = -1;
# endif

        if ((id = sy->stsy_return) != DTRACE_IDNONE) {
		/***********************************************/
		/*   Map   Linux   style   syscall  codes  to  */
		/*   standard Unix format.		       */
		/***********************************************/
		(*systrace_probe)(id, (uintptr_t) (rval < 0 ? -1 : rval), 
		    (uintptr_t)(int64_t) rval,
                    (uintptr_t)((int64_t)rval >> 32), 0, 0, 0);
		}

	if (do_slock) {
		mutex_exit(&slock);
	}

        return (rval);
}
/**********************************************************************/
/*   Compute  which  callback we will need for the specific syscall.  */
/*   amd64 needs some special handlers, because of the layout of the  */
/*   stack on syscall entry.					      */
/**********************************************************************/
static void *
get_interposer(int sysnum, int enable)
{
	/***********************************************/
	/*   On  x86,  some syscalls have a different  */
	/*   arrangement   to   save  the  stack  and  */
	/*   registers.  So  we  have to handle these  */
	/*   with  stubs.  For all other syscalls, we  */
	/*   have  a common stub - so we can retrieve  */
	/*   the syscall number (in RAX or EAX).       */
	/***********************************************/
# if defined(__amd64)
	switch (sysnum) {
	  case __NR_clone:
		return (void *) systrace_part1_sys_clone;
	  case __NR_execve:
	  	patch_enable(&patch_execve, enable);
		return NULL;
	  case __NR_fork:
		return (void *) systrace_part1_sys_fork;
	  case __NR_iopl:
		return (void *) systrace_part1_sys_iopl;
	  case __NR_sigaltstack:
		return (void *) systrace_part1_sys_sigaltstack;
	  case __NR_rt_sigreturn:
	  	patch_enable(&patch_rt_sigreturn, enable);
		return NULL;
	  case __NR_rt_sigsuspend:
		return (void *) systrace_part1_sys_rt_sigsuspend;
	  case __NR_vfork:
		return (void *) systrace_part1_sys_vfork;
	  }
# endif
	return syscall_info[sysnum].s_template;
}
#if SYSCALL_64_32
static void *
get_interposer32(int sysnum, int enable)
{
	return syscall_info[sysnum].s_template32;
//	return syscall_template;
//	return (void *) dtrace_systrace_syscall;
}
#endif

static void
systrace_do_init(struct sysent *actual, systrace_sysent_t **interposed, 
	int nsyscall, void *systrace_syscall)
{
	systrace_sysent_t *sysent = *interposed;
	int i;

	/***********************************************/
	/*   Maybe 32b/64b - dont die badly here.      */
	/***********************************************/
	if (actual == NULL)
		return;

	if (sysent == NULL) {
		*interposed = sysent = kmem_zalloc(sizeof (systrace_sysent_t) *
		    nsyscall, KM_SLEEP);
	}

HERE();
//printk("systrace_do_init: nsyscall=%d\n", nsyscall);
	for (i = 0; i < nsyscall; i++) {
		struct sysent *a = &actual[i];
		systrace_sysent_t *s = &sysent[i];

		if (a->sy_callc == systrace_syscall)
			continue;

		if (s->stsy_underlying == NULL)
			s->stsy_underlying = a->sy_callc;
//printk("stsy_underlying=%p\n", s->stsy_underlying);
	}
}

/**********************************************************************/
/*   Code to enable or disable a patch structure.		      */
/**********************************************************************/
void
patch_enable(patch_t *pp, int enable)
{	unsigned char *codep;
	int     i;
	char	buf[128];
	int	last_s = 0;
	int	failed = FALSE;
static int dtrace_show_patches = TRUE;
static int do_patch = TRUE; // Set to FALSE whilst browsing for a patch

	/***********************************************/
	/*   If  we  are  disabling the patch, put it  */
	/*   back.				       */
	/***********************************************/
	if (!enable) {
		if (pp->p_taddr)
			*(int32_t *) pp->p_taddr = pp->p_val;
		return;
	}

	/***********************************************/
	/*   Initting  the  patch. Do first time hunt  */
	/*   if necessary.			       */
	/***********************************************/
	if (pp->p_taddr) {
		if (do_patch && pp->p_taddr)
			*(int32_t *) pp->p_taddr = pp->p_newval;
		return;
	}

	if (pp->p_baddr == NULL)
		pp->p_baddr = get_proc_addr(pp->p_name);

	codep = pp->p_baddr;
	if (dtrace_show_patches)
		printk("dtrace:patch_enable: %s\n", pp->p_name);
        for (i = 0; pp->p_array[i] && i < sizeof pp->p_array; i++) {
                int s = dtrace_instr_size(codep);
                if (s < 0)
                        break;
                sprintf(buf, "%02d:%p: sz=%02d", i, codep, s);
		last_s = s;
		while (s-- > 0)
			sprintf(buf + strlen(buf), " %02x", *codep++);
		strcat(buf, "\n");
		if (dtrace_show_patches)
			printk("%s", buf);

		/***********************************************/
		/*   If just browsing, let us see the dump.    */
		/***********************************************/
		if (!do_patch)
			continue;

		if (last_s != pp->p_array[i]) {
			if (dtrace_show_patches)
				printk("dtrace:patch: FAILED (size %d vs %d)\n",
					last_s, pp->p_array[i]);
			failed = TRUE;
			if (do_patch)
				return;
		}
		if (codep[-last_s] != pp->p_array[++i]) {
			if (dtrace_show_patches) {
				printk("dtrace:patch: FAILED (byte %02x vs %02x)\n",
					codep[-last_s], pp->p_array[i]);
			}
			failed = TRUE;
			if (do_patch)
				return;
		}
        }
	if (failed)
		return;
	codep = codep - last_s + pp->p_offset;
	if (dtrace_show_patches)
		printk("dtrace:patch: success @%p\n", codep);
	pp->p_taddr = codep;
	/***********************************************/
	/*   Save the old value.		       */
	/***********************************************/
	pp->p_val = *(int32_t *) pp->p_taddr;
	/***********************************************/
	/*   Compute relative jump target.	       */
	/***********************************************/
	pp->p_newval = ((unsigned char *) pp->p_code - (codep + 4));
	if (do_patch) {
		/***********************************************/
		/*   Ensure this block of memory is writable.  */
		/***********************************************/
		if (memory_set_rw(pp->p_taddr, 1, TRUE) == 0)
			printk("dtrace:patch_enable:Cannot make %p rd/wr\n", pp->p_taddr);
		*(int32_t *) pp->p_taddr = pp->p_newval;
	}
}
/**********************************************************************/
/*   This  gets  called when someone is trying to launch a probe and  */
/*   we need to intercept the syscall entries.			      */
/**********************************************************************/
/*ARGSUSED*/
static void
systrace_provide(void *arg, const dtrace_probedesc_t *desc)
{	int i;

//HERE();
//printk("descr=%p\n", desc);
	if (desc != NULL)
		return;

	init_syscalls();

	if (sysent == NULL)
		sysent = fbt_get_sys_call_table();
#if defined(__amd64)
	if (sysent32 == NULL)
		sysent32 = get_proc_addr("ia32_sys_call_table");
#endif

	if (sysent == NULL) {
		printk("systrace.c: Cannot locate sys_call_table - not enabling syscalls\n");
		return;
	}

	/***********************************************/
	/*   Handle specials for the amd64 syscalls.   */
	/***********************************************/
#if defined(__amd64)
	/***********************************************/
	/*   Special handling for the syscalls - find  */
	/*   the bits of code we want to patch.	       */
	/***********************************************/
	if (sys_clone_ptr == NULL)
		sys_clone_ptr = get_proc_addr("sys_clone");
	if (sys_execve_ptr == NULL)
		sys_execve_ptr = get_proc_addr("sys_execve");
	if (sys_fork_ptr == NULL)
		sys_fork_ptr = get_proc_addr("sys_fork");
	if (sys_iopl_ptr == NULL)
		sys_iopl_ptr = get_proc_addr("sys_iopl");
	if (sys_rt_sigreturn_ptr == NULL)
		sys_rt_sigreturn_ptr = get_proc_addr("sys_rt_sigreturn");
	if (sys_rt_sigsuspend_ptr == NULL)
		sys_rt_sigsuspend_ptr = get_proc_addr("sys_rt_sigsuspend");
	if (sys_sigaltstack_ptr == NULL)
		sys_sigaltstack_ptr = get_proc_addr("sys_sigaltstack");
	if (sys_vfork_ptr == NULL)
		sys_vfork_ptr = get_proc_addr("sys_vfork");

	if (int_ret_from_sys_call_ptr == NULL)
		int_ret_from_sys_call_ptr = (char *) get_proc_addr("int_ret_from_sys_call");
	if (ptregscall_common_ptr == NULL)
		ptregscall_common_ptr = (char *) get_proc_addr("ptregscall_common");
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
	if (save_rest_ptr == NULL)
		save_rest_ptr = (char *) get_proc_addr("save_rest");
# endif
	if (do_fork_ptr == NULL)
		do_fork_ptr = (void *) get_proc_addr("do_fork");
#endif

	systrace_do_init(sysent, &systrace_sysent, NSYSCALL, dtrace_systrace_syscall);
	for (i = 0; i < NSYSCALL; i++) {
		char	*name = syscallnames[i];

		/***********************************************/
		/*   Be  careful  in  case we have rubbish in  */
		/*   the name.				       */
		/***********************************************/
		if (name == NULL || !validate_ptr(name))
			continue;

		/***********************************************/
		/*   Prefix     now    stripped    off    via  */
		/*   mksyscall.pl,  dont  need to waste space  */
		/*   doing this at runtime.		       */
		/***********************************************/
		/*
		if (strncmp(name, "__NR_", 5) == 0)
			name += 5;
		*/

#if defined(__amd64)
#  define bits_name "x64"
#else
#  define bits_name "x32"
#endif

		if (systrace_sysent[i].stsy_underlying == NULL)
			continue;

		if (dtrace_probe_lookup(systrace_id, bits_name, name, "entry") != 0)
			continue;

		if (dtrace_here)
			printk("systrace_provide: patch syscall #%d: %s\n", i, name);
		(void) dtrace_probe_create(systrace_id, bits_name, name,
		    "entry", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_ENTRY(i)));

		(void) dtrace_probe_create(systrace_id, bits_name, name,
		    "return", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_RETURN(i)));

		systrace_sysent[i].stsy_entry = DTRACE_IDNONE;
		systrace_sysent[i].stsy_return = DTRACE_IDNONE;
	}

	/***********************************************/
	/*   On 64b kernel, we need to ensure the 32b  */
	/*   syscalls map to the same probe id as the  */
	/*   64b  equivalent,  but  we are not 1:1 or  */
	/*   using  the same index values, so we need  */
	/*   to  hunt  for  the 64b equiv if there is  */
	/*   one.				       */
	/***********************************************/
#ifdef SYSCALL_64_32
	systrace_do_init(sysent32, &systrace_sysent32, NSYSCALL32, dtrace_systrace_syscall);
	for (i = 0; sysent32 && i < NSYSCALL32; i++) {
		char	*name = syscallnames32[i];

		/***********************************************/
		/*   Be  careful  in  case we have rubbish in  */
		/*   the name.				       */
		/***********************************************/
		if (name == NULL || !validate_ptr(name))
			continue;

		/***********************************************/
		/*   Prefix     now    stripped    off    via  */
		/*   mksyscall.pl,  dont  need to waste space  */
		/*   doing this at runtime.		       */
		/***********************************************/
		/*
		if (strncmp(name, "__NR_", 5) == 0)
			name += 5;
		*/

		if (systrace_sysent32[i].stsy_underlying == NULL)
			continue;

		/***********************************************/
		/*   This  avoids  us  recreating more probes  */
		/*   with  the  same  name.  We  use  the x32  */
		/*   family  to  avoid  a  clash with the x64  */
		/***********************************************/
		if (dtrace_probe_lookup(systrace_id, "x32", name, "entry") != 0)
			continue;

		if (dtrace_here)
			printk("systrace_provide: patch syscall32 #%d: %s\n", i, name);
		(void) dtrace_probe_create(systrace_id, "x32", name,
		    "entry", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_ENTRY32(i)));

		(void) dtrace_probe_create(systrace_id, "x32", name,
		    "return", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_RETURN32(i)));

		systrace_sysent32[i].stsy_entry = DTRACE_IDNONE;
		systrace_sysent32[i].stsy_return = DTRACE_IDNONE;
	}
#endif
}

/*ARGSUSED*/
static void
systrace_destroy(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);

	sysnum = sysnum; // avoid compiler warning.

	/*
	 * There's nothing to do here but assert that we have actually been
	 * disabled.
	 */
	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		ASSERT(systrace_sysent[sysnum].stsy_entry == DTRACE_IDNONE);
#ifdef SYSCALL_64_32
		ASSERT(systrace_sysent32[sysnum].stsy_entry == DTRACE_IDNONE);
#endif
	} else {
		ASSERT(systrace_sysent[sysnum].stsy_return == DTRACE_IDNONE);
#ifdef SYSCALL_64_32
		ASSERT(systrace_sysent32[sysnum].stsy_return == DTRACE_IDNONE);
#endif
	}
}


#if SYSCALL_64_32
/*ARGSUSED*/
static void
systrace_disable32(void *arg, dtrace_id_t id, void *parg)
{	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);
	void	*syscall_func = get_interposer32(sysnum, FALSE);
	int disable = (systrace_sysent32[sysnum].stsy_entry == DTRACE_IDNONE ||
	    systrace_sysent32[sysnum].stsy_return == DTRACE_IDNONE);

	/***********************************************/
	/*   This  may  because the patch_enable code  */
	/*   got invoked.			       */
	/***********************************************/
	if (syscall_func == NULL)
		return;

	if (disable) {
		casptr(&sysent32[sysnum].sy_callc,
		    syscall_func,
		    (void *)systrace_sysent32[sysnum].stsy_underlying);
	}

	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		systrace_sysent32[sysnum].stsy_entry = DTRACE_IDNONE;
	} else {
		systrace_sysent32[sysnum].stsy_return = DTRACE_IDNONE;
	}
}
#endif

/*ARGSUSED*/
static void
systrace_disable(void *arg, dtrace_id_t id, void *parg)
{	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);
	void	*syscall_func = get_interposer(sysnum, FALSE);
	int	disable;

#if SYSCALL_64_32
	if (CAST_TO_INT(parg) & STF_32BIT) {
		systrace_disable32(arg, id, parg);
		return;
	}
#endif
	disable = (systrace_sysent[sysnum].stsy_entry == DTRACE_IDNONE ||
	    systrace_sysent[sysnum].stsy_return == DTRACE_IDNONE);

	/***********************************************/
	/*   This  may  because the patch_enable code  */
	/*   got invoked.			       */
	/***********************************************/
	if (syscall_func == NULL)
		return;

	if (disable) {
		casptr(&sysent[sysnum].sy_callc,
		    syscall_func,
		    (void *)systrace_sysent[sysnum].stsy_underlying);
	}

	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		systrace_sysent[sysnum].stsy_entry = DTRACE_IDNONE;
	} else {
		systrace_sysent[sysnum].stsy_return = DTRACE_IDNONE;
	}
}

#if SYSCALL_64_32
static int
systrace_enable32(void *arg, dtrace_id_t id, void *parg)
{	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);
	void	*syscall_func = get_interposer32(sysnum, TRUE);
	int enabled = (systrace_sysent32[sysnum].stsy_entry != DTRACE_IDNONE ||
			systrace_sysent32[sysnum].stsy_return != DTRACE_IDNONE);
	    
	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		systrace_sysent32[sysnum].stsy_entry = id;
	} else {
		systrace_sysent32[sysnum].stsy_return = id;
	}

	if (enabled)
		return 0;

	/***********************************************/
	/*   This  may  be  because  the patch_enable  */
	/*   code got invoked.			       */
	/***********************************************/
	if (syscall_func == NULL)
		return 0;

	/***********************************************/
	/*   The  x86  kernel  will  page protect the  */
	/*   sys_call_table  and panic if we write to  */
	/*   it.  So....lets just turn off write-only  */
	/*   on  the  target page. We might even turn  */
	/*   it  back  on  when  we are finished, but  */
	/*   dont care for now.			       */
	/***********************************************/
	if (memory_set_rw(&sysent32[sysnum].sy_callc, 1, TRUE) == 0)
		return 0;

	if (dtrace_here) {
		printk("enable: sysnum=%d %p %p %p -> %p\n", sysnum,
			&sysent32[sysnum].sy_callc,
			    (void *)systrace_sysent32[sysnum].stsy_underlying,
			    (void *)syscall_func,
				sysent32[sysnum].sy_callc);
	}

	casptr(&sysent32[sysnum].sy_callc,
	    (void *)systrace_sysent32[sysnum].stsy_underlying,
	    (void *)syscall_func);

	if (dtrace_here) {
		printk("enable: ------=%d %p %p %p -> %p\n", sysnum,
			&sysent[sysnum].sy_callc,
			    (void *)systrace_sysent[sysnum].stsy_underlying,
			    (void *)syscall_func,
				sysent[sysnum].sy_callc);
	}
	return 0;
}
#endif
/*static void
dump_syscalls(void)
{	int	i;
	long sum = 0;

	for (i = 0; i < NSYSCALL; i++) {
		printk("syscall[%d] %p %s\n", i, sysent[i].sy_callc, syscallnames[i]);
		sum += (long) sysent[i].sy_callc;
	}
	printk("cksum=%lx\n", sum);
}
*/
/**********************************************************************/
/*   Someone  is trying to trace a syscall. Enable/patch the syscall  */
/*   table (if not done already).				      */
/**********************************************************************/
/*ARGSUSED*/
static int
systrace_enable(void *arg, dtrace_id_t id, void *parg)
{	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);
	void	*syscall_func = get_interposer(sysnum, TRUE);
	int enabled = (systrace_sysent[sysnum].stsy_entry != DTRACE_IDNONE ||
			systrace_sysent[sysnum].stsy_return != DTRACE_IDNONE);

#if SYSCALL_64_32
	if (CAST_TO_INT(parg) & STF_32BIT) {
		systrace_enable32(arg, id, parg);
		return 0;
	}
#endif

	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		systrace_sysent[sysnum].stsy_entry = id;
	} else {
		systrace_sysent[sysnum].stsy_return = id;
	}

	if (enabled)
		return 0;

	/***********************************************/
	/*   This  may  be  because  the patch_enable  */
	/*   code got invoked.			       */
	/***********************************************/
	if (syscall_func == NULL)
		return 0;

	/***********************************************/
	/*   The  x86  kernel  will  page protect the  */
	/*   sys_call_table  and panic if we write to  */
	/*   it.  So....lets just turn off write-only  */
	/*   on  the  target page. We might even turn  */
	/*   it  back  on  when  we are finished, but  */
	/*   dont care for now.			       */
	/***********************************************/
	if (memory_set_rw(&sysent[sysnum].sy_callc, 1, TRUE) == 0)
		return 0;

	if (dtrace_here) {
		printk("enable: sysnum=%d %p %p %p\n", sysnum,
			sysent[sysnum].sy_callc,
			(void *)systrace_sysent[sysnum].stsy_underlying,
			(void *)syscall_func);
	}

	casptr(&sysent[sysnum].sy_callc,
	    (void *)systrace_sysent[sysnum].stsy_underlying,
	    (void *)syscall_func);

	if (dtrace_here) {
		printk("enable: ------=%d %p %p %p\n", sysnum,
			sysent[sysnum].sy_callc,
			(void *)systrace_sysent[sysnum].stsy_underlying,
			(void *)syscall_func);
	}

	return 0;
}
/*ARGSUSED*/
void
systrace_stub(dtrace_id_t id, uintptr_t arg0, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
}

static dtrace_pattr_t systrace_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

static dtrace_pops_t systrace_pops = {
	systrace_provide,
	NULL,
	systrace_enable,
	systrace_disable,
	NULL, // dtps_suspend
	NULL, // dtps_resume
	NULL, // dtps_getargdesc
	NULL, // dtps_getargval
	NULL, // dtps_usermode
	systrace_destroy
};

static int
systrace_attach(void)
{

	systrace_probe = (void (*)(dtrace_id_t, uintptr_t arg0, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)) dtrace_probe;
	membar_enter();

	if (dtrace_register("syscall", &systrace_attr, DTRACE_PRIV_USER, NULL,
	    &systrace_pops, NULL, &systrace_id) != 0) {
		systrace_probe = systrace_stub;
		return DDI_FAILURE;
		}

	return (DDI_SUCCESS);
}
static int
systrace_detach(void)
{
	if (dtrace_unregister(systrace_id) != 0)
		return (DDI_FAILURE);

	systrace_probe = systrace_stub;
	if (templates)
		vfree(templates);
	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
systrace_open(struct inode *inode, struct file *file)
{
	return (0);
}

static const struct file_operations systrace_fops = {
        .open = systrace_open,
};

static struct miscdevice systrace_dev = {
        MISC_DYNAMIC_MINOR,
        "systrace",
        &systrace_fops
};

static int initted;

int systrace_init(void)
{	int	ret;

	/***********************************************/
	/*   This  is  a  run-time  and not a compile  */
	/*   time test.				       */
	/***********************************************/
	if (SYSTRACE_MASK <= NSYSCALL) {
		printk("systrace: too many syscalls? %d is too large\n", (int) NSYSCALL);
		return 0;
	}
	if (SYSTRACE_MASK <= NSYSCALL32) {
		printk("systrace: too many syscalls(32b)? %d is too large\n", (int) NSYSCALL32);
		return 0;
	}

	ret = misc_register(&systrace_dev);
	if (ret) {
		printk(KERN_WARNING "systrace: Unable to register misc device\n");
		return ret;
		}

	initted = TRUE;

	systrace_attach();

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
//	printk(KERN_WARNING "systrace loaded: /dev/systrace now available\n");

	return 0;
}
void systrace_exit(void)
{
	if (initted) {
		systrace_detach();

//	printk(KERN_WARNING "systrace driver unloaded.\n");
		misc_deregister(&systrace_dev);
	}
}

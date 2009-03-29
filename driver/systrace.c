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

//#pragma ident	"@(#)systrace.c	1.6	06/09/19 SMI"

#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include <linux/sched.h>
#include <linux/sys.h>
#include <linux/highmem.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
//#include <asm/asm-offsets.h>
//#include <asm/tlbflush.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <linux/miscdevice.h>
#include <linux/syscalls.h>
#include <sys/dtrace.h>
#include <sys/systrace.h>
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

#define	SYSTRACE_SHIFT			16
#define	SYSTRACE_ISENTRY(x)		((int)(x) >> SYSTRACE_SHIFT)
#define	SYSTRACE_SYSNUM(x)		((int)(x) & ((1 << SYSTRACE_SHIFT) - 1))
#define	SYSTRACE_ENTRY(id)		((1 << SYSTRACE_SHIFT) | (id))
#define	SYSTRACE_RETURN(id)		(id)

# if !defined(__NR_syscall_max)                                              
#       if !defined(NR_syscalls)                                             
#               define NSYSCALL (sizeof syscallnames / sizeof syscallnames[0])
#       else                                                                 
#               define NSYSCALL NR_syscalls                                  
#       endif                                                                
# else                                                                       
#       define NSYSCALL __NR_syscall_max                                     
# endif

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

struct sysent {
        asmlinkage int64_t         (*sy_callc)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);  /* C-style call hander or wrapper */
};

#define LOADABLE_SYSCALL(s)     (s->sy_flags & SE_LOADABLE)
#define LOADED_SYSCALL(s)       (s->sy_flags & SE_LOADED)
#define SE_LOADABLE     0x08            /* syscall is loadable */
#define SE_LOADED       0x10            /* syscall is completely loaded */

systrace_sysent_t *systrace_sysent;

struct sysent *sysent;
static dtrace_provider_id_t systrace_id;

/**********************************************************************/
/*   This  needs to be defined in sysent.c - just need to figure out  */
/*   the equivalent in Linux...					      */
/**********************************************************************/
void	*fbt_get_sys_call_table(void);
void (*systrace_probe)(dtrace_id_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t, uintptr_t);
void	*par_setup_thread2(void);

# define linux_get_syscall() get_current()->thread.trap_no

DEFINE_MUTEX(slock);
static int do_slock;

/**********************************************************************/
/*   Dont  make  this  static else gcc will optimise it all away and  */
/*   the functions inside it will be undefs.			      */
/**********************************************************************/
void
systrace_assembler_dummy()
{
	__asm(
		FUNCTION(zz)
/*		"push %rdi\n"
		"push %rsi\n"
		"push %rdx\n"
		"push %rcx\n"
		"push %rax\n"
		"push %r8\n"
		"push %r9\n"
		"push %r10\n"
		"push %r11\n"
		"push %rbx\n"
		"push %rbp\n"
		"push %r12\n"
		"push %r13\n"
		"push %r14\n"
		"push %r15\n"
*/
		"call dtrace_systrace_syscall\n"
/*
		"pop %r15\n"
		"pop %r14\n"
		"pop %r13\n"
		"pop %r12\n"
		"pop %rbp\n"
		"pop %rbx\n"
		"pop %r11\n"
		"pop %r10\n"
		"pop %r9\n"
		"pop %r8\n"
		"add $8,%rsp\n"
		"pop %rcx\n"
		"pop %rdx\n"
		"pop %rsi\n"
		"pop %rdi\n"
*/
		"ret\n"
		END_FUNCTION(zz)
		);

}

/**********************************************************************/
/*   This  is  the  function which is called when a syscall probe is  */
/*   hit. We essentially wrap the call with the entry/return probes.  */
/*   Some assembler mess to hide what we did.			      */
/**********************************************************************/
asmlinkage int64_t
dtrace_systrace_syscall(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
#if defined(sun)
	int syscall = curthread->t_sysnum;
#else
	int syscall; // = linux_get_syscall();
#endif
        systrace_sysent_t *sy;
        dtrace_id_t id;
        intptr_t	rval;
	void **ptr = (void **) &arg0;

	/***********************************************/
	/*   May  want  to single thread this code if  */
	/*   we are debugging.			       */
	/***********************************************/
	if (do_slock) {
		mutex_enter(&slock);
	}

	/***********************************************/
	/*   Following   useful   to  help  find  the  */
	/*   syscall arg on the stack.		       */
	/***********************************************/
	if (0) {
		int i; 
		for (i = 0; i < 20; i++) {
			printk("%p: stack[%d] = %p\n", ptr + i, i, ptr[i]);
		}
	}

	/***********************************************/
	/*   We  need to use the appropriate framereg  */
	/*   struct,  but  I  havent been bothered to  */
	/*   dig  it out. These magic offsets seem to  */
	/*   work.				       */
	/***********************************************/
# if defined(__i386)
	syscall = (int) ptr[6]; // horrid hack
# else
	/***********************************************/
	/*   This  is  slightly  nicer than the older  */
	/*   [12] hack -- the position of the syscall  */
	/*   can  depend  on the inline assembler and  */
	/*   function       optimisation.       Using  */
	/*   __builtin_frame_address()  means  we get  */
	/*   to  the right value irrespective of what  */
	/*   happens   inside  this  functions  stack  */
	/*   layout/optimisation.		       */
	/***********************************************/
	{unsigned long *ret_sp = __builtin_frame_address(0);
	syscall = (int) ret_sp[6];
//	syscall = (int) (long) ptr[12]; // horrid hack
	}
# endif
printk("syscall=%d\n", syscall);
	if ((unsigned) syscall >= NSYSCALL) {
		printk("dtrace:help: Got syscall=%d - out of range (max=%d)\n", syscall, NSYSCALL);
		return -EINVAL;
	}

        sy = &systrace_sysent[syscall];

	if (dtrace_here) {
		printk("syscall=%d %s current=%p syscall=%ld\n", syscall, 
			syscall >= 0 && syscall < NSYSCALL ? syscallnames[syscall] : "dont-know", 
			get_current(), linux_get_syscall());
	}

//printk("arg0=%s %p %p %p %p %p\n", arg0, arg1, arg2, arg3, arg4, arg5);
        if ((id = sy->stsy_entry) != DTRACE_IDNONE) {
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
	{
	struct pt_regs *pregs = (struct pt_regs *) &arg0;
	__asm(
		// Move the stack pt_regs to be in the right
		// place for the underlying syscall.
		"push 64(%%esi)\n"
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

		// Copy the output pt_regs back to the home location
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
		"pop 64(%%esi)\n"
		
                : "=a" (rval)
                : "S" (pregs), "D" (sy->stsy_underlying)
		);
	}
# else
# if 0
///dirty hack land going on here...ignore this mess for now...
// this is to debug x86-64 sys_clone syscalls.
	uintptr_t args[] = {arg0, arg1, arg2, arg3, arg4, arg5, sy->stsy_underlying};
//        rval = (*sy->stsy_underlying)(arg0, arg1, arg2, arg3, arg4, arg5);

__asm(
"mov 0x00(%%rax),%%rdi\n"
"mov 0x08(%%rax),%%rsi\n"
"mov 0x10(%%rax),%%rdx\n"
"mov 0x18(%%rax),%%rcx\n"
"mov 0x20(%%rax),%%r8\n"
"mov 0x28(%%rax),%%r9\n"
"mov 0x30(%%rax),%%rax\n"
"call *%%rax\n"
"mov %%rbx,-0x28(%%rbp)\n"
"mov %%r12,-0x20(%%rbp)\n"
"mov %%r13,-0x18(%%rbp)\n"
"mov %%r14,-0x10(%%rbp)\n"
"mov %%r15,-8(%%rbp)\n"
                : "=a" (rval)
                : "A" (args)
		: "rbx", "rsi", "rdi", "rcx", "r8", "r9", "rdx", "r10", "r11", "r12", "r13", "r14", "r15"
);
# elif 0
dtrace_dump_mem64(__builtin_frame_address(0) + 16, 20 + 4);
	{
	uintptr_t args[6] = {arg0, arg1, arg2, arg3, arg4, arg5};

printk("syscall %p arg0=%p %p %p %p %p %p\n", &arg0, arg0, arg1, arg2, arg3, arg4, arg5);
printk("calling %p\n", sy->stsy_underlying);
	__asm(
		// Move the stack pt_regs to be in the right
		// place for the underlying syscall.


/*		"pushq 0x50(%%rbx)\n"
		"pushq 0x48(%%rbx)\n"
		"pushq 0x40(%%rbx)\n"
		"pushq 0x38(%%rbx)\n"
		"pushq 0x30(%%rbx)\n"
		"pushq 0x28(%%rbx)\n"
		"pushq 0x20(%%rbx)\n"
		"pushq 0x18(%%rbx)\n"
		"pushq 0x10(%%rbx)\n"
		"pushq 8(%%rbx)\n"
		"pushq 0(%%rbx)\n"
*/
		"mov %%rbp,%%rbx\n"
		"add $16, %%rbx\n"
		"mov %1,%%rbx\n"
		"mov %%r8,%%rbx\n"
//"mov %%rbx,%%rcx\n"
		"mov 0x70(%%rbx),%%rdi\n"
		"mov 0x68(%%rbx),%%rsi\n"
		"mov 0x60(%%rbx),%%rdx\n"
		"mov 0x58(%%rbx),%%rcx\n"
		//0x50
		"mov 0x48(%%rbx),%%r8\n"
		"mov 0x40(%%rbx),%%r9\n"
		"mov 0x38(%%rbx),%%r10\n"
		"mov 0x30(%%rbx),%%r11\n"
		//rbx
		//rbp
		"mov 0x18(%%rbx),%%r12\n"
		"mov 0x10(%%rbx),%%r13\n"
		"mov 0x08(%%rbx),%%r14\n"
		"mov 0x00(%%rbx),%%r15\n"
		"mov 0x28(%%rbx),%%rbx\n"

		"call *%%rax\n"
"mov %%rbx,-0x28(%%rbp)\n"
"mov %%r12,-0x20(%%rbp)\n"
"mov %%r13,-0x18(%%rbp)\n"
"mov %%r14,-0x10(%%rbp)\n"
"mov %%r15,-8(%%rbp)\n"

/*		"mov %%rbp,%%rbx\n"
		"add $16, %%rbx\n"
"nop\n" "nop\n" "nop\n"
        "movq %%rdi,0x70(%%rbx)\n"
	"movq %%rsi,0x68(%%rbx)\n"
	"movq %%rdx,0x60(%%rbx)\n"
	"movq %%rcx,0x58(%%rbx)\n"
	"movq %%rax,0x50(%%rbx)\n"
	"movq %%r8,0x48(%%rbx)\n"
	"movq %%r9,0x40(%%rbx)\n"
	"movq %%r10,0x38(%%rbx)\n"
	"movq %%r11,0x30(%%rbx)\n"
	//rbx
	"movq %%rbp,0x20(%%rbx)\n"
	"movq %%r12,0x18(%%rbx)\n"
	"movq %%r13,0x10(%%rbx)\n"
	"movq %%r14,0x08(%%rbx)\n"
	"movq %%r15,0x00(%%rbx)\n"
	"movq %%rbx,0x28(%%rbx)\n"
*/
                : "=a" (rval)
                : "A" (sy->stsy_underlying), "S" (args)
		: "rbx", "rdi", "rcx", "r8", "r9", "rdx", "r10", "r11", "r12", "r13", "r14", "r15"
		);

	}
//printk("after syscall rval=%d \n", rval);
//dtrace_dump_mem64(__builtin_frame_address(0) + 16, 20 + 4);
# else
printk("syscall arg0=%p %p %p %p %p %p\n", arg0, arg1, arg2, arg3, arg4, arg5);
        rval = (*sy->stsy_underlying)(arg0, arg1, arg2, arg3, arg4, arg5);
//{asmlinkage long (*func)() = sy->stsy_underlying;
//        rval = func(arg0, arg1, arg2, arg3, arg4);
//}
printk("after=%d\n", rval);
# endif
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

static void
systrace_do_init(struct sysent *actual, systrace_sysent_t **interposed)
{
	systrace_sysent_t *sysent = *interposed;
	int i;

	if (sysent == NULL) {
		*interposed = sysent = kmem_zalloc(sizeof (systrace_sysent_t) *
		    NSYSCALL, KM_SLEEP);
	}

HERE();
//printk("NSYSCALL=%d\n", (int) NSYSCALL);
	for (i = 0; i < NSYSCALL; i++) {
		struct sysent *a = &actual[i];
		systrace_sysent_t *s = &sysent[i];

# if defined(sun)
		if (LOADABLE_SYSCALL(a) && !LOADED_SYSCALL(a))
			continue;
# endif

		if (a->sy_callc == dtrace_systrace_syscall)
			continue;

#ifdef _SYSCALL32_IMPL
		if (a->sy_callc == dtrace_systrace_syscall32)
			continue;
#endif

		s->stsy_underlying = a->sy_callc;
//printk("stsy_underlying=%p\n", s->stsy_underlying);
	}
}

/*ARGSUSED*/
static void
systrace_provide(void *arg, const dtrace_probedesc_t *desc)
{
	int i;
HERE();
//printk("descr=%p\n", desc);
	if (desc != NULL)
		return;

	if (sysent == NULL)
		sysent = fbt_get_sys_call_table();

	if (sysent == NULL) {
		printk("systrace.c: Cannot locate sys_call_table - not enabling syscalls\n");
		return;
	}

	systrace_do_init(sysent, &systrace_sysent);
#ifdef _SYSCALL32_IMPL
	systrace_do_init(sysent32, &systrace_sysent32);
#endif
	for (i = 0; i < NSYSCALL; i++) {
		char	*name = syscallnames[i];

		/***********************************************/
		/*   Be  careful  in  case we have rubbish in  */
		/*   the name.				       */
		/***********************************************/
		if (name == NULL || !validate_ptr(name))
			continue;

		if (strncmp(name, "__NR_", 5) == 0)
			name += 5;

		if (systrace_sysent[i].stsy_underlying == NULL)
			continue;

		if (dtrace_probe_lookup(systrace_id, NULL,
		    name, "entry") != 0)
			continue;

		if (dtrace_here)
			printk("systrace_provide: patch syscall #%d: %s\n", i, name);
		(void) dtrace_probe_create(systrace_id, NULL, name,
		    "entry", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_ENTRY(i)));

		(void) dtrace_probe_create(systrace_id, NULL, name,
		    "return", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_RETURN(i)));

		systrace_sysent[i].stsy_entry = DTRACE_IDNONE;
		systrace_sysent[i].stsy_return = DTRACE_IDNONE;
#ifdef _SYSCALL32_IMPL
		systrace_sysent32[i].stsy_entry = DTRACE_IDNONE;
		systrace_sysent32[i].stsy_return = DTRACE_IDNONE;
#endif
	}
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
#ifdef _SYSCALL32_IMPL
		ASSERT(systrace_sysent32[sysnum].stsy_entry == DTRACE_IDNONE);
#endif
	} else {
		ASSERT(systrace_sysent[sysnum].stsy_return == DTRACE_IDNONE);
#ifdef _SYSCALL32_IMPL
		ASSERT(systrace_sysent32[sysnum].stsy_return == DTRACE_IDNONE);
#endif
	}
}

/**********************************************************************/
/*   Someone  is trying to trace a syscall. Enable/patch the syscall  */
/*   table (if not done already).				      */
/**********************************************************************/
/*ARGSUSED*/
static void
systrace_enable(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);
/*
	int enabled = (systrace_sysent[sysnum].stsy_entry != DTRACE_IDNONE ||
	    systrace_sysent[sysnum].stsy_return != DTRACE_IDNONE);
*/

//printk("\n\nsystrace_sysent[%p].stsy_entry = %x\n", parg, systrace_sysent[sysnum].stsy_entry);
//printk("\n\nsystrace_sysent[%p].stsy_return = %x\n", parg, systrace_sysent[sysnum].stsy_return);
	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		systrace_sysent[sysnum].stsy_entry = id;
	} else {
		systrace_sysent[sysnum].stsy_return = id;
	}

	/***********************************************/
	/*   The  x86  kernel  will  page protect the  */
	/*   sys_call_table  and panic if we write to  */
	/*   it.  So....lets just turn off write-only  */
	/*   on  the  target page. We might even turn  */
	/*   it  back  on  when  we are finished, but  */
	/*   dont care for now.			       */
	/***********************************************/
	if (memory_set_rw(&sysent[sysnum].sy_callc, 1, TRUE) == 0)
		return;

void	(*syscall_func)() = dtrace_systrace_syscall;
void zz();
//syscall_func = zz;
	if (dtrace_here) {
		printk("enable: sysnum=%d %p %p %p -> %p\n", sysnum,
			&sysent[sysnum].sy_callc,
			    (void *)systrace_sysent[sysnum].stsy_underlying,
			    (void *)syscall_func,
				sysent[sysnum].sy_callc);
	}

//sysent[sysnum].sy_callc = dtrace_systrace_syscall;
HERE();
/*
printk("calling caspt %p %p %p\n", &sysent[sysnum].sy_callc,
	    (void *)systrace_sysent[sysnum].stsy_underlying,
	    (void *)syscall_func);
*/
	casptr(&sysent[sysnum].sy_callc,
	    (void *)systrace_sysent[sysnum].stsy_underlying,
	    (void *)syscall_func);
HERE();
	if (dtrace_here) {
		printk("enable: ------=%d %p %p %p -> %p\n", sysnum,
			&sysent[sysnum].sy_callc,
			    (void *)systrace_sysent[sysnum].stsy_underlying,
			    (void *)syscall_func,
				sysent[sysnum].sy_callc);
	}
}

/*ARGSUSED*/
static void
systrace_disable(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);
	int disable = (systrace_sysent[sysnum].stsy_entry == DTRACE_IDNONE ||
	    systrace_sysent[sysnum].stsy_return == DTRACE_IDNONE);

	if (disable) {
		casptr(&sysent[sysnum].sy_callc,
		    (void *)dtrace_systrace_syscall,
		    (void *)systrace_sysent[sysnum].stsy_underlying);

	}

	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		systrace_sysent[sysnum].stsy_entry = DTRACE_IDNONE;
	} else {
		systrace_sysent[sysnum].stsy_return = DTRACE_IDNONE;
	}
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
static int initted;

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

	initted = 1;

	return (DDI_SUCCESS);
}
static int
systrace_detach(void)
{
	if (!initted)
		return DDI_SUCCESS;

	if (dtrace_unregister(systrace_id) != 0)
		return (DDI_FAILURE);

	systrace_probe = systrace_stub;
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
	if ((1 << SYSTRACE_SHIFT) <= NSYSCALL) {
		printk("systrace: 1 << SYSTRACE_SHIFT must exceed number of system calls\n");
		return 0;
	}
	ret = misc_register(&systrace_dev);
	if (ret) {
		printk(KERN_WARNING "systrace: Unable to register misc device\n");
		return ret;
		}

	systrace_attach();

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "systrace loaded: /dev/systrace now available\n");

	initted = 1;

	return 0;
}
void systrace_exit(void)
{
	systrace_detach();

//	printk(KERN_WARNING "systrace driver unloaded.\n");
	misc_deregister(&systrace_dev);
}

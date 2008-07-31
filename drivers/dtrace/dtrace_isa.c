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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

//#pragma ident	"@(#)dtrace_isa.c	1.10	04/12/18 SMI"

# if linux
#define _SYSCALL32
#include "dtrace_linux.h"
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <asm/ucontext.h>
#include <linux/thread_info.h>
#include <linux/stacktrace.h>
# endif

#include <sys/dtrace_impl.h>
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/cmn_err.h>
#include <sys/privregs.h>
#include <sys/sysmacros.h>

typedef struct ucontext ucontext_t;
typedef struct ucontext32 ucontext32_t;
//typedef struct siginfo32 siginfo32_t;
# define siginfo32_t siginfo_t

void *fbt_get_kernel_text_address(void);

/*
 * This is gross knowledge to have to encode here...
 */
extern void _interrupt(void);
extern void _cmntrap(void);
extern void _allsyscalls(void);

extern size_t _interrupt_size;
extern size_t _cmntrap_size;
extern size_t _allsyscalls_size;

/*ARGSUSED*/
void
dtrace_getpcstack(pc_t *pcstack, int pcstack_limit, int aframes,
    uint32_t *ignored)
{
	struct frame *fp = (struct frame *)dtrace_getfp();
	struct frame *nextfp, *minfp, *stacktop;
	int depth = 0;
	int is_intr = 0;
	int on_intr, last = 0;
	uintptr_t pc;
	uintptr_t caller = CPU->cpu_dtrace_caller;
	int	*sp = (int *) fp;
	int (*kernel_text_address)(unsigned long) = fbt_get_kernel_text_address();
	struct thread_info *ti = (unsigned long) sp & ~(THREAD_SIZE - 1);

	minfp = fp;
//printk("fp=====%p\n", fp);
	aframes++;

# define valid_stack_ptr(tinfo, p, size) \
	((char *) p > (char *) tinfo && (char *) p + size < (char *) tinfo + THREAD_SIZE)

# if !defined(CONFIG_FRAME_POINTER)
	/***********************************************/
	/*   Hack:  we'll  just  find something worth  */
	/*   having on the stack.		       */
	/***********************************************/
/*
printk("depth=%d pcstack_limit=%d valid=%d\n", depth, pcstack_limit,
	       valid_stack_ptr(ti, sp, sizeof *sp));
printk("sp=%p ti=%p\n", sp, ti);
*/
	while (depth < pcstack_limit && 
	       valid_stack_ptr(ti, sp, sizeof *sp)) {
		unsigned int *a = *sp++;
//printk("ex: %p %d\n", a, kernel_text_address(a));
		if (kernel_text_address(a))
			pcstack[depth++] = (pc_t) a;
	}
# endif

# if defined(CONFIG_FRAME_POINTER)
	while (depth < pcstack_limit && 
	       sp >= minfp &&	
	       valid_stack_ptr(ti, sp, sizeof *sp)) {
		nextfp = *sp;
		pc = sp[1];
		if (kernel_text_address(pc))
			pcstack[depth++] = (pc_t) pc;
		minfp = nextfp;
		sp = nextfp;
	}
# endif
	while (depth < pcstack_limit)
		pcstack[depth++] = NULL;

}
void
dtrace_getupcstack(uint64_t *pcstack, int pcstack_limit)
{
	volatile uint8_t *flags =
	    (volatile uint8_t *)&cpu_core[CPU->cpu_id].cpuc_dtrace_flags;

	if (*flags & CPU_DTRACE_FAULT)
		return;

	if (pcstack_limit <= 0)
		return;

	*pcstack++ = (uint64_t)current->pid;
	pcstack_limit--;

	if (pcstack_limit <= 0)
		return;

	/***********************************************/
	/*   Linux provides a built in function which  */
	/*   is  good  because  stack walking is arch  */
	/*   dependent.            (save_stack_trace)  */
	/*   					       */
	/*   Unfortunately  this is options dependent  */
	/*   (CONFIG_STACKTRACE) so we cannot use it.  */
	/*   And  its  GPL  anyhow, so we cannot copy  */
	/*   it.				       */
	/*   					       */
	/*   Whats worse is that we might be compiled  */
	/*   with a frame pointer (only on x86-32) so  */
	/*   we have three scenarios to handle.	       */
	/*   					       */
	/*   Following  simple code will work so long  */
	/*   as we have a framepointer.		       */
	/***********************************************/
	{
	unsigned long *sp;
	unsigned long *bos;

//	sp = current->thread.rsp;
	sp = KSTK_ESP(current);
	bos = sp;
//printk("sp=%p limit=%d esp0=%p stack=%p\n", sp, pcstack_limit, current->thread.esp0, current->stack);
//{int i; for (i = 0; i < 32; i++) printk("  [%d] %p %p\n", i, sp + i, sp[i]);}
	while (pcstack_limit-- > 0 &&
	       sp >= bos) {
		if (!validate_ptr(sp))
			break;
		*pcstack++ = sp[1];
//printk("sp=%p limit=%d\n", sp, pcstack_limit);
		if (sp[0] < sp)
			break;
		sp = sp[0];
	}
	}

	while (pcstack_limit-- > 0)
		*pcstack++ = NULL;
}

int
dtrace_getustackdepth(void)
{
# if 0
	klwp_t *lwp = ttolwp(curthread);
	proc_t *p = curproc;
	struct regs *rp;
	uintptr_t pc, sp;
	int n = 0;

	if (lwp == NULL || p == NULL || (rp = lwp->lwp_regs) == NULL)
		return (0);

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_FAULT))
		return (-1);

	pc = rp->r_pc;
	sp = rp->r_fp;

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_ENTRY)) {
		n++;

		if (p->p_model == DATAMODEL_NATIVE)
			pc = dtrace_fulword((void *)rp->r_sp);
		else
			pc = dtrace_fuword32((void *)rp->r_sp);
	}

	n += dtrace_getustack_common(NULL, 0, pc, sp);
	return (n);
# else
	TODO();
	return 0;
# endif

}

/*ARGSUSED*/
void
dtrace_getufpstack(uint64_t *pcstack, uint64_t *fpstack, int pcstack_limit)
{
# if 0
	klwp_t *lwp = ttolwp(curthread);
	proc_t *p = ttoproc(curthread);
	struct regs *rp;
	uintptr_t pc, sp, oldcontext;
	volatile uint8_t *flags =
	    (volatile uint8_t *)&cpu_core[CPU->cpu_id].cpuc_dtrace_flags;
	size_t s1, s2;

	if (lwp == NULL || p == NULL || (rp = lwp->lwp_regs) == NULL)
		return;

	if (*flags & CPU_DTRACE_FAULT)
		return;

	if (pcstack_limit <= 0)
		return;

	*pcstack++ = (uint64_t)p->p_pid;
	pcstack_limit--;

	if (pcstack_limit <= 0)
		return;

	pc = rp->r_pc;
	sp = rp->r_fp;
	oldcontext = lwp->lwp_oldcontext;

	if (p->p_model == DATAMODEL_NATIVE) {
		s1 = sizeof (struct frame) + 2 * sizeof (long);
		s2 = s1 + sizeof (siginfo_t);
	} else {
		s1 = sizeof (struct frame32) + 3 * sizeof (int);
		s2 = s1 + sizeof (siginfo32_t);
	}

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_ENTRY)) {
		*pcstack++ = (uint64_t)pc;
		*fpstack++ = 0;
		pcstack_limit--;
		if (pcstack_limit <= 0)
			return;

		if (p->p_model == DATAMODEL_NATIVE)
			pc = dtrace_fulword((void *)rp->r_sp);
		else
			pc = dtrace_fuword32((void *)rp->r_sp);
	}

	while (pc != 0 && sp != 0) {
		*pcstack++ = (uint64_t)pc;
		*fpstack++ = sp;
		pcstack_limit--;
		if (pcstack_limit <= 0)
			break;

		if (oldcontext == sp + s1 || oldcontext == sp + s2) {
			if (p->p_model == DATAMODEL_NATIVE) {
				ucontext_t *ucp = (ucontext_t *)oldcontext;
				greg_t *gregs = ucp->uc_mcontext.gregs;

				sp = dtrace_fulword(&gregs[REG_FP]);
				pc = dtrace_fulword(&gregs[REG_PC]);

				oldcontext = dtrace_fulword(&ucp->uc_link);
			} else {
				ucontext_t *ucp = (ucontext_t *)oldcontext;
				greg_t *gregs = ucp->uc_mcontext.gregs;

				sp = dtrace_fuword32(&gregs[EBP]);
				pc = dtrace_fuword32(&gregs[EIP]);

				oldcontext = dtrace_fuword32(&ucp->uc_link);
			}
		} else {
			if (p->p_model == DATAMODEL_NATIVE) {
				struct frame *fr = (struct frame *)sp;

				pc = dtrace_fulword(&fr->fr_savpc);
				sp = dtrace_fulword(&fr->fr_savfp);
			} else {
				struct frame32 *fr = (struct frame32 *)sp;

				pc = dtrace_fuword32(&fr->fr_savpc);
				sp = dtrace_fuword32(&fr->fr_savfp);
			}
		}

		/*
		 * This is totally bogus:  if we faulted, we're going to clear
		 * the fault and break.  This is to deal with the apparently
		 * broken Java stacks on x86.
		 */
		if (*flags & CPU_DTRACE_FAULT) {
			*flags &= ~CPU_DTRACE_FAULT;
			break;
		}
	}

	while (pcstack_limit-- > 0)
		*pcstack++ = NULL;
# endif
}

/*ARGSUSED*/
uint64_t
dtrace_getarg(int arg, int aframes)
{
# if 0
	uintptr_t val;
	struct frame *fp = (struct frame *)dtrace_getfp();
	uintptr_t *stack;
	int i;
#if defined(__amd64)
	int inreg = offsetof(struct regs, r_r9) / sizeof (greg_t);
#endif

	for (i = 1; i <= aframes; i++) {
		fp = (struct frame *)(fp->fr_savfp);

		if (fp->fr_savpc == (pc_t)dtrace_invop_callsite) {
#if !defined(__amd64)
			/*
			 * If we pass through the invalid op handler, we will
			 * use the pointer that it passed to the stack as the
			 * second argument to dtrace_invop() as the pointer to
			 * the stack.  When using this stack, we must step
			 * beyond the EIP/RIP that was pushed when the trap was
			 * taken -- hence the "+ 1" below.
			 */
			stack = ((uintptr_t **)&fp[1])[1] + 1;
#else
			/*
			 * In the case of amd64, we will use the pointer to the
			 * regs structure that was pushed when we took the
			 * trap.  To get this structure, we must increment
			 * beyond the frame structure, and then again beyond
			 * the calling RIP stored in dtrace_invop().  If the
			 * argument that we're seeking is passed on the stack,
			 * we'll pull the true stack pointer out of the saved
			 * registers and decrement our argument by the number
			 * of arguments passed in registers; if the argument
			 * we're seeking is passed in regsiters, we can just
			 * load it directly.
			 */
			struct regs *rp = (struct regs *)((uintptr_t)&fp[1] +
			    sizeof (uintptr_t));

			if (arg <= inreg) {
				stack = (uintptr_t *)rp;
			} else {
				stack = (uintptr_t *)(rp->r_rsp);
				arg -= inreg;
			}
#endif
			goto load;
		}

	}

	/*
	 * We know that we did not come through a trap to get into
	 * dtrace_probe() -- the provider simply called dtrace_probe()
	 * directly.  As this is the case, we need to shift the argument
	 * that we're looking for:  the probe ID is the first argument to
	 * dtrace_probe(), so the argument n will actually be found where
	 * one would expect to find argument (n + 1).
	 */
	arg++;

#if defined(__amd64)
	if (arg <= inreg) {
		/*
		 * This shouldn't happen.  If the argument is passed in a
		 * register then it should have been, well, passed in a
		 * register...
		 */
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	}

	arg -= (inreg + 1);
#endif
	stack = (uintptr_t *)&fp[1];

load:
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	val = stack[arg];
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);

	return (val);
# endif
}

/*ARGSUSED*/
int
dtrace_getstackdepth(int aframes)
{
	struct frame *fp = (struct frame *)dtrace_getfp();
	struct frame *nextfp, *minfp, *stacktop;
	int depth = 0;
	int is_intr = 0;
	int on_intr;
	uintptr_t pc;

# if 0
	if ((on_intr = CPU_ON_INTR(CPU)) != 0)
		stacktop = (struct frame *)(CPU->cpu_intr_stack + SA(MINFRAME));
	else
		stacktop = (struct frame *)curthread->t_stk;
	minfp = fp;

	aframes++;

	for (;;) {
		depth++;

		if (is_intr) {
			struct regs *rp = (struct regs *)fp;
			nextfp = (struct frame *)rp->r_fp;
			pc = rp->r_pc;
		} else {
			nextfp = (struct frame *)fp->fr_savfp;
			pc = fp->fr_savpc;
		}

		if (nextfp <= minfp || nextfp >= stacktop) {
			if (on_intr) {
				/*
				 * Hop from interrupt stack to thread stack.
				 */
				stacktop = (struct frame *)curthread->t_stk;
				minfp = (struct frame *)curthread->t_stkbase;
				on_intr = 0;
				continue;
			}
			break;
		}

		is_intr = pc - (uintptr_t)_interrupt < _interrupt_size ||
		    pc - (uintptr_t)_allsyscalls < _allsyscalls_size ||
		    pc - (uintptr_t)_cmntrap < _cmntrap_size;

		fp = nextfp;
		minfp = fp;
	}

	if (depth <= aframes)
		return (0);

	return (depth - aframes);
# endif
}

ulong_t
dtrace_getreg(struct regs *rp, uint_t reg)
{
#if defined(__amd64)
	int regmap[] = {
		REG_GS,		/* GS */
		REG_FS,		/* FS */
		REG_ES,		/* ES */
		REG_DS,		/* DS */
		REG_RDI,	/* EDI */
		REG_RSI,	/* ESI */
		REG_RBP,	/* EBP */
		REG_RSP,	/* ESP */
		REG_RBX,	/* EBX */
		REG_RDX,	/* EDX */
		REG_RCX,	/* ECX */
		REG_RAX,	/* EAX */
		REG_TRAPNO,	/* TRAPNO */
		REG_ERR,	/* ERR */
		REG_RIP,	/* EIP */
		REG_CS,		/* CS */
		REG_RFL,	/* EFL */
		REG_RSP,	/* UESP */
		REG_SS		/* SS */
	};

	if (reg <= SS) {
		if (reg >= sizeof (regmap) / sizeof (int)) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
			return (0);
		}

		reg = regmap[reg];
	} else {
		reg -= SS + 1;
	}

	switch (reg) {
	case REG_RDI:
		return (rp->r_rdi);
	case REG_RSI:
		return (rp->r_rsi);
	case REG_RDX:
		return (rp->r_rdx);
	case REG_RCX:
		return (rp->r_rcx);
	case REG_R8:
		return (rp->r_r8);
	case REG_R9:
		return (rp->r_r9);
	case REG_RAX:
		return (rp->r_rax);
	case REG_RBX:
		return (rp->r_rbx);
	case REG_RBP:
		return (rp->r_rbp);
	case REG_R10:
		return (rp->r_r10);
	case REG_R11:
		return (rp->r_r11);
	case REG_R12:
		return (rp->r_r12);
	case REG_R13:
		return (rp->r_r13);
	case REG_R14:
		return (rp->r_r14);
	case REG_R15:
		return (rp->r_r15);
	case REG_DS:
		return (rp->r_ds);
	case REG_ES:
		return (rp->r_es);
	case REG_FS:
		return (rp->r_fs);
	case REG_GS:
		return (rp->r_gs);
	case REG_TRAPNO:
		return (rp->r_trapno);
	case REG_ERR:
		return (rp->r_err);
	case REG_RIP:
		return (rp->r_rip);
	case REG_CS:
		return (rp->r_cs);
	case REG_SS:
		return (rp->r_ss);
	case REG_RFL:
		return (rp->r_rfl);
	case REG_RSP:
		return (rp->r_rsp);
	default:
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	}

#else
	if (reg > SS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	}

	return ((&rp->r_gs)[reg]);
#endif
}

static int
dtrace_copycheck(uintptr_t uaddr, uintptr_t kaddr, size_t size)
{

	/***********************************************/
	/*   Spot the Sun bug in the line below.       */
	/***********************************************/
//	ASSERT(kaddr >= kernelbase && kaddr + size >= kaddr);

HERE();
	if (!__addr_ok(uaddr) || !__addr_ok(uaddr + size)) {
HERE2();
//printk("uaddr=%p size=%d\n", uaddr, size);
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CPU->cpu_id].cpuc_dtrace_illval = uaddr;
		return (0);
	}

	return (1);
}

void
dtrace_copyin(uintptr_t uaddr, uintptr_t kaddr, size_t size,
    volatile uint16_t *flags)
{
	if (dtrace_copycheck(uaddr, kaddr, size))
		dtrace_copy(uaddr, kaddr, size);
}

void
dtrace_copyout(uintptr_t kaddr, uintptr_t uaddr, size_t size,
    volatile uint16_t *flags)
{
	if (dtrace_copycheck(uaddr, kaddr, size))
		dtrace_copy(kaddr, uaddr, size);
}

void
dtrace_copyinstr(uintptr_t uaddr, uintptr_t kaddr, size_t size,
    volatile uint16_t *flags)
{
	if (dtrace_copycheck(uaddr, kaddr, size))
		dtrace_copystr(uaddr, kaddr, size);
}

void
dtrace_copyoutstr(uintptr_t kaddr, uintptr_t uaddr, size_t size,
    volatile uint16_t *flags)
{
	if (dtrace_copycheck(uaddr, kaddr, size))
		dtrace_copystr(kaddr, uaddr, size);
}

uint8_t
dtrace_fuword8(void *uaddr)
{
	extern uint8_t dtrace_fuword8_nocheck(void *);
	if ((uintptr_t)uaddr >= _userlimit) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
HERE2();
		cpu_core[CPU->cpu_id].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword8_nocheck(uaddr));
}

uint16_t
dtrace_fuword16(void *uaddr)
{
	extern uint16_t dtrace_fuword16_nocheck(void *);
	if ((uintptr_t)uaddr >= _userlimit) {
HERE2();
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CPU->cpu_id].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword16_nocheck(uaddr));
}

uint32_t
dtrace_fuword32(void *uaddr)
{
	extern uint32_t dtrace_fuword32_nocheck(void *);
	if ((uintptr_t)uaddr >= _userlimit) {
HERE2();
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CPU->cpu_id].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword32_nocheck(uaddr));
}

uint64_t
dtrace_fuword64(void *uaddr)
{
	extern uint64_t dtrace_fuword64_nocheck(void *);
	if ((uintptr_t)uaddr >= _userlimit) {
HERE2();
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CPU->cpu_id].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword64_nocheck(uaddr));
}

/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

//#pragma ident	"@(#)dtrace_subr.c	1.9	04/11/19 SMI"

#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <dtrace_proto.h>
#include <sys/fasttrap.h>
#include <sys/rwlock.h>
#include <sys/privregs.h>

# if defined(sun)
#include <sys/x_call.h>
#include <sys/cmn_err.h>
#endif

#include <sys/trap.h>
# if defined(sun)
#include <sys/psw.h>
#include <sys/privregs.h>
#include <sys/machsystm.h>
#include <vm/seg_kmem.h>
# endif

//# define ASSERT(x) {if (!(x)) panic("%s(%d): dtrace assertion failed", __FILE__, __LINE__);}

typedef struct dtrace_invop_hdlr {
	int (*dtih_func)(uintptr_t, uintptr_t *, uintptr_t, trap_instr_t *);
	struct dtrace_invop_hdlr *dtih_next;
} dtrace_invop_hdlr_t;

dtrace_invop_hdlr_t *dtrace_invop_hdlr;

/**********************************************************************/
/*   On a breakpoint trap, see which provider or providers will take  */
/*   the  trap.  Because of our prov provider, we can end up hitting  */
/*   the  same  probe  instruction  from  fbt  and  prov  (or  other  */
/*   providers).  We  need  to  let each provider have a go, not the  */
/*   first provider, to avoid FBT covering up for a later provider.   */
/**********************************************************************/
int
dtrace_invop(uintptr_t addr, uintptr_t *stack, uintptr_t eax, trap_instr_t *tinfo)
{
	dtrace_invop_hdlr_t *hdlr;
	int rval = 0;
static int once = TRUE;

	/***********************************************/
	/*   Let  us see the handler chain, so we can  */
	/*   check  if its in the right logical order  */
	/*   for performance reasons.		       */
	/***********************************************/
	if (once) {
		once = FALSE;
		dtrace_printf("hdlr chain:\n");
		for (hdlr = dtrace_invop_hdlr; hdlr != NULL; hdlr = hdlr->dtih_next) {
			dtrace_printf("  %p\n", hdlr->dtih_func);
		}
	}

	for (hdlr = dtrace_invop_hdlr; hdlr != NULL; hdlr = hdlr->dtih_next) {
		int	ret;
		if ((ret = hdlr->dtih_func(addr, stack, eax, tinfo)) != 0)
			rval = 1;
	}

	return rval;
}

void
dtrace_invop_add(int (*func)(uintptr_t, uintptr_t *, uintptr_t, trap_instr_t *))
{
	dtrace_invop_hdlr_t *hdlr;

	hdlr = kmem_alloc(sizeof (dtrace_invop_hdlr_t), KM_SLEEP);
	hdlr->dtih_func = func;
	hdlr->dtih_next = dtrace_invop_hdlr;
	dtrace_invop_hdlr = hdlr;
}

void
dtrace_invop_remove(int (*func)(uintptr_t, uintptr_t *, uintptr_t, trap_instr_t *))
{
	dtrace_invop_hdlr_t *hdlr = dtrace_invop_hdlr, *prev = NULL;

	for (;;) {
		if (hdlr == NULL)
			panic("attempt to remove non-existent invop handler");

		if (hdlr->dtih_func == func)
			break;

		prev = hdlr;
		hdlr = hdlr->dtih_next;
	}

	if (prev == NULL) {
		ASSERT(dtrace_invop_hdlr == hdlr);
		dtrace_invop_hdlr = hdlr->dtih_next;
	} else {
		ASSERT(dtrace_invop_hdlr != hdlr);
		prev->dtih_next = hdlr->dtih_next;
	}

	kmem_free(hdlr, sizeof (dtrace_invop_hdlr_t));
}

# if 0
int
dtrace_getipl(void)
{
	return (CPU->cpu_pri);
}
#endif

/*ARGSUSED*/
void
dtrace_toxic_ranges(void (*func)(uintptr_t base, uintptr_t limit))
{
# if 0
	extern const uintptr_t _userlimit;
#ifdef __amd64
	extern uintptr_t toxic_addr;
	extern size_t toxic_size;
	extern const uintptr_t _userlimit;

	(*func)(0, _userlimit);

	if (hole_end > hole_start)
		(*func)((uintptr_t)hole_start, (uintptr_t)hole_end);
	(*func)(toxic_addr, toxic_addr + toxic_size);
#else
	extern void *device_arena_contains(void *, size_t, size_t *);
	caddr_t	vaddr;
	size_t	len;

	for (vaddr = (caddr_t)kernelbase; vaddr < (caddr_t)KERNEL_TEXT;
	    vaddr += len) {
		len = (caddr_t)KERNEL_TEXT - vaddr;
		vaddr = device_arena_contains(vaddr, len, &len);
		if (vaddr == NULL)
		    break;
		(*func)((uintptr_t)vaddr, (uintptr_t)vaddr + len);
	}
#endif
	(*func)(0, _userlimit);
#endif
}

# if 0
static int
dtrace_xcall_func(dtrace_xcall_t func, void *arg)
{
	(*func)(arg);

	return (0);
}

/*ARGSUSED*/
void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	cpuset_t set;

	CPUSET_ZERO(set);

	if (cpu == DTRACE_CPUALL) {
		CPUSET_ALL(set);
	} else {
		CPUSET_ADD(set, cpu);
	}

	kpreempt_disable();
	xc_sync((xc_arg_t)func, (xc_arg_t)arg, 0, X_CALL_HIPRI, set,
		(xc_func_t)dtrace_xcall_func);
	kpreempt_enable();
}
# endif

# if 0
void
dtrace_sync_func(void)
{}

void
dtrace_sync(void)
{
	dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
}
# endif

int (*dtrace_fasttrap_probe_ptr)(struct pt_regs *);
int (*dtrace_pid_probe_ptr)(struct pt_regs *);
int (*dtrace_return_probe_ptr)(struct pt_regs *);

/**********************************************************************/
/*   Slight  change  from  Solaris. The trap handler calls us, so we  */
/*   need to let the handler know if we took it or to pass it up the  */
/*   chain.							      */
/**********************************************************************/
int
dtrace_user_probe(int trapno, struct pt_regs *rp, caddr_t addr, processorid_t cpuid)
{
	krwlock_t *rwp;
	proc_t *p;
#if defined(sun)
	extern void trap(struct regs *, caddr_t, processorid_t);
#endif

	par_setup_thread();
	p = curproc;

//	if (USERMODE(rp->r_cs) || (rp->r_ps & PS_VM)) {
HERE();
	if (user_mode(rp) || (rp->r_rfl & PS_VM)) {
# if 0
HERE();
		if (curthread->t_cred != p->p_cred) {
			cred_t *oldcred = curthread->t_cred;
			/*
			 * DTrace accesses t_cred in probe context.  t_cred
			 * must always be either NULL, or point to a valid,
			 * allocated cred structure.
			 */
			curthread->t_cred = crgetcred();
			crfree(oldcred);
		}
# endif
	}

	if (trapno == T_DTRACE_RET) {
		uint8_t step = curthread->t_dtrace_step;
		uint8_t ret = curthread->t_dtrace_ret;
		uintptr_t npc = curthread->t_dtrace_npc;

		if (curthread->t_dtrace_ast) {
			aston(curthread);
			curthread->t_sig_check = 1;
		}

		/*
		 * Clear all user tracing flags.
		 */
		curthread->t_dtrace_ft = 0;

		/*
		 * If we weren't expecting to take a return probe trap, kill
		 * the process as though it had just executed an unassigned
		 * trap instruction.
		 */
		if (step == 0) {
			tsignal(curthread, SIGILL);
			return 1;
		}

		/*
		 * If we hit this trap unrelated to a return probe, we're
		 * just here to reset the AST flag since we deferred a signal
		 * until after we logically single-stepped the instruction we
		 * copied out.
		 */
		if (ret == 0) {
			rp->r_pc = npc;
			return 1;
		}

		/*
		 * We need to wait until after we've called the
		 * dtrace_return_probe_ptr function pointer to set %pc.
		 */
		rwp = &CPU->cpu_ft_lock;
		rw_enter(rwp, RW_READER);
		if (dtrace_return_probe_ptr != NULL)
			(void) (*dtrace_return_probe_ptr)(rp);
		rw_exit(rwp);
		rp->r_pc = npc;
		return 1;

	} else if (trapno == T_DTRACE_PROBE) {
		rwp = &CPU->cpu_ft_lock;
		rw_enter(rwp, RW_READER);
		if (dtrace_fasttrap_probe_ptr != NULL)
			(void) (*dtrace_fasttrap_probe_ptr)(rp);
		rw_exit(rwp);
		return 1;

	} else if (trapno == T_BPTFLT) {
		uint8_t instr;
HERE();

		if (dtrace_pid_probe_ptr == NULL)
			return 0;
		rwp = &CPU->cpu_ft_lock;

		/*
		 * The DTrace fasttrap provider uses the breakpoint trap
		 * (int 3). We let DTrace take the first crack at handling
		 * this trap; if it's not a probe that DTrace knowns about,
		 * we call into the trap() routine to handle it like a
		 * breakpoint placed by a conventional debugger.
		 */
		rw_enter(rwp, RW_READER);
		if (dtrace_pid_probe_ptr != NULL &&
		    (*dtrace_pid_probe_ptr)(rp) == 0) {
HERE_WITH_INFO("Good probe exit!");
			rw_exit(rwp);
			return 1;
		}
		rw_exit(rwp);

		/*
		 * If the instruction that caused the breakpoint trap doesn't
		 * look like an int 3 anymore, it may be that this tracepoint
		 * was removed just after the user thread executed it. In
		 * that case, return to user land to retry the instuction.
		 */
		if (fuword8((void *)(rp->r_pc - 1), &instr) == 0 &&
		    instr != FASTTRAP_INSTR) {
HERE();
printk("instr=%x\n", instr);
			rp->r_pc--;
			return 1;
		}
HERE();

//		trap(rp, addr, cpuid);
		return 0;
	} else {
//		trap(rp, addr, cpuid);
		return 0;
	}
}

# if linux
void
dtrace_safe_synchronous_signal(int x, int y, struct pt_regs *rp)
#else
void
dtrace_safe_synchronous_signal(void)
#endif
{
#if linux
	proc_t *t = par_setup_thread1(current);
#else
	kthread_t *t = curthread;
	struct regs *rp = lwptoregs(ttolwp(t));
	size_t isz = t->t_dtrace_npc - t->t_dtrace_pc;
#endif

#if linux
	/***********************************************/
	/*   We  get  called when delivering a signal  */
	/*   to  any  process,  so need to filter out  */
	/*   redundant calls.			       */
	/***********************************************/
	if (t->t_dtrace_on == 0)
		return;

//printk("sig: pc=%p scr=%p ast=%p\n", rp->r_pc, t->t_dtrace_scrpc, t->t_dtrace_astpc);
#else
	ASSERT(t->t_dtrace_on);
#endif

	/*
	 * If we're not in the range of scratch addresses, we're not actually
	 * tracing user instructions so turn off the flags. If the instruction
	 * we copied out caused a synchonous trap, reset the pc back to its
	 * original value and turn off the flags.
	 */
#if linux
	if (rp->r_pc >= t->t_dtrace_scrpc && rp->r_pc < t->t_dtrace_scrpc + PAGE_SIZE) {
		printk("sigreset to %p\n", (void *) t->t_dtrace_pc);
		rp->r_pc = t->t_dtrace_pc;
	}
#else
	if (rp->r_pc < t->t_dtrace_scrpc ||
	    rp->r_pc > t->t_dtrace_astpc + isz) {
		t->t_dtrace_ft = 0;
	} else if (rp->r_pc == t->t_dtrace_scrpc ||
	    rp->r_pc == t->t_dtrace_astpc) {
		rp->r_pc = t->t_dtrace_pc;
		t->t_dtrace_ft = 0;
	}
#endif
}
#if 0
int
dtrace_safe_defer_signal(void)
{
	kthread_t *t = curthread;
	struct regs *rp = lwptoregs(ttolwp(t));
	size_t isz = t->t_dtrace_npc - t->t_dtrace_pc;

	ASSERT(t->t_dtrace_on);

	/*
	 * If we're not in the range of scratch addresses, we're not actually
	 * tracing user instructions so turn off the flags.
	 */
	if (rp->r_pc < t->t_dtrace_scrpc ||
	    rp->r_pc > t->t_dtrace_astpc + isz) {
		t->t_dtrace_ft = 0;
		return (0);
	}

	/*
	 * If we've executed the original instruction, but haven't performed
	 * the jmp back to t->t_dtrace_npc or the clean up of any registers
	 * used to emulate %rip-relative instructions in 64-bit mode, do that
	 * here and take the signal right away. We detect this condition by
	 * seeing if the program counter is the range [scrpc + isz, astpc).
	 */
	if (t->t_dtrace_astpc - rp->r_pc <
	    t->t_dtrace_astpc - t->t_dtrace_scrpc - isz) {
#ifdef __amd64
		/*
		 * If there is a scratch register and we're on the
		 * instruction immediately after the modified instruction,
		 * restore the value of that scratch register.
		 */
		if (t->t_dtrace_reg != 0 &&
		    rp->r_pc == t->t_dtrace_scrpc + isz) {
			switch (t->t_dtrace_reg) {
			case REG_RAX:
				rp->r_rax = t->t_dtrace_regv;
				break;
			case REG_RCX:
				rp->r_rcx = t->t_dtrace_regv;
				break;
			case REG_R8:
				rp->r_r8 = t->t_dtrace_regv;
				break;
			case REG_R9:
				rp->r_r9 = t->t_dtrace_regv;
				break;
			}
		}
#endif
		rp->r_pc = t->t_dtrace_npc;
		t->t_dtrace_ft = 0;
		return (0);
	}

	/*
	 * Otherwise, make sure we'll return to the kernel after executing
	 * the copied out instruction and defer the signal.
	 */
	if (!t->t_dtrace_step) {
		ASSERT(rp->r_pc < t->t_dtrace_astpc);
		rp->r_pc += t->t_dtrace_astpc - t->t_dtrace_scrpc;
		t->t_dtrace_step = 1;
	}

	t->t_dtrace_ast = 1;

	return (1);
}
# endif


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
#define zone mm_zone
#include <linux/mm.h>
#undef zone
#include "dtrace_linux.h"
#include <linux/sched.h>
#include <asm/ucontext.h>
#include <linux/thread_info.h>
#include <sys/privregs.h>
#include "../port.h"
# define regs pt_regs
# if defined(__arm__)
#	undef HAVE_STACKTRACE_OPS
# endif
# endif

#include <sys/dtrace_impl.h>
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/cmn_err.h>
#include <sys/privregs.h>
#include <sys/sysmacros.h>
# if defined(HAVE_STACKTRACE_OPS)
#	include <asm/stacktrace.h>
# endif
#include <asm/processor.h>
#include "dtrace_proto.h"

typedef struct ucontext ucontext_t;
typedef struct ucontext32 ucontext32_t;
//typedef struct siginfo32 siginfo32_t;
# define siginfo32_t siginfo_t

/*
 * This is gross knowledge to have to encode here...
 */
extern void _interrupt(void);
extern void _cmntrap(void);
extern void _allsyscalls(void);

extern size_t _interrupt_size;
extern size_t _cmntrap_size;
extern size_t _allsyscalls_size;

/**********************************************************************/
/*   We  use  the  kernels  stack  dumper  to  avoid issues with cpu  */
/*   architecture and frame pointer.				      */
/**********************************************************************/
DEFINE_MUTEX(dtrace_stack_mutex);
# if defined(HAVE_STACKTRACE_OPS)
static pc_t	*g_pcstack;
static int	g_pcstack_limit;
static int	g_depth;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 39)
static void
print_trace_warning_symbol(void *data, char *msg, unsigned long symbol)
{
}

static void print_trace_warning(void *data, char *msg)
{
}
#endif

static int print_trace_stack(void *data, char *name)
{
	return 0;
}

static void print_trace_address(void *data, unsigned long addr, int reliable)
{
//printk("in print_trace_address addr=%p\n", addr);
	if (g_depth < g_pcstack_limit)
		g_pcstack[g_depth++] = addr;
}
/**********************************************************************/
/*   For 2.6.39 kernels and above.				      */
/**********************************************************************/

static inline int valid_stack_ptr(struct thread_info *tinfo,
           void *p, unsigned int size, void *end)
{
        void *t = tinfo;
        if (end) {
               if (p < end && p >= (end-THREAD_SIZE))
                      return 1;
               else
                      return 0;
        }
        return p > t && p < t + THREAD_SIZE - size;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
static unsigned long
walk_stack(struct thread_info *tinfo,
               unsigned long *stack, unsigned long bp,
               const struct stacktrace_ops *ops, void *data,
               unsigned long *end, int *graph)
{
        struct stack_frame *frame = (struct stack_frame *)bp;

        while (valid_stack_ptr(tinfo, stack, sizeof(*stack), end)) {
               unsigned long addr;

               addr = *stack;
               if (is_kernel_text(addr)) {
                      if ((unsigned long) stack == bp + sizeof(long)) {
                             ops->address(data, addr, 1);
                             frame = frame->next_frame;
                             bp = (unsigned long) frame;
                      } else {
                             ops->address(data, addr, 0);
                      }
               }
               stack++;
        }
        return bp;   
}
#endif

#endif

/**********************************************************************/
/*   Linux  kernel stacktrace arrangements are painful to use across  */
/*   the  kernel  releases,  its  difficult to get the compile right  */
/*   here, so lets turn it off. 				      */
/**********************************************************************/
# if defined(HAVE_STACKTRACE_OPS)
static const struct stacktrace_ops print_trace_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	.walk_stack = walk_stack,
#else
	.warning = print_trace_warning,
	.warning_symbol = print_trace_warning_symbol,
#endif
	.stack = print_trace_stack,
	.address = print_trace_address,
};
# endif

/**********************************************************************/
/*   Used by the stack() D function to get a stack dump at the point  */
/*   of the probe.						      */
/**********************************************************************/
/*ARGSUSED*/
void
dtrace_getpcstack(pc_t *pcstack, int pcstack_limit, int aframes,
    uint32_t *ignored)
{
	int	depth;
#if !defined(HAVE_STACKTRACE_OPS)
	int	lim;
	/***********************************************/
	/*   This  is  a basic stack walker - we dont  */
	/*   care  about  omit-frame-pointer,  and we  */
	/*   can  have  false positives. We also dont  */
	/*   handle  exception  stacks properly - but  */
	/*   this  is  for  older  kernels, where the  */
	/*   kernel  wont  help  us,  so they may not  */
	/*   have exception stacks anyhow.	       */
	/***********************************************/

	/***********************************************/
	/*   20121125 Lets use this always - it avoid  */
	/*   kernel  specific  issues in the official  */
	/*   stack  walker and will give us a vehicle  */
	/*   later  for adding reliable vs guess-work  */
	/*   stack entries.			       */
	/***********************************************/
	cpu_core_t	*this_cpu = cpu_get_this();
	struct pt_regs *regs = this_cpu->cpuc_regs;
	struct thread_info *context;
	uintptr_t *sp;
	uintptr_t *spend;
	
	/***********************************************/
	/*   For   syscalls,  we  will  have  a  null  */
	/*   cpuc_regs,  since  we dont intercept the  */
	/*   trap,   but   instead  intercept  the  C  */
	/*   syscall function.			       */
	/***********************************************/
	if (regs == NULL)
		sp = (uintptr_t *) &depth;
	else
		sp = (uintptr_t *) regs->r_rsp;

	/***********************************************/
	/*   Daisy  chain the interrupt and any other  */
	/*   stacks.  Limit  ourselves in case of bad  */
	/*   corruptions.			       */
	/***********************************************/
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	depth = 0;
	for (lim = 0; lim < 3 && depth < pcstack_limit; lim++) {
		int	ndepth = depth;
		uintptr_t *prev_esp;

		context = (struct thread_info *) ((unsigned long) sp & (~(THREAD_SIZE - 1)));
		spend = (uintptr_t *) ((unsigned long) sp | (THREAD_SIZE - 1));
		for ( ; depth < pcstack_limit && sp < spend; sp++) {
			if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_FAULT))
				goto end_stack;
			if (*sp && is_kernel_text((unsigned long) *sp)) {
				pcstack[depth++] = *sp;
			}
		}
		if (depth >= pcstack_limit || ndepth == depth)
			break;

		prev_esp = (uintptr_t *) ((char *) context + sizeof(struct thread_info));
		if ((sp = prev_esp) == NULL)
			break;
		/***********************************************/
		/*   Special signal to mark the IRQ stack.     */
		/***********************************************/
		if (depth < pcstack_limit) {
			pcstack[depth++] = 1;
		}
	}
end_stack:
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_FAULT);
#else

	/***********************************************/
	/*   I'm  a  little tired of the kernel dying  */
	/*   in  the  callback, so lets avoid relying  */
	/*   on the kernel stack walker.	       */
	/***********************************************/
	mutex_enter(&dtrace_stack_mutex);
	g_depth = 0;
	g_pcstack = pcstack;
	g_pcstack_limit = pcstack_limit;

#if FUNC_DUMP_TRACE_ARGS == 6
	dump_trace(NULL, NULL, NULL, 0, &print_trace_ops, NULL);
#else
	dump_trace(NULL, NULL, NULL, &print_trace_ops, NULL);
#endif
	depth = g_depth;
	mutex_exit(&dtrace_stack_mutex);
#endif

	while (depth < pcstack_limit)
		pcstack[depth++] = (pc_t) NULL;
}

/**********************************************************************/
/*   Get  user space stack for the probed process. We need to handle  */
/*   32 + 64 bit binaries, if we are on a 64b kernel, and also frame  */
/*   pointer/no frame pointer. We use the DWARF code to walk the CFA  */
/*   frames to tell us where we can find the return address.	      */
/**********************************************************************/
void
dtrace_getupcstack(uint64_t *pcstack, int pcstack_limit)
{	uint64_t *pcstack_end = pcstack + pcstack_limit;
	volatile uint8_t *flags =
	    (volatile uint8_t *)&cpu_core[cpu_get_id()].cpuc_dtrace_flags;
	unsigned long *sp;
	unsigned long *bos;

	if (*flags & CPU_DTRACE_FAULT)
		return;

	if (pcstack_limit <= 0)
		return;

	*pcstack++ = (uint64_t)current->pid;

	if (pcstack >= pcstack_end)
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
	/***********************************************/

	/***********************************************/
	/*   Ye  gods! The world is an awful place to  */
	/*   live. The target process, may or may not  */
	/*   have   frame  pointers.  In  fact,  some  */
	/*   frames  may have it and some may not (eg  */
	/*   different   libraries  may  be  compiled  */
	/*   differently).			       */
	/*   					       */
	/*   Looks like distro owners dont care about  */
	/*   debuggabiity,   and  give  us  no  frame  */
	/*   pointers.				       */
	/*   					       */
	/*   This  function  is  really important and  */
	/*   useful.  On  modern  Linux  systems, gdb  */
	/*   (and  pstack) contain all the smarts. In  */
	/*   fact,  pstack  is often a wrapper around  */
	/*   gdb  -  i.e. its so complex we cannot do  */
	/*   this.				       */
	/***********************************************/

	/***********************************************/
	/*   Bear  in  mind  that  user stacks can be  */
	/*   megabytes  in  size,  vs  kernel  stacks  */
	/*   which  are  limited  to a few K (4 or 8K  */
	/*   typically).			       */
	/***********************************************/

//	sp = current->thread.rsp;
# if defined(__i386)
	bos = sp = KSTK_ESP(current);
#	define	ALIGN_MASK	3
# else
	/***********************************************/
	/*   KSTK_ESP()  doesnt exist for x86_64 (its  */
	/*   set to -1).			       */
	/***********************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
#  if defined(KSTK_EIP)
	/***********************************************/
	/*   Handle  ARM and more kernel independent,  */
	/*   but might not exist.		       */
	/***********************************************/
	bos = sp = (unsigned long *) KSTK_EIP(current);
#  else
	bos = sp = (unsigned long *) task_pt_regs(current)->sp;
#  endif
#else
	bos = sp = task_pt_regs(current)->rsp;
#endif
#	define	ALIGN_MASK	7
#endif

	/***********************************************/
	/*   Walk  the  stack.  We  cannot  rely on a  */
	/*   frame  pointer  at  each  level,  and we  */
	/*   really  want to avoid probing every word  */
	/*   in  the  stack  - a large stack will eat  */
	/*   cpu  looking at thousands of entries. So  */
	/*   try  and  heuristically see if we have a  */
	/*   likely  frame  pointer  to jump over the  */
	/*   frame,  but, if not, just go one word at  */
	/*   a time.				       */
	/*   					       */
	/*   Try  and be careful we dont walk outside  */
	/*   the  stack  or  walk  backwards  in  the  */
	/*   stack, too.			       */
	/***********************************************/
	{uintptr_t *spend = sp + 1024;
	struct vm_area_struct *vma = find_vma(current->mm, (unsigned long) sp);
	if (vma)
		spend = (uintptr_t *) (vma->vm_end - sizeof(int *));

/*printk("xbos=%p %p\n", bos, spend);*/

	/***********************************************/
	/*   Have  you ever looked at the output from  */
	/*   GCC in amd64 mode? Things like:	       */
	/*   					       */
	/*   push %r12				       */
	/*   push %rbp				       */
	/*   					       */
	/*   will make you come out in a cold sweat -  */
	/*   no   way  to  find  the  frame  pointer,  */
	/*   without doing what GDB does (ie read the  */
	/*   DWARF  stack  unwind info). So, for now,  */
	/*   you  get  some  false  positives  in the  */
	/*   output - but we try to be conservative.   */
	/***********************************************/
        while (sp >= bos && sp < spend && validate_ptr(sp)) {
/*printk("  %p %d: %p %d\n", sp, validate_ptr(sp), sp[0], validate_ptr(sp[0]));*/
		if (validate_ptr((void *) sp[0])) {
			uintptr_t p = sp[-1];
			/***********************************************/
			/*   Try  and  avoid false positives in stack  */
			/*   entries   -   we  want  this  to  be  an  */
			/*   executable instruction.		       */
			/***********************************************/
		 	if (((unsigned long *) sp[0] < bos || (unsigned long *) sp[0] > spend) &&
			    (vma = find_vma(current->mm, sp[0])) != NULL &&
			    vma->vm_flags & VM_EXEC) {
				*pcstack++ = sp[0];
				if (pcstack >= pcstack_end)
					break;
			}
			if (((int) p & ALIGN_MASK) == 0 && p > (uintptr_t) sp && p < (uintptr_t) spend)
				sp = (unsigned long *) p;
		}
                sp++;
	}
	}

	/***********************************************/
	/*   Erase  anything  else  in  the buffer to  */
	/*   avoid confusion.			       */
	/***********************************************/
	while (pcstack < pcstack_end)
		*pcstack++ = (pc_t) NULL;
}

int
dtrace_getustackdepth(void)
{
printk("need to do this dtrace_getustackdepth\n");
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

		if (dtrace_data_model(p) == DATAMODEL_NATIVE)
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
printk("need to do this dtrace_getufpstack\n");
# if 0
	klwp_t *lwp = ttolwp(curthread);
	proc_t *p = ttoproc(curthread);
	struct regs *rp;
	uintptr_t pc, sp, oldcontext;
	volatile uint8_t *flags =
	    (volatile uint8_t *)&cpu_core[cpu_get_id()].cpuc_dtrace_flags;
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

	if (dtrace_data_model(p) == DATAMODEL_NATIVE) {
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

		if (dtrace_data_model(p) == DATAMODEL_NATIVE)
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
			if (dtrace_data_model(p) == DATAMODEL_NATIVE) {
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
			if (dtrace_data_model(p) == DATAMODEL_NATIVE) {
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

/**********************************************************************/
/*   20100705  This  is  a  hack  for now to satisfy dtrace_getarg()  */
/*   below.  This  is needed so that we can walk from kernel back to  */
/*   user space.						      */
/*
http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/dev/i386/fbt_x86.c
*/
/**********************************************************************/
void
dtrace_invop_callsite(void)
{
}

/*ARGSUSED*/
uint64_t
dtrace_getarg(int arg, int aframes)
{
	uintptr_t val;
	struct frame *fp;
	uintptr_t *stack;
	int i;

#if defined(__amd64)
	/***********************************************/
	/*   First 6 args in a register.	       */
	/***********************************************/
	int inreg = 5;
	/*int inreg = offsetof(struct regs, r_r9) / sizeof (greg_t);*/
#endif

	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	fp = (struct frame *)dtrace_getfp();
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);
//printk("dtrace_getarg: fp=%p %p\n", fp, fp->fr_savfp);

//printk("arg=%d aframes=%d\n", arg, aframes);

	for (i = 1; i <= aframes; i++) {
		pc_t savpc;
		DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
		fp = (struct frame *)(fp->fr_savfp);
		savpc = fp->fr_savpc;
		DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);
//printk("fp=%p savpc=%p\n", fp, savpc);
		if (savpc == (pc_t)dtrace_invop_callsite) {
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
				stack = (uintptr_t *)&rp->r_rdi;
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
}

/*ARGSUSED*/
int
dtrace_getstackdepth(int aframes)
{
# if 1
	TODO();
	return 0;
# else
	struct frame *fp = (struct frame *)dtrace_getfp();
	struct frame *nextfp, *minfp, *stacktop;
	int depth = 0;
	int is_intr = 0;
	int on_intr;
	uintptr_t pc;

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
dtrace_getreg(struct pt_regs *rp, uint_t reg)
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
# if defined(sun)
	case REG_ERR:
		return (rp->r_err);
# endif
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

#elif defined(__i386)
	if (reg > SS) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	}

	/***********************************************/
	/*   This is likely to be wrong and we should  */
	/*   likely have code like above to avoid the  */
	/*   funnyism of Linux reg layout.	       */
	/***********************************************/
	return ((&rp->r_gs)[reg]);
# elif defined(__arm__)
	return 0;
# else
#	error "dtrace_isa.c: Unsupported CPU architecture"
#endif
}

/*
int dtrace_isa_asm(void)
{
# if defined(__i386)
	int __d0, __d1, __d2;						\
	__asm__ __volatile__(						\
		"	cmp  $7,%0\n"					\
		"	jbe  1f\n"					\
		"	movl %1,%0\n"					\
		"	negl %0\n"					\
		"	andl $7,%0\n"					\
		"	subl %0,%3\n"					\
		"4:	rep; movsb\n"					\
		"	movl %3,%0\n"					\
		"	shrl $2,%0\n"					\
		"	andl $3,%3\n"					\
		"	.align 2,0x90\n"				\
		"0:	rep; movsl\n"					\
		"	movl %3,%0\n"					\
		"1:	rep; movsb\n"					\
		"	clr %0\n"					\
		"2:\n"							\
		".section .fixup,\"ax\"\n"				\
		"5:	mov $0,%0\n"					\
		"	jmp 2b\n"					\
		".previous\n"						\
		".section __ex_table,\"a\"\n"				\
		"	.align 4\n"					\
		"	.long 4b,5b\n"					\
		"	.long 0b,5b\n"					\
		"	.long 1b,5b\n"					\
		".previous"						\
		: "=&c"(size), "=&D" (__d0), "=&S" (__d1), "=r"(__d2)	\
		: "3"(size), "0"(size), "1"(dst), "2"(src)		\
		: "memory");
*/

# define addr_valid(addr) __addr_ok((addr))

# if 0
static int
dtrace_copycheck(uintptr_t uaddr, uintptr_t kaddr, size_t size)
{

if (dtrace_here) {
printk("copycheck: uaddr=%p kaddr=%p size=%d\n", (void *) uaddr, (void*) kaddr, (int) size);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	if (__range_not_ok(uaddr, size)) {
#else
	if (!addr_valid(uaddr) || !addr_valid(uaddr + size)) {
#endif
//printk("uaddr=%p size=%d\n", uaddr, size);
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[cpu_get_id()].cpuc_dtrace_illval = uaddr;
		return (0);
	}
	return (1);
}
# endif

void
dtrace_copyin(uintptr_t uaddr, uintptr_t kaddr, size_t size,
    volatile uint16_t *flags)
{
	if (dtrace_memcpy_with_error((void *) kaddr, (void *) uaddr, size) == 0) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		return;
		}
}

void
dtrace_copyout(uintptr_t kaddr, uintptr_t uaddr, size_t size,
    volatile uint16_t *flags)
{
	if (dtrace_memcpy_with_error((void *) uaddr, (void *) kaddr, size) == 0) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		return;
		}
}

void
dtrace_copyinstr(uintptr_t uaddr, uintptr_t kaddr, size_t size,
    volatile uint16_t *flags)
{
	if (dtrace_memcpy_with_error((void *) kaddr, (void *) uaddr, size) == 0) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		return;
		}
}

void
dtrace_copyoutstr(uintptr_t kaddr, uintptr_t uaddr, size_t size,
    volatile uint16_t *flags)
{
	if (dtrace_memcpy_with_error((void *) kaddr, (void *) uaddr, size) == 0) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		return;
		}
}

uint8_t
dtrace_fuword8(void *uaddr)
{
	extern uint8_t dtrace_fuword8_nocheck(void *);
	if (!access_ok(VERIFY_READ, uaddr, 1)) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		printk("dtrace_fuword8: uaddr=%p CPU_DTRACE_BADADDR\n", uaddr);
		cpu_core[cpu_get_id()].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword8_nocheck(uaddr));
}

uint16_t
dtrace_fuword16(void *uaddr)
{
	extern uint16_t dtrace_fuword16_nocheck(void *);
	if (!access_ok(VERIFY_WRITE, uaddr, 2)) {
		printk("dtrace_fuword16: uaddr=%p CPU_DTRACE_BADADDR\n", uaddr);
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[cpu_get_id()].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword16_nocheck(uaddr));
}

uint32_t
dtrace_fuword32(void *uaddr)
{
	extern uint32_t dtrace_fuword32_nocheck(void *);
	if (!addr_valid(uaddr)) {
HERE2();
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[cpu_get_id()].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword32_nocheck(uaddr));
}

uint64_t
dtrace_fuword64(void *uaddr)
{
	extern uint64_t dtrace_fuword64_nocheck(void *);
	if (!addr_valid(uaddr)) {
HERE2();
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[cpu_get_id()].cpuc_dtrace_illval = (uintptr_t)uaddr;
		return (0);
	}
	return (dtrace_fuword64_nocheck(uaddr));
}

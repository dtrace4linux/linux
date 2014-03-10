/**********************************************************************/
/*   This  file  contains  interrupt  code for the x86/64 processor.  */
/*   Specifically,  we  need  a very low level intercept on the INT3  */
/*   interrupt  vector,  so  that  on  old  kernels, we dont have to  */
/*   blacklist lots of functions, such as timers or kprobes, because  */
/*   we do want to watch them (possibly).			      */
/*   								      */
/*   Later  kernels  support  nested  interrupt  handling, but 2.6.9  */
/*   specifically  does  not,  and  crashes if we hit a probe whilst  */
/*   processing another probe.					      */
/*   								      */
/*   The  goals  are  simple:  if it could be ours, try it, and exit  */
/*   back to the caller, else dispatch to the "int3" function in the  */
/*   main kernel.						      */
/*   								      */
/*   Some  user traps are ignored and just passthru (USDT can invoke  */
/*   user traps we care about so we may need to modify them).	      */
/*   								      */
/*   We  have  to handle Xen PV guest specially since it virtualises  */
/*   interrupts  and  doesnt  present  the same stack layout as real  */
/*   host.							      */
/*   								      */
/*   Author: Paul Fox						      */
/*   								      */
/*   Date: May 2009						      */
/*   $Header: Last edited: 07-Nov-2012 1.3 $ 			      */
/**********************************************************************/

/*
    * 0 - Division by zero exception
    * 1 - Debug exception
    * 2 - Non maskable interrupt
    * 3 - Breakpoint exception
    * 4 - 'Into detected overflow'
    * 5 - Out of bounds exception
    * 6 - Invalid opcode exception
    * 7 - No coprocessor exception
    * 8 - Double fault (pushes an error code)
    * 9 - Coprocessor segment overrun
    * 10 - Bad TSS (pushes an error code)
    * 11 - Segment not present (pushes an error code)
    * 12 - Stack fault (pushes an error code)
    * 13 - General protection fault (pushes an error code)
    * 14 - Page fault (pushes an error code)
    * 15 - Unknown interrupt exception
    * 16 - Coprocessor fault
    * 17 - Alignment check exception
    * 18 - Machine check exception
    * 19-31 - Reserved
*/

# if defined(__amd64)

#include <linux/linkage.h>
#include <asm/segment.h>
#include <asm/calling.h>
#include <asm/asm-offsets.h>
#ifdef CONFIG_PARAVIRT
#	include <asm/paravirt.h>
#endif
#include <linux/version.h>
# if defined(HAVE_INCLUDE_ASM_MSR_INDEX_H)
#   include <asm/msr-index.h>
# else
#   define MSR_GS_BASE	0xc0000101
# endif
# include <asm/page.h>
# include <sys/trap.h>

/**********************************************************************/
/*   If   running   under   Xen/paravirt,   we   need  to  undo  the  */
/*   PARAVIRT_ADJUST_EXCEPTION_FRAME  which  pops two registers from  */
/*   the  stack.  We  need  this  as  we jump to the original kernel  */
/*   handler.							      */
/**********************************************************************/
#ifdef CONFIG_PARAVIRT
	.macro undo_PARAVIRT_ADJUST_EXCEPTION_FRAME
	push    %r11
	push    %rcx
	.endm

	/***********************************************/
	/*   Inline  the  paravirt  call  - so we can  */
	/*   trace the real one.		       */
	/***********************************************/
	#undef PARAVIRT_ADJUST_EXCEPTION_FRAME
        .macro PARAVIRT_ADJUST_EXCEPTION_FRAME
        pop     %rcx
        pop     %r11
        .endm
#else
	.macro undo_PARAVIRT_ADJUST_EXCEPTION_FRAME
	.endm
	#undef PARAVIRT_ADJUST_EXCEPTION_FRAME
        .macro PARAVIRT_ADJUST_EXCEPTION_FRAME
	.endm
#endif

//#undef INTERRUPT_RETURN
//#undef SWAPGS
#if !defined(INTERRUPT_RETURN)
#	define	INTERRUPT_RETURN	iretq
#	define	SWAPGS			swapgs
#endif

# define NOTIFY_DONE	0

.macro FUNCTION name
	.text
	.globl \name
	.type \name, @function
.endm

/**********************************************************************/
/*   Used  to  define  labels which can be accessed by intr.c, so we  */
/*   can stub out the paravirt stuff if we are not in a Xen guest.    */
/**********************************************************************/
.macro DEF_LABEL a, b
	.global \a\b
\a\b:
.endm

.macro INTR_RETURN xen

	.if \xen == 1
	  INTERRUPT_RETURN
	.else
	  iretq
	.endif
.endm

/**********************************************************************/
/*   For  each  interrupt,  we define the handler twice - once for a  */
/*   non-Xen  environment,  and the other for Xen. The difference is  */
/*   that  instructions (iretq, swapgs, and the initial stack frame)  */
/*   are handled by macros from paravirt.h. The swapgs/iretq involve  */
/*   indirect  function  calls  -  enough  complexity that we cannot  */
/*   easily hide from the public header file.			      */
/*   								      */
/*   So,  using  macros, we define each twice, and let intr.c decide  */
/*   which one we are going to call.				      */
/**********************************************************************/
.macro INTERRUPT nr, fault, allow_user, func, handler, kernel_handler
	/***********************************************/
	/*   Non-Xen function - no suffix.	       */
	/***********************************************/
	INTERRUPT2 0, \nr, \fault, \allow_user, \
		\func, , \
		\handler, \kernel_handler

	/***********************************************/
	/*   Xen version - with "_xen" suffix.	       */
	/***********************************************/
	INTERRUPT2 1, \nr, \fault, \allow_user, \
		\func, _xen, \
		\handler, \kernel_handler
.endm

/**********************************************************************/
/*   Wrap all the interrupts into a single macro.		      */
/**********************************************************************/
.macro INTERRUPT2 xen, nr, fault, allow_user, func, suffix, handler, kernel_handler
	FUNCTION \func\suffix
\func\suffix:

	/***********************************************/
	/*   If  running  as  a Xen guest, we need to  */
	/*   adjust the stack so it looks like a real  */
	/*   interrupt.                                */
	/***********************************************/
.if \xen == 1
	PARAVIRT_ADJUST_EXCEPTION_FRAME
.endif

	/***********************************************/
	/*   Some interrupts are for the kernel only.  */
	/*   Just   passthru   the  interrupt  if  it  */
	/*   occurred in user space.		       */
	/*   We  check  the  bottom 3 bits to get the  */
	/*   kernel  segment  selector,  rather  than  */
	/*   compare  with  __KERNEL_CS,  since under  */
	/*   Xen, the kernel cs will be in ring 1 and  */
	/*   not ring 0, so wont compare.	       */
	/***********************************************/
.if \allow_user == 0
	.if \fault == 1
	testl $3,16(%rsp)
	.else
	testl $3,8(%rsp)
	.endif

	jne 3f /* Jump to kernel if not for us. */
.endif
	/***********************************************/
	/*   Ensure  consistent  stack  frame setup -  */
	/*   some   interrupts  have  an  error  code  */
	/*   pushed, others do not. Short circuit the  */
	/*   common  case  where  its a user trap and  */
	/*   not    a    kernel    one,   and   avoid  */
	/*   pushing/popping all the regs.	       */
	/***********************************************/

	/***********************************************/
	/*   Now  save  all  the registers in pt_regs  */
	/*   order.				       */
	/***********************************************/
	PUSH_REGS \xen, \fault

	/***********************************************/
	/*   dtrace_XXX_handler(nr, regs)	       */
	/***********************************************/
	mov %rsp,%rsi
	mov $\nr,%rdi
	call \handler

	cmp $NOTIFY_DONE,%rax
	je 2f // exit_intr

	/***********************************************/
	/*   Not handled - so let kernel have it.      */
	/***********************************************/
	POP_REGS \xen, \fault
	jmp 3f

	/***********************************************/
	/*   We  processed  the  interrupt, so we can  */
	/*   exit back to the caller.		       */
	/***********************************************/
2:
	POP_REGS \xen, \fault
	/***********************************************/
	/*   If  we  are  going home, then we need to  */
	/*   remove   the   error   code.  Note  that  */
	/*   POP_REGS  is  using  negative  logic, to  */
	/*   remove  the  redundant  orig_eax  on the  */
	/*   stack,  but  *here*, we must not do that  */
	/*   as we return after handling the fault.    */
	/***********************************************/
	.if \fault
	add $8,%rsp
	.endif

	INTR_RETURN \xen

	/***********************************************/
	/*   Undo the PARAVIRT_ADJUST_EXCEPTION_FRAME  */
	/***********************************************/
3:
	.if \xen == 1
	undo_PARAVIRT_ADJUST_EXCEPTION_FRAME
	.endif
	jmp *\kernel_handler
.endm

/**********************************************************************/
/*   Macros to pop the registers after taking a fault. Two scenarios  */
/*   to  handle  those  interrupts  which do/dont push an error code  */
/*   onto the stack.						      */
/**********************************************************************/
.macro POP_REGS xen fault
	testl %ebx, %ebx
	jz 4f
	.if \xen == 1
	  SWAPGS
	.else
	  swapgs
	.endif

4:
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rax
	pop %rcx
	pop %rdx
	pop %rsi
	pop %rdi
	.if \fault == 0
	/***********************************************/
	/*   Discard    the    error   code   without  */
	/*   destroying rax.			       */
	/***********************************************/
	add $8,%rsp
	.endif
.endm

/**********************************************************************/
/*   Push  the  registers  on  the  kernel stack, as we just took an  */
/*   exception. Need to do this in struct pt_regs order.	      */
/**********************************************************************/
.macro PUSH_REGS xen fault
	.if \fault == 0
	push %rax // orig_eax - any value will do
	.endif

	cld
	push %rdi
	push %rsi
	push %rdx
	push %rcx
	push %rax
	push %r8
	push %r9
	push %r10
	push %r11
	push %rbx
	push %rbp
	push %r12
	push %r13
	push %r14
	push %r15

	/***********************************************/
	/*   Following  handles a nested interrupt...  */
	/*   either  start  afresh,  or continue with  */
	/*   the  stack  frame from before. EBX tells  */
	/*   us,  after  the handler, that we need to  */
	/*   restore GS or not. 		       */
	/***********************************************/
	xorl %ebx,%ebx
	mov    $MSR_GS_BASE,%ecx
	rdmsr
	test %edx,%edx
	js 5f

	.if \xen == 1
	  SWAPGS
	.else
	  swapgs
	.endif

	incl %ebx
5:
# if defined(pda_data_offset)
	movq %gs:pda_data_offset,%rbp
# endif

.endm

/**********************************************************************/
/*   Single step trap.						      */
/**********************************************************************/
INTERRUPT  1, 0, 0, dtrace_int1, dtrace_int1_handler, kernel_int1_handler

/**********************************************************************/
/*   Breakpoint instruction.					      */
/**********************************************************************/
INTERRUPT  3, 0, 1, dtrace_int3, dtrace_int3_handler, kernel_int3_handler

/**********************************************************************/
/*   Double fault.						      */
/**********************************************************************/
INTERRUPT  8, 1, 0, dtrace_double_fault, dtrace_double_fault_handler, kernel_double_fault_handler

/**********************************************************************/
/*   Segment not present.					      */
/**********************************************************************/
INTERRUPT  11, 1, 0, dtrace_int11, dtrace_int11_handler, kernel_int11_handler

/**********************************************************************/
/*   General protection fault.					      */
/**********************************************************************/
INTERRUPT  13, 1, 0, dtrace_int13, dtrace_int13_handler, kernel_int13_handler

/**********************************************************************/
/*   Page fault.						      */
/**********************************************************************/
INTERRUPT  14, 1, 0, dtrace_page_fault, dtrace_int_page_fault_handler, kernel_page_fault_handler

/**********************************************************************/
/*   T_DTRACE_RET  (0x7f) is invoked by the pid provider when single  */
/*   stepping  a user space trap. I dont think we really need this -  */
/*   we  could  overload  the  INT3  trap,  but  for compliance with  */
/*   Solaris/FreeBSD, lets define it.				      */
/**********************************************************************/
INTERRUPT  T_DTRACE_RET, 1, 0, dtrace_int_dtrace_ret, dtrace_int_dtrace_ret_handler, kernel_int_dtrace_ret_handler

/**********************************************************************/
/*   Handle  the  IPI  interrupt - inter-process subroutine call. We  */
/*   bypass  Linux's  smp_call_function calls since the requirements  */
/*   of  not  being  able to call from an interrupt are incompatible  */
/*   with the Solaris mechanism.				      */
/**********************************************************************/
	FUNCTION dtrace_int_ipi
dtrace_int_ipi:
	PUSH_REGS 0, 0
	//call xcall_slave
	POP_REGS 0, 0
	INTR_RETURN 0

/**********************************************************************/
/*   We  use  the  NMI  interrupt  for IPI code, but only if the IPI  */
/*   interrupt isnt responding -- possibly because the target cpu is  */
/*   blocking  interrupts.  We  have  to be careful since NMI may be  */
/*   used  for  watchdogs  and other things, and we have to know who  */
/*   this NMI is for.						      */
/**********************************************************************/
	FUNCTION dtrace_int_nmi
dtrace_int_nmi:
	PUSH_REGS 0, 0
	call func_smp_processor_id
	incq cnt_nmi1

	/***********************************************/
	/*   HACK  ALERT!  Disable NMIs whilst dtrace  */
	/*   loaded,  since we cannot allow probes on  */
	/*   the NMI call graph.		       */
	/***********************************************/
	POP_REGS 0, 0
	INTR_RETURN 0
	
	/* END HACK */

	cmpb $0,nmi_masks(%rax)
	jz  do_kernel_nmi
	// For us...
	movb $0,nmi_masks(%rax)
	//call xcall_slave
	POP_REGS 0, 0
	INTR_RETURN 0

do_kernel_nmi:
	incq cnt_nmi2
	POP_REGS 0, 0
	jmp *kernel_nmi_handler

//INTERRUPT  0xb0, 0, 1, dtrace_int_ipi, dtrace_int_ipi_handler, iret_addr
iret_instr:
	INTR_RETURN 0

iret_addr: .quad iret_instr

/**********************************************************************/
/*   We  define  mcount  function,  so  that  we  dont call into the  */
/*   kernels  mcount. If we try and probe mcount, we want to see the  */
/*   kernels  calls into it, not our own - which will cause a kernel  */
/*   recursion  panic  if  we let this happen. (Ubuntu seems to have  */
/*   some  kernels  with this turned on for some reason, e.g. Ubuntu  */
/*   8.10 2.6.27 kernels).					      */
/**********************************************************************/
	FUNCTION mcount
mcount:
	retq

	FUNCTION dtrace_memcpy_with_error

/**********************************************************************/
/*   Do  a  memcpy, but let caller know if a fault occurred, so this  */
/*   can  be propagated to the user space app as an invalid address.  */
/*   Ideally  we  want  exactly  the  faulting  address, rather than  */
/*   assuming  the  first byte of the target is the area of problem.  */
/*   Additionally,    we    should   use   an   optimised   memcpy()  */
/*   implementation  using movsb/movsl/movsb to do wide transfers on  */
/*   word aligned entities. We will worry about this another day.     */
/**********************************************************************/

dtrace_memcpy_with_error:
	movq %rdx,%rcx
dt_try:	rep
	movsb
	movq $1, %rax
	/***********************************************/
	/*   If  rcx  is  not zero, then we must have  */
	/*   page  faulted and the movsb was abruptly  */
	/*   terminated.			       */
	/***********************************************/
	cmp $0, %rcx
	jne dt_catch
	retq

dt_catch:	
	mov $0, %rax
 	retq
.section __ex_table,"a"
	.align 8
	.quad dt_try,dt_catch
	.previous

# endif

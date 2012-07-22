/**********************************************************************/
/*   This  file  contains  interrupt  code for the x86/32 processor.  */
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
/*   User traps are redirected directly to the kernel - we dont have  */
/*   an interest in them for now.				      */
/*   								      */
/*   Author: Paul Fox						      */
/*   								      */
/*   Date: May 2009						      */
/*   $Header: Last edited: 29-Dec-2011 1.3 $ 			      */
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

# if defined(__i386)

# include <asm/segment.h>
#include <linux/version.h>
# include <sys/trap.h>

# define NOTIFY_DONE	0

.macro FUNCTION name
	.text
	.globl \name
	.type \name, @function
.endm

/**********************************************************************/
/*   Wrap all the interrupts into a single macro.		      */
/**********************************************************************/
.macro INTERRUPT nr, fault, allow_user, func, handler, kernel_handler
	FUNCTION \func
\func:
	/***********************************************/
	/*   Some interrupts are for the kernel only.  */
	/*   Just   passthru   the  interrupt  if  it  */
	/*   occurred in user space.		       */
	/***********************************************/
.if \allow_user == 0
	.if \fault == 1
	cmp $__KERNEL_CS,8(%esp)
	.else
	cmp $__KERNEL_CS,4(%esp)
	.endif

	je 1f
	jmp *\kernel_handler
.endif

1:

	/***********************************************/
	/*   Now  save  all  the registers in pt_regs  */
	/*   order.				       */
	/***********************************************/
	PUSH_REGS \fault

#if defined(__KERNEL_STACK_CANARY)
	/***********************************************/
	/*   2.6.31 and above define this. If we dont  */
	/*   do  this,  then we get strange user land  */
	/*   segmentation   violations  when  %gs  is  */
	/*   touched/used (or maybe destroyed even).   */
	/***********************************************/
	mov $(__KERNEL_STACK_CANARY),%eax
	mov %eax,%gs
#endif
	
	/***********************************************/
	/*   dtrace_XXX_handler(nr, regs)	       */
	/***********************************************/
	mov %esp,%edx
	mov $\nr,%eax
	call \handler

	cmp $NOTIFY_DONE,%eax
	je 2f // exit_intr

	/***********************************************/
	/*   Not handled - so let kernel have it.      */
	/***********************************************/
	POP_REGS \fault
	jmp *\kernel_handler

	/***********************************************/
	/*   We  processed  the  interrupt, so we can  */
	/*   exit back to the caller.		       */
	/***********************************************/
2:
	POP_REGS \fault
	/***********************************************/
	/*   If  we  are  going home, then we need to  */
	/*   remove   the   error   code.  Note  that  */
	/*   POP_REGS  is  using  negative  logic, to  */
	/*   remove  the  redundant  orig_eax  on the  */
	/*   stack,  but  *here*, we must not do that  */
	/*   as we return after handling the fault.    */
	/***********************************************/
	.if \fault
	add $4,%esp
	.endif
	iret
.endm

/**********************************************************************/
/*   Macros to pop the registers after taking a fault. Two scenarios  */
/*   to  handle  those  interrupts  which do/dont push an error code  */
/*   onto the stack.						      */
/**********************************************************************/

.macro POP_REGS fault

	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp
	pop %eax
	pop %ds
	pop %es
	pop %fs
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	/***********************************************/
	/*   struct  pt_regs  suddenly  got  the  %gs  */
	/*   field,  so  we need to ensure the layout  */
	/*   of the stack agrees.		       */
	/***********************************************/
	pop %gs
#endif
	.if \fault == 0
	pop %eax
	.endif
.endm

/**********************************************************************/
/*   Push  the  registers  on  the  kernel stack, as we just took an  */
/*   exception. Need to do this in struct pt_regs order.	      */
/**********************************************************************/

.macro PUSH_REGS fault
	.if \fault == 0
	push %eax // orig_eax - any value will do
	.endif

	cld
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	/***********************************************/
	/*   struct  pt_regs  suddenly  got  the  %gs  */
	/*   field,  so  we need to ensure the layout  */
	/*   of the stack agrees.		       */
	/***********************************************/
	push %gs
#endif
	push %fs
	push %es
	push %ds
	push %eax
	push %ebp
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx

	// Ensure kernel side segment registers point wherever the
	// kernel expects.
	movl $__USER_DS, %edx
	movl %edx,%ds
	movl %edx,%es

#if defined(__KERNEL_PERCPU)
	movl $__KERNEL_PERCPU, %edx
	movl %edx, %fs
#endif
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
/*   T_DTRACE_RET  (0x80) is invoked by the pid provider when single  */
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
	PUSH_REGS 0
	call xcall_slave
	POP_REGS 0
	iret

/**********************************************************************/
/*   We  use  the  NMI  interrupt  for IPI code, but only if the IPI  */
/*   interrupt isnt responding -- possibly because the target cpu is  */
/*   blocking  interrupts.  We  have  to be careful since NMI may be  */
/*   used  for  watchdogs  and other things, and we have to know who  */
/*   this NMI is for.						      */
/**********************************************************************/
	FUNCTION dtrace_int_nmi
dtrace_int_nmi:
	PUSH_REGS 0
	call func_smp_processor_id
	incl cnt_nmi1
	cmpb $0,nmi_masks(%eax)
	jz  do_kernel_nmi
	// For us...
	movb $0,nmi_masks(%eax)
	call xcall_slave
	POP_REGS 0
	iret

do_kernel_nmi:
	incl cnt_nmi2
	POP_REGS 0
	jmp *kernel_nmi_handler

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
	ret

/**********************************************************************/
/*   Do  a  memcpy, but let caller know if a fault occurred, so this  */
/*   can  be propagated to the user space app as an invalid address.  */
/*   Ideally  we  want  exactly  the  faulting  address, rather than  */
/*   assuming  the  first byte of the target is the area of problem.  */
/*   Additionally,    we    should   use   an   optimised   memcpy()  */
/*   implementation  using movsb/movsl/movsb to do wide transfers on  */
/*   word aligned entities. We will worry about this another day.     */
/**********************************************************************/
	FUNCTION dtrace_memcpy_with_error

dtrace_memcpy_with_error:
	mov 12(%esp),%ecx
	mov 8(%esp),%esi
	mov 4(%esp),%edi
dt_try:	rep; movsb
	mov $1, %eax
	/***********************************************/
	/*   If  rcx  is  not zero, then we must have  */
	/*   page  faulted and the movsb was abruptly  */
	/*   terminated.			       */
	/***********************************************/
	cmp $0, %ecx
	jne dt_catch
	ret

dt_catch:
	mov $0, %eax
	ret
.section __ex_table,"a"
	.align 4
	.long dt_try,dt_catch
	.previous
# endif

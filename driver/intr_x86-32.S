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

# define NOTIFY_DONE	0

.macro FUNCTION name
	.text
	.globl \name
	.type \name, @function
.endm

/**********************************************************************/
/*   Macros to pop the registers after taking a fault. Two scenarios  */
/*   to  handle  those  interrupts  which do/dont push an error code  */
/*   onto the stack.						      */
/**********************************************************************/

.macro POP_REGS
	POP_REGS_NO_FAULT_CODE
	pop %eax
.endm

.macro POP_REGS_NO_FAULT_CODE
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
.endm

/**********************************************************************/
/*   Push  the  registers  on  the  kernel stack, as we just took an  */
/*   exception. Need to do this in struct pt_regs order.	      */
/**********************************************************************/
.macro PUSH_REGS ist
	push %eax // orig_eax

	PUSH_REGS_NO_FAULT_CODE
.endm

.macro PUSH_REGS_NO_FAULT_CODE ist
	cld
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


// Common exit point for a handler trap.
exit_intr:
	POP_REGS
	iret

/**********************************************************************/
/*   Function wired into the IDT for INT3 interrupts.		      */
/**********************************************************************/
	FUNCTION dtrace_int1

dtrace_int1:
	PUSH_REGS
	/***********************************************/
	/*   If this is a userspace INT1 instruction,  */
	/*   just give it to the kernel.	       */
	/***********************************************/
	cmp $__KERNEL_CS, 0x30(%esp)
	jne user_int1

	/***********************************************/
	/*   dtrace_int1_handler(1, regs)	       */
	/***********************************************/
	mov %esp,%edx
	mov $2,%eax
	call dtrace_int1_handler
	cmp $NOTIFY_DONE,%eax
	je exit_intr

	// Not handled - so let kernel have it.
user_int1:
	POP_REGS
	jmp *kernel_int1_handler

/**********************************************************************/
/*   Function wired into the IDT for INT3 interrupts.		      */
/**********************************************************************/
	FUNCTION dtrace_int3

dtrace_int3:
	PUSH_REGS

	/***********************************************/
	/*   If this is a userspace INT3 instruction,  */
	/*   just give it to the kernel.	       */
	/***********************************************/
	cmp $__KERNEL_CS, 0x30(%esp)
	jne user_int3

	/***********************************************/
	/*   dtrace_int3_handler(1, regs)	       */
	/***********************************************/
	mov %esp,%edx
	mov $1,%eax
	call dtrace_int3_handler
	cmp $NOTIFY_DONE,%eax
	je exit_intr

	// Not handled - so let kernel have it.
user_int3:
	POP_REGS
	jmp *kernel_int3_handler

/**********************************************************************/
/*   Interrupt  handler  for  GPF - used to detect us being naughty,  */
/*   else pass on to kernel.					      */
/**********************************************************************/
	FUNCTION dtrace_int11

dtrace_int11:
	PUSH_REGS

	/***********************************************/
	/*   If this is a userspace GPF, just give it  */
	/*   to the kernel.			       */
	/***********************************************/
	cmp $__KERNEL_CS, 0x30(%esp)
	jne user_int11

	/***********************************************/
	/*   dtrace_int11_handler(1, regs)	       */
	/***********************************************/
	mov %esp,%edx
	mov $1,%eax
	call dtrace_int11_handler
	cmp $NOTIFY_DONE,%eax
	je exit_intr

	// Not handled - so let kernel have it.
user_int11:
	POP_REGS
	jmp *kernel_int11_handler

/**********************************************************************/
/*   Interrupt  handler  for  GPF - used to detect us being naughty,  */
/*   else pass on to kernel.					      */
/**********************************************************************/
	FUNCTION dtrace_int13

dtrace_int13:
	PUSH_REGS

	/***********************************************/
	/*   If this is a userspace GPF, just give it  */
	/*   to the kernel.			       */
	/***********************************************/
	cmp $__KERNEL_CS, 0x30(%esp)
	jne user_int13

	/***********************************************/
	/*   dtrace_int13_handler(1, regs)	       */
	/***********************************************/
	mov %esp,%edx
	mov $1,%eax
	call dtrace_int13_handler
	cmp $NOTIFY_DONE,%eax
	je exit_intr

	// Not handled - so let kernel have it.
user_int13:
	POP_REGS
	jmp *kernel_int13_handler

/**********************************************************************/
/*   Function wired into the IDT for Page Faults (14) interrupts.     */
/**********************************************************************/
	FUNCTION dtrace_page_fault

dtrace_page_fault:
	PUSH_REGS_NO_FAULT_CODE DEBUG_STACK

	/***********************************************/
	/*   If this is a userspace INT3 instruction,  */
	/*   just give it to the kernel.	       */
	/***********************************************/
	cmp $__KERNEL_CS, 0x30(%esp)
	jne user_page_fault

	/***********************************************/
	/*   dtrace_page_fault_handler(1, regs)	       */
	/***********************************************/
	mov %esp,%edx
	mov $1,%eax
	call dtrace_page_fault_handler
	cmp $NOTIFY_DONE,%eax
	je exit_intr

	// Not handled - so let kernel have it.
user_page_fault:
	POP_REGS_NO_FAULT_CODE
	jmp *kernel_page_fault_handler

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
	mov %edx,%ecx
0:	rep; movsb
	mov $1, %eax
	ret

1:	mov $0, %eax
2: 	ret
.section __ex_table,"a"
	.align 4
	.long 0b,1b
	.previous
# endif

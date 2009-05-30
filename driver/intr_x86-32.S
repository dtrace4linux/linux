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

# if defined(__i386)

# include <asm/segment.h>

# define NOTIFY_DONE	0

.macro FUNCTION name
	.text
	.globl \name
	.type \name, @function
.endm

.macro POP_REGS
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
	pop %eax
.endm

.macro PUSH_REGS
	cld
	push %eax // orig_eax
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
	movl $__KERNEL_PERCPU, %edx
	movl %edx, %fs
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
	/*   dtrace_int3_handler(1, regs)	       */
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
	PUSH_REGS

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
	POP_REGS
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

# endif

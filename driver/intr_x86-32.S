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


/**********************************************************************/
/*   Function wired into the IDT for INT3 interrupts.		      */
/**********************************************************************/
	.text
	.globl dtrace_int1
	.type dtrace_int1, @function

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
	cmp $0x8001,%eax
	je int1_handled

	// Not handled - so let kernel have it.
user_int1:
	POP_REGS
	jmp *kernel_int1_handler
	iret

int1_handled:
	POP_REGS
	iret

/**********************************************************************/
/*   Function wired into the IDT for INT3 interrupts.		      */
/**********************************************************************/
	.text
	.globl dtrace_int3
	.type dtrace_int3, @function

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
	cmp $0x8001,%eax
	je int3_handled

	// Not handled - so let kernel have it.
user_int3:
	POP_REGS
	jmp *kernel_int3_handler
	iret

int3_handled:
	POP_REGS
	iret

# endif

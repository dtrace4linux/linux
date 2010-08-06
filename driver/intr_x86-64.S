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
/*   User traps are redirected directly to the kernel - we dont have  */
/*   an interest in them for now.				      */
/*   								      */
/*   Author: Paul Fox						      */
/*   								      */
/*   Date: May 2009						      */
/**********************************************************************/

# if defined(__amd64)

# include <asm/segment.h>
# include <asm/calling.h>
# include <linux/version.h>
# if defined(HAVE_INCLUDE_ASM_MSR_INDEX_H)
#   include <asm/msr-index.h>
//#   include <asm/asm-offsets.h>
# else
#   define MSR_GS_BASE	0xc0000101
# endif
# include <asm/page.h>

# define NOTIFY_DONE	0

.macro FUNCTION name
	.text
	.globl \name
	.type \name, @function
.endm

.macro POP_REGS
	testl %ebx, %ebx
	jnz 2f
	swapgs

2:
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
	pop %rax
.endm

/**********************************************************************/
/*   Push  the  registers  on  the  kernel stack, as we just took an  */
/*   exception. Need to do this in struct pt_regs order.	      */
/**********************************************************************/
.macro PUSH_REGS ist
	cld
	push %rax // orig_eax
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
	mov $1,%ebx
	movl $MSR_GS_BASE,%ecx
	rdmsr
	testl %edx,%edx
	js 1f

	swapgs
	xorl %ebx,%ebx
1:
# if defined(pda_data_offset)
	movq %gs:pda_data_offset,%rbp
# endif

.endm

// Common exit point for a handler trap.
exit_intr:
	POP_REGS
	iretq


	FUNCTION dtrace_double_fault
dtrace_double_fault:
	PUSH_REGS DEBUG_STACK
	/***********************************************/
	/*   If this is a exception, just give it to   */
	/*   the kernel.	                       */
	/***********************************************/
	cmp $__KERNEL_CS, CS(%rsp) // 0x88
	jne user_double_fault

	/***********************************************/
	/*   dtrace_int3_handler(1, regs)	       */
	/***********************************************/
	mov %rsp,%rsi
	mov $2,%rdi
	call dtrace_double_fault_handler
	cmp $NOTIFY_DONE,%rax
	je exit_intr

	// Not handled - so let kernel have it.
user_double_fault:
	POP_REGS
	jmp *kernel_double_fault_handler

/**********************************************************************/
/*   Function wired into the IDT for INT3 interrupts.		      */
/**********************************************************************/
	FUNCTION dtrace_int1
dtrace_int1:
	PUSH_REGS DEBUG_STACK
	/***********************************************/
	/*   If this is a userspace INT1 instruction,  */
	/*   just  give  it  to  the  kernel. Dont do  */
	/*   this, else we cannot single step past an  */
	/*   IRET  where  we  took  an interrupt from  */
	/*   user  space.  We  can  handle non-dtrace  */
	/*   step   traps   via   the   return   from  */
	/*   dtrace_int1_handler, below.	       */
	/***********************************************/
	cmp $__KERNEL_CS, CS(%rsp) // 0x88
	jne user_int1

	/***********************************************/
	/*   dtrace_int3_handler(1, regs)	       */
	/***********************************************/
	mov %rsp,%rsi
	mov $2,%rdi
	call dtrace_int1_handler

	cmp $NOTIFY_DONE,%rax
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
	PUSH_REGS DEBUG_STACK

	/***********************************************/
	/*   If this is a userspace INT3 instruction,  */
	/*   just give it to the kernel.	       */
	/***********************************************/
	cmp $__KERNEL_CS, CS(%rsp) // 0x88
	jne user_int3

	/***********************************************/
	/*   dtrace_int3_handler(1, regs)	       */
	/***********************************************/
	mov %rsp,%rsi
	mov $1,%rdi
	call dtrace_int3_handler
	cmp $NOTIFY_DONE,%rax
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
	PUSH_REGS DEBUG_STACK

	/***********************************************/
	/*   If this is a userspace GPF, just give it  */
	/*   to the kernel.			       */
	/***********************************************/
	cmp $__KERNEL_CS, CS(%rsp) // 0x88
	jne user_int13

	/***********************************************/
	/*   dtrace_int3_handler(1, regs)	       */
	/***********************************************/
	mov %rsp,%rsi
	mov $2,%rdi
	call dtrace_int13_handler

	cmp $NOTIFY_DONE,%rax
	je exit_intr

	// Not handled - so let kernel have it.
user_int13:
	POP_REGS
	jmp *kernel_int13_handler

/**********************************************************************/
/*   Function wired into the IDT for page fault handling.	      */
/**********************************************************************/
	FUNCTION dtrace_page_fault
dtrace_page_fault:
	PUSH_REGS DEBUG_STACK
	/***********************************************/
	/*   If this is a userspace INT1 instruction,  */
	/*   just  give  it  to  the  kernel. Dont do  */
	/*   this, else we cannot single step past an  */
	/*   IRET  where  we  took  an interrupt from  */
	/*   user  space.  We  can  handle non-dtrace  */
	/*   step   traps   via   the   return   from  */
	/*   dtrace_page_fault_handler, below.	       */
	/***********************************************/
	cmp $__KERNEL_CS, CS(%rsp) // 0x88
	jne user_page_fault

	/***********************************************/
	/*   dtrace_int3_handler(1, regs)	       */
	/***********************************************/
	mov %rsp,%rsi
	mov $2,%rdi
	call dtrace_page_fault_handler

	cmp $NOTIFY_DONE,%rax
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
0:	rep; movsb
	movq $1, %rax
	retq

1:	mov $0, %rax
2: 	retq
.section __ex_table,"a"
	.align 8
	.quad 0b,1b
	.previous

# endif

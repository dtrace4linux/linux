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
/*   $Header: Last edited: 06-Apr-2011 1.1 $ 			      */
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

#define X86_PLATFORM_IPI_VECTOR         0xed

# define NOTIFY_DONE	0

.macro FUNCTION name
	.text
	.globl \name
	.type \name, @function
.endm

/**********************************************************************/
/*   Wrap all the interrupts into a single macro.		      */
/**********************************************************************/
.macro INTERRUPT nr, fault, ign_user, func, handler, kernel_handler
	FUNCTION \func
\func:
	/***********************************************/
	/*   Some interrupts are for the kernel only.  */
	/*   Just   passthru   the  interrupt  if  it  */
	/*   occurred in user space.		       */
	/***********************************************/
.if \ign_user == 0
	.if \fault == 1
	cmp $__KERNEL_CS,16(%rsp)
	.else
	cmp $__KERNEL_CS,8(%rsp)
	.endif

	je 1f
	jmp *\kernel_handler
.endif
	/***********************************************/
	/*   Ensure  consistent  stack  frame setup -  */
	/*   some   interrupts  have  an  error  code  */
	/*   pushed, others do not. Short circuit the  */
	/*   common  case  where  its a user trap and  */
	/*   not    a    kernel    one,   and   avoid  */
	/*   pushing/popping all the regs.	       */
	/***********************************************/
1:

	/***********************************************/
	/*   Now  save  all  the registers in pt_regs  */
	/*   order.				       */
	/***********************************************/
	PUSH_REGS \fault
	
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
	add $8,%rsp
	.endif
	iretq
.endm

/**********************************************************************/
/*   Macros to pop the registers after taking a fault. Two scenarios  */
/*   to  handle  those  interrupts  which do/dont push an error code  */
/*   onto the stack.						      */
/**********************************************************************/
.macro POP_REGS fault
	testl %ebx, %ebx
	jz 3f
	swapgs

3:
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
	pop %rax
	.endif
.endm
/**********************************************************************/
/*   Push  the  registers  on  the  kernel stack, as we just took an  */
/*   exception. Need to do this in struct pt_regs order.	      */
/**********************************************************************/
.macro PUSH_REGS fault
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
	js 4f
//	testl $3,CS(%rsp)
//	je 4f

	swapgs
	incl %ebx
4:
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
INTERRUPT  3, 0, 0, dtrace_int3, dtrace_int3_handler, kernel_int3_handler

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
	iretq

//INTERRUPT  0xb0, 0, 1, dtrace_int_ipi, dtrace_int_ipi_handler, iret_addr
iret_instr:
	iretq
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

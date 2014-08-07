#include <linux/linkage.h>
#include <asm/segment.h>
#include <asm/cache.h>
#include <asm/errno.h>
#include <asm/calling.h>
#include <asm/asm-offsets.h>
#include <asm/unistd.h>
#include <asm/thread_info.h>
#include <asm/paravirt.h>
#include <asm/ftrace.h>
#include <asm/percpu.h>
#include <asm/asm.h>

.macro FUNCTION name
	.text
	.globl \name
	.type \name, @function
.endm


/**********************************************************************/
/*   Where  old_rsp  is  in  my  kernel  - but we cannot use it in a  */
/*   module. Not sure why, but we can get it indirectly.	      */
/**********************************************************************/
//#define old_rsp 0xbfc0

	.macro fork_like func
	/***********************************************/
	/*   3.7  and above moved away from the ptreg  */
	/*   structure,  but we need to marry up with  */
	/*   the assembler for stub_clone.	       */
	/***********************************************/
	popq %r11
	sub    $0x30,%rsp
	mov    %rbx,0x28(%rsp)
	mov    %rbp,0x20(%rsp)
	mov    %r12,0x18(%rsp)
	mov    %r13,0x10(%rsp)
	mov    %r14,0x8(%rsp)
	mov    %r15,(%rsp)
	push   %r11

	/***********************************************/
	/*   Do  an  indirect  fetch  of  the old_rsp  */
	/*   variable in the %gsp segment.	       */
	/***********************************************/
	//movq %gs:0xbfc0,%r11
	movq old_rsp_37,%r11
	movq  %gs:(%r11) ,%r11

	mov    %r11,0xa0(%rsp)	// RSP
	movq   $__USER_DS,0xa8(%rsp)
	movq   $__USER_CS,0x90(%rsp)
	movq   $-1,0x60(%rsp)	// RCX
	mov    0x38(%rsp),%r11
	mov    %r11,0x98(%rsp)

	call \func
		// child does not get here

	mov    0xa0(%rsp),%r11

	/***********************************************/
	/*   We  need  to  do  an  indirect  fetch of  */
	/*   old_rsp in the %gs segment.	       */
	/***********************************************/
	push %rax
	movq old_rsp_37,%rax
	mov    %r11,%gs:(%rax)
	pop %rax

	mov    0x98(%rsp),%r11
	mov    %r11,0x38(%rsp)
	ret $0x30

	.endm

	FUNCTION systrace_part1_sys_clone
systrace_part1_sys_clone:
	fork_like dtrace_systrace_syscall_clone

	FUNCTION systrace_part1_sys_fork
systrace_part1_sys_fork:
	fork_like dtrace_systrace_syscall_fork

	FUNCTION systrace_part1_sys_vfork
systrace_part1_sys_vfork:
	fork_like dtrace_systrace_syscall_vfork

	FUNCTION systrace_part1_sys_iopl
systrace_part1_sys_iopl:
	/***********************************************/
	/*   Do  an  indirect  fetch  of  the old_rsp  */
	/*   variable in the %gsp segment.	       */
	/***********************************************/
	//movq %gs:0xbfc0,%r11
	movq old_rsp_37,%r11
	movq  %gs:(%r11) ,%r11

	mov    %r11,0x70(%rsp)	// RSP
	movq   $__USER_DS,0x78(%rsp)
	movq   $__USER_CS,0x60(%rsp)
	movq   $-1,0x30(%rsp)	// RCX
	mov    0x8(%rsp),%r11
	mov    %r11,0x68(%rsp)

	call dtrace_systrace_syscall_iopl

	mov    0x70(%rsp),%r11

	/***********************************************/
	/*   We  need  to  do  an  indirect  fetch of  */
	/*   old_rsp in the %gs segment.	       */
	/***********************************************/
	push %rax
	movq old_rsp_37,%rax
	mov    %r11,%gs:(%rax)
	pop %rax

	mov    0x68(%rsp),%r11
	mov    %r11,0x8(%rsp)
	retq



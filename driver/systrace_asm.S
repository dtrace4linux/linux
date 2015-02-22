/* 
  Author: Paul D Fox
  Date: July 2014

  File to contain assembler code to compliment systrace.c, due to changing
  calling sequences in the 'ptreg' syscalls.
*/

#include <linux/linkage.h>
#include <linux/version.h>
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
/*   This  comes  from  the  build  script  (tools/kcore.c)  to find  */
/*   old_rsp							      */
/**********************************************************************/
#if defined(old_rsp_val)
old_rsp2: .quad old_rsp_val
#	define	old_rsp_37 old_rsp2
#endif

# if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
/**********************************************************************/
/*   Only  do  this  for new kernerls. Older kernels seem to compile  */
/*   and run fine.						      */
/**********************************************************************/

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
	/*   variable in the %gs segment.	       */
	/***********************************************/
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

	FUNCTION systrace_part1_sys_execve
systrace_part1_sys_execve:
	add	$8,%rsp
	sub	$0x30,%rsp
	mov    %rbx,0x28(%rsp)
	mov    %rbp,0x20(%rsp)
	mov    %r12,0x18(%rsp)
	mov    %r13,0x10(%rsp)
	mov    %r14,0x8(%rsp)
	mov    %r15,(%rsp)
	mov old_rsp_37,%r11
	mov    %gs:(%r11),%r11
	mov    %r11,0x98(%rsp)

	movq   $__USER_DS,0xa0(%rsp)
	movq   $__USER_CS,0x88(%rsp)

	movq   $-1,0x58(%rsp)
	mov    0x30(%rsp),%r11
	mov    %r11,0x90(%rsp)

	call dtrace_systrace_syscall_execve
#if 0
mov 0x98(%rsp),%r11
mov old_rsp_37,%r13
mov %r11,%gs:(%r13)
mov 0x90(%rsp),%r11
mov %r11,0x30(%rsp)
#endif

	mov    %rax,0x50(%rsp)
	mov    (%rsp),%r15
	mov    0x8(%rsp),%r14
	mov    0x10(%rsp),%r13
	mov    0x18(%rsp),%r12
	mov    0x20(%rsp),%rbp
	mov    0x28(%rsp),%rbx
	add    $0x30,%rsp
	jmpq   *int_ret_from_sys_call_ptr

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
	/*   variable in the %gs segment.	       */
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

	FUNCTION systrace_part1_sys_sigaltstack
systrace_part1_sys_sigaltstack:
	/***********************************************/
	/*   Do  an  indirect  fetch  of  the old_rsp  */
	/*   variable in the %gs segment.	       */
	/***********************************************/
	movq old_rsp_37,%rdx
	movq  %gs:(%rdx),%rdx

	call dtrace_systrace_syscall_sigaltstack
	retq

	/***********************************************/
	/*   rt_sigreturn  is  special  (according to  */
	/*   the  Linux kernel comments), because all  */
	/*   registers need to be restored.	       */
	/***********************************************/
	FUNCTION systrace_part1_sys_rt_sigreturn
systrace_part1_sys_rt_sigreturn:

	add $8,%rsp
	sub $0x30,%rsp
	mov %rbx,0x28(%rsp)
	mov %rbp,0x20(%rsp)
	mov %r12,0x18(%rsp)
	mov %r13,0x10(%rsp)
	mov %r14,0x8(%rsp)
	mov %r15,(%rsp)

	movq	old_rsp_37,%r11
	movq 	%gs:(%r11),%r11
	mov	%r11,0x98(%rsp)

	movq	$__USER_DS,0xa0(%rsp)
	movq	$__USER_CS,0x88(%rsp)
	movq	$-1,0x58(%rsp)
	call dtrace_systrace_syscall_rt_sigreturn
//	call *sys_rt_sigreturn_ptr
	mov %rax,0x50(%rsp)
	mov 8(%rsp),%r14
	mov 0x10(%rsp),%r13
	mov 0x18(%rsp),%r12
	mov 0x20(%rsp),%rbp
	mov 0x28(%rsp),%rbx
	add $0x30,%rsp

	jmp *int_ret_from_sys_call_ptr


# endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0) */


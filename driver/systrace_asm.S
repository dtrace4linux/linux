/* 
  Author: Paul D Fox
  Date: July 2014

  File to contain assembler code to compliment systrace.c, due to changing
  calling sequences in the 'ptreg' syscalls.
*/

#include <linux/linkage.h>
#include <linux/version.h>
#include "../port.h"

# if defined(__amd64)
	/***********************************************/
	/*   We  do x86_64 here - the i386 code is in  */
	/*   systrace.c.  Since  Linux  dropped  i386  */
	/*   support, we hopefully dont need to worry  */
	/*   about future i386 changes. Joy !	       */
	/***********************************************/
#include <asm/segment.h>
#include <asm/cache.h>
#include <asm/errno.h>
/*#include <asm/calling.h>*/
#include <asm/asm-offsets.h>
#include <asm/unistd.h>
#include <asm/thread_info.h>

.macro FUNCTION name
	.text
	.globl \name
	.type \name, @function
.endm

/**********************************************************************/
/*   This  comes  from  the  build  script  (tools/kcore.c)  to find  */
/*   old_rsp							      */
/**********************************************************************/
#if !defined(OLD_RSP_VAL)
#	error "OLD_RSP_VAL is not defined - cannot continue"
#endif

/**********************************************************************/
/*   Only  do  this  for new kernerls. Older kernels seem to compile  */
/*   and run fine.						      */
/**********************************************************************/

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

	movq  %gs:OLD_RSP_VAL,%r11

	mov    %r11,0xa0(%rsp)	// RSP
	movq   $__USER_DS,0xa8(%rsp)
	movq   $__USER_CS,0x90(%rsp)
	movq   $-1,0x60(%rsp)	// RCX
	mov    0x38(%rsp),%r11
	mov    %r11,0x98(%rsp)

	call \func
		// child does not get here

	mov    0xa0(%rsp),%r11
	mov    %r11,%gs:OLD_RSP_VAL

	mov    0x98(%rsp),%r11
	mov    %r11,0x38(%rsp)
	ret $0x30

	.endm

# if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
	FUNCTION systrace_part1_sys_clone
systrace_part1_sys_clone:
	fork_like dtrace_systrace_syscall_clone
# endif

	FUNCTION systrace_part1_sys_execve
systrace_part1_sys_execve:
	add	$8,%rsp
	sub	$0x30,%rsp
	mov	%rbx,0x28(%rsp)
	mov	%rbp,0x20(%rsp)
	mov	%r12,0x18(%rsp)
	mov	%r13,0x10(%rsp)
	mov	%r14,0x8(%rsp)
	mov	%r15,(%rsp)
	mov	%gs:OLD_RSP_VAL,%r11
	mov	%r11,0x98(%rsp)

	movq	$__USER_DS,0xa0(%rsp)
	movq	$__USER_CS,0x88(%rsp)

	movq   $-1,0x58(%rsp)
	mov    0x30(%rsp),%r11
	mov    %r11,0x90(%rsp)

	// Linux 2.6.25 or earlier copies regs to %rcx
	// Linux 3.4+ doesnt need this. Dont think it matters that we corrupt rcx
//	mov %rsp,%rcx

//call *sys_execve_ptr
	call dtrace_systrace_syscall_execve

	/***********************************************/
	/*   Do  the syscall::execve:return here - do  */
	/*   not   merge   the   functions  together,  */
	/*   because of the stack sensitivity.	       */
	/***********************************************/
	mov $__NR_execve,%rdi
	mov %rax,%rsi
	call dtrace_systrace_return
#if 1
mov 0x98(%rsp),%r11
mov %r11,%gs:OLD_RSP_VAL
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

# if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
	FUNCTION systrace_part1_sys_fork
systrace_part1_sys_fork:
	fork_like dtrace_systrace_syscall_fork

	FUNCTION systrace_part1_sys_vfork
systrace_part1_sys_vfork:
	fork_like dtrace_systrace_syscall_vfork

	FUNCTION systrace_part1_sys_iopl
systrace_part1_sys_iopl:
	movq  %gs:OLD_RSP_VAL,%r11

	mov    %r11,0x70(%rsp)	// RSP
	movq   $__USER_DS,0x78(%rsp)
	movq   $__USER_CS,0x60(%rsp)
	movq   $-1,0x30(%rsp)	// RCX
	mov    0x8(%rsp),%r11
	mov    %r11,0x68(%rsp)

	call dtrace_systrace_syscall_iopl

	mov    0x70(%rsp),%r11
	mov    %r11,%gs:OLD_RSP_VAL
	mov    0x68(%rsp),%r11
	mov    %r11,0x8(%rsp)
	retq

	FUNCTION systrace_part1_sys_sigaltstack
systrace_part1_sys_sigaltstack:
	movq  %gs:OLD_RSP_VAL,%rdx

	call dtrace_systrace_syscall_sigaltstack
	retq
#endif

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

	mov %rsp,%rdi

	movq 	%gs:OLD_RSP_VAL,%r11
	mov	%r11,0x98(%rsp)

	movq	$__USER_DS,0xa0(%rsp)
	movq	$__USER_CS,0x88(%rsp)
	movq	$-1,0x58(%rsp)
mov 0x30(%rsp),%r11
mov %r11,0x90(%rsp)
	call dtrace_systrace_syscall_rt_sigreturn
	mov %rax,0x50(%rsp)
	mov (%rsp),%r15
	mov 8(%rsp),%r14
	mov 0x10(%rsp),%r13
	mov 0x18(%rsp),%r12
	mov 0x20(%rsp),%rbp
	mov 0x28(%rsp),%rbx
	add $0x30,%rsp

	jmp *int_ret_from_sys_call_ptr

# endif /* defined(__amd64) */


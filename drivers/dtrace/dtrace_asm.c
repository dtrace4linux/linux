/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)dtrace_asm.s	1.5	04/11/17 SMI"

#include <linux_types.h>

int
dtrace_getfp(void)
{

	__asm(
#if defined(__amd64)
		"movq	%rbp, %rax\n"
		"ret"

#elif defined(__i386)

		"movl	%ebp, %eax\n"
		"ret"

#endif	/* __i386 */
	);
}

#if defined(__amd64)
uint32_t
dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new)
{
	__asm(
		"movl	%esi, %eax\n"
		"lock\n"
		"cmpxchgl %edx, (%rdi)\n"
		"ret\n"
		);
}

void *
dtrace_casptr(void *target, void *cmp, void *new)
{
	__asm(
		"movq	%rsi, %rax\n"
		"lock\n"
		"cmpxchgq %rdx, (%rdi)\n"
		"ret\n"
		);
}

#elif defined(__i386)
uint32_t
dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new)
{
	__asm(
		" movl	4(%esp), %edx\n"
		" movl	8(%esp), %eax\n"
		" movl	12(%esp), %ecx\n"
		" lock\n"
		" cmpxchgl %ecx, (%edx)\n"
		" ret\n"
		);
}
void *
dtrace_casptr(void *target, void *cmp, void *new)
{
	__asm(
		" movl	4(%esp), %edx\n"
		" movl	8(%esp), %eax\n"
		" movl	12(%esp), %ecx\n"
		" lock\n"
		" cmpxchgl %ecx, (%edx)\n"
		" ret\n"
		);
}

#endif	/* __i386 */

long
dtrace_caller()
{

	__asm(
#if defined(__amd64)
	
		"movq	$-1, %rax\n"
		"ret\n"

#elif defined(__i386)

		"movl	$-1, %eax\n"
		"ret\n"

#endif	/* __i386 */
		);
}

/*ARGSUSED*/
void
dtrace_copy(uintptr_t src, uintptr_t dest, size_t size)
{
	__asm(
#if defined(__amd64)

		"pushq	%rbp\n"
		"movq	%rsp, %rbp\n"

		"xchgq	%rdi, %rsi\n"		/* make %rsi source, %rdi dest */
		"movq	%rdx, %rcx\n"		/* load count */
		"repz\n"				/* repeat for count ... */
		"smovb\n"				/*   move from %ds:rsi to %ed:rdi */
		"leave\n"
		"ret\n"

#elif defined(__i386)

		"pushl	%ebp\n"
		"movl	%esp, %ebp\n"
		"pushl	%esi\n"
		"pushl	%edi\n"

		"movl	8(%ebp), %esi\n"	// Load source address
		"movl	12(%ebp), %edi\n"	// Load destination address
		"movl	16(%ebp), %ecx\n"	// Load count
		"repz\n"			// Repeat for count...
		"smovb\n"			//   move from %ds:si to %es:di

		"popl	%edi\n"
		"popl	%esi\n"
		"movl	%ebp, %esp\n"
		"popl	%ebp\n"
		"ret\n"

#endif	/* __i386 */
	);
}

/*ARGSUSED*/
void
dtrace_copystr(uintptr_t uaddr, uintptr_t kaddr, size_t size)
{

	__asm(
#if defined(__amd64)

		"pushq	%rbp\n"
		"movq	%rsp, %rbp\n"

"0:\n"
		"movb	(%rdi), %al\n"		/* load from source */
		"movb	%al, (%rsi)\n"		/* store to destination */
		"addq	$1, %rdi\n"		/* increment source pointer */
		"addq	$1, %rsi\n"		/* increment destination pointer */
		"subq	$1, %rdx\n"		/* decrement remaining count */
		"cmpb	$0, %al\n"
		"je	1f\n"
		"cmpq	$0, %rdx\n"
		"jne	0b\n"
"1:\n"
		"leave\n"
		"ret\n"

#elif defined(__i386)

		"pushl	%ebp\n"			// Setup stack frame
		"movl	%esp, %ebp\n"
		"pushl	%ebx\n"			// Save registers

		"movl	8(%ebp), %ebx\n"	// Load source address
		"movl	12(%ebp), %edx\n"	// Load destination address
		"movl	16(%ebp), %ecx\n"	// Load count

"0:\n"
		"movb	(%ebx), %al\n"		// Load from source
		"movb	%al, (%edx)\n"		// Store to destination
		"incl	%ebx\n"			// Increment source pointer
		"incl	%edx\n"			// Increment destination pointer
		"decl	%ecx\n"			// Decrement remaining count
		"cmpb	$0, %al\n"
		"je	1f\n"
		"cmpl	$0, %ecx\n"
		"jne	0b\n"

"1:\n"
		"popl	%ebx\n"
		"movl	%ebp, %esp\n"
		"popl	%ebp\n"
		"ret\n"

#endif	/* __i386 */
		);
}

/*ARGSUSED*/
uintptr_t
dtrace_fulword(void *addr)
{

	__asm(
#if defined(__amd64)

		"movq	(%rdi), %rax\n"
		"ret\n"

#elif defined(__i386)

		"movl	4(%esp), %ecx\n"
		"xorl	%eax, %eax\n"
		"movl	(%ecx), %eax\n"
		"ret\n"

#endif	/* __i386 */
		);
}

/*ARGSUSED*/
uint8_t
dtrace_fuword8_nocheck(void *addr)
{
	__asm(
#if defined(__amd64)

		"xorq	%rax, %rax\n"
		"movb	(%rdi), %al\n"
		"ret\n"

#elif defined(__i386)

		"movl	4(%esp), %ecx\n"
		"xorl	%eax, %eax\n"
		"movzbl	(%ecx), %eax\n"
		"ret\n"

#endif	/* __i386 */
		);
}

/*ARGSUSED*/
uint16_t
dtrace_fuword16_nocheck(void *addr)
{
	__asm(
#if defined(__amd64)

		"xorq	%rax, %rax\n"
		"movw	(%rdi), %ax\n"
		"ret\n"

#elif defined(__i386)

		"movl	4(%esp), %ecx\n"
		"xorl	%eax, %eax\n"
		"movzwl	(%ecx), %eax\n"
		"ret\n"

#endif	/* __i386 */
		);
}

/*ARGSUSED*/
uint32_t
dtrace_fuword32_nocheck(void *addr)
{
	__asm(
#if defined(__amd64)

		"xorq	%rax, %rax\n"
		"movl	(%rdi), %eax\n"
		"ret\n"

#elif defined(__i386)

		"movl	4(%esp), %ecx\n"
		"xorl	%eax, %eax\n"
		"movl	(%ecx), %eax\n"
		"ret\n"

#endif	/* __i386 */
	);
}

/*ARGSUSED*/
uint64_t
dtrace_fuword64_nocheck(void *addr)
{
	__asm(
#if defined(__amd64)

		"movq	(%rdi), %rax\n"
		"ret\n"

#elif defined(__i386)

		"movl	4(%esp), %ecx\n"
		"xorl	%eax, %eax\n"
		"xorl	%edx, %edx\n"
		"movl	(%ecx), %eax\n"
		"movl	4(%ecx), %edx\n"
		"ret\n"

#endif	/* __i386 */
		);
}
void
dtrace_interrupt_enable()
{
	__asm(
#if defined(__amd64)
	        "pushq   %rdi\n"
	        "popfq\n"

#elif defined(__i386)
		"movl	4(%esp), %eax\n"
		"push %eax\n"
		"popf\n"
#endif
	);
}

typedef int dtrace_state_t;
typedef int dtrace_epid_t;
/*ARGSUSED*/
void
dtrace_probe_error(dtrace_state_t *state, dtrace_epid_t epid, int which,
    int fault, int fltoffs, uintptr_t illval)
{
	__asm(
#if defined(__amd64)

		"pushq	%rbp\n"
		"movq	%rsp, %rbp\n"
		"subq	$0x8, %rsp\n"
		"movq	%r9, (%rsp)\n"
		"movq	%r8, %r9\n"
		"movq	%rcx, %r8\n"
		"movq	%rdx, %rcx\n"
		"movq	%rsi, %rdx\n"
		"movq	%rdi, %rsi\n"
		"movl	dtrace_probeid_error(%rip), %edi\n"
		"call	dtrace_probe\n"
		"addq	$0x8, %rsp\n"
		"leave\n"
		"ret\n"

#elif defined(__i386)

		"pushl	%ebp\n"
		"movl	%esp, %ebp\n"
		"pushl	0x1c(%ebp)\n"
		"pushl	0x18(%ebp)\n"
		"pushl	0x14(%ebp)\n"
		"pushl	0x10(%ebp)\n"
		"pushl	0xc(%ebp)\n"
		"pushl	0x8(%ebp)\n"
		"pushl	dtrace_probeid_error\n"
		"call	dtrace_probe\n"
		"movl	%ebp, %esp\n"
		"popl	%ebp\n"
		"ret\n"

#endif	/* __i386 */
	);
}

void dtrace_membar_consumer(void)
{
	/* use 2 byte return instruction when branch target */
        /* AMD Software Optimization Guide - Section 6.2 */
	__asm(
		"rep\n"
		"ret\n"
		);
}
void dtrace_membar_producer(void)
{
	/* use 2 byte return instruction when branch target */
        /* AMD Software Optimization Guide - Section 6.2 */
	__asm(
		"rep\n"
		"ret\n"
		);
}
long
dtrace_interrupt_disable()
{
	__asm(
#if defined(__amd64)
        	"pushfq\n"
        	"popq    %rax\n"
        	"cli\n"
        	"ret\n"
#elif defined(__i386)
		"pushf\n"
		"pop %eax\n"
		"cli\n"
		"ret\n"
# endif
	);
}

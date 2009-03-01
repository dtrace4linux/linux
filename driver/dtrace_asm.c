/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

//#pragma ident	"@(#)dtrace_asm.s	1.5	04/11/17 SMI"

#include <linux_types.h>
//#include <asm/cmpxchg_32.h>

# define	SWAP_REG(a, b) \
	"push " #a "\n" \
	"push " #b "\n" \
	"pop  " #a "\n" \
	"pop  " #b "\n"


int
dtrace_getfp(void)
{

	__asm(
#if defined(__amd64)
		"movq	%rbp, %rax\n"
		"ret"

#elif defined(__i386)

# if defined(CONFIG_FRAME_POINTER)
		"movl	%ebp, %eax\n"
		"ret"
# else
		"movl	(%esp), %eax\n"
		"ret"
# endif

#endif	/* __i386 */
	);
	return 0; // notreached
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
	return 0; // notreached
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
	return 0; // notreached
}

#elif defined(__i386)
/**********************************************************************/
/*   Watch  out - if we are compiled with -mregparm=3, then the args  */
/*   are in registers. This is dependent on how the kernel is built,  */
/*   not  on  how  we are built. But we can use what Linux provides,  */
/*   and avoid the assembler dependency. 			      */
/**********************************************************************/
uint32_t
dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new)
{	uint32_t ret;

	ret = cmpxchg((void **) target, (void **) cmp, (void **) new);

/*	__asm(
		" movl 4(%esp), %edx\n"
		" movl 8(%esp), %eax\n"
		" movl 12(%esp), %ecx\n"
		" lock\n"
		" cmpxchgl %ecx, (%edx)\n"
		" ret\n"
		);*/
	return ret;

}
void *
dtrace_casptr(void *target, void *cmp, void *new) 
{	void *ret;

	ret = cmpxchg((void **) target, (void **) cmp, (void **) new);
/*	__asm(
		" movl 4(%esp), %edx\n"
		" movl 8(%esp), %eax\n"
		" movl 12(%esp), %ecx\n"
		" lock\n"
		" cmpxchgl %ecx, (%edx)\n"
		" ret\n"
		);*/
	return ret;
}

#endif	/* __i386 */

long
dtrace_caller(void)
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
	return 0; // notreached
}

/*ARGSUSED*/
void
dtrace_copy(uintptr_t src, uintptr_t dest, size_t size) 
{
#if 0 && defined(__amd64)
	__asm(
		"pushq	%rbp\n"
		"movq	%rsp, %rbp\n"

		"xchgq	%rdi, %rsi\n"		/* make %rsi source, %rdi dest */
		"movq	%rdx, %rcx\n"		/* load count */
		"repz\n"				/* repeat for count ... */
		"smovb\n"				/*   move from %ds:rsi to %ed:rdi */
		"leave\n"
		"ret\n"
		);

#elif defined(__i386)
	memcpy((void *) dest, (void *) src, size);
#endif	/* __i386 */
}

/*ARGSUSED*/
void
dtrace_copystr(uintptr_t uaddr, uintptr_t kaddr, size_t size,
	 volatile uint16_t *flags) 
{

#if 0 && defined(__amd64)
	/***********************************************/
	/*   This doesnt work -- could be a silly gcc  */
	/*   calling  convention,  but  for  now  the  */
	/*   portable  C code for 386 mode works just  */
	/*   fine.				       */
	/***********************************************/
	__asm(
		"pushq	%rbp\n"
		"movq	%rsp, %rbp\n"

"0:\n"
		"movb	(%rdi), %al\n"		/* load from source */
		"movb	%al, (%rsi)\n"		/* store to destination */
		"addq	$1, %rdi\n"		/* increment source pointer */
		"addq	$1, %rsi\n"		/* increment destination pointer */
		"subq	$1, %rdx\n"		/* decrement remaining count */
		"cmpb	$0, %al\n"
		"je	2f\n"
	        "testq   $0xfff, %rdx\n"        /* test if count is 4k-aligned */
                "jnz     1f\n"                  /* if not, continue with copying */
		// CPU_DTRACE_BADADDR == 0x04
		"testq   $0x04, (%rcx)\n" /* load and test dtrace flags */  
	        "jnz     2f\n"
"1:\n"
		"cmpq	$0, %rdx\n"
		"jne	0b\n"
"2:\n"
		"leave\n"
		"ret\n"
		);

#elif defined(__i386)
	char *src = (char *) uaddr;
	char *dst = (char *) kaddr;
	while (size > 0) {
		if ((*dst++ = *src++) == 0)
			break;
		size--;
	}
#endif	/* __i386 */
}

/*ARGSUSED*/
uintptr_t
dtrace_fulword(void *addr) 
{
#if defined(__amd64)
	__asm(
		"movq	(%rdi), %rax\n"
		"ret\n"
		);
	return 0; // notreached

#elif defined(__i386)

	return *(uintptr_t *) addr;

#endif	/* __i386 */
}

/*ARGSUSED*/
uint8_t 
dtrace_fuword8_nocheck(void *addr)
{
#if defined(__amd64)
	__asm(
		"xorq	%rax, %rax\n"
		"movb	(%rdi), %al\n"
		"ret\n"
		);
	return 0; // notreached

#elif defined(__i386)

	return *(unsigned char *) addr;;
#endif	/* __i386 */
}

/*ARGSUSED*/
uint16_t 
dtrace_fuword16_nocheck(void *addr) 
{
#if defined(__amd64)

	__asm(
		"xorq	%rax, %rax\n"
		"movw	(%rdi), %ax\n"
		"ret\n"
		);
	return 0; // notreached

#elif defined(__i386)

	return *(uint16_t *) addr;
#endif	/* __i386 */
}

/*ARGSUSED*/
uint32_t 
dtrace_fuword32_nocheck(void *addr) 
{
#if defined(__amd64)
	__asm(

		"xorq	%rax, %rax\n"
		"movl	(%rdi), %eax\n"
		"ret\n"
	);
	return 0; // notreached

#elif defined(__i386)

	return *(uint32_t *) addr;
#endif	/* __i386 */
}

/*ARGSUSED*/
uint64_t 
dtrace_fuword64_nocheck(void *addr) 
{
#if defined(__amd64)
	__asm(
		"movq	(%rdi), %rax\n"
		"ret\n"
		);
	return 0; // notreached

#elif defined(__i386)

	return *(uint64_t *) addr;
#endif	/* __i386 */
}
/**********************************************************************/
/*   This   routine   restores   interrupts   previously   saved  by  */
/*   dtrace_interrupt_disable.  This  allows  nested disable/enable,  */
/*   which  is  what  dtrace_probe()  is assuming, since we can come  */
/*   from an interrupt context, or not, as the case may be.	      */
/**********************************************************************/
void
dtrace_interrupt_enable(int flags)
{
#if defined(__amd64)
	__asm(
	        "pushq   %rdi\n"
	        "popfq\n"
	);

#elif defined(__i386)

//	/***********************************************/
//	/*   We  get kernel warnings because we break  */
//	/*   the  rules  if  we  do the equivalent to  */
//	/*   x86-64. This seems to work.	       */
//	/***********************************************/
//	raw_local_irq_enable();
////	native_irq_enable();
//	return;
	__asm(
		"push %0\n"
		"popf\n"
		:
		: "a" (flags)
	);
#endif
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
#if defined(__amd64)
	/* use 2 byte return instruction when branch target */
        /* AMD Software Optimization Guide - Section 6.2 */
	__asm(
		"rep\n"
		"ret\n"
		);
# else
	/***********************************************/
	/*   I dont know if this is correct, or slow,  */
	/*   but  the amd64 sequence above crashes on  */
	/*   a  real  32-bit  cpu (Pentium-M and Dual  */
	/*   Core 1.6GHz).			       */
	/***********************************************/
	__asm("sfence\n");
#endif
}
void dtrace_membar_producer(void)
{
#if defined(__amd64)
	/* use 2 byte return instruction when branch target */
        /* AMD Software Optimization Guide - Section 6.2 */
	__asm(
		"rep\n"
		"ret\n"
		);
# else
	/***********************************************/
	/*   I dont know if this is correct, or slow,  */
	/*   but  the amd64 sequence above crashes on  */
	/*   a  real  32-bit  cpu (Pentium-M and Dual  */
	/*   Core 1.6GHz).			       */
	/***********************************************/
	__asm("sfence\n");
#endif
}
/**********************************************************************/
/*   Disable  interrupts (they may be disabled already), but let the  */
/*   caller nest the interrupt disable.				      */
/**********************************************************************/
long
dtrace_interrupt_disable(void)
{
#if defined(__amd64)
	__asm(
        	"pushfq\n"
        	"popq    %rax\n"
        	"cli\n"
        	"ret\n"
	);
	return 0; // notreached
#elif defined(__i386)
	long	ret;

	__asm(
		"pushf\n"
		"pop %%eax\n"
		"cli\n"
		: "=a" (ret)
		:
	);
	return ret;

//	/***********************************************/
//	/*   We  get kernel warnings because we break  */
//	/*   the  rules  if  we  do the equivalent to  */
//	/*   x86-64. This seems to work.	       */
//	/***********************************************/
//	raw_local_irq_disable();
////	native_irq_disable();
//	return 0;
# endif
}

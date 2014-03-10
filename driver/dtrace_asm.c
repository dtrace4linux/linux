/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

//#pragma ident	"@(#)dtrace_asm.s	1.5	04/11/17 SMI"

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"

extern dtrace_id_t		dtrace_probeid_error;	/* special ERROR probe */


/*ARGSUSED*/
uint64_t
dtrace_getvmreg(uint32_t reg, volatile uint16_t *flags)
{
#if defined(__amd64)
	uint64_t ret;

	__asm(
		"movq	%%rdi, %%rdx\n"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
		/***********************************************/
		/*   This  is  real  gas/gcc  dependent,  not  */
		/*   kernel  dependent,  but  we'll  take the  */
		/*   tactic  that  old kernels are likely not  */
		/*   that interesting.			       */
		/***********************************************/
		".byte 0x0f, 0x78, 0xd0\n"
#else
		"vmread	%%rdx, %%rax\n"
#endif
		: "=a" (ret)
	);
	return ret;

#elif defined(__i386)

	*flags |= CPU_DTRACE_ILLOP;
	return 0;

#elif defined(__arm__)
	*flags |= CPU_DTRACE_ILLOP;
	return 0;
#endif
}

/**********************************************************************/
/*   Return callers frame pointer.				      */
/**********************************************************************/
greg_t
dtrace_getfp(void)
{
	return (greg_t) __builtin_frame_address(1);
}

#if defined(__amd64)
uint32_t
dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new)
{
	return cmpxchg(target, cmp, new);
/*
	__asm(
		"movl	%esi, %eax\n"
		"lock\n"
		"cmpxchgl %edx, (%rdi)\n"
		);
*/
	return 0;
}
void *
dtrace_casptr(void *target, void *cmp, void *new)
{
	return cmpxchg((void **) target, cmp, new);
/*
	if (*(void **) target == cmp) {
		printk("swapping...\n");
		*(void **) target = new;
		printk("done\n");
	}
return 0;
	return cmpxchg64((void **) target, cmp, new);
*/
/*
	__asm(
		"movq	%rsi, %rax\n"
		"lock\n"
		"cmpxchgq %rdx, (%rdi)\n"
		);
*/
	return 0;
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

#elif defined(__arm__)
uint32_t
dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new)
{
	return (uint32_t) cmpxchg((void **) target, (void **) cmp, (void **) new);
}
void *
dtrace_casptr(void *target, void *cmp, void *new) 
{
	return cmpxchg((void **) target, (void **) cmp, (void **) new);
}

#endif

/**********************************************************************/
/*   Used in probe mode to get the n'th caller, but GCC, nor can any  */
/*   compiler  really  do  this  well,  so  dtrace will default to a  */
/*   slower,   and   equally   bad   way   to  do  things.  (If  you  */
/*   -fomit-frame-pointer, you lose, but thats life).		      */
/**********************************************************************/
uintptr_t
dtrace_caller(int arg)
{
	return -1;
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

#else
	dtrace_memcpy((void *) dest, (void *) src, size);
#endif	/* __i386 */
}

/**********************************************************************/
/*   I  think  this  function  wants to detect a fault when touching  */
/*   user space...so we may need to change this to copyin/out.	      */
/**********************************************************************/
/*ARGSUSED*/
void
dtrace_copystr(uintptr_t uaddr, uintptr_t kaddr, size_t size,
	 volatile uint16_t *flags) 
{
	(void) copy_from_user((void *) kaddr, (void *) uaddr, size);
}

/*ARGSUSED*/
uintptr_t
dtrace_fulword(void *addr) 
{
	return *(uintptr_t *) addr;
}

/*ARGSUSED*/
uint8_t 
dtrace_fuword8_nocheck(void *addr)
{
	return *(unsigned char *) addr;
}

/*ARGSUSED*/
uint16_t 
dtrace_fuword16_nocheck(void *addr) 
{
	return *(uint16_t *) addr;
}

/*ARGSUSED*/
uint32_t 
dtrace_fuword32_nocheck(void *addr) 
{
	return *(uint32_t *) addr;
}

/*ARGSUSED*/
uint64_t 
dtrace_fuword64_nocheck(void *addr) 
{
	return *(uint64_t *) addr;
}
/**********************************************************************/
/*   Disable  interrupts (they may be disabled already), but let the  */
/*   caller nest the interrupt disable.				      */
/**********************************************************************/
dtrace_icookie_t
dtrace_interrupt_disable(void)
{
	unsigned long ret;
	raw_local_irq_save(ret);
	return ret;
}

dtrace_icookie_t
dtrace_interrupt_get(void)
{	unsigned long ret;
	dtrace_interrupt_enable(ret = dtrace_interrupt_disable());
	return ret;
}
/**********************************************************************/
/*   This   routine   restores   interrupts   previously   saved  by  */
/*   dtrace_interrupt_disable.  This  allows  nested disable/enable,  */
/*   which  is  what  dtrace_probe()  is assuming, since we can come  */
/*   from an interrupt context, or not, as the case may be.	      */
/**********************************************************************/
void
dtrace_interrupt_enable(dtrace_icookie_t flags)
{
	raw_local_irq_restore(flags);
}

/*ARGSUSED*/
void
dtrace_probe_error(dtrace_state_t *state, dtrace_epid_t epid, int which,
    int fault, int fltoffs, uintptr_t illval)
{
	/***********************************************/
	/*   Store   away   the  faulting  address  -  */
	/*   dtrace_probe()  doesnt  have enough argN  */
	/*   to let us pass this in, so we keep it to  */
	/*   one side.				       */
	/***********************************************/
	state->dts_arg_error_illval = illval;
    	dtrace_probe(dtrace_probeid_error, 
		(uintptr_t) state, epid, which, fault, fltoffs);
}

/**********************************************************************/
/*   We try and mix C + Assembler. We have to be very careful with C  */
/*   functions  which  may have variable calling/entry sequences and  */
/*   break  our expectations on some of the more delicate functions.  */
/*   So  we  have  a  dummy function which can contain the assembler  */
/*   with our own explicit (or missing) stack frame setup.	      */
/*   								      */
/*   We  could  do  this  all  in a *.S file, but I didnt want to do  */
/*   that,  especially  as most of the functions here can be done in  */
/*   plain C.							      */
/**********************************************************************/

void
asm_placeholder(void)
{
#if defined(__amd64)
	__asm(
		FUNCTION(dtrace_membar_consumer)
        	/* AMD Software Optimization Guide - Section 6.2 */
		"rep\n"
		"ret\n"

		FUNCTION(dtrace_membar_producer)
        	/* AMD Software Optimization Guide - Section 6.2 */
		"rep\n"
		"ret\n"
		
		);
# elif defined(__i386)
	__asm(
		FUNCTION(dtrace_membar_consumer)
		"sfence\n"
		"ret\n"

		FUNCTION(dtrace_membar_producer)
		"sfence\n"
		"ret\n"

		);
# elif defined(__arm__)

# else
#	error "dtrace_asm.c: Not supported on this cpu.\n"
#endif
}
#if defined(__arm__)
void
dtrace_membar_consumer()
{
}
void
dtrace_membar_producer()
{
}
#endif

/**********************************************************************/
/*   Stubbed  out  code  --  this  was the original, but its getting  */
/*   painful  to  support 32+64 * versions of GCC and kernel compile  */
/*   options.							      */
/**********************************************************************/
#if 0
void dtrace_membar_consumer(void)
{
#if defined(__amd64)
	/* use 2 byte return instruction when branch target */
        /* AMD Software Optimization Guide - Section 6.2 */
	__asm(
		"leaveq\n"
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

	/***********************************************/
	/*   We  do  an  internal call/ret to get the  */
	/*   semantics  of  rep/ret, without worrying  */
	/*   about  the  GCC  call  frame setup/clear  */
	/*   down.				       */
	/***********************************************/
	__asm(
"nop\n"
		"call 0f\n"
"nop\n"
		"jmp 1f\n"
"0:\n"
		"rep\n"
		"ret\n"
"1:\n"
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
#endif

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

# if !defined(SYS_PRIVREGS_H)
# define SYS_PRIVREGS_H

#if defined(sun)
#if defined(__i386)
struct regs {
        /*
         * Extra frame for mdb to follow through high level interrupts and
         * system traps.  Set them to 0 to terminate stacktrace.
         */
        greg_t  r_savfp;        /* a copy of %ebp */
        greg_t  r_savpc;        /* a copy of %eip */

        greg_t  r_gs;
        greg_t  r_fs;
        greg_t  r_es;
        greg_t  r_ds;
        greg_t  r_edi;
        greg_t  r_esi;
        greg_t  r_ebp;
        greg_t  r_esp;
        greg_t  r_ebx;
        greg_t  r_edx;
        greg_t  r_ecx;
        greg_t  r_eax;
        greg_t  r_trapno;
        greg_t  r_err;
        greg_t  r_eip;
        greg_t  r_cs;
        greg_t  r_efl;
        greg_t  r_uesp;
        greg_t  r_ss;
};

#define r_r0    r_eax           /* r0 for portability */
#define r_r1    r_edx           /* r1 for portability */
#define r_fp    r_ebp           /* system frame pointer */
#define r_sp    r_uesp          /* user stack pointer */
#define r_pc    r_eip           /* user's instruction pointer */
#define r_ps    r_efl           /* user's EFLAGS */

#define GREG_NUM        8

#else
struct regs {
        /*
         * Extra frame for mdb to follow through high level interrupts and
         * system traps.  Set them to 0 to terminate stacktrace.
         */
        greg_t  r_savfp;        /* a copy of %rbp */
        greg_t  r_savpc;        /* a copy of %rip */

        greg_t  r_rdi;          /* 1st arg to function */
        greg_t  r_rsi;          /* 2nd arg to function */
        greg_t  r_rdx;          /* 3rd arg to function, 2nd return register */
        greg_t  r_rcx;          /* 4th arg to function */

        greg_t  r_r8;           /* 5th arg to function */
        greg_t  r_r9;           /* 6th arg to function */
        greg_t  r_rax;          /* 1st return register, # SSE registers */
        greg_t  r_rbx;          /* callee-saved, optional base pointer */

        greg_t  r_rbp;          /* callee-saved, optional frame pointer */
        greg_t  r_r10;          /* temporary register, static chain pointer */
        greg_t  r_r11;          /* temporary register */
        greg_t  r_r12;          /* callee-saved */

        greg_t  r_r13;          /* callee-saved */
        greg_t  r_r14;          /* callee-saved */
        greg_t  r_r15;          /* callee-saved */

        /*
         * fsbase and gsbase are sampled on every exception in DEBUG kernels
         * only.  They remain in the non-DEBUG kernel to avoid any flag days.
         */
        greg_t  __r_fsbase;     /* no longer used in non-DEBUG builds */
        greg_t  __r_gsbase;     /* no longer used in non-DEBUG builds */
        greg_t  r_ds;
        greg_t  r_es;
        greg_t  r_fs;           /* %fs is *never* used by the kernel */
        greg_t  r_gs;

        greg_t  r_trapno;

        /*
         * (the rest of these are defined by the hardware)
         */
        greg_t  r_err;
        greg_t  r_rip;
        greg_t  r_cs;
        greg_t  r_rfl;
        greg_t  r_rsp;
        greg_t  r_ss;
};

#define r_r0    r_rax   /* r0 for portability */
#define r_r1    r_rdx   /* r1 for portability */
#define r_fp    r_rbp   /* kernel frame pointer */
#define r_sp    r_rsp   /* user stack pointer */
#define r_pc    r_rip   /* user's instruction pointer */
#define r_ps    r_rfl   /* user's RFLAGS */

# endif
# endif /* sun */

/**********************************************************************/
/*   Compatibility for pt_regs.					      */
/**********************************************************************/
#if linux

#if defined(__arm__)
#   define r_rax ARM_r0
#   define r_rfl ARM_cpsr
#   define r_ps	ARM_cpsr
#   define r_rsp ARM_sp
#   define r_sp ARM_sp
#   define r_pc ARM_pc
#   define r_fp ARM_lr
#   define r_r0 ARM_r0
#   define r_r1 ARM_r1
#   define X86_EFLAGS_IF PSR_Q_BIT

#elif defined(__amd64) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)

#   define esp          rsp
#   define r_cs         cs
#   define r_ds         cs   /* cs==ds==es==fs==gs */
#   define r_es         cs
#   define r_fs         cs
#   define r_gs         cs
#   define r_pc         rip
#   define r_ps		eflags
#   define r_r0         rax
#   define r_r1         rdx
#   define r_r10        r10
#   define r_r11        r11
#   define r_r12        r12
#   define r_r13        r13
#   define r_r14        r14
#   define r_r15        r15
#   define r_r8         r8
#   define r_r9         r9
#   define r_rax        rax
#   define r_rbp        rbp
#   define r_rbx        rbx
#   define r_rcx        rcx
#   define r_rdi        rdi
#   define r_rdx        rdx
#   define r_rfl        eflags
#   define r_fp         rbp
#   define r_rip        rip
#   define r_rsi        rsi
#   define r_rsp        rsp
#   define r_sp         rsp
#   define r_ss         ss
#   define r_trapno     orig_rax

#elif defined(__amd64) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)

#   define r_cs 	cs
#   define r_ds		cs   /* cs==ds==es==fs==gs */
#   define r_es		cs
#   define r_fp    	bp           /* system frame pointer */
#   define r_fs		cs
#   define r_gs		cs
#   define r_pc		ip
#   define r_ps 	flags
#   define r_r0		ax
#   define r_r1 	dx
#   define r_r10 	r10
#   define r_r11 	r11
#   define r_r12 	r12
#   define r_r13 	r13
#   define r_r14 	r14
#   define r_r15 	r15
#   define r_r8 	r8
#   define r_r9 	r9
#   define r_rax 	ax
#   define r_rbp 	bp
#   define r_rbx 	bx
#   define r_rcx 	cx
#   define r_rdi 	di
#   define r_rdx 	dx
#   define r_rfl 	flags
#   define r_rip 	ip
#   define r_rsi 	si
#   define r_rsp	sp
#   define r_sp		sp
#   define r_ss 	ss
#   define r_trapno	orig_ax
   
#elif defined(__i386) && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)

#   define r_cs         xcs
#   define r_ds         xds
#   define r_es         xes
#   define r_ecx        ecx
#   define r_fp         ebp
#   define r_fs         xfs
#   define r_gs         orig_eax
#   define r_pc         eip
#   define r_ps         eflags
#   define r_r0		eax
#   define r_r1		edx
#   define r_rax        eax
#   define r_rbp        ebp
#   define r_rbx        ebx
#   define r_rcx        ecx
#   define r_rdi        edi
#   define r_rdx        edx
#   define r_rfl        eflags
#   define r_rip 	eip
#   define r_rsi 	esi
#   define r_rsp        esp
#   define r_sp         esp
#   define r_ss         xss
#   define r_trapno     orig_eax

#elif defined(__i386)

#   define r_cs         cs
#   define r_ds         ds
#   define r_es         es
#   define r_fs         fs
#   define r_ecx        cx
#   define r_fp         bp
#   define r_fs         fs
#   define r_gs         orig_ax
#   define r_pc         ip
#   define r_ps         flags
#   define r_r0		ax
#   define r_r1		dx
#   define r_rax        ax
#   define r_rbp        bp
#   define r_rbx        bx
#   define r_rcx        cx
#   define r_rdi        di
#   define r_rdx        dx
#   define r_rfl        flags
#   define r_rip 	ip
#   define r_rsi 	si
#   define r_rsp        sp
#   define r_sp         sp
#   define r_ss         ss
#   define r_trapno     orig_ax

#endif

#endif

/**********************************************************************/
/*   Following  needed  for  the  invop()  functions  to  find the C  */
/*   arguments.							      */
/**********************************************************************/
#if defined(__amd64)
#  define c_arg0 r_rdi
#  define c_arg1 r_rsi
#  define c_arg2 r_rdx
#  define c_arg3 r_rcx
#  define c_arg4 r_r8
#  define c_arg5 r_r9

#elif defined(__i386)
#  if defined(HAVE_EBX_REGISTER)
#     define c_arg0 ebx
#     define c_arg1 ecx
#     define c_arg2 edx
#     define c_arg3 esi
#     define c_arg4 edi
#  else
#     define c_arg0 bx
#     define c_arg1 cx
#     define c_arg2 dx
#     define c_arg3 si
#     define c_arg4 di
#  endif
#  define c_arg5 ebp

# elif defined(__arm__)
#  define c_arg0 ARM_r0
#  define c_arg1 ARM_r1
#  define c_arg2 ARM_r2
#  define c_arg3 ARM_r3
#  define c_arg4 ARM_r4
#  define c_arg5 ARM_r5

#endif

# endif /* !defined(SYS_PRIVREGS_H) */

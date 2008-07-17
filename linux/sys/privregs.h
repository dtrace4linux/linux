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

# endif /* !defined(SYS_PRIVREGS_H) */

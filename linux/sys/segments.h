/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_SEGMENTS_H
#define _SYS_SEGMENTS_H

//#pragma ident   "@(#)segments.h 1.11    07/11/06 SMI"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 1989, 1990 William F. Jolitz
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      from: @(#)segments.h    7.1 (Berkeley) 5/9/91
 * $FreeBSD: src/sys/i386/include/segments.h,v 1.34 2003/09/10 01:07:04
 * jhb Exp $
 *
 * 386 Segmentation Data Structures and definitions
 *      William F. Jolitz (william@ernie.berkeley.edu) 6/20/1989
 */
 
/*
 * User segment descriptors (code and data).
 * Legacy mode 64-bits wide.
 */
# if 0 /* only needed by fasttrap_isa.c */
typedef struct user_desc {
        uint32_t usd_lolimit:16;        /* segment limit 15:0 */
        uint32_t usd_lobase:16;         /* segment base 15:0 */
        uint32_t usd_midbase:8;         /* segment base 23:16 */
        uint32_t usd_type:5;            /* segment type, includes S bit */
        uint32_t usd_dpl:2;             /* segment descriptor priority level */
        uint32_t usd_p:1;               /* segment descriptor present */
        uint32_t usd_hilimit:4;         /* segment limit 19:16 */
        uint32_t usd_avl:1;             /* available to sw, but not used */
        uint32_t usd_reserved:1;        /* unused, ignored */
        uint32_t usd_def32:1;           /* default 32 vs 16 bit operand */
        uint32_t usd_gran:1;            /* limit units (bytes vs pages) */
        uint32_t usd_hibase:8;          /* segment base 31:24 */
} user_desc_t;
#endif

#define SELTOIDX(s)     ((s) >> 3)      /* selector to index */

#define	SEL_UPL		3		/* user priority level */
#define	TRP_UPL		3		/* system gate priv (user allowed) */
#define	SEL_TI_LDT	4		/* local descriptor table */
#define	SEL_LDT(s)	(IDXTOSEL(s) | SEL_TI_LDT | SEL_UPL)	/* local sel */
#define	CPL_MASK	3		/* RPL mask for selector */
#define	SELISLDT(s)	(((s) & SEL_TI_LDT) == SEL_TI_LDT)
#define	SELISUPL(s)	(((s) & CPL_MASK) == SEL_UPL)

#ifdef  __cplusplus
}
#endif

# endif /* _SYS_SEGMENTS_H */

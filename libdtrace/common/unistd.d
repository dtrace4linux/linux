/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)unistd.d	1.2	04/09/27 SMI"

inline int DTRACEFLT_UNKNOWN = 0;	/* Unknown fault */
#pragma D binding "1.0" DTRACEFLT_UNKNOWN

inline int DTRACEFLT_BADADDR = 1;	/* Bad address */
#pragma D binding "1.0" DTRACEFLT_BADADDR

inline int DTRACEFLT_BADALIGN = 2;	/* Bad alignment */
#pragma D binding "1.0" DTRACEFLT_BADALIGN

inline int DTRACEFLT_ILLOP = 3;		/* Illegal operation */
#pragma D binding "1.0" DTRACEFLT_ILLOP

inline int DTRACEFLT_DIVZERO = 4;	/* Divide-by-zero */
#pragma D binding "1.0" DTRACEFLT_DIVZERO

inline int DTRACEFLT_NOSCRATCH = 5;	/* Out of scratch space */
#pragma D binding "1.0" DTRACEFLT_NOSCRATCH

inline int DTRACEFLT_KPRIV = 6;		/* Illegal kernel access */
#pragma D binding "1.0" DTRACEFLT_KPRIV

inline int DTRACEFLT_UPRIV = 7;		/* Illegal user access */
#pragma D binding "1.0" DTRACEFLT_UPRIV

inline int DTRACEFLT_TUPOFLOW = 8;	/* Tuple stack overflow */
#pragma D binding "1.0" DTRACEFLT_TUPOFLOW

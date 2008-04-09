/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)dt_asmsubr.s	1.1	04/03/01 SMI"

#include <sys/asm_linkage.h>
#include <sys/trap.h>

#ifdef lint

#include <dtrace.h>

/*ARGSUSED*/
int
dtrace_probe(uintptr_t arg0, ...)
{ return (0); }

#else

	ENTRY(dtrace_probe)
	int	$T_DTRACE_PROBE
	xorl	%eax, %eax
	ret
	SET_SIZE(dtrace_probe)

#endif

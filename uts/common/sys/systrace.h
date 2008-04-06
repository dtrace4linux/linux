/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef _SYS_SYSTRACE_H
#define	_SYS_SYSTRACE_H

#pragma ident	"@(#)systrace.h	1.1	03/09/02 SMI"

#include <sys/dtrace.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef _KERNEL

typedef struct systrace_sysent {
	dtrace_id_t	stsy_entry;
	dtrace_id_t	stsy_return;
	int64_t		(*stsy_underlying)();
} systrace_sysent_t;

extern systrace_sysent_t *systrace_sysent;
extern systrace_sysent_t *systrace_sysent32;

extern void (*systrace_probe)(dtrace_id_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t);
extern void systrace_stub(dtrace_id_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t);

extern int64_t dtrace_systrace_syscall(uintptr_t arg0, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);

#ifdef _SYSCALL32_IMPL
extern int64_t dtrace_systrace_syscall32(uintptr_t arg0, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);
#endif

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SYSTRACE_H */

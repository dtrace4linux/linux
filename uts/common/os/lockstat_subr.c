/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)lockstat_subr.c	1.3	03/09/02 SMI"

#include <sys/types.h>
#include <sys/inttypes.h>
#include <sys/time.h>
#include <sys/thread.h>
#include <sys/cpuvar.h>
#include <sys/cyclic.h>
#include <sys/lockstat.h>
#include <sys/spl.h>

/*
 * Resident support for the lockstat driver.
 */

dtrace_id_t lockstat_probemap[LS_NPROBES];
void (*lockstat_probe)(dtrace_id_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t);

int
lockstat_active_threads(void)
{
	kthread_t *tp;
	int active = 0;

	mutex_enter(&pidlock);
	tp = curthread;
	do {
		if (tp->t_lockstat)
			active++;
	} while ((tp = tp->t_next) != curthread);
	mutex_exit(&pidlock);
	return (active);
}

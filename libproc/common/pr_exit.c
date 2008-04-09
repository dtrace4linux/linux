/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)pr_exit.c	1.3	03/01/03 SMI"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/lwp.h>
#include "libproc.h"

/*
 * exit() system call -- executed by subject process.
 */
int
pr_exit(struct ps_prochandle *Pr, int status)
{
	sysret_t rval;			/* return value from exit() */
	argdes_t argd[1];		/* arg descriptors for exit() */
	argdes_t *adp;
	int error;

	if (Pr == NULL) {		/* no subject process */
		exit(status);
		return (0);		/* not reached */
	}

	adp = &argd[0];		/* status argument */
	adp->arg_value = status;
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	error = Psyscall(Pr, &rval, SYS_exit, 1, &argd[0]);
	/* actually -- never returns.  Expect ENOENT */

	if (error < 0) {
		if (errno == ENOENT)	/* expected case */
			error = ENOENT;
		else
			error = ENOSYS;
	}

	if (error == 0)		/* can't happen? */
		return (rval.sys_rval1);

	if (error == ENOENT)	/* expected case */
		return (0);

	errno = error;
	return (-1);
}

/*
 * lwp_exit() system call -- executed by subject lwp.
 */
int
pr_lwp_exit(struct ps_prochandle *Pr)
{
	sysret_t rval;			/* return value from lwp_exit() */
	int error;

	if (Pr == NULL) {		/* no subject process */
		(void) syscall(SYS_lwp_exit);
		return (0);		/* not reached */
	}

	error = Psyscall(Pr, &rval, SYS_lwp_exit, 0, NULL);
	/* actually -- never returns.  Expect ENOENT */

	if (error < 0) {
		if (errno == ENOENT)	/* expected case */
			error = ENOENT;
		else
			error = ENOSYS;
	}

	if (error == 0)		/* can't happen? */
		return (rval.sys_rval1);

	if (error == ENOENT)	/* expected case */
		return (0);

	errno = error;
	return (-1);
}

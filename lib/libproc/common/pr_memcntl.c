/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)pr_memcntl.c	1.3	01/04/18 SMI"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "libproc.h"

/*
 * memcntl() system call -- executed by subject process
 */

int
pr_memcntl(struct ps_prochandle *Pr,
	caddr_t addr, size_t len, int cmd, caddr_t arg, int attr, int mask)
{
	sysret_t rval;			/* return value from memcntl() */
	argdes_t argd[6];		/* arg descriptors for memcntl() */
	argdes_t *adp;
	int error;

	if (Pr == NULL)		/* no subject process */
		return (memcntl(addr, len, cmd, arg, attr, mask));

	adp = &argd[0];		/* addr argument */
	adp->arg_value = (uintptr_t)addr;
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	adp++;			/* len argument */
	adp->arg_value = len;
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	adp++;			/* cmd argument */
	adp->arg_value = cmd;
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	adp++;			/* arg argument */
	if (cmd == MC_HAT_ADVISE) {
		adp->arg_value = 0;
		adp->arg_object = arg;
		adp->arg_type = AT_BYREF;
		adp->arg_inout = AI_INPUT;
#ifdef _LP64
		if (Pstatus(Pr)->pr_dmodel == PR_MODEL_ILP32)
			adp->arg_size = sizeof (struct memcntl_mha32);
		else
			adp->arg_size = sizeof (struct memcntl_mha);
#else
		adp->arg_size = sizeof (struct memcntl_mha);
#endif
	} else {
		adp->arg_value = (uintptr_t)arg;
		adp->arg_object = NULL;
		adp->arg_type = AT_BYVAL;
		adp->arg_inout = AI_INPUT;
		adp->arg_size = 0;
	}

	adp++;			/* attr argument */
	adp->arg_value = attr;
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	adp++;			/* mask argument */
	adp->arg_value = mask;
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	error = Psyscall(Pr, &rval, SYS_memcntl, 6, &argd[0]);

	if (error) {
		errno = (error > 0)? error : ENOSYS;
		return (-1);
	}
	return (0);
}

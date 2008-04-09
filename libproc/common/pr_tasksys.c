/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)pr_tasksys.c	1.3	04/04/30 SMI"

#define	_LARGEFILE64_SOURCE

#include <sys/task.h>
#include <sys/types.h>

#include <zone.h>
#include <errno.h>
#include <project.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "libproc.h"

zoneid_t
pr_getzoneid(struct ps_prochandle *Pr)
{
	sysret_t rval;
	argdes_t argd[2];
	argdes_t *adp;
	int error;

	if (Pr == NULL)		/* no subject process */
		return (getzoneid());

	adp = &argd[0];
	adp->arg_value = 6;	/* switch for zone_lookup in zone */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	adp = &argd[1];
	adp->arg_value = 0;	/* arguement for zone_lookup in zone */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	error = Psyscall(Pr, &rval, SYS_zone, 2, &argd[0]);

	if (error) {
		errno = (error > 0) ? error : ENOSYS;
		return (-1);
	}
	return (rval.sys_rval1);
}

projid_t
pr_getprojid(struct ps_prochandle *Pr)
{
	sysret_t rval;
	argdes_t argd[1];
	argdes_t *adp;
	int error;

	if (Pr == NULL)		/* no subject process */
		return (getprojid());

	adp = &argd[0];
	adp->arg_value = 2;	/* switch for getprojid in tasksys */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	error = Psyscall(Pr, &rval, SYS_tasksys, 1, &argd[0]);

	if (error) {
		errno = (error > 0) ? error : ENOSYS;
		return (-1);
	}
	return (rval.sys_rval1);
}

taskid_t
pr_gettaskid(struct ps_prochandle *Pr)
{
	sysret_t rval;
	argdes_t argd[1];
	argdes_t *adp;
	int error;

	if (Pr == NULL)		/* no subject process */
		return (gettaskid());

	adp = &argd[0];
	adp->arg_value = 1;	/* switch for gettaskid in tasksys */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	error = Psyscall(Pr, &rval, SYS_tasksys, 1, &argd[0]);

	if (error) {
		errno = (error > 0) ? error : ENOSYS;
		return (-1);
	}
	return (rval.sys_rval1);
}

taskid_t
pr_settaskid(struct ps_prochandle *Pr, projid_t project, int flags)
{
	sysret_t rval;
	argdes_t argd[3];
	argdes_t *adp;
	int error;

	if (Pr == NULL)		/* No subject process */
		return (settaskid(project, flags));

	adp = &argd[0];
	adp->arg_value = 0;	/* switch for settaskid in tasksys */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	adp++;
	adp->arg_value = project;
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = sizeof (project);

	adp++;
	adp->arg_value = flags;
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;

	error = Psyscall(Pr, &rval, SYS_tasksys, 3, &argd[0]);

	if (error) {
		errno = (error > 0) ? error : ENOSYS;
		return (-1);
	}
	return (rval.sys_rval1);
}

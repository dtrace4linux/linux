/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)pr_pbind.c	1.1	04/07/15 SMI"

#include <sys/types.h>
#include <sys/procset.h>
#include <sys/processor.h>
#include <sys/pset.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "libproc.h"

int
pr_processor_bind(struct ps_prochandle *Pr, idtype_t idtype, id_t id,
    int processorid, int *obind)
{
	sysret_t rval;			/* return value */
	argdes_t argd[4];		/* arg descriptors */
	argdes_t *adp = &argd[0];	/* first argument */
	int error;

	if (Pr == NULL)		/* no subject process */
		return (processor_bind(idtype, id, processorid, obind));

	adp->arg_value = idtype;	/* idtype */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;
	adp++;

	adp->arg_value = id;		/* id */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;
	adp++;

	adp->arg_value = processorid;	/* processorid */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;
	adp++;

	if (obind == NULL) {
		adp->arg_value = 0;	/* obind */
		adp->arg_object = NULL;
		adp->arg_type = AT_BYVAL;
		adp->arg_inout = AI_INPUT;
		adp->arg_size = 0;
	} else {
		adp->arg_value = 0;
		adp->arg_object = obind;
		adp->arg_type = AT_BYREF;
		adp->arg_inout = AI_INOUT;
		adp->arg_size = sizeof (int);
	}

	error = Psyscall(Pr, &rval, SYS_processor_bind, 4, &argd[0]);

	if (error) {
		errno = (error < 0)? ENOSYS : error;
		return (-1);
	}
	return (rval.sys_rval1);
}

int
pr_pset_bind(struct ps_prochandle *Pr, int pset, idtype_t idtype, id_t id,
    int *opset)
{
	sysret_t rval;			/* return value */
	argdes_t argd[5];		/* arg descriptors */
	argdes_t *adp = &argd[0];	/* first argument */
	int error;

	if (Pr == NULL)		/* no subject process */
		return (pset_bind(pset, idtype, id, opset));

	adp->arg_value = PSET_BIND;	/* PSET_BIND */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;
	adp++;

	adp->arg_value = pset;		/* pset */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;
	adp++;

	adp->arg_value = idtype;	/* idtype */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;
	adp++;

	adp->arg_value = id;		/* id */
	adp->arg_object = NULL;
	adp->arg_type = AT_BYVAL;
	adp->arg_inout = AI_INPUT;
	adp->arg_size = 0;
	adp++;

	if (opset == NULL) {
		adp->arg_value = 0;	/* opset */
		adp->arg_object = NULL;
		adp->arg_type = AT_BYVAL;
		adp->arg_inout = AI_INPUT;
		adp->arg_size = 0;
	} else {
		adp->arg_value = 0;
		adp->arg_object = opset;
		adp->arg_type = AT_BYREF;
		adp->arg_inout = AI_INOUT;
		adp->arg_size = sizeof (int);
	}

	error = Psyscall(Pr, &rval, SYS_pset, 5, &argd[0]);

	if (error) {
		errno = (error < 0)? ENOSYS : error;
		return (-1);
	}
	return (rval.sys_rval1);
}

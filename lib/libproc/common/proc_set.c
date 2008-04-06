/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)proc_set.c	1.4	04/03/26 SMI"

#include "libproc.h"
#include <alloca.h>
#include <string.h>

/*
 * Convenience wrapper to set the cred attributes of a victim process
 * to a set of new values. Caller must supply a prochandle and a
 * fully populated prcred structure.
 */
int
Psetcred(struct ps_prochandle *Pr, const prcred_t *credp)
{
	int ngrp;
	int ctlsize;
	struct {
		long cmd;
		prcred_t cred;
	} *ctlp;

	if (Pr == NULL || credp == NULL)
		return (-1);

	ngrp = credp->pr_ngroups;
	ctlsize = sizeof (prcred_t) + (ngrp - 1) * sizeof (gid_t);
	ctlp = alloca(ctlsize + sizeof (long));

	ctlp->cmd = PCSCREDX;
	(void) memcpy(&ctlp->cred, credp, ctlsize);

	if (write(Pctlfd(Pr), ctlp, sizeof (long) + ctlsize) < 0)
		return (-1);

	return (0);
}

/*
 * Convenience wrapper to set the zoneid attribute of a victim process to a new
 * value (only to and from GLOBAL_ZONEID makes sense).  Caller must supply a
 * prochandle and a valid zoneid.
 */
int
Psetzoneid(struct ps_prochandle *Pr, zoneid_t zoneid)
{
	struct {
		long cmd;
		long zoneid;
	} ctl;

	if (Pr == NULL)
		return (-1);

	ctl.zoneid = zoneid;
	ctl.cmd = PCSZONE;

	if (write(Pctlfd(Pr), &ctl, sizeof (ctl)) < 0)
		return (-1);
	return (0);
}

/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)drti.c	1.4	04/08/31 SMI"

#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <link.h>
#include <sys/dtrace.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static const char *modname;	/* Name of this load object */
static int gen;			/* DOF helper generation */
extern dof_hdr_t __SUNW_dof;	/* DOF defined in the .SUNW_dof section */

static void
dprintf(int debug, const char *fmt, ...)
{
	va_list ap;

	if (debug && getenv("DTRACE_DOF_INIT_DEBUG") == NULL)
		return;

	va_start(ap, fmt);

	if (modname == NULL)
		(void) fprintf(stderr, "dtrace DOF: ");
	else
		(void) fprintf(stderr, "dtrace DOF %s: ", modname);

	(void) vfprintf(stderr, fmt, ap);

	if (fmt[strlen(fmt) - 1] != '\n')
		(void) fprintf(stderr, ": %s\n", strerror(errno));

	va_end(ap);
}

#pragma init(dtrace_dof_init)
static void
dtrace_dof_init(void)
{
	dof_hdr_t *dof = &__SUNW_dof;
#ifdef _LP64
	Elf64_Ehdr *elf;
#else
	Elf32_Ehdr *elf;
#endif
	dof_helper_t dh;
	Link_map *lmp;
	Lmid_t lmid;
	int fd;

	if (getenv("DTRACE_DOF_INIT_DISABLE") != NULL)
		return;

	if (dlinfo(RTLD_SELF, RTLD_DI_LINKMAP, &lmp) == -1 || lmp == NULL) {
		dprintf(1, "couldn't discover module name or address\n");
		return;
	}

	if (dlinfo(RTLD_SELF, RTLD_DI_LMID, &lmid) == -1) {
		dprintf(1, "couldn't discover link map ID\n");
		return;
	}

	if ((modname = strrchr(lmp->l_name, '/')) == NULL)
		modname = lmp->l_name;
	else
		modname++;

	if (dof->dofh_ident[DOF_ID_MAG0] != DOF_MAG_MAG0 ||
	    dof->dofh_ident[DOF_ID_MAG1] != DOF_MAG_MAG1 ||
	    dof->dofh_ident[DOF_ID_MAG2] != DOF_MAG_MAG2 ||
	    dof->dofh_ident[DOF_ID_MAG3] != DOF_MAG_MAG3) {
		dprintf(0, ".SUNW_dof section corrupt\n");
		return;
	}

	elf = (void *)lmp->l_addr;

	dh.dofhp_dof = (uintptr_t)dof;
	dh.dofhp_addr = elf->e_type == ET_DYN ? lmp->l_addr : 0;

	if (lmid == 0) {
		(void) snprintf(dh.dofhp_mod, sizeof (dh.dofhp_mod),
		    "%s", modname);
	} else {
		(void) snprintf(dh.dofhp_mod, sizeof (dh.dofhp_mod),
		    "LM%lu`%s", lmid, modname);
	}

	if ((fd = open64("/devices/pseudo/dtrace@0:helper", O_RDWR)) < 0) {
		dprintf(1, "failed to open helper device");
		return;
	}

	if ((gen = ioctl(fd, DTRACEHIOC_ADDDOF, &dh)) == -1)
		dprintf(1, "DTrace ioctl failed for DOF at %p", dof);
	else
		dprintf(1, "DTrace ioctl succeeded for DOF at %p\n", dof);

	(void) close(fd);
}

#pragma fini(dtrace_dof_fini)
static void
dtrace_dof_fini(void)
{
	int fd;

	if ((fd = open64("/devices/pseudo/dtrace@0:helper", O_RDWR)) < 0) {
		dprintf(1, "failed to open helper device");
		return;
	}

	if ((gen = ioctl(fd, DTRACEHIOC_REMOVE, gen)) == -1)
		dprintf(1, "DTrace ioctl failed to remove DOF (%d)\n", gen);
	else
		dprintf(1, "DTrace ioctl removed DOF (%d)\n", gen);

	(void) close(fd);
}

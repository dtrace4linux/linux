/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_PID_H
#define	_DT_PID_H

#pragma ident	"@(#)dt_pid.h	1.4	04/06/23 SMI"

#include <libproc.h>
#include <sys/fasttrap.h>
#include <dt_impl.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define	DT_PROC_ERR	(-1)
#define	DT_PROC_ALIGN	(-2)

extern void dt_pid_create_probes(dtrace_probedesc_t *, dtrace_hdl_t *);

extern int dt_pid_create_entry_probe(struct ps_prochandle *, dtrace_hdl_t *,
    fasttrap_probe_spec_t *, const GElf_Sym *);

extern int dt_pid_create_return_probe(struct ps_prochandle *, dtrace_hdl_t *,
    fasttrap_probe_spec_t *, const GElf_Sym *, uint64_t *);

extern int dt_pid_create_offset_probe(struct ps_prochandle *, dtrace_hdl_t *,
    fasttrap_probe_spec_t *, const GElf_Sym *, ulong_t);

extern int dt_pid_create_glob_offset_probes(struct ps_prochandle *,
    dtrace_hdl_t *, fasttrap_probe_spec_t *, const GElf_Sym *, const char *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_PID_H */

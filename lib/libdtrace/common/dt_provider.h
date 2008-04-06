/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_PROVIDER_H
#define	_DT_PROVIDER_H

#pragma ident	"@(#)dt_provider.h	1.3	04/06/22 SMI"

#include <dt_impl.h>
#include <dt_ident.h>
#include <dt_list.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dt_provider {
	dt_list_t pv_list;		/* list forward/back pointers */
	struct dt_provider *pv_next;	/* pointer to next provider in hash */
	dtrace_providerdesc_t pv_desc;	/* provider name and attributes */
	dt_idhash_t *pv_probes;		/* probe defs (if user-declared) */
	dt_node_t *pv_nodes;		/* parse node allocation list */
	ulong_t pv_gen;			/* generation # that created me */
} dt_provider_t;

typedef struct dt_probe_instance dt_probe_instance_t;

struct dt_probe_instance {
	char pi_fname[DTRACE_FUNCNAMELEN]; /* function name */
	uint32_t *pi_offs;		/* offsets into the function */
	uint_t pi_noffs;		/* number of offsets */
	uint_t pi_maxoffs;		/* size of pi_offs allocation */
	dt_probe_instance_t *pi_next;	/* next instance in the list */
};

typedef struct dt_probe {
	dt_ident_t *pr_ident;		/* pointer to probe identifier */
	dt_node_t *pr_nargs;		/* native argument list */
	uint_t pr_nargc;		/* native argument count */
	dt_node_t *pr_xargs;		/* translated argument list */
	uint_t pr_xargc;		/* translated argument count */
	uint8_t *pr_mapping;		/* translated argument mapping */
	dt_probe_instance_t *pr_inst;	/* list of functions and offsets */
} dt_probe_t;

extern dt_provider_t *dt_provider_lookup(dtrace_hdl_t *, const char *);
extern dt_provider_t *dt_provider_create(dtrace_hdl_t *, const char *);
extern void dt_provider_destroy(dtrace_hdl_t *, dt_provider_t *);

extern dt_probe_t *dt_probe_lookup(dt_provider_t *, const char *);
extern dt_probe_t *dt_probe_create(dt_ident_t *,
    dt_node_t *, uint_t, dt_node_t *, uint_t);
extern void dt_probe_destroy(dt_ident_t *);
extern int dt_probe_add(dtrace_hdl_t *, dt_probe_t *, const char *,
    uint64_t);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_PROVIDER_H */

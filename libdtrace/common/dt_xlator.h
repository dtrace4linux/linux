/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_XLATOR_H
#define	_DT_XLATOR_H

#pragma ident	"@(#)dt_xlator.h	1.1	03/09/02 SMI"

#include <libctf.h>
#include <dtrace.h>
#include <dt_ident.h>
#include <dt_list.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct dt_node;

typedef struct dt_xlator {
	dt_list_t dx_list;		/* list forward/back pointers */
	dt_idhash_t *dx_locals;		/* hash of local scope identifiers */
	dt_ident_t *dx_ident;		/* identifier ref for input param */
	dt_ident_t dx_souid;		/* fake identifier for sou output */
	dt_ident_t dx_ptrid;		/* fake identifier for ptr output */
	ctf_file_t *dx_src_ctfp;	/* CTF container for input type */
	ctf_id_t dx_src_type;		/* CTF reference for input type */
	ctf_id_t dx_src_base;		/* CTF reference for input base */
	ctf_file_t *dx_dst_ctfp;	/* CTF container for output type */
	ctf_id_t dx_dst_type;		/* CTF reference for output type */
	ctf_id_t dx_dst_base;		/* CTF reference for output base */
	struct dt_node *dx_members;	/* list of member translations */
	struct dt_node *dx_nodes;	/* list of parse tree nodes */
	ulong_t dx_gen;			/* generation number that created me */
} dt_xlator_t;

extern dt_xlator_t *dt_xlator_create(dtrace_hdl_t *,
    const dtrace_typeinfo_t *, const dtrace_typeinfo_t *,
    const char *, struct dt_node *, struct dt_node *);

#define	DT_XLATE_FUZZY	0		/* lookup any matching translator */
#define	DT_XLATE_EXACT	1		/* lookup only exact type matches */

extern dt_xlator_t *dt_xlator_lookup(dtrace_hdl_t *,
    struct dt_node *, struct dt_node *, int);

extern dt_ident_t *dt_xlator_ident(dt_xlator_t *, ctf_file_t *, ctf_id_t);
extern struct dt_node *dt_xlator_member(dt_xlator_t *, const char *);
extern void dt_xlator_destroy(dtrace_hdl_t *, dt_xlator_t *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_XLATOR_H */

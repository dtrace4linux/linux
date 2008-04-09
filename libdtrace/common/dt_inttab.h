/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_INTTAB_H
#define	_DT_INTTAB_H

#pragma ident	"@(#)dt_inttab.h	1.1	04/11/13 SMI"

#include <dtrace.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dt_inthash {
	struct dt_inthash *inh_hash;	/* next dt_inthash in hash chain */
	struct dt_inthash *inh_next;	/* next dt_inthash in output table */
	uint64_t inh_value;		/* value associated with this element */
	uint_t inh_index;		/* index associated with this element */
	uint_t inh_flags;		/* flags (see below) */
} dt_inthash_t;

typedef struct dt_inttab {
	dtrace_hdl_t *int_hdl;		/* pointer back to library handle */
	dt_inthash_t **int_hash;	/* array of hash buckets */
	uint_t int_hashlen;		/* size of hash bucket array */
	uint_t int_nelems;		/* number of elements hashed */
	dt_inthash_t *int_head;		/* head of table in index order */
	dt_inthash_t *int_tail;		/* tail of table in index order */
	uint_t int_index;		/* next index to hand out */
} dt_inttab_t;

#define	DT_INT_PRIVATE	0		/* only a single ref for this entry */
#define	DT_INT_SHARED	1		/* multiple refs can share entry */

extern dt_inttab_t *dt_inttab_create(dtrace_hdl_t *);
extern void dt_inttab_destroy(dt_inttab_t *);
extern int dt_inttab_insert(dt_inttab_t *, uint64_t, uint_t);
extern uint_t dt_inttab_size(const dt_inttab_t *);
extern void dt_inttab_write(const dt_inttab_t *, uint64_t *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_INTTAB_H */

/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_LIST_H
#define	_DT_LIST_H

#pragma ident	"@(#)dt_list.h	1.1	03/09/02 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dt_list {
	struct dt_list *dl_prev;
	struct dt_list *dl_next;
} dt_list_t;

#define	dt_list_prev(elem)	((void *)(((dt_list_t *)(elem))->dl_prev))
#define	dt_list_next(elem)	((void *)(((dt_list_t *)(elem))->dl_next))

extern void dt_list_append(dt_list_t *, void *);
extern void dt_list_prepend(dt_list_t *, void *);
extern void dt_list_insert(dt_list_t *, void *, void *);
extern void dt_list_delete(dt_list_t *, void *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_LIST_H */

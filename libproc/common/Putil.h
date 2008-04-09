/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_PUTIL_H
#define	_PUTIL_H

#pragma ident	"@(#)Putil.h	1.2	04/07/15 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Circular doubly-linked list:
 */
typedef struct P_list {
	struct P_list	*list_forw;
	struct P_list	*list_back;
} list_t;

/*
 * Routines to manipulate linked lists:
 */
extern void list_link(void *, void *);
extern void list_unlink(void *);

#define	list_next(elem)	(void *)(((list_t *)(elem))->list_forw)
#define	list_prev(elem)	(void *)(((list_t *)(elem))->list_back)

/*
 * Routines to manipulate sigset_t, fltset_t, or sysset_t.
 */
extern void prset_fill(void *, size_t);
extern void prset_empty(void *, size_t);
extern void prset_add(void *, size_t, uint_t);
extern void prset_del(void *, size_t, uint_t);
extern int prset_ismember(void *, size_t, uint_t);

/*
 * Routine to print debug messages:
 */
extern void dprintf(const char *, ...);

#ifdef	__cplusplus
}
#endif

#endif	/* _PUTIL_H */

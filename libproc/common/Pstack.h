/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_PSTACK_H
#define	_PSTACK_H

#pragma ident	"@(#)Pstack.h	1.1	04/09/28 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Support functions for ISA-dependent Pstack_iter().
 */
int stack_loop(prgreg_t fp, prgreg_t **prevfpp, int *nfpp, uint_t *pfpsizep);

typedef struct {
	struct ps_prochandle *uc_proc;	/* libproc handle */
	uintptr_t *uc_addrs;		/* array of stack addresses */
	uint_t uc_nelems;		/* number of valid elements */
	uint_t uc_size;			/* actual size of array */
	uint_t uc_cached;		/* is cached in the ps_prochandle */
} uclist_t;

int load_uclist(uclist_t *ucl, const lwpstatus_t *psp);
int sort_uclist(const void *lhp, const void *rhp);
void init_uclist(uclist_t *ucl, struct ps_prochandle *P);
void free_uclist(uclist_t *ucl);
int find_uclink(uclist_t *ucl, uintptr_t addr);



#ifdef	__cplusplus
}
#endif

#endif	/* _PSTACK_H */

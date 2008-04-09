/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_AS_H
#define	_DT_AS_H

#pragma ident	"@(#)dt_as.h	1.1	03/09/02 SMI"

#include <sys/types.h>
#include <sys/dtrace.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct dt_ident;

typedef struct dt_irnode {
	uint_t di_label;		/* label number or DT_LBL_NONE */
	dif_instr_t di_instr;		/* instruction opcode */
	struct dt_ident *di_ident;	/* ident pointer for relocation */
	struct dt_irnode *di_next;	/* next instruction */
} dt_irnode_t;

#define	DT_LBL_NONE	0		/* no label on this instruction */

typedef struct dt_irlist {
	dt_irnode_t *dl_list;		/* pointer to first node in list */
	dt_irnode_t *dl_last;		/* pointer to last node in list */
	uint_t dl_len;			/* number of valid instructions */
	uint_t dl_label;		/* next label number to assign */
} dt_irlist_t;

extern void dt_irlist_create(dt_irlist_t *);
extern void dt_irlist_destroy(dt_irlist_t *);
extern void dt_irlist_append(dt_irlist_t *, dt_irnode_t *);
extern uint_t dt_irlist_label(dt_irlist_t *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_AS_H */

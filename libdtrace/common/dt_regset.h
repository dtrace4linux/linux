/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_REGSET_H
#define	_DT_REGSET_H

#pragma ident	"@(#)dt_regset.h	1.1	03/09/02 SMI"

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dt_regset {
	ulong_t dr_size;		/* number of registers in set */
	ulong_t *dr_bitmap;		/* bitmap of active registers */
} dt_regset_t;

extern dt_regset_t *dt_regset_create(ulong_t);
extern void dt_regset_destroy(dt_regset_t *);
extern void dt_regset_reset(dt_regset_t *);
extern int dt_regset_alloc(dt_regset_t *);
extern void dt_regset_free(dt_regset_t *, int);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_REGSET_H */

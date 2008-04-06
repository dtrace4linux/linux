/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_STRTAB_H
#define	_DT_STRTAB_H

#pragma ident	"@(#)dt_strtab.h	1.2	04/11/13 SMI"

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dt_strhash {
	const char *str_data;		/* pointer to actual string data */
	ulong_t str_buf;		/* index of string data buffer */
	size_t str_off;			/* offset in bytes of this string */
	size_t str_len;			/* length in bytes of this string */
	struct dt_strhash *str_next;	/* next string in hash chain */
} dt_strhash_t;

typedef struct dt_strtab {
	dt_strhash_t **str_hash;	/* array of hash buckets */
	ulong_t str_hashsz;		/* size of hash bucket array */
	char **str_bufs;		/* array of buffer pointers */
	char *str_ptr;			/* pointer to current buffer location */
	ulong_t str_nbufs;		/* size of buffer pointer array */
	size_t str_bufsz;		/* size of individual buffer */
	ulong_t str_nstrs;		/* total number of strings in strtab */
	size_t str_size;		/* total size of strings in bytes */
} dt_strtab_t;

typedef ssize_t dt_strtab_write_f(const char *, size_t, size_t, void *);

extern dt_strtab_t *dt_strtab_create(size_t);
extern void dt_strtab_destroy(dt_strtab_t *);
extern ssize_t dt_strtab_insert(dt_strtab_t *, const char *);
extern size_t dt_strtab_size(const dt_strtab_t *);
extern ssize_t dt_strtab_write(const dt_strtab_t *,
    dt_strtab_write_f *, void *);
extern ulong_t dt_strtab_hash(const char *, size_t *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_STRTAB_H */

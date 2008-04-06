/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_STRING_H
#define	_DT_STRING_H

#pragma ident	"@(#)dt_string.h	1.2	04/04/30 SMI"

#include <sys/types.h>
#include <strings.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern char *strndup(const char *, size_t);
extern size_t stresc2chr(char *);
extern char *strchr2esc(const char *, size_t);
extern const char *strbasename(const char *);
extern const char *strbadidnum(const char *);
extern int strisglob(const char *);
extern char *strhyphenate(char *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_STRING_H */

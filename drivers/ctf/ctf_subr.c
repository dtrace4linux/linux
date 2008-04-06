/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)ctf_subr.c	1.2	04/05/22 SMI"

#include <ctf_impl.h>
#include <sys/kobj.h>
#include <sys/kobj_impl.h>

/*
 * This module is used both during the normal operation of the kernel (i.e.
 * after kmem has been initialized) and during boot (before unix`_start has
 * been called).  kobj_alloc is able to tell the difference between the two
 * cases, and as such must be used instead of kmem_alloc.
 */

void *
ctf_data_alloc(size_t size)
{
	void *buf = kobj_alloc(size, KM_NOWAIT|KM_SCRATCH);

	if (buf == NULL)
		return (MAP_FAILED);

	return (buf);
}

void
ctf_data_free(void *buf, size_t size)
{
	kobj_free(buf, size);
}

/*ARGSUSED*/
void
ctf_data_protect(void *buf, size_t size)
{
	/* we don't support this operation in the kernel */
}

void *
ctf_alloc(size_t size)
{
	return (kobj_alloc(size, KM_NOWAIT|KM_TMP));
}

/*ARGSUSED*/
void
ctf_free(void *buf, size_t size)
{
	kobj_free(buf, size);
}

/*ARGSUSED*/
const char *
ctf_strerror(int err)
{
	return (NULL); /* we don't support this operation in the kernel */
}

/*PRINTFLIKE1*/
void
ctf_dprintf(const char *format, ...)
{
	if (_libctf_debug) {
		va_list alist;

		va_start(alist, format);
		(void) printf("ctf DEBUG: ");
		(void) vprintf(format, alist);
		va_end(alist);
	}
}

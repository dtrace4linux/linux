/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)ctf_subr.c	1.1	03/09/02 SMI"

#include <linux_types.h>
#include <ctf_impl.h>
#include <libctf.h>
#include <sys/mman.h>
#include <stdarg.h>

void *
ctf_data_alloc(size_t size)
{
	return (mmap(NULL, size, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_ANON, -1, 0));
}

void
ctf_data_free(void *buf, size_t size)
{
	(void) munmap(buf, size);
}

void
ctf_data_protect(void *buf, size_t size)
{
	(void) mprotect(buf, size, PROT_READ);
}

void *
ctf_alloc(size_t size)
{
	return (malloc(size));
}

/*ARGSUSED*/
void
ctf_free(void *buf, size_t size)
{
	free(buf);
}

const char *
ctf_strerror(int err)
{
	return (strerror(err));
}

/*PRINTFLIKE1*/
void
ctf_dprintf(const char *format, ...)
{
	if (_libctf_debug) {
		va_list alist;

		va_start(alist, format);
		(void) fputs("libctf DEBUG: ", stderr);
		(void) vfprintf(stderr, format, alist);
		va_end(alist);
	}
}

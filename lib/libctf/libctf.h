/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

/*
 * This header file defines the interfaces available from the CTF debugger
 * library, libctf.  This library provides functions that a debugger can
 * use to operate on data in the Compact ANSI-C Type Format (CTF).  This
 * is NOT a public interface, although it may eventually become one in
 * the fullness of time after we gain more experience with the interfaces.
 *
 * In the meantime, be aware that any program linked with libctf in this
 * release of Solaris is almost guaranteed to break in the next release.
 *
 * In short, do not user this header file or libctf for any purpose.
 */

#ifndef	_LIBCTF_H
#define	_LIBCTF_H

#pragma ident	"@(#)libctf.h	1.5	03/09/02 SMI"

#include <sys/ctf_api.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * This flag can be used to enable debug messages.
 */
extern int _libctf_debug;

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCTF_H */

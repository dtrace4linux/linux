/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_DT_MODULE_H
#define	_DT_MODULE_H

#pragma ident	"@(#)dt_module.h	1.2	04/09/27 SMI"

#include <dt_impl.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern dt_module_t *dt_module_create(dtrace_hdl_t *, const char *);
extern int dt_module_load(dtrace_hdl_t *, dt_module_t *);
extern void dt_module_unload(dtrace_hdl_t *, dt_module_t *);
extern void dt_module_destroy(dtrace_hdl_t *, dt_module_t *);

extern dt_module_t *dt_module_lookup_by_name(dtrace_hdl_t *, const char *);
extern dt_module_t *dt_module_lookup_by_ctf(dtrace_hdl_t *, ctf_file_t *);

extern ctf_file_t *dt_module_getctf(dtrace_hdl_t *, dt_module_t *);
extern dt_ident_t *dt_module_extern(dtrace_hdl_t *, dt_module_t *,
    const char *, const dtrace_typeinfo_t *);

extern const char *dt_module_modelname(dt_module_t *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_MODULE_H */

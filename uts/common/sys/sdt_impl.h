/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef _SYS_SDT_IMPL_H
#define	_SYS_SDT_IMPL_H

#pragma ident	"@(#)sdt_impl.h	1.2	04/09/28 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/dtrace.h>

#if defined(__i386) || defined(__amd64)
typedef uint8_t sdt_instr_t;
#else
typedef uint32_t sdt_instr_t;
#endif

typedef struct sdt_provider {
	char			*sdtp_name;	/* name of provider */
	char			*sdtp_prefix;	/* prefix for probe names */
	dtrace_pattr_t		*sdtp_attr;	/* stability attributes */
	dtrace_provider_id_t	sdtp_id;	/* provider ID */
} sdt_provider_t;

extern sdt_provider_t sdt_providers[];		/* array of providers */

typedef struct sdt_probe {
	sdt_provider_t	*sdp_provider;		/* provider */
	char		*sdp_name;		/* name of probe */
	int		sdp_namelen;		/* length of allocated name */
	dtrace_id_t	sdp_id;			/* probe ID */
	struct modctl	*sdp_ctl;		/* modctl for module */
	int		sdp_loadcnt;		/* load count for module */
	int		sdp_primary;		/* non-zero if primary mod */
	sdt_instr_t	*sdp_patchpoint;	/* patch point */
	sdt_instr_t	sdp_patchval;		/* instruction to patch */
	sdt_instr_t	sdp_savedval;		/* saved instruction value */
	struct sdt_probe *sdp_next;		/* next probe */
	struct sdt_probe *sdp_hashnext;		/* next on hash */
} sdt_probe_t;

typedef struct sdt_argdesc {
	const char *sda_provider;		/* provider for arg */
	const char *sda_name;			/* name of probe */
	const int sda_ndx;			/* argument index */
	const int sda_mapping;			/* mapping of argument */
	const char *sda_native;			/* native type of argument */
	const char *sda_xlate;			/* translated type of arg */
} sdt_argdesc_t;

extern void sdt_getargdesc(void *, dtrace_id_t, void *, dtrace_argdesc_t *);

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_SDT_IMPL_H */

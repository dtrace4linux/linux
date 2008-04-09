/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)dt_dof.c	1.6	04/11/13 SMI"

#include <linux_types.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <strings.h>
#include <alloca.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <dt_impl.h>
#include <dt_strtab.h>
#include <dt_provider.h>

typedef struct dt_dof {
	uchar_t *ddo_buf;	/* DOF data buffer base address */
	dof_sec_t *ddo_secp;	/* DOF section header table pointer */
	uchar_t *ddo_lptr;	/* DOF buffer pointer for loadable data */
	uchar_t *ddo_uptr;	/* DOF buffer pointer for unloadable data */
	size_t ddo_lsize;	/* size required for loadable DOF data */
	size_t ddo_usize;	/* size required for unloadable DOF data */
	size_t ddo_fsize;	/* size required for entire DOF file */
	uint_t ddo_secs;	/* number of DOF sections */
	size_t ddo_strs;	/* size of global string table */
} dt_dof_t;

/*
 * Add a loadable DOF section to the file using the specified data buffer and
 * the specified DOF section attributes.  DOF_SECF_LOAD must be set in flags.
 */
static dof_secidx_t
dof_add_lsect(dt_dof_t *ddo, const void *data, uint32_t type,
    uint32_t align, uint32_t flags, uint32_t entsize, uint64_t size)
{
	assert(flags & DOF_SECF_LOAD);
	ddo->ddo_lptr = (uchar_t *)roundup((uintptr_t)ddo->ddo_lptr, align);

	ddo->ddo_secp->dofs_type = type;
	ddo->ddo_secp->dofs_align = align;
	ddo->ddo_secp->dofs_flags = flags;
	ddo->ddo_secp->dofs_entsize = entsize;
	ddo->ddo_secp->dofs_offset = (size_t)(ddo->ddo_lptr - ddo->ddo_buf);
	ddo->ddo_secp->dofs_size = size;

	bcopy(data, ddo->ddo_lptr, size);
	ddo->ddo_lptr += size;

	ddo->ddo_secp++;
	return (ddo->ddo_secs++);
}

#ifdef _when_needed
/*
 * Add a unloadable DOF section to the file using the specified data buffer
 * and DOF section attributes.  DOF_SECF_LOAD must *not* be set in flags.
 */
static dof_secidx_t
dof_add_usect(dt_dof_t *ddo, const void *data, uint32_t type,
    uint32_t align, uint32_t flags, uint32_t entsize, uint64_t size)
{
	assert(!(flags & DOF_SECF_LOAD));
	ddo->ddo_uptr = (uchar_t *)roundup((uintptr_t)ddo->ddo_uptr, align);

	ddo->ddo_secp->dofs_type = type;
	ddo->ddo_secp->dofs_align = align;
	ddo->ddo_secp->dofs_flags = flags;
	ddo->ddo_secp->dofs_entsize = entsize;
	ddo->ddo_secp->dofs_offset = (size_t)(ddo->ddo_uptr - ddo->ddo_buf);
	ddo->ddo_secp->dofs_size = size;

	bcopy(data, ddo->ddo_uptr, size);
	ddo->ddo_uptr += size;

	ddo->ddo_secp++;
	return (ddo->ddo_secs++);
}
#endif

static void
dof_difo_size(dt_dof_t *ddo, const dtrace_difo_t *dp)
{
	uint_t osecs = ddo->ddo_secs;

	if (dp->dtdo_buf != NULL) {
		ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (dif_instr_t)) +
		    sizeof (dif_instr_t) * dp->dtdo_len;
		ddo->ddo_secs++; /* DOF_SECT_DIF */
	}

	if (dp->dtdo_inttab != NULL) {
		ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (uint64_t)) +
		    sizeof (uint64_t) * dp->dtdo_intlen;
		ddo->ddo_secs++; /* DOF_SECT_INTTAB */
	}

	if (dp->dtdo_strtab != NULL) {
		ddo->ddo_lsize += dp->dtdo_strlen;
		ddo->ddo_secs++; /* DOF_SECT_STRTAB */
	}

	if (dp->dtdo_vartab != NULL) {
		ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (uint_t)) +
		    sizeof (dtrace_difv_t) * dp->dtdo_varlen;
		ddo->ddo_secs++; /* DOF_SECT_VARTAB */
	}

	/*
	 * The DOF_SECT_DIFOHDR should have room for one dtrace_diftype_t and
	 * then one dof_secidx_t for each section header reserved so far.
	 */
	ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (dof_secidx_t)) +
	    sizeof (dtrace_diftype_t) +
	    sizeof (dof_secidx_t) * (ddo->ddo_secs - osecs);
	ddo->ddo_secs++; /* DOF_SECT_DIFOHDR */

	/*
	 * Now allocate space for any kernel and user relocation sections.
	 * These sections refer to the DIFO, but are not mentioned in DIFOHDR.
	 */
	if (dp->dtdo_kreltab != NULL) {
		ddo->ddo_lsize =
		    roundup(ddo->ddo_lsize, sizeof (uint64_t)) +
		    sizeof (dof_relodesc_t) * dp->dtdo_krelen;
		ddo->ddo_secs++; /* DOF_SECT_RELTAB */

		ddo->ddo_lsize = roundup(ddo->ddo_lsize,
		    sizeof (dof_secidx_t)) + sizeof (dof_relohdr_t);
		ddo->ddo_secs++; /* DOF_SECT_KRELHDR */
	}

	if (dp->dtdo_ureltab != NULL) {
		ddo->ddo_lsize =
		    roundup(ddo->ddo_lsize, sizeof (uint64_t)) +
		    sizeof (dof_relodesc_t) * dp->dtdo_urelen;
		ddo->ddo_secs++; /* DOF_SECT_RELTAB */

		ddo->ddo_lsize = roundup(ddo->ddo_lsize,
		    sizeof (dof_secidx_t)) + sizeof (dof_relohdr_t);
		ddo->ddo_secs++; /* DOF_SECT_URELHDR */
	}
}

static dof_secidx_t
dof_difo_save(dt_dof_t *ddo, const dtrace_difo_t *dp)
{
	dof_secidx_t dsecs[4]; /* enough for all possible DIFO sections */
	uint_t nsecs = 0;

	dof_difohdr_t *dofd;
	dof_relohdr_t dofr;
	dof_secidx_t relsec;

	dof_secidx_t strsec = DOF_SECIDX_NONE;
	dof_secidx_t intsec = DOF_SECIDX_NONE;
	dof_secidx_t hdrsec = DOF_SECIDX_NONE;

	if (dp->dtdo_buf != NULL) {
		dsecs[nsecs++] = dof_add_lsect(ddo, dp->dtdo_buf,
		    DOF_SECT_DIF, sizeof (dif_instr_t), DOF_SECF_LOAD,
		    sizeof (dif_instr_t), sizeof (dif_instr_t) * dp->dtdo_len);
	}

	if (dp->dtdo_inttab != NULL) {
		dsecs[nsecs++] = intsec = dof_add_lsect(ddo, dp->dtdo_inttab,
		    DOF_SECT_INTTAB, sizeof (uint64_t), DOF_SECF_LOAD,
		    sizeof (uint64_t), sizeof (uint64_t) * dp->dtdo_intlen);
	}

	if (dp->dtdo_strtab != NULL) {
		dsecs[nsecs++] = strsec = dof_add_lsect(ddo, dp->dtdo_strtab,
		    DOF_SECT_STRTAB, sizeof (char), DOF_SECF_LOAD,
		    0, dp->dtdo_strlen);
	}

	if (dp->dtdo_vartab != NULL) {
		dsecs[nsecs++] = dof_add_lsect(ddo, dp->dtdo_vartab,
		    DOF_SECT_VARTAB, sizeof (uint_t), DOF_SECF_LOAD,
		    sizeof (dtrace_difv_t),
		    sizeof (dtrace_difv_t) * dp->dtdo_varlen);
	}

	/*
	 * Copy the return type and the array of section indices that form the
	 * DIFO into a single dof_difohdr_t and then add DOF_SECT_DIFOHDR.
	 */
	assert(nsecs <= sizeof (dsecs) / sizeof (dsecs[0]));
	dofd = alloca(sizeof (dtrace_diftype_t) + sizeof (dsecs));
	bcopy(&dp->dtdo_rtype, &dofd->dofd_rtype, sizeof (dtrace_diftype_t));
	bcopy(dsecs, &dofd->dofd_links, sizeof (dof_secidx_t) * nsecs);

	hdrsec = dof_add_lsect(ddo, dofd, DOF_SECT_DIFOHDR,
	    sizeof (dof_secidx_t), DOF_SECF_LOAD, 0,
	    sizeof (dtrace_diftype_t) + sizeof (dof_secidx_t) * nsecs);

	/*
	 * Add any other sections related to dtrace_difo_t.  These are not
	 * referenced in dof_difohdr_t because they are not used by emulation.
	 */
	if (dp->dtdo_kreltab != NULL) {
		relsec = dof_add_lsect(ddo, dp->dtdo_kreltab, DOF_SECT_RELTAB,
		    sizeof (uint64_t), DOF_SECF_LOAD, sizeof (dof_relodesc_t),
		    sizeof (dof_relodesc_t) * dp->dtdo_krelen);

		/*
		 * This code assumes the target of all relocations is the
		 * integer table 'intsec' (DOF_SECT_INTTAB).  If other sections
		 * need relocation in the future this will need to change.
		 */
		dofr.dofr_strtab = strsec;
		dofr.dofr_relsec = relsec;
		dofr.dofr_tgtsec = intsec;

		(void) dof_add_lsect(ddo, &dofr, DOF_SECT_KRELHDR,
		    sizeof (dof_secidx_t), DOF_SECF_LOAD, 0,
		    sizeof (dof_relohdr_t));
	}

	if (dp->dtdo_ureltab != NULL) {
		relsec = dof_add_lsect(ddo, dp->dtdo_ureltab, DOF_SECT_RELTAB,
		    sizeof (uint64_t), DOF_SECF_LOAD, sizeof (dof_relodesc_t),
		    sizeof (dof_relodesc_t) * dp->dtdo_urelen);

		/*
		 * This code assumes the target of all relocations is the
		 * integer table 'intsec' (DOF_SECT_INTTAB).  If other sections
		 * need relocation in the future this will need to change.
		 */
		dofr.dofr_strtab = strsec;
		dofr.dofr_relsec = relsec;
		dofr.dofr_tgtsec = intsec;

		(void) dof_add_lsect(ddo, &dofr, DOF_SECT_URELHDR,
		    sizeof (dof_secidx_t), DOF_SECF_LOAD, 0,
		    sizeof (dof_relohdr_t));
	}

	return (hdrsec);
}

typedef struct dof_probe_info {
	dof_probe_t *dpi_probes;	/* array of DOF probes */
	size_t dpi_nprobes;		/* probe count */
	uint8_t *dpi_args;		/* array of argument mappings */
	size_t dpi_nargs;		/* argument count */
	uint32_t *dpi_offs;		/* array of function offsets */
	size_t dpi_noffs;		/* offset count */
	char *dpi_strtab;		/* string table */
	size_t dpi_strlen;		/* string table size */
	dof_relodesc_t *dpi_rel;	/* DOF relocations */
} dof_probe_info_t;

/*ARGSUSED*/
static void
dof_probe_size(dt_idhash_t *dhp, dt_ident_t *idp, void *data)
{
	dof_probe_info_t *dpip = data;
	dt_probe_instance_t *pip;
	dt_probe_t *prp = idp->di_data;
	char buf[256];
	dt_node_t *dnp;
	int i;

	dpip->dpi_strlen += strlen(prp->pr_ident->di_name) + 1;

	for (i = 0, dnp = prp->pr_nargs; i < prp->pr_nargc;
	    i++, dnp = dnp->dn_list) {
		assert(dnp->dn_kind == DT_NODE_TYPE);

		(void) ctf_type_name(dnp->dn_ctfp, dnp->dn_type, buf,
		    sizeof (buf));

		dpip->dpi_strlen += strlen(buf) + 1;
	}

	for (i = 0, dnp = prp->pr_xargs; i < prp->pr_xargc;
	    i++, dnp = dnp->dn_list) {
		assert(dnp->dn_kind == DT_NODE_TYPE);

		(void) ctf_type_name(dnp->dn_ctfp, dnp->dn_type, buf,
		    sizeof (buf));

		dpip->dpi_strlen += strlen(buf) + 1;
		dpip->dpi_nargs++;
	}

	for (pip = prp->pr_inst; pip != NULL; pip = pip->pi_next) {
		dpip->dpi_strlen += strlen(pip->pi_fname) + 1;
		dpip->dpi_nprobes++;
		dpip->dpi_noffs += pip->pi_noffs;
	}
}

static void
dof_provider_size(dt_dof_t *ddo, const dt_provider_t *pvp)
{
	dof_probe_info_t dpi;

	/*
	 * Ignore providers that aren't user defined.
	 */
	if (pvp->pv_probes == NULL)
		return;

	dpi.dpi_nprobes = 0;
	dpi.dpi_nargs = 0;
	dpi.dpi_noffs = 0;
	dpi.dpi_strlen = 1; /* leading '\0' */

	dpi.dpi_strlen += strlen(pvp->pv_desc.dtvd_name) + 1;

	(void) dt_idhash_iter(pvp->pv_probes, dof_probe_size, &dpi);

	ddo->ddo_lsize += dpi.dpi_strlen;
	ddo->ddo_secs++; /* DOF_SECT_STRTAB */

	ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (uint64_t));
	ddo->ddo_lsize += sizeof (dof_probe_t) * dpi.dpi_nprobes;
	ddo->ddo_secs++; /* DOF_SECT_PROBES */

	ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (uint_t));
	ddo->ddo_lsize += sizeof (uint8_t) * dpi.dpi_nargs;
	ddo->ddo_secs++; /* DOF_SECT_PRARGS */

	ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (uint_t));
	ddo->ddo_lsize += sizeof (uint32_t) * dpi.dpi_noffs;
	ddo->ddo_secs++; /* DOF_SECT_PROFFS */

	ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (uint_t)) +
	    sizeof (dof_provider_t);
	ddo->ddo_secs++; /* DOF_SECT_PROVIDER */

	ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (uint64_t));
	ddo->ddo_lsize += sizeof (dof_relodesc_t) * dpi.dpi_nprobes;
	ddo->ddo_secs++; /* DOF_SECT_RELTAB */

	ddo->ddo_lsize = roundup(ddo->ddo_lsize, sizeof (dof_secidx_t));
	ddo->ddo_lsize += sizeof (dof_relohdr_t);
	ddo->ddo_secs++; /* DOF_SECT_URELHDR */
}

static uint8_t
dof_find_arg(dt_node_t *argv, uint_t argc, const char *name)
{
	uint8_t i = 0;

	for (i = 0; i < argc; i++, argv = argv->dn_list) {
		assert(argv->dn_kind == DT_NODE_TYPE);
		if (strcmp(argv->dn_string, name) == 0)
			return (i);
	}

	/* Corresponding argument not found */
	assert(0);
	return ((uint8_t)-1);
}

/*ARGSUSED*/
static void
dof_probe_save(dt_idhash_t *dhp, dt_ident_t *idp, void *data)
{
	dof_probe_info_t *dpip = data;
	dt_probe_instance_t *pip;
	dt_probe_t *prp = idp->di_data;
	dof_stridx_t name, nargv, xargv;
	uint32_t argidx;
	dt_node_t *dnp;
	char buf[256];
	int i;

	name = dpip->dpi_strlen;
	(void) strcpy(dpip->dpi_strtab + dpip->dpi_strlen,
	    prp->pr_ident->di_name);
	dpip->dpi_strlen += strlen(prp->pr_ident->di_name) + 1;

	nargv = dpip->dpi_strlen;
	for (i = 0, dnp = prp->pr_nargs; i < prp->pr_nargc;
	    i++, dnp = dnp->dn_list) {
		assert(dnp->dn_kind == DT_NODE_TYPE);

		(void) ctf_type_name(dnp->dn_ctfp, dnp->dn_type, buf,
		    sizeof (buf));

		(void) strcpy(dpip->dpi_strtab + dpip->dpi_strlen, buf);
		dpip->dpi_strlen += strlen(buf) + 1;
	}

	xargv = dpip->dpi_strlen;
	argidx = dpip->dpi_nargs;
	for (i = 0, dnp = prp->pr_xargs; i < prp->pr_xargc;
	    i++, dnp = dnp->dn_list) {
		assert(dnp->dn_kind == DT_NODE_TYPE);

		(void) ctf_type_name(dnp->dn_ctfp, dnp->dn_type, buf,
		    sizeof (buf));

		(void) strcpy(dpip->dpi_strtab + dpip->dpi_strlen, buf);
		dpip->dpi_strlen += strlen(buf) + 1;

		dpip->dpi_args[dpip->dpi_nargs++] = (dnp->dn_string == NULL) ?
		    i : dof_find_arg(prp->pr_nargs, prp->pr_nargc,
		    dnp->dn_string);
	}

	for (pip = prp->pr_inst; pip != NULL; pip = pip->pi_next) {
		dof_probe_t *dofprp = &dpip->dpi_probes[dpip->dpi_nprobes];
		dof_relodesc_t *dofrp = &dpip->dpi_rel[dpip->dpi_nprobes];
		int i;

		dpip->dpi_nprobes++;

		dofprp->dofpr_addr = 0;
		dofprp->dofpr_func = dpip->dpi_strlen;
		(void) strcpy(dpip->dpi_strtab + dpip->dpi_strlen,
		    pip->pi_fname);
		dpip->dpi_strlen += strlen(pip->pi_fname) + 1;

		dofprp->dofpr_name = name;
		dofprp->dofpr_nargv = nargv;
		dofprp->dofpr_xargv = xargv;

		dofprp->dofpr_argidx = argidx;

		dofprp->dofpr_nargc = prp->pr_nargc;
		dofprp->dofpr_xargc = prp->pr_xargc;

		dofprp->dofpr_offidx = dpip->dpi_noffs;
		dofprp->dofpr_noffs = pip->pi_noffs;
		for (i = 0; i < dofprp->dofpr_noffs; i++) {
			dpip->dpi_offs[dpip->dpi_noffs++] = pip->pi_offs[i];
		}

		dofrp->dofr_name = dofprp->dofpr_func;
		dofrp->dofr_type = DOF_RELO_SETX;
		dofrp->dofr_offset = (char *)&dofprp->dofpr_addr -
		    (char *)dpip->dpi_probes;
	}
}

static dof_attr_t
dof_attr(const dtrace_attribute_t *ap)
{
	return (DOF_ATTR(ap->dtat_name, ap->dtat_data, ap->dtat_class));
}

static int
dof_provider_save(dtrace_hdl_t *dtp, dt_dof_t *ddo, const dt_provider_t *pvp)
{
	dof_provider_t dofpv;
	dof_probe_info_t dpi;
	dof_relohdr_t dofr;

	/*
	 * Ignore providers that aren't user defined.
	 */
	if (pvp->pv_probes == NULL)
		return (0);

	dofpv.dofpv_provattr = dof_attr(&pvp->pv_desc.dtvd_attr.dtpa_provider);
	dofpv.dofpv_modattr = dof_attr(&pvp->pv_desc.dtvd_attr.dtpa_mod);
	dofpv.dofpv_funcattr = dof_attr(&pvp->pv_desc.dtvd_attr.dtpa_func);
	dofpv.dofpv_nameattr = dof_attr(&pvp->pv_desc.dtvd_attr.dtpa_name);
	dofpv.dofpv_argsattr = dof_attr(&pvp->pv_desc.dtvd_attr.dtpa_args);

	dpi.dpi_nprobes = 0;
	dpi.dpi_nargs = 0;
	dpi.dpi_noffs = 0;
	dpi.dpi_strlen = 1; /* leading '\0' */

	dpi.dpi_strlen += strlen(pvp->pv_desc.dtvd_name) + 1;

	(void) dt_idhash_iter(pvp->pv_probes, dof_probe_size, &dpi);

	if ((dpi.dpi_probes = calloc(dpi.dpi_nprobes,
	    sizeof (dof_probe_t))) == NULL) {
		return (dt_set_errno(dtp, EDT_NOMEM));
	}

	if ((dpi.dpi_args = calloc(dpi.dpi_nargs, sizeof (uint8_t))) == NULL) {
		free(dpi.dpi_probes);
		return (dt_set_errno(dtp, EDT_NOMEM));
	}

	if ((dpi.dpi_offs = calloc(dpi.dpi_noffs, sizeof (uint32_t))) == NULL) {
		free(dpi.dpi_probes);
		free(dpi.dpi_args);
		return (dt_set_errno(dtp, EDT_NOMEM));
	}

	if ((dpi.dpi_strtab = malloc(dpi.dpi_strlen)) == NULL) {
		free(dpi.dpi_probes);
		free(dpi.dpi_args);
		free(dpi.dpi_offs);
		return (dt_set_errno(dtp, EDT_NOMEM));
	}

	if ((dpi.dpi_rel = calloc(dpi.dpi_nprobes,
	    sizeof (dof_relodesc_t))) == NULL) {
		free(dpi.dpi_probes);
		free(dpi.dpi_args);
		free(dpi.dpi_offs);
		free(dpi.dpi_strtab);
		return (dt_set_errno(dtp, EDT_NOMEM));
	}

	dpi.dpi_nprobes = 0;
	dpi.dpi_nargs = 0;
	dpi.dpi_noffs = 0;
	dpi.dpi_strtab[0] = '\0';
	dpi.dpi_strlen = 1;

	dofpv.dofpv_name = dpi.dpi_strlen;
	(void) strcpy(dpi.dpi_strtab + dpi.dpi_strlen, pvp->pv_desc.dtvd_name);
	dpi.dpi_strlen += strlen(pvp->pv_desc.dtvd_name) + 1;

	(void) dt_idhash_iter(pvp->pv_probes, dof_probe_save, &dpi);

	dofpv.dofpv_strtab = dof_add_lsect(ddo, dpi.dpi_strtab,
	    DOF_SECT_STRTAB, sizeof (char), DOF_SECF_LOAD, 0, dpi.dpi_strlen);

	dofpv.dofpv_probes = dof_add_lsect(ddo, dpi.dpi_probes,
	    DOF_SECT_PROBES, sizeof (uint64_t), DOF_SECF_LOAD,
	    sizeof (dof_probe_t), sizeof (dof_probe_t) * dpi.dpi_nprobes);

	dofpv.dofpv_prargs = dof_add_lsect(ddo, dpi.dpi_args,
	    DOF_SECT_PRARGS, sizeof (uint_t), DOF_SECF_LOAD,
	    sizeof (uint8_t), sizeof (uint8_t) * dpi.dpi_nargs);

	dofpv.dofpv_proffs = dof_add_lsect(ddo, dpi.dpi_offs,
	    DOF_SECT_PROFFS, sizeof (uint_t), DOF_SECF_LOAD,
	    sizeof (uint32_t), sizeof (uint32_t) * dpi.dpi_noffs);

	(void) dof_add_lsect(ddo, &dofpv, DOF_SECT_PROVIDER, sizeof (uint_t),
	    DOF_SECF_LOAD, 0, sizeof (dof_provider_t));

	dofr.dofr_strtab = dofpv.dofpv_strtab;
	dofr.dofr_relsec = dof_add_lsect(ddo, dpi.dpi_rel, DOF_SECT_RELTAB,
	    sizeof (uint64_t), DOF_SECF_LOAD, sizeof (dof_relodesc_t),
	    sizeof (dof_relodesc_t) * dpi.dpi_nprobes);
	dofr.dofr_tgtsec = dofpv.dofpv_probes;

	(void) dof_add_lsect(ddo, &dofr, DOF_SECT_URELHDR,
	    sizeof (dof_secidx_t), DOF_SECF_LOAD, 0, sizeof (dof_relohdr_t));

	free(dpi.dpi_probes);
	free(dpi.dpi_args);
	free(dpi.dpi_offs);
	free(dpi.dpi_strtab);
	free(dpi.dpi_rel);

	return (0);
}

static int
dtrace_dof_hdr(dtrace_hdl_t *dtp, dof_hdr_t *hp)
{
	/*
	 * If our config values cannot fit in a uint8_t, we can't generate a
	 * DOF header since the values won't fit.  This can only happen if the
	 * user forcibly compiles a program with an artificial configuration.
	 */
	if (dtp->dt_conf.dtc_difversion > UINT8_MAX ||
	    dtp->dt_conf.dtc_difintregs > UINT8_MAX ||
	    dtp->dt_conf.dtc_diftupregs > UINT8_MAX) {
		return (dt_set_errno(dtp, EOVERFLOW));
	}

	bzero(hp, sizeof (dof_hdr_t));

	hp->dofh_ident[DOF_ID_MAG0] = DOF_MAG_MAG0;
	hp->dofh_ident[DOF_ID_MAG1] = DOF_MAG_MAG1;
	hp->dofh_ident[DOF_ID_MAG2] = DOF_MAG_MAG2;
	hp->dofh_ident[DOF_ID_MAG3] = DOF_MAG_MAG3;

	if (dtp->dt_conf.dtc_ctfmodel == CTF_MODEL_LP64)
		hp->dofh_ident[DOF_ID_MODEL] = DOF_MODEL_LP64;
	else
		hp->dofh_ident[DOF_ID_MODEL] = DOF_MODEL_ILP32;

	hp->dofh_ident[DOF_ID_ENCODING] = DOF_ENCODE_NATIVE;
	hp->dofh_ident[DOF_ID_VERSION] = DOF_VERSION_1;
	hp->dofh_ident[DOF_ID_DIFVERS] = dtp->dt_conf.dtc_difversion;
	hp->dofh_ident[DOF_ID_DIFIREG] = dtp->dt_conf.dtc_difintregs;
	hp->dofh_ident[DOF_ID_DIFTREG] = dtp->dt_conf.dtc_diftupregs;

	hp->dofh_flags = 0;
	hp->dofh_hdrsize = sizeof (dof_hdr_t);
	hp->dofh_secsize = sizeof (dof_sec_t);
	hp->dofh_secoff = sizeof (dof_hdr_t);
	hp->dofh_pad = 0;

	return (0);
}

void *
dtrace_dof_create(dtrace_hdl_t *dtp, dtrace_prog_t *pgp, uint_t flags)
{
	const dtrace_ecbdesc_t *edp, *last;
	const dtrace_probedesc_t *pdp;
	const dtrace_actdesc_t *ap;
	const dt_stmt_t *stp, *next;

	dof_secidx_t strsec;
	char *strtab, *strp;
	size_t stroff;
	dof_hdr_t *hp;
	dof_sec_t *secbase;
	dt_dof_t ddo;
	dof_stridx_t strndx;
	dof_secidx_t *actsecs;
	uint_t maxacts = 0;

	if (flags & ~DTRACE_D_MASK) {
		(void) dt_set_errno(dtp, EINVAL);
		return (NULL);
	}

	bzero(&ddo, sizeof (dt_dof_t));
	ddo.ddo_lsize = sizeof (dof_hdr_t);

	/*
	 * First, iterate through the statements and compute the total size of
	 * the DOF file we will need to create.  We use the statements to find
	 * the unique set of ECB descriptions associated with the program,
	 * each of which results in a set of sections in the DOF file.  To that
	 * we must add a file header, section header table, and string table.
	 */
	stp = dt_list_next(&pgp->dp_stmts);
	last = NULL;

	for (; stp != NULL; stp = dt_list_next(stp)) {
		uint_t nacts = 0;
		dtrace_stmtdesc_t *sdp = stp->ds_desc;
		dtrace_actdesc_t *ap = sdp->dtsd_action;

		/*
		 * If the action associated with this statement has format
		 * data, we must allocate space for it in the string table.
		 */
		if (ap != NULL && sdp->dtsd_fmtdata != NULL) {
			ddo.ddo_strs += dtrace_printf_format(dtp,
			    sdp->dtsd_fmtdata, NULL, 0) + 1;
		}

		if ((edp = sdp->dtsd_ecbdesc) == last)
			continue; /* same ecb as previous statement */

		pdp = &edp->dted_probe;
		last = edp;

		ddo.ddo_lsize = roundup(ddo.ddo_lsize, sizeof (dof_secidx_t)) +
		    sizeof (dof_probedesc_t);
		ddo.ddo_secs++; /* DOF_SECT_PROBEDESC */

		if (pdp->dtpd_provider[0] != '\0')
			ddo.ddo_strs += strlen(pdp->dtpd_provider) + 1;
		if (pdp->dtpd_mod[0] != '\0')
			ddo.ddo_strs += strlen(pdp->dtpd_mod) + 1;
		if (pdp->dtpd_func[0] != '\0')
			ddo.ddo_strs += strlen(pdp->dtpd_func) + 1;
		if (pdp->dtpd_name[0] != '\0')
			ddo.ddo_strs += strlen(pdp->dtpd_name) + 1;

		if (edp->dted_pred.dtpdd_difo != NULL)
			dof_difo_size(&ddo, edp->dted_pred.dtpdd_difo);

		for (ap = edp->dted_action; ap != NULL; ap = ap->dtad_next) {
			if (ap->dtad_difo != NULL)
				dof_difo_size(&ddo, ap->dtad_difo);
			nacts++;
		}

		maxacts = MAX(maxacts, nacts); /* track biggest ecbdesc */

		if (nacts != 0) {
			ddo.ddo_lsize = roundup(ddo.ddo_lsize,
			    sizeof (uint64_t)) + sizeof (dof_actdesc_t) * nacts;
			ddo.ddo_secs++; /* DOF_SECT_ACTDESC */
		}

		ddo.ddo_lsize = roundup(ddo.ddo_lsize, sizeof (uint64_t)) +
		    sizeof (dof_ecbdesc_t);
		ddo.ddo_secs++; /* DOF_SECT_ECBDESC */
	}

	if (flags & DTRACE_D_PROBES) {
		uint_t h;
		dt_provider_t *pvp;

		for (h = 0; h < dtp->dt_provbuckets; h++) {
			for (pvp = dtp->dt_provs[h]; pvp != NULL;
			    pvp = pvp->pv_next) {
				dof_provider_size(&ddo, pvp);
			}
		}
	}

	ddo.ddo_lsize += ddo.ddo_strs + 1;
	ddo.ddo_secs++; /* DOF_SECT_STRTAB */

	/*
	 * Now that we've added up all the sections, add in the size of the
	 * section header table.  We know that the dof_hdr_t is aligned so that
	 * that dof_sec_t's can follow it, and that dof_sec_t is aligned so
	 * that the first section (PROBEDESC) can follow it.  As a result,
	 * no extra padding bytes are required between either of these things.
	 */
	ddo.ddo_lsize += sizeof (dof_sec_t) * ddo.ddo_secs;
	stroff = ddo.ddo_lsize - (ddo.ddo_strs + 1);

	/*
	 * If our DOF file has unloadable data (ddo_usize), then round up
	 * the loadable size to a 64-byte boundary and include it in the
	 * final file size.  Otherwise the file size is the loadable size.
	 */
	if (ddo.ddo_usize != 0)
		ddo.ddo_fsize = roundup(ddo.ddo_lsize, 64) + ddo.ddo_usize;
	else
		ddo.ddo_fsize = ddo.ddo_lsize;

	/*
	 * Allocate a buffer for the entire DOF file, bzero it so padding does
	 * not "leak" uninitialized memory values, and then fill in the header.
	 */
	if ((ddo.ddo_buf = malloc(ddo.ddo_fsize)) == NULL) {
		(void) dt_set_errno(dtp, EDT_NOMEM);
		return (NULL);
	}

	dt_dprintf("allocated DOF buffer of size %lu bytes\n",
	    (ulong_t)ddo.ddo_fsize);
	bzero(ddo.ddo_buf, ddo.ddo_fsize);
	hp = (dof_hdr_t *)(uintptr_t)ddo.ddo_buf;

	secbase = (dof_sec_t *)((uintptr_t)ddo.ddo_buf + sizeof (dof_hdr_t));

	ddo.ddo_secp = secbase;
	ddo.ddo_lptr = (uchar_t *)(secbase + ddo.ddo_secs);

	if (dtrace_dof_hdr(dtp, hp) != 0) {
		free(ddo.ddo_buf);
		return (NULL);
	}

	hp->dofh_secnum = ddo.ddo_secs;
	hp->dofh_loadsz = ddo.ddo_lsize;
	hp->dofh_filesz = ddo.ddo_fsize;

	if (ddo.ddo_usize != 0) {
		ddo.ddo_uptr = (uchar_t *)
		    ddo.ddo_buf + roundup(ddo.ddo_lsize, 64);
	} else {
		ddo.ddo_uptr = (uchar_t *)
		    ddo.ddo_buf + ddo.ddo_lsize;
	}

	/*
	 * The program-level string table is the last section and sits at the
	 * end of our data buffer.  Set up pointers and indices for it.
	 */
	strsec = ddo.ddo_secs - 1;
	strtab = (char *)ddo.ddo_buf + stroff;
	strp = strtab + 1; /* initial byte is '\0' */

	/*
	 * Allocate an array of section indices for tracking DIFOHDR sections
	 * for each ECBDESC.  Reset ddo_secs to zero so we can keep track of
	 * section header indices as we fill in each section's header and data.
	 */
	actsecs = alloca(sizeof (dof_secidx_t) * maxacts);
	ddo.ddo_secs = 0;

	/*
	 * Now iterate through the statements again, this time copying the
	 * actual data into our buffer and filling in the section headers.
	 */
	stp = dt_list_next(&pgp->dp_stmts);
	last = NULL;

	for (; stp != NULL; stp = dt_list_next(stp)) {
		dof_secidx_t probesec = DOF_SECIDX_NONE;
		dof_secidx_t prdsec = DOF_SECIDX_NONE;
		dof_secidx_t actsec = DOF_SECIDX_NONE;

		dtrace_stmtdesc_t *sdp = stp->ds_desc;
		dof_probedesc_t *dofp;
		dof_ecbdesc_t *dofe;
		dof_actdesc_t *dofa;

		uint_t nacts = 0;
		uint_t nactsecs = 0;

		if ((edp = stp->ds_desc->dtsd_ecbdesc) == last)
			continue; /* same ecb as previous statement */

		pdp = &edp->dted_probe;
		last = edp;

		/*
		 * Add a DOF_SECT_PROBEDESC for the ECB's probe description,
		 * and copy each of the probe description strings into the
		 * program-level string table we previously set up.
		 */
		ddo.ddo_lptr = (uchar_t *)
		    roundup((uintptr_t)ddo.ddo_lptr, sizeof (dof_secidx_t));

		ddo.ddo_secp->dofs_type = DOF_SECT_PROBEDESC;
		ddo.ddo_secp->dofs_align = sizeof (dof_secidx_t);
		ddo.ddo_secp->dofs_flags = DOF_SECF_LOAD;
		ddo.ddo_secp->dofs_entsize = sizeof (dof_probedesc_t);
		ddo.ddo_secp->dofs_offset = (size_t)
		    (ddo.ddo_lptr - ddo.ddo_buf);
		ddo.ddo_secp->dofs_size = sizeof (dof_probedesc_t);

		probesec = ddo.ddo_secs++;
		ddo.ddo_secp++;

		dofp = (dof_probedesc_t *)(uintptr_t)ddo.ddo_lptr;
		dofp->dofp_strtab = strsec;

		if (pdp->dtpd_provider[0] != '\0') {
			dofp->dofp_provider = (dof_stridx_t)(strp - strtab);
			(void) strcpy(strp, pdp->dtpd_provider);
			strp += strlen(strp) + 1;
		} else
			dofp->dofp_provider = 0;

		if (pdp->dtpd_mod[0] != '\0') {
			dofp->dofp_mod = (dof_stridx_t)(strp - strtab);
			(void) strcpy(strp, pdp->dtpd_mod);
			strp += strlen(strp) + 1;
		} else
			dofp->dofp_mod = 0;

		if (pdp->dtpd_func[0] != '\0') {
			dofp->dofp_func = (dof_stridx_t)(strp - strtab);
			(void) strcpy(strp, pdp->dtpd_func);
			strp += strlen(strp) + 1;
		} else
			dofp->dofp_func = 0;

		if (pdp->dtpd_name[0] != '\0') {
			dofp->dofp_name = (dof_stridx_t)(strp - strtab);
			(void) strcpy(strp, pdp->dtpd_name);
			strp += strlen(strp) + 1;
		} else
			dofp->dofp_name = 0;

		ddo.ddo_lptr += sizeof (dof_probedesc_t);

		/*
		 * If there is a predicate DIFO associated with the ecbdesc,
		 * write out the DIFO sections and save the DIFO section index.
		 */
		if (edp->dted_pred.dtpdd_difo != NULL)
			prdsec = dof_difo_save(&ddo, edp->dted_pred.dtpdd_difo);

		/*
		 * Iterate through the action list generating sections for each
		 * DIFO referenced by an actdesc.  We accumulate the indices of
		 * the corresponding DOF_SECT_DIFOHDR sections in actsecs[].
		 */
		for (ap = edp->dted_action; ap != NULL; ap = ap->dtad_next) {
			if (ap->dtad_difo != NULL) {
				actsecs[nactsecs++] =
				    dof_difo_save(&ddo, ap->dtad_difo);
			}
			nacts++;
		}

		if (nacts != 0) {
			/*
			 * Iterate through the action list again filling in the
			 * actual DOF_SECT_ACTDESC.  This section is an array
			 * of descriptions where each one is filled in by
			 * copying the relevant fields of the dtrace_actdesc_t,
			 * except for the DIFO, for which we grab the saved
			 * section index of the DOF_SECT_DIFOHDR that we saved
			 * in actsecs[] from the action loop above.
			 */
			ddo.ddo_lptr = (uchar_t *)
			    roundup((uintptr_t)ddo.ddo_lptr, sizeof (uint64_t));

			ddo.ddo_secp->dofs_type = DOF_SECT_ACTDESC;
			ddo.ddo_secp->dofs_align = sizeof (uint64_t);
			ddo.ddo_secp->dofs_flags = DOF_SECF_LOAD;
			ddo.ddo_secp->dofs_entsize = sizeof (dof_actdesc_t);
			ddo.ddo_secp->dofs_offset =
			    (size_t)(ddo.ddo_lptr - ddo.ddo_buf);
			ddo.ddo_secp->dofs_size =
			    sizeof (dof_actdesc_t) * nacts;

			actsec = ddo.ddo_secs++;
			ddo.ddo_secp++;

			dofa = (dof_actdesc_t *)(uintptr_t)ddo.ddo_lptr;
			nactsecs = 0;
			next = stp;
			strndx = 0;

			for (ap = edp->dted_action; ap != NULL;
			    ap = ap->dtad_next) {
				if (ap->dtad_difo != NULL)
					dofa->dofa_difo = actsecs[nactsecs++];
				else
					dofa->dofa_difo = DOF_SECIDX_NONE;

				if (sdp != NULL && ap == sdp->dtsd_action) {
					void *fmt = sdp->dtsd_fmtdata;
					size_t len;

					/*
					 * This is the first action in a
					 * statement.  If it has format data,
					 * that data must be formatted and
					 * printed to the string section.
					 */
					if (fmt != NULL) {
						len = dtrace_printf_format(dtp,
						    fmt, NULL, 0);

						(void) dtrace_printf_format(dtp,
						    fmt, strp, len + 1);

						strndx = (dof_stridx_t)
						    (strp - strtab);
						strp += len + 1;

						dofa->dofa_arg = strndx;
					} else {
						strndx = 0;
					}

					if ((next = dt_list_next(next)) != NULL)
						sdp = next->ds_desc;
					else
						sdp = NULL;
				}

				if (strndx != 0) {
					dofa->dofa_arg = strndx;
					dofa->dofa_strtab = strsec;
				} else {
					dofa->dofa_arg = ap->dtad_arg;
					dofa->dofa_strtab = DOF_SECIDX_NONE;
				}

				dofa->dofa_kind = ap->dtad_kind;
				dofa->dofa_ntuple = ap->dtad_ntuple;
				dofa->dofa_uarg = ap->dtad_uarg;
				dofa++;
			}

			ddo.ddo_lptr += sizeof (dof_actdesc_t) * nacts;
		}

		/*
		 * Now finally, add the DOF_SECT_ECBDESC referencing all the
		 * previously created sub-sections.
		 */
		ddo.ddo_lptr = (uchar_t *)
		    roundup((uintptr_t)ddo.ddo_lptr, sizeof (uint64_t));

		ddo.ddo_secp->dofs_type = DOF_SECT_ECBDESC;
		ddo.ddo_secp->dofs_align = sizeof (uint64_t);
		ddo.ddo_secp->dofs_flags = DOF_SECF_LOAD;
		ddo.ddo_secp->dofs_entsize = 0;
		ddo.ddo_secp->dofs_offset = (size_t)
		    (ddo.ddo_lptr - ddo.ddo_buf);
		ddo.ddo_secp->dofs_size = sizeof (dof_ecbdesc_t);

		ddo.ddo_secs++;
		ddo.ddo_secp++;

		dofe = (dof_ecbdesc_t *)(uintptr_t)ddo.ddo_lptr;

		dofe->dofe_probes = probesec;
		dofe->dofe_pred = prdsec;
		dofe->dofe_actions = actsec;
		dofe->dofe_pad = 0;
		dofe->dofe_uarg = edp->dted_uarg;

		ddo.ddo_lptr += sizeof (dof_ecbdesc_t);
	}

	if (flags & DTRACE_D_PROBES) {
		uint_t h;
		dt_provider_t *pvp;

		for (h = 0; h < dtp->dt_provbuckets; h++) {
			for (pvp = dtp->dt_provs[h]; pvp != NULL;
			    pvp = pvp->pv_next) {
				if (dof_provider_save(dtp, &ddo, pvp) != 0) {
					free(ddo.ddo_buf);
					return (NULL);
				}
			}
		}
	}

	assert((size_t)(ddo.ddo_lptr - ddo.ddo_buf) == stroff);

	ddo.ddo_secp->dofs_type = DOF_SECT_STRTAB;
	ddo.ddo_secp->dofs_align = sizeof (char);
	ddo.ddo_secp->dofs_flags = DOF_SECF_LOAD;
	ddo.ddo_secp->dofs_entsize = 0;
	ddo.ddo_secp->dofs_offset = stroff;
	ddo.ddo_secp->dofs_size = ddo.ddo_strs + 1;

	ddo.ddo_secs++;
	ddo.ddo_secp++;
	ddo.ddo_lptr += ddo.ddo_strs + 1;

	assert(ddo.ddo_lptr <= ddo.ddo_buf + hp->dofh_loadsz);
	assert(ddo.ddo_uptr <= ddo.ddo_buf + hp->dofh_filesz);

	assert(ddo.ddo_secs == hp->dofh_secnum);
	assert(ddo.ddo_secp == secbase + hp->dofh_secnum);

	return (ddo.ddo_buf);
}

void *
dtrace_getopt_dof(dtrace_hdl_t *dtp)
{
	dof_hdr_t *dof;
	dof_sec_t *sec;
	dof_optdesc_t *dofo;
	int i, nopts = 0, len = sizeof (dof_hdr_t) +
	    roundup(sizeof (dof_sec_t), sizeof (uint64_t));

	for (i = 0; i < DTRACEOPT_MAX; i++) {
		if (dtp->dt_options[i] != DTRACEOPT_UNSET)
			nopts++;
	}

	len += sizeof (dof_optdesc_t) * nopts;

	if ((dof = malloc(len)) == NULL) {
		(void) dt_set_errno(dtp, EDT_NOMEM);
		return (NULL);
	}

	bzero(dof, len);

	if (dtrace_dof_hdr(dtp, dof) != 0) {
		free(dof);
		return (NULL);
	}

	dof->dofh_secnum = 1;	/* only DOF_SECT_OPTDESC */
	dof->dofh_loadsz = len;
	dof->dofh_filesz = len;

	/*
	 * Fill in the option section header...
	 */
	sec = (dof_sec_t *)((uintptr_t)dof + sizeof (dof_hdr_t));
	sec->dofs_type = DOF_SECT_OPTDESC;
	sec->dofs_align = sizeof (uint64_t);
	sec->dofs_flags = DOF_SECF_LOAD;
	sec->dofs_entsize = sizeof (dof_optdesc_t);

	dofo = (dof_optdesc_t *)((uintptr_t)sec +
	    roundup(sizeof (dof_sec_t), sizeof (uint64_t)));

	sec->dofs_offset = (uintptr_t)dofo - (uintptr_t)dof;
	sec->dofs_size = sizeof (dof_optdesc_t) * nopts;

	for (i = 0; i < DTRACEOPT_MAX; i++) {
		if (dtp->dt_options[i] == DTRACEOPT_UNSET)
			continue;

		dofo->dofo_option = i;
		dofo->dofo_strtab = DOF_SECIDX_NONE;
		dofo->dofo_value = dtp->dt_options[i];
		dofo++;
	}

	return (dof);
}

void *
dtrace_geterr_dof(dtrace_hdl_t *dtp)
{
	if (dtp->dt_errprog != NULL)
		return (dtrace_dof_create(dtp, dtp->dt_errprog, 0));

	(void) dt_set_errno(dtp, EDT_BADERROR);
	return (NULL);
}

void
dtrace_dof_destroy(void *dof)
{
	free(dof);
}

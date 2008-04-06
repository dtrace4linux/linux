/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)dt_provider.c	1.4	04/11/13 SMI"

#include <linux_types.h>
#include <assert.h>
#include <limits.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <dt_provider.h>
#include <dt_list.h>

static dt_provider_t *
dt_provider_insert(dtrace_hdl_t *dtp, dt_provider_t *pvp, uint_t h)
{
	dt_list_append(&dtp->dt_provlist, pvp);

	pvp->pv_next = dtp->dt_provs[h];
	dtp->dt_provs[h] = pvp;
	dtp->dt_nprovs++;

	return (pvp);
}

dt_provider_t *
dt_provider_lookup(dtrace_hdl_t *dtp, const char *name)
{
	uint_t h = dt_strtab_hash(name, NULL) % dtp->dt_provbuckets;
	dt_provider_t *pvp;

	for (pvp = dtp->dt_provs[h]; pvp != NULL; pvp = pvp->pv_next) {
		if (strcmp(pvp->pv_desc.dtvd_name, name) == 0)
			return (pvp);
	}

	if ((pvp = malloc(sizeof (dt_provider_t))) == NULL) {
		(void) dt_set_errno(dtp, EDT_NOMEM);
		return (NULL);
	}

	bzero(pvp, sizeof (dt_provider_t));
	(void) strncpy(pvp->pv_desc.dtvd_name, name, DTRACE_PROVNAMELEN - 1);

	if (dt_ioctl(dtp, DTRACEIOC_PROVIDER, &pvp->pv_desc) == -1) {
		(void) dt_set_errno(dtp, errno == ESRCH ? EDT_NOPROV : errno);
		free(pvp);
		return (NULL);
	}

	dt_dprintf("caching provider attributes for %s\n", name);
	return (dt_provider_insert(dtp, pvp, h));
}

dt_provider_t *
dt_provider_create(dtrace_hdl_t *dtp, const char *name)
{
	dt_provider_t *pvp;

	if ((pvp = malloc(sizeof (dt_provider_t))) == NULL) {
		(void) dt_set_errno(dtp, EDT_NOMEM);
		return (NULL);
	}

	bzero(pvp, sizeof (dt_provider_t));
	(void) strncpy(pvp->pv_desc.dtvd_name, name, DTRACE_PROVNAMELEN - 1);
	pvp->pv_probes = dt_idhash_create(pvp->pv_desc.dtvd_name, NULL, 0, 0);
	pvp->pv_gen = dtp->dt_gen;

	if (pvp->pv_probes == NULL) {
		free(pvp);
		(void) dt_set_errno(dtp, EDT_NOMEM);
		return (NULL);
	}

	pvp->pv_desc.dtvd_attr.dtpa_provider = _dtrace_prvattr;
	pvp->pv_desc.dtvd_attr.dtpa_mod = _dtrace_prvattr;
	pvp->pv_desc.dtvd_attr.dtpa_func = _dtrace_prvattr;
	pvp->pv_desc.dtvd_attr.dtpa_name = _dtrace_prvattr;
	pvp->pv_desc.dtvd_attr.dtpa_args = _dtrace_prvattr;

	return (dt_provider_insert(dtp, pvp,
	    dt_strtab_hash(name, NULL) % dtp->dt_provbuckets));
}

void
dt_provider_destroy(dtrace_hdl_t *dtp, dt_provider_t *pvp)
{
	dt_provider_t **pp;
	uint_t h;

	h = dt_strtab_hash(pvp->pv_desc.dtvd_name, NULL) % dtp->dt_provbuckets;
	pp = &dtp->dt_provs[h];

	while (*pp != NULL && *pp != pvp)
		pp = &(*pp)->pv_next;

	assert(*pp != NULL && *pp == pvp);
	*pp = pvp->pv_next;

	dt_list_delete(&dtp->dt_provlist, pvp);
	dtp->dt_nprovs--;

	if (pvp->pv_probes != NULL)
		dt_idhash_destroy(pvp->pv_probes);

	dt_node_link_free(&pvp->pv_nodes);
	free(pvp);
}

static uint8_t
dt_probe_argmap(dt_node_t *xnp, dt_node_t *nnp)
{
	uint8_t i;

	for (i = 0; nnp != NULL; i++) {
		if (nnp->dn_string != NULL &&
		    strcmp(nnp->dn_string, xnp->dn_string) == 0)
			break;
		else
			nnp = nnp->dn_list;
	}

	return (i);
}

dt_probe_t *
dt_probe_lookup(dt_provider_t *pvp, const char *pname)
{
	dt_ident_t *idp;

	if ((idp = dt_idhash_lookup(pvp->pv_probes, pname)) == NULL)
		return (NULL);

	return (idp->di_data);
}

dt_probe_t *
dt_probe_create(dt_ident_t *idp,
    dt_node_t *nargs, uint_t nargc, dt_node_t *xargs, uint_t xargc)
{
	dt_probe_t *prp;
	uint_t i;

	if (xargs == NULL) {
		xargs = nargs;
		xargc = nargc;
	}

	if ((prp = malloc(sizeof (dt_probe_t))) == NULL)
		return (NULL);

	prp->pr_ident = idp;
	prp->pr_nargs = nargs;
	prp->pr_nargc = nargc;
	prp->pr_xargs = xargs;
	prp->pr_xargc = xargc;
	prp->pr_mapping = NULL;
	prp->pr_inst = NULL;

	if (xargc != 0 &&
	    (prp->pr_mapping = malloc(sizeof (uint8_t) * xargc)) == NULL) {
		free(prp);
		return (NULL);
	}

	for (i = 0; i < xargc; i++, xargs = xargs->dn_list) {
		if (xargs->dn_string != NULL)
			prp->pr_mapping[i] = dt_probe_argmap(xargs, nargs);
		else
			prp->pr_mapping[i] = i;
	}

	return (prp);
}

void
dt_probe_destroy(dt_ident_t *idp)
{
	dt_probe_t *prp;
	dt_probe_instance_t *pip, *pip_next;

	if (idp == NULL || (prp = idp->di_data) == NULL)
		return; /* ignore NULL pointers to simplify caller code */

	dt_node_list_free(&prp->pr_nargs);
	dt_node_list_free(&prp->pr_xargs);

	for (pip = prp->pr_inst; pip != NULL; pip = pip_next) {
		pip_next = pip->pi_next;
		free(pip->pi_offs);
		free(pip);
	}

	free(prp->pr_mapping);
	free(prp);
}

int
dt_probe_add(dtrace_hdl_t *dtp, dt_probe_t *prp, const char *fname,
    uint64_t offset)
{
	dt_probe_instance_t *pip;

	for (pip = prp->pr_inst; pip != NULL; pip = pip->pi_next) {
		if (strcmp(pip->pi_fname, fname) == 0)
			break;
	}

	if (pip == NULL) {
		if ((pip = malloc(sizeof (dt_probe_instance_t))) == NULL)
			return (dt_set_errno(dtp, EDT_NOMEM));

		(void) strncpy(pip->pi_fname, fname, sizeof (pip->pi_fname));
		pip->pi_noffs = 0;
		pip->pi_maxoffs = 1;
		pip->pi_offs = malloc(sizeof (pip->pi_offs[0]) *
		    pip->pi_maxoffs);
		if (pip->pi_offs == NULL) {
			free(pip);
			return (dt_set_errno(dtp, EDT_NOMEM));
		}

		pip->pi_next = prp->pr_inst;
		prp->pr_inst = pip;
	}

	assert(pip->pi_noffs <= pip->pi_maxoffs);

	if (pip->pi_noffs == pip->pi_maxoffs) {
		uint32_t *offs = pip->pi_offs;

		pip->pi_offs = malloc(sizeof (pip->pi_offs[0]) *
		    pip->pi_maxoffs * 2);
		if (pip->pi_offs == NULL)
			return (dt_set_errno(dtp, EDT_NOMEM));

		bcopy(offs, pip->pi_offs, sizeof (pip->pi_offs[0]) *
		    pip->pi_maxoffs);
		pip->pi_maxoffs *= 2;

		free(offs);
	}

	pip->pi_offs[pip->pi_noffs++] = offset;

	return (0);
}

/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)dt_ident.c	1.8	04/11/13 SMI"

#include <linux_types.h>
#include <sys/sysmacros.h>
#include <strings.h>
#include <stdlib.h>
#include <alloca.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <sys/procfs_isa.h>
#include <limits.h>

#include <dt_ident.h>
#include <dt_parser.h>
#include <dt_provider.h>
#include <dt_strtab.h>
#include <dt_impl.h>

/*
 * Common code for cooking an identifier that uses a typed signature list (we
 * use this for associative arrays and functions).  If the argument list is
 * of the same length and types, then return the return type.  Otherwise
 * print an appropriate compiler error message and abort the compile.
 */
static void
dt_idcook_sign(dt_node_t *dnp, dt_ident_t *idp,
    int argc, dt_node_t *args, const char *prefix, const char *suffix)
{
	dt_idsig_t *isp = idp->di_data;
	int i, compat, mismatch, arglimit;

	char n1[DT_TYPE_NAMELEN];
	char n2[DT_TYPE_NAMELEN];

	if (isp->dis_varargs >= 0) {
		mismatch = argc < isp->dis_varargs;
		arglimit = isp->dis_varargs;
	} else {
		mismatch = argc != isp->dis_argc;
		arglimit = isp->dis_argc;
	}

	if (mismatch) {
		xyerror(D_PROTO_LEN, "%s%s%s prototype mismatch: %d arg%s"
		    "passed, %d expected\n", prefix, idp->di_name, suffix, argc,
		    argc == 1 ? " " : "s ", arglimit);
	}

	for (i = 0; i < arglimit; i++, args = args->dn_list) {
		if (isp->dis_args[i].dn_ctfp != NULL)
			compat = dt_node_is_argcompat(&isp->dis_args[i], args);
		else
			compat = 1; /* "@" matches any type */

		if (!compat) {
			xyerror(D_PROTO_ARG,
			    "%s%s%s argument #%d is incompatible with "
			    "prototype:\n\tprototype: %s\n\t argument: %s\n",
			    prefix, idp->di_name, suffix, i + 1,
			    dt_node_type_name(&isp->dis_args[i], n1,
			    sizeof (n1)),
			    dt_node_type_name(args, n2, sizeof (n2)));
		}
	}

	dt_node_type_assign(dnp, idp->di_ctfp, idp->di_type);
}

/*
 * Cook an associative array identifier.  If this is the first time we are
 * cooking this array, create its signature based on the argument list.
 * Otherwise validate the argument list against the existing signature.
 */
static void
dt_idcook_assc(dt_node_t *dnp, dt_ident_t *idp, int argc, dt_node_t *args)
{
	if (idp->di_data == NULL) {
		dt_idsig_t *isp = idp->di_data = malloc(sizeof (dt_idsig_t));
		char n[DT_TYPE_NAMELEN];
		int i;

		if (isp == NULL)
			longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

		isp->dis_varargs = -1;
		isp->dis_argc = argc;
		isp->dis_args = NULL;

		if (argc != 0 && (isp->dis_args = calloc(argc,
		    sizeof (dt_node_t))) == NULL) {
			idp->di_data = NULL;
			free(isp);
			longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);
		}

		/*
		 * If this identifier has not been explicitly declared earlier,
		 * set the identifier's base type to be our special type <DYN>.
		 * If this ident is an aggregation, it will remain as is.  If
		 * this ident is an associative array, it will be reassigned
		 * based on the result type of the first assignment statement.
		 */
		if (!(idp->di_flags & DT_IDFLG_DECL)) {
			idp->di_ctfp = DT_DYN_CTFP(yypcb->pcb_hdl);
			idp->di_type = DT_DYN_TYPE(yypcb->pcb_hdl);
		}

		for (i = 0; i < argc; i++, args = args->dn_list) {
			if (dt_node_is_dynamic(args) || dt_node_is_void(args)) {
				xyerror(D_KEY_TYPE, "%s expression may not be "
				    "used as %s index: key #%d\n",
				    dt_node_type_name(args, n, sizeof (n)),
				    dt_idkind_name(idp->di_kind), i + 1);
			}

			dt_node_type_propagate(args, &isp->dis_args[i]);
			isp->dis_args[i].dn_list = &isp->dis_args[i + 1];
		}

		if (argc != 0)
			isp->dis_args[argc - 1].dn_list = NULL;

		dt_node_type_assign(dnp, idp->di_ctfp, idp->di_type);

	} else {
		dt_idcook_sign(dnp, idp, argc, args,
		    idp->di_kind == DT_IDENT_AGG ? "@" : "", "[ ]");
	}
}

/*
 * Cook a function call.  If this is the first time we are cooking this
 * identifier, create its type signature based on predefined prototype stored
 * in di_iarg.  We then validate the argument list against this signature.
 */
static void
dt_idcook_func(dt_node_t *dnp, dt_ident_t *idp, int argc, dt_node_t *args)
{
	if (idp->di_data == NULL) {
		dtrace_hdl_t *dtp = yypcb->pcb_hdl;
		dtrace_typeinfo_t dtt;
		dt_idsig_t *isp;
		char *s, *p1, *p2;
		int i = 0;

		assert(idp->di_iarg != NULL);
		s = alloca(strlen(idp->di_iarg) + 1);
		(void) strcpy(s, idp->di_iarg);

		if ((p2 = strrchr(s, ')')) != NULL)
			*p2 = '\0'; /* mark end of parameter list string */

		if ((p1 = strchr(s, '(')) != NULL)
			*p1++ = '\0'; /* mark end of return type string */

		if (p1 == NULL || p2 == NULL) {
			xyerror(D_UNKNOWN, "internal error: malformed entry "
			    "for built-in function %s\n", idp->di_name);
		}

		for (p2 = p1; *p2 != '\0'; p2++) {
			if (!isspace(*p2)) {
				i++;
				break;
			}
		}

		for (p2 = strchr(p2, ','); p2++ != NULL; i++)
			p2 = strchr(p2, ',');

		/*
		 * We first allocate a new ident signature structure with the
		 * appropriate number of argument entries, and then look up
		 * the return type and store its CTF data in di_ctfp/type.
		 */
		if ((isp = idp->di_data = malloc(sizeof (dt_idsig_t))) == NULL)
			longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

		isp->dis_varargs = -1;
		isp->dis_argc = i;
		isp->dis_args = NULL;

		if (i != 0 && (isp->dis_args = calloc(i,
		    sizeof (dt_node_t))) == NULL) {
			idp->di_data = NULL;
			free(isp);
			longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);
		}

		if (dt_type_lookup(s, &dtt) == -1) {
			xyerror(D_UNKNOWN, "failed to resolve type of %s (%s):"
			    " %s\n", idp->di_name, s,
			    dtrace_errmsg(dtp, dtrace_errno(dtp)));
		}

		if (idp->di_kind == DT_IDENT_AGGFUNC) {
			idp->di_ctfp = DT_DYN_CTFP(dtp);
			idp->di_type = DT_DYN_TYPE(dtp);
		} else {
			idp->di_ctfp = dtt.dtt_ctfp;
			idp->di_type = dtt.dtt_type;
		}

		/*
		 * For each comma-delimited parameter in the prototype string,
		 * we look up the corresponding type and store its CTF data in
		 * the corresponding location in dis_args[].  We also recognize
		 * the special type string "@" to indicate that the specified
		 * parameter may be a D expression of *any* type (represented
		 * as a dis_args[] element with ctfp = NULL, type == CTF_ERR).
		 * If a varargs "..." is present, we record the argument index
		 * in dis_varargs for the benefit of dt_idcook_sign(), above.
		 */
		for (i = 0; i < isp->dis_argc; i++, p1 = p2) {
			while (isspace(*p1))
				p1++; /* skip leading whitespace */

			if ((p2 = strchr(p1, ',')) == NULL)
				p2 = p1 + strlen(p1);
			else
				*p2++ = '\0';

			if (strcmp(p1, "@") == 0 || strcmp(p1, "...") == 0) {
				isp->dis_args[i].dn_ctfp = NULL;
				isp->dis_args[i].dn_type = CTF_ERR;
				if (*p1 == '.')
					isp->dis_varargs = i;
				continue;
			}

			if (dt_type_lookup(p1, &dtt) == -1) {
				xyerror(D_UNKNOWN, "failed to resolve type of "
				    "%s arg#%d (%s): %s\n", idp->di_name, i + 1,
				    p1, dtrace_errmsg(dtp, dtrace_errno(dtp)));
			}

			dt_node_type_assign(&isp->dis_args[i],
			    dtt.dtt_ctfp, dtt.dtt_type);
		}
	}

	dt_idcook_sign(dnp, idp, argc, args, "", "( )");
}

/*
 * Cook a reference to the dynamically typed args[] array.  We verify that the
 * reference is using a single integer constant, and then return the type found
 * at the corresponding location in yypcb->pcb_pargv[].
 */
static void
dt_idcook_args(dt_node_t *dnp, dt_ident_t *idp, int argc, dt_node_t *ap)
{
	char n[DT_TYPE_NAMELEN];
	dt_ident_t *xidp;
	dtrace_hdl_t *dtp = yypcb->pcb_hdl;
	dt_arg_t *argp;

	if (argc != 1) {
		xyerror(D_PROTO_LEN, "%s[ ] prototype mismatch: %d arg%s"
		    "passed, 1 expected\n", idp->di_name, argc,
		    argc == 1 ? " " : "s ");
	}

	if (ap->dn_kind != DT_NODE_INT) {
		xyerror(D_PROTO_ARG, "%s[ ] argument #1 is incompatible with "
		    "prototype:\n\tprototype: %s\n\t argument: %s\n",
		    idp->di_name, "integer constant",
		    dt_type_name(ap->dn_ctfp, ap->dn_type, n, sizeof (n)));
	}

	if (yypcb->pcb_pdesc == NULL) {
		xyerror(D_ARGS_NONE, "%s[ ] may not be referenced outside "
		    "of a probe clause\n", idp->di_name);
	}

	if (yypcb->pcb_pargs == NULL) {
		if (yypcb->pcb_nprobes > 1) {
			xyerror(D_ARGS_MULTI, "%s[ ] may not be referenced "
			    "because probe description %s matches more than "
			    "one probe\n", idp->di_name,
			    dtrace_desc2str(yypcb->pcb_pdesc, n, sizeof (n)));
		}

		xyerror(D_ARGS_NONE, "no %s[ ] are available for probe %s\n",
		    idp->di_name, dtrace_desc2str(yypcb->pcb_pdesc,
		    n, sizeof (n)));
	}

	for (argp = yypcb->pcb_pargs; argp != NULL; argp = argp->da_next) {
		if (argp->da_ndx == ap->dn_value)
			break;
	}

	if (((ap->dn_flags & DT_NF_SIGNED) && (int64_t)ap->dn_value < 0) ||
	    argp == NULL) {
		xyerror(D_ARGS_IDX, "index %lld is out of range for probe %s "
		    "%s[ ]\n", (longlong_t)ap->dn_value,
		    dtrace_desc2str(yypcb->pcb_pdesc,
		    n, sizeof (n)), idp->di_name);
	}

	if ((xidp = argp->da_xlator) != NULL) {
		/*
		 * If we have an argument translator, we need to transform the
		 * ident for the node based on the ident of the translator.
		 * We perform just enough transformation such that the node
		 * appears to be a translator when looked at as a translator,
		 * but not so much that it doesn't look like an array when it
		 * needs to be viewed as an array.
		 */
		idp->di_kind = xidp->di_kind;
		idp->di_data = xidp->di_data;
		idp->di_ctfp = xidp->di_ctfp;
		idp->di_type = xidp->di_type;

		dt_node_type_assign(dnp, DT_DYN_CTFP(dtp), DT_DYN_TYPE(dtp));
	} else {
		idp->di_kind = DT_IDENT_ARRAY;
		idp->di_data = NULL;
		idp->di_ctfp = DT_DYN_CTFP(yypcb->pcb_hdl);
		idp->di_type = DT_DYN_TYPE(yypcb->pcb_hdl);

		dt_node_type_assign(dnp, argp->da_ctfp, argp->da_type);
	}
}

static void
dt_idcook_regs(dt_node_t *dnp, dt_ident_t *idp, int argc, dt_node_t *ap)
{
	dtrace_typeinfo_t dtt;
	dtrace_hdl_t *dtp = yypcb->pcb_hdl;
	char n[DT_TYPE_NAMELEN];

	if (argc != 1) {
		xyerror(D_PROTO_LEN, "%s[ ] prototype mismatch: %d arg%s"
		    "passed, 1 expected\n", idp->di_name,
		    argc, argc == 1 ? " " : "s ");
	}

	if (ap->dn_kind != DT_NODE_INT) {
		xyerror(D_PROTO_ARG, "%s[ ] argument #1 is incompatible with "
		    "prototype:\n\tprototype: %s\n\t argument: %s\n",
		    idp->di_name, "integer constant",
		    dt_type_name(ap->dn_ctfp, ap->dn_type, n, sizeof (n)));
	}

	if ((ap->dn_flags & DT_NF_SIGNED) && (int64_t)ap->dn_value < 0) {
		xyerror(D_REGS_IDX, "index %lld is out of range for array %s\n",
		    (longlong_t)ap->dn_value, idp->di_name);
	}

	if (dt_type_lookup("uint64_t", &dtt) == -1) {
		xyerror(D_UNKNOWN, "failed to resolve type of %s: %s\n",
		    idp->di_name, dtrace_errmsg(dtp, dtrace_errno(dtp)));
	}

	idp->di_ctfp = dtt.dtt_ctfp;
	idp->di_type = dtt.dtt_type;

	dt_node_type_assign(dnp, idp->di_ctfp, idp->di_type);
}

/*ARGSUSED*/
static void
dt_idcook_type(dt_node_t *dnp, dt_ident_t *idp, int argc, dt_node_t *args)
{
	if (idp->di_type == CTF_ERR) {
		dtrace_hdl_t *dtp = yypcb->pcb_hdl;
		dtrace_typeinfo_t dtt;

		if (dt_type_lookup(idp->di_iarg, &dtt) == -1) {
			xyerror(D_UNKNOWN,
			    "failed to resolve type %s for identifier %s: %s\n",
			    (const char *)idp->di_iarg, idp->di_name,
			    dtrace_errmsg(dtp, dtrace_errno(dtp)));
		}

		idp->di_ctfp = dtt.dtt_ctfp;
		idp->di_type = dtt.dtt_type;
	}

	dt_node_type_assign(dnp, idp->di_ctfp, idp->di_type);
}

/*ARGSUSED*/
static void
dt_idcook_thaw(dt_node_t *dnp, dt_ident_t *idp, int argc, dt_node_t *args)
{
	if (idp->di_ctfp != NULL && idp->di_type != CTF_ERR)
		dt_node_type_assign(dnp, idp->di_ctfp, idp->di_type);
}

/*ARGSUSED*/
static dt_ident_t *
dt_idctor_default(dt_node_t *dnp, int argc, dt_node_t *ap)
{
	return (dnp->dn_ident);
}

static dt_ident_t *
dt_idctor_args(dt_node_t *dnp, int argc, dt_node_t *ap)
{
	dt_ident_t *idp = dnp->dn_ident;
	char n[DT_TYPE_NAMELEN], *name;
	int len;

	/*
	 * The args[] array is special because two identifiers with different
	 * indices will have different ident attributes.  To support this,
	 * we create an ident for each index.  The di_data field of the "args"
	 * ident in the hash points to a list of indexed idents; each of the
	 * indexed args[] idents has DT_IDFLG_ARGNDX set in its di_flags field.
	 */
	if (idp->di_flags & DT_IDFLG_ARGNDX)
		return (idp);

	if (argc != 1) {
		xyerror(D_PROTO_LEN, "%s[ ] prototype mismatch: %d arg%s"
		    "passed, 1 expected\n", idp->di_name, argc,
		    argc == 1 ? " " : "s ");
	}

	if (ap->dn_kind != DT_NODE_INT) {
		xyerror(D_PROTO_ARG, "%s[ ] argument #1 is incompatible with "
		    "prototype:\n\tprototype: %s\n\t argument: %s\n",
		    idp->di_name, "integer constant",
		    dt_type_name(ap->dn_ctfp, ap->dn_type, n, sizeof (n)));
	}

	if (ap->dn_value > UCHAR_MAX) {
		xyerror(D_ARGS_IDX, "index %lld is out of range for probe %s "
		    "%s[ ]\n", (longlong_t)ap->dn_value,
		    dtrace_desc2str(yypcb->pcb_pdesc,
		    n, sizeof (n)), idp->di_name);
	}

	len = strlen(idp->di_name) + 1;

	for (idp = idp->di_data; idp != NULL; idp = idp->di_next) {
		if (idp->di_name[len] == ap->dn_value)
			return (idp);
	}

	/*
	 * We need to create a new ident using the existing one as a template.
	 */
	idp = dnp->dn_ident;
	if ((name = malloc(strlen(idp->di_name) + 2)) == NULL)
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

	(void) strcpy(name, idp->di_name);
	name[len] = ap->dn_value;

	idp = dt_ident_create(NULL, idp->di_kind, idp->di_flags, idp->di_id,
	    idp->di_attr, idp->di_vers, idp->di_ops, idp->di_iarg,
	    idp->di_gen);

	idp->di_next = dnp->dn_ident->di_data;
	dnp->dn_ident->di_data = idp;
	idp->di_flags |= DT_IDFLG_ARGNDX;
	idp->di_name = name;

	return (idp);
}

static void
dt_iddtor_sign(dt_ident_t *idp)
{
	if (idp->di_data != NULL)
		free(((dt_idsig_t *)idp->di_data)->dis_args);
	free(idp->di_data);
}

static void
dt_iddtor_node(dt_ident_t *idp)
{
	dt_idnode_t *inp = idp->di_data;

	if (inp != NULL) {
		dt_node_link_free(&inp->din_list);
		free(inp);
	}
}

static void
dt_iddtor_free(dt_ident_t *idp)
{
	free(idp->di_data);
}

static void
dt_iddtor_args(dt_ident_t *idp)
{
	dt_ident_t *next, *old;

	assert(!(idp->di_flags & DT_IDFLG_ARGNDX));

	for (old = idp->di_data; old != NULL; old = next) {
		next = old->di_next;
		free(old->di_name);
		free(old);
	}
}

static size_t
dt_idsize_type(dt_ident_t *idp)
{
	return (ctf_type_size(idp->di_ctfp, idp->di_type));
}

/*ARGSUSED*/
static size_t
dt_idsize_none(dt_ident_t *idp)
{
	return (0);
}

const dt_idops_t dt_idops_assc = {
	dt_idcook_assc,
	dt_idctor_default,
	dt_iddtor_sign,
	dt_idsize_none,
};

const dt_idops_t dt_idops_func = {
	dt_idcook_func,
	dt_idctor_default,
	dt_iddtor_sign,
	dt_idsize_none,
};

const dt_idops_t dt_idops_args = {
	dt_idcook_args,
	dt_idctor_args,
	dt_iddtor_args,
	dt_idsize_none,
};

const dt_idops_t dt_idops_regs = {
	dt_idcook_regs,
	dt_idctor_default,
	dt_iddtor_free,
	dt_idsize_none,
};

const dt_idops_t dt_idops_type = {
	dt_idcook_type,
	dt_idctor_default,
	dt_iddtor_free,
	dt_idsize_type,
};

const dt_idops_t dt_idops_thaw = {
	dt_idcook_thaw,
	dt_idctor_default,
	dt_iddtor_free,
	dt_idsize_type,
};

const dt_idops_t dt_idops_inline = {
	dt_idcook_thaw,
	dt_idctor_default,
	dt_iddtor_node,
	dt_idsize_type,
};

const dt_idops_t dt_idops_probe = {
	dt_idcook_thaw,
	dt_idctor_default,
	dt_probe_destroy,
	dt_idsize_none,
};

static void
dt_idhash_populate(dt_idhash_t *dhp)
{
	const dt_ident_t *idp = dhp->dh_tmpl;

	dhp->dh_tmpl = NULL; /* clear dh_tmpl first to avoid recursion */
	dt_dprintf("populating %s idhash from %p\n", dhp->dh_name, (void *)idp);

	for (; idp->di_name != NULL; idp++) {
		if (dt_idhash_insert(dhp, idp->di_name,
		    idp->di_kind, idp->di_flags, idp->di_id, idp->di_attr,
		    idp->di_vers, idp->di_ops ? idp->di_ops : &dt_idops_thaw,
		    idp->di_iarg, 0) == NULL)
			longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);
	}
}

dt_idhash_t *
dt_idhash_create(const char *name, const dt_ident_t *tmpl,
    uint_t min, uint_t max)
{
	dt_idhash_t *dhp;
	size_t size;

	assert(min <= max);

	size = sizeof (dt_idhash_t) +
	    sizeof (dt_ident_t *) * (_dtrace_strbuckets - 1);

	if ((dhp = malloc(size)) == NULL)
		return (NULL);

	bzero(dhp, size);
	dhp->dh_name = name;
	dhp->dh_tmpl = tmpl;
	dhp->dh_nextid = min;
	dhp->dh_minid = min;
	dhp->dh_maxid = max;
	dhp->dh_hashsz = _dtrace_strbuckets;

	return (dhp);
}

void
dt_idhash_destroy(dt_idhash_t *dhp)
{
	dt_ident_t *idp, *next;
	ulong_t i;

	for (i = 0; i < dhp->dh_hashsz; i++) {
		for (idp = dhp->dh_hash[i]; idp != NULL; idp = next) {
			next = idp->di_next;
			dt_ident_destroy(idp);
		}
	}

	free(dhp);
}

void
dt_idhash_update(dt_idhash_t *dhp)
{
	uint_t nextid = dhp->dh_minid;
	dt_ident_t *idp;
	ulong_t i;

	for (i = 0; i < dhp->dh_hashsz; i++) {
		for (idp = dhp->dh_hash[i]; idp != NULL; idp = idp->di_next) {
			if (idp->di_kind == DT_IDENT_ARRAY ||
			    idp->di_kind == DT_IDENT_SCALAR)
				nextid = MAX(nextid, idp->di_id + 1);
		}
	}

	dhp->dh_nextid = nextid;
}

dt_ident_t *
dt_idhash_lookup(dt_idhash_t *dhp, const char *name)
{
	size_t len;
	ulong_t h = dt_strtab_hash(name, &len) % dhp->dh_hashsz;
	dt_ident_t *idp;

	if (dhp->dh_tmpl != NULL)
		dt_idhash_populate(dhp); /* fill hash w/ initial population */

	for (idp = dhp->dh_hash[h]; idp != NULL; idp = idp->di_next) {
		if (strcmp(idp->di_name, name) == 0)
			return (idp);
	}

	return (NULL);
}

int
dt_idhash_nextid(dt_idhash_t *dhp, uint_t *p)
{
	if (dhp->dh_nextid >= dhp->dh_maxid)
		return (-1); /* no more id's are free to allocate */

	*p = dhp->dh_nextid++;
	return (0);
}

ulong_t
dt_idhash_size(const dt_idhash_t *dhp)
{
	return (dhp->dh_nelems);
}

const char *
dt_idhash_name(const dt_idhash_t *dhp)
{
	return (dhp->dh_name);
}

dt_ident_t *
dt_idhash_insert(dt_idhash_t *dhp, const char *name, ushort_t kind,
    ushort_t flags, uint_t id, dtrace_attribute_t attr, uint_t vers,
    const dt_idops_t *ops, void *iarg, ulong_t gen)
{
	dt_ident_t *idp;
	ulong_t h;

	if (dhp->dh_tmpl != NULL)
		dt_idhash_populate(dhp); /* fill hash w/ initial population */

	idp = dt_ident_create(name, kind, flags, id,
	    attr, vers, ops, iarg, gen);

	if (idp == NULL)
		return (NULL);

	h = dt_strtab_hash(name, NULL) % dhp->dh_hashsz;
	idp->di_next = dhp->dh_hash[h];

	dhp->dh_hash[h] = idp;
	dhp->dh_nelems++;

	if (dhp->dh_defer != NULL)
		dhp->dh_defer(dhp, idp);

	return (idp);
}

void
dt_idhash_xinsert(dt_idhash_t *dhp, dt_ident_t *idp)
{
	ulong_t h;

	if (dhp->dh_tmpl != NULL)
		dt_idhash_populate(dhp); /* fill hash w/ initial population */

	h = dt_strtab_hash(idp->di_name, NULL) % dhp->dh_hashsz;
	idp->di_next = dhp->dh_hash[h];

	dhp->dh_hash[h] = idp;
	dhp->dh_nelems++;

	if (dhp->dh_defer != NULL)
		dhp->dh_defer(dhp, idp);
}

void
dt_idhash_delete(dt_idhash_t *dhp, dt_ident_t *key)
{
	size_t len;
	ulong_t h = dt_strtab_hash(key->di_name, &len) % dhp->dh_hashsz;
	dt_ident_t **pp = &dhp->dh_hash[h];
	dt_ident_t *idp;

	for (idp = dhp->dh_hash[h]; idp != NULL; idp = idp->di_next) {
		if (idp == key)
			break;
		else
			pp = &idp->di_next;
	}

	assert(idp == key);
	*pp = idp->di_next;

	assert(dhp->dh_nelems != 0);
	dhp->dh_nelems--;

	dt_ident_destroy(idp);
}

static int
dt_idhash_comp(const void *lp, const void *rp)
{
	const dt_ident_t *lhs = *((const dt_ident_t **)lp);
	const dt_ident_t *rhs = *((const dt_ident_t **)rp);

	return ((int)(lhs->di_id - rhs->di_id));
}

void
dt_idhash_iter(dt_idhash_t *dhp, dt_idhash_f *func, void *data)
{
	dt_ident_t **ids;
	dt_ident_t *idp;
	ulong_t i, j;

	if (dhp->dh_tmpl != NULL)
		dt_idhash_populate(dhp); /* fill hash w/ initial population */

	ids = alloca(sizeof (dt_ident_t *) * dhp->dh_nelems);

	for (i = 0, j = 0; i < dhp->dh_hashsz; i++) {
		for (idp = dhp->dh_hash[i]; idp != NULL; idp = idp->di_next)
			ids[j++] = idp;
	}

	qsort(ids, dhp->dh_nelems, sizeof (dt_ident_t *), dt_idhash_comp);

	for (i = 0; i < dhp->dh_nelems; i++)
		func(dhp, ids[i], data);
}

dt_ident_t *
dt_ident_create(const char *name, ushort_t kind, ushort_t flags, uint_t id,
    dtrace_attribute_t attr, uint_t vers,
    const dt_idops_t *ops, void *iarg, ulong_t gen)
{
	dt_ident_t *idp;
	char *s = NULL;

	if ((name != NULL && (s = strdup(name)) == NULL) ||
	    (idp = malloc(sizeof (dt_ident_t))) == NULL) {
		free(s);
		return (NULL);
	}

	idp->di_name = s;
	idp->di_kind = kind;
	idp->di_flags = flags;
	idp->di_id = id;
	idp->di_attr = attr;
	idp->di_vers = vers;
	idp->di_ops = ops;
	idp->di_iarg = iarg;
	idp->di_data = NULL;
	idp->di_ctfp = NULL;
	idp->di_type = CTF_ERR;
	idp->di_next = NULL;
	idp->di_gen = gen;
	idp->di_lineno = yylineno;

	return (idp);
}

void
dt_ident_destroy(dt_ident_t *idp)
{
	idp->di_ops->di_dtor(idp);
	free(idp->di_name);
	free(idp);
}

void
dt_ident_morph(dt_ident_t *idp, ushort_t kind,
    const dt_idops_t *ops, void *iarg)
{
	idp->di_ops->di_dtor(idp);
	idp->di_kind = kind;
	idp->di_ops = ops;
	idp->di_iarg = iarg;
	idp->di_data = NULL;
}

dtrace_attribute_t
dt_ident_cook(dt_node_t *dnp, dt_ident_t *idp, dt_node_t **pargp)
{
	dtrace_attribute_t attr;
	dt_node_t *args, *argp;
	int argc = 0;

	attr = dt_node_list_cook(pargp, DT_IDFLG_REF);
	args = pargp ? *pargp : NULL;

	for (argp = args; argp != NULL; argp = argp->dn_list)
		argc++;

	if (idp == dnp->dn_ident) {
		/*
		 * If the ident we've been passed is the ident that corresponds
		 * to the node, we're going to call through the identifier ops
		 * to construct the identifier -- potentially changing it.
		 */
		dnp->dn_ident = idp->di_ops->di_ctor(dnp, argc, args);
		idp = dnp->dn_ident;
	}

	idp->di_ops->di_cook(dnp, idp, argc, args);

	if (idp->di_flags & DT_IDFLG_USER)
		dnp->dn_flags |= DT_NF_USERLAND;

	return (dt_attr_min(attr, idp->di_attr));
}

void
dt_ident_type_assign(dt_ident_t *idp, ctf_file_t *fp, ctf_id_t type)
{
	idp->di_ctfp = fp;
	idp->di_type = type;
}

dt_ident_t *
dt_ident_resolve(dt_ident_t *idp)
{
	while (idp->di_flags & DT_IDFLG_INLINE) {
		const dt_node_t *dnp = ((dt_idnode_t *)idp->di_data)->din_root;

		switch (dnp->dn_kind) {
		case DT_NODE_VAR:
		case DT_NODE_SYM:
		case DT_NODE_FUNC:
		case DT_NODE_AGG:
		case DT_NODE_INLINE:
			idp = dnp->dn_ident;
			continue;
		}

		if (dt_node_is_dynamic(dnp))
			idp = dnp->dn_ident;
		else
			break;
	}

	return (idp);
}

size_t
dt_ident_size(dt_ident_t *idp)
{
	idp = dt_ident_resolve(idp);
	return (idp->di_ops->di_size(idp));
}

int
dt_ident_unref(const dt_ident_t *idp)
{
	return (idp->di_gen == yypcb->pcb_hdl->dt_gen &&
	    (idp->di_flags & (DT_IDFLG_REF|DT_IDFLG_MOD|DT_IDFLG_DECL)) == 0);
}

const char *
dt_idkind_name(uint_t kind)
{
	switch (kind) {
	case DT_IDENT_ARRAY:	return ("associative array");
	case DT_IDENT_SCALAR:	return ("scalar");
	case DT_IDENT_PTR:	return ("pointer");
	case DT_IDENT_FUNC:	return ("function");
	case DT_IDENT_AGG:	return ("aggregation");
	case DT_IDENT_AGGFUNC:	return ("aggregating function");
	case DT_IDENT_ACTFUNC:	return ("tracing function");
	case DT_IDENT_XLSOU:	return ("translated data");
	case DT_IDENT_XLPTR:	return ("pointer to translated data");
	case DT_IDENT_SYMBOL:	return ("external symbol reference");
	case DT_IDENT_ENUM:	return ("enumerator");
	case DT_IDENT_PRAGAT:	return ("#pragma attributes");
	case DT_IDENT_PRAGBN:	return ("#pragma binding");
	case DT_IDENT_PROBE:	return ("probe definition");
	default:		return ("<?>");
	}
}

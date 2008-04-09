/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)dt_cc.c	1.14	04/12/18 SMI"

/*
 * DTrace D Language Compiler
 *
 * The code in this source file implements the main engine for the D language
 * compiler.  The driver routine for the compiler is dt_compile(), below.  The
 * compiler operates on either stdio FILEs or in-memory strings as its input
 * and can produce either dtrace_prog_t structures from a D program or a single
 * dtrace_difo_t structure from a D expression.  Multiple entry points are
 * provided as wrappers around dt_compile() for the various input/output pairs.
 * The compiler itself is implemented across the following source files:
 *
 * dt_lex.l - lex scanner
 * dt_grammar.y - yacc grammar
 * dt_parser.c - parse tree creation and semantic checking
 * dt_decl.c - declaration stack processing
 * dt_xlator.c - D translator lookup and creation
 * dt_ident.c - identifier and symbol table routines
 * dt_pragma.c - #pragma processing and D pragmas
 * dt_printf.c - D printf() and printa() argument checking and processing
 * dt_cc.c - compiler driver and dtrace_prog_t construction
 * dt_cg.c - DIF code generator
 * dt_as.c - DIF assembler
 * dt_dof.c - dtrace_prog_t -> DOF conversion
 *
 * Several other source files provide collections of utility routines used by
 * these major files.  The compiler itself is implemented in multiple passes:
 *
 * (1) The input program is scanned and parsed by dt_lex.l and dt_grammar.y
 *     and parse tree nodes are constructed using the routines in dt_parser.c.
 *     This node construction pass is described further in dt_parser.c.
 *
 * (2) The parse tree is "cooked" by assigning each clause a context (see the
 *     routine dt_setcontext(), below) based on its probe description and then
 *     recursively descending the tree performing semantic checking.  The cook
 *     routines are also implemented in dt_parser.c and described there.
 *
 * (3) For actions that are DIF expression statements, the DIF code generator
 *     and assembler are invoked to create a finished DIFO for the statement.
 *
 * (4) The dtrace_prog_t data structures for the program clauses and actions
 *     are built, containing pointers to any DIFOs created in step (3).
 *
 * (5) The caller invokes a routine in dt_dof.c to convert the finished program
 *     into DOF format for use in anonymous tracing or enabling in the kernel.
 *
 * In the implementation, steps 2-4 are intertwined in that they are performed
 * in order for each clause as part of a loop that executes over the clauses.
 *
 * The D compiler currently implements nearly no optimization.  The compiler
 * implements integer constant folding as part of pass (1), and a set of very
 * simple peephole optimizations as part of pass (3).  As with any C compiler,
 * a large number of optimizations are possible on both the intermediate data
 * structures and the generated DIF code.  These possibilities should be
 * investigated in the context of whether they will have any substantive effect
 * on the overall DTrace probe effect before they are undertaken.
 */

#include <linux_types.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ucontext.h>
#include <limits.h>
#include <alloca.h>
#include <ctype.h>

#define	_POSIX_PTHREAD_SEMANTICS
#include <dirent.h>
#undef	_POSIX_PTHREAD_SEMANTICS

#include <dt_module.h>
#include <dt_provider.h>
#include <dt_printf.h>
#include <dt_pid.h>
#include <dt_grammar.h>
#include <dt_ident.h>
#include <dt_string.h>
#include <dt_impl.h>

static const dtrace_pattr_t dt_def_pattr = {
{ DTRACE_STABILITY_UNSTABLE, DTRACE_STABILITY_UNSTABLE, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_UNSTABLE, DTRACE_STABILITY_UNSTABLE, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_UNSTABLE, DTRACE_STABILITY_UNSTABLE, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_UNSTABLE, DTRACE_STABILITY_UNSTABLE, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_UNSTABLE, DTRACE_STABILITY_UNSTABLE, DTRACE_CLASS_COMMON },
};

static const dtrace_diftype_t dt_void_rtype = {
	DIF_TYPE_CTF, CTF_K_INTEGER, 0, 0, 0
};

static const dtrace_diftype_t dt_int_rtype = {
	DIF_TYPE_CTF, CTF_K_INTEGER, 0, 0, sizeof (uint64_t)
};

/*ARGSUSED*/
static void
dt_idreset(dt_idhash_t *dhp, dt_ident_t *idp, void *ignored)
{
	idp->di_flags &= ~(DT_IDFLG_REF | DT_IDFLG_MOD |
	    DT_IDFLG_DIFR | DT_IDFLG_DIFW);
}

static void
dt_idkill(dt_idhash_t *dhp, dt_ident_t *idp, void *gen)
{
	if (idp->di_gen == (ulong_t)gen)
		dt_idhash_delete(dhp, idp);
}

/*ARGSUSED*/
static void
dt_idpragma(dt_idhash_t *dhp, dt_ident_t *idp, void *ignored)
{
	yylineno = idp->di_lineno;
	xyerror(D_PRAGMA_UNUSED, "unused #pragma %s\n", (char *)idp->di_iarg);
}

static dtrace_stmtdesc_t *
dt_stmt_create(dtrace_hdl_t *dtp, dtrace_ecbdesc_t *edp,
    dtrace_attribute_t descattr, dtrace_attribute_t stmtattr)
{
	dtrace_stmtdesc_t *sdp = dtrace_stmt_create(dtp, edp);

	if (sdp == NULL)
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

	assert(yypcb->pcb_stmt == NULL);
	yypcb->pcb_stmt = sdp;

	sdp->dtsd_descattr = descattr;
	sdp->dtsd_stmtattr = stmtattr;

	return (sdp);
}

static dtrace_actdesc_t *
dt_stmt_action(dtrace_hdl_t *dtp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *new;

	if ((new = dtrace_stmt_action(dtp, sdp)) == NULL)
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

	return (new);
}

/*
 * Utility function to determine if a given action description is destructive.
 * The dtdo_destructive bit is set for us by the DIF assembler (see dt_as.c).
 */
static int
dt_action_destructive(const dtrace_actdesc_t *ap)
{
	return (DTRACEACT_ISDESTRUCTIVE(ap->dtad_kind) || (ap->dtad_kind ==
	    DTRACEACT_DIFEXPR && ap->dtad_difo->dtdo_destructive));
}

static void
dt_stmt_append(dtrace_stmtdesc_t *sdp, const dt_node_t *dnp)
{
	dtrace_ecbdesc_t *edp = sdp->dtsd_ecbdesc;
	dtrace_actdesc_t *ap, *tap;
	int commit = 0;
	int speculate = 0;
	int datarec = 0;

	/*
	 * Make sure that the new statement jibes with the rest of the ECB.
	 */
	for (ap = edp->dted_action; ap != NULL; ap = ap->dtad_next) {
		if (ap->dtad_kind == DTRACEACT_COMMIT) {
			if (commit) {
				dnerror(dnp, D_COMM_COMM, "commit( ) may "
				    "not follow commit( )\n");
			}

			if (datarec) {
				dnerror(dnp, D_COMM_DREC, "commit( ) may "
				    "not follow data-recording action(s)\n");
			}

			for (tap = ap; tap != NULL; tap = tap->dtad_next) {
				if (!DTRACEACT_ISAGG(tap->dtad_kind))
					continue;

				dnerror(dnp, D_AGG_COMM, "aggregating actions "
				    "may not follow commit( )\n");
			}

			commit = 1;
			continue;
		}

		if (ap->dtad_kind == DTRACEACT_SPECULATE) {
			if (speculate) {
				dnerror(dnp, D_SPEC_SPEC, "speculate( ) may "
				    "not follow speculate( )\n");
			}

			if (commit) {
				dnerror(dnp, D_SPEC_COMM, "speculate( ) may "
				    "not follow commit( )\n");
			}

			if (datarec) {
				dnerror(dnp, D_SPEC_DREC, "speculate( ) may "
				    "not follow data-recording action(s)\n");
			}

			speculate = 1;
			continue;
		}

		if (DTRACEACT_ISAGG(ap->dtad_kind)) {
			if (speculate) {
				dnerror(dnp, D_AGG_SPEC, "aggregating actions "
				    "may not follow speculate( )\n");
			}

			datarec = 1;
			continue;
		}

		if (speculate) {
			if (dt_action_destructive(ap)) {
				dnerror(dnp, D_ACT_SPEC, "destructive actions "
				    "may not follow speculate( )\n");
			}

			if (ap->dtad_kind == DTRACEACT_EXIT) {
				dnerror(dnp, D_EXIT_SPEC, "exit( ) may not "
				    "follow speculate( )\n");
			}
		}

		/*
		 * Exclude all non data-recording actions.
		 */
		if (dt_action_destructive(ap) ||
		    ap->dtad_kind == DTRACEACT_DISCARD)
			continue;

		if (ap->dtad_kind == DTRACEACT_DIFEXPR &&
		    ap->dtad_difo->dtdo_rtype.dtdt_kind == DIF_TYPE_CTF &&
		    ap->dtad_difo->dtdo_rtype.dtdt_size == 0)
			continue;

		if (commit) {
			dnerror(dnp, D_DREC_COMM, "data-recording actions "
			    "may not follow commit( )\n");
		}

		if (!speculate)
			datarec = 1;
	}

	if (dtrace_stmt_add(yypcb->pcb_hdl, yypcb->pcb_prog, sdp) != 0)
		longjmp(yypcb->pcb_jmpbuf, dtrace_errno(yypcb->pcb_hdl));

	if (yypcb->pcb_stmt == sdp)
		yypcb->pcb_stmt = NULL;
}

/*
 * For the first element of an aggregation tuple or for printa(), we create a
 * simple DIF program that simply returns the immediate value that is the ID
 * of the aggregation itself.  This could be optimized in the future by
 * creating a new in-kernel dtad_kind that just returns an integer.
 */
static void
dt_action_difconst(dtrace_actdesc_t *ap, uint_t id, dtrace_actkind_t kind)
{
	dtrace_difo_t *dp = malloc(sizeof (dtrace_difo_t));

	if (dp == NULL)
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

	bzero(dp, sizeof (dtrace_difo_t));
	dtrace_difo_hold(dp);

	dp->dtdo_buf = malloc(sizeof (dif_instr_t) * 2);
	dp->dtdo_inttab = malloc(sizeof (uint64_t));

	if (dp->dtdo_buf == NULL || dp->dtdo_inttab == NULL) {
		dtrace_difo_release(dp);
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);
	}

	dp->dtdo_buf[0] = DIF_INSTR_SETX(0, 1); /* setx	DIF_INTEGER[0], %r1 */
	dp->dtdo_buf[1] = DIF_INSTR_RET(1);	/* ret	%r1 */
	dp->dtdo_len = 2;
	dp->dtdo_inttab[0] = id;
	dp->dtdo_intlen = 1;
	dp->dtdo_rtype = dt_int_rtype;

	ap->dtad_difo = dp;
	ap->dtad_kind = kind;
}

static void
dt_action_clear(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dt_ident_t *aid;
	dtrace_actdesc_t *ap;
	dt_node_t *anp;

	char n[DT_TYPE_NAMELEN];
	int argc = 0;

	for (anp = dnp->dn_args; anp != NULL; anp = anp->dn_list)
		argc++; /* count up arguments for error messages below */

	if (argc != 1) {
		dnerror(dnp, D_CLEAR_PROTO,
		    "%s( ) prototype mismatch: %d args passed, 1 expected\n",
		    dnp->dn_ident->di_name, argc);
	}

	anp = dnp->dn_args;
	assert(anp != NULL);

	if (anp->dn_kind != DT_NODE_AGG) {
		dnerror(dnp, D_CLEAR_AGGARG,
		    "%s( ) argument #1 is incompatible with prototype:\n"
		    "\tprototype: aggregation\n\t argument: %s\n",
		    dnp->dn_ident->di_name,
		    dt_node_type_name(anp, n, sizeof (n)));
	}

	aid = anp->dn_ident;

	if (aid->di_gen == dtp->dt_gen && !(aid->di_flags & DT_IDFLG_MOD)) {
		dnerror(dnp, D_CLEAR_AGGBAD,
		    "undefined aggregation: @%s\n", aid->di_name);
	}

	ap = dt_stmt_action(dtp, sdp);
	dt_action_difconst(ap, anp->dn_ident->di_id, DTRACEACT_LIBACT);
	ap->dtad_arg = DT_ACT_CLEAR;
}

static void
dt_action_normalize(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dt_ident_t *aid;
	dtrace_actdesc_t *ap;
	dt_node_t *anp, *normal;
	int denormal = (strcmp(dnp->dn_ident->di_name, "denormalize") == 0);

	char n[DT_TYPE_NAMELEN];
	int argc = 0;

	for (anp = dnp->dn_args; anp != NULL; anp = anp->dn_list)
		argc++; /* count up arguments for error messages below */

	if ((denormal && argc != 1) || (!denormal && argc != 2)) {
		dnerror(dnp, D_NORMALIZE_PROTO,
		    "%s( ) prototype mismatch: %d args passed, %d expected\n",
		    dnp->dn_ident->di_name, argc, denormal ? 1 : 2);
	}

	anp = dnp->dn_args;
	assert(anp != NULL);

	if (anp->dn_kind != DT_NODE_AGG) {
		dnerror(dnp, D_NORMALIZE_AGGARG,
		    "%s( ) argument #1 is incompatible with prototype:\n"
		    "\tprototype: aggregation\n\t argument: %s\n",
		    dnp->dn_ident->di_name,
		    dt_node_type_name(anp, n, sizeof (n)));
	}

	if ((normal = anp->dn_list) != NULL && !dt_node_is_scalar(normal)) {
		dnerror(dnp, D_NORMALIZE_SCALAR,
		    "%s( ) argument #2 must be of scalar type\n",
		    dnp->dn_ident->di_name);
	}

	aid = anp->dn_ident;

	if (aid->di_gen == dtp->dt_gen && !(aid->di_flags & DT_IDFLG_MOD)) {
		dnerror(dnp, D_NORMALIZE_AGGBAD,
		    "undefined aggregation: @%s\n", aid->di_name);
	}

	ap = dt_stmt_action(dtp, sdp);
	dt_action_difconst(ap, anp->dn_ident->di_id, DTRACEACT_LIBACT);

	if (denormal) {
		ap->dtad_arg = DT_ACT_DENORMALIZE;
		return;
	}

	ap->dtad_arg = DT_ACT_NORMALIZE;

	assert(normal != NULL);
	ap = dt_stmt_action(dtp, sdp);
	dt_cg(yypcb, normal);

	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_LIBACT;
	ap->dtad_arg = DT_ACT_NORMALIZE;
}

static void
dt_action_trunc(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dt_ident_t *aid;
	dtrace_actdesc_t *ap;
	dt_node_t *anp, *trunc;

	char n[DT_TYPE_NAMELEN];
	int argc = 0;

	for (anp = dnp->dn_args; anp != NULL; anp = anp->dn_list)
		argc++; /* count up arguments for error messages below */

	if (argc > 2 || argc < 1) {
		dnerror(dnp, D_TRUNC_PROTO,
		    "%s( ) prototype mismatch: %d args passed, %s expected\n",
		    dnp->dn_ident->di_name, argc,
		    argc < 1 ? "at least 1" : "no more than 2");
	}

	anp = dnp->dn_args;
	assert(anp != NULL);
	trunc = anp->dn_list;

	if (anp->dn_kind != DT_NODE_AGG) {
		dnerror(dnp, D_TRUNC_AGGARG,
		    "%s( ) argument #1 is incompatible with prototype:\n"
		    "\tprototype: aggregation\n\t argument: %s\n",
		    dnp->dn_ident->di_name,
		    dt_node_type_name(anp, n, sizeof (n)));
	}

	if (argc == 2) {
		assert(trunc != NULL);
		if (!dt_node_is_scalar(trunc)) {
			dnerror(dnp, D_TRUNC_SCALAR,
			    "%s( ) argument #2 must be of scalar type\n",
			    dnp->dn_ident->di_name);
		}
	}

	aid = anp->dn_ident;

	if (aid->di_gen == dtp->dt_gen && !(aid->di_flags & DT_IDFLG_MOD)) {
		dnerror(dnp, D_TRUNC_AGGBAD,
		    "undefined aggregation: @%s\n", aid->di_name);
	}

	ap = dt_stmt_action(dtp, sdp);
	dt_action_difconst(ap, anp->dn_ident->di_id, DTRACEACT_LIBACT);
	ap->dtad_arg = DT_ACT_TRUNC;

	ap = dt_stmt_action(dtp, sdp);

	if (argc == 1) {
		dt_action_difconst(ap, 0, DTRACEACT_LIBACT);
	} else {
		assert(trunc != NULL);
		dt_cg(yypcb, trunc);
		ap->dtad_difo = dt_as(yypcb);
		ap->dtad_kind = DTRACEACT_LIBACT;
	}

	ap->dtad_arg = DT_ACT_TRUNC;
}

static void
dt_action_printa(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dt_ident_t *aid, *fid;
	dtrace_actdesc_t *ap;
	const char *format;
	dt_node_t *anp;

	char n[DT_TYPE_NAMELEN];
	int argc = 0, argr = 0;

	for (anp = dnp->dn_args; anp != NULL; anp = anp->dn_list)
		argc++; /* count up arguments for error messages below */

	switch (dnp->dn_args->dn_kind) {
	case DT_NODE_STRING:
		format = dnp->dn_args->dn_string;
		anp = dnp->dn_args->dn_list;
		argr = 2;
		break;
	case DT_NODE_AGG:
		format = NULL;
		anp = dnp->dn_args;
		argr = 1;
		break;
	default:
		format = NULL;
		anp = dnp->dn_args;
		argr = 1;
	}

	if (argc != argr) {
		dnerror(dnp, D_PRINTA_PROTO,
		    "%s( ) prototype mismatch: %d args passed, %d expected\n",
		    dnp->dn_ident->di_name, argc, argr);
	}

	if (anp->dn_kind != DT_NODE_AGG) {
		dnerror(dnp, D_PRINTA_AGGARG,
		    "%s( ) argument #%d is incompatible with prototype:\n"
		    "\tprototype: aggregation\n\t argument: %s\n",
		    dnp->dn_ident->di_name, argr,
		    dt_node_type_name(anp, n, sizeof (n)));
	}

	aid = anp->dn_ident;
	fid = aid->di_iarg;

	if (aid->di_gen == dtp->dt_gen && !(aid->di_flags & DT_IDFLG_MOD)) {
		dnerror(dnp, D_PRINTA_AGGBAD,
		    "undefined aggregation: @%s\n", aid->di_name);
	}

	if (format != NULL) {
		yylineno = dnp->dn_line;

		sdp->dtsd_fmtdata = dt_printf_create(yypcb->pcb_hdl, format);
		dt_printf_validate(sdp->dtsd_fmtdata,
		    DT_PRINTF_AGGREGATION, dnp->dn_ident, 1,
		    fid->di_id, ((dt_idsig_t *)aid->di_data)->dis_args);
	}

	ap = dt_stmt_action(dtp, sdp);
	dt_action_difconst(ap, anp->dn_ident->di_id, DTRACEACT_PRINTA);
}

static void
dt_action_printflike(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp,
    dtrace_actkind_t kind)
{
	dt_node_t *anp, *arg1;
	dtrace_actdesc_t *ap = NULL;
	char n[DT_TYPE_NAMELEN];

	assert(DTRACEACT_ISPRINTFLIKE(kind));

	if (dnp->dn_args->dn_kind != DT_NODE_STRING) {
		dnerror(dnp, D_PRINTF_ARG_FMT,
		    "%s( ) argument #1 is incompatible with prototype:\n"
		    "\tprototype: string constant\n\t argument: %s\n",
		    dnp->dn_ident->di_name,
		    dt_node_type_name(dnp->dn_args, n, sizeof (n)));
	}

	arg1 = dnp->dn_args->dn_list;
	yylineno = dnp->dn_line;

	sdp->dtsd_fmtdata = dt_printf_create(yypcb->pcb_hdl,
	    dnp->dn_args->dn_string);

	dt_printf_validate(sdp->dtsd_fmtdata, DT_PRINTF_EXACTLEN,
	    dnp->dn_ident, 1, DTRACEACT_AGGREGATION, arg1);

	if (arg1 == NULL) {
		dif_instr_t *dbuf;
		dtrace_difo_t *dp;

		if ((dbuf = malloc(sizeof (dif_instr_t))) == NULL ||
		    (dp = malloc(sizeof (dtrace_difo_t))) == NULL) {
			free(dbuf);
			longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);
		}

		dbuf[0] = DIF_INSTR_RET(DIF_REG_R0); /* ret %r0 */

		bzero(dp, sizeof (dtrace_difo_t));
		dp->dtdo_buf = dbuf;
		dp->dtdo_len = 1;
		dp->dtdo_rtype = dt_int_rtype;
		dp->dtdo_refcnt = 1;

		ap = dt_stmt_action(dtp, sdp);
		ap->dtad_difo = dp;
		ap->dtad_kind = kind;
		return;
	}

	for (anp = arg1; anp != NULL; anp = anp->dn_list) {
		ap = dt_stmt_action(dtp, sdp);
		dt_cg(yypcb, anp);
		ap->dtad_difo = dt_as(yypcb);
		ap->dtad_kind = kind;
	}
}

static void
dt_action_trace(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	if (dt_node_is_void(dnp->dn_args)) {
		dnerror(dnp->dn_args, D_TRACE_VOID,
		    "trace( ) may not be applied to a void expression\n");
	}

	if (dt_node_is_dynamic(dnp->dn_args)) {
		dnerror(dnp->dn_args, D_TRACE_DYN,
		    "trace( ) may not be applied to a dynamic expression\n");
	}

	dt_cg(yypcb, dnp->dn_args);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_DIFEXPR;
}

static void
dt_action_tracemem(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	dt_node_t *addr = dnp->dn_args;
	dt_node_t *size = dnp->dn_args->dn_list;

	char n[DT_TYPE_NAMELEN];

	if (dt_node_is_integer(addr) == 0 && dt_node_is_pointer(addr) == 0) {
		dnerror(addr, D_TRACEMEM_ADDR,
		    "tracemem( ) argument #1 is incompatible with "
		    "prototype:\n\tprototype: pointer or integer\n"
		    "\t argument: %s\n",
		    dt_node_type_name(addr, n, sizeof (n)));
	}

	if (dt_node_is_posconst(size) == 0) {
		dnerror(size, D_TRACEMEM_SIZE, "tracemem( ) argument #2 must "
		    "be a non-zero positive integral constant expression\n");
	}

	dt_cg(yypcb, addr);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_DIFEXPR;

	ap->dtad_difo->dtdo_rtype.dtdt_flags |= DIF_TF_BYREF;
	ap->dtad_difo->dtdo_rtype.dtdt_size = size->dn_value;
}

static void
dt_action_stack_args(dtrace_hdl_t *dtp, dtrace_actdesc_t *ap, dt_node_t *arg0)
{
	ap->dtad_kind = DTRACEACT_STACK;

	if (dtp->dt_options[DTRACEOPT_STACKFRAMES] != DTRACEOPT_UNSET) {
		ap->dtad_arg = dtp->dt_options[DTRACEOPT_STACKFRAMES];
	} else {
		ap->dtad_arg = 0;
	}

	if (arg0 != NULL) {
		if (arg0->dn_list != NULL) {
			dnerror(arg0, D_STACK_PROTO, "stack( ) prototype "
			    "mismatch: too many arguments\n");
		}

		if (dt_node_is_posconst(arg0) == 0) {
			dnerror(arg0, D_STACK_SIZE, "stack( ) size must be a "
			    "non-zero positive integral constant expression\n");
		}

		ap->dtad_arg = arg0->dn_value;
	}
}

static void
dt_action_stack(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);
	dt_action_stack_args(dtp, ap, dnp->dn_args);
}

static void
dt_action_ustack_args(dtrace_hdl_t *dtp, dtrace_actdesc_t *ap, dt_node_t *dnp)
{
	uint32_t nframes = 0;
	uint32_t strsize = 0;	/* default string table size */
	dt_node_t *arg0 = dnp->dn_args;
	dt_node_t *arg1 = arg0 != NULL ? arg0->dn_list : NULL;

	assert(dnp->dn_ident->di_id == DT_ACT_JSTACK ||
	    dnp->dn_ident->di_id == DT_ACT_USTACK);

	if (dnp->dn_ident->di_id == DT_ACT_JSTACK) {
		if (dtp->dt_options[DTRACEOPT_JSTACKFRAMES] != DTRACEOPT_UNSET)
			nframes = dtp->dt_options[DTRACEOPT_JSTACKFRAMES];

		if (dtp->dt_options[DTRACEOPT_JSTACKSTRSIZE] != DTRACEOPT_UNSET)
			strsize = dtp->dt_options[DTRACEOPT_JSTACKSTRSIZE];

		ap->dtad_kind = DTRACEACT_JSTACK;
	} else {
		assert(dnp->dn_ident->di_id == DT_ACT_USTACK);

		if (dtp->dt_options[DTRACEOPT_USTACKFRAMES] != DTRACEOPT_UNSET)
			nframes = dtp->dt_options[DTRACEOPT_USTACKFRAMES];

		ap->dtad_kind = DTRACEACT_USTACK;
	}

	if (arg0 != NULL) {
		if (!dt_node_is_posconst(arg0)) {
			dnerror(arg0, D_USTACK_FRAMES, "ustack( ) argument #1 "
			    "must be a non-zero positive integer constant\n");
		}
		nframes = (uint32_t)arg0->dn_value;
	}

	if (arg1 != NULL) {
		if (arg1->dn_kind != DT_NODE_INT ||
		    ((arg1->dn_flags & DT_NF_SIGNED) &&
		    (int64_t)arg1->dn_value < 0)) {
			dnerror(arg1, D_USTACK_STRSIZE, "ustack( ) argument #2 "
			    "must be a positive integer constant\n");
		}

		if (arg1->dn_list != NULL) {
			dnerror(arg1, D_USTACK_PROTO, "ustack( ) prototype "
			    "mismatch: too many arguments\n");
		}

		strsize = (uint32_t)arg1->dn_value;
	}

	ap->dtad_arg = DTRACE_USTACK_ARG(nframes, strsize);
}

static void
dt_action_ustack(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);
	dt_action_ustack_args(dtp, ap, dnp);
}

/*ARGSUSED*/
static void
dt_action_ftruncate(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	/*
	 * Library actions need a DIFO that serves as an argument.  As
	 * ftruncate() doesn't take an argument, we generate the constant 0
	 * in a DIFO; this constant will be ignored when the ftruncate() is
	 * processed.
	 */
	dt_action_difconst(ap, 0, DTRACEACT_LIBACT);
	ap->dtad_arg = DT_ACT_FTRUNCATE;
}

/*ARGSUSED*/
static void
dt_action_stop(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	ap->dtad_kind = DTRACEACT_STOP;
	ap->dtad_arg = 0;
}

/*ARGSUSED*/
static void
dt_action_breakpoint(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	ap->dtad_kind = DTRACEACT_BREAKPOINT;
	ap->dtad_arg = 0;
}

/*ARGSUSED*/
static void
dt_action_panic(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	ap->dtad_kind = DTRACEACT_PANIC;
	ap->dtad_arg = 0;
}

static void
dt_action_chill(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	dt_cg(yypcb, dnp->dn_args);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_CHILL;
}

static void
dt_action_raise(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	dt_cg(yypcb, dnp->dn_args);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_RAISE;
}

static void
dt_action_exit(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	dt_cg(yypcb, dnp->dn_args);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_EXIT;
	ap->dtad_difo->dtdo_rtype.dtdt_size = sizeof (int);
}

static void
dt_action_speculate(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	dt_cg(yypcb, dnp->dn_args);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_SPECULATE;
}

static void
dt_action_commit(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	dt_cg(yypcb, dnp->dn_args);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_COMMIT;
}

static void
dt_action_discard(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	dt_cg(yypcb, dnp->dn_args);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_kind = DTRACEACT_DISCARD;
}

static void
dt_compile_fun(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	switch (dnp->dn_expr->dn_ident->di_id) {
	case DT_ACT_BREAKPOINT:
		dt_action_breakpoint(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_CHILL:
		dt_action_chill(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_CLEAR:
		dt_action_clear(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_COMMIT:
		dt_action_commit(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_DENORMALIZE:
		dt_action_normalize(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_DISCARD:
		dt_action_discard(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_EXIT:
		dt_action_exit(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_FTRUNCATE:
		dt_action_ftruncate(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_NORMALIZE:
		dt_action_normalize(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_PANIC:
		dt_action_panic(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_PRINTA:
		dt_action_printa(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_PRINTF:
		dt_action_printflike(dtp, dnp->dn_expr, sdp, DTRACEACT_PRINTF);
		break;
	case DT_ACT_RAISE:
		dt_action_raise(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_SPECULATE:
		dt_action_speculate(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_STACK:
		dt_action_stack(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_STOP:
		dt_action_stop(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_SYSTEM:
		dt_action_printflike(dtp, dnp->dn_expr, sdp, DTRACEACT_SYSTEM);
		break;
	case DT_ACT_TRACE:
		dt_action_trace(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_TRACEMEM:
		dt_action_tracemem(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_TRUNC:
		dt_action_trunc(dtp, dnp->dn_expr, sdp);
		break;
	case DT_ACT_USTACK:
	case DT_ACT_JSTACK:
		dt_action_ustack(dtp, dnp->dn_expr, sdp);
		break;
	default:
		dnerror(dnp->dn_expr, D_UNKNOWN, "tracing function %s( ) is "
		    "not yet supported\n", dnp->dn_expr->dn_ident->di_name);
	}
}

static void
dt_compile_exp(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dtrace_actdesc_t *ap = dt_stmt_action(dtp, sdp);

	dt_cg(yypcb, dnp->dn_expr);
	ap->dtad_difo = dt_as(yypcb);
	ap->dtad_difo->dtdo_rtype = dt_void_rtype;
	ap->dtad_kind = DTRACEACT_DIFEXPR;
}

static void
dt_compile_agg(dtrace_hdl_t *dtp, dt_node_t *dnp, dtrace_stmtdesc_t *sdp)
{
	dt_ident_t *aid, *fid;
	dt_node_t *anp;
	dtrace_actdesc_t *ap;
	uint_t n = 1;

	/*
	 * If the aggregation has no aggregating function applied to it, then
	 * this statement has no effect.  Flag this as a programming error.
	 */
	if (dnp->dn_aggfun == NULL) {
		dnerror(dnp, D_AGG_NULL, "expression has null effect: @%s\n",
		    dnp->dn_ident->di_name);
	}

	aid = dnp->dn_ident;
	fid = dnp->dn_aggfun->dn_ident;

	if (dnp->dn_aggfun->dn_args != NULL &&
	    dt_node_is_scalar(dnp->dn_aggfun->dn_args) == 0) {
		dnerror(dnp->dn_aggfun, D_AGG_SCALAR, "%s( ) argument #1 must "
		    "be of scalar type\n", fid->di_name);
	}

	/*
	 * The ID of the aggregation itself is implicitly recorded as the first
	 * member of each aggregation tuple so we can distinguish them later.
	 */
	ap = dt_stmt_action(dtp, sdp);
	dt_action_difconst(ap, aid->di_id, DTRACEACT_DIFEXPR);

	for (anp = dnp->dn_aggtup; anp != NULL; anp = anp->dn_list) {
		ap = dt_stmt_action(dtp, sdp);
		n++;

		if (anp->dn_kind == DT_NODE_FUNC) {
			if (anp->dn_ident->di_id == DT_ACT_STACK) {
				dt_action_stack_args(dtp, ap, anp->dn_args);
				continue;
			}

			if (anp->dn_ident->di_id == DT_ACT_USTACK ||
			    anp->dn_ident->di_id == DT_ACT_JSTACK) {
				dt_action_ustack_args(dtp, ap, anp);
				continue;
			}
		}

		dt_cg(yypcb, anp);
		ap->dtad_difo = dt_as(yypcb);
		ap->dtad_kind = DTRACEACT_DIFEXPR;
	}

	assert(sdp->dtsd_aggdata == NULL);
	sdp->dtsd_aggdata = aid;

	ap = dt_stmt_action(dtp, sdp);
	assert(fid->di_kind == DT_IDENT_AGGFUNC);
	assert(DTRACEACT_ISAGG(fid->di_id));
	ap->dtad_kind = fid->di_id;
	ap->dtad_ntuple = n;

	if (dnp->dn_aggfun->dn_args != NULL) {
		dt_cg(yypcb, dnp->dn_aggfun->dn_args);
		ap->dtad_difo = dt_as(yypcb);
	}

	if (fid->di_id == DTRACEAGG_LQUANTIZE) {
		/*
		 * For linear quantization, we have between two and three
		 * arguments:
		 *
		 *    arg1 => Base value
		 *    arg2 => Limit value
		 *    arg3 => Quantization level step size (defaults to 1)
		 */
		dt_node_t *arg1 = dnp->dn_aggfun->dn_args->dn_list;
		dt_node_t *arg2 = arg1->dn_list;
		dt_node_t *arg3 = arg2->dn_list;
		uint64_t nlevels, step = 1;
		int64_t baseval, limitval;

		if (arg1->dn_kind != DT_NODE_INT) {
			dnerror(arg1, D_LQUANT_BASETYPE, "lquantize( ) "
			    "argument #1 must be an integer constant\n");
		}

		baseval = (int64_t)arg1->dn_value;

		if (baseval < INT32_MIN || baseval > INT32_MAX) {
			dnerror(arg1, D_LQUANT_BASEVAL, "lquantize( ) "
			    "argument #1 must be a 32-bit quantity\n");
		}

		if (arg2->dn_kind != DT_NODE_INT) {
			dnerror(arg2, D_LQUANT_LIMTYPE, "lquantize( ) "
			    "argument #2 must be an integer constant\n");
		}

		limitval = (int64_t)arg2->dn_value;

		if (limitval < INT32_MIN || limitval > INT32_MAX) {
			dnerror(arg2, D_LQUANT_LIMVAL, "lquantize( ) "
			    "argument #2 must be a 32-bit quantity\n");
		}

		if (limitval < baseval) {
			dnerror(dnp, D_LQUANT_MISMATCH,
			    "lquantize( ) base (argument #1) must be less "
			    "than limit (argument #2)\n");
		}

		if (arg3 != NULL) {
			if (!dt_node_is_posconst(arg3)) {
				dnerror(arg3, D_LQUANT_STEPTYPE, "lquantize( ) "
				    "argument #3 must be a non-zero positive "
				    "integer constant\n");
			}

			if ((step = arg3->dn_value) > UINT16_MAX) {
				dnerror(arg3, D_LQUANT_STEPVAL, "lquantize( ) "
				    "argument #3 must be a 16-bit quantity\n");
			}
		}

		nlevels = (limitval - baseval) / step;

		if (nlevels == 0) {
			dnerror(dnp, D_LQUANT_STEPLARGE,
			    "lquantize( ) step (argument #3) too large: must "
			    "have at least one quantization level\n");
		}

		if (nlevels > UINT16_MAX) {
			dnerror(dnp, D_LQUANT_STEPSMALL, "lquantize( ) step "
			    "(argument #3) too small: number of quantization "
			    "levels must be a 16-bit quantity\n");
		}

		ap->dtad_arg = (step << DTRACE_LQUANTIZE_STEPSHIFT) |
		    (nlevels << DTRACE_LQUANTIZE_LEVELSHIFT) |
		    ((baseval << DTRACE_LQUANTIZE_BASESHIFT) &
		    DTRACE_LQUANTIZE_BASEMASK);
	}
}

static void
dt_compile_one_clause(dtrace_hdl_t *dtp, dt_node_t *cnp, dt_node_t *pnp)
{
	dtrace_ecbdesc_t *edp;
	dtrace_stmtdesc_t *sdp;
	dt_node_t *dnp;

	yylineno = pnp->dn_line;
	dt_setcontext(dtp, pnp->dn_desc);
	(void) dt_node_cook(cnp, DT_IDFLG_REF);

	if (DT_TREEDUMP_PASS(dtp, 2))
		dt_node_printr(cnp, stderr, 0);

	if ((edp = dtrace_ecbdesc_create(dtp, pnp->dn_desc)) == NULL)
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

	assert(yypcb->pcb_ecbdesc == NULL);
	yypcb->pcb_ecbdesc = edp;

	if (cnp->dn_pred != NULL) {
		dt_cg(yypcb, cnp->dn_pred);
		edp->dted_pred.dtpdd_difo = dt_as(yypcb);
	}

	if (cnp->dn_acts == NULL) {
		dt_stmt_append(dt_stmt_create(dtp, edp,
		    cnp->dn_ctxattr, _dtrace_defattr), cnp);
	}

	for (dnp = cnp->dn_acts; dnp != NULL; dnp = dnp->dn_list) {
		assert(yypcb->pcb_stmt == NULL);
		sdp = dt_stmt_create(dtp, edp, cnp->dn_ctxattr, cnp->dn_attr);

		switch (dnp->dn_kind) {
		case DT_NODE_DEXPR:
			if (dnp->dn_expr->dn_kind == DT_NODE_AGG)
				dt_compile_agg(dtp, dnp->dn_expr, sdp);
			else
				dt_compile_exp(dtp, dnp, sdp);
			break;
		case DT_NODE_DFUNC:
			dt_compile_fun(dtp, dnp, sdp);
			break;
		case DT_NODE_AGG:
			dt_compile_agg(dtp, dnp, sdp);
			break;
		default:
			dnerror(dnp, D_UNKNOWN, "internal error -- node kind "
			    "%u is not a valid statement\n", dnp->dn_kind);
		}

		assert(yypcb->pcb_stmt == sdp);
		dt_stmt_append(sdp, dnp);
	}

	assert(yypcb->pcb_ecbdesc == edp);
	dtrace_ecbdesc_release(edp);
	dt_endcontext(dtp);
	yypcb->pcb_ecbdesc = NULL;
}

static void
dt_compile_clause(dtrace_hdl_t *dtp, dt_node_t *cnp)
{
	dt_node_t *pnp;

	for (pnp = cnp->dn_pdescs; pnp != NULL; pnp = pnp->dn_list)
		dt_compile_one_clause(dtp, cnp, pnp);
}

void
dt_setcontext(dtrace_hdl_t *dtp, dtrace_probedesc_t *pdp)
{
	dtrace_probedesc_t pd, mpd;
	uint_t n, narg;

	dtrace_id_t id = pdp->dtpd_id;
	dt_module_t *dmp = NULL;

	dt_provider_t *pvp = NULL;
	const dtrace_pattr_t *pap;
	dt_ident_t *idp;
	char attrstr[8];
	dtrace_argdesc_t arg;

	/*
	 * If the provider name ends with what could be interpreted as a
	 * number, we assume that it's a pid and that we may need to
	 * dynamically create those probes for that process.
	 */
	if (isdigit(pdp->dtpd_provider[strlen(pdp->dtpd_provider) - 1]))
		dt_pid_create_probes(pdp, dtp);

	/*
	 * If the provider is specified and is not a glob-matching expression,
	 * then attempt to discover the stability attributes of the specified
	 * provider.  If the provider is unspecified or this fails, we set the
	 * provider attributes to a default set of Unstable attributes.  We
	 * then compute the clause stability by taking the minimum stability
	 * and dependency class of all specified probe description members.
	 */
	if (pdp->dtpd_provider[0] != '\0' && !strisglob(pdp->dtpd_provider))
		pvp = dt_provider_lookup(dtp, pdp->dtpd_provider);

	if (pvp != NULL)
		pap = &pvp->pv_desc.dtvd_attr;
	else
		pap = &dt_def_pattr;

	yypcb->pcb_attr = pap->dtpa_provider;
	if (pdp->dtpd_mod[0] != '\0' && !strisglob(pdp->dtpd_mod))
		yypcb->pcb_attr = dt_attr_min(yypcb->pcb_attr, pap->dtpa_mod);
	if (pdp->dtpd_func[0] != '\0' && !strisglob(pdp->dtpd_func))
		yypcb->pcb_attr = dt_attr_min(yypcb->pcb_attr, pap->dtpa_func);
	if (pdp->dtpd_name[0] != '\0' && !strisglob(pdp->dtpd_name))
		yypcb->pcb_attr = dt_attr_min(yypcb->pcb_attr, pap->dtpa_name);

	/*
	 * Convert the caller's partial probe description into a fully-formed
	 * matching probe description by invoking DTRACEIOC_PROBEMATCH.  We
	 * select the module resulting from the first match.  We do this at
	 * most twice in order to determine if the description matches exactly
	 * one probe, in which case the args[] array becomes valid for use.
	 */
	for (n = 0; n < 2; id = pd.dtpd_id + 1) {
		bcopy(pdp, &pd, sizeof (dtrace_probedesc_t));
		pd.dtpd_id = id;

		if (dt_ioctl(dtp, DTRACEIOC_PROBEMATCH, &pd) == -1) {
			if (errno == ESRCH || errno == EBADF)
				break; /* no more matching probes */

			xyerror(D_PDESC_GLOB, "probe description %s:%s:%s:%s "
			    "contains too many glob meta-characters\n",
			    pdp->dtpd_provider, pdp->dtpd_mod,
			    pdp->dtpd_func, pdp->dtpd_name);
		}

		if (n < 1 && pd.dtpd_mod[0] != '\0' &&
		    (dmp = dt_module_create(dtp, pd.dtpd_mod)) == NULL)
			longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

		if (n++ == 0)
			bcopy(&pd, &mpd, sizeof (dtrace_probedesc_t));

		if (pdp->dtpd_id != 0)
			break; /* id was specified by caller */
	}

	dt_dprintf("context %s:%s:%s:%s [%u] matched %u probes (mod %s) %s\n",
	    pdp->dtpd_provider, pdp->dtpd_mod, pdp->dtpd_func, pdp->dtpd_name,
	    pdp->dtpd_id, n, dmp == NULL ? "<exec>" : dmp->dm_name,
	    dt_attr_str(yypcb->pcb_attr, attrstr, sizeof (attrstr)));

	if (n == 0 && !(yypcb->pcb_cflags & DTRACE_C_ZDEFS)) {
		xyerror(D_PDESC_ZERO, "probe description %s:%s:%s:%s does not "
		    "match any probes\n", pdp->dtpd_provider, pdp->dtpd_mod,
		    pdp->dtpd_func, pdp->dtpd_name);
	}

	/*
	 * Reset the stability attributes of D global variables that vary
	 * based on the attributes of the provider and context itself.
	 */
	if ((idp = dt_idhash_lookup(dtp->dt_globals, "probeprov")) != NULL)
		idp->di_attr = pap->dtpa_provider;
	if ((idp = dt_idhash_lookup(dtp->dt_globals, "probemod")) != NULL)
		idp->di_attr = pap->dtpa_mod;
	if ((idp = dt_idhash_lookup(dtp->dt_globals, "probefunc")) != NULL)
		idp->di_attr = pap->dtpa_func;
	if ((idp = dt_idhash_lookup(dtp->dt_globals, "probename")) != NULL)
		idp->di_attr = pap->dtpa_name;
	if ((idp = dt_idhash_lookup(dtp->dt_globals, "args")) != NULL)
		idp->di_attr = pap->dtpa_args;

	/*
	 * Save the number of matching probes associated with the current
	 * probe description, and free any arguments from any previous context.
	 */
	yypcb->pcb_pdesc = pdp;
	yypcb->pcb_nprobes = n;

	while (yypcb->pcb_pargs != NULL) {
		dt_arg_t *old = yypcb->pcb_pargs;

		yypcb->pcb_pargs = old->da_next;
		free(old);
	}

	/*
	 * There are two conditions under which we query for the type and
	 * mapping of the args[] array:
	 *
	 * (1) Exactly one probe is matched.
	 *
	 * (2) Multiple probes are matched, but all of the following are true:
	 *
	 *	(a) The Arguments Data stability of the provider is at least
	 *	    Evolving.
	 *
	 *	(b) All of the Name stability components that are at least
	 *	    Evolving have been explicitly specified.
	 *
	 *	(c) No Evolving Name stability component has been specified
	 *	    using globbing.
	 *
	 * As this implies, providers that provide Evolving or better Arguments
	 * Data stability are required to guarantee that all probes that have
	 * like Evolving or better Name stability components have identical
	 * argument types and mappings.
	 */
	if (n != 1) {
		if (pap->dtpa_args.dtat_data < DTRACE_STABILITY_EVOLVING)
			return;

		if ((pap->dtpa_mod.dtat_name >= DTRACE_STABILITY_EVOLVING) &&
		    (pdp->dtpd_mod[0] == '\0' || strisglob(pdp->dtpd_mod)))
			return;

		if ((pap->dtpa_func.dtat_name >= DTRACE_STABILITY_EVOLVING) &&
		    (pdp->dtpd_func[0] == '\0' || strisglob(pdp->dtpd_func)))
			return;

		if ((pap->dtpa_name.dtat_name >= DTRACE_STABILITY_EVOLVING) &&
		    (pdp->dtpd_name[0] == '\0' || strisglob(pdp->dtpd_name)))
			return;
	}

	/*
	 * We need to explicitly query for the types and mappings of the
	 * arguments.
	 */
	bzero(&arg, sizeof (arg));
	arg.dtargd_id = mpd.dtpd_id;

	for (narg = 0; ; narg++) {
		dt_arg_t *argp;
		dtrace_typeinfo_t ntv, xlt;
		dt_node_t sn, dn;
		dt_xlator_t *dxp;
		int identical;

		arg.dtargd_ndx = narg;

		if (dt_ioctl(dtp, DTRACEIOC_PROBEARG, &arg) == -1) {
			xyerror(D_UNKNOWN, "failed to get argument for "
			    "probe %d: %s\n", pd.dtpd_id, strerror(errno));
		}

		if (arg.dtargd_ndx == DTRACE_ARGNONE)
			break;

		if ((argp = malloc(sizeof (dt_arg_t))) == NULL)
			longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

		bzero(argp, sizeof (dt_arg_t));
		argp->da_ndx = narg;
		argp->da_mapping = arg.dtargd_mapping;
		argp->da_next = yypcb->pcb_pargs;
		yypcb->pcb_pargs = argp;

		identical = (strcmp(arg.dtargd_native, arg.dtargd_xlate) == 0);

retry:
		if (dt_type_lookup(arg.dtargd_native, &ntv) == -1) {
			/*
			 * There are conditions (especially with function
			 * pointers) where dt_type_lookup() will fail to find
			 * the proper name.  We have an incredibly naive
			 * heuristic to detect this case, and fall back to
			 * "void *".
			 */
			if (strstr(arg.dtargd_native, "(*)") != NULL) {
				(void) strcpy(arg.dtargd_native, "void *");
				goto retry;
			}

			/*
			 * Unfortunately, a condition can arise when we cannot
			 * look up the valid type of a function argument:  if
			 * an argument is of a pointer type when the pointer
			 * type only exists in the child container and the
			 * base type only exists in the parent, we will be
			 * unable to lookup the type.  Ultimately, this
			 * situation needs to be rectified in
			 * dt_type_lookup(); for now, we just fall back to
			 * "void *" for these types -- forcing the user to
			 * explicitly cast to the true type as required.
			 */
			if (strstr(arg.dtargd_native, "*") != NULL) {
				(void) strcpy(arg.dtargd_native, "void *");
				goto retry;
			}

			xyerror(D_UNKNOWN, "failed to resolve native argument"
			    " type \"%s\" (arg#%d): %s", arg.dtargd_native,
			    narg, dtrace_errmsg(dtp, dtrace_errno(dtp)));
		}

		argp->da_type = ntv.dtt_type;
		argp->da_ctfp = ntv.dtt_ctfp;

		/*
		 * If there's no translated type or it's identical to the
		 * native type, we can skip the translation portion.
		 */
		if (identical || arg.dtargd_xlate[0] == '\0')
			continue;

		if (dt_type_lookup(arg.dtargd_xlate, &xlt) == -1) {
			xyerror(D_UNKNOWN, "failed to resolve translated "
			    "argument type \"%s\" (arg#%d): %s",
			    arg.dtargd_xlate,
			    narg, dtrace_errmsg(dtp, dtrace_errno(dtp)));
		}

		/*
		 * If the types are the same, we don't have to worry about
		 * a translator.
		 */
		if (ctf_type_cmp(ntv.dtt_ctfp, ntv.dtt_type,
		    xlt.dtt_ctfp, xlt.dtt_type) == 0)
			continue;

		bzero(&sn, sizeof (sn));
		dt_node_type_assign(&sn, ntv.dtt_ctfp, ntv.dtt_type);

		bzero(&dn, sizeof (dn));
		dt_node_type_assign(&dn, xlt.dtt_ctfp, xlt.dtt_type);

		dxp = dt_xlator_lookup(dtp, &sn, &dn, DT_XLATE_FUZZY);

		if (dxp == NULL) {
			xyerror(D_UNKNOWN, "cannot translate arg#%d "
			    "from \"%s\" to \"%s\"", argp->da_ndx,
			    arg.dtargd_native, arg.dtargd_xlate);
		}

		argp->da_xlator =
		    dt_xlator_ident(dxp, xlt.dtt_ctfp, xlt.dtt_type);
	}
}

/*
 * Reset context-dependent variables and state at the end of cooking a D probe
 * definition clause.  This ensures that external declarations between clauses
 * do not reference any stale context-dependent data from the previous clause.
 */
void
dt_endcontext(dtrace_hdl_t *dtp)
{
	static const char *const cvars[] = {
		"probeprov", "probemod", "probefunc", "probename", "args", NULL
	};

	dt_ident_t *idp;
	int i;

	for (i = 0; cvars[i] != NULL; i++) {
		if ((idp = dt_idhash_lookup(dtp->dt_globals, cvars[i])) != NULL)
			idp->di_attr = _dtrace_defattr;
	}

	yypcb->pcb_pdesc = NULL;
	yypcb->pcb_nprobes = 0;

	while (yypcb->pcb_pargs != NULL) {
		dt_arg_t *old = yypcb->pcb_pargs;

		yypcb->pcb_pargs = old->da_next;
		free(old);
	}
}

static void
dt_reduce_ident(dt_idhash_t *dhp, dt_ident_t *idp, dtrace_hdl_t *dtp)
{
	if (idp->di_vers != 0 && idp->di_vers > dtp->dt_vmax)
		dt_idhash_delete(dhp, idp);
}

/*
 * When dtrace_setopt() is called for "version", it calls dt_reduce() to remove
 * any identifiers or translators that have been previously defined as bound to
 * a version greater than the specified version.  Therefore, in our current
 * version implementation, establishing a binding is a one-way transformation.
 * In addition, no versioning is currently provided for types as our .d library
 * files do not define any types and we reserve prefixes DTRACE_ and dtrace_
 * for our exclusive use.  If required, type versioning will require more work.
 */
int
dt_reduce(dtrace_hdl_t *dtp, dt_version_t v)
{
	char s[DT_VERSION_STRMAX];
	dt_xlator_t *dxp, *nxp;

	if (v > dtp->dt_vmax)
		return (dt_set_errno(dtp, EDT_VERSREDUCED));
	else if (v == dtp->dt_vmax)
		return (0); /* no reduction necessary */

	dt_dprintf("reducing api version to %s\n",
	    dt_version_num2str(v, s, sizeof (s)));

	dtp->dt_vmax = v;

	for (dxp = dt_list_next(&dtp->dt_xlators); dxp != NULL; dxp = nxp) {
		nxp = dt_list_next(dxp);
		if ((dxp->dx_souid.di_vers != 0 && dxp->dx_souid.di_vers > v) ||
		    (dxp->dx_ptrid.di_vers != 0 && dxp->dx_ptrid.di_vers > v))
			dt_list_delete(&dtp->dt_xlators, dxp);
	}

	dt_idhash_iter(dtp->dt_macros, (dt_idhash_f *)dt_reduce_ident, dtp);
	dt_idhash_iter(dtp->dt_aggs, (dt_idhash_f *)dt_reduce_ident, dtp);
	dt_idhash_iter(dtp->dt_globals, (dt_idhash_f *)dt_reduce_ident, dtp);
	dt_idhash_iter(dtp->dt_tls, (dt_idhash_f *)dt_reduce_ident, dtp);

	return (0);
}

/*
 * Fork and exec the cpp(1) preprocessor to run over the specified input file,
 * and return a FILE handle for the cpp output.  We use the /dev/fd filesystem
 * here to simplify the code by leveraging file descriptor inheritance.
 */
static FILE *
dt_preproc(dtrace_hdl_t *dtp, FILE *ifp)
{
	int argc = dtp->dt_cpp_argc;
	char **argv = malloc(sizeof (char *) * (argc + 5));
	FILE *ofp = tmpfile();

	char ipath[20], opath[20]; /* big enough for /dev/fd/ + INT_MAX + \0 */
	char verdef[32]; /* big enough for -D__SUNW_D_VERSION=0x%08x + \0 */

	struct sigaction act, oact;
	sigset_t mask, omask;

	int wstat, estat;
	pid_t pid;
	off64_t off;
	int c;

	if (argv == NULL || ofp == NULL) {
		(void) dt_set_errno(dtp, errno);
		goto err;
	}

	/*
	 * If the input is a seekable file, see if it is an interpreter file.
	 * If we see #!, seek past the first line because cpp will choke on it.
	 * We start cpp just prior to the \n at the end of this line so that
	 * it still sees the newline, ensuring that #line values are correct.
	 */
	if (isatty(fileno(ifp)) == 0 && (off = ftello64(ifp)) != -1) {
		if ((c = fgetc(ifp)) == '#' && (c = fgetc(ifp)) == '!') {
			for (off += 2; c != '\n'; off++) {
				if ((c = fgetc(ifp)) == EOF)
					break;
			}
			if (c == '\n')
				off--; /* start cpp just prior to \n */
		}
		(void) fflush(ifp);
		(void) fseeko64(ifp, off, SEEK_SET);
	}

	(void) snprintf(ipath, sizeof (ipath), "/dev/fd/%d", fileno(ifp));
	(void) snprintf(opath, sizeof (opath), "/dev/fd/%d", fileno(ofp));

	bcopy(dtp->dt_cpp_argv, argv, sizeof (char *) * argc);

	(void) snprintf(verdef, sizeof (verdef),
	    "-D__SUNW_D_VERSION=0x%08x", dtp->dt_vmax);
	argv[argc++] = verdef;

	switch (dtp->dt_stdcmode) {
	case DT_STDC_XA:
	case DT_STDC_XT:
		argv[argc++] = "-D__STDC__=0";
		break;
	case DT_STDC_XC:
		argv[argc++] = "-D__STDC__=1";
		break;
	}

	argv[argc++] = ipath;
	argv[argc++] = opath;
	argv[argc] = NULL;

	/*
	 * libdtrace must be able to be embedded in other programs that may
	 * include application-specific signal handlers.  Therefore, if we
	 * need to fork to run cpp(1), we must avoid generating a SIGCHLD
	 * that could confuse the containing application.  To do this,
	 * we block SIGCHLD and reset its disposition to SIG_DFL.
	 * We restore our signal state once we are done.
	 */
	(void) sigemptyset(&mask);
	(void) sigaddset(&mask, SIGCHLD);
	(void) sigprocmask(SIG_BLOCK, &mask, &omask);

	bzero(&act, sizeof (act));
	act.sa_handler = SIG_DFL;
	(void) sigaction(SIGCHLD, &act, &oact);

	if ((pid = fork1()) == -1) {
		(void) sigaction(SIGCHLD, &oact, NULL);
		(void) sigprocmask(SIG_SETMASK, &omask, NULL);
		(void) dt_set_errno(dtp, EDT_CPPFORK);
		goto err;
	}

	if (pid == 0) {
		(void) execvp(dtp->dt_cpp_path, argv);
		_exit(errno == ENOENT ? 127 : 126);
	}

	do {
		dt_dprintf("waiting for %s (PID %d)\n", dtp->dt_cpp_path,
		    (int)pid);
	} while (waitpid(pid, &wstat, 0) == -1 && errno == EINTR);

	(void) sigaction(SIGCHLD, &oact, NULL);
	(void) sigprocmask(SIG_SETMASK, &omask, NULL);

	dt_dprintf("%s returned exit status 0x%x\n", dtp->dt_cpp_path, wstat);
	estat = WIFEXITED(wstat) ? WEXITSTATUS(wstat) : -1;

	if (estat != 0) {
		switch (estat) {
		case 126:
			(void) dt_set_errno(dtp, EDT_CPPEXEC);
			break;
		case 127:
			(void) dt_set_errno(dtp, EDT_CPPENT);
			break;
		default:
			(void) dt_set_errno(dtp, EDT_CPPERR);
		}
		goto err;
	}

	free(argv);
	(void) fflush(ofp);
	(void) fseek(ofp, 0, SEEK_SET);
	return (ofp);

err:
	free(argv);
	(void) fclose(ofp);
	return (NULL);
}

/*
 * Open all of the .d library files found in the specified directory and try to
 * compile each one in order to cache its inlines and translators, etc.  We
 * silently ignore any missing directories and other files found therein.
 * We only fail (and thereby fail dt_load_libs()) if we fail to compile a
 * library and the error is something other than #pragma D depends_on.
 * Dependency errors are silently ignored to permit a library directory to
 * contain libraries which may not be accessible depending on our privileges.
 *
 * Note that at present, no ordering is defined between library files found in
 * the same directory: if cross-library dependencies are eventually required,
 * we will need to extend the #pragma D depends_on directive with an additional
 * class for libraries, and this function will need to create a graph of the
 * various library pathnames and then perform a topological ordering using the
 * dependency information before we attempt to compile any of them.
 */
static int
dt_load_libs_dir(dtrace_hdl_t *dtp, const char *path)
{
	struct dirent *dp, *ep;
	const char *p;
	DIR *dirp;

	char fname[PATH_MAX];
	dtrace_prog_t *pgp;
	FILE *fp;

	if ((dirp = opendir(path)) == NULL) {
		dt_dprintf("skipping lib dir %s: %s\n", path, strerror(errno));
		return (0);
	}

	ep = alloca(sizeof (struct dirent) + PATH_MAX + 1);
	bzero(ep, sizeof (struct dirent) + PATH_MAX + 1);

	while (readdir_r(dirp, ep, &dp) == 0 && dp != NULL) {
		if ((p = strrchr(dp->d_name, '.')) == NULL || strcmp(p, ".d"))
			continue; /* skip any filename not ending in .d */

		(void) snprintf(fname, sizeof (fname),
		    "%s/%s", path, dp->d_name);

		if ((fp = fopen(fname, "r")) == NULL) {
			dt_dprintf("skipping library %s: %s\n",
			    fname, strerror(errno));
			continue;
		}

		dtp->dt_filetag = fname;
		pgp = dtrace_program_fcompile(dtp, fp, DTRACE_C_EMPTY, 0, NULL);
		(void) fclose(fp);
		dtp->dt_filetag = NULL;

		if (pgp == NULL && (dtp->dt_errno != EDT_COMPILER ||
		    dtp->dt_errtag != dt_errtag(D_PRAGMA_DEPEND))) {
			(void) closedir(dirp);
			return (-1); /* preserve dt_errno */
		}

		if (pgp == NULL) {
			dt_dprintf("skipping library: %s\n",
			    dtrace_errmsg(dtp, dtrace_errno(dtp)));
		} else
			dtrace_program_destroy(dtp, pgp);
	}

	(void) closedir(dirp);
	return (0);
}

/*
 * Load the contents of any appropriate DTrace .d library files.  These files
 * contain inlines and translators that will be cached by the compiler.  We
 * defer this activity until the first compile to permit libdtrace clients to
 * add their own library directories and so that we can properly report errors.
 */
static int
dt_load_libs(dtrace_hdl_t *dtp)
{
	dt_dirpath_t *dirp;

	if (dtp->dt_cflags & DTRACE_C_NOLIBS)
		return (0); /* libraries already processed */

	dtp->dt_cflags |= DTRACE_C_NOLIBS;

	for (dirp = dt_list_next(&dtp->dt_lib_path);
	    dirp != NULL; dirp = dt_list_next(dirp)) {
		if (dt_load_libs_dir(dtp, dirp->dir_path) != 0) {
			dtp->dt_cflags &= ~DTRACE_C_NOLIBS;
			return (-1); /* errno is set for us */
		}
	}

	return (0);
}

static void *
dt_compile(dtrace_hdl_t *dtp, int context, dtrace_probespec_t pspec,
    dtrace_probedesc_t *pdp, uint_t cflags, int argc,
    char *const argv[], FILE *fp, const char *s)
{
	dt_node_t *dnp;
	dtrace_difo_t *difo;
	dt_pcb_t pcb;
	int err;

	if ((fp == NULL && s == NULL) || (cflags & ~DTRACE_C_MASK) != 0) {
		(void) dt_set_errno(dtp, EINVAL);
		return (NULL);
	}

	if (dt_list_next(&dtp->dt_lib_path) != NULL && dt_load_libs(dtp) != 0)
		return (NULL); /* errno is set for us */

	(void) ctf_discard(dtp->dt_cdefs->dm_ctfp);
	(void) ctf_discard(dtp->dt_ddefs->dm_ctfp);

	dt_idhash_iter(dtp->dt_globals, dt_idreset, NULL);
	dt_idhash_iter(dtp->dt_tls, dt_idreset, NULL);

	if (fp && (cflags & DTRACE_C_CPP) && (fp = dt_preproc(dtp, fp)) == NULL)
		return (NULL); /* errno is set for us */

	bzero(&pcb, sizeof (dt_pcb_t));
	dt_scope_create(&pcb.pcb_dstack);
	dt_irlist_create(&pcb.pcb_ir);

	pcb.pcb_hdl = dtp;
	pcb.pcb_fileptr = fp;
	pcb.pcb_string = s;
	pcb.pcb_strptr = s;
	pcb.pcb_strlen = s ? strlen(s) : 0;
	pcb.pcb_sargc = argc;
	pcb.pcb_sargv = argv;
	pcb.pcb_sflagv = argc ? calloc(argc, sizeof (ushort_t)) : NULL;
	pcb.pcb_pspec = pspec;
	pcb.pcb_cflags = dtp->dt_cflags | cflags;
	pcb.pcb_amin = dtp->dt_amin;
	pcb.pcb_yystate = -1;
	pcb.pcb_context = context;
	pcb.pcb_token = context;

	dtp->dt_pcb = &pcb;
	dtp->dt_gen++;

	yyinit(&pcb);

	if (context == DT_CTX_DPROG)
		yybegin(YYS_CLAUSE);
	else
		yybegin(YYS_EXPR);

	if ((err = setjmp(yypcb->pcb_jmpbuf)) != 0)
		goto out;

	if (yypcb->pcb_sargc != 0 && yypcb->pcb_sflagv == NULL)
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

	yypcb->pcb_idents = dt_idhash_create("ambiguous", NULL, 0, 0);
	yypcb->pcb_locals = dt_idhash_create("clause local", NULL,
	    DIF_VAR_OTHER_UBASE, DIF_VAR_OTHER_MAX);

	if (yypcb->pcb_idents == NULL || yypcb->pcb_locals == NULL)
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);

	if (pdp != NULL)
		dt_setcontext(dtp, pdp);

	/*
	 * Invoke the parser to evaluate the D source code.  If any errors
	 * occur during parsing, an error function will be called and we
	 * will longjmp back to pcb_jmpbuf to abort.  If parsing succeeds,
	 * we optionally display the parse tree if debugging is enabled.
	 */
	if (yyparse() != 0 || yypcb->pcb_root == NULL)
		xyerror(D_EMPTY, "empty D program translation unit\n");

	if (DT_TREEDUMP_PASS(dtp, 1))
		dt_node_printr(yypcb->pcb_root, stderr, 0);

	if (yypcb->pcb_pragmas != NULL)
		dt_idhash_iter(yypcb->pcb_pragmas, dt_idpragma, NULL);

	if (argc > 1 && !(yypcb->pcb_cflags & DTRACE_C_ARGREF) &&
	    !(yypcb->pcb_sflagv[argc - 1] & DT_IDFLG_REF)) {
		xyerror(D_MACRO_UNUSED, "extraneous argument '%s' ($%d is "
		    "not referenced)\n", yypcb->pcb_sargv[argc - 1], argc - 1);
	}

	/*
	 * If we have successfully created a parse tree for a D program, loop
	 * over the clauses and actions and instantiate the corresponding
	 * libdtrace program.  If we are parsing a D expression, then we
	 * simply run the code generator and assembler on the resulting tree.
	 */
	if (context == DT_CTX_DPROG) {
		assert(yypcb->pcb_root->dn_kind == DT_NODE_PROG);

		if ((dnp = yypcb->pcb_root->dn_list) == NULL &&
		    !(yypcb->pcb_cflags & DTRACE_C_EMPTY))
			xyerror(D_EMPTY, "empty D program translation unit\n");

		if ((yypcb->pcb_prog = dtrace_program_create(dtp)) == NULL)
			longjmp(yypcb->pcb_jmpbuf, dtrace_errno(dtp));

		for (; dnp != NULL; dnp = dnp->dn_list) {
			if (dnp->dn_kind == DT_NODE_CLAUSE)
				dt_compile_clause(dtp, dnp);
		}

	} else {
		(void) dt_node_cook(yypcb->pcb_root, DT_IDFLG_REF);
		dt_cg(yypcb, yypcb->pcb_root);
		difo = dt_as(yypcb);
	}

out:
	if (DT_TREEDUMP_PASS(dtp, 3))
		dt_node_printr(yypcb->pcb_root, stderr, 0);

	if (dtp->dt_cdefs_fd != -1 && (ftruncate64(dtp->dt_cdefs_fd, 0) == -1 ||
	    lseek64(dtp->dt_cdefs_fd, 0, SEEK_SET) == -1 ||
	    ctf_write(dtp->dt_cdefs->dm_ctfp, dtp->dt_cdefs_fd) == CTF_ERR))
		dt_dprintf("failed to update CTF cache: %s\n", strerror(errno));

	if (dtp->dt_ddefs_fd != -1 && (ftruncate64(dtp->dt_ddefs_fd, 0) == -1 ||
	    lseek64(dtp->dt_ddefs_fd, 0, SEEK_SET) == -1 ||
	    ctf_write(dtp->dt_ddefs->dm_ctfp, dtp->dt_ddefs_fd) == CTF_ERR))
		dt_dprintf("failed to update CTF cache: %s\n", strerror(errno));

	while (yypcb->pcb_dstack.ds_next != NULL)
		(void) dt_scope_pop();

	dt_scope_destroy(&yypcb->pcb_dstack);
	dt_irlist_destroy(&yypcb->pcb_ir);

	dt_node_link_free(&yypcb->pcb_list);
	dt_node_link_free(&yypcb->pcb_hold);

	if (yypcb->pcb_pragmas != NULL)
		dt_idhash_destroy(yypcb->pcb_pragmas);
	if (yypcb->pcb_locals != NULL)
		dt_idhash_destroy(yypcb->pcb_locals);
	if (yypcb->pcb_idents != NULL)
		dt_idhash_destroy(yypcb->pcb_idents);
	if (yypcb->pcb_inttab != NULL)
		dt_inttab_destroy(yypcb->pcb_inttab);
	if (yypcb->pcb_strtab != NULL)
		dt_strtab_destroy(yypcb->pcb_strtab);
	if (yypcb->pcb_regs != NULL)
		dt_regset_destroy(yypcb->pcb_regs);
	if (yypcb->pcb_difo != NULL)
		dtrace_difo_release(yypcb->pcb_difo);

	while (yypcb->pcb_pargs != NULL) {
		dt_arg_t *old = yypcb->pcb_pargs;

		yypcb->pcb_pargs = old->da_next;
		free(old);
	}

	free(yypcb->pcb_filetag);
	free(yypcb->pcb_sflagv);

	if (yypcb->pcb_fileptr && (cflags & DTRACE_C_CPP))
		(void) fclose(yypcb->pcb_fileptr); /* close dt_preproc() file */

	if (err != 0) {
		dt_xlator_t *dxp, *nxp;
		dt_provider_t *pvp, *nvp;

		if (yypcb->pcb_pred != NULL)
			dtrace_difo_release(yypcb->pcb_pred);
		if (yypcb->pcb_prog != NULL)
			dtrace_program_destroy(dtp, yypcb->pcb_prog);
		if (yypcb->pcb_stmt != NULL)
			dtrace_stmt_destroy(yypcb->pcb_stmt);
		if (yypcb->pcb_ecbdesc != NULL)
			dtrace_ecbdesc_release(yypcb->pcb_ecbdesc);

		for (dxp = dt_list_next(&dtp->dt_xlators); dxp; dxp = nxp) {
			nxp = dt_list_next(dxp);
			if (dxp->dx_gen == dtp->dt_gen)
				dt_xlator_destroy(dtp, dxp);
		}

		for (pvp = dt_list_next(&dtp->dt_provlist); pvp; pvp = nvp) {
			nvp = dt_list_next(pvp);
			if (pvp->pv_gen == dtp->dt_gen)
				dt_provider_destroy(dtp, pvp);
		}

		dt_idhash_iter(dtp->dt_aggs, dt_idkill, (void *)dtp->dt_gen);
		dt_idhash_update(dtp->dt_aggs);

		dt_idhash_iter(dtp->dt_globals, dt_idkill, (void *)dtp->dt_gen);
		dt_idhash_update(dtp->dt_globals);

		dt_idhash_iter(dtp->dt_tls, dt_idkill, (void *)dtp->dt_gen);
		dt_idhash_update(dtp->dt_tls);

		(void) ctf_discard(dtp->dt_cdefs->dm_ctfp);
		(void) ctf_discard(dtp->dt_ddefs->dm_ctfp);

		(void) dt_set_errno(dtp, err);
		dtp->dt_pcb = NULL;
		yypcb = NULL;

		return (NULL);
	}

	(void) dt_set_errno(dtp, 0);
	dtp->dt_pcb = NULL;
	yypcb = NULL;

	if (context == DT_CTX_DPROG)
		return (pcb.pcb_prog);
	else
		return (difo);
}

dtrace_prog_t *
dtrace_program_strcompile(dtrace_hdl_t *dtp, const char *s,
    dtrace_probespec_t spec, uint_t cflags, int argc, char *const argv[])
{
	return (dt_compile(dtp, DT_CTX_DPROG,
	    spec, NULL, cflags, argc, argv, NULL, s));
}

dtrace_prog_t *
dtrace_program_fcompile(dtrace_hdl_t *dtp, FILE *fp,
    uint_t cflags, int argc, char *const argv[])
{
	return (dt_compile(dtp, DT_CTX_DPROG,
	    DTRACE_PROBESPEC_NAME, NULL, cflags, argc, argv, fp, NULL));
}

dtrace_difo_t *
dtrace_difo_create(dtrace_hdl_t *dtp, const char *s, dtrace_probedesc_t *pdp)
{
	return (dt_compile(dtp, DT_CTX_DEXPR,
	    DTRACE_PROBESPEC_NONE, pdp, 0, 0, NULL, NULL, s));
}

void
dtrace_difo_hold(dtrace_difo_t *dp)
{
	dp->dtdo_refcnt++;
	assert(dp->dtdo_refcnt != 0);
}

void
dtrace_difo_release(dtrace_difo_t *dp)
{
	assert(dp->dtdo_refcnt != 0);

	if (--dp->dtdo_refcnt != 0)
		return;

	free(dp->dtdo_buf);
	free(dp->dtdo_inttab);
	free(dp->dtdo_strtab);
	free(dp->dtdo_vartab);
	free(dp->dtdo_kreltab);
	free(dp->dtdo_ureltab);

	free(dp);
}

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
 
# if !defined(RTLD_DB_H)
# define	RTLD_DB_H

struct ps_prochandle;

# include	<linux_types.h>
# include	<link.h>

/*
 * librtld_db interface versions
 */
#define RD_VERSION1     1
#define RD_VERSION2     2
#define RD_VERSION3     3
#define RD_VERSION4     4
#define RD_VERSION      RD_VERSION4

typedef enum {
        RD_ERR,         /* generic */
        RD_OK,          /* generic "call" succeeded */
        RD_NOCAPAB,     /* capability not available */
        RD_DBERR,       /* import service failed */
        RD_NOBASE,      /* 5.x: aux tag AT_BASE not found */
        RD_NODYNAM,     /* symbol 'DYNAMIC' not found */
        RD_NOMAPS       /* link-maps are not yet available */
} rd_err_e;

/*
 * ways that the event notification can take place:
 */
typedef enum {
        RD_NOTIFY_BPT,          /* set break-point at address */
        RD_NOTIFY_AUTOBPT,      /* 4.x compat. not used in 5.x */
        RD_NOTIFY_SYSCALL       /* watch for syscall */
} rd_notify_e;

/*
 * information on ways that the event notification can take place:
 */
typedef struct rd_notify {
        rd_notify_e     type;
        union {
                psaddr_t        bptaddr;        /* break point address */
                long            syscallno;      /* system call id */
        } u;
} rd_notify_t;

/*
 * information about event instance:
 */
typedef enum {
        RD_NOSTATE = 0,         /* no state information */
        RD_CONSISTENT,          /* link-maps are stable */
        RD_ADD,                 /* currently adding object to link-maps */
        RD_DELETE               /* currently deleteing object from link-maps */
} rd_state_e;

typedef struct rd_event_msg {
        rd_event_e      type;
        union {
                rd_state_e      state;  /* for DLACTIVITY */
        } u;
} rd_event_msg_t;

/*
 * iteration over load objects
 */
typedef struct rd_loadobj {
        psaddr_t        rl_nameaddr;    /* address of the name in user space */
        unsigned        rl_flags;
        psaddr_t        rl_base;        /* base of address of code */
        psaddr_t        rl_data_base;   /* base of address of data */
        Lmid_t          rl_lmident;     /* ident of link map */
        psaddr_t        rl_refnameaddr; /* reference name of filter in user */
                                        /* space.  If non null object is a */
                                        /* filter. */
        psaddr_t        rl_plt_base;    /* These fields are present for 4.x */
        unsigned        rl_plt_size;    /* compatibility and are not */
                                        /* currently used  in SunOS5.x */
        psaddr_t        rl_bend;        /* end of image (text+data+bss) */
        psaddr_t        rl_padstart;    /* start of padding */
        psaddr_t        rl_padend;      /* end of image after padding */
        psaddr_t        rl_dynamic;     /* points to the DYNAMIC section */
                                        /* in the target process */
        unsigned long   rl_tlsmodid;    /* module ID for TLS references */
} rd_loadobj_t;

/*
 * Commands for rd_ctl()
 */
#define RD_CTL_SET_HELPPATH     0x01    /* Set the path used to find helpers */

typedef int rl_iter_f(const rd_loadobj_t *, void *);

typedef struct rd_agent rd_agent_t;

/*
 * State kept for brand helper libraries
 *
 * All librtld_db brand plugin libraries need to specify a Lmid_t value
 * that controls how link map ids are assigned to native solaris objects
 * (as pointed to by the processes aux vectors) which are enumerated by
 * librtld_db.  In most cases this value will either be LM_ID_NONE or
 * LM_ID_BRAND.
 *
 * If LM_ID_NONE is specified in the structure below, then when native solaris
 * objects are enumerated by librtld_db, their link map id values will match
 * the link map ids assigned to those objects by the solaris linker within
 * the target process.
 *
 * If LM_ID_BRAND is specified in the structure below, then when native solaris
 * objects are enumerated by librtld_db, their link map id value will be
 * explicity set to LM_ID_BRAND, regardless of the link map ids assigned to
 * those objects by the solaris linker within the target process.
 *
 * In all cases the librtld_db brand plugin library can report any link
 * map id value that it wants for objects that it enumerates via it's
 * rho_loadobj_iter() entry point.
 */
typedef struct __rd_helper_data	*rd_helper_data_t;
typedef struct rd_helper_ops {
	Lmid_t			rho_lmid;
	rd_helper_data_t	(*rho_init)(rd_agent_t *,
				    struct ps_prochandle *);
	void			(*rho_fini)(rd_helper_data_t);
	int			(*rho_loadobj_iter)(rd_helper_data_t,
				    rl_iter_f *, void *);
	rd_err_e		(*rho_get_dyns)(rd_helper_data_t,
				    psaddr_t, void **, size_t *);
} rd_helper_ops_t;

typedef struct rd_helper {
	void			*rh_dlhandle;
	rd_helper_ops_t		*rh_ops;
	rd_helper_data_t	rh_data;
} rd_helper_t;

struct rd_agent {
	mutex_t				rd_mutex;
	struct ps_prochandle		*rd_psp;	/* prochandle pointer */
	psaddr_t			rd_rdebug;	/* rtld r_debug */
	psaddr_t			rd_preinit;	/* rtld_db_preinit */
	psaddr_t			rd_postinit;	/* rtld_db_postinit */
	psaddr_t			rd_dlact;	/* rtld_db_dlact */
	psaddr_t			rd_tbinder;	/* tail of binder */
	psaddr_t			rd_rtlddbpriv;	/* rtld rtld_db_priv */
	ulong_t				rd_flags;	/* flags */
	ulong_t				rd_rdebugvers;	/* rtld_db_priv.vers */
	int				rd_dmodel;	/* data model */
	rd_helper_t			rd_helper;	/* private to helper */
#if defined(linux)
	int				rd_pid;
	uintptr_t			rda_addr;
#endif
};

# endif

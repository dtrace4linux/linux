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

#pragma ident	"%Z%%M%	%I%	%E% SMI"


#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <rtld_db.h>
#include "_rtld_db.h"
#include "rtld_msg.h"

struct ps_prochandle;

#include "Pcontrol.h"
#include "Putil.h"

# define ps_plog printf
# define MSG_ORIG(x) x

/*
 * Mutex to protect global data
 */
mutex_t	glob_mutex = DEFAULTMUTEX;
int	rtld_db_version = RD_VERSION1;
int	rtld_db_logging = 0;
char	rtld_db_helper_path[MAXPATHLEN];

# define	_rd_reset64 _rd_reset32
rd_err_e
_rd_reset32(struct rd_agent *rap)
{
	struct ps_prochandle	*php = rap->rd_psp;
	rd_err_e		rc = RD_OK;
	const auxv_t		*auxvp = NULL;

	LOG(ps_plog(MSG_ORIG(MSG_DB_RDRESET), rap->rd_dmodel));

	if (ps_pauxv(php, &auxvp) != PS_OK) {
		LOG(ps_plog(MSG_ORIG(MSG_DB_NOAUXV)));
		rc = RD_ERR;
	}
	return rc;
}

/*
 * Versioning Notes.
 *
 * The following have been added as the versions of librtld_db
 * have grown:
 *
 *	RD_VERSION1:
 *		o baseline version
 *
 *	RD_VERSION2:
 *		o added support for the use of the AT_SUN_LDBASE auxvector
 *		  to find the initialial debugging (r_debug) structures
 *		  in ld.so.1
 *		o added the rl_dynamic field to rd_loadobj_t
 *		o added the RD_FLG_MEM_OBJECT to be used with the
 *		  rl_dynamic->rl_flags field.
 *
 *	RD_VERSION3:
 *		o added the following fields/flags to the rd_plt_info_t
 *		  type:
 *			pi_baddr	- bound address of PLT (if bound)
 *			pi_flags	- flag field
 *			RD_FLG_PI_PLTBOUND	(flag for pi_flags)
 *				if set - the PLT is bound and pi_baddr
 *				is filled in with the destination of the PLT.
 *
 *	RD_VERSION4:
 *		o added the following field to the rd_loadobj_t structure:
 *			rl_tlsmodid	- module ID for TLS references
 */
rd_err_e
rd_init(int version)
{
	if ((version < RD_VERSION1) ||
	    (version > RD_VERSION))
		return (RD_NOCAPAB);
	rtld_db_version = version;
	LOG(ps_plog(MSG_ORIG(MSG_DB_RDINIT), rtld_db_version));

	return (RD_OK);
}
rd_err_e
rd_ctl(int cmd, void *arg)
{
	if (cmd != RD_CTL_SET_HELPPATH || arg == NULL ||
	    strlen((char *)arg) >= MAXPATHLEN)
		return (RD_ERR);

	(void) strcpy(rtld_db_helper_path, (char *)arg);

	return (RD_OK);
}
void
rd_delete(rd_agent_t *rap)
{
/*	printf("rd_delete:%s\n", __func__);*/
	free(rap);
}

rd_err_e
rd_reset(struct rd_agent *rap)
{
	rd_err_e			err;

	RDAGLOCK(rap);

	rap->rd_flags = 0;

#ifdef _LP64
	/*
	 * Determine if client is 32-bit or 64-bit.
	 */
	if (ps_pdmodel(rap->rd_psp, &rap->rd_dmodel) != PS_OK) {
		LOG(ps_plog(MSG_ORIG(MSG_DB_DMLOOKFAIL)));
		RDAGUNLOCK(rap);
		return (RD_DBERR);
	}

	if (rap->rd_dmodel == PR_MODEL_LP64)
		err = _rd_reset64(rap);
	else
#endif
		err = _rd_reset32(rap);

	RDAGUNLOCK(rap);
	return (err);
}

rd_agent_t *
rd_new(struct ps_prochandle *php, int pid) 
{	rd_agent_t *rap;

	rap = (rd_agent_t *) calloc(sizeof *rap, 1);

	if (rap == NULL)
		return NULL;

	rap->rd_pid = pid;
	rap->rd_psp = php;
/*printf("rd_new:%s\n", __func__); */
	(void) mutex_init(&rap->rd_mutex, USYNC_THREAD, 0);
        if (rd_reset(rap) != RD_OK) {
		if (rap->rd_helper.rh_dlhandle != NULL) {
                	rap->rd_helper.rh_ops->rho_fini(rap->rd_helper.rh_data);
			(void) dlclose(rap->rd_helper.rh_dlhandle);
                }
		free(rap);
                LOG(ps_plog(MSG_ORIG(MSG_DB_RESETFAIL)));
                return ((rd_agent_t *)0);
        }

	return rap;
}
static rd_err_e
iter_map(rd_agent_t *rap, unsigned long ident, psaddr_t lmaddr,
        rl_iter_f *cb, void *client_data, uint_t *abort_iterp)
{
//        while (lmaddr) {
//}
}
rd_err_e
rd_loadobj_iter(rd_agent_t *rap, rl_iter_f *cb, void *client_data)
{	char	buf[256];
	FILE	*fp;
	int	ret;

	printf("%s\n", __func__);
	snprintf(buf, sizeof buf, "/proc/%d/maps", rap->rd_pid);
	if ((fp = fopen(buf, "r")) == NULL) {
		return RD_ERR;
	}
	while (fgets(buf, sizeof buf, fp) != NULL) {
		char	*addr_str;
		void	*addr;
		char	*perms;
		char	*lib;
		uint_t	abort_iter;
		rd_loadobj_t lobj;

		if (strcmp(buf + strlen(buf) - 4, ".so\n") != 0)
			continue;
		buf[strlen(buf)-1] = '\0';
		lib = strrchr(buf, ' ');
		if (lib == NULL)
			continue;
		lib++;
		addr_str = strtok(buf, " ");
		perms = strtok(NULL, " ");
		if (strchr(perms, 'x') == NULL)
			continue;

		addr = strtol(addr_str, NULL, 16);

printf("rd_loadobj_iter: %s %p\n", lib, addr);
		lobj.rl_base = addr;
		lobj.rl_nameaddr = lib;
		ret = cb(&lobj, client_data);
	}
	fclose(fp);
	return RD_OK;
}
rd_err_e
rd_get_dyns(rd_agent_t *rap, psaddr_t addr, void **dynpp, size_t *dynpp_sz)
{
	*dynpp  = NULL;
	printf("%s\n", __func__);
	return RD_ERR;
}
void
rd_log(const int on_off)
{
	(void) mutex_lock(&glob_mutex);
	rtld_db_logging = on_off;
	(void) mutex_unlock(&glob_mutex);
}
char *
rd_errstr(int rderr)
{
	switch (rderr) {
	  case RD_OK: return "rd_errstr: RD_OK\n";
	  case RD_ERR: return "rd_errstr: RD_ERR\n";
	  case RD_DBERR: return "rd_errstr: RD_DBERR\n";
          case RD_NOCAPAB: return "rd_errstr: RD_NOCAPAB\n";
          case RD_NODYNAM: return "rd_errstr: RD_NODYNAM\n";
          case RD_NOBASE: return "rd_errstr: RD_NOBASE\n";
          case RD_NOMAPS: return "rd_errstr: RD_NOMAPS\n";
	}
	printf("proc-stub:%s err=%d\n", __func__, rderr);
	return "rd_errstr:dont know what error\n";
}
rd_err_e
rd_event_addr(rd_agent_t *rdap, rd_event_e event, rd_notify_t *notify)
{
	printf("proc-stub:%s addr=%p\n", __func__, (void *) rdap->rda_addr);
	notify->type = RD_NOTIFY_BPT;
        notify->u.bptaddr = rdap->rda_addr;

	return RD_OK;
}
int
rd_event_enable(rd_agent_t *rdap, int onoff)
{
	printf("proc-stub:%s\n", __func__);
	return RD_OK;
}
rd_err_e
rd_event_getmsg(rd_agent_t *rdap, rd_event_msg_t *msg)
{
	printf("proc-stub:%s\n", __func__);
	msg->type = RD_POSTINIT;
        msg->u.state = RD_CONSISTENT;

        return RD_OK;
}


/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include "dtrace_proto.h"
#include <linux/cpumask.h>

#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>

# define PRINT_CASE(x) printk("%s(%d):%s: %s\n", __FILE__, __LINE__, __func__, #x)

extern dtrace_provider_t *dtrace_provider;	/* provider list */
static void		*dtrace_softstate;	/* softstate pointer */
extern dtrace_probe_t	**dtrace_probes;	/* array of all probes */
extern int		dtrace_nprobes;		/* number of probes */
extern kmutex_t		dtrace_lock;		/* probe state lock */
extern kmutex_t		dtrace_provider_lock;	/* provider state lock */
extern int		dtrace_error;		/* temporary variable */
extern dtrace_state_t	*dtrace_state;		/* temporary variable */

/**********************************************************************/
/*   Code to support the ioctl interface for /dev/dtrace.	      */
/**********************************************************************/
int
dtrace_ioctl(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv)
{
//	minor_t minor = getminor(dev);
	dtrace_state_t *state = NULL;
	int rval;

printk("fp=%p cmd=%x\n", fp, cmd);
# if defined(sun)
	if (minor == DTRACEMNRN_HELPER)
		return (dtrace_ioctl_helper(cmd, arg, rv));

	state = ddi_get_soft_state(dtrace_softstate, minor);

	if (state->dts_anon) {
		ASSERT(dtrace_anon.dta_state == NULL);
		state = state->dts_anon;
	}
# else
TODO();
	state = fp->private_data;
printk("state=%p\n", state);
TODO();
	if (state->dts_anon) {
		ASSERT(dtrace_anon.dta_state == NULL);
		state = state->dts_anon;
	}
# endif


	switch (cmd) {
	case DTRACEIOC_CONF: {
		dtrace_conf_t conf;

PRINT_CASE(DTRACEIOC_CONF);
		bzero(&conf, sizeof (conf));
		conf.dtc_difversion = DIF_VERSION;
		conf.dtc_difintregs = DIF_DIR_NREGS;
		conf.dtc_diftupregs = DIF_DTR_NREGS;
		conf.dtc_ctfmodel = CTF_MODEL_NATIVE;

		if (copyout(&conf, (void *)arg, sizeof (conf)) != 0)
			return (EFAULT);

		return (0);
	}

	case DTRACEIOC_PROBEMATCH:
	case DTRACEIOC_PROBES: {
		dtrace_probe_t *probe = NULL;
		dtrace_probedesc_t desc;
		dtrace_probekey_t pkey;
		dtrace_id_t i;
		int m = 0;
		uint32_t priv;
		uid_t uid;

PRINT_CASE(DTRACEIOC_PROBES);
		if (copyin((void *)arg, &desc, sizeof (desc)) != 0)
			return (EFAULT);

		desc.dtpd_provider[DTRACE_PROVNAMELEN - 1] = '\0';
		desc.dtpd_mod[DTRACE_MODNAMELEN - 1] = '\0';
		desc.dtpd_func[DTRACE_FUNCNAMELEN - 1] = '\0';
		desc.dtpd_name[DTRACE_NAMELEN - 1] = '\0';

		/*
		 * Before we attempt to match this probe, we want to give
		 * providers the opportunity to provide it.
		 */
		if (desc.dtpd_id == DTRACE_IDNONE) {
			mutex_enter(&dtrace_provider_lock);
			dtrace_probe_provide(&desc, NULL);
			mutex_exit(&dtrace_provider_lock);
			desc.dtpd_id++;
		}

		if (cmd == DTRACEIOC_PROBEMATCH)  {
			dtrace_probekey(&desc, &pkey);
			pkey.dtpk_id = DTRACE_IDNONE;
		}

# if linux
                uid = get_current()->uid;
# else
                uid = crgetuid(cr);
# endif
		dtrace_cred2priv(cr, &priv, &uid);

		mutex_enter(&dtrace_lock);

		if (cmd == DTRACEIOC_PROBEMATCH) {
			for (i = desc.dtpd_id; i <= dtrace_nprobes; i++) {
				if ((probe = dtrace_probes[i - 1]) != NULL &&
				    (m = dtrace_match_probe(probe, &pkey,
				    priv, uid)) != 0)
					break;
			}

			if (m < 0) {
				mutex_exit(&dtrace_lock);
				return (EINVAL);
			}

		} else {
printk("nprobes=%d\n", dtrace_nprobes);
			for (i = desc.dtpd_id; i <= dtrace_nprobes; i++) {
				if ((probe = dtrace_probes[i - 1]) != NULL &&
				    dtrace_match_priv(probe, priv, uid))
					break;
			}
		}

		if (probe == NULL) {
			mutex_exit(&dtrace_lock);
			return (ESRCH);
		}

		dtrace_probe_description(probe, &desc);
		mutex_exit(&dtrace_lock);

		if (copyout(&desc, (void *)arg, sizeof (desc)) != 0)
			return (EFAULT);

		return (0);
	}

	case DTRACEIOC_PROVIDER: {
		dtrace_providerdesc_t pvd;
		dtrace_provider_t *pvp;

PRINT_CASE(DTRACEIOC_PROVIDER);
		if (copyin((void *)arg, &pvd, sizeof (pvd)) != 0)
			return (EFAULT);

		pvd.dtvd_name[DTRACE_PROVNAMELEN - 1] = '\0';
		mutex_enter(&dtrace_provider_lock);

		for (pvp = dtrace_provider; pvp != NULL; pvp = pvp->dtpv_next) {
			if (strcmp(pvp->dtpv_name, pvd.dtvd_name) == 0)
				break;
		}

		mutex_exit(&dtrace_provider_lock);

		if (pvp == NULL)
			return (ESRCH);

		bcopy(&pvp->dtpv_priv, &pvd.dtvd_priv, sizeof (dtrace_ppriv_t));
		bcopy(&pvp->dtpv_attr, &pvd.dtvd_attr, sizeof (dtrace_pattr_t));
		if (copyout(&pvd, (void *)arg, sizeof (pvd)) != 0)
			return (EFAULT);

		return (0);
	}

# if 0
	case DTRACEIOC_EPROBE: {
		dtrace_eprobedesc_t epdesc;
		dtrace_ecb_t *ecb;
		dtrace_action_t *act;
		void *buf;
		size_t size;
		uintptr_t dest;
		int nrecs;

PRINT_CASE(DTRACEIOC_EPROBE);
		if (copyin((void *)arg, &epdesc, sizeof (epdesc)) != 0)
			return (EFAULT);

		mutex_enter(&dtrace_lock);

		if ((ecb = dtrace_epid2ecb(state, epdesc.dtepd_epid)) == NULL) {
			mutex_exit(&dtrace_lock);
			return (EINVAL);
		}

		if (ecb->dte_probe == NULL) {
			mutex_exit(&dtrace_lock);
			return (EINVAL);
		}

		epdesc.dtepd_probeid = ecb->dte_probe->dtpr_id;
		epdesc.dtepd_uarg = ecb->dte_uarg;
		epdesc.dtepd_size = ecb->dte_size;

		nrecs = epdesc.dtepd_nrecs;
		epdesc.dtepd_nrecs = 0;
		for (act = ecb->dte_action; act != NULL; act = act->dta_next) {
			if (DTRACEACT_ISAGG(act->dta_kind) || act->dta_intuple)
				continue;

			epdesc.dtepd_nrecs++;
		}

		/*
		 * Now that we have the size, we need to allocate a temporary
		 * buffer in which to store the complete description.  We need
		 * the temporary buffer to be able to drop dtrace_lock()
		 * across the copyout(), below.
		 */
		size = sizeof (dtrace_eprobedesc_t) +
		    (epdesc.dtepd_nrecs * sizeof (dtrace_recdesc_t));

		buf = kmem_alloc(size, KM_SLEEP);
		dest = (uintptr_t)buf;

		bcopy(&epdesc, (void *)dest, sizeof (epdesc));
		dest += offsetof(dtrace_eprobedesc_t, dtepd_rec[0]);

		for (act = ecb->dte_action; act != NULL; act = act->dta_next) {
			if (DTRACEACT_ISAGG(act->dta_kind) || act->dta_intuple)
				continue;

			if (nrecs-- == 0)
				break;

			bcopy(&act->dta_rec, (void *)dest,
			    sizeof (dtrace_recdesc_t));
			dest += sizeof (dtrace_recdesc_t);
		}

		mutex_exit(&dtrace_lock);

		if (copyout(buf, (void *)arg, dest - (uintptr_t)buf) != 0) {
			kmem_free(buf, size);
			return (EFAULT);
		}

		kmem_free(buf, size);
		return (0);
	}

	case DTRACEIOC_AGGDESC: {
		dtrace_aggdesc_t aggdesc;
		dtrace_action_t *act;
		dtrace_aggregation_t *agg;
		int nrecs;
		uint32_t offs;
		dtrace_recdesc_t *lrec;
		void *buf;
		size_t size;
		uintptr_t dest;

PRINT_CASE(DTRACEIOC_AGGDESC);
		if (copyin((void *)arg, &aggdesc, sizeof (aggdesc)) != 0)
			return (EFAULT);

		mutex_enter(&dtrace_lock);

		if ((agg = dtrace_aggid2agg(state, aggdesc.dtagd_id)) == NULL) {
			mutex_exit(&dtrace_lock);
			return (EINVAL);
		}

		aggdesc.dtagd_epid = agg->dtag_ecb->dte_epid;

		nrecs = aggdesc.dtagd_nrecs;
		aggdesc.dtagd_nrecs = 0;

		offs = agg->dtag_base;
		lrec = &agg->dtag_action.dta_rec;
		aggdesc.dtagd_size = lrec->dtrd_offset + lrec->dtrd_size - offs;

		for (act = agg->dtag_first; ; act = act->dta_next) {
			ASSERT(act->dta_intuple ||
			    DTRACEACT_ISAGG(act->dta_kind));

			/*
			 * If this action has a record size of zero, it
			 * denotes an argument to the aggregating action.
			 * Because the presence of this record doesn't (or
			 * shouldn't) affect the way the data is interpreted,
			 * we don't copy it out to save user-level the
			 * confusion of dealing with a zero-length record.
			 */
			if (act->dta_rec.dtrd_size == 0) {
				ASSERT(agg->dtag_hasarg);
				continue;
			}

			aggdesc.dtagd_nrecs++;

			if (act == &agg->dtag_action)
				break;
		}

		/*
		 * Now that we have the size, we need to allocate a temporary
		 * buffer in which to store the complete description.  We need
		 * the temporary buffer to be able to drop dtrace_lock()
		 * across the copyout(), below.
		 */
		size = sizeof (dtrace_aggdesc_t) +
		    (aggdesc.dtagd_nrecs * sizeof (dtrace_recdesc_t));

		buf = kmem_alloc(size, KM_SLEEP);
		dest = (uintptr_t)buf;

		bcopy(&aggdesc, (void *)dest, sizeof (aggdesc));
		dest += offsetof(dtrace_aggdesc_t, dtagd_rec[0]);

		for (act = agg->dtag_first; ; act = act->dta_next) {
			dtrace_recdesc_t rec = act->dta_rec;

			/*
			 * See the comment in the above loop for why we pass
			 * over zero-length records.
			 */
			if (rec.dtrd_size == 0) {
				ASSERT(agg->dtag_hasarg);
				continue;
			}

			if (nrecs-- == 0)
				break;

			rec.dtrd_offset -= offs;
			bcopy(&rec, (void *)dest, sizeof (rec));
			dest += sizeof (dtrace_recdesc_t);

			if (act == &agg->dtag_action)
				break;
		}

		mutex_exit(&dtrace_lock);

		if (copyout(buf, (void *)arg, dest - (uintptr_t)buf) != 0) {
			kmem_free(buf, size);
			return (EFAULT);
		}

		kmem_free(buf, size);
		return (0);
	}
# endif

	case DTRACEIOC_ENABLE: {
		dof_hdr_t *dof;
		dtrace_enabling_t *enab = NULL;
		dtrace_vstate_t *vstate;
		int i, err = 0;

PRINT_CASE(DTRACEIOC_ENABLE);
		if (rv == 0) {
			TODO();
			printk("Sorry - but rv is null and should not be!\n");
			return EFAULT;
		}
		*rv = 0;

                /*
                 * If a NULL argument has been passed, we take this as our
                 * cue to reevaluate our enablings.
                 */
                if (arg == NULL) {
TODO();
                        mutex_enter(&cpu_lock);
                        mutex_enter(&dtrace_lock);
                        err = dtrace_enabling_matchstate(state, rv);
TODO();
                        mutex_exit(&dtrace_lock);
                        mutex_exit(&cpu_lock);

                        return (err);
                }

TODO();
		if ((dof = dtrace_dof_copyin(arg, &rval)) == NULL)
			return (rval);

		mutex_enter(&cpu_lock);
		mutex_enter(&dtrace_lock);
TODO();
		if (state == NULL) {
			TODO();
			printk("Sorry - but state is null and it should not be!\n");
			return EFAULT;
		}

		vstate = &state->dts_vstate;

		if (state->dts_activity != DTRACE_ACTIVITY_INACTIVE) {
TODO();
			mutex_exit(&dtrace_lock);
			mutex_exit(&cpu_lock);
			dtrace_dof_destroy(dof);
			return (EBUSY);
		}
TODO();

		if (dtrace_dof_slurp(dof, vstate, cr, &enab, 0, B_TRUE) != 0) {
			mutex_exit(&dtrace_lock);
			mutex_exit(&cpu_lock);
			dtrace_dof_destroy(dof);
			return (EINVAL);
		}
TODO();

		if ((rval = dtrace_dof_options(dof, state)) != 0) {
TODO();
			dtrace_enabling_destroy(enab);
			mutex_exit(&dtrace_lock);
			mutex_exit(&cpu_lock);
			dtrace_dof_destroy(dof);
			return (rval);
		}
TODO();

                if ((err = dtrace_enabling_match(enab, rv)) == 0) {
TODO();
                        err = dtrace_enabling_retain(enab);
                } else {
                        dtrace_enabling_destroy(enab);
                }
TODO();
		mutex_exit(&cpu_lock);
		mutex_exit(&dtrace_lock);
TODO();
		dtrace_dof_destroy(dof);
TODO();
printk("err=%d rv=%d\n", err, *rv);
//		if (err == 0)
//			copy_to_user(arg , &rv, sizeof rv);

		return err;
	}


	case DTRACEIOC_PROBEARG: {
		dtrace_argdesc_t desc;
		dtrace_probe_t *probe;
		dtrace_provider_t *prov;

PRINT_CASE(DTRACEIOC_PROBEARG);
		if (copyin((void *)arg, &desc, sizeof (desc)) != 0)
			return (EFAULT);

		if (desc.dtargd_id == DTRACE_IDNONE)
			return (EINVAL);

		if (desc.dtargd_ndx == DTRACE_ARGNONE)
			return (EINVAL);

		mutex_enter(&dtrace_provider_lock);
		mutex_enter(&mod_lock);
		mutex_enter(&dtrace_lock);

		if (desc.dtargd_id > dtrace_nprobes) {
			mutex_exit(&dtrace_lock);
			mutex_exit(&mod_lock);
			mutex_exit(&dtrace_provider_lock);
			return (EINVAL);
		}

		if ((probe = dtrace_probes[desc.dtargd_id - 1]) == NULL) {
			mutex_exit(&dtrace_lock);
			mutex_exit(&mod_lock);
			mutex_exit(&dtrace_provider_lock);
			return (EINVAL);
		}

		mutex_exit(&dtrace_lock);

		prov = probe->dtpr_provider;

		if (prov->dtpv_pops.dtps_getargdesc == NULL) {
			/*
			 * There isn't any typed information for this probe.
			 * Set the argument number to DTRACE_ARGNONE.
			 */
			desc.dtargd_ndx = DTRACE_ARGNONE;
		} else {
			desc.dtargd_native[0] = '\0';
			desc.dtargd_xlate[0] = '\0';
			desc.dtargd_mapping = desc.dtargd_ndx;

			prov->dtpv_pops.dtps_getargdesc(prov->dtpv_arg,
			    probe->dtpr_id, probe->dtpr_arg, &desc);
		}

		mutex_exit(&mod_lock);
		mutex_exit(&dtrace_provider_lock);

		if (copyout(&desc, (void *)arg, sizeof (desc)) != 0)
			return (EFAULT);

		return (0);
	}

	case DTRACEIOC_GO: {
		processorid_t cpuid;

PRINT_CASE(DTRACEIOC_GO);
TODO();
		rval = dtrace_state_go(state, &cpuid);
TODO();

		if (rval != 0)
			return (rval);

		if (copyout(&cpuid, (void *)arg, sizeof (cpuid)) != 0)
			return (EFAULT);

		return (0);
	}

	case DTRACEIOC_STOP: {
		processorid_t cpuid;

PRINT_CASE(DTRACEIOC_STOP);
		mutex_enter(&dtrace_lock);
		rval = dtrace_state_stop(state, &cpuid);
		mutex_exit(&dtrace_lock);

		if (rval != 0)
			return (rval);

		if (copyout(&cpuid, (void *)arg, sizeof (cpuid)) != 0)
			return (EFAULT);

		return (0);
	}

# if 0
	case DTRACEIOC_DOFGET: {
		dof_hdr_t hdr, *dof;
		uint64_t len;

PRINT_CASE(DTRACEIOC_DOFGET);
		if (copyin((void *)arg, &hdr, sizeof (hdr)) != 0)
			return (EFAULT);

		mutex_enter(&dtrace_lock);
		dof = dtrace_dof_create(state);
		mutex_exit(&dtrace_lock);

		len = MIN(hdr.dofh_loadsz, dof->dofh_loadsz);
		rval = copyout(dof, (void *)arg, len);
		dtrace_dof_destroy(dof);

		return (rval == 0 ? 0 : EFAULT);
	}

	case DTRACEIOC_AGGSNAP:
	case DTRACEIOC_BUFSNAP: {
		dtrace_bufdesc_t desc;
		caddr_t cached;
		dtrace_buffer_t *buf;

PRINT_CASE(DTRACEIOC_BUFSNAP);
		if (copyin((void *)arg, &desc, sizeof (desc)) != 0)
			return (EFAULT);

		if (desc.dtbd_cpu < 0 || desc.dtbd_cpu >= NCPU)
			return (EINVAL);

		mutex_enter(&dtrace_lock);

		if (cmd == DTRACEIOC_BUFSNAP) {
			buf = &state->dts_buffer[desc.dtbd_cpu];
		} else {
			buf = &state->dts_aggbuffer[desc.dtbd_cpu];
		}

		if (buf->dtb_flags & (DTRACEBUF_RING | DTRACEBUF_FILL)) {
			size_t sz = buf->dtb_offset;

			if (state->dts_activity != DTRACE_ACTIVITY_STOPPED) {
				mutex_exit(&dtrace_lock);
				return (EBUSY);
			}

			/*
			 * If this buffer has already been consumed, we're
			 * going to indicate that there's nothing left here
			 * to consume.
			 */
			if (buf->dtb_flags & DTRACEBUF_CONSUMED) {
				mutex_exit(&dtrace_lock);

				desc.dtbd_size = 0;
				desc.dtbd_drops = 0;
				desc.dtbd_errors = 0;
				desc.dtbd_oldest = 0;
				sz = sizeof (desc);

				if (copyout(&desc, (void *)arg, sz) != 0)
					return (EFAULT);

				return (0);
			}

			/*
			 * If this is a ring buffer that has wrapped, we want
			 * to copy the whole thing out.
			 */
			if (buf->dtb_flags & DTRACEBUF_WRAPPED) {
				dtrace_buffer_polish(buf);
				sz = buf->dtb_size;
			}

			if (copyout(buf->dtb_tomax, desc.dtbd_data, sz) != 0) {
				mutex_exit(&dtrace_lock);
				return (EFAULT);
			}

			desc.dtbd_size = sz;
			desc.dtbd_drops = buf->dtb_drops;
			desc.dtbd_errors = buf->dtb_errors;
			desc.dtbd_oldest = buf->dtb_xamot_offset;

			mutex_exit(&dtrace_lock);

			if (copyout(&desc, (void *)arg, sizeof (desc)) != 0)
				return (EFAULT);

			buf->dtb_flags |= DTRACEBUF_CONSUMED;

			return (0);
		}

		if (buf->dtb_tomax == NULL) {
			ASSERT(buf->dtb_xamot == NULL);
			mutex_exit(&dtrace_lock);
			return (ENOENT);
		}

		cached = buf->dtb_tomax;
		ASSERT(!(buf->dtb_flags & DTRACEBUF_NOSWITCH));

		dtrace_xcall(desc.dtbd_cpu,
		    (dtrace_xcall_t)dtrace_buffer_switch, buf);

		state->dts_errors += buf->dtb_xamot_errors;

		/*
		 * If the buffers did not actually switch, then the cross call
		 * did not take place -- presumably because the given CPU is
		 * not in the ready set.  If this is the case, we'll return
		 * ENOENT.
		 */
		if (buf->dtb_tomax == cached) {
			ASSERT(buf->dtb_xamot != cached);
			mutex_exit(&dtrace_lock);
			return (ENOENT);
		}

		ASSERT(cached == buf->dtb_xamot);

		/*
		 * We have our snapshot; now copy it out.
		 */
		if (copyout(buf->dtb_xamot, desc.dtbd_data,
		    buf->dtb_xamot_offset) != 0) {
			mutex_exit(&dtrace_lock);
			return (EFAULT);
		}

		desc.dtbd_size = buf->dtb_xamot_offset;
		desc.dtbd_drops = buf->dtb_xamot_drops;
		desc.dtbd_errors = buf->dtb_xamot_errors;
		desc.dtbd_oldest = 0;

		mutex_exit(&dtrace_lock);

		/*
		 * Finally, copy out the buffer description.
		 */
		if (copyout(&desc, (void *)arg, sizeof (desc)) != 0)
			return (EFAULT);

		return (0);
	}

# endif
	case DTRACEIOC_STATUS: {
		dtrace_status_t stat;
		dtrace_dstate_t *dstate;
		int i, j;
		uint64_t nerrs;

PRINT_CASE(DTRACEIOC_STATUS);
		/*
		 * See the comment in dtrace_state_deadman() for the reason
		 * for setting dts_laststatus to INT64_MAX before setting
		 * it to the correct value.
		 */
		state->dts_laststatus = INT64_MAX;
		dtrace_membar_producer();
		state->dts_laststatus = dtrace_gethrtime();

		bzero(&stat, sizeof (stat));

		mutex_enter(&dtrace_lock);

		if (state->dts_activity == DTRACE_ACTIVITY_INACTIVE) {
			mutex_exit(&dtrace_lock);
			return (ENOENT);
		}

		if (state->dts_activity == DTRACE_ACTIVITY_DRAINING)
			stat.dtst_exiting = 1;

		nerrs = state->dts_errors;
		dstate = &state->dts_vstate.dtvs_dynvars;

		for (i = 0; i < NCPU; i++) {
			dtrace_dstate_percpu_t *dcpu = &dstate->dtds_percpu[i];

			stat.dtst_dyndrops += dcpu->dtdsc_drops;
			stat.dtst_dyndrops_dirty += dcpu->dtdsc_dirty_drops;
			stat.dtst_dyndrops_rinsing += dcpu->dtdsc_rinsing_drops;

			if (state->dts_buffer[i].dtb_flags & DTRACEBUF_FULL)
				stat.dtst_filled++;

			nerrs += state->dts_buffer[i].dtb_errors;

			for (j = 0; j < state->dts_nspeculations; j++) {
				dtrace_speculation_t *spec;
				dtrace_buffer_t *buf;

				spec = &state->dts_speculations[j];
				buf = &spec->dtsp_buffer[i];
				stat.dtst_specdrops += buf->dtb_xamot_drops;
			}
		}

		stat.dtst_specdrops_busy = state->dts_speculations_busy;
		stat.dtst_specdrops_unavail = state->dts_speculations_unavail;
		stat.dtst_killed =
		    (state->dts_activity == DTRACE_ACTIVITY_KILLED);
		stat.dtst_errors = nerrs;

		mutex_exit(&dtrace_lock);

		if (copyout(&stat, (void *)arg, sizeof (stat)) != 0)
			return (EFAULT);

		return (0);
	}

	case DTRACEIOC_FORMAT: {
		dtrace_fmtdesc_t fmt;
		char *str;
		int len;

PRINT_CASE(DTRACEIOC_FORMAT);
		if (copyin((void *)arg, &fmt, sizeof (fmt)) != 0)
			return (EFAULT);

		mutex_enter(&dtrace_lock);

		if (fmt.dtfd_format == 0 ||
		    fmt.dtfd_format > state->dts_nformats) {
			mutex_exit(&dtrace_lock);
			return (EINVAL);
		}

		/*
		 * Format strings are allocated contiguously and they are
		 * never freed; if a format index is less than the number
		 * of formats, we can assert that the format map is non-NULL
		 * and that the format for the specified index is non-NULL.
		 */
		ASSERT(state->dts_formats != NULL);
		str = state->dts_formats[fmt.dtfd_format - 1];
		ASSERT(str != NULL);

		len = strlen(str) + 1;

		if (len > fmt.dtfd_length) {
			fmt.dtfd_length = len;

			if (copyout(&fmt, (void *)arg, sizeof (fmt)) != 0) {
				mutex_exit(&dtrace_lock);
				return (EINVAL);
			}
		} else {
			if (copyout(str, fmt.dtfd_string, len) != 0) {
				mutex_exit(&dtrace_lock);
				return (EINVAL);
			}
		}

		mutex_exit(&dtrace_lock);
		return (0);
	}

	default:
		PRINT_CASE(unknown_case_stmt_for_ioctl);
		printk("cmd=%x\n", cmd);
		break;
	}

	return (ENOTTY);
}


# if !defined(DTRACE_PROTO_H)
# define	DTRACE_PROTO_H
/**********************************************************************/
/*   Prototypes  we  need  because  we  split dtrace.c up to make it  */
/*   easier to debug.						      */
/**********************************************************************/
int dtrace_state_go(dtrace_state_t *state, processorid_t *cpu);
void	dtrace_probe_provide(dtrace_probedesc_t *desc, dtrace_provider_t *);
void	dtrace_probekey(const dtrace_probedesc_t *pdp, dtrace_probekey_t *pkp);
void	dtrace_cred2priv(cred_t *cr, uint32_t *privp, uid_t *uidp);
int	dtrace_match_priv(const dtrace_probe_t *prp, uint32_t priv, uid_t uid);
int	dtrace_match_probe(const dtrace_probe_t *prp, const dtrace_probekey_t *pkp,
    uint32_t priv, uid_t uid);
void	dtrace_probe_description(const dtrace_probe_t *prp, dtrace_probedesc_t *pdp);
int dtrace_state_stop(dtrace_state_t *state, processorid_t *cpu);
dof_hdr_t *dtrace_dof_copyin(uintptr_t uarg, int *errp);
void dtrace_dof_destroy(dof_hdr_t *dof);
int dtrace_dof_slurp(dof_hdr_t *dof, dtrace_vstate_t *vstate, cred_t *cr, dtrace_enabling_t **enabp, uint64_t ubase, int noprobes);
void dtrace_enabling_destroy(dtrace_enabling_t *enab);
int dtrace_dof_options(dof_hdr_t *dof, dtrace_state_t *state);
int dtrace_probe_enable(const dtrace_probedesc_t *desc, dtrace_enabling_t *ep);
int dtrace_enabling_matchstate(dtrace_state_t *state, int *nmatched);
int dtrace_enabling_match(dtrace_enabling_t *enab, int *nmatched);
int dtrace_enabling_retain(dtrace_enabling_t *enab);
int dtrace_detach(void);
int dtrace_ioctl_helper(int cmd, intptr_t arg, int *rv);
int dtrace_ioctl(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);
int dtrace_attach(dev_info_t *devi, int cmd);
int dtrace_open(struct file *fp, int flag, int otyp, cred_t *cred_p);
# endif


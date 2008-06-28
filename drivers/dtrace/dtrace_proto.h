# if !defined(DTRACE_PROTO_H)
# define	DTRACE_PROTO_H
/**********************************************************************/
/*   Prototypes  we  *used*  to need because we split dtrace.c up to  */
/*   make it easier to debug.   				      */
/**********************************************************************/
void	dtrace_probe_provide(dtrace_probedesc_t *desc, dtrace_provider_t *);
void	dtrace_cred2priv(cred_t *cr, uint32_t *privp, uid_t *uidp, zoneid_t *zoneidp);
void	dtrace_probe_description(const dtrace_probe_t *prp, dtrace_probedesc_t *pdp);
dof_hdr_t *dtrace_dof_copyin(uintptr_t uarg, int *errp);
void dtrace_dof_destroy(dof_hdr_t *dof);
int dtrace_dof_options(dof_hdr_t *dof, dtrace_state_t *state);
int dtrace_probe_enable(const dtrace_probedesc_t *desc, dtrace_enabling_t *ep);
int dtrace_detach(dev_info_t *devi, ddi_detach_cmd_t cmd);
int dtrace_ioctl_helper(int cmd, intptr_t arg, int *rv);
int dtrace_ioctl(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);
int dtrace_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);
int dtrace_open(struct file *fp, int flag, int otyp, cred_t *cred_p);
int dtrace_close(struct file *fp, int flag, int otyp, cred_t *cred_p);
void dump_mem(char *cp, int len);
# endif


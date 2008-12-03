# if !defined(DTRACE_PROTO_H)
# define	DTRACE_PROTO_H
/**********************************************************************/
/*   Prototypes  we  *used*  to need because we split dtrace.c up to  */
/*   make it easier to debug.   				      */
/**********************************************************************/
ssize_t syms_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *pos);
unsigned long get_proc_addr(char *name);
void	hunt_init(void);

void	dtrace_probe_provide(dtrace_probedesc_t *desc, dtrace_provider_t *);
void	dtrace_cred2priv(cred_t *cr, uint32_t *privp, uid_t *uidp, zoneid_t *zoneidp);
int dtrace_detach(dev_info_t *devi, ddi_detach_cmd_t cmd);
int dtrace_ioctl_helper(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);
int dtrace_ioctl(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);
int dtrace_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);
int dtrace_open(struct file *fp, int flag, int otyp, cred_t *cred_p);
int dtrace_close(struct file *fp, int flag, int otyp, cred_t *cred_p);
void dump_mem(char *cp, int len);
int	lx_get_curthread_id(void);
void	par_setup_thread(void);
void	*par_setup_thread2(void);
u64	lx_rdmsr(int);
int uread(proc_t *, void *, size_t, uintptr_t);
int uwrite(proc_t *, void *, size_t, uintptr_t);
void *fbt_get_access_process_vm(void);
void *fbt_get_kernel_text_address(void);
int	fasttrap_attach(void);
int	fasttrap_detach(void);
proc_t *prfind(int p);

# endif


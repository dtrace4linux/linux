# if !defined(DTRACE_PROTO_H)
# define	DTRACE_PROTO_H
/**********************************************************************/
/*   Prototypes  we  *used*  to need because we split dtrace.c up to  */
/*   make it easier to debug.   				      */
/**********************************************************************/
ssize_t syms_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *pos);
void *get_proc_addr(char *name);
void	hunt_init(void);

void	dtrace_probe_provide(dtrace_probedesc_t *desc, dtrace_provider_t *);
void	dtrace_cred2priv(cred_t *cr, uint32_t *privp, uid_t *uidp, zoneid_t *zoneidp);
int dtrace_detach(dev_info_t *devi, ddi_detach_cmd_t cmd);
int dtrace_ioctl_helper(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);
int dtrace_ioctl(struct file *fp, int cmd, intptr_t arg, int md, cred_t *cr, int *rv);
int dtrace_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);
int dtrace_open(struct file *fp, int flag, int otyp, cred_t *cred_p);
int dtrace_close(struct file *fp, int flag, int otyp, cred_t *cred_p);
void dtrace_dump_mem(char *cp, int len);
void dtrace_dump_mem32(int *cp, int len);
void dtrace_dump_mem64(unsigned long *cp, int len);
int	lx_get_curthread_id(void);
void	par_setup_thread(void);
void	*par_setup_thread2(void);
u64	lx_rdmsr(int);
int uread(proc_t *, void *, size_t, uintptr_t);
int uwrite(proc_t *, void *, size_t, uintptr_t);
void *fbt_get_access_process_vm(void);
int	fasttrap_attach(void);
int	fasttrap_detach(void);
proc_t *prfind(int p);
int	tsignal(proc_t *, int);
void	trap(struct pt_regs *rp, caddr_t addr, processorid_t cpu);
int	dtrace_invop(uintptr_t addr, uintptr_t *stack, uintptr_t eax);
void dtrace_cpu_emulate(int instr, int opcode, struct pt_regs *regs);

int dtrace_user_probe(int, struct pt_regs *rp, caddr_t addr, processorid_t cpuid);
void	fbt_provide_kernel(void);

# if !defined(kmem_alloc)
void	*kmem_alloc(size_t, int);
void	*kmem_zalloc(size_t, int);
void	kmem_free(void *, int size);
# endif

char	*dtrace_memchr(const char *, int, int);
int	is_toxic_func(unsigned long a, const char *name);
int	memory_set_rw(void *addr, int num_pages, int is_kernel_addr);
int	on_notifier_list(uint8_t *);

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
int	mutex_is_locked(struct mutex *mp);
# endif

# endif


# if !defined(DTRACE_PROTO_H)
# define	DTRACE_PROTO_H

/**********************************************************************/
/*   Prototypes  we  *used*  to need because we split dtrace.c up to  */
/*   make it easier to debug.   				      */
/**********************************************************************/
ssize_t syms_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *pos);
ssize_t syms_read(struct file *fp, char __user *buf, size_t len, loff_t *off);
void *get_proc_addr(char *name);

int	init_cyclic(void);
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
int	dtrace_invop(uintptr_t addr, uintptr_t *stack, uintptr_t eax, trap_instr_t *);
void dtrace_cpu_emulate(int instr, int opcode, struct pt_regs *regs);
void	dtrace_print_regs(struct pt_regs *);
void	dtrace_vprintf(const char *fmt, va_list ap);
void	dtrace_printf(const char *fmt, ...);

int dtrace_user_probe(int, struct pt_regs *rp, caddr_t addr, processorid_t cpuid);
void	fbt_provide_kernel(void);
void	instr_provide_kernel(void);

# if !defined(kmem_alloc)
void	*kmem_alloc(size_t, int);
void	*kmem_zalloc(size_t, int);
void	kmem_free(void *, int size);
# endif

char	*dtrace_memchr(const char *, int, int);
int	is_toxic_func(unsigned long a, const char *name);
int	is_toxic_return(const char *name);
int	memory_set_rw(void *addr, int num_pages, int is_kernel_addr);
void	set_page_prot(unsigned long addr, int len, long and_prot, long or_prot);
int	on_notifier_list(uint8_t *);
int	mem_is_writable(volatile char *addr);

int	cpu_adjust(cpu_core_t *this_cpu, cpu_trap_t *tp, struct pt_regs *regs);
void	cpu_copy_instr(cpu_core_t *this_cpu, cpu_trap_t *tp, struct pt_regs *regs);
void	prcom_add_instruction(char *name, uint8_t *instr);
void	prcom_add_callback(char *probe, char *func, int (*callback)(dtrace_id_t, struct pt_regs *regs));
void	prcom_add_function(char *probe, char *func);
void	dtrace_parse_kernel(int, void (*callback)(uint8_t *, int), uint8_t *);
int	is_probable_instruction(instr_t *, int is_entry);
void	dtrace_instr_dump(char *label, uint8_t *insn);
dtrace_icookie_t dtrace_interrupt_get(void);
char * hrtime_str(hrtime_t s);
int dtrace_xen_hypercall(int call, void *a, void *b, void *c);
int	dtrace_is_xen(void);
int	xen_send_ipi(cpumask_t *, int);
void	xen_xcall_init(void);
void	xen_xcall_fini(void);

# endif

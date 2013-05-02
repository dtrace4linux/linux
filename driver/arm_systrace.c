/**********************************************************************/
/*   This  file  is  included  by  systrace.c.  I  know  its ugly to  */
/*   #include  a  C  source  file,  but  separating  out  the global  */
/*   definitions from implementation will just add more complexity.   */
/*   								      */
/*   Author: Paul Fox						      */
/*   Date: Apr 2013						      */
/**********************************************************************/

# define IS_PTREG_SYSCALL(n) \
		( \
		 (n) == __NR_clone || \
		 (n) == __NR_execve || \
		 (n) == __NR_fork || \
		 (n) == __NR_fstatfs64 || \
		 (n) == __NR_mmap2 || \
		 (n) == __NR_rt_sigreturn || \
		 (n) == __NR_sigaltstack || \
		 (n) == __NR_sigreturn || \
		 (n) == __NR_statfs64 || \
		 (n) == __NR_vfork \
		 )

int syscall_ptreg_template_size;
void syscall_ptreg_template(void) {}

static int (*do_sigaltstack_ptr)(const stack_t __user *uss, stack_t __user *uoss, unsigned long sp);
static asmlinkage int (*do_fork_ptr)(unsigned long, unsigned long, struct pt_regs *, unsigned long, int __user *, int __user *);
static asmlinkage int (*sys_fork_ptr)(struct pt_regs *);
static asmlinkage int (*sys_sigreturn_ptr)(struct pt_regs *);
static asmlinkage int (*sys_rt_sigreturn_ptr)(struct pt_regs *);
static asmlinkage int (*sys_execve_ptr)(const char __user *, 
	const char __user *const __user *, const char __user *const __user *, struct pt_regs *);
static asmlinkage int (*sys_vfork_ptr)(struct pt_regs *);
static asmlinkage long (*sys_statfs64_ptr)(const char __user *path, size_t sz, struct statfs64 __user *buf);
static asmlinkage long (*sys_fstatfs64_ptr)(unsigned int fd, size_t sz, struct statfs64 __user *buf);
static asmlinkage long (*sys_mmap_pgoff_ptr)(unsigned long addr, unsigned long len,
                      unsigned long prot, unsigned long flags,
                      unsigned long fd, unsigned long pgoff);

/**********************************************************************/
/*   Funky prototypes for the assembler wrappers.		      */
/**********************************************************************/
void syscall_clone_wrapper(void);
void syscall_execve_wrapper(void);
void syscall_fork_wrapper(void);
void syscall_fstatfs64_wrapper(void);
void syscall_mmap2_wrapper(void);
void syscall_rt_sigreturn_wrapper(void);
void syscall_sigaltstack_wrapper(void);
void syscall_sigreturn_wrapper(void);
void syscall_statfs64_wrapper(void);
void syscall_vfork_wrapper(void);

void
arm_systrace_init(void)
{
	do_fork_ptr = (void *) get_proc_addr("do_fork");
	do_sigaltstack_ptr = (void *) get_proc_addr("do_sigaltstack");
	sys_execve_ptr = (void *) get_proc_addr("sys_execve");
	sys_fork_ptr = (void *) get_proc_addr("sys_fork");
	sys_fstatfs64_ptr = (void *) get_proc_addr("sys_fstatfs64");
	sys_rt_sigreturn_ptr = (void *) get_proc_addr("sys_rt_sigreturn");
	sys_sigreturn_ptr = (void *) get_proc_addr("sys_sigreturn");
	sys_statfs64_ptr = (void *) get_proc_addr("sys_statfs64");
	sys_vfork_ptr = (void *) get_proc_addr("sys_vfork");
	sys_mmap_pgoff_ptr = (void *) get_proc_addr("sys_mmap_pgoff");
}
void
systrace_arm_assembler_dummy(void)
{
	__asm(
		/***********************************************/
		/*   Following function is used as a template  */
		/*   for  all syscalls. We replicate the code  */
		/*   for  each syscall, but replace the dummy  */
		/*   value  with  the ordinal of the syscall.  */
		/*   We   have   to  be  very  careful  about  */
		/*   re-entrancy   (a   syscall  shouldnt  be  */
		/*   interrupted  by another task), but we do  */
		/*   have  to care about which cpu we are on.  */
		/*   RAX  holds  the  syscall  number, but it  */
		/*   is/maybe           corrupted          in  */
		/*   dtrace_systrace_syscall() by GCC code.    */
		/***********************************************/
		FUNCTION(syscall_template)
		/***********************************************/
		/*   We  have 4 args in registers, two on the  */
		/*   stack, but we need to make space for two  */
		/*   extra arguments.			       */
		/***********************************************/
			//arg6   //16
			//arg5   //12
		"push {lr}\n"

		"ldr lr,[sp, #8]\n"
		"push {lr}\n"
		"ldr lr,[sp, #8]\n"
		"push {lr}\n"

		"push {r3}\n"  // bubble up the args
		"push {r2}\n"

		"mov r3,r1\n"
		"mov r2,r0\n"
//		"add r1,sp,#24\n" // r1 = regs
		"mov r1,#0\n"
		"ldr r0,syscall_num\n"
		"ldr r12,addr\n"
		"blx r12\n"
		"add sp, sp, #16\n"
		"pop {pc}\n"

		"syscall_num: .word 0x1234\n"
		"addr: .word dtrace_systrace_syscall\n"
		"syscall_template_size: .word .-syscall_template\n"
		END_FUNCTION(syscall_template)

		/***********************************************/
		/*   Following      for     sys_clone_wrapper  */
		/*   (entry-common.S)			       */
		/***********************************************/
		".equ S_OFF,8\n"
		"why .req r8\n"

		FUNCTION(syscall_clone_wrapper)
		"add ip, sp, #S_OFF\n"
		"str ip,[sp, #4]\n"
		"b dtrace_systrace_syscall_clone\n"
		END_FUNCTION(syscall_clone_wrapper)

		FUNCTION(syscall_execve_wrapper)
		"add r3, sp, #S_OFF\n"
		"b dtrace_systrace_syscall_execve\n"
		END_FUNCTION(syscall_execve_wrapper)

		FUNCTION(syscall_fork_wrapper)
		"add r0, sp, #S_OFF\n"
		"b dtrace_systrace_syscall_fork\n"
		END_FUNCTION(syscall_fork_wrapper)

		FUNCTION(syscall_mmap2_wrapper)
# if PAGE_SHIFT > 12
		"tst r5, #PGOFF_MASK\n"
		"moveq r5, r5, lsr #PAGE_SHIFT - 12\n"
		"streq r5, [sp, #4]\n"
		"beq dtrace_systrace_syscall_mmap2\n"
		"mov r0,#-EINVAL\n"
		"mov pc, lr\n"
# else
		"str r5,[sp, #4]\n"
		"b dtrace_systrace_syscall_mmap2\n"
# endif
		END_FUNCTION(syscall_mmap2_wrapper)

		FUNCTION(syscall_sigaltstack_wrapper)
		"ldr r2,[sp, #S_OFF + 0x60]\n" // S_SP
		"b dtrace_systrace_syscall_sigaltstack\n"
		END_FUNCTION(syscall_sigaltstack_wrapper)

		FUNCTION(syscall_sigreturn_wrapper)
		"add r0, sp, #S_OFF\n"
		"mov why,#0\n"    // prevent syscall restart handling
		"b dtrace_systrace_syscall_sigreturn\n"
		END_FUNCTION(syscall_sigreturn_wrapper)

		FUNCTION(syscall_rt_sigreturn_wrapper)
		"add r0, sp, #S_OFF\n"
		"mov why,#0\n"    // prevent syscall restart handling
		"b dtrace_systrace_syscall_rt_sigreturn\n"
		END_FUNCTION(syscall_rt_sigreturn_wrapper)

		FUNCTION(syscall_fstatfs64_wrapper)
		"teq r1, #88\n"
		"moveq r1, #84\n"
		"b dtrace_systrace_syscall_fstatfs64\n"
		END_FUNCTION(syscall_fstatfs64_wrapper)

		FUNCTION(syscall_statfs64_wrapper)
		"teq r1, #88\n"
		"moveq r1, #84\n"
		"b dtrace_systrace_syscall_statfs64\n"
		END_FUNCTION(syscall_statfs64_wrapper)

		FUNCTION(syscall_vfork_wrapper)
		"add r0, sp, #S_OFF\n"
		"b dtrace_systrace_syscall_vfork\n"
		END_FUNCTION(syscall_vfork_wrapper)
		);
}

# define TRACE_BEFORE(call, arg0, arg1, arg2, arg3, arg4, arg5) \
        if ((id = systrace_sysent[call].stsy_entry) != DTRACE_IDNONE) { \
		cpu_core_t *this_cpu = cpu_get_this();			\
		this_cpu->cpuc_regs = (struct pt_regs *) regs;		\
									\
                (*systrace_probe)(id, (uintptr_t) (arg0), (uintptr_t) arg1, 	\
			(uintptr_t) arg2, (uintptr_t) arg3,  		\
			(uintptr_t) arg4, arg5);			\
	}
# define TRACE_AFTER(call, a, b, c, d, e, f) \
        if ((id = systrace_sysent[call].stsy_return) != DTRACE_IDNONE) { \
		/***********************************************/	\
		/*   Map   Linux   style   syscall returns to  */	\
		/*   standard Unix format.		       */	\
		/***********************************************/	\
		(*systrace_probe)(id, (uintptr_t) (a),			\
		    (uintptr_t) (b),					\
                    (uintptr_t) (c), d, e, f);				\
	}

asmlinkage int
dtrace_systrace_syscall_clone(unsigned long clone_flags, unsigned long newsp,
	void __user *parent_tid, int tls_val,
	void __user *child_tid, struct pt_regs *regs)
{      	dtrace_id_t id;
	int	ret;

	TRACE_BEFORE(__NR_clone, clone_flags, newsp, parent_tid, child_tid, regs, 0);

	/***********************************************/
	/*   Cant call sys_clone directly because the  */
	/*   stack  confuses  sys_clone  in  the  new  */
	/*   child,  as  it  tries  to return to user  */
	/*   space. This seems to work.		       */
	/***********************************************/
	if (newsp == 0)
		newsp = regs->ARM_sp;

	ret = do_fork_ptr(clone_flags, newsp, regs, 0, parent_tid, child_tid);

	TRACE_AFTER(__NR_clone, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_execve(const char __user *name,
		const char __user *const __user *argv,
		const char __user *const __user *envp,
		struct pt_regs *regs)
{      	dtrace_id_t id;
	int	ret;

	TRACE_BEFORE(__NR_execve, name, argv, envp, regs, 0, 0);

        ret = sys_execve_ptr(name, argv, envp, regs);

	TRACE_AFTER(__NR_execve, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_fork(struct pt_regs *regs)
{      	dtrace_id_t id;
	int	ret;

	TRACE_BEFORE(__NR_fork, regs, 0, 0, 0, 0, 0);

        ret = sys_fork_ptr(regs);

	TRACE_AFTER(__NR_fork, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_fstatfs64(unsigned int fd, size_t sz, struct statfs64 __user *buf)
{      	dtrace_id_t id;
	int	ret;
	struct pt_regs *regs = NULL; // hack

	TRACE_BEFORE(__NR_fstatfs64, fd, sz, buf, 0, 0, 0);

        ret = sys_fstatfs64_ptr(fd, sz, buf);

	TRACE_AFTER(__NR_fstatfs64, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_mmap2(unsigned long addr, unsigned long len,
                      unsigned long prot, unsigned long flags,
                      unsigned long fd, unsigned long pgoff)
{      	dtrace_id_t id;
	int	ret;
	struct pt_regs *regs = NULL; // hack

	TRACE_BEFORE(__NR_mmap2, addr, len, prot, flags, fd, pgoff);

        ret = sys_mmap_pgoff_ptr(addr, len, prot, flags, fd, pgoff);

	TRACE_AFTER(__NR_mmap2, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_sigaltstack(const stack_t __user *uss, stack_t __user *uoss, unsigned long sp)
{      	dtrace_id_t id;
	int	ret;
	struct pt_regs *regs = NULL; // hack

	TRACE_BEFORE(__NR_sigaltstack, uss, uss, sp, 0, 0, 0);

        ret = do_sigaltstack_ptr(uss, uoss, sp);

	TRACE_AFTER(__NR_sigaltstack, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_sigreturn(struct pt_regs *regs)
{      	dtrace_id_t id;
	int	ret;

	TRACE_BEFORE(__NR_sigreturn, regs, 0, 0, 0, 0, 0);

        ret = sys_sigreturn_ptr(regs);

	TRACE_AFTER(__NR_sigreturn, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_rt_sigreturn(struct pt_regs *regs)
{      	dtrace_id_t id;
	int	ret;

	TRACE_BEFORE(__NR_rt_sigreturn, regs, 0, 0, 0, 0, 0);

        ret = sys_rt_sigreturn_ptr(regs);

	TRACE_AFTER(__NR_rt_sigreturn, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_statfs64(const char __user *path, size_t sz, struct statfs64 __user *buf)
{      	dtrace_id_t id;
	int	ret;
	struct pt_regs *regs = NULL; // hack

	TRACE_BEFORE(__NR_statfs64, path, sz, buf, 0, 0, 0);

        ret = sys_statfs64_ptr(path, sz, buf);

	TRACE_AFTER(__NR_statfs64, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

asmlinkage int
dtrace_systrace_syscall_vfork(struct pt_regs *regs)
{      	dtrace_id_t id;
	int	ret;

	TRACE_BEFORE(__NR_vfork, regs, 0, 0, 0, 0, 0);

        ret = sys_vfork_ptr(regs);

	TRACE_AFTER(__NR_vfork, ret < 0 ? -1 : ret, (int64_t) ret, (int64_t) ret >> 32, 0, 0, 0);

	return ret;
}

/**********************************************************************/
/*   Handle  which  code  does  interposing  -  some  syscalls  have  */
/*   modified  assembler call sequences, so we handle the difference  */
/*   here.							      */
/**********************************************************************/
static void *
get_interposer(int sysnum, int enable)
{
	switch (sysnum) {
	  case __NR_clone:
		return (void *) syscall_clone_wrapper;

	  case __NR_execve:
		return (void *) syscall_execve_wrapper;

	  case __NR_fork:
		return (void *) syscall_fork_wrapper;

	  case __NR_fstatfs64:
		return (void *) syscall_fstatfs64_wrapper;

	  case __NR_mmap2:
		return (void *) syscall_mmap2_wrapper;

	  case __NR_sigaltstack:
		return (void *) syscall_sigaltstack_wrapper;

	  case __NR_sigreturn:
		return (void *) syscall_sigreturn_wrapper;

	  case __NR_rt_sigreturn:
		return (void *) syscall_rt_sigreturn_wrapper;

	  case __NR_statfs64:
		return (void *) syscall_statfs64_wrapper;

	  case __NR_vfork:
		return (void *) syscall_vfork_wrapper;
	  }

	return syscall_info[sysnum].s_template;
}


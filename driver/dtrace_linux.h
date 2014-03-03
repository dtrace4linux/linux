# if !defined(DTRACE_LINUX_H)
# define DTRACE_LINUX_H 1

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
 *
 *
 */

#include "../port.h"
#include <linux/zone.h>
#include <linux_types.h>
#include <linux/smp.h>
#include <linux/gfp.h>
#include <linux/kdev_t.h>
#include <sys/cyclic.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <asm/uaccess.h>

/**********************************************************************/
/*   Set  this to GPL or CDDL if debugging the driver. Really we are  */
/*   CDDL.							      */
/**********************************************************************/
# define	DRIVER_LICENSE "CDDL"

# if HAVE_INCLUDE_LINUX_SLAB_H
#     include <linux/slab.h>
# endif

#if defined(HAVE_INCLUDE_ASM_KDEBUG_H)
#  include <asm/kdebug.h>
#endif
#if defined(HAVE_INCLUDE_LINUX_KDEBUG_H)
#  include <linux/kdebug.h>
#endif
#if defined(HAVE_INCLUDE_LINUX_HRTIMER_H)
#  include <linux/hrtimer.h>
#endif

/**********************************************************************/
/*   Breakpoint instruction.					      */
/**********************************************************************/
#define	PATCHVAL	0xcc	/* INT3 instruction */

# define MUTEX_HELD mutex_is_locked

#define PRIV_EFFECTIVE          (1 << 0)
#define PRIV_DTRACE_KERNEL      (1 << 1)
#define PRIV_DTRACE_PROC        (1 << 2)
#define PRIV_DTRACE_USER        (1 << 3)
#define PRIV_PROC_OWNER         (1 << 4)
#define PRIV_PROC_ZONE          (1 << 5)
#define PRIV_ALL                ~0

#define LOCK_LEVEL      10

#define ttoproc(x)      ((x))
#define	uprintf		printk
#define	vuprintf	vprintk

# define PRINT_CASE(x) do { if (dtrace_here) printk("%s:%s:%d: %s\n", dtrace_basename(__FILE__), __func__, __LINE__, #x); } while (0)

# define crhold(x)
# define priv_isequalset(a, b) 1
# define priv_getset(a, b) 1

typedef uint32_t ipaddr_t;

/*
 * Macro for checking power of 2 address alignment.
 */
#define IS_P2ALIGNED(v, a) ((((uintptr_t)(v)) & ((uintptr_t)(a) - 1)) == 0)
#define NSIG _NSIG

# define 	hz	HZ

/**********************************************************************/
/*   Code for 3.8.0 kernels which do strict kuid_t type checking.     */
/**********************************************************************/
#ifdef CONFIG_UIDGID_STRICT_TYPE_CHECKS
#	define	KUIDT_VALUE(v) v.val
#	define	KGIDT_VALUE(v) v.val
#else
#	define	KUIDT_VALUE(v) v
#	define	KGIDT_VALUE(v) v
#endif

/**********************************************************************/
/*   File  based  on  code  from  FreeBSD  to  support  the  missing  */
/*   task_struct fields which dtrace wants.			      */
/**********************************************************************/

typedef struct	sol_proc_t {

	uint32_t	p_flag;
	int		p_stat;
	pid_t pid;
	pid_t ppid;
	struct task_struct *p_task;
	char		*p_private_page;	/* Page allocated in user space for */
						/* fasttrap scratch buffer.	*/

        uint_t          t_predcache;    /* DTrace predicate cache */
	
        hrtime_t        t_dtrace_vtime; /* DTrace virtual time */
        hrtime_t        t_dtrace_start; /* DTrace slice start time */

        uint8_t         t_dtrace_stop;  /* indicates a DTrace-desired stop */
        uint8_t         t_dtrace_sig;   /* signal sent via DTrace's raise() */
	union __tdu {
		struct __tds {
			uint8_t	_t_dtrace_on;	/* hit a fasttrap tracepoint */
			uint8_t	_t_dtrace_step;	/* about to return to kernel */
			uint8_t	_t_dtrace_ret;	/* handling a return probe */
			uint8_t	_t_dtrace_ast;	/* saved ast flag */
#ifdef __amd64
			uint8_t	_t_dtrace_reg;	/* modified register */
#endif
		} _tds;
		ulong_t	_t_dtrace_ft;		/* bitwise or of these flags */
	} _tdu;
#define	t_dtrace_ft	_tdu._t_dtrace_ft
#define	t_dtrace_on	_tdu._tds._t_dtrace_on
#define	t_dtrace_step	_tdu._tds._t_dtrace_step
#define	t_dtrace_ret	_tdu._tds._t_dtrace_ret
#define	t_dtrace_ast	_tdu._tds._t_dtrace_ast
#ifdef __amd64
#define	t_dtrace_reg	_tdu._tds._t_dtrace_reg
#endif

	struct sol_proc *parent;
	int		p_pid;
	int		p_ppid;
	int		t_sig_check;
	void            *p_dtrace_helpers; /* DTrace helpers, if any */
	struct sol_proc_t *t_procp;
        struct  cred    *p_cred;        /* process credentials */
	mutex_t		p_lock;
	mutex_t		p_crlock;
	int             p_dtrace_probes; /* are there probes for this proc? */
	uint64_t        p_dtrace_count; /* number of DTrace tracepoints */
                                        /* (protected by P_PR_LOCK) */
	int		p_model;
	uintptr_t	t_dtrace_pc;	/* DTrace saved pc from fasttrap */
	uintptr_t	t_dtrace_npc;	/* DTrace next pc from fasttrap */
	uintptr_t	t_dtrace_scrpc;	/* DTrace per-thread scratch location */
	uintptr_t	t_dtrace_astpc;	/* DTrace return sequence location */
#ifdef __amd64
	uint64_t	t_dtrace_regv;	/* DTrace saved reg from fasttrap */
#endif
	struct pt_regs	*t_regs;
	} sol_proc_t;

typedef sol_proc_t proc_t;
//# define task_struct sol_proc_t
# define	curthread curproc
extern sol_proc_t	*curthread;

# define	td_tid	pid
# define	t_tid   td_tid
# define	comm t_proc->comm

int priv_policy(const cred_t *, int, int, int, const char *);
int priv_policy_only(const cred_t *, int, int);
//int priv_policy_choice(const cred_t *, int, int);

/**********************************************************************/
/*   Some  kernels dont have read_cr2(), so inline it here and avoid  */
/*   a naming conflict.						      */
/**********************************************************************/
static inline unsigned long read_cr2_register(void)
{

#if defined(__arm__)
	return 0;
#else
	unsigned long val;
	asm volatile("mov %%cr2,%0\n" : "=r" (val));
	return val;
#endif
}

/*
 * Test privilege. Audit success or failure, allow privilege debugging.
 * Returns 0 for success, err for failure.
 */
#define PRIV_POLICY(cred, priv, all, err, reason) \
                priv_policy((cred), (priv), (all), (err), (reason))

/*
 * Test privilege. Audit success only, no privilege debugging.
 * Returns 1 for success, and 0 for failure.
 */
#define PRIV_POLICY_CHOICE(cred, priv, all) \
                priv_policy_choice((cred), (priv), (all))

/*
 * Test privilege. No priv_debugging, no auditing.
 * Returns 1 for success, and 0 for failure.
 */

#define PRIV_POLICY_ONLY(cred, priv, all) \
                priv_policy_only((cred), (priv), (all))

typedef int ddi_attach_cmd_t;
typedef int ddi_detach_cmd_t;
# define	DDI_FAILURE	-1
# define	DDI_SUCCESS	0

char	*getenv(char *);
int	copyin(void *, void *, int);
int kill_pid(struct pid *pid, int sig, int priv);
//void	*kmem_cache_alloc(kmem_cache_t *cache, int flags);
//void	*new_unr(struct unrhdr *uh, void **p1, void **p2);
void	*vmem_alloc(vmem_t *, size_t, int);
void	*vmem_zalloc(vmem_t *, size_t, int);
void	membar_enter(void);
void	membar_producer(void);
void	debug_enter(char *);
int	dtrace_data_model(proc_t *);
void	dtrace_vtime_disable(void);
void	dtrace_vtime_enable(void);
void	freeenv(char *);
//void	kmem_cache_free(kmem_cache_t *, void *);
//void	kmem_free(void *, int);
void	vmem_destroy(vmem_t *);
void	vmem_free(vmem_t *, void *, size_t);
#define	vmem_alloc_t int
#define	vmem_free_t int
#define	vmem_mem_t int
void *vmem_create(const char *name, void *base, size_t size, size_t quantum,
        vmem_alloc_t *afunc, vmem_free_t *ffunc, vmem_t *source,
        size_t qcache_max, int vmflag);
//void	*kmem_zalloc(size_t size, int kmflags);

extern int panic_quiesce;

# if linux
#	define	cpu_get_id()	smp_processor_id()
# else
#	define	cpu_get_id()	CPU->cpu_id
# endif
# define copyin(a, b, c) copy_from_user(b, a, c)
# define copyout(a, b, c) copy_to_user(b, a, c)

char *linux_get_proc_comm(void);
int validate_ptr(const void *);

/**********************************************************************/
/*   Used by cyclic.c						      */
/**********************************************************************/
typedef unsigned long ksema_t;

typedef enum {
        SEMA_DEFAULT,
        SEMA_DRIVER
} ksema_type_t;

/**********************************************************************/
/*   Macro to create a function (like ENTRY()) inside an __asm block  */
/*   in a C function.						      */
/**********************************************************************/
# if defined(__i386) || defined(__amd64)
# 	define FUNCTION(x) 			\
	        ".text\n" 			\
		".globl " #x "\n" 		\
	        ".type   " #x ", @function\n"	\
		#x ":\n"
# elif defined(__arm__)
# 	define FUNCTION(x) 			\
	        ".text\n" 			\
		".global " #x "\n" 		\
	        ".type   " #x ", %function\n"	\
		#x ":\n"
#endif
# define END_FUNCTION(x) \
	".size " #x ", .-" #x "\n"

/**********************************************************************/
/*   Parallel  alloc  mechanism functions. We dont want to patch the  */
/*   kernel  but  we need per-structure additions, so we need a hash  */
/*   table or list so we can go from kernel object to dtrace object.  */
/**********************************************************************/
# define	PARD_FBT	0
# define	PARD_INSTR	1
typedef struct par_alloc_t {
	int	pa_domain;	/* Avoid collisions when modules needed by */
				/* different providers. */
	void	*pa_ptr;
	struct par_alloc_t *pa_next;
	} par_alloc_t;

typedef struct par_module_t {
	par_alloc_t	pm_alloc;
	int	fbt_nentries;
	} par_module_t;

/**********************************************************************/
/*   The following implements the security model for dtrace. We have  */
/*   a list of items which are used to match process attributes.      */
/*   								      */
/*   We  want  to be generic, and allow the user to customise to the  */
/*   local   security   regime.   We  allow  specific  ids  to  have  */
/*   privileges and also to do the same against group ids.	      */
/*   								      */
/*   For  each security item, we can assign a distinct set of dtrace  */
/*   privileges   (set   of   flags).   These   are   based  on  the  */
/*   DTRACE_PRIV_xxxx  definitions.  On  Solaris, these would be set  */
/*   via  policy  files accessed as the driver is loaded. For Linux,  */
/*   we  try  to generalise the mechanism to provide finer levels of  */
/*   granularity  and  allow  group  ids  or  groups  of ids to have  */
/*   similar settings.						      */
/*   								      */
/*   The  load.pl script will load up the security settings from the  */
/*   file  /etc/dtrace.conf  (if  available). See the etc/ directory  */
/*   for an example config file.				      */
/*   								      */
/*   The format of a command line is as follows:		      */
/*   								      */
/*   clear                              			      */
/*   uid 1 priv_user priv_kernel				      */
/*   gid 23 priv_proc						      */
/*   all priv_owner						      */
/*   								      */
/*   Multiple  privilege  flags  can  be set for an id. The array of  */
/*   descriptors   is  searched  linearly,  so  you  can  specify  a  */
/*   fallback, as in the example above. 			      */
/*   								      */
/*   There  is  a limit to the max number of descriptors. This could  */
/*   change  to  be  based  on  a dynamic array rather than a static  */
/*   array,  but  it  is  expected  that a typical scenario will use  */
/*   group  mappings,  and  at  most, a handful. (Current array size  */
/*   limited to 64).						      */
/**********************************************************************/
# define	DIT_UID	1
# define	DIT_GID	2
# define	DIT_ALL 3
typedef struct dsec_item_t {
	unsigned char	di_flags;
	unsigned char	di_type;	/* DIT_UID, DIT_GID, DIT_ALL */
	unsigned short	di_priv;	/* The dtrace credentials */
	unsigned int	di_id;		/* The uid or group id */
	} dsec_item_t;

/**********************************************************************/
/*   The  following  is  common for fbt/sdt so we can parse function  */
/*   entry/return functions using shared code.			      */
/**********************************************************************/
typedef struct pf_info_t {
	struct modctl	*mp;
	par_module_t	*pmp;
	char		*modname;
	char		*func;
	char		*name;
	char		*name2;	/* Name of the 'return' part of the probes. */
	int		symndx;
	int		do_print;
	uint8_t		*st_value;
	int		(*func_entry)(struct pf_info_t *, uint8_t *, int, int);
	int		(*func_return)(struct pf_info_t *, uint8_t *, int);
	int		(*func_sdt)(struct pf_info_t *, uint8_t *, int, int);
	void		*retptr;
	int		flags;
	} pf_info_t;
void dtrace_parse_function(pf_info_t *, uint8_t *, uint8_t *);
int dtrace_function_size(char *name, uint8_t **start, int *size);

#define	PARSE_GS_INC	0	/* Look for per-thread counter increments */
#define	PARSE_CALL	1	/* Look for specific call opcodes */

/**********************************************************************/
/*   Used  to  turn  off/on  page protection whilst we update kernel  */
/*   structures.  Preferable  to  do this than touch the page tables  */
/*   direct (especially on a Xen based kernel).			      */
/**********************************************************************/
#define wp_disable() write_cr0(read_cr0() & ~X86_CR0_WP)
#define wp_enable() write_cr0(read_cr0() | X86_CR0_WP)

/**********************************************************************/
/*   Set  to  true  when  something  bad  goes wrong - for debugging  */
/*   purposes.							      */
/**********************************************************************/
extern int dtrace_shutdown;

/**********************************************************************/
/*   Stats   counters   -  for  seeing  where  we  got  to:  ad  hoc  */
/*   debugging/performance monitoring.				      */
/**********************************************************************/
# define MAX_DCNT	32
extern unsigned long dcnt[MAX_DCNT];

int priv_policy_choice(const cred_t *a, int priv, int allzone);
void *par_alloc(int, void *, int, int *);
proc_t * par_find_thread(struct task_struct *t);
void par_free(int, void *ptr);
int fulword(const void *addr, uintptr_t *valuep);
int fuword8(const void *addr, unsigned char *valuep);
int fuword32(const void *addr, uint32_t *valuep);
int suword32(const void *addr, uint32_t value);
int sulword(const void *addr, ulong_t value);
int instr_in_text_seg(struct module *mp, char *name, Elf_Sym *sym);
cpu_core_t	*cpu_get_this(void);
int	is_kernel_text(unsigned long);
asmlinkage int dtrace_memcpy_with_error(void *, void *, size_t);
void set_console_on(int flag);
void dtrace_linux_panic(const char *, ...);
int libc_strlen(const char *str);
int libc_strcmp(const char *p1, const char *p2);
int libc_strncmp(const char *s1, const char *s2, int len);
timeout_id_t timeout(void (*func)(void *), void *arg, unsigned long ticks);
void untimeout(timeout_id_t id);
void *prcom_get_arg(int n, int size);
void *par_setup_thread1(struct task_struct *tp);
int	get_proc_name(unsigned long, char *buf);

/**********************************************************************/
/*   Some  kernels  dont  define if not SMP, but we define anyway so  */
/*   long  as  we  agree with smp.h header file. Even worse, on some  */
/*   2.6.18  kernels we have 4 args and 5 on others so we cannot use  */
/*   the KERNEL_VERSION macro to help us.			      */
/**********************************************************************/
#if SMP_CALL_FUNCTION_SINGLE_ARGS == 5
int smp_call_function_single(int cpuid, void (*func)(void *), void *info, int retry, int wait);
#define SMP_CALL_FUNCTION_SINGLE(a, b, c, d) smp_call_function_single(a, b, c, 0, d)
#else
int smp_call_function_single(int cpuid, void (*func)(void *), void *info, int wait);
#define SMP_CALL_FUNCTION_SINGLE(a, b, c, d) smp_call_function_single(a, b, c, d)
# endif

#if SMP_CALL_FUNCTION_ARGS == 3
#define SMP_CALL_FUNCTION(a, b, c) smp_call_function(a, b, c)
#else
#define SMP_CALL_FUNCTION(a, b, c) smp_call_function(a, b, 0, c)
#endif

# endif

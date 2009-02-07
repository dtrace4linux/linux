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

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
#  include <asm/kdebug.h>
#else
#  include <linux/hrtimer.h>
#  include <linux/kdebug.h>
#  include <linux/uaccess.h>
#endif

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

#define	mutex_enter(x)	mutex_lock(x)
#define	mutex_exit(x)	mutex_unlock(x)

# define PRINT_CASE(x) do { if (dtrace_here) printk("%s(%d):%s: %s\n", __FILE__, __LINE__, __func__, #x); } while (0)

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
/*   File  based  on  code  from  FreeBSD  to  support  the  missing  */
/*   task_struct fields which dtrace wants.			      */
/**********************************************************************/

typedef struct	sol_proc_t {

	uint32_t	p_flag;
	int		p_stat;
	pid_t pid;
	pid_t ppid;
	struct task_struct *p_task;

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
	struct mutex	p_lock;
	struct mutex	p_crlock;
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
extern uintptr_t	_userlimit;
# define	dtrace_panic	panic

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
/*   Parallel  alloc  mechanism functions. We dont want to patch the  */
/*   kernel  but  we need per-structure additions, so we need a hash  */
/*   table or list so we can go from kernel object to dtrace object.  */
/**********************************************************************/
typedef struct par_alloc_t {
	void	*pa_ptr;
	struct par_alloc_t *pa_next;
	} par_alloc_t;

typedef struct par_module_t {
	void	*pa_ptr;
	struct par_alloc_t *pa_next;
	int	fbt_nentries;
	} par_module_t;

void *par_alloc(void *, int, int *);
proc_t * par_find_thread(struct task_struct *t);
void par_free(void *ptr);
int fulword(const void *addr, uintptr_t *valuep);
int fuword8(const void *addr, unsigned char *valuep);
int fuword32(const void *addr, uint32_t *valuep);
int suword32(const void *addr, uint32_t value);
int sulword(const void *addr, ulong_t value);

# endif

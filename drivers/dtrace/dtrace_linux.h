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

#include <linux_types.h>

#include <smp.h>
#include <gfp.h>
#include <sys/kdev_t.h>
#include <sys/cyclic.h>
#include <sched.h>
#include <linux/hardirq.h>

# define TODO()	printk("%s:%d: please fill me in\n", __func__, __LINE__)
# define TODO_END()

#define	current	_current /* is a macro in <current.h> */
#define PRIV_EFFECTIVE          (1 << 0)
#define PRIV_DTRACE_KERNEL      (1 << 1)
#define PRIV_DTRACE_PROC        (1 << 2)
#define PRIV_DTRACE_USER        (1 << 3)
#define PRIV_PROC_OWNER         (1 << 4)
#define PRIV_PROC_ZONE          (1 << 5)
#define PRIV_ALL                ~0

#define LOCK_LEVEL      10

//#define ttoproc(x)      ((x)->t_procp)
#define ttoproc(x)      ((x))
#define	makedevice	MKDEV
#define	getminor(x)	MINOR(x)
#define	minor(x)	MINOR(x)
#define	getmajor(x)	MAJOR(x)
#define	uprintf		printk
#define	vuprintf	vprintk

#define	mutex_enter(x)	mutex_lock(x)

/*
typedef int	major_t;
typedef int	minor_t;
*/

/*
 * Macro for checking power of 2 address alignment.
 */
#define IS_P2ALIGNED(v, a) ((((uintptr_t)(v)) & ((uintptr_t)(a) - 1)) == 0)
#define NSIG _NSIG

/**********************************************************************/
/*   File  based  on  code  from  FreeBSD  to  support  the  missing  */
/*   task_struct fields which dtrace wants.			      */
/**********************************************************************/

typedef struct	sol_proc_t {

	uint32_t	p_flag;
	pid_t pid;

        uint_t          t_predcache;    /* DTrace predicate cache */
	
        hrtime_t        t_dtrace_vtime; /* DTrace virtual time */
        hrtime_t        t_dtrace_start; /* DTrace slice start time */

        uint8_t         t_dtrace_stop;  /* indicates a DTrace-desired stop */
        uint8_t         t_dtrace_sig;   /* signal sent via DTrace's raise() */

	struct sol_proc *parent;
	int		p_pid;
	int		t_sig_check;
	struct task_struct *t_proc;
	void            *p_dtrace_helpers; /* DTrace helpers, if any */
	struct sol_proc_t *t_procp;
	} sol_proc_t;

typedef sol_proc_t proc_t;
# define task_struct sol_proc_t
# define	curthread curproc
extern sol_proc_t	*curthread;

# define	td_tid	pid
# define	t_tid   td_tid
# define	comm t_proc->comm

int priv_policy(const cred_t *, int, int, int, const char *);
int priv_policy_only(const cred_t *, int, int);
int priv_policy_choice(const cred_t *, int, int);

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

char	*getenv(char *);
int	copyin(void *, void *, int);
int kill_pid(struct pid *pid, int sig, int priv);
//void	*kmem_cache_alloc(kmem_cache_t *cache, int flags);
void	*new_unr(struct unrhdr *uh, void **p1, void **p2);
void	*vmem_alloc(vmem_t *, size_t, int);
void	*vmem_zalloc(vmem_t *, size_t, int);
# define	bcopy(a, b, c) memcpy(b, a, c)
# define	bzero(a, b) memset(a, 0, b)
void	debug_enter(int);
void	dtrace_vtime_disable(void);
void	dtrace_vtime_enable(void);
void	freeenv(char *);
//void	kmem_cache_free(kmem_cache_t *, void *);
void	kmem_free(void *, int);
void	vmem_destroy(vmem_t *);
void	vmem_free(vmem_t *, void *, size_t);
void	*kmem_zalloc(size_t size, int kmflags);
void    mutex_exit(kmutex_t *);;

extern int panic_quiesce;
extern uintptr_t	_userlimit;
# if linux
#	define	cpu_get_id()	smp_processor_id()
# else
#	define	cpu_get_id()	CPU->cpu_id
# endif

# endif

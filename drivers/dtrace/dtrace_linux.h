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

#define	current	_current /* is a macro in <current.h> */
#define PRIV_EFFECTIVE          (1 << 0)
#define PRIV_DTRACE_KERNEL      (1 << 1)
#define PRIV_DTRACE_PROC        (1 << 2)
#define PRIV_DTRACE_USER        (1 << 3)
#define PRIV_PROC_OWNER         (1 << 4)
#define PRIV_PROC_ZONE          (1 << 5)
#define PRIV_ALL                ~0

#define LOCK_LEVEL      10

#define	makedevice	MKDEV
#define	getminor(x)	MINOR(x)
#define	minor(x)	MINOR(x)
#define	getmajor(x)	MAJOR(x)

#define	mutex_enter(x)	mutex_lock(x)

/*
typedef int	major_t;
typedef int	minor_t;
*/

/*
 * Macro for checking power of 2 address alignment.
 */
#define IS_P2ALIGNED(v, a) ((((uintptr_t)(v)) & ((uintptr_t)(a) - 1)) == 0)


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

	struct task_struct *t_proc;
	} sol_proc_t;

typedef sol_proc_t proc_t;
# define task_struct sol_proc_t
proc_t	*curthread;

# define	td_tid	pid
# define	t_tid   td_tid
# define	comm t_proc->comm

void	freeenv(char *);
void *kmem_cache_alloc(kmem_cache_t *cache, int flags);
int	copyin(void *, void *, int);
char	*getenv(char *);
void *vmem_alloc(vmem_t *, size_t, int);
void kmem_cache_free(kmem_cache_t *, void *);
void vmem_destroy(vmem_t *);
void	bzero(void *, int);
void	bcopy(void *, void *, int);

# endif

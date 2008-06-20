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
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

//#pragma ident	"@(#)systrace.c	1.6	06/09/19 SMI"

#include <dtrace_linux.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
# if __i386
#	include <asm-i386/unistd.h>
# else
#	include <asm/unistd_64.h>
# endif
#include <linux/sys.h>
#include <asm/asm-offsets.h>
#include <linux/miscdevice.h>
#include <linux/syscalls.h>
#include <sys/dtrace.h>
#include <sys/systrace.h>

# if defined(sun)
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/atomic.h>
# endif

#define	SYSTRACE_ARTIFICIAL_FRAMES	1

#define	SYSTRACE_SHIFT			16
#define	SYSTRACE_ISENTRY(x)		((int)(x) >> SYSTRACE_SHIFT)
#define	SYSTRACE_SYSNUM(x)		((int)(x) & ((1 << SYSTRACE_SHIFT) - 1))
#define	SYSTRACE_ENTRY(id)		((1 << SYSTRACE_SHIFT) | (id))
#define	SYSTRACE_RETURN(id)		(id)

# if !defined(__NR_syscall_max)
#	define NSYSCALL NR_syscalls
# else
#	define NSYSCALL __NR_syscall_max
# endif

#if ((1 << SYSTRACE_SHIFT) <= NSYSCALL)
#error 1 << SYSTRACE_SHIFT must exceed number of system calls
#endif

/**********************************************************************/
/*   Get a list of system call names here.			      */
/**********************************************************************/
static char *syscallnames[] = {

# undef _ASM_X86_64_UNISTD_H_
# undef __SYSCALL
# define __SYSCALL(nr, func) [nr] = #nr,
# undef __KERNEL_SYSCALLS_
# include <asm/unistd.h>

	};

struct sysent {
        asmlinkage int64_t         (*sy_callc)();  /* C-style call hander or wrapper */
};

#define LOADABLE_SYSCALL(s)     (s->sy_flags & SE_LOADABLE)
#define LOADED_SYSCALL(s)       (s->sy_flags & SE_LOADED)
#define SE_LOADABLE     0x08            /* syscall is loadable */
#define SE_LOADED       0x10            /* syscall is completely loaded */

systrace_sysent_t *systrace_sysent;

struct sysent *sysent;
static dtrace_provider_id_t systrace_id;

/**********************************************************************/
/*   This  needs to be defined in sysent.c - just need to figure out  */
/*   the equivalent in Linux...					      */
/**********************************************************************/
void	*fbt_get_sys_call_table(void);
void *dtrace_casptr(void *target, void *cmp, void *new);
# define	casptr(a, b, c) dtrace_casptr(a, b, c)
void (*systrace_probe)(dtrace_id_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t, uintptr_t);

# define linux_get_syscall() get_current()->thread.trap_no

asmlinkage int64_t
dtrace_systrace_syscall(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
    uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
#if defined(sun)
	int syscall = curthread->t_sysnum;
#else
	int syscall = linux_get_syscall();
#endif
        systrace_sysent_t *sy;
        dtrace_id_t id;
        int64_t rval;

{int i; 
long *ptr = &arg0;
//for (i = 0; i < 32; i++) {
//printk("stack[%d] = %p\n", i, ptr[i]);
//}
syscall = ptr[12]; // horrid hack
        sy = &systrace_sysent[syscall];
}
printk("syscall=%d %s current=%p syscall=%d\n", syscall, 
	syscall >= 0 && syscall < NSYSCALL ? syscallnames[syscall] : "dont-know", 
	get_current(), linux_get_syscall());
//printk("arg0=%s %p %p %p %p %p\n", arg0, arg1, arg2, arg3, arg4, arg5);
        if ((id = sy->stsy_entry) != DTRACE_IDNONE) {
                (*systrace_probe)(id, arg0, arg1, arg2, arg3, arg4, arg5);
	}

        /*
         * We want to explicitly allow DTrace consumers to stop a process
         * before it actually executes the meat of the syscall.
         */
	TODO();
# if defined(TODOxxx)
        {proc_t *p = ttoproc(curthread);
        mutex_enter(&p->p_lock);
        if (curthread->t_dtrace_stop && !curthread->t_lwp->lwp_nostop) {
                curthread->t_dtrace_stop = 0;
                stop(PR_REQUESTED, 0);
        }
        mutex_exit(&p->p_lock);
	}
# endif

        rval = (*sy->stsy_underlying)(arg0, arg1, arg2, arg3, arg4, arg5);

# if defined(TODOxxx)
        if (ttolwp(curthread)->lwp_errno != 0)
                rval = -1;
# endif

        if ((id = sy->stsy_return) != DTRACE_IDNONE) {
                (*systrace_probe)(id, (uintptr_t)rval, (uintptr_t)rval,
                    (uintptr_t)((int64_t)rval >> 32), 0, 0, 0);
		}

        return (rval);
}

static void
systrace_do_init(struct sysent *actual, systrace_sysent_t **interposed)
{
	systrace_sysent_t *sysent = *interposed;
	int i;

	if (sysent == NULL) {
		*interposed = sysent = kmem_zalloc(sizeof (systrace_sysent_t) *
		    NSYSCALL, KM_SLEEP);
	}

HERE();
printk("NSYSCALL=%d\n", NSYSCALL);
	for (i = 0; i < NSYSCALL; i++) {
		struct sysent *a = &actual[i];
		systrace_sysent_t *s = &sysent[i];

# if defined(sun)
		if (LOADABLE_SYSCALL(a) && !LOADED_SYSCALL(a))
			continue;
# endif

		if (a->sy_callc == dtrace_systrace_syscall)
			continue;

#ifdef _SYSCALL32_IMPL
		if (a->sy_callc == dtrace_systrace_syscall32)
			continue;
#endif

		s->stsy_underlying = a->sy_callc;
	}
}

/*ARGSUSED*/
static void
systrace_provide(void *arg, const dtrace_probedesc_t *desc)
{
	int i;
HERE();
printk("descr=%p\n", desc);
	if (desc != NULL)
		return;

	if (sysent == NULL)
		sysent = fbt_get_sys_call_table();

	systrace_do_init(sysent, &systrace_sysent);
HERE();
#ifdef _SYSCALL32_IMPL
	systrace_do_init(sysent32, &systrace_sysent32);
#endif
	for (i = 0; i < NSYSCALL; i++) {
		char	*name = syscallnames[i];

		if (name == NULL)
			continue;

		if (strncmp(name, "__NR_", 5) == 0)
			name += 5;

printk("i=%d %p %p\n", i, systrace_sysent[i].stsy_underlying, name);
//continue;
		if (systrace_sysent[i].stsy_underlying == NULL)
			continue;

HERE();
		if (dtrace_probe_lookup(systrace_id, NULL,
		    name, "entry") != 0)
			continue;

HERE();
		(void) dtrace_probe_create(systrace_id, NULL, name,
		    "entry", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_ENTRY(i)));
HERE();
		(void) dtrace_probe_create(systrace_id, NULL, name,
		    "return", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_RETURN(i)));

HERE();
		systrace_sysent[i].stsy_entry = DTRACE_IDNONE;
		systrace_sysent[i].stsy_return = DTRACE_IDNONE;
#ifdef _SYSCALL32_IMPL
		systrace_sysent32[i].stsy_entry = DTRACE_IDNONE;
		systrace_sysent32[i].stsy_return = DTRACE_IDNONE;
#endif
	}
}

/*ARGSUSED*/
static void
systrace_destroy(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);

	/*
	 * There's nothing to do here but assert that we have actually been
	 * disabled.
	 */
	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		ASSERT(systrace_sysent[sysnum].stsy_entry == DTRACE_IDNONE);
#ifdef _SYSCALL32_IMPL
		ASSERT(systrace_sysent32[sysnum].stsy_entry == DTRACE_IDNONE);
#endif
	} else {
		ASSERT(systrace_sysent[sysnum].stsy_return == DTRACE_IDNONE);
#ifdef _SYSCALL32_IMPL
		ASSERT(systrace_sysent32[sysnum].stsy_return == DTRACE_IDNONE);
#endif
	}
}

/*ARGSUSED*/
static void
systrace_enable(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);
	int enabled = (systrace_sysent[sysnum].stsy_entry != DTRACE_IDNONE ||
	    systrace_sysent[sysnum].stsy_return != DTRACE_IDNONE);

	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		systrace_sysent[sysnum].stsy_entry = id;
#ifdef _SYSCALL32_IMPL
		systrace_sysent32[sysnum].stsy_entry = id;
#endif
	} else {
		systrace_sysent[sysnum].stsy_return = id;
#ifdef _SYSCALL32_IMPL
		systrace_sysent32[sysnum].stsy_return = id;
#endif
	}

	if (enabled) {
		printk("panic: sysnum=%d callc=%p\n", sysnum, sysent[sysnum].sy_callc);
//		ASSERT(sysent[sysnum].sy_callc == dtrace_systrace_syscall);
		return;
	}

HERE();
printk("enable: sysnum=%d %p %p %p\n", sysnum,
	&sysent[sysnum].sy_callc,
	    (void *)systrace_sysent[sysnum].stsy_underlying,
	    (void *)dtrace_systrace_syscall);

	(void) casptr(&sysent[sysnum].sy_callc,
	    (void *)systrace_sysent[sysnum].stsy_underlying,
	    (void *)dtrace_systrace_syscall);
HERE();
#ifdef _SYSCALL32_IMPL
	(void) casptr(&sysent32[sysnum].sy_callc,
	    (void *)systrace_sysent32[sysnum].stsy_underlying,
	    (void *)dtrace_systrace_syscall32);
#endif
}

/*ARGSUSED*/
static void
systrace_disable(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);
	int disable = (systrace_sysent[sysnum].stsy_entry == DTRACE_IDNONE ||
	    systrace_sysent[sysnum].stsy_return == DTRACE_IDNONE);

	if (disable) {
		(void) casptr(&sysent[sysnum].sy_callc,
		    (void *)dtrace_systrace_syscall,
		    (void *)systrace_sysent[sysnum].stsy_underlying);

#ifdef _SYSCALL32_IMPL
		(void) casptr(&sysent32[sysnum].sy_callc,
		    (void *)dtrace_systrace_syscall32,
		    (void *)systrace_sysent32[sysnum].stsy_underlying);
#endif
	}

	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		systrace_sysent[sysnum].stsy_entry = DTRACE_IDNONE;
#ifdef _SYSCALL32_IMPL
		systrace_sysent32[sysnum].stsy_entry = DTRACE_IDNONE;
#endif
	} else {
		systrace_sysent[sysnum].stsy_return = DTRACE_IDNONE;
#ifdef _SYSCALL32_IMPL
		systrace_sysent32[sysnum].stsy_return = DTRACE_IDNONE;
#endif
	}
}
/*ARGSUSED*/
void
systrace_stub(dtrace_id_t id, uintptr_t arg0, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
}

static dtrace_pattr_t systrace_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

static dtrace_pops_t systrace_pops = {
	systrace_provide,
	NULL,
	systrace_enable,
	systrace_disable,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	systrace_destroy
};
static int initted;

static int
systrace_attach(void)
{

	systrace_probe = (void (*)(dtrace_id_t, uintptr_t arg0, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3, uintptr_t arg4))dtrace_probe;
	membar_enter();

	if (dtrace_register("syscall", &systrace_attr, DTRACE_PRIV_USER, NULL,
	    &systrace_pops, NULL, &systrace_id) != 0) {
		systrace_probe = systrace_stub;
		return DDI_FAILURE;
		}

	initted = 1;

	return (DDI_SUCCESS);
}
static int
systrace_detach(void)
{
	if (!initted)
		return DDI_SUCCESS;
TODO();

	if (dtrace_unregister(systrace_id) != 0)
		return (DDI_FAILURE);

	systrace_probe = systrace_stub;
	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
systrace_open(struct inode *inode, struct file *file)
{
	return (0);
}

static const struct file_operations systrace_fops = {
        .open = systrace_open,
};

static struct miscdevice systrace_dev = {
        MISC_DYNAMIC_MINOR,
        "systrace",
        &systrace_fops
};

static int initted;

int systrace_init(void)
{	int	ret;

	ret = misc_register(&systrace_dev);
	if (ret) {
		printk(KERN_WARNING "systrace: Unable to register misc device\n");
		return ret;
		}

	systrace_attach();

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "systrace loaded: /dev/systrace now available\n");

	initted = 1;

	return 0;
}
void systrace_exit(void)
{
HERE();
	systrace_detach();
HERE();

	printk(KERN_WARNING "systrace driver unloaded.\n");
	misc_deregister(&systrace_dev);
}

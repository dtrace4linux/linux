/**********************************************************************/
/*                                                                    */
/*  File:          prov_proc.c                                        */
/*  Author:        P. D. Fox                                          */
/*  Created:       6 Nov 2011                                         */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  Description:  proc::: provider callbacks                          */
/*--------------------------------------------------------------------*/
/*  $Header: Last edited: 15-Feb-2015 1.2 $ 			      */
/**********************************************************************/

#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include <dtrace_linux.h>
#include <sys/privregs.h>
#include <sys/dtrace_impl.h>
#include <linux/sys.h>
#undef comm /* For 2.6.36 and above - conflict with perf_event.h */
#include <sys/dtrace.h>
#include <dtrace_proto.h>
#include <sys/fault.h>
#include "ctf_struct.h"

static uintptr_t
psinfo_arg(int n, struct pt_regs *regs)
{	psinfo_t *ps = prcom_get_arg(n, sizeof *ps);

	ps->pr_pid = current->pid;
	ps->pr_pgid = current->tgid;
	ps->pr_ppid = current->parent ? current->parent->pid : 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	ps->pr_uid = KUIDT_VALUE(current->cred->uid);
	ps->pr_gid = KGIDT_VALUE(current->cred->gid);
	ps->pr_euid = KUIDT_VALUE(current->cred->euid);
	ps->pr_egid = KGIDT_VALUE(current->cred->egid);
#else
	ps->pr_uid = current->uid;
	ps->pr_gid = current->gid;
	ps->pr_euid = current->euid;
	ps->pr_egid = current->egid;
#endif
	ps->pr_addr = current;
	ps->pr_start = current->start_time;
	
	return (uintptr_t) ps;
}

/**********************************************************************/
/*   Probe handlers.						      */
/**********************************************************************/
static int
probe_proc_create(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, psinfo_arg(0, regs), 0, 0, 0, 0);
	return FALSE;
}
static int
probe_proc_fault(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, FLTPAGE, 0, 0, 0, 0);
	return FALSE;
}
static int
probe_proc_ill(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, FLTILL, 0, 0, 0, 0);
	return FALSE;
}
static int
probe_proc_int3(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, FLTBPT, 0, 0, 0, 0);
	return FALSE;
}
static int
probe_proc_access(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, FLTACCESS, 0, 0, 0, 0);
	return FALSE;
}
static int
probe_proc_iovf(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, FLTIOVF, 0, 0, 0, 0);
	return FALSE;
}
static int
probe_proc_izdiv(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, FLTIZDIV, 0, 0, 0, 0);
	return FALSE;
}
static int
probe_proc_debug(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, FLTTRACE, 0, 0, 0, 0);
	return FALSE;
}
static int
probe_proc_bounds(dtrace_id_t id, struct pt_regs *regs)
{
	dtrace_probe(id, FLTBOUNDS, 0, 0, 0, 0);
	return FALSE;
}

void
prov_proc_init(void)
{
	prcom_add_callback("proc:::create", "wake_up_new_task", probe_proc_create);

	prcom_add_callback("proc:::fault", "do_page_fault", probe_proc_fault);
	prcom_add_callback("proc:::fault", "do_invalid_op", probe_proc_ill);
	prcom_add_callback("proc:::fault", "do_int3", probe_proc_int3);
	prcom_add_callback("proc:::fault", "do_general_protection", probe_proc_access);
	prcom_add_callback("proc:::fault", "do_overflow", probe_proc_iovf);
	prcom_add_callback("proc:::fault", "do_divide_error", probe_proc_izdiv);
	prcom_add_callback("proc:::fault", "do_debug", probe_proc_debug);
	prcom_add_callback("proc:::fault", "do_bounds", probe_proc_bounds);
}

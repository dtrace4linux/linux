/**********************************************************************/
/*   Notifier probe -- allow probes on specific notifier chains.      */
/*   								      */
/*   Author: Paul D. Fox					      */
/*   $Header: Last edited: 11-Jun-2011 1.2 $ 			      */
/**********************************************************************/

#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <linux/miscdevice.h>
#include "dtrace_proto.h"

#define regs pt_regs
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/privregs.h>

static dtrace_provider_id_t id;

typedef struct provider {
        char                    *p_module;     /* name of provider */
        char                    *p_function;   /* prefix for probe names */
	void			*p_addr;
	dtrace_id_t		p_id;
	dtrace_id_t		p_id2;
	} provider_t;
static provider_t map[] = {
	{"atomic", "die_chain"},
	{"atomic", "hci_notifier"},
	{"atomic", "idle_notifier"},
	{"atomic", "inet6addr_chain"},
	{"atomic", "keyboard_notifier_list"},
	{"atomic", "mn10300_die_chain"},
	{"atomic", "netevent_notif_chain"},
	{"atomic", "netlink_chain"},
	{"atomic", "panic_notifier_list"},
	{"atomic", "task_free_notifier"},
	{"atomic", "thread_notify_head"},
	{"atomic", "vt_notifier_list"},
	{"atomic", "xaction_notifier_list"},
	{"blocking", "acpi_bus_notify_list"},
	{"blocking", "acpi_chain_head"},
	{"blocking", "acpi_lid_notifier"},
	{"blocking", "adb_client_list"},
	{"blocking", "cpu_dma_lat_notifier"},
	{"blocking", "cpufreq_policy_notifier_list"},
	{"blocking", "crypto_chain"},
	{"blocking", "dca_provider_chain"},
	{"blocking", "dnaddr_chain"},
	{"blocking", "fb_notifier_list"},
	{"blocking", "inetaddr_chain"},
	{"blocking", "ipcns_chain"},
	{"blocking", "memory_chain"},
	{"blocking", "module_notify_list"},
	{"blocking", "munmap_notifier"},
	{"blocking", "network_lat_notifier"},
	{"blocking", "network_throughput_notifier"},
	{"blocking", "oom_notify_list"},
	{"blocking", "pSeries_reconfig_chain"},
	{"blocking", "pm_chain_head"},
	{"blocking", "reboot_notifier_list"},
	{"blocking", "spu_switch_notifier"},
	{"blocking", "task_exit_notifier"},
	{"blocking", "ubi_notifiers"},
	{"blocking", "usb_notifier_list"},
	{"blocking", "wf_client_list"},
	{"blocking", "xenstore_chain"},
	{"raw", "clockevents_chain"},
	{"raw", "cpu_chain"},
	{"raw", "netdev_chain"},
	{NULL, NULL}
	};

static int enabled;
static void (*fn_notifier_call_chain)(void);
static int patchval;
static int inslen;
static int modrm;

/*ARGSUSED*/
static int
notifier_invop(uintptr_t addr, uintptr_t *stack, uintptr_t eax, trap_instr_t *tinfo)
{
	uintptr_t stack0, stack1, stack2, stack3, stack4, n, n1, n2;
	struct pt_regs *regs;
	int	i;

	if (!enabled || addr != (uintptr_t) fn_notifier_call_chain)
		return 0;

	tinfo->t_opcode = patchval;
	tinfo->t_inslen = inslen;
	tinfo->t_modrm = modrm;
	/***********************************************/
	/*   Dont fire probe if this is unsafe.	       */
	/***********************************************/
	if (!tinfo->t_doprobe)
		return (DTRACE_INVOP_NOP);
	/*
	 * When accessing the arguments on the stack, we must
	 * protect against accessing beyond the stack.  We can
	 * safely set NOFAULT here -- we know that interrupts
	 * are already disabled.
	 */
	regs = (struct pt_regs *) stack;
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	stack0 = regs->c_arg0;
	stack1 = regs->c_arg1;
	stack2 = regs->c_arg2;
	stack3 = regs->c_arg3;
	stack4 = regs->c_arg4;
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT | CPU_DTRACE_BADADDR);

	/***********************************************/
	/*   stack0  is ptr to the head of the chain,  */
	/*   but  theres  a  spinlock before this, so  */
	/*   adjust the value.			       */
	/***********************************************/
	n = stack0 - offsetof(struct atomic_notifier_head, head);
	n1 = stack0 - offsetof(struct blocking_notifier_head, head);
	n2 = stack0 - offsetof(struct raw_notifier_head, head);
	for (i = 0; map[i].p_module; i++) {
		if (map[i].p_addr == n ||
		    map[i].p_addr == n1 ||
		    map[i].p_addr == n2
		    )
			break;
		}

	/***********************************************/
	/*   Dont  do this for the return probe - the  */
	/*   arguments  are  going  to be junk and we  */
	/*   will  hang/panic  the  kernel.  At  some  */
	/*   point  we  need  something better than a  */
	/*   entry/return  indicator -- maybe an enum  */
	/*   type.				       */
	/***********************************************/
//printk("probe %p: %p %p %p %p %p\n", &addr, stack0, stack1, stack2, stack3, stack4);
	if (map[i].p_module) {
		dtrace_probe(map[i].p_id, stack0, stack0, stack0, 0, 0);
	} else {
		static int cnt;
		if (cnt++ < 50)
			printk("dtrace: cannot map notifier %p\n", stack0);
	}

	return (DTRACE_INVOP_NOP);
}

/*ARGSUSED*/
static void
notifier_provide(void *arg, const dtrace_probedesc_t *desc)
{
//	TODO();
}

/*ARGSUSED*/
static void
notifier_destroy(void *arg, dtrace_id_t id, void *parg)
{
}

/*ARGSUSED*/
static int
notifier_enable(void *arg, dtrace_id_t id, void *parg)
{	provider_t *pp = (provider_t *) parg;

	if (fn_notifier_call_chain == NULL) {
		fn_notifier_call_chain = get_proc_addr("notifier_call_chain");
		if (fn_notifier_call_chain &&
		    memory_set_rw(fn_notifier_call_chain, 1, TRUE)) {
			patchval = *(unsigned char *) fn_notifier_call_chain;
			inslen = dtrace_instr_size_modrm((uchar_t *) fn_notifier_call_chain, &modrm);
		} else
			fn_notifier_call_chain = NULL;
	}
	if (fn_notifier_call_chain == NULL)
		return 0;

	if (pp->p_addr == NULL)
		pp->p_addr = get_proc_addr(pp->p_function);

	enabled = TRUE;
	if (*(unsigned char *) fn_notifier_call_chain != PATCHVAL) {
		*(unsigned char *) fn_notifier_call_chain = PATCHVAL;
	}

	return 0;
}

/*ARGSUSED*/
static void
notifier_disable(void *arg, dtrace_id_t id, void *parg)
{
	enabled = FALSE;
	if (fn_notifier_call_chain && patchval &&
	    *(unsigned char *) fn_notifier_call_chain == PATCHVAL) {
		*(unsigned char *) fn_notifier_call_chain = patchval;
	}
}

/*ARGSUSED*/
static uint64_t
notifier_getarg(void *arg, dtrace_id_t id, void *parg, int argno, int aframes)
{
	uintptr_t val;
	struct frame *fp = (struct frame *)dtrace_getfp();
	uintptr_t *stack = NULL;
	int i;
#if defined(__amd64)
	/*
	 * A total of 6 arguments are passed via registers; any argument with
	 * index of 5 or lower is therefore in a register.
	 */
	int inreg = 5;
#endif

	for (i = 1; i <= aframes; i++) {
//printk("i=%d fp=%p aframes=%d pc:%p\n", i, fp, aframes, fp->fr_savpc);
		fp = (struct frame *)(fp->fr_savfp);
#if defined(linux)
		/***********************************************/
		/*   Temporary hack - not sure which stack we  */
		/*   have here and it is faultiing us.	       */
		/***********************************************/
		if (fp == NULL)
			return 0;
#endif

		if (fp->fr_savpc == (pc_t)dtrace_invop_callsite) {
#if !defined(__amd64)
			/*
			 * If we pass through the invalid op handler, we will
			 * use the pointer that it passed to the stack as the
			 * second argument to dtrace_invop() as the pointer to
			 * the stack.
			 */
			stack = ((uintptr_t **)&fp[1])[1];
#else
			/*
			 * In the case of amd64, we will use the pointer to the
			 * regs structure that was pushed when we took the
			 * trap.  To get this structure, we must increment
			 * beyond the frame structure.  If the argument that
			 * we're seeking is passed on the stack, we'll pull
			 * the true stack pointer out of the saved registers
			 * and decrement our argument by the number of
			 * arguments passed in registers; if the argument
			 * we're seeking is passed in regsiters, we can just
			 * load it directly.
			 */
			struct regs *rp = (struct regs *)((uintptr_t)&fp[1] +
			    sizeof (uintptr_t));

			if (argno <= inreg) {
				stack = (uintptr_t *)&rp->r_rdi;
			} else {
				stack = (uintptr_t *)(rp->r_rsp);
				argno -= (inreg + 1);
			}
#endif
			goto load;
		}
	}
//printk("stack %d: %p %p %p %p\n", i, ((long *) fp)[0], ((long *) fp)[1], ((long *) fp)[2], ((long *) fp)[3], ((long *) fp)[4]);

	/*
	 * We know that we did not come through a trap to get into
	 * dtrace_probe() -- the provider simply called dtrace_probe()
	 * directly.  As this is the case, we need to shift the argument
	 * that we're looking for:  the probe ID is the first argument to
	 * dtrace_probe(), so the argument n will actually be found where
	 * one would expect to find argument (n + 1).
	 */
	argno++;

#if defined(__amd64)
	if (argno <= inreg) {
		/*
		 * This shouldn't happen.  If the argument is passed in a
		 * register then it should have been, well, passed in a
		 * register...
		 */
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	}

	argno -= (inreg + 1);
#endif
	stack = (uintptr_t *)&fp[1];

load:
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	val = stack[argno];
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);
printk("notifier_getarg#%d: %lx aframes=%d\n", argno, val, aframes);

	return (val);
}

static void
notifier_getargdesc(void *arg, dtrace_id_t id, void *parg, dtrace_argdesc_t *desc)
{
}

static dtrace_pops_t notifier_pops = {
	notifier_provide,
	NULL,
	notifier_enable,
	notifier_disable,
	NULL,
	NULL,
	NULL, //	notifier_getargdesc,
	NULL, //	notifier_getarg,
	NULL,
	notifier_destroy
};

static dtrace_pattr_t pattr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_UNSTABLE, DTRACE_STABILITY_UNSTABLE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
};

/*ARGSUSED*/
static int
notifier_attach(void)
{	provider_t	*pp;

	dtrace_invop_add(notifier_invop);

	if (dtrace_register("notifier", &pattr,
	    DTRACE_PRIV_KERNEL, NULL,
	    &notifier_pops, NULL, &id) != 0) {
	    	printk("notifier_attach: Failed to register 'notifier' provider\n");
		return (DDI_FAILURE);
	}

	for (pp = map; pp->p_module; pp++) {
		pp->p_id = dtrace_probe_create(id,
		    pp->p_module, pp->p_function, "entry", 0, pp);
		pp->p_id2 = dtrace_probe_create(id,
		    pp->p_module, pp->p_function, "return", 0, pp);
	}

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
notifier_detach(void)
{
	if (fn_notifier_call_chain && patchval &&
	    *(unsigned char *) fn_notifier_call_chain == PATCHVAL) {
		*(unsigned char *) fn_notifier_call_chain = patchval;
	}

	if (id && dtrace_unregister(id) != 0)
		return (DDI_FAILURE);

	dtrace_invop_remove(notifier_invop);

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
notifier_open(struct inode *inode, struct file *file)
{
	return (0);
}

static const struct file_operations notifier_fops = {
	.owner = THIS_MODULE,
        .open = notifier_open,
};

#if 0
static struct miscdevice notifier_dev = {
        MISC_DYNAMIC_MINOR,
        "notifier",
        &notifier_fops
};
#endif

static int initted;

int dtrace_notifier_init(void)
{

#if 0
	int ret = misc_register(&notifier_dev);
	if (ret) {
		printk(KERN_WARNING "notifier: Unable to register misc device\n");
		return ret;
		}
	dtrace_printf("notifier loaded: /dev/notifier now available\n");
#endif

	printk(KERN_WARNING "dtrace: notifier provider being initialised\n");
	notifier_attach();

	initted = 1;

	return 0;
}
void dtrace_notifier_exit(void)
{
	if (initted) {
		notifier_detach();
#if 0
		misc_deregister(&notifier_dev);
#endif
	}

/*	printk(KERN_WARNING "notifier driver unloaded.\n");*/
}


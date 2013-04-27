/**********************************************************************/
/*   File  containing  a list of toxic functions which we must never  */
/*   probe.  Ideally  we need the children of these functions too to  */
/*   avoid touching something in a probe which we depend upon.	      */
/*   								      */
/*   Date: March 2009						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/*   								      */
/*  $Header: Last edited: 31-Jan-2012 1.3 $ 			      */
/**********************************************************************/

/**********************************************************************/
/*   We  avoid  #include  dependencies here..because I want to avoid  */
/*   inlining  or  calling renamed functions. We need to shut up GCC  */
/*   because it can see what we are doing.			      */
/**********************************************************************/
# include <linux/string.h>
# include <linux/version.h>

/**********************************************************************/
/*   We  avoiding  having  issues  with  prototypes  by stringifying  */
/*   everything.  Also  we avoid issues where the kernel has no such  */
/*   symbol  --  we  just  avoid fbt_provide_kernel() from trying to  */
/*   touch any of these babies. They cry if we touch them.	      */
/*   								      */
/*   Easiest way to find functions is to take output of 'dtrace -l',  */
/*   massage into a set of probes, and use binary search to find the  */
/*   offending probe, e.g.					      */
/*   								      */
/*   ...							      */
/*   fbt:kernel:__raw_notifier_call_chain:entry {}		      */
/*   fbt:kernel:raw_notifier_call_chain:entry {}		      */
/*   fbt:kernel:raw_notifier_call_chain:return {}		      */
/*   ...							      */
/*   								      */
/*   These  functions  are mostly on the toxic list because they are  */
/*   called,  or  they  call us, from the int3 interrupt handler. We  */
/*   can reduce some of these by patching the interrupt vector table  */
/*   direct,  and  by implementing private copies of functions where  */
/*   possible.							      */
/*   								      */
/*   Some of these can be removed - the initial list was gotten from  */
/*   looking at dtracedrv.ko's dependencies.			      */
/*   								      */
/*   I've  also found earlier kernels dont have re-entrant INT3/INT1  */
/*   trap  handlers,  so  we have to blacklist a few extra functions  */
/*   for them.							      */
/**********************************************************************/
char toxic_probe_tbl[] = {
# if defined(__arm__)
	/***********************************************/
	/*   FBT  leverages  the register_undef_hook,  */
	/*   so need to skip this.		       */
	/***********************************************/
	"do_undefinstr "
	"v6_pabort "
	"v6_early_abort "
	"vector_swi "
	"call_fpe " // entry-armv.S determine if an FP co-processor instruction
	"do_translation_fault " // nested trap?
	"do_DataAbort " 	// Needed as part of implementation
	"do_PrefetchAbort " 	// Needed as part of implementation
	"__und_invalid " 	// Needed as part of implementation
	"__und_fault "		// Needed as part of implementation
	"__und_svc "		// Needed as part of implementation
	"__und_usr "		// Needed as part of implementation
	"__und_usr_thumb "	// Needed as part of implementation
	"__dabt_svc "		// Needed as part of implementation
	"__dabt_invalid "	// Needed as part of implementation
	"__und_svc_finish:return "
	"__pabt_svc "		// :return probe
	"__irq_svc:return "	// clrex happens just before this, so need to be careful
# endif

	"__atomic_notifier_call_chain "
	"__kmalloc "
	"__kprobes_text_start"
	"__mod_timer "
	"__show_registers "
//	"__mutex_init "
//	"__mutex_lock_interruptible_slowpath "
//	"__mutex_lock_killable_slowpath "
//	"__mutex_lock_slowpath "
//	"__mutex_trylock_slowpath "
//	"__mutex_unlock_slowpath "
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
	"__tasklet_hi_schedule "
	"__tasklet_schedule "
        "kprobe_fault_handler kprobe_handler kprobe_flush_task post_kprobe_handler "
	"scheduler_tick "
#endif
	"atomic_notifier_call_chain " // [ind] used by us registering INT3 handler
	"common_interrupt "
	"copy_from_user "
	"copy_to_user "
	"del_timer "
	"do_debug "
	"do_oops_enter_exit "
	"do_timer "
	"do_trap "
	"dump_task_regs "
	"dump_trace "
	"do_int3 " // Used by us for probes
	"find_task_by_vpid "
	"get_debugreg "
	"get_kprobe "
	"ia32_sysenter_target "	// In x86-64 kernel, entry point for ia32 SYSENTER
	"init_timer "
	"int3 "
	"iret_exc "
	"kfree "
	"kmem_cache_alloc "
	"kmem_cache_create "
	"kmem_cache_destroy "
	"kmem_cache_free "
	"level3_kernel_pgt " // this is a data element in the code segment
	"level2_kernel_pgt " // this is a data element in the code segment
//	"mutex_lock "
//	"mutex_unlock "
	"native_get_debugreg "
	"on_each_cpu " 		// Needed by dtrace_xcall
	"oops_exit "
	"oops_may_print "
	"panic "
	"paranoid_exit "
	"per_cpu__cpu_number "
	"per_cpu__current_task "
	"printk "
	"read_tsc "
	"save_paranoid "
	"send_sig "
	"send_sig_info "
	"show_registers "
	"show_regs "
	"smp_call_function "
	"smp_call_function_single "
	"sysenter_past_esp "
	"system_call "
	"system_call_after_swapgs "
	"vfree "
	"vmalloc "
	"vprintk "
	"xen_int3 "
	"xen_debug "
	"zlib_inflate "
	"zlib_inflateEnd "
	"zlib_inflateInit2 "
	};

int 
is_toxic_func(unsigned long a, const char *name)
{	char	*cp;
	int	len = strlen((char *) name);

	for (cp = toxic_probe_tbl; *cp; ) {
		if (strncmp(cp, (char *) name, len) == 0 &&
		    cp[len] == ' ')
		    	return 1;
		while (*cp && *cp != ' ')
			cp++;
		while (*cp == ' ')
			cp++;
	}
	return 0;
}

/**********************************************************************/
/*   For ARM, there are some :return probes we mustnt intercept.      */
/**********************************************************************/
int 
is_toxic_return(const char *name)
{	char	*cp;
	int	len = strlen((char *) name);

	for (cp = toxic_probe_tbl; *cp; ) {
		if (*cp == *name &&
		    strncmp(cp, (char *) name, len) == 0 &&
		    cp[len] == ':')
		    	return 1;
		while (*cp && *cp != ' ')
			cp++;
		while (*cp == ' ')
			cp++;
	}
	return 0;
}


/**********************************************************************/
/*   File  containing  a list of toxic functions which we must never  */
/*   probe.  Ideally  we need the children of these functions too to  */
/*   avoid touching something in a probe which we depend upon.	      */
/*   								      */
/*   Date: March 2009						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/**********************************************************************/

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
/**********************************************************************/
char toxic_probe_tbl[] = {
	"__atomic_notifier_call_chain "
	"__kmalloc "
	"__mod_timer "
	"__show_registers "
	"__mutex_init "
	"__mutex_lock_interruptible_slowpath "
	"__mutex_lock_killable_slowpath "
	"__mutex_lock_slowpath "
	"__mutex_trylock_slowpath "
	"__mutex_unlock_slowpath "
	"atomic_notifier_call_chain " // [ind] used by us registering INT3 handler
	"common_interrupt "
	"copy_from_user "
	"copy_to_user "
	"del_timer "
	"do_gettimeofday "
	"do_oops_enter_exit "
	"dump_task_regs "
	"dump_trace "
	"do_int3 " // Used by us for probes
	"find_task_by_vpid "
	"get_kprobe "
	"init_timer "
	"int3 "
	"iret_exc "
	"kfree "
	"kmalloc_caches "
	"kmem_cache_alloc "
	"kmem_cache_create "
	"kmem_cache_destroy "
	"kmem_cache_free "
	"mutex_lock "
	"mutex_unlock "
	"notify_die "
	/***********************************************/
	/*   We  need  to  patch the interrupt vector  */
	/*   direct  to allow us to avoid this, since  */
	/*   we  are  called  by the first level trap  */
	/*   handler from the following functions.     */
	/***********************************************/
	"notifier_call_chain "
	"on_each_cpu " 		// Needed by dtrace_xcall
	"oops_exit "
	"oops_may_print "
	"panic "
	"per_cpu__cpu_number "
	"per_cpu__current_task "
	"printk "
	"read_tsc "
	"send_sig "
	"send_sig_info "
	"show_registers "
	"show_regs "
	"smp_call_function_single "
	"sysenter_past_esp "
	"vfree "
	"vmalloc "
	"vprintk "
	"zlib_inflate "
	"zlib_inflateEnd "
	"zlib_inflateInit2 "
	};

int strlen(const char *);
char	*strncmp(const char *, const char *, int);

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


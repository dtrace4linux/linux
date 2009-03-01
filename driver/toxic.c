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
/**********************************************************************/
char toxic_probe_tbl[] = {
	"__atomic_notifier_call_chain "
	"atomic_notifier_call_chain "
	"__kmalloc "
	"__mod_timer "
	"copy_from_user "
	"copy_to_user "
	"del_timer "
	"do_gettimeofday "
	"dump_trace "
	"find_task_by_vpid "
	"get_kprobe "
	"init_timer "
	"kfree "
	"kmalloc_caches "
	"kmem_cache_alloc "
	"kmem_cache_create "
	"kmem_cache_destroy "
	"kmem_cache_free "
	"memcpy "
	"memset "
	"mutex_lock "
	"mutex_unlock "
	"notify_die "
	"on_each_cpu "
	"oops_may_print "
	"oops_exit "
	"do_oops_enter_exit "
	"panic "
	"per_cpu__cpu_number "
	"per_cpu__current_task "
	"printk "
	"read_tsc "
	"send_sig "
	"send_sig_info "
	"simple_strtoul "
	"smp_call_function_single "
	"snprintf "
	"sort "
	"sprintf "
	"strcasecmp "
	"strcat "
	"strchr "
	"strcmp "
	"strcpy "
	"strlen "
	"strncmp "
	"strncpy "
	"strpbrk "
	"strstr "
	"vfree "
	"vmalloc "
	"vprintk "
	"zlib_inflate "
	"zlib_inflateEnd "
	"zlib_inflateInit2 "
	};

int 	strlen(char *);
char	*strncmp(char *, char *, int);

int 
is_toxic_func(unsigned long a, char *name)
{	char	*cp;
	int	len = strlen(name);

	for (cp = toxic_probe_tbl; *cp; ) {
		if (strncmp(cp, name, len) == 0 &&
		    cp[len] == ' ')
		    	return 1;
		while (*cp && *cp != ' ')
			cp++;
		while (*cp == ' ')
			cp++;
	}
	return 0;
}


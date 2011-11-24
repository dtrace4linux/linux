/**********************************************************************/
/*   Common  provider  code.  Linux/dtrace  works  by  not modifying  */
/*   kernel  source.  To support the many SDT and other providers in  */
/*   the  kernel,  we need to emulate by intercepting certain kernel  */
/*   functions. This file provides a common approach to intercepting  */
/*   these functions and exposing them as Solaris-like providers.     */
/*   								      */
/*   Date: July 2010						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: GPL3						      */
/*   								      */
/*   $Header: Last edited: 04-Apr-2011 1.11 $ 			      */
/**********************************************************************/

#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <dtrace_proto.h>
#include <linux/miscdevice.h>
#include <linux/kallsyms.h>

#define regs pt_regs
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/privregs.h>

extern int dtrace_unhandled;
extern int fbt_name_opcodes;

# define MAX_PROVIDER	32
# define MAX_MODULE	32
# define MAX_FUNCTION	64
# define MAX_NAME	64

/**********************************************************************/
/*   Structure  describing  the  probes  we  create,  and the dtrace  */
/*   housekeeping  against  them.  These  are  all probes utilisiing  */
/*   breakpoints  to  trap them, but the interpretation is dependent  */
/*   on  the  address or arguments. Many of the providers will share  */
/*   the same underlying function.				      */
/**********************************************************************/
typedef struct provider {
	char			*p_probe;

	char			*p_func_name;  /* Set for function based probes. */
	void			*p_instr_addr; /* Set for non-function based probe. */
	void			*p_func_addr;
	char			*p_arg0_name;

	char			*p_arg0;
	char			*p_arg1;
	void			*p_arg_addr;
	int			p_stack0_cond;
	int			p_stack0_offset;
	uintptr_t		p_stack0_addr;
	int			p_enabled;
	dtrace_id_t		p_id;
	dtrace_id_t		p_id2;
	int			p_patchval;
	int			p_inslen;
	int			p_modrm;
	dtrace_provider_id_t 	p_provider_id;
	int			p_want_return;
	} provider_t;
#define MAX_PROVIDER_TBL 1024
static provider_t map[MAX_PROVIDER_TBL] = {
	{
		.p_probe = "notifier::atomic:die_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:hci_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:idle_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:inet6addr_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:keyboard_notifier_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:mn10300_die_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:netevent_notif_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:panic_notifier_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:task_free_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:thread_notify_head",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:vt_notifier_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::atomic:xaction_notifier_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:acpi_bus_notify_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:acpi_chain_head",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:acpi_lid_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:adb_client_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:cpu_dma_lat_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:cpufreq_policy_notifier_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:crypto_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:dca_provider_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:dnaddr_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:fb_notifier_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:inetaddr_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:ipcns_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:memory_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:module_notify_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:munmap_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:network_lat_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:network_throughput_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:oom_notify_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:pSeries_reconfig_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:pm_chain_head",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:reboot_notifier_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:spu_switch_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:task_exit_notifier",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:ubi_notifiers",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:usb_notifier_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:wf_client_list",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::blocking:xenstore_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::raw:clockevents_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::raw:cpu_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "notifier::raw:netdev_chain",
		.p_func_name = "notifier_call_chain",
	},
	{
		.p_probe = "sched:::off-cpu",
		.p_func_name = "perf_event_task_sched_out",
		.p_arg0 = "lwpsinfo_t *",
		.p_arg1 = "psinfo_t *",
	},
	{
		.p_probe = "sched:::on-cpu",
		.p_func_name = "perf_event_task_sched_in",
	},
	{NULL}
	};
static int probe_cnt;

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
static	const char *(*my_kallsyms_lookup)(unsigned long addr,
                        unsigned long *symbolsize,
                        unsigned long *offset,
                        char **modname, char *namebuf);
uint64_t prcom_getarg(void *arg, dtrace_id_t id, void *parg, int argno, int aframes);
void tcp_init(void);
void vminfo_init(void);

/**********************************************************************/
/*   Find a function and compute the size of the function, so we can  */
/*   disassemble it.						      */
/**********************************************************************/
int
dtrace_function_size(char *name, uint8_t **start, int *size)
{
	unsigned long addr;
	unsigned long symsize = 0;
	unsigned long offset;
	char	*modname = NULL;
	char	namebuf[KSYM_NAME_LEN];

	if ((addr = (unsigned long) get_proc_addr(name)) == 0)
		return 0;

	if (my_kallsyms_lookup == NULL) {
		my_kallsyms_lookup = get_proc_addr("kallsyms_lookup");
		}

	my_kallsyms_lookup(addr, &symsize, &offset, &modname, namebuf);
	printk("dtrace_function_size: %s %p size=%lu\n", name, (void *) addr, symsize);
	*size = symsize;
	*start = (uint8_t *) addr;
	return 1;
}
/**********************************************************************/
/*   Function  to  parse a function - so we can create the probes on  */
/*   the  start  and  (multiple) return exits. We call this from FBT  */
/*   and  SDT. We do this so we can avoid the very delicate function  */
/*   parser  code  being  implemented  in  two or more locations for  */
/*   these providers.						      */
/**********************************************************************/
void
dtrace_parse_function(pf_info_t *infp, uint8_t *instr, uint8_t *limit)
{
	int	do_print = FALSE;
	int	invop = 0;
	int	size;
	int	modrm;

	/***********************************************/
	/*   Dont  let  us  register  anything on the  */
	/*   notifier list(s) we are on, else we will  */
	/*   have recursion trouble.		       */
	/***********************************************/
	if (on_notifier_list(instr)) {
		printk("fbt_provide_function: Skip %s:%s - on notifier_chain\n",
			infp->modname, infp->name);
		return;
	}

# define UNHANDLED_FBT() if (infp->do_print || dtrace_unhandled) { \
		printk("fbt:unhandled instr %s:%p %02x %02x %02x %02x\n", \
			infp->name, instr, instr[0], instr[1], instr[2], instr[3]); \
			}

	/***********************************************/
	/*   Make sure we dont try and handle data or  */
	/*   bad instructions.			       */
	/***********************************************/
	if ((size = dtrace_instr_size_modrm(instr, &modrm)) <= 0)
		return;

	infp->retptr = NULL;
	/***********************************************/
	/*   Allow  us  to  work on a single function  */
	/*   for  debugging/tracing  else we generate  */
	/*   too  much  printk() output and swamp the  */
	/*   log daemon.			       */
	/***********************************************/
//	do_print = strncmp(infp->name, "vfs_read", 9) == 0;

#ifdef __amd64
// am not happy with 0xe8 CALLR instructions, so disable them for now.
if (*instr == 0xe8) return;
	invop = DTRACE_INVOP_ANY;
	switch (instr[0] & 0xf0) {
	  case 0x00:
	  case 0x10:
	  case 0x20:
	  case 0x30:
	  	break;

	  case 0x40:
	  	if (instr[0] == 0x48 && instr[1] == 0xcf)
			return;
	  	break;

	  case 0x50:
	  case 0x60:
	  case 0x70:
	  case 0x80:
	  case 0x90:
	  case 0xa0:
	  case 0xb0:
		break;
	  case 0xc0:
	  	/***********************************************/
	  	/*   We  cannot  single step an IRET for now,  */
	  	/*   so just ditch them as potential probes.   */
	  	/***********************************************/
	  	if (instr[0] == 0xcf)
			return;
		break;
	  case 0xd0:
	  case 0xe0:
		break;
	  case 0xf0:
	  	/***********************************************/
	  	/*   This  doesnt  work - if we single step a  */
	  	/*   HLT, well, the kernel doesnt really like  */
	  	/*   it.				       */
	  	/***********************************************/
	  	if (instr[0] == 0xf4)
			return;

//		printk("fbt:F instr %s:%p size=%d %02x %02x %02x %02x %02x %02x\n", name, instr, size, instr[0], instr[1], instr[2], instr[3], instr[4], instr[5]);
		break;
	  }
#else
	/***********************************************/
	/*   GCC    generates   lots   of   different  */
	/*   assembler  functions plus we have inline  */
	/*   assembler  to  deal  with.  We break the  */
	/*   opcodes  down  into groups, which helped  */
	/*   during  debugging,  or  if  we  need  to  */
	/*   single  out  specific  opcodes,  but for  */
	/*   now,   we  can  pretty  much  allow  all  */
	/*   opcodes.  (Apart  from HLT - which I may  */
	/*   get around to fixing).		       */
	/***********************************************/
	invop = DTRACE_INVOP_ANY;
	switch (instr[0] & 0xf0) {
	  case 0x00:
	  case 0x10:
	  case 0x20:
	  case 0x30:
	  case 0x40:
	  case 0x50:
	  case 0x60:
	  case 0x70:
	  case 0x80:
	  case 0x90:
	  case 0xa0:
	  case 0xb0:
		break;
	  case 0xc0:
	  	/***********************************************/
	  	/*   We  cannot  single step an IRET for now,  */
	  	/*   so just ditch them as potential probes.   */
	  	/***********************************************/
	  	if (instr[0] == 0xcf)
			return;
		break;
	  case 0xd0:
	  case 0xe0:
		break;
	  case 0xf0:
	  	/***********************************************/
	  	/*   This  doesnt  work - if we single step a  */
	  	/*   HLT, well, the kernel doesnt really like  */
	  	/*   it.				       */
	  	/***********************************************/
	  	if (instr[0] == 0xf4)
			return;

//		printk("fbt:F instr %s:%p size=%d %02x %02x %02x %02x %02x %02x\n", name, instr, size, instr[0], instr[1], instr[2], instr[3], instr[4], instr[5]);
		break;
	  }
#endif
	/***********************************************/
	/*   Handle  fact we may not be ready to cope  */
	/*   with this instruction yet.		       */
	/***********************************************/
	if (invop == 0) {
		UNHANDLED_FBT();
		return;
	}

	if ((*infp->func_entry)(infp, instr, size, modrm) == 0)
		return;

	/***********************************************/
	/*   Special code here for debugging - we can  */
	/*   make  the  name of a probe a function of  */
	/*   the  instruction, so we can validate and  */
	/*   binary search to find faulty instruction  */
	/*   stepping code in cpu_x86.c		       */
	/***********************************************/
	if (fbt_name_opcodes) {
		static char buf[128];
		if (fbt_name_opcodes == 2)
			snprintf(buf, sizeof buf, "x%02x%02x_%s", *instr, instr[1], infp->name);
		else
			snprintf(buf, sizeof buf, "x%02x_%s", *instr, infp->name);
		infp->name = buf;
	}

	if (do_print)
		printk("%d:alloc entry-patchpoint: %s %p sz=%d %02x %02x %02x\n", 
			__LINE__, 
			infp->name, 
			instr, size,
			instr[0], instr[1], instr[2]);

	/***********************************************/
	/*   This   point   is   part   of   a   loop  */
	/*   (implemented  with goto) to find the end  */
	/*   part  of  a  function.  Original Solaris  */
	/*   code assumes a single exit, via RET, for  */
	/*   amd64,  but  is more accepting for i386.  */
	/*   However,  with  GCC  as a compiler a lot  */
	/*   more    things   can   happen   -   very  */
	/*   specifically,  we can have multiple exit  */
	/*   points in a function. So we need to find  */
	/*   each of those.			       */
	/***********************************************/
	for (; instr < limit; instr += size) {
		/*
		 * If this disassembly fails, then we've likely walked off into
		 * a jump table or some other unsuitable area.  Bail out of the
		 * disassembly now.
		 */
		size = dtrace_instr_size(instr);
		if (do_print) {
			int i;
			printk("%p: sz=%2d ", instr, size);
			for (i = 0; i < size; i++) {
				printk(" %02x", instr[i] & 0xff);
			}
			printk("\n");
		}
		if (size <= 0)
			return;


		/***********************************************/
		/*   Handle  the BUG_ON macro which generates  */
		/*   a  ud2a  instruction  followed by a long  */
		/*   and a short to indicate the filename and  */
		/*   line  number  of  where  we  bugged. The  */
		/*   kernel  handles  the illegal instruction  */
		/*   and  does  the appropriate lookup and we  */
		/*   need   to   skip   over   these   pseudo  */
		/*   instructions.			       */
		/***********************************************/
		if (*instr == 0x0f && instr[1] == 0x0b) {
	                size = 2 + 8 + 2;
	                /*printk("got a bugon: %p\n", instr);*/
	                continue;
	        }
		
#define	FBT_RET			0xc3
#define	FBT_RET_IMM16		0xc2
#ifdef __amd64
		/*
		 * We only instrument "ret" on amd64 -- we don't yet instrument
		 * ret imm16, largely because the compiler doesn't seem to
		 * (yet) emit them in the kernel...
		 */
		switch (*instr) {
		  case FBT_RET:
			invop = DTRACE_INVOP_RET;
			break;
		  default:
		  	continue;
		  }
#else /* i386 */
		switch (*instr) {
		  case FBT_RET:
			invop = DTRACE_INVOP_RET;
			break;
		  case FBT_RET_IMM16:
			invop = DTRACE_INVOP_RET_IMM16;
			break;
		  /***********************************************/
		  /*   Some  functions  can end in a jump, e.g.	 */
		  /*   security_file_permission has an indirect	 */
		  /*   jmp.  Dont  think we care too much about	 */
		  /*   this,  but  we  could  handle direct and	 */
		  /*   indirect jumps out of the function body.	 */
		  /***********************************************/
		  default:
		  	continue;
		  }
#endif

		/*
		 * We have a winner!
		 */
		if ((*infp->func_return)(infp, instr, size) == 0)
			continue;

		if (do_print) {
			printk("%d:alloc return-patchpoint: %s %p: %02x %02x\n", 
				__LINE__, infp->name, instr, instr[0], instr[1]);
		}
	}
}

/**********************************************************************/
/*   Disassemble  the  kernel,  and  allow us to invoke callbacks to  */
/*   determine which instructions we want to add SDT probes from.     */
/**********************************************************************/
static	const char *(*my_kallsyms_lookup)(unsigned long addr,
                        unsigned long *symbolsize,
                        unsigned long *offset,
                        char **modname, char *namebuf);
void
dtrace_parse_kernel_function(void (*callback)(uint8_t *, int), struct module *modp, char *name, uint8_t *instr, uint8_t *limit)
{
	int	size;
	int	modrm;

//printk("dtrace_parse_kernel_function: %s, size=%d\n", name, limit - instr);
	for (; instr < limit; instr += size) {
		/***********************************************/
		/*   Make sure we dont try and handle data or  */
		/*   bad instructions.			       */
		/***********************************************/
		if ((size = dtrace_instr_size_modrm(instr, &modrm)) <= 0)
			return;
/*
if (size == 9 && *instr == 0x65)
printk("inst:%p %d %02x %02x %02x %02x %02x %04x\n", instr, size, *instr, instr[1], instr[2], instr[3], instr[4],
*(int *) (&instr[5]));
*/
		/***********************************************/
		/*   incq %gs:nnnn			       */
		/***********************************************/
		if (//size == 9 &&
		    instr[0] == 0x65 &&
		    instr[1] == 0x48 &&
		    instr[2] == 0xff &&
		    instr[3] == 0x04 &&
		    instr[4] == 0x25) {
		    	callback(instr, size);
		    	}
	}
}
void
dtrace_parse_kernel(void (*callback)(uint8_t *, int))
{
	static struct module kern;
	int	n;
static	caddr_t ktext;
static	caddr_t ketext;
static int first_time = TRUE;
	caddr_t a, aend;
	char	name[KSYM_NAME_LEN];

	if (first_time) {
		first_time = FALSE;
		ktext = get_proc_addr("_text");
		if (ktext == NULL)
			ktext = get_proc_addr("_stext");
		ketext = get_proc_addr("_etext");
		my_kallsyms_lookup = get_proc_addr("kallsyms_lookup");
		}

	if (ktext == NULL) {
		printk("dtracedrv:dtrace_parse_kernel: Cannot find _text/_stext\n");
		return;
	}
	if (kern.name[0])
		return;

	strcpy(kern.name, "kernel");
	/***********************************************/
	/*   Walk   the  code  segment,  finding  the  */
	/*   symbols,  and  creating a probe for each  */
	/*   one.				       */
	/***********************************************/
	for (n = 0, a = ktext; my_kallsyms_lookup && a < ketext; ) {
		const char *cp;
		unsigned long size;
		unsigned long offset;
		char	*modname = NULL;

		cp = my_kallsyms_lookup((unsigned long) a, &size, &offset, &modname, name);
		if (cp == NULL)
			aend = a + 4;
		else
			aend = a + (size - offset);

		/***********************************************/
		/*   If  this  function  is  toxic, we mustnt  */
		/*   touch it.				       */
		/***********************************************/
		if (cp && *cp && !is_toxic_func((unsigned long) a, cp)) {
			dtrace_parse_kernel_function(callback, &kern, name, a, aend);
//			instr_provide_function(&kern, 
//				(par_module_t *) &kern, //uck on the cast..we dont really need it
//				"kernel", name, 
//				a, a, aend, n);
		}
		a = aend;
		n++;
	}
}
/**********************************************************************/
/*   Crack open a provider:module:function spec.		      */
/**********************************************************************/
static void
extract_elements(char *str, char *provider, char *module, char *function, char *name)
{	int	i;

	for (i = 0; *str && *str != ':' && i < MAX_PROVIDER - 1; i++)
		*provider++ = *str++;
	*provider = '\0';
	while (*str && *str != ':') str++;
	if (*str) str++;

	for (i = 0; *str && *str != ':' && i < MAX_MODULE - 1; i++)
		*module++ = *str++;
	*module = '\0';
	while (*str && *str != ':') str++;
	if (*str) str++;

	for (i = 0; *str && *str != ':' && i < MAX_FUNCTION - 1; i++)
		*function++ = *str++;
	*function = '\0';
	while (*str && *str != ':') str++;
	if (*str) str++;

	for (i = 0; *str && *str != ':' && i < MAX_NAME - 1; i++)
		*name++ = *str++;
	*name = '\0';
	while (*str && *str != ':') str++;
	if (*str) str++;
}

/**********************************************************************/
/*   Initialise  the  provider  addresses.  Do  this  when dtrace is  */
/*   safely loaded with the core function pointers.		      */
/**********************************************************************/
static void
prcom_init(void)
{	static int first_time = TRUE;
	provider_t	*pp;
	provider_t	*pp1;
	void	*memp;

	if (!first_time)
		return;
	first_time = FALSE;

	/***********************************************/
	/*   Note  that many providers may map to the  */
	/*   same  function,  so  handle  the  shared  */
	/*   approach.				       */
	/***********************************************/
	for (pp = map; pp < &map[probe_cnt]; pp++) {
		if (pp->p_func_addr)
			continue;

		/***********************************************/
		/*   If  we  want  an  argument  but argument  */
		/*   doesnt  exist  in this kernel, then just  */
		/*   skip this entry.			       */
		/***********************************************/
		if (pp->p_arg0_name) {
			if ((pp->p_stack0_addr = (uintptr_t) get_proc_addr(pp->p_arg0_name)) == 0)
				continue;
			}

		if ((memp = pp->p_instr_addr) == NULL) {
			memp = get_proc_addr(pp->p_func_name);
			if (memp == NULL)
				continue;
			}

		if (!memory_set_rw(memp, 1, TRUE))
			continue;

		pp->p_patchval = *(unsigned char *) memp;
		pp->p_inslen = dtrace_instr_size_modrm((uchar_t *) memp, &pp->p_modrm);
		pp->p_func_addr = memp;

		/***********************************************/
		/*   Share  this  to  all functions which are  */
		/*   the same.				       */
		/***********************************************/
		for (pp1 = pp + 1; pp1 < &map[probe_cnt]; pp1++) {
			int	same = FALSE;
			if (pp1->p_func_name && pp->p_func_name &&
			    strcmp(pp1->p_func_name, pp->p_func_name) == 0)
			    	same = TRUE;
			if (pp1->p_instr_addr && pp->p_instr_addr && pp1->p_instr_addr == pp->p_instr_addr)
				same = TRUE;

			if (same) {
				pp1->p_func_addr = memp;
				pp1->p_inslen = pp->p_inslen;
				pp1->p_modrm = pp->p_modrm;
				pp1->p_patchval = pp->p_patchval;
			}
		}
	}
}

/**********************************************************************/
/*   Called  from  other  providers  to  add  a  probe to a specific  */
/*   instruction.						      */
/**********************************************************************/
void
prcom_add_instruction(char *probe, uint8_t *instr)
{
	if (probe_cnt + 1 >= MAX_PROVIDER_TBL) {
		printk("prov_common: Cannot add '%s' as MAX_PROVIDER_TBL too small\n", probe);
		return;
	}

	map[probe_cnt].p_probe = probe;
	map[probe_cnt].p_instr_addr = instr;
	probe_cnt++;
//printk("prcom_add_instruction: %s %p\n", name, instr);
}
void
prcom_add_function(char *probe, char *func)
{	uint8_t *addr;

	if (probe_cnt + 1 >= MAX_PROVIDER_TBL) {
		printk("prov_common: Cannot add '%s' as MAX_PROVIDER_TBL too small\n", probe);
		return;
	}

	if ((addr = get_proc_addr(func)) == 0) {
		printk("prcom_add_function: cannot locate '%s'\n", func);
		return;
	}
	map[probe_cnt].p_probe = probe;
	map[probe_cnt].p_instr_addr = addr;
	probe_cnt++;
//printk("prcom_add_instruction: %s %p\n", name, instr);
}
/*ARGSUSED*/
static int
prcom_invop(uintptr_t addr, uintptr_t *stack, uintptr_t eax, trap_instr_t *tinfo)
{
	uintptr_t stack0, stack1, stack2, stack3, stack4, n;
	struct pt_regs *regs;
	provider_t *pp;

	/***********************************************/
	/*   Dont fire probe if this is unsafe.	       */
	/***********************************************/
	if (!tinfo->t_doprobe)
		return (DTRACE_INVOP_NOP);

	/***********************************************/
	/*   Some probes are matching on the value of  */
	/*   arg1   (shared   dispatch).   Get   arg1  */
	/*   (stack0)  early  so we can use it in the  */
	/*   loop below.			       */
	/***********************************************/
	regs = (struct pt_regs *) stack;
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	stack0 = regs->c_arg0;
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT | CPU_DTRACE_BADADDR);
	for (pp = map; pp < &map[probe_cnt]; pp++) {
		if (addr != (uintptr_t) pp->p_func_addr)
			continue;
		if (!pp->p_enabled) {
			continue;
		}
		if (!pp->p_stack0_cond)
			break;
		n = stack0 - pp->p_stack0_offset;
		if (pp->p_stack0_addr == n)
			break;
	}

	if (pp->p_probe == NULL) {
		static int cnt;
		if (cnt++ < 50)
			printk("dtrace: [cpu%d] prov_common: cannot map %p\n", 
				smp_processor_id(), (void *) stack0);
		return 0;
	}
	/***********************************************/
	/*   Bubble  up  entries  to  the  top of the  */
	/*   list.				       */
	/***********************************************/
	if (pp != map && 0) {
		provider_t t = pp[-1];
		pp[-1] = *pp;
		*pp = t;
	}

	tinfo->t_opcode = pp->p_patchval;
	tinfo->t_inslen = pp->p_inslen;
	tinfo->t_modrm = pp->p_modrm;

	/***********************************************/
	/*   Get  remaining arguments, in a protected  */
	/*   fashion.				       */
	/***********************************************/
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	stack1 = regs->c_arg1;
	stack2 = regs->c_arg2;
	stack3 = regs->c_arg3;
	stack4 = regs->c_arg4;
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT | CPU_DTRACE_BADADDR);

	stack0 = prcom_getarg(pp, pp->p_id, NULL, 0, 0);
	stack1 = prcom_getarg(pp, pp->p_id, NULL, 1, 0);

//printk("common probe %p: %p %p %p %p %p\n", &addr, stack0, stack1, stack2, stack3, stack4);
	dtrace_probe(pp->p_id, stack0, stack0, stack0, 0, 0);

	return (DTRACE_INVOP_NOP);
}

/*ARGSUSED*/
static void
prcom_provide(void *arg, const dtrace_probedesc_t *desc)
{
//	TODO();
}

/*ARGSUSED*/
static void
prcom_destroy(void *arg, dtrace_id_t id, void *parg)
{
}

/*ARGSUSED*/
static int
prcom_enable(void *arg, dtrace_id_t id, void *parg)
{	provider_t *pp = (provider_t *) parg;

	prcom_init();

	pp->p_enabled = TRUE;
	if (pp->p_func_addr &&
	    *(unsigned char *) pp->p_func_addr != PATCHVAL) {
		*(unsigned char *) pp->p_func_addr = PATCHVAL;
	}

	return 0;
}

/*ARGSUSED*/
static void
prcom_disable(void *arg, dtrace_id_t id, void *parg)
{	provider_t *pp = (provider_t *) parg;

	pp->p_enabled = FALSE;
	if (pp->p_func_addr && pp->p_patchval &&
	    *(unsigned char *) pp->p_func_addr == PATCHVAL) {
		*(unsigned char *) pp->p_func_addr = pp->p_patchval;
	}
}

/*ARGSUSED*/
void
prcom_getargdesc(void *arg, dtrace_id_t id, void *parg, dtrace_argdesc_t *desc)
{	provider_t *pp = parg;

printk("getargdesc %s %d\n", pp->p_probe, desc->dtargd_ndx);
	desc->dtargd_native[0] = '\0';
	desc->dtargd_xlate[0] = '\0';

	if (pp->p_arg0 && desc->dtargd_ndx == 0) {
		(void) strcpy(desc->dtargd_native, pp->p_arg0);
//printk(" .. '%s'\n", desc->dtargd_native);
		return;
	}

	if (pp->p_arg1 && desc->dtargd_ndx == 1) {
		(void) strcpy(desc->dtargd_native, pp->p_arg1);
//printk(" .. '%s'\n", desc->dtargd_native);
		return;
	}

	desc->dtargd_ndx = DTRACE_ARGNONE;
}
/*ARGSUSED*/
#include "ctf_struct.h"
uint64_t
prcom_getarg(void *arg, dtrace_id_t id, void *parg, int argno, int aframes)
{static psinfo_t psinfo[NCPU];
	psinfo_t *ps = &psinfo[smp_processor_id()];
	provider_t *pp = arg;

//dtrace_printf("getarg %s %d\n", pp->p_probe, argno);
	if (strcmp(pp->p_probe, "sched:::off-cpu") == 0) {
		if (current) {
			ps->pr_pid = current->pid;
			ps->pr_pgid = current->tgid;
			ps->pr_ppid = current->parent ? current->parent->pid : 0;
		} else {
			memset(ps, 0, sizeof *ps);
		}
		return (uint64_t) ps;
	}
	return dtrace_getarg(argno, aframes);
}
static dtrace_pops_t prcom_pops = {
	prcom_provide,
	NULL,
	prcom_enable,
	prcom_disable,
	NULL,
	NULL,
	prcom_getargdesc,
	prcom_getarg,
	NULL,
	prcom_destroy
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
prcom_attach(void)
{	provider_t	*pp;
	provider_t	*pp1;
static	char	provider[MAX_PROVIDER];
static	char	module[MAX_MODULE];
static	char	function[MAX_FUNCTION];
static	char	name[MAX_NAME];
	int	plen;

	dtrace_invop_add(prcom_invop);

	for (pp = map; ; pp++) {
		if (pp->p_func_name == NULL && pp->p_instr_addr == NULL)
			break;
		probe_cnt++;
		}

	/***********************************************/
	/*   Add the common SDT providers.	       */
	/***********************************************/
	tcp_init();
	vminfo_init();

	for (pp = map; pp < &map[probe_cnt]; pp++) {
		extract_elements(pp->p_probe, provider, module, function, name);
		if (pp->p_provider_id == 0) {
			plen = strlen(provider);

			if (dtrace_register(provider, &pattr,
			    DTRACE_PRIV_KERNEL, NULL,
			    &prcom_pops, NULL, &pp->p_provider_id) != 0) {
			    	printk("prcom_attach: Failed to register '%s' probe\n",
					pp->p_probe);
				return (DDI_FAILURE);
			}

			/***********************************************/
			/*   Share the provider id.		       */
			/***********************************************/
			for (pp1 = pp + 1; pp1 < &map[probe_cnt]; pp1++) {
				if (strncmp(provider, pp1->p_probe, plen) == 0 &&
				    pp1->p_probe[plen] == ':') {
					pp1->p_provider_id = pp->p_provider_id;
				}
			}
		}

		/***********************************************/
		/*   Create the probe.			       */
		/***********************************************/
		if (pp->p_want_return) {
			pp->p_id = dtrace_probe_create(pp->p_provider_id,
			    module[0] ? module : NULL, function, "entry", 0, pp);
			pp->p_id2 = dtrace_probe_create(pp->p_provider_id,
			    module[0] ? module : NULL, function, "return", 0, pp);
		} else {
			pp->p_id = dtrace_probe_create(pp->p_provider_id,
			    module[0] ? module : NULL, function, 
			    name[0] ? name : NULL, 0, pp);
		}
	}

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
prcom_detach(void)
{	provider_t *pp;
	provider_t *pp1;

	for (pp = map; pp < &map[probe_cnt]; pp++) {
		if (pp->p_func_addr && pp->p_patchval &&
		    *(unsigned char *) pp->p_func_addr == PATCHVAL) {
			*(unsigned char *) pp->p_func_addr = pp->p_patchval;
		}
	}

	/***********************************************/
	/*   Unregister the distinct providers.	       */
	/***********************************************/
	for (pp = map; pp < &map[probe_cnt]; pp++) {
		dtrace_provider_id_t id = pp->p_provider_id;
		if (id == 0)
			continue;
		if (dtrace_unregister(id) != 0) {
			printk("dtrace: prcomm: cannot unregister %s\n",
				pp->p_probe);
		}
		for (pp1 = pp + 1; pp1 < &map[probe_cnt]; pp1++) {
			if (pp1->p_provider_id == id)
				pp1->p_provider_id = 0;
		}
	}

	dtrace_invop_remove(prcom_invop);

	return (DDI_SUCCESS);
}

int dtrace_prcom_init(void)
{

	printk(KERN_WARNING "dtrace: common provider being initialised\n");
	prcom_attach();

	return 0;
}
void dtrace_prcom_exit(void)
{
	prcom_detach();
}



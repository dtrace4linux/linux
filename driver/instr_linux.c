/**********************************************************************/
/*   This  is  the  Instruction  Provider  code  -- based on FBT and  */
/*   fbt_linux.c,  but  it  allows  us  to  place  probes  based  on  */
/*   instruction type, rather than function entry/exit.		      */
/*   								      */
/*   Can  be used for interesting things like branches, REP/LOOP and  */
/*   other  instructions,  but  could  be a performance hit since it  */
/*   could  allow  for lots of probes (much more than FBT) and could  */
/*   stress a system.						      */
/*   								      */
/*   Originally  conceived  to help debug FBT by allowing us to find  */
/*   more  instructions of interest which need to be debugged during  */
/*   the single step phase.					      */
/*   								      */
/*   CDDL License applies (for now).				      */
/*   								      */
/*   Date: June 2008						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   $Header: Last edited: 07-Jun-2009 1.2 $ 			      */
/*
*/
/**********************************************************************/

#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <dtrace_proto.h>
#include <linux/cpumask.h>

#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/kallsyms.h>
#include <linux/seq_file.h>

# undef NULL
# define NULL 0

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE("CDDL");
MODULE_DESCRIPTION("DTRACE/Instruction Tracing Driver");

/**********************************************************************/
/*   Under  Solaris, they use the LOCK prefix opcode for instruction  */
/*   traps,  but  this  wont work for Linux, since we have lots more  */
/*   candidate  patchable instructions, and LOCK needs the next byte  */
/*   to  decide  if we have an invalid instruction or not. Dont know  */
/*   why they do this, but lets just use INT3 everywhere. (Consider:  */
/*   LOCK  for  a  RET; in this case the next byte may be garbage as  */
/*   GCC aligns to a quad boundary).				      */
/**********************************************************************/
#if defined(linux)
#  define	INSTR_PATCHVAL		0xcc
#else
#  if defined(__amd64)
#    define	INSTR_PATCHVAL		0xcc
#  else
#    define	INSTR_PATCHVAL		0xf0
#  endif
#endif

#define	INSTR_ADDR2NDX(addr)	((((uintptr_t)(addr)) >> 4) & instr_probetab_mask)
#define	INSTR_PROBETAB_SIZE	0x8000		/* 32k entries -- 128K total */

typedef struct instr_probe {
	struct instr_probe *insp_hashnext;
	uint8_t		*insp_patchpoint;
	uint8_t		insp_patchval;
	uint8_t		insp_savedval;
	uint8_t		insp_inslen;	/* Length of instr we are patching */
	char		insp_modrm;	/* Offset to modrm byte of instruction */
	uintptr_t	insp_roffset;
	dtrace_id_t	insp_id;
	char		*insp_name;
	struct modctl	*insp_ctl;
	int		insp_loadcnt;
	int		insp_symndx;
	struct instr_probe *insp_next;
} instr_probe_t;

//static dev_info_t		*instr_devi;
static dtrace_provider_id_t	instr_id;
static instr_probe_t		**instr_probetab;
static int			instr_probetab_size;
static int			instr_probetab_mask;
static int			instr_verbose = 0;
static int			num_probes;

extern int dtrace_unhandled;

static void instr_provide_function(struct modctl *mp, 
    	par_module_t *pmp,
	char *modname, char *name, uint8_t *st_value, 
	uint8_t *instr, uint8_t *limit, int);
static	const char *(*my_kallsyms_lookup)(unsigned long addr,
                        unsigned long *symbolsize,
                        unsigned long *offset,
                        char **modname, char *namebuf);

/**********************************************************************/
/*   For debugging - make sure we dont add a patch to the same addr.  */
/**********************************************************************/
static int
instr_is_patched(char *name, uint8_t *addr)
{
	instr_probe_t *fbt = instr_probetab[INSTR_ADDR2NDX(addr)];

	for (; fbt != NULL; fbt = fbt->insp_hashnext) {
		if (fbt->insp_patchpoint == addr) {
			dtrace_printf("fbt:dup patch: %p %s\n", addr, name);
			return 1;
		}
	}
	return 0;
}
/**********************************************************************/
/*   Here  from  INT3 interrupt context to see if the address we hit  */
/*   is one of ours.						      */
/**********************************************************************/
static int
instr_invop(uintptr_t addr, uintptr_t *stack, uintptr_t rval, trap_instr_t *tinfo)
{
	uintptr_t stack0, stack1, stack2, stack3, stack4;
	instr_probe_t *fbt = instr_probetab[INSTR_ADDR2NDX(addr)];

//HERE();
	for (; fbt != NULL; fbt = fbt->insp_hashnext) {
		if ((uintptr_t)fbt->insp_patchpoint == addr) {
			tinfo->t_opcode = fbt->insp_savedval;
			tinfo->t_inslen = fbt->insp_inslen;
			tinfo->t_modrm = fbt->insp_modrm;
			if (!tinfo->t_doprobe)
				return DTRACE_INVOP_ANY;
			if (fbt->insp_roffset == 0) {
				/*
				 * When accessing the arguments on the stack,
				 * we must protect against accessing beyond
				 * the stack.  We can safely set NOFAULT here
				 * -- we know that interrupts are already
				 * disabled.
				 */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				CPU->cpu_dtrace_caller = stack[0];
				stack0 = stack[1];
				stack1 = stack[2];
				stack2 = stack[3];
				stack3 = stack[4];
				stack4 = stack[5];
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);

				dtrace_probe(fbt->insp_id, stack0, stack1,
				    stack2, stack3, stack4);

				CPU->cpu_dtrace_caller = NULL;
			} else {
#ifdef __amd64
				/*
				 * On amd64, we instrument the ret, not the
				 * leave.  We therefore need to set the caller
				 * to assure that the top frame of a stack()
				 * action is correct.
				 */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				CPU->cpu_dtrace_caller = stack[0];
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);
#endif

				dtrace_probe(fbt->insp_id, fbt->insp_roffset,
				    rval, 0, 0, 0);
				CPU->cpu_dtrace_caller = NULL;
			}

			return DTRACE_INVOP_ANY;
		}
	}
//HERE();

	return (0);
}
static int
get_refcount(struct module *mp)
{	int	sum = 0;

	if (mp == NULL)
		return 0;

#if defined(CONFIG_MODULE_UNLOAD) && defined(CONFIG_SMP)
	/***********************************************/
	/*   Linux   2.6.29   does   something   here  */
	/*   presumably   to  avoid  bad  cache  line  */
	/*   behavior. We dont really care about this  */
	/*   for now.				       */
	/***********************************************/
# else
	{int	i;
	for (i = 0; i < NR_CPUS; i++)
		sum += local_read(&mp->ref[i].count);
	}
# endif
	return sum;
}

/**********************************************************************/
/*   This  code  is called from dtrace.c (dtrace_probe_provide) when  */
/*   we  are arming the fbt functions. Since the kernel doesnt exist  */
/*   as a module, we need to handle that as a special case.	      */
/**********************************************************************/
void
instr_provide_kernel(void)
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
		printk("dtracedrv:instr_provide_kernel: Cannot find _text/_stext\n");
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

//printk("lookup %p kallsyms_lookup=%p\n", a, kallsyms_lookup);
		cp = my_kallsyms_lookup((unsigned long) a, &size, &offset, &modname, name);
if (cp && size == 0)
printk("instr_linux: size=0 %p %s\n", a, cp ? cp : "??");
/*		printk("a:%p cp:%s size:%lx offset:%lx\n", a, cp ? cp : "--undef--", size, offset);*/
		if (cp == NULL)
			aend = a + 4;
		else
			aend = a + (size - offset);

		/***********************************************/
		/*   If  this  function  is  toxic, we mustnt  */
		/*   touch it.				       */
		/***********************************************/
		if (cp && *cp && !is_toxic_func((unsigned long) a, cp)) {
			instr_provide_function(&kern, 
				(par_module_t *) &kern, //uck on the cast..we dont really need it
				"kernel", name, 
				a, a, aend, n);
		}
		a = aend;
		n++;
	}
}
/*ARGSUSED*/
static void
instr_provide_module(void *arg, struct modctl *ctl)
{	int	i;
	struct module *mp = (struct module *) ctl;
	char *modname = mp->name;
	char	*str = mp->strtab;
	char	*name;
    	par_module_t *pmp;

	int	init;
	/***********************************************/
	/*   Possible  memleak  here...we  allocate a  */
	/*   parallel  struct, but need to free if we  */
	/*   are offloaded.			       */
	/***********************************************/
	pmp = par_alloc(mp, sizeof *pmp, &init);
	if (pmp->fbt_nentries) {
		/*
		 * This module has some FBT entries allocated; we're afraid
		 * to screw with it.
		 */
		return;
	}

	if (dtrace_here) 
		printk("%s(%d):modname=%s num_symtab=%u\n", __FILE__, __LINE__, modname, (unsigned) mp->num_symtab);
	if (strcmp(modname, "dtracedrv") == 0)
		return;

	for (i = 1; i < mp->num_symtab; i++) {
		uint8_t *instr, *limit;
		Elf_Sym *sym = (Elf_Sym *) &mp->symtab[i];
int dtrace_here = 0;
if (strcmp(modname, "dummy") == 0) dtrace_here = 1;

		name = str + sym->st_name;
		if (sym->st_name == NULL || *name == '\0')
			continue;

		/***********************************************/
		/*   Linux re-encodes the symbol types.	       */
		/***********************************************/
		if (sym->st_info != 't' && sym->st_info != 'T')
			continue;

//		if (strstr(name, "init")) 
//			continue;
//if (sym->st_info != 'T') {printk("skip -- %02d %c:%s\n", i, sym->st_info, name); continue;}

#if 0
		if (ELF_ST_TYPE(sym->st_info) != STT_FUNC)
			continue;


		/*
		 * Weak symbols are not candidates.  This could be made to
		 * work (where weak functions and their underlying function
		 * appear as two disjoint probes), but it's not simple.
		 */
		if (ELF_ST_BIND(sym->st_info) == STB_WEAK)
			continue;
#endif

		if (strstr(name, "dtrace_") == name &&
		    strstr(name, "dtrace_safe_") != name) {
			/*
			 * Anything beginning with "dtrace_" may be called
			 * from probe context unless it explitly indicates
			 * that it won't be called from probe context by
			 * using the prefix "dtrace_safe_".
			 */
			continue;
		}

		if (strstr(name, "dtracedrv_") == name)
			continue;

		if (strstr(name, "kdi_") == name ||
		    strstr(name, "kprobe") == name) {
			/*
			 * Anything beginning with "kdi_" is a part of the
			 * kernel debugger interface and may be called in
			 * arbitrary context -- including probe context.
			 */
			continue;
		}

		if (strcmp(name, "_init") == 0)
			continue;

		if (strcmp(name, "_fini") == 0)
			continue;

		instr = (uint8_t *)sym->st_value;
		limit = (uint8_t *)(sym->st_value + sym->st_size);
//printk("trying -- %02d %c:%s\n", i, sym->st_info, name);
//HERE();

		/***********************************************/
		/*   Ignore the init function of modules - we  */
		/*   will  never  execute them now the module  */
		/*   is loaded, and we dont want to be poking  */
		/*   potential  pages  which  dont  exist  in  */
		/*   memory or which are being used for data.  */
		/***********************************************/
		if (instr == (uint8_t *) mp->init)
			continue;

		/***********************************************/
		/*   We  do have syms that appear to point to  */
		/*   unmapped  pages.  Maybe  these are freed  */
		/*   pages after a driver loads. Double check  */
		/*   -  if /proc/kallsyms says its not there,  */
		/*   then ignore it. 			       */
		/***********************************************/
		if (!validate_ptr(instr))
			continue;

		/***********************************************/
		/*   Look  at the section this symbol is in -  */
		/*   we   dont   want   sections   which  can  */
		/*   disappear   or   have   disappeared  (eg  */
		/*   .init).				       */
		/*   					       */
		/*   I'm  not sure I follow this code for all  */
		/*   kernel  releases - if we have the field,  */
		/*   it should have a fixed meaning, but some  */
		/*   modules  have  bogus  section attributes  */
		/*   pointers   (maybe   pointers   to  freed  */
		/*   segments?). Lets be careful out there.    */
		/*   					       */
		/*   20090425  Ok  - heres the deal. In 2.6.9  */
		/*   (at   least)   the   section   table  is  */
		/*   allocated  but  only  for sections which  */
		/*   are  SHF_ALLOC.  This means the array of  */
		/*   sections    cannot    be    indexed   by  */
		/*   sym->st_shndx since the mappings are now  */
		/*   bogus  (kernel  doesnt adjust the symbol  */
		/*   section  indexes). What we need to do is  */
		/*   attempt to find the section by address.   */
		/***********************************************/
		if (!instr_in_text_seg(mp, name, sym))
			continue;

		/***********************************************/
		/*   We are good to go...		       */
		/***********************************************/
		instr_provide_function(mp, pmp,
			modname, name, 
			(uint8_t *) sym->st_value, 
			instr, limit, i);
		}
}

/**********************************************************************/
/*   Common code to handle module and kernel functions.		      */
/**********************************************************************/
static void
instr_provide_function(struct modctl *mp, par_module_t *pmp,
	char *modname, char *name, uint8_t *st_value,
	uint8_t *instr, uint8_t *limit, int symndx)
{
	int	do_print = FALSE;
	int	invop = 0;
	instr_probe_t *fbt;
	int	size;
	int	modrm;
	char	*orig_name = name;
	char	name_buf[128];
	char	pred_buf[128];

printk("instr_provide %s\n", name);
# define UNHANDLED_FBT() if (do_print || dtrace_unhandled) { \
		printk("fbt:unhandled instr %s:%p %02x %02x %02x %02x\n", \
			name, instr, instr[0], instr[1], instr[2], instr[3]); \
			}

# define INSTR(val, opcode_name) if (instr[0] == val) { \
	snprintf(name_buf, sizeof name_buf, "%s-%s", orig_name, opcode_name); \
	name = name_buf; \
	}

	for (; instr < limit; instr += size) {
		/***********************************************/
		/*   Make sure we dont try and handle data or  */
		/*   bad instructions.			       */
		/***********************************************/
		if ((size = dtrace_instr_size_modrm(instr, &modrm)) <= 0)
			return;

		name_buf[0] = '\0';

		INSTR(0x70, "jo");
		INSTR(0x71, "jno");
		INSTR(0x72, "jb");
		INSTR(0x73, "jae");
		INSTR(0x74, "je");
		INSTR(0x75, "jne");
		INSTR(0x76, "jbe");
		INSTR(0x77, "ja");
		INSTR(0x78, "js");
		INSTR(0x79, "jns");
		INSTR(0x7a, "jp");
		INSTR(0x7b, "jnp");
		INSTR(0x7c, "jl");
		INSTR(0x7d, "jge");
		INSTR(0x7e, "jle");
		INSTR(0x7f, "jg");

		INSTR(0x90, "nop");

		INSTR(0xa6, "scas");
		INSTR(0xe0, "loopne");
		INSTR(0xe1, "loope");
		INSTR(0xe2, "loop");

		INSTR(0xe8, "callr");

		INSTR(0xf0, "lock");
		INSTR(0xf1, "icebp");
		INSTR(0xf2, "repz");
		INSTR(0xf3, "repz");
		INSTR(0xfa, "cli");
		INSTR(0xfb, "sti");
		
		if (name_buf[0] == 0) {
			continue;
		}

		sprintf(pred_buf, "0x%p", instr);
		/***********************************************/
		/*   Make  sure  this  doesnt overlap another  */
		/*   sym. We are in trouble when this happens  */
		/*   - eg we will mistaken what the emulation  */
		/*   is  for,  but  also,  it means something  */
		/*   strange  happens, like kernel is reusing  */
		/*   a  page  (eg  for init/exit section of a  */
		/*   module).				       */
		/***********************************************/
		if (instr_is_patched(name, instr))
			return;

		fbt = kmem_zalloc(sizeof (instr_probe_t), KM_SLEEP);
		fbt->insp_name = name;

		fbt->insp_id = dtrace_probe_create(instr_id, modname,
		    name, pred_buf, 3, fbt);
		num_probes++;
		fbt->insp_patchpoint = instr;
		fbt->insp_ctl = mp; // ctl;
		fbt->insp_loadcnt = get_refcount(mp);
		/***********************************************/
		/*   Save  potential overwrite of instruction  */
		/*   and  length,  because  we  will need the  */
		/*   entire  instruction  when we single step  */
		/*   over it.				       */
		/***********************************************/
		fbt->insp_savedval = *instr;
		fbt->insp_inslen = size;
	//if (modrm >= 0 && (instr[modrm] & 0xc7) == 0x05) printk("modrm %s %p rm=%d\n", name, instr, modrm);
		fbt->insp_modrm = modrm;
		fbt->insp_patchval = INSTR_PATCHVAL;

		fbt->insp_hashnext = instr_probetab[INSTR_ADDR2NDX(instr)];
		fbt->insp_symndx = symndx;
		instr_probetab[INSTR_ADDR2NDX(instr)] = fbt;

		if (do_print)
			printk("%d:alloc entry-patchpoint: %s %p sz=%d %02x %02x %02x\n", 
				__LINE__, 
				name, 
				fbt->insp_patchpoint, 
				fbt->insp_inslen,
				instr[0], instr[1], instr[2]);

		pmp->fbt_nentries++;
	}
}
/*ARGSUSED*/
static void
instr_destroy(void *arg, dtrace_id_t id, void *parg)
{
	instr_probe_t *fbt = parg, *next, *hash, *last;
	struct modctl *ctl = fbt->insp_ctl;
	struct module *mp = ctl;
	int ndx;

	do {
		if (mp != NULL && get_refcount(mp) == fbt->insp_loadcnt) {
			if ((get_refcount(mp) == fbt->insp_loadcnt &&
			    mp->state == MODULE_STATE_LIVE)) {
			    	par_module_t *pmp = par_alloc(mp, sizeof *pmp, NULL);
				if (--pmp->fbt_nentries == 0)
					par_free(pmp);
			}
		}

		/*
		 * Now we need to remove this probe from the instr_probetab.
		 */
		ndx = INSTR_ADDR2NDX(fbt->insp_patchpoint);
		last = NULL;
		hash = instr_probetab[ndx];

		while (hash != fbt) {
			ASSERT(hash != NULL);
			last = hash;
			hash = hash->insp_hashnext;
		}

		if (last != NULL) {
			last->insp_hashnext = fbt->insp_hashnext;
		} else {
			instr_probetab[ndx] = fbt->insp_hashnext;
		}

		next = fbt->insp_next;
		kmem_free(fbt, sizeof (instr_probe_t));

		fbt = next;
	} while (fbt != NULL);
}

/*ARGSUSED*/
static int
instr_enable(void *arg, dtrace_id_t id, void *parg)
{
	instr_probe_t *fbt = parg;
	struct modctl *ctl = fbt->insp_ctl;
	struct module *mp = (struct module *) ctl;

# if 0
	ctl->mod_nenabled++;
# endif
	if (mp->state != MODULE_STATE_LIVE) {
		if (instr_verbose) {
			cmn_err(CE_NOTE, "fbt is failing for probe %s "
			    "(module %s unloaded)",
			    fbt->insp_name, mp->name);
		}

		return 0;
	}

	/*
	 * Now check that our modctl has the expected load count.  If it
	 * doesn't, this module must have been unloaded and reloaded -- and
	 * we're not going to touch it.
	 */
	if (get_refcount(mp) != fbt->insp_loadcnt) {
		if (instr_verbose) {
			cmn_err(CE_NOTE, "fbt is failing for probe %s "
			    "(module %s reloaded)",
			    fbt->insp_name, mp->name);
		}

		return 0;
	}

	for (; fbt != NULL; fbt = fbt->insp_next) {
		if (dtrace_here) 
			printk("instr_enable:patch %p p:%02x\n", fbt->insp_patchpoint, fbt->insp_patchval);
		if (memory_set_rw(fbt->insp_patchpoint, 1, TRUE)) {
			*fbt->insp_patchpoint = fbt->insp_patchval;
		}
	}
	return 0;
}

/*ARGSUSED*/
static void
instr_disable(void *arg, dtrace_id_t id, void *parg)
{
	instr_probe_t *fbt = parg;
	struct modctl *ctl = fbt->insp_ctl;
	struct module *mp = (struct module *) ctl;

# if 0
	ASSERT(ctl->mod_nenabled > 0);
	ctl->mod_nenabled--;

	if (!ctl->mod_loaded || (ctl->mod_loadcnt != fbt->insp_loadcnt))
		return;
# else
	if (mp->state != MODULE_STATE_LIVE ||
	    get_refcount(mp) != fbt->insp_loadcnt)
		return;
# endif

	for (; fbt != NULL; fbt = fbt->insp_next) {
		if (dtrace_here) {
			printk("%s:%d: Disable %p:%s:%s\n", 
				__func__, __LINE__, 
				fbt->insp_patchpoint, 
				fbt->insp_ctl->name,
				fbt->insp_name);
		}
		/***********************************************/
		/*   Memory  should  be  writable,  but if we  */
		/*   failed  in  the  instr_enable  code,  e.g.  */
		/*   because  kernel  freed an .init section,  */
		/*   then  dont  try and unpatch something we  */
		/*   didnt patch.			       */
		/***********************************************/
		if (*fbt->insp_patchpoint == fbt->insp_patchval) {
			if (memory_set_rw(fbt->insp_patchpoint, 1, TRUE))
				*fbt->insp_patchpoint = fbt->insp_savedval;
		}
	}
}

/*ARGSUSED*/
static void
instr_suspend(void *arg, dtrace_id_t id, void *parg)
{
	instr_probe_t *fbt = parg;
	struct modctl *ctl = fbt->insp_ctl;
	struct module *mp = (struct module *) ctl;

# if 0
	ASSERT(ctl->mod_nenabled > 0);

	if (!ctl->mod_loaded || (ctl->mod_loadcnt != fbt->insp_loadcnt))
		return;
# else
	if (mp->state != MODULE_STATE_LIVE ||
	    get_refcount(mp) != fbt->insp_loadcnt)
		return;
# endif

	for (; fbt != NULL; fbt = fbt->insp_next)
		*fbt->insp_patchpoint = fbt->insp_savedval;
}

/*ARGSUSED*/
static void
instr_resume(void *arg, dtrace_id_t id, void *parg)
{
	instr_probe_t *fbt = parg;
	struct modctl *ctl = fbt->insp_ctl;
	struct module *mp = (struct module *) ctl;

# if 0
	ASSERT(ctl->mod_nenabled > 0);

	if (!ctl->mod_loaded || (ctl->mod_loadcnt != fbt->insp_loadcnt))
		return;
# else
	if (mp->state != MODULE_STATE_LIVE ||
	    get_refcount(mp) != fbt->insp_loadcnt)
		return;
# endif

	for (; fbt != NULL; fbt = fbt->insp_next)
		*fbt->insp_patchpoint = fbt->insp_patchval;
}

/*ARGSUSED*/
static void
instr_getargdesc(void *arg, dtrace_id_t id, void *parg, dtrace_argdesc_t *desc)
{
	instr_probe_t *fbt = parg;
	struct modctl *ctl = fbt->insp_ctl;
	struct module *mp = (struct module *) ctl;
	ctf_file_t *fp = NULL;
	ctf_funcinfo_t f;
//	int error;
	ctf_id_t argv[32], type;
	int argc = sizeof (argv) / sizeof (ctf_id_t);
//	const char *parent;

	if (mp->state != MODULE_STATE_LIVE ||
	    get_refcount(mp) != fbt->insp_loadcnt)
		return;

	if (fbt->insp_roffset != 0 && desc->dtargd_ndx == 0) {
		(void) strcpy(desc->dtargd_native, "int");
		return;
	}

# if 0
	if ((fp = ctf_modopen(mp, &error)) == NULL) {
		/*
		 * We have no CTF information for this module -- and therefore
		 * no args[] information.
		 */
		goto err;
	}
# endif

	//TODO();
	if (fp == NULL)
		goto err;
# if 0

	/*
	 * If we have a parent container, we must manually import it.
	 */
	if ((parent = ctf_parent_name(fp)) != NULL) {
		ctf_file_t *pfp;
		TODO();
		struct modctl *mod;

		/*
		 * We must iterate over all modules to find the module that
		 * is our parent.
		 */
		for (mod = &modules; mod != NULL; mod = mod->mod_next) {
			if (strcmp(mod->mod_filename, parent) == 0)
				break;
		}

		if (mod == NULL)
			goto err;

		if ((pfp = ctf_modopen(mod->mod_mp, &error)) == NULL)
			goto err;

		if (ctf_import(fp, pfp) != 0) {
			ctf_close(pfp);
			goto err;
		}

		ctf_close(pfp);
	}
# endif

	if (ctf_func_info(fp, fbt->insp_symndx, &f) == CTF_ERR)
		goto err;

	if (fbt->insp_roffset != 0) {
		if (desc->dtargd_ndx > 1)
			goto err;

		ASSERT(desc->dtargd_ndx == 1);
		type = f.ctc_return;
	} else {
		if (desc->dtargd_ndx + 1 > f.ctc_argc)
			goto err;

		if (ctf_func_args(fp, fbt->insp_symndx, argc, argv) == CTF_ERR)
			goto err;

		type = argv[desc->dtargd_ndx];
	}

	if (ctf_type_name(fp, type, desc->dtargd_native,
	    DTRACE_ARGTYPELEN) != NULL) {
		ctf_close(fp);
		return;
	}
err:
	if (fp != NULL)
		ctf_close(fp);

	desc->dtargd_ndx = DTRACE_ARGNONE;
}

static dtrace_pattr_t instr_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

static dtrace_pops_t instr_pops = {
	NULL,
	instr_provide_module,
	instr_enable,
	instr_disable,
	instr_suspend,
	instr_resume,
	instr_getargdesc,
	NULL,
	NULL,
	instr_destroy
};
static void
instr_cleanup(dev_info_t *devi)
{
	dtrace_invop_remove(instr_invop);
	if (instr_id)
		dtrace_unregister(instr_id);

//	ddi_remove_minor_node(devi, NULL);
	if (instr_probetab)
		kmem_free(instr_probetab, instr_probetab_size * sizeof (instr_probe_t *));
	instr_probetab = NULL;
	instr_probetab_mask = 0;
}

/**********************************************************************/
/*   Module interface to the kernel.				      */
/**********************************************************************/
static int instr_ioctl(struct inode *inode, struct file *file,
                     unsigned int cmd, unsigned long arg)
{
	return -EIO;
}
static int
instr_open(struct inode *inode, struct file *file)
{
	return 0;
}

/**********************************************************************/
/*   Code for /proc/dtrace/fbt					      */
/**********************************************************************/

static void *instr_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos > num_probes)
		return 0;
	return (void *) (*pos + 1);
}
static void *instr_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{	long	n = (long) v;

//printk("%s pos=%p mcnt=%d\n", __func__, *pos, mcnt);
	++*pos;

	return (void *) (n - 2 > num_probes ? NULL : (n + 1));
}
static void instr_seq_stop(struct seq_file *seq, void *v)
{
//printk("%s v=%p\n", __func__, v);
}
static int instr_seq_show(struct seq_file *seq, void *v)
{
	int	i;
	long	n = (long) v;
	int	target;
	instr_probe_t *fbt = NULL;
	unsigned long size;
	unsigned long offset;
	char	*modname = NULL;
	char	name[KSYM_NAME_LEN];
	const	char	*cp;

//printk("%s v=%p\n", __func__, v);
	if (n == 1) {
		seq_printf(seq, "# patchpoint opcode inslen modrm name\n");
		return 0;
	}
	if (n > num_probes)
		return 0;

	/***********************************************/
	/*   Find first probe.			       */
	/***********************************************/
	target = n;
	for (i = 0; target > 0 && i < instr_probetab_size; i++) {
		fbt = instr_probetab[i];
		if (fbt == NULL)
			continue;
		target--;
		while (target > 0 && fbt) {
			if ((fbt = fbt->insp_hashnext) == NULL)
				break;
			target--;
		}
	}
	if (fbt == NULL)
		return 0;

	cp = my_kallsyms_lookup((unsigned long) fbt->insp_patchpoint, 
		&size, &offset, &modname, name);
	seq_printf(seq, "%ld %p %02x %d %2d %s:%s\n", n-1, 
		fbt->insp_patchpoint,
		fbt->insp_savedval,
		fbt->insp_inslen,
		fbt->insp_modrm,
		modname ? modname : "kernel",
		cp ? cp : "NULL");
	return 0;
}
static struct seq_operations seq_ops = {
	.start = instr_seq_start,
	.next = instr_seq_next,
	.stop = instr_seq_stop,
	.show = instr_seq_show,
	};
static int instr_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_ops);
}
static const struct file_operations instr_proc_fops = {
        .owner   = THIS_MODULE,
        .open    = instr_seq_open,
        .read    = seq_read,
        .llseek  = seq_lseek,
        .release = seq_release,
};

/**********************************************************************/
/*   Main starting interface for the driver.			      */
/**********************************************************************/
static const struct file_operations instr_fops = {
        .ioctl = instr_ioctl,
        .open = instr_open,
};

static struct miscdevice instr_dev = {
        MISC_DYNAMIC_MINOR,
        "instr",
        &instr_fops
};

static int initted;

int instr_init(void)
{	int	ret;
	struct proc_dir_entry *ent;

	ret = misc_register(&instr_dev);
	if (ret) {
		printk(KERN_WARNING "instr: Unable to register misc device\n");
		return ret;
		}

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "instr loaded: /dev/instr now available\n");

	if (instr_probetab_size == 0)
		instr_probetab_size = INSTR_PROBETAB_SIZE;

	instr_probetab_mask = instr_probetab_size - 1;
	instr_probetab =
	    kmem_zalloc(instr_probetab_size * sizeof (instr_probe_t *), KM_SLEEP);

	dtrace_invop_add(instr_invop);
	
	ent = create_proc_entry("dtrace/instr", 0444, NULL);
	if (ent)
		ent->proc_fops = &instr_proc_fops;

	if (dtrace_register("instr", &instr_attr, DTRACE_PRIV_KERNEL, 0,
	    &instr_pops, NULL, &instr_id)) {
		printk("Failed to register instr - insufficient privilege\n");
	}

	initted = 1;

	return 0;
}
void instr_exit(void)
{
	remove_proc_entry("dtrace/instr", 0);
	if (initted) {
		instr_cleanup(NULL);
		misc_deregister(&instr_dev);
	}

	printk(KERN_WARNING "instr driver unloaded.\n");
}

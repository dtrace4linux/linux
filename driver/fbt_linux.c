/**********************************************************************/
/*   This file contains much of the glue between the Solaris code in  */
/*   dtrace.c  and  the linux kernel. We emulate much of the missing  */
/*   functionality, or map into the kernel.			      */
/*   								      */
/*   This file was (incorrectly) previously marked as GPL, but it is  */
/*   actually  CDL  -  the  entire  dtrace  for  linux suite needs a  */
/*   consistent license arrangement to ensure we dont have difficult  */
/*   to justify licensing arrangements.				      */
/*   								      */
/*   Date: April 2008						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   $Header: Last edited: 28-Jul-2010 1.3 $ 			      */
/*
07-Jun-2009 PDF Add /proc/dtrace/fbt support to view the probe data
*/
/**********************************************************************/

//#pragma ident	"@(#)fbt.c	1.11	04/12/18 SMI"

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
#define regs pt_regs
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/privregs.h>
#include <sys/procfs_isa.h>

# undef NULL
# define NULL 0

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_DESCRIPTION("DTRACE/Function Boundary Tracing Driver");

#define	FBT_PUSHL_EBP		0x55
#define	FBT_MOVL_ESP_EBP0_V0	0x8b
#define	FBT_MOVL_ESP_EBP1_V0	0xec
#define	FBT_MOVL_ESP_EBP0_V1	0x89
#define	FBT_MOVL_ESP_EBP1_V1	0xe5
#define	FBT_REX_RSP_RBP		0x48

#define	FBT_POPL_EBP		0x5d
#define	FBT_RET			0xc3
#define	FBT_RET_IMM16		0xc2
#define	FBT_LEAVE		0xc9

/**********************************************************************/
/*   Following  are  generated  by  gcc/32bit compiler so we need to  */
/*   intercept them too.					      */
/**********************************************************************/
#define	FBT_PUSHL_EDI		0x57
#define	FBT_PUSHL_ESI		0x56
#define	FBT_PUSHL_EBX		0x53
#define	FBT_TEST_EAX_EAX	0x85 // c0
#define	FBT_SUBL_ESP_nn		0x83 // ec NN
#define	FBT_MOVL_nnn_EAX        0xb8 // b8 nnnnnnnn

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
#  if defined(__arm__)
#    define	FBT_PATCHVAL		0xe7f001f8 // undefined-instr (kprobes)
//#    define	FBT_PATCHVAL		0xe1200070 // BKPT
#  else
#    define	FBT_PATCHVAL		0xcc
#  endif
#else
#  if defined(__amd64)
#    define	FBT_PATCHVAL		0xcc
#  else
#    define	FBT_PATCHVAL		0xf0
#  endif
#endif

#define	FBT_ENTRY	"entry"
#define	FBT_RETURN	"return"
#define	FBT_ADDR2NDX(addr)	((((uintptr_t)(addr)) >> 4) & fbt_probetab_mask)
#define	FBT_PROBETAB_SIZE	0x8000		/* 32k entries -- 128K total */

typedef struct fbt_probe {
	struct fbt_probe *fbtp_hashnext;
	instr_t		*fbtp_patchpoint;
	int8_t		fbtp_rval;
	char		fbtp_enabled;
	instr_t		fbtp_patchval;
	instr_t		fbtp_savedval;
	uint8_t		fbtp_inslen;	/* Length of instr we are patching */
	char		fbtp_modrm;	/* Offset to modrm byte of instruction */
	uint8_t		fbtp_type;
	uintptr_t	fbtp_roffset;
	dtrace_id_t	fbtp_id;
	char		*fbtp_name;
	struct modctl	*fbtp_ctl;
	short		fbtp_loadcnt;
	char		fbtp_overrun;
	int		fbtp_symndx;
	unsigned int	fbtp_fired;
//	int		fbtp_primary;
	struct fbt_probe *fbtp_next;
# if defined(__arm__)
	/***********************************************/
	/*   Because  ARM doesnt handle a single-step  */
	/*   trap,  when  we  hit a probe, we need to  */
	/*   execute the original instruction and JMP  */
	/*   to  the  instruction  afterwards.  We do  */
	/*   this  in  the N-instruction buffer. This  */
	/*   avoids  a  lot  of  complexity  and race  */
	/*   conditions,  at  the  expense  of  extra  */
	/*   memory.				       */
	/***********************************************/
	int		fbtp_instr_buf[5];
# endif
} fbt_probe_t;

//static dev_info_t		*fbt_devi;
static dtrace_provider_id_t	fbt_id;
static fbt_probe_t		**fbt_probetab;
static int			fbt_probetab_size;
static int			fbt_probetab_mask;
static int			fbt_verbose = 0;
static int			num_probes;
static int			invop_loaded;

extern int dtrace_unhandled;
extern int fbt_name_opcodes;

static void fbt_provide_function(struct modctl *mp, 
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
fbt_is_patched(char *name, instr_t *addr)
{
	fbt_probe_t *fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if (fbt->fbtp_patchpoint == addr) {
			dtrace_printf("fbt:dup patch: %p %s\n", addr, name);
			return 1;
		}
	}
	return 0;
}
# if defined(__arm__)
/**********************************************************************/
/*   Find  the  3-instruction  buffer  for  ARM,  so when we hit the  */
/*   probe,  we can JMP to the fbtp_instr_buf, which consists of the  */
/*   first  instruction  being  the one we probed, followed by a JMP  */
/*   back   to   the  original  location  +1  (actually,  +4,  since  */
/*   instructions are 4-bytes long). Called from cpy_arm.c	      */
/**********************************************************************/
int *
fbt_get_instr_buf(instr_t *addr)
{
	fbt_probe_t *fbt = fbt_probetab[FBT_ADDR2NDX(addr)];
	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if (fbt->fbtp_patchpoint == addr)
			return fbt->fbtp_instr_buf;
	}
	return NULL;
}
# endif
/**********************************************************************/
/*   Here  from  INT3 interrupt context to see if the address we hit  */
/*   is one of ours.						      */
/**********************************************************************/
static int
fbt_invop(uintptr_t addr, uintptr_t *stack, uintptr_t rval, trap_instr_t *tinfo)
{
	uintptr_t stack0, stack1, stack2, stack3, stack4;
	fbt_probe_t *fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

//HERE();
//int dtrace_here = 1;
//if (dtrace_here) printk("fbt_invop:addr=%lx stack=%p eax=%lx\n", addr, stack, (long) rval);
	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
//if (dtrace_here) printk("patchpoint: %p rval=%x\n", fbt->fbtp_patchpoint, fbt->fbtp_rval);
		if ((uintptr_t)fbt->fbtp_patchpoint != addr)
			continue;

		/***********************************************/
		/*   If  probe  is  not  enabled,  but  still  */
		/*   fired,  then it *might have* overran. Likely cause is  */
		/*   cpu cache consistency issue - some other  */
		/*   CPU  hasnt  seen  the  breakpoint  being  */
		/*   removed.  Flag  it  so  we  can  show in  */
		/*   /proc/dtrace/fbt.			       */
		/*   Additionally, another provider (eg prov)  */
		/*   is  sitting  on the same function, so we  */
		/*   have to broadcast to both/all providers.  */
		/***********************************************/
		if (!fbt->fbtp_enabled) {
			fbt->fbtp_overrun = TRUE;
		}

		/***********************************************/
		/*   Always  handle  the  probe - if we dont,  */
		/*   nobody  else  will  know what to do with  */
		/*   it. Its possible somebody else does want  */
		/*   it,  e.g.  INSTR,  but we cant know who.  */
		/*   Probably  need  to  call  all providers,  */
		/*   rather than just the first one.	       */
		/***********************************************/
		if (1) {
			tinfo->t_opcode = fbt->fbtp_savedval;
//printk("fbt: opc=%p %p\n", tinfo->t_opcode, fbt->fbtp_savedval);
			tinfo->t_inslen = fbt->fbtp_inslen;
			tinfo->t_modrm = fbt->fbtp_modrm;
			if (!tinfo->t_doprobe)
				return fbt->fbtp_rval;
			fbt->fbtp_fired++;
			if (fbt->fbtp_roffset == 0) {
				struct pt_regs *ptregs = (struct pt_regs *) stack;
				/*
				 * When accessing the arguments on the stack,
				 * we must protect against accessing beyond
				 * the stack.  We can safely set NOFAULT here
				 * -- we know that interrupts are already
				 * disabled.
				 */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				CPU->cpu_dtrace_caller = ptregs->r_pc;
				stack0 = ptregs->c_arg0;
				stack1 = ptregs->c_arg1;
				stack2 = ptregs->c_arg2;
				stack3 = ptregs->c_arg3;
				stack4 = ptregs->c_arg4;
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);

				dtrace_probe(fbt->fbtp_id, stack0, stack1,
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

				dtrace_probe(fbt->fbtp_id, fbt->fbtp_roffset,
				    rval, 0, 0, 0);
				CPU->cpu_dtrace_caller = NULL;
			}

			return (fbt->fbtp_rval);
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
# elif defined(CONFIG_SMP)
	{int	i;
	for (i = 0; i < NR_CPUS; i++) {
		sum += local_read(&mp->ref[i].count);
	}
	}
# endif
	return sum;
}

/**********************************************************************/
/*   This  code  is called from dtrace.c (dtrace_probe_provide) when  */
/*   we  are arming the fbt functions. Since the kernel doesnt exist  */
/*   as a module, we need to handle that as a special case.	      */
/**********************************************************************/
caddr_t ktext;
caddr_t ketext;

void
fbt_provide_kernel()
{
	static struct module kern;
	int	n;
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
		printk("dtracedrv:fbt_provide_kernel: Cannot find _text/_stext\n");
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
printk("fbt_linux: size=0 %p %s\n", a, cp ? cp : "??");
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
			fbt_provide_function(&kern, 
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
fbt_provide_module(void *arg, struct modctl *ctl)
{	int	i;
	struct module *mp = (struct module *) ctl;
	char *modname = mp->name;
	char	*str = mp->strtab;
	char	*name;
    	par_module_t *pmp;
	int	ret;

# if 0
	struct module *mp = ctl->mod_mp;
	char *str = mp->strings;
	int nsyms = mp->nsyms;
	Shdr *symhdr = mp->symhdr;
	char *name;
	size_t symsize;

	/*
	 * Employees of dtrace and their families are ineligible.  Void
	 * where prohibited.
	 */
	if (strcmp(modname, "dtrace") == 0)
		return;

TODO();
	if (ctl->mod_requisites != NULL) {
		struct modctl_list *list;

		list = (struct modctl_list *)ctl->mod_requisites;

		for (; list != NULL; list = list->modl_next) {
			if (strcmp(list->modl_modp->mod_modname, "dtrace") == 0)
				return;
		}
	}

TODO();
	/*
	 * KMDB is ineligible for instrumentation -- it may execute in
	 * any context, including probe context.
	 */
	if (strcmp(modname, "kmdbmod") == 0)
		return;

	if (str == NULL || symhdr == NULL || symhdr->sh_addr == NULL) {
		/*
		 * If this module doesn't (yet) have its string or symbol
		 * table allocated, clear out.
		 */
		return;
	}

	symsize = symhdr->sh_entsize;

# endif

	int	init;

	if (strcmp(modname, "dtracedrv") == 0)
		return;

	/***********************************************/
	/*   Possible  memleak  here...we  allocate a  */
	/*   parallel  struct, but need to free if we  */
	/*   are offloaded.			       */
	/***********************************************/
	pmp = par_alloc(PARD_FBT, mp, sizeof *pmp, &init);
	if (pmp == NULL || pmp->fbt_nentries) {
		/*
		 * This module has some FBT entries allocated; we're afraid
		 * to screw with it.
		 */
		return;
	}

	if (dtrace_here) 
		printk("%s(%d):modname=%s num_symtab=%u\n", dtrace_basename(__FILE__), __LINE__, modname, (unsigned) mp->num_symtab);

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

		/*
		 * Due to 4524008, _init and _fini may have a bloated st_size.
		 * While this bug was fixed quite some time ago, old drivers
		 * may be lurking.  We need to develop a better solution to
		 * this problem, such that correct _init and _fini functions
		 * (the vast majority) may be correctly traced.  One solution
		 * may be to scan through the entire symbol table to see if
		 * any symbol overlaps with _init.  If none does, set a bit in
		 * the module structure that this module has correct _init and
		 * _fini sizes.  This will cause some pain the first time a
		 * module is scanned, but at least it would be O(N) instead of
		 * O(N log N)...
		 */
		if (strcmp(name, "_init") == 0)
			continue;

		if (strcmp(name, "_fini") == 0)
			continue;

		/*
		 * In order to be eligible, the function must begin with the
		 * following sequence:
		 *
		 * 	pushl	%esp
		 *	movl	%esp, %ebp
		 *
		 * Note that there are two variants of encodings that generate
		 * the movl; we must check for both.  For 64-bit, we would
		 * normally insist that a function begin with the following
		 * sequence:
		 *
		 *	pushq	%rbp
		 *	movq	%rsp, %rbp
		 *
		 * However, the compiler for 64-bit often splits these two
		 * instructions -- and the first instruction in the function
		 * is often not the pushq.  As a result, on 64-bit we look
		 * for any "pushq %rbp" in the function and we instrument
		 * this with a breakpoint instruction.
		 */
		instr = (uint8_t *)sym->st_value;
		limit = (uint8_t *)(sym->st_value + sym->st_size);
//printk("trying -- %02d %c:%s\n", i, sym->st_info, name);
//HERE();

		/***********************************************/
		/*   If  we  were  called because a module is  */
		/*   loaded  after  dtrace,  then mp contains  */
		/*   references to the .init section. We want  */
		/*   to  ignore these because the pages where  */
		/*   the  code resides will go away. Not only  */
		/*   dont   we   want   to   put   probes  on  */
		/*   non-existant pages, but /proc/dtrace/fbt  */
		/*   will  have problems and, fbt_enable will  */
		/*   if  that  page is now used by some other  */
		/*   driver.				       */
		/***********************************************/
		if (mp->module_init && mp->init_size &&
		    instr >= (uint8_t *) mp->module_init && 
		    instr < (uint8_t *) mp->module_init + mp->init_size) {
		    	continue;
		}

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
		ret = instr_in_text_seg(mp, name, sym);
		if (!ret)
			continue;

		/***********************************************/
		/*   We are good to go...		       */
		/***********************************************/
		fbt_provide_function(mp, pmp,
			modname, name, 
			(uint8_t *) sym->st_value, 
			instr, limit, i);
	}
}

static int
fbt_prov_entry(pf_info_t *infp, instr_t *instr, int size, int modrm)
{
	fbt_probe_t *fbt;

	/***********************************************/
	/*   Avoid patching a patched probe point.     */
	/***********************************************/
	if (*instr == 0xcc)
		return 1;

	/***********************************************/
	/*   This  is temporary. Because of the issue  */
	/*   of   %RIP   relative   addressing  modes  */
	/*   potentially not being addressable in our  */
	/*   single-step  jump  buffer, disable those  */
	/*   probes which look like one of these.      */
	/***********************************************/
	if (modrm >= 0 && (instr[modrm] & 0xc7) == 0x05) 
		return 1;

	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
	fbt->fbtp_name = infp->name;

	fbt->fbtp_id = dtrace_probe_create(fbt_id, infp->modname,
	    infp->name, FBT_ENTRY, 3, fbt);
	num_probes++;
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = infp->mp; // ctl;
	fbt->fbtp_loadcnt = get_refcount(infp->mp);
	fbt->fbtp_rval = DTRACE_INVOP_ANY;
	/***********************************************/
	/*   Save  potential overwrite of instruction  */
	/*   and  length,  because  we  will need the  */
	/*   entire  instruction  when we single step  */
	/*   over it.				       */
	/***********************************************/
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_inslen = size;
	fbt->fbtp_type = 0; /* entry */
//if (modrm >= 0 && (instr[modrm] & 0xc7) == 0x05) printk("modrm %s %p rm=%d\n", name, instr, modrm);
	fbt->fbtp_modrm = modrm;
	fbt->fbtp_patchval = FBT_PATCHVAL;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt->fbtp_symndx = infp->symndx;
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	infp->pmp->fbt_nentries++;
	infp->retptr = NULL;

	return 1;
}
/**********************************************************************/
/*   Create  a  new  entry  for  a  function  entry,  possible daisy  */
/*   chaining  the  multiple exits together (so that the probe names  */
/*   represent the entire suite).				      */
/**********************************************************************/
static int
fbt_prov_return(pf_info_t *infp, instr_t *instr, int size)
{
	fbt_probe_t *fbt;
	fbt_probe_t *retfbt = infp->retptr;

# if defined(__i386) || defined(__amd64)
	if (*instr == 0xcc)
		return 1;
# endif
	/***********************************************/
	/*   Sanity check for bad things happening.    */
	/***********************************************/
	if (fbt_is_patched(infp->name, instr)) {
		return 0;
	}

	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
	fbt->fbtp_name = infp->name;

	if (retfbt == NULL) {
		fbt->fbtp_id = dtrace_probe_create(fbt_id, infp->modname,
		    infp->name, FBT_RETURN, 3, fbt);
		num_probes++;
	} else {
		retfbt->fbtp_next = fbt;
		fbt->fbtp_id = retfbt->fbtp_id;
	}
	infp->retptr = fbt;
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = infp->mp; //ctl;
	fbt->fbtp_loadcnt = get_refcount(infp->mp);

	/***********************************************/
	/*   Swapped  sense  of  the  following ifdef  */
	/*   around so we are consistent.	       */
	/***********************************************/
	fbt->fbtp_rval = DTRACE_INVOP_ANY;
#if defined(__amd64)
	ASSERT(*instr == FBT_RET);
	fbt->fbtp_roffset =
	    (uintptr_t)(instr - (uint8_t *)infp->st_value);
#elif defined(__i386)
	fbt->fbtp_roffset =
	    (uintptr_t)(instr - (uint8_t *)infp->st_value) + 1;
#elif defined(__arm__)
	fbt->fbtp_roffset = 0;
#endif

	/***********************************************/
	/*   Save  potential overwrite of instruction  */
	/*   and  length,  because  we  will need the  */
	/*   entire  instruction  when we single step  */
	/*   over it.				       */
	/***********************************************/
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_inslen = size;
	fbt->fbtp_type = 1; /* return */
	fbt->fbtp_modrm = -1;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt->fbtp_symndx = infp->symndx;

	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	infp->pmp->fbt_nentries++;
	return 1;
}
/**********************************************************************/
/*   Common code to handle module and kernel functions.		      */
/**********************************************************************/
static void
fbt_provide_function(struct modctl *mp, par_module_t *pmp,
	char *modname, char *name, uint8_t *st_value,
	uint8_t *instr, uint8_t *limit, int symndx)
{
	pf_info_t	inf;

	inf.mp = mp;
	inf.pmp = pmp;
	inf.modname = modname;
	inf.name = name;
	inf.symndx = symndx;
	inf.st_value = st_value;
	inf.do_print = FALSE;

	inf.func_entry = fbt_prov_entry;
	inf.func_return = fbt_prov_return;

	dtrace_parse_function(&inf, instr, limit);
}
/*ARGSUSED*/
static void
fbt_destroy(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg, *next, *hash, *last;
	struct modctl *ctl = fbt->fbtp_ctl;
	struct module *mp = ctl;
	int ndx;

	do {
//printk("refc=%d load=%d\n", get_refcount(mp), fbt->fbtp_loadcnt);
		if (mp != NULL && get_refcount(mp) == fbt->fbtp_loadcnt) {
			if ((get_refcount(mp) == fbt->fbtp_loadcnt &&
			    mp->state == MODULE_STATE_LIVE)) {
			    	par_module_t *pmp = par_alloc(PARD_FBT, mp, sizeof *pmp, NULL);
				if (pmp && --pmp->fbt_nentries == 0)
					par_free(PARD_FBT, pmp);
			}
		}

		/*
		 * Now we need to remove this probe from the fbt_probetab.
		 */
		ndx = FBT_ADDR2NDX(fbt->fbtp_patchpoint);
		last = NULL;
		hash = fbt_probetab[ndx];

		while (hash != fbt) {
			ASSERT(hash != NULL);
			last = hash;
			hash = hash->fbtp_hashnext;
		}

		if (last != NULL) {
			last->fbtp_hashnext = fbt->fbtp_hashnext;
		} else {
			fbt_probetab[ndx] = fbt->fbtp_hashnext;
		}

		next = fbt->fbtp_next;
		kmem_free(fbt, sizeof (fbt_probe_t));

		fbt = next;
	} while (fbt != NULL);
}

/*ARGSUSED*/
static int
fbt_enable(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
	struct modctl *ctl = fbt->fbtp_ctl;
	struct module *mp = (struct module *) ctl;

# if 0
	ctl->mod_nenabled++;
# endif
	if (mp->state != MODULE_STATE_LIVE) {
		if (fbt_verbose) {
			cmn_err(CE_NOTE, "fbt is failing for probe %s "
			    "(module %s unloaded)",
			    fbt->fbtp_name, mp->name);
		}

		return 0;
	}

	/*
	 * Now check that our modctl has the expected load count.  If it
	 * doesn't, this module must have been unloaded and reloaded -- and
	 * we're not going to touch it.
	 */
	if (get_refcount(mp) != fbt->fbtp_loadcnt) {
		if (fbt_verbose) {
			cmn_err(CE_NOTE, "fbt is failing for probe %s "
			    "(module %s reloaded)",
			    fbt->fbtp_name, mp->name);
		}

		return 0;
	}

	for (; fbt != NULL; fbt = fbt->fbtp_next) {
		fbt->fbtp_enabled = TRUE;
		if (dtrace_here) 
			printk("fbt_enable:patch %p p:%02x %s\n", fbt->fbtp_patchpoint, fbt->fbtp_patchval, fbt->fbtp_name);
		if (memory_set_rw(fbt->fbtp_patchpoint, 1, TRUE)) {
			*fbt->fbtp_patchpoint = fbt->fbtp_patchval;
//printk("FBT: set pp=%p:%p\n", fbt->fbtp_patchpoint, *fbt->fbtp_patchpoint);
		}
	}
	return 0;
}

/*ARGSUSED*/
static void
fbt_disable(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
	struct modctl *ctl = fbt->fbtp_ctl;
	struct module *mp = (struct module *) ctl;

# if 0
	ASSERT(ctl->mod_nenabled > 0);
	ctl->mod_nenabled--;

	if (!ctl->mod_loaded || (ctl->mod_loadcnt != fbt->fbtp_loadcnt))
		return;
# else
	if (mp->state != MODULE_STATE_LIVE ||
	    get_refcount(mp) != fbt->fbtp_loadcnt)
		return;
# endif

	for (; fbt != NULL; fbt = fbt->fbtp_next) {
		if (dtrace_here) {
			printk("%s:%d: Disable %p:%s:%s\n", 
				__func__, __LINE__, 
				fbt->fbtp_patchpoint, 
				fbt->fbtp_ctl->name,
				fbt->fbtp_name);
		}
		/***********************************************/
		/*   Memory  should  be  writable,  but if we  */
		/*   failed  in  the  fbt_enable  code,  e.g.  */
		/*   because  kernel  freed an .init section,  */
		/*   then  dont  try and unpatch something we  */
		/*   didnt patch.			       */
		/*   We  were  trying to be clever and ensure  */
		/*   memory is writable but this seems to GPF  */
		/*   on  us.  Might be a temporary issue as I  */
		/*   fiddle with other things tho.	       */
		/***********************************************/
		if (*fbt->fbtp_patchpoint == fbt->fbtp_patchval) {
			*fbt->fbtp_patchpoint = fbt->fbtp_savedval;

			/***********************************************/
			/*   "Logically"  mark  probe  as gone. So we  */
			/*   can  detect overruns. But even tho it is  */
			/*   disabled,  doesnt  mean we cannot handle  */
			/*   it.				       */
			/***********************************************/
			fbt->fbtp_enabled = FALSE;
		}
	}
}

/*ARGSUSED*/
static void
fbt_suspend(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
	struct modctl *ctl = fbt->fbtp_ctl;
	struct module *mp = (struct module *) ctl;

# if 0
	ASSERT(ctl->mod_nenabled > 0);

	if (!ctl->mod_loaded || (ctl->mod_loadcnt != fbt->fbtp_loadcnt))
		return;
# else
	if (mp->state != MODULE_STATE_LIVE ||
	    get_refcount(mp) != fbt->fbtp_loadcnt)
		return;
# endif

	for (; fbt != NULL; fbt = fbt->fbtp_next)
		*fbt->fbtp_patchpoint = fbt->fbtp_savedval;
}

/*ARGSUSED*/
static void
fbt_resume(void *arg, dtrace_id_t id, void *parg)
{
	fbt_probe_t *fbt = parg;
	struct modctl *ctl = fbt->fbtp_ctl;
	struct module *mp = (struct module *) ctl;

# if 0
	ASSERT(ctl->mod_nenabled > 0);

	if (!ctl->mod_loaded || (ctl->mod_loadcnt != fbt->fbtp_loadcnt))
		return;
# else
	if (mp->state != MODULE_STATE_LIVE ||
	    get_refcount(mp) != fbt->fbtp_loadcnt)
		return;
# endif

	for (; fbt != NULL; fbt = fbt->fbtp_next)
		*fbt->fbtp_patchpoint = fbt->fbtp_patchval;
}

/*ARGSUSED*/
static void
fbt_getargdesc(void *arg, dtrace_id_t id, void *parg, dtrace_argdesc_t *desc)
{
	fbt_probe_t *fbt = parg;
	struct modctl *ctl = fbt->fbtp_ctl;
	struct module *mp = (struct module *) ctl;
	ctf_file_t *fp = NULL;
	ctf_funcinfo_t f;
//	int error;
	ctf_id_t argv[32], type;
	int argc = sizeof (argv) / sizeof (ctf_id_t);
//	const char *parent;

	if (mp->state != MODULE_STATE_LIVE ||
	    get_refcount(mp) != fbt->fbtp_loadcnt)
		return;

	if (fbt->fbtp_roffset != 0 && desc->dtargd_ndx == 0) {
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

	if (ctf_func_info(fp, fbt->fbtp_symndx, &f) == CTF_ERR)
		goto err;

	if (fbt->fbtp_roffset != 0) {
		if (desc->dtargd_ndx > 1)
			goto err;

		ASSERT(desc->dtargd_ndx == 1);
		type = f.ctc_return;
	} else {
		if (desc->dtargd_ndx + 1 > f.ctc_argc)
			goto err;

		if (ctf_func_args(fp, fbt->fbtp_symndx, argc, argv) == CTF_ERR)
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

static dtrace_pattr_t fbt_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_UNSTABLE, DTRACE_STABILITY_UNSTABLE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
};

static dtrace_pops_t fbt_pops = {
	NULL,
	fbt_provide_module,
	fbt_enable,
	fbt_disable,
	fbt_suspend,
	fbt_resume,
	fbt_getargdesc,
	NULL,
	NULL,
	fbt_destroy
};
static void
fbt_cleanup(dev_info_t *devi)
{
	if (invop_loaded)
		dtrace_invop_remove(fbt_invop);
	if (fbt_id)
		dtrace_unregister(fbt_id);

//	ddi_remove_minor_node(devi, NULL);
	if (fbt_probetab)
		kmem_free(fbt_probetab, fbt_probetab_size * sizeof (fbt_probe_t *));
	fbt_probetab = NULL;
	fbt_probetab_mask = 0;
}

# if 0
static int
fbt_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	if (fbt_probetab_size == 0)
		fbt_probetab_size = FBT_PROBETAB_SIZE;

	fbt_probetab_mask = fbt_probetab_size - 1;
	fbt_probetab =
	    kmem_zalloc(fbt_probetab_size * sizeof (fbt_probe_t *), KM_SLEEP);

	dtrace_invop_add(fbt_invop);

	if (ddi_create_minor_node(devi, "fbt", S_IFCHR, 0,
	    DDI_PSEUDO, NULL) == DDI_FAILURE ||
	    dtrace_register("fbt", &fbt_attr, DTRACE_PRIV_KERNEL, 0,
	    &fbt_pops, NULL, &fbt_id) != 0) {
		fbt_cleanup(devi);
		return (DDI_FAILURE);
	}

	ddi_report_dev(devi);
	fbt_devi = devi;

	return (DDI_SUCCESS);
}

static int
fbt_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	switch (cmd) {
	case DDI_DETACH:
		break;
	case DDI_SUSPEND:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	if (dtrace_unregister(fbt_id) != 0)
		return (DDI_FAILURE);

	fbt_cleanup(devi);

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
fbt_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	int error;

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		*result = (void *)fbt_devi;
		error = DDI_SUCCESS;
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)0;
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}

/*ARGSUSED*/
static int
fbt_open(dev_t *devp, int flag, int otyp, cred_t *cred_p)
{
	return (0);
}

static struct cb_ops fbt_cb_ops = {
	fbt_open,		/* open */
	nodev,			/* close */
	nulldev,		/* strategy */
	nulldev,		/* print */
	nodev,			/* dump */
	nodev,			/* read */
	nodev,			/* write */
	nodev,			/* ioctl */
	nodev,			/* devmap */
	nodev,			/* mmap */
	nodev,			/* segmap */
	nochpoll,		/* poll */
	ddi_prop_op,		/* cb_prop_op */
	0,			/* streamtab  */
	D_NEW | D_MP		/* Driver compatibility flag */
};

static struct dev_ops fbt_ops = {
	DEVO_REV,		/* devo_rev */
	0,			/* refcnt */
	fbt_info,		/* get_dev_info */
	nulldev,		/* identify */
	nulldev,		/* probe */
	fbt_attach,		/* attach */
	fbt_detach,		/* detach */
	nodev,			/* reset */
	&fbt_cb_ops,		/* driver operations */
  	NULL,			/* bus operations */
	nodev			/* dev power */
};
# endif

/**********************************************************************/
/*   Module interface to the kernel.				      */
/**********************************************************************/
#if 0
static int fbt_ioctl(struct inode *inode, struct file *file,
                     unsigned int cmd, unsigned long arg)
{
	return -EIO;
}
#endif
static int
fbt_open(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t
fbt_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
	return syms_read(fp, buf, len, off);
}
/**********************************************************************/
/*   User  is  writing to us to tell us where the kprobes symtab is.  */
/*   We  do this to avoid tripping over the GPL vs CDDL issue and to  */
/*   avoid  a tight compile-time coupling against bits of the kernel  */
/*   which are deemed private.					      */
/**********************************************************************/
static ssize_t 
fbt_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *pos)
{
	return syms_write(file, buf, count, pos);
}

/**********************************************************************/
/*   Code for /proc/dtrace/fbt					      */
/**********************************************************************/

static void *fbt_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos > num_probes)
		return 0;
	return (void *) (long) (*pos + 1);
}
static void *fbt_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{	long	n = (long) v;
//printk("%s pos=%p mcnt=%d\n", __func__, *pos, mcnt);
	++*pos;

	return (void *) (n - 2 > num_probes ? NULL : (n + 1));
}
static void fbt_seq_stop(struct seq_file *seq, void *v)
{
//printk("%s v=%p\n", __func__, v);
}
static int fbt_seq_show(struct seq_file *seq, void *v)
{
	int	i, s;
	int	n = (int) (long) v;
	int	target;
	fbt_probe_t *fbt = NULL;
	unsigned long size;
	unsigned long offset;
	char	*modname = NULL;
	char	name[KSYM_NAME_LEN];
	char	ibuf[64];
	char	*cp;

//printk("%s v=%p\n", __func__, v);
	if (n == 1) {
		seq_printf(seq, "# count patchpoint opcode inslen modrm name\n");
		return 0;
	}
	if (n > num_probes)
		return 0;

	/***********************************************/
	/*   Find  first  probe.  This  is incredibly  */
	/*   slow  and  inefficient, but we dont want  */
	/*   to waste memory keeping a list (an extra  */
	/*   ptr for each probe).		       */
	/***********************************************/
	target = n;
	for (i = 0; target > 0 && i < fbt_probetab_size; i++) {
		fbt = fbt_probetab[i];
		if (fbt == NULL)
			continue;
		target--;
		while (target > 0 && fbt) {
			if ((fbt = fbt->fbtp_hashnext) == NULL)
				break;
			target--;
		}
	}
	if (fbt == NULL)
		return 0;

	/***********************************************/
	/*   Dump the instruction so we can make sure  */
	/*   we  can  single  step them in the adjust  */
	/*   cpu code.				       */
	/*   When a module is loaded and we parse the  */
	/*   module,  this may include syms which may  */
	/*   get  unmapped  later  on.  So, trying to  */
	/*   access a patchpoint instruction can GPF.  */
	/*   We need to tread very carefully here not  */
	/*   to  GPF whilst catting /proc/dtrace/fbt.  */
	/*   We   actually   need  to  disable  these  */
	/*   probes.				       */
	/***********************************************/
//printk("fbtproc %p\n", fbt->fbtp_patchpoint);
	if (!validate_ptr(fbt->fbtp_patchpoint)) {
		s = 0xfff;
		strcpy(ibuf, "<unmapped>");
	} else {
		s = dtrace_instr_size((uchar_t *) fbt->fbtp_patchpoint);
		ibuf[0] = '\0';
		for (cp = ibuf, i = 0; i < s; i++) {
			snprintf(cp, sizeof ibuf - (cp - ibuf) - 3, "%02x ", 
				fbt->fbtp_patchpoint[i] & 0xff);
			cp += 3;
			}
	}
	cp = (char *) my_kallsyms_lookup((unsigned long) fbt->fbtp_patchpoint, 
		&size, &offset, &modname, name);
# if defined(__arm__)
	seq_printf(seq, "%d %04u%c %p %08x %d %2d %s:%s:%s %s\n", n-1, 
# else
	seq_printf(seq, "%d %04u%c %p %02x %d %2d %s:%s:%s %s\n", n-1, 
# endif
		fbt->fbtp_fired,
		fbt->fbtp_overrun ? '*' : ' ',
		fbt->fbtp_patchpoint,
		fbt->fbtp_savedval,
		fbt->fbtp_inslen,
		fbt->fbtp_modrm,
		modname ? modname : "kernel",
		cp ? cp : "NULL",
		fbt->fbtp_type ? "return" : "entry",
		ibuf);
	return 0;
}
struct seq_operations seq_ops = {
	.start = fbt_seq_start,
	.next = fbt_seq_next,
	.stop = fbt_seq_stop,
	.show = fbt_seq_show,
	};
static int fbt_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_ops);
}
static const struct file_operations fbt_proc_fops = {
        .owner   = THIS_MODULE,
        .open    = fbt_seq_open,
        .read    = seq_read,
        .llseek  = seq_lseek,
        .release = seq_release,
};

/**********************************************************************/
/*   Main starting interface for the driver.			      */
/**********************************************************************/
static const struct file_operations fbt_fops = {
	.owner = THIS_MODULE,
//	.ioctl = fbt_ioctl,
	.open = fbt_open,
	.read = fbt_read,
	.write = fbt_write,
};

static struct miscdevice fbt_dev = {
        MISC_DYNAMIC_MINOR,
        "fbt",
        &fbt_fops
};

static int initted;

int fbt_init(void)
{	int	ret;
	struct proc_dir_entry *ent;

	ret = misc_register(&fbt_dev);
	if (ret) {
		printk(KERN_WARNING "fbt: Unable to register misc device\n");
		return ret;
		}

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
//	printk(KERN_WARNING "fbt loaded: /dev/fbt now available\n");

	if (fbt_probetab_size == 0)
		fbt_probetab_size = FBT_PROBETAB_SIZE;

	fbt_probetab_mask = fbt_probetab_size - 1;
	fbt_probetab =
	    kmem_zalloc(fbt_probetab_size * sizeof (fbt_probe_t *), KM_SLEEP);

/*	dtrace_invop_add(fbt_invop);*/
	
	ent = create_proc_entry("dtrace/fbt", 0444, NULL);
	if (ent)
		ent->proc_fops = &fbt_proc_fops;

	if (dtrace_register("fbt", &fbt_attr, DTRACE_PRIV_KERNEL, 0,
	    &fbt_pops, NULL, &fbt_id)) {
		printk("Failed to register fbt - insufficient priviledge\n");
	}

	initted = 1;

	return 0;
}
/**********************************************************************/
/*   This  gets  called last so that FBT tries to get first crack at  */
/*   the  probes,  since it has the most likelihood of being used or  */
/*   hit.							      */
/**********************************************************************/
void
fbt_init2(void)
{
	invop_loaded = TRUE;
	dtrace_invop_add(fbt_invop);
}

void fbt_exit(void)
{
	remove_proc_entry("dtrace/fbt", 0);
	if (initted) {
		fbt_cleanup(NULL);
		misc_deregister(&fbt_dev);
	}

//	printk(KERN_WARNING "fbt driver unloaded.\n");
}

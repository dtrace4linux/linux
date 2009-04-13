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
/*   $Header: Last edited: 13-Apr-2009 1.1 $ 			      */
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

# undef NULL
# define NULL 0

MODULE_AUTHOR("Paul D. Fox");
MODULE_LICENSE("CDDL");
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
#  define	FBT_PATCHVAL		0xcc
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
	uint8_t		*fbtp_patchpoint;
	int8_t		fbtp_rval;
	uint8_t		fbtp_patchval;
	uint8_t		fbtp_savedval;
	uintptr_t	fbtp_roffset;
	dtrace_id_t	fbtp_id;
	char		*fbtp_name;
	struct modctl	*fbtp_ctl;
	int		fbtp_loadcnt;
	int		fbtp_symndx;
	int		fbtp_primary;
	struct fbt_probe *fbtp_next;
} fbt_probe_t;

//static dev_info_t		*fbt_devi;
static dtrace_provider_id_t	fbt_id;
static fbt_probe_t		**fbt_probetab;
static int			fbt_probetab_size;
static int			fbt_probetab_mask;
static int			fbt_verbose = 0;

static void fbt_provide_function(struct modctl *mp, 
    	par_module_t *pmp,
	char *modname, char *name, uint8_t *st_value, 
	uint8_t *instr, uint8_t *limit, int);

/**********************************************************************/
/*   For debugging - make sure we dont add a patch to the same addr.  */
/**********************************************************************/
static int
fbt_is_patched(uint8_t *addr)
{
	fbt_probe_t *fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if (fbt->fbtp_patchpoint == addr) {
			printk("Dup patch: %p\n", addr);
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
fbt_invop(uintptr_t addr, uintptr_t *stack, uintptr_t rval, unsigned char *opcode)
{
	uintptr_t stack0, stack1, stack2, stack3, stack4;
	fbt_probe_t *fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

HERE();
if (dtrace_here) printk("fbt_invop:addr=%lx stack=%p eax=%lx\n", addr, stack, (long) rval);
	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
if (dtrace_here) printk("patchpoint: %p rval=%x\n", fbt->fbtp_patchpoint, fbt->fbtp_rval);
		if ((uintptr_t)fbt->fbtp_patchpoint == addr) {
HERE();
			*opcode = fbt->fbtp_savedval;
			if (fbt->fbtp_roffset == 0) {
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
HERE();

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
fbt_provide_kernel()
{
	static struct module kern;
	int	n;
	const char *(*kallsyms_lookup)(unsigned long addr,
                        unsigned long *symbolsize,
                        unsigned long *offset,
                        char **modname, char *namebuf);
	caddr_t ktext = sym_get_static("_text");
	caddr_t ketext = sym_get_static("_etext");
	caddr_t a, aend;
	char	name[KSYM_NAME_LEN];

	if (kern.name[0])
		return;

	kallsyms_lookup = get_proc_addr("kallsyms_lookup");
	strcpy(kern.name, "kernel");
	/***********************************************/
	/*   Walk   the  code  segment,  finding  the  */
	/*   symbols,  and  creating a probe for each  */
	/*   one.				       */
	/***********************************************/
	for (n = 0, a = ktext; kallsyms_lookup && a < ketext; ) {
		const char *cp;
		unsigned long size;
		unsigned long offset;
		char	*modname = NULL;

//printk("lookup %p kallsyms_lookup=%p\n", a, kallsyms_lookup);
		cp = kallsyms_lookup((unsigned long) a, &size, &offset, &modname, name);

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

HERE();
	for (i = 1; i < mp->num_symtab; i++) {
		uint8_t *instr, *limit;
		Elf_Sym *sym = (Elf_Sym *) &mp->symtab[i];

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
		/***********************************************/
		{
		struct module_sect_attr
		{
		        struct module_attribute mattr;
		        char *name;
		        unsigned long address;
		};
		struct module_sect_attrs
		{
		        struct attribute_group grp;
		        unsigned int nsections;
		        struct module_sect_attr attrs[0];
		};
		struct module_sect_attrs *secp = (struct module_sect_attrs *) mp->sect_attrs;
		char	*secname = NULL;
//printk("attrs=%p shndx=%d\n", secp->attrs, sym->st_shndx);
		if (secp->attrs)
			secname = secp->attrs[sym->st_shndx].name;
		if (secname == NULL || strcmp(secname, ".text") != 0)
			continue;
//		printk("elf: %s info=%x other=%x shndx=%x sec=%p name=%s\n", name, sym->st_info, sym->st_other, sym->st_shndx, mp->sect_attrs, secname);
		}

		/***********************************************/
		/*   We are good to go...		       */
		/***********************************************/
		fbt_provide_function(mp, pmp,
			modname, name, 
			(uint8_t *) sym->st_value, 
			instr, limit, i);
		}
}

/**********************************************************************/
/*   Common code to handle module and kernel functions.		      */
/**********************************************************************/
static void
fbt_provide_function(struct modctl *mp, par_module_t *pmp,
	char *modname, char *name, uint8_t *st_value,
	uint8_t *instr, uint8_t *limit, int i)
{
	int	do_print = TRUE;
	int	invop = 0;
	fbt_probe_t *fbt, *retfbt;
	int	 size = -1;

	/***********************************************/
	/*   Dont  let  us  register  anything on the  */
	/*   notifier list(s) we are on, else we will  */
	/*   have recursion trouble.		       */
	/***********************************************/
	if (on_notifier_list(instr)) {
		printk("fbt_provide_function: Skip %s:%s - on notifier_chain\n",
			modname, name);
		return;
	}

#ifdef __amd64
	while (instr < limit) {
		if (*instr == FBT_PUSHL_EBP)
			break;

		if ((size = dtrace_instr_size(instr)) <= 0)
			break;

		instr += size;
	}

	/***********************************************/
	/*   Careful  in  case  we walked outside the  */
	/*   function    or    its   an   unsupported  */
	/*   instruction.			       */
	/***********************************************/
	if (instr >= limit) {
		printk("fbt:unhandled limit %s:%p %02x %02x %02x %02x %02x\n", name, instr, instr[0], instr[1], instr[2], instr[3], instr[4]);
		return;
	}
	if (*instr != FBT_PUSHL_EBP) {
		printk("fbt:unhandled instr %s:%p %02x %02x %02x %02x %02x\n", name, instr, instr[0], instr[1], instr[2], instr[3], instr[4]);
		return;
	}
	invop = DTRACE_INVOP_PUSHL_EBP;
#else
	/***********************************************/
	/*   GCC    generates   lots   of   different  */
	/*   assembler  functions plus we have inline  */
	/*   assembler  to  deal with - so we disable  */
	/*   this for now.			       */
	/***********************************************/
# define UNHANDLED_FBT() printk("fbt:unhandled instr %s:%p %02x %02x %02x %02x\n", \
			name, instr, instr[0], instr[1], instr[2], instr[3])

	switch (instr[0]) {
	  case FBT_PUSHL_EBP:
		invop = DTRACE_INVOP_PUSHL_EBP;
		break;

	  case FBT_PUSHL_EDI:
		invop = DTRACE_INVOP_PUSHL_EDI;
		break;

	  case FBT_PUSHL_ESI:
		invop = DTRACE_INVOP_PUSHL_ESI;
		break;

	  case FBT_PUSHL_EBX:
		invop = DTRACE_INVOP_PUSHL_EBX;
		break;

	  case FBT_TEST_EAX_EAX:
	  	if (instr[1] == 0xc0)
			invop = DTRACE_INVOP_TEST_EAX_EAX;
		else {
			UNHANDLED_FBT();
			return;
		}
		break;

	  case FBT_SUBL_ESP_nn:
	  	if (instr[1] == 0xec)
			invop = DTRACE_INVOP_SUBL_ESP_nn;
		else {
			UNHANDLED_FBT();
			return;
		}
		break;

	  case FBT_MOVL_nnn_EAX:
		invop = DTRACE_INVOP_MOVL_nnn_EAX;
		break;

	  case 0x31:
	  	if ((instr[1] & 0xc0) == 0xc0)
			invop = DTRACE_INVOP_XOR_REG_REG;
		else {
			UNHANDLED_FBT();
			return;
		}
		break;

	  case 0x50:
		invop = DTRACE_INVOP_PUSHL_EAX;
		break;

	  case 0x68:
		invop = DTRACE_INVOP_PUSH_NN32;
		break;

	  case 0x6a:
		invop = DTRACE_INVOP_PUSH_NN;
		break;

	  case 0x89:
	  	if ((instr[1] & 0xc0) == 0xc0)
			invop = DTRACE_INVOP_MOV_REG_REG;
		else {
			UNHANDLED_FBT();
			return;
		}
		break;

	  case 0xe9:
		invop = DTRACE_INVOP_JMP;
		break;

	  case 0xfa:
	  	invop = DTRACE_INVOP_CLI;
		break;

	  default:
		UNHANDLED_FBT();
		return;
	  }
#endif
	/***********************************************/
	/*   Allow  us  to  work on a single function  */
	/*   for  debugging/tracing  else we generate  */
	/*   too  much  printk() output and swamp the  */
	/*   log daemon.			       */
	/***********************************************/
//		do_print = strcmp(name, "init_memory_mapping") != NULL;

	/***********************************************/
	/*   Make  sure  this  doesnt overlap another  */
	/*   sym. We are in trouble when this happens  */
	/*   - eg we will mistaken what the emulation  */
	/*   is  for,  but  also,  it means something  */
	/*   strange  happens, like kernel is reusing  */
	/*   a  page  (eg  for init/exit section of a  */
	/*   module).				       */
	/***********************************************/
	if (fbt_is_patched(instr))
		return;

	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
			fbt->fbtp_name = name;

	fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
	    name, FBT_ENTRY, 3, fbt);
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = mp; // ctl;
	fbt->fbtp_loadcnt = get_refcount(mp);
	fbt->fbtp_rval = invop;
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt->fbtp_symndx = i;
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	if (do_print)
		printk("%d:alloc entry-patchpoint: %s %p invop=%x %02x %02x %02x\n", 
			__LINE__, 
			name, 
			fbt->fbtp_patchpoint, 
			fbt->fbtp_rval, instr[0], instr[1], instr[2]);

	pmp->fbt_nentries++;

	retfbt = NULL;

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
again:
	if (instr >= limit) {
		return;
	}

	/*
	 * If this disassembly fails, then we've likely walked off into
	 * a jump table or some other unsuitable area.  Bail out of the
	 * disassembly now.
	 */
	if ((size = dtrace_instr_size(instr)) <= 0)
		return;

//HERE();
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
		instr += size;
		goto again;
	  }
#else
	switch (*instr) {
/*		  case FBT_POPL_EBP:
		invop = DTRACE_INVOP_POPL_EBP;
		break;
	  case FBT_LEAVE:
		invop = DTRACE_INVOP_LEAVE;
		break;
*/
	  case FBT_RET:
		invop = DTRACE_INVOP_RET;
		break;
	  case FBT_RET_IMM16:
		invop = DTRACE_INVOP_RET_IMM16;
		break;
	  default:
		instr += size;
		goto again;
	  }
#endif

	/***********************************************/
	/*   Sanity check for bad things happening.    */
	/***********************************************/
	if (fbt_is_patched(instr)) {
		instr += size;
		goto again;
	}
	/*
	 * We have a winner!
	 */
	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
	fbt->fbtp_name = name;

	if (retfbt == NULL) {
		fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
		    name, FBT_RETURN, 3, fbt);
	} else {
		retfbt->fbtp_next = fbt;
		fbt->fbtp_id = retfbt->fbtp_id;
	}
	retfbt = fbt;
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = mp; //ctl;
	fbt->fbtp_loadcnt = get_refcount(mp);

	/***********************************************/
	/*   Swapped  sense  of  the  following ifdef  */
	/*   around so we are consistent.	       */
	/***********************************************/
	fbt->fbtp_rval = invop;
#ifdef __amd64
	ASSERT(*instr == FBT_RET);
	fbt->fbtp_roffset =
	    (uintptr_t)(instr - (uint8_t *)st_value);
#else
	fbt->fbtp_roffset =
	    (uintptr_t)(instr - (uint8_t *)st_value) + 1;

#endif

	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt->fbtp_symndx = i;

	if (do_print) {
		printk("%d:alloc return-patchpoint: %s %p: %02x %02x invop=%d\n", __LINE__, name, instr, instr[0], instr[1], invop);
	}

	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	pmp->fbt_nentries++;

	instr += size;
	goto again;
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
		if (mp != NULL && get_refcount(mp) == fbt->fbtp_loadcnt) {
			if ((get_refcount(mp) == fbt->fbtp_loadcnt &&
			    mp->state == MODULE_STATE_LIVE)) {
			    	par_module_t *pmp = par_alloc(mp, sizeof *pmp, NULL);
				if (--pmp->fbt_nentries == 0)
					par_free(pmp);
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
static void
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

		return;
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

		return;
	}
HERE();

	for (; fbt != NULL; fbt = fbt->fbtp_next) {
		if (dtrace_here) 
			printk("fbt_enable:patch %p p:%02x\n", fbt->fbtp_patchpoint, fbt->fbtp_patchval);
		if (memory_set_rw(fbt->fbtp_patchpoint, 1, TRUE))
			*fbt->fbtp_patchpoint = fbt->fbtp_patchval;
	}
HERE();
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
		/***********************************************/
		if (memory_set_rw(fbt->fbtp_patchpoint, 1, TRUE))
			*fbt->fbtp_patchpoint = fbt->fbtp_savedval;
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
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
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
	dtrace_invop_remove(fbt_invop);
	if (fbt_id)
		dtrace_unregister(fbt_id);

//	ddi_remove_minor_node(devi, NULL);
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
static int fbt_ioctl(struct inode *inode, struct file *file,
                     unsigned int cmd, unsigned long arg)
{
	return -EIO;
}
static int
fbt_open(struct inode *inode, struct file *file)
{
	return 0;
}
static ssize_t
fbt_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
	return -EIO;
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

static const struct file_operations fbt_fops = {
        .ioctl = fbt_ioctl,
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

	ret = misc_register(&fbt_dev);
	if (ret) {
		printk(KERN_WARNING "fbt: Unable to register misc device\n");
		return ret;
		}

	/***********************************************/
	/*   Helper not presently implemented :-(      */
	/***********************************************/
	printk(KERN_WARNING "fbt loaded: /dev/fbt now available\n");
/*{char buf[256];
extern int kallsyms_lookup_name(char *);
extern int kallsyms_lookup_size_offset(char *);
extern int kallsyms_op;
unsigned long p;

//p = kallsyms_lookup_name("kallsyms_init");
printk("fbt: kallsyms_op = %p\n", kallsyms_lookup_size_offset);
}
*/
	if (fbt_probetab_size == 0)
		fbt_probetab_size = FBT_PROBETAB_SIZE;

	fbt_probetab_mask = fbt_probetab_size - 1;
	fbt_probetab =
	    kmem_zalloc(fbt_probetab_size * sizeof (fbt_probe_t *), KM_SLEEP);

	dtrace_invop_add(fbt_invop);
	
	dtrace_register("fbt", &fbt_attr, DTRACE_PRIV_KERNEL, 0,
	    &fbt_pops, NULL, &fbt_id);

	initted = 1;

	return 0;
}
void fbt_exit(void)
{
	if (initted) {
		fbt_cleanup(NULL);
		misc_deregister(&fbt_dev);
	}

	printk(KERN_WARNING "fbt driver unloaded.\n");
}

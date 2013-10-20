/**********************************************************************/
/*   Linux  definitions  for  SDT probes declared dynamically in the  */
/*   code  where  they are invoked. sdt_linux.c will scan the module  */
/*   code   to   find  the  calling  sequence  and  function  naming  */
/*   convention to demangle the names.				      */
/**********************************************************************/

/**********************************************************************/
/*   A  probe is just a subroutine call to a function with a mangled  */
/*   name  which  we  parse  as  the module is loaded. We place int3  */
/*   instructions after the subroutine as a means of detecting calls  */
/*   which are dtrace/sdt calls.				      */
/**********************************************************************/
#define	DTRACE_SDT_DEFINE_PROBE(provider, module, name, func) \
	asm(".pushsection .dtrace_section, \"ax\"\n"); 			\
	asm(".global __dtrace_" #provider "___" #module "___" #name "___" #func "\n"); \
	asm(".type __dtrace_" #provider "___" #module "___" #name "___" #func ", @function\n"); \
	asm("__dtrace_" #provider "___" #module "___" #name "___" #func ": ret\n"); \
	asm(" int3 ; int3 ; int3 ; int3 \n"); \
	asm(".popsection\n");

#define DTRACE_SDT_PROBE(provider, module, name, func) \
	{extern void __dtrace_##provider##___##module##___##name##___##func(unsigned long);	\
	__dtrace_##provider##___##module##___##name##___##func ();		\
	DTRACE_SDT_DEFINE_PROBE(provider, module, name, func); \
	}

#define DTRACE_SDT_PROBE1(provider, module, name, func, arg1) \
	{extern void __dtrace_##provider##___##module##___##name##___##func(unsigned long);	\
	__dtrace_##provider##___##module##___##name##___##func ((unsigned long)arg1);		\
	DTRACE_SDT_DEFINE_PROBE(provider, module, name, func); \
	}



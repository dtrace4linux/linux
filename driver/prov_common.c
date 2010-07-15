/**********************************************************************/
/*   Common code for the providers.				      */
/*   								      */
/*   Date: July 20108						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: GPL3						      */
/*   								      */
/*   $Header: Last edited: 16-Jul-2010 1.9 $ 			      */
/**********************************************************************/

#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <dtrace_proto.h>

extern int dtrace_unhandled;
extern int fbt_name_opcodes;

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
//	do_print = strncmp(name, "update_process", 9) == NULL;

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
		if ((size = dtrace_instr_size(instr)) <= 0)
			return;

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


/**********************************************************************/
/*   This  file  emulates  an  x86-64  64-bit  processor. Its needed  */
/*   because  when  placing  probes  in the kernel, we overwrite the  */
/*   first  byte  of the kernel with a INT3 (0xcc) which mangles it.  */
/*   We   cannot   easily  undo  that  to  step  past  the  trapping  */
/*   instruction,  so  we need to emulate those instructions matched  */
/*   by fbt_linux.c 						      */
/*   								      */
/*   The  variation  of instructions is based on observation of what  */
/*   GCC  generates  for  kernels.  It  is  not  a  complete  64-bit  */
/*   emulator.							      */
/*   								      */
/*   The core of this is that we are sitting inside an interrupt, so  */
/*   we  may  have  to  mangle the return stack, and never exit from  */
/*   here.							      */
/*   								      */
/*   Date: Feb 2009  						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/**********************************************************************/

# if defined(__amd64)

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <sys/privregs.h>

/**********************************************************************/
/*   Called  from  interrupt  context when we hit one of our patched  */
/*   instructions.  Emulate  the  byte  that  is  missing  so we can  */
/*   continue (after notifying of the probe match).		      */
/**********************************************************************/
void
dtrace_cpu_emulate(int instr, int opcode, struct pt_regs *regs)
{
	/***********************************************/
	/*   We  patched an instruction so we need to  */
	/*   emulate  what would have happened had we  */
	/*   not done so.			       */
	/***********************************************/
	switch (instr) {
	  case DTRACE_INVOP_PUSHL_EBP:
PRINT_CASE(DTRACE_INVOP_PUSHL_EBP);
	  	regs->r_rsp -= sizeof(void *);
		((void **) regs->r_rsp)[0] = (void *) regs->r_fp;
	  	break;
	  case DTRACE_INVOP_POPL_EBP:
PRINT_CASE(DTRACE_INVOP_POPL_EBP);
break;
		TODO();
		break;

	  case DTRACE_INVOP_LEAVE:
PRINT_CASE(DTRACE_INVOP_LEAVE);
break;
		TODO();
		break;

	  case DTRACE_INVOP_NOP:
PRINT_CASE(DTRACE_INVOP_NOP);
		break;

	  case DTRACE_INVOP_RET:
PRINT_CASE(DTRACE_INVOP_RET);
		regs->r_pc = (greg_t) ((void **) regs->r_rsp)[0];
	  	regs->r_rsp += sizeof(void *);
	  	break;

	  case DTRACE_INVOP_MOVL_nnn_EAX:
PRINT_CASE(DTRACE_INVOP_MOVL_nnn_EAX);
		regs->r_rax = (greg_t) ((greg_t *) (regs->r_pc + 1))[0];
	  	regs->r_pc += 1 + sizeof(greg_t);
	  	break;

	  default:
	  	printk("Help me!!!!!!!!!!!!\n");
		break;
	  }
}
# endif

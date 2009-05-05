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
/*   								      */
/* $Header: Last edited: 26-Apr-2009 1.2 $ 			      */
/**********************************************************************/

# if defined(__amd64)

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <sys/privregs.h>

/**********************************************************************/
/*   Map x86 reg encoding to the pt_reg register offset.	      */
/**********************************************************************/
static const int reg_map[] = {
	//  rax  rcx  rdx  rbx  rsp  rbp  rsi  rdi
	     10,  11, 12,    5,  19,   4,  13,  14
	     };
static const int reg_map2[] = {
	//   r8  r9   r10  r11  r12  r13  r14  r15
	      9,  8,   7,   6,   3,   2,   1,    0
	     };

# define READ_REG(r) ((unsigned long *) regs)[r]

/**********************************************************************/
/*   Called  from  interrupt  context when we hit one of our patched  */
/*   instructions.  Emulate  the  byte  that  is  missing  so we can  */
/*   continue (after notifying of the probe match).		      */
/**********************************************************************/
void
dtrace_cpu_emulate(int instr, int opcode, struct pt_regs *regs)
{	int	r;
	unsigned long *sp = NULL;

	/***********************************************/
	/*   We  patched an instruction so we need to  */
	/*   emulate  what would have happened had we  */
	/*   not done so.			       */
	/***********************************************/
	switch (instr) {
	  case DTRACE_INVOP_PUSHL_REG: /* 0x50..0x57 */
PRINT_CASE(DTRACE_INVOP_PUSHL_REG);
//		__asm__("mov %%gs:0x20,%0":"=r" (sp));

	  	*sp -= sizeof(void *);
		r = reg_map[opcode & 0x7];
printk("rsp=%p %p rbp=%p r=%d rreg=%p\n", regs->r_rsp, sp, regs->r_rbp, r, &((unsigned long *) regs)[r]);
		*sp = READ_REG(r);
//((unsigned long *) regs->r_rsp)[0] = 0;
	  	break;

	  case DTRACE_INVOP_PUSHL_REG2: /* 0x41 + 0x50..0x57 */
PRINT_CASE(DTRACE_INVOP_PUSHL_REG2);
	  	regs->r_rsp -= sizeof(void *);
		r = reg_map2[*(unsigned char *) regs->r_pc & 0x7];
		((unsigned long *) regs->r_rsp)[0] = READ_REG(r);
		regs->r_pc++; // 2-byte opcode
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

	  case DTRACE_INVOP_XOR_REG_REG: { // 31 c0..c7 64-bit 'r' regs only
	  	int r = reg_map[*(unsigned char *) regs->r_pc & 0x7];
		((unsigned long *) regs)[r] = 0;
		regs->r_pc++;
	  	break;
		}
	  default:
	  	printk("Help me!!!!!!!!!!!! instr=%d\n", instr);
		break;
	  }
}
# endif

/**********************************************************************/
/*   The  code  in  this  file  implements  the pre/post single step  */
/*   handler.  When  we  hit  a  breakpoint, we need to single step,  */
/*   which we do by copying the instruction to a cpu specific buffer  */
/*   - to avoid race conditions.				      */
/*   								      */
/*   Things  like  CALL  and  JMP instructions need to be treated to  */
/*   ensure  we have no side effects, despite being in the temporary  */
/*   buffer.							      */
/*   								      */
/*   These  routines  are  called from dtrace_linux in the interrupt  */
/*   handlers, e.g. dtrace_int[13]_handler.			      */
/*   								      */
/*   Date: April 2008						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/*   								      */
/*   $Header: Last edited: 22-Nov-2011 1.6 $ 			      */
/**********************************************************************/

# if defined(__i386) || defined(__amd64)
#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <sys/privregs.h>

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
static void cpu_fix_rel(cpu_core_t *, cpu_trap_t *tp, unsigned char *orig_pc);
static uchar_t *cpu_skip_prefix(uchar_t *pc);

/**********************************************************************/
/*   After  a single step trap, we need to readjust the PC and maybe  */
/*   other things, so handle the exceptions here.		      */
/**********************************************************************/
int
cpu_adjust(cpu_core_t *this_cpu, cpu_trap_t *tp, struct pt_regs *regs)
{	uchar_t *pc = tp->ct_instr_buf;
	greg_t *sp;

	pc = cpu_skip_prefix(pc);

/* Just some debug to watch flag changes...
	if ((regs->r_rfl & X86_EFLAGS_IF) != (tp->ct_eflags & X86_EFLAGS_IF)) {
printk("flag change: %p: %02x : %02x %02x %02x %lx %lx\n",
tp->ct_orig_pc0, tp->ct_orig_pc0[0], *pc, 
tp->ct_orig_pc0[1], tp->ct_orig_pc0[2], regs->r_rfl, tp->ct_eflags);
	}
*/

	/***********************************************/
	/*   For  some  instructions,  we  need to be  */
	/*   careful of the IF/TF flags. So, do these  */
	/*   first.				       */
	/***********************************************/
	switch (*pc) {
	  case 0x9c: // pushfl
	  	/***********************************************/
	  	/*   If  we push flags, we have two copies on  */
	  	/*   the stack - the one for the instruction,  */
	  	/*   and the one for pt_regs we pushed on the  */
	  	/*   stack  as  part  of  the  trap  handler.  */
	  	/*   Update  the  invokers  flags, since ours  */
	  	/*   are useless at this point (or, that they  */
	  	/*   need to agree).			       */
		/*   We  just  pushed  the  flags,  but those  */
		/*   flags  would  contain the TF bit set, so  */
		/*   we  must turn that off. Also, dont touch  */
		/*   the  IF bit - that stays as whatever the  */
		/*   original  code  had it. If we attempt to  */
		/*   execute the switch-statement below, then  */
		/*   we  would  turn  off  IF,  and  hit  the  */
		/*   infamous    "BUG_ON/WARN_ONCE"    errors  */
		/*   complaining  that  a  caller  may  sleep  */
		/*   because interrupts are disabled.	       */
	  	/***********************************************/
		{greg_t *fl = (greg_t *) regs->r_sp;
		regs->r_rfl &= ~X86_EFLAGS_TF;
		/***********************************************/
		/*   Put back the original flags at the point  */
		/*   we were going to breakpoint.	       */
		/***********************************************/
		*fl = tp->ct_eflags;
		regs->r_pc = (greg_t) tp->ct_orig_pc;
	  	return FALSE;
		}

	  case 0x9d: // popf
	  	/***********************************************/
	  	/*   Very  rare  - a "subroutine" starting by  */
	  	/*   popping   the  flags.  e.g.  "not_same".  */
	  	/*   Really  limited  to  just assembler/trap  */
	  	/*   handler   code.  Believe  whatever  this  */
	  	/*   instruction has done to the flags.	       */
	  	/***********************************************/
		regs->r_pc = (greg_t) tp->ct_orig_pc;
		return FALSE;

	  case 0xcf: { //iret
//	  	greg_t *flp = (greg_t *) (&regs->r_rfl + 3 * sizeof(greg_t));
//dtrace_printf("iret rfl=%p\n", regs->r_rfl);
		regs->r_rfl = (regs->r_rfl & ~(X86_EFLAGS_TF));
//		*flp = (*flp & ~X86_EFLAGS_IF) | X86_EFLAGS_TF;
	  	return FALSE;
		}
	  }

	/***********************************************/
	/*   Put  back  the  IF  flag  the way it was  */
	/*   before  we turned off interrupts for the  */
	/*   single  step. Also, turn off the TF flag  */
	/*   because  we  will  have  finished single  */
	/*   stepping.				       */
	/***********************************************/
	regs->r_rfl = (regs->r_rfl & ~(X86_EFLAGS_TF|X86_EFLAGS_IF)) | 
		(tp->ct_eflags & (X86_EFLAGS_IF));

#define	SEE_ABOVE(opcode, text)
#define	SEE_BELOW(opcode, text)

	switch (*pc) {
	  case 0x70: case 0x71: case 0x72: case 0x73:
	  case 0x74: case 0x75: case 0x76: case 0x77:
	  case 0x78: case 0x79: case 0x7a: case 0x7b:
	  case 0x7c: case 0x7d: case 0x7e: case 0x7f:
	  	/***********************************************/
	  	/*   These can happen for instr provider.      */
		/*   If  the  instruction  pointer  isnt  the  */
		/*   'next'  instruction  then  a  branch was  */
		/*   taken,   so   handle   the   destination  */
		/*   relative jump.			       */
	  	/***********************************************/
		if ((uchar_t *) regs->r_pc != tp->ct_instr_buf + 2)
			regs->r_pc = (greg_t) tp->ct_orig_pc + (char) tp->ct_instr_buf[1];
		else
			regs->r_pc = (greg_t) tp->ct_orig_pc;
		break;

	  SEE_ABOVE(0x9c, pushfl)

	  case 0xc2: //ret imm16
	  case 0xc3: //ret
	  case 0xca: //lret nn
	  case 0xcb: //lret
	  case 0xcd: //int $n -- hope we dont do this in kernel space.
	  case 0xce: //into -- hope we dont do this in kernel space.
	  	break;

	  SEE_ABOVE(0x9c, iret);

	  case 0xe0: // LOOP rel8 - jump if count != 0
		regs->r_pc = (greg_t) tp->ct_orig_pc;
	  	if (regs->r_rcx) {
			regs->r_pc += (char) pc[1];
			return FALSE;
		}
		regs->r_rfl |= X86_EFLAGS_TF;
		return TRUE;
	  case 0xe1: // LOOPZ rel8 - jump if count != 0 AND ZF=1
		regs->r_pc = (greg_t) tp->ct_orig_pc;
	  	if (regs->r_rcx && regs->r_rfl & X86_EFLAGS_ZF) {
			regs->r_pc += (char) pc[1];
			return FALSE;
		}
		regs->r_rfl |= X86_EFLAGS_TF;
		return TRUE;
	  case 0xe2: // LOOPZ rel8 - jump if count != 0 AND ZF=0
		regs->r_pc = (greg_t) tp->ct_orig_pc;
	  	if (regs->r_rcx && !(regs->r_rfl & X86_EFLAGS_ZF)) {
			regs->r_pc += (char) pc[1];
			return FALSE;
		}
		regs->r_rfl |= X86_EFLAGS_TF;
		return TRUE;
	  case 0xe3: // JRCXZ rel8 - jump if RCX == 0
		regs->r_pc = (greg_t) tp->ct_orig_pc;
	  	if (regs->r_rcx == 0) {
			regs->r_pc += (char) pc[1];
			return FALSE;
		}
		regs->r_rfl |= X86_EFLAGS_TF;
		return TRUE;

	  SEE_BELOW(0xe4, in);
	  SEE_BELOW(0xe5, in);
	  SEE_BELOW(0xe6, out);
	  SEE_BELOW(0xe7, out);

	  case 0xe8: // CALLR nn32 call relative
		sp = (greg_t *) stack_ptr(regs);
		sp[0] = (greg_t) tp->ct_orig_pc;

		regs->r_pc = (long) tp->ct_orig_pc + 
			*(s32 *) (tp->ct_orig_pc - 4);
//printk("PC now %p\n", regs->r_pc);
		break;

	  case 0xe9: // 0xe9 nn32 jmp relative
		regs->r_pc = (greg_t) tp->ct_orig_pc +
			*(s32 *) (tp->ct_instr_buf + 1);
	  	break;
	  case 0xea: // 0xea jmp abs
	  	break;
	  case 0xeb: // 0xeb nn jmp relative
		regs->r_pc = (greg_t) tp->ct_orig_pc +
			*(char *) (tp->ct_instr_buf + 1);
	  	break;

	  case 0xf2: // REPZ -- need to keep going
	  	if ((regs->r_rfl & X86_EFLAGS_ZF) == 0 && regs->r_rcx) {
			regs->r_pc = (greg_t) tp->ct_instr_buf;
			regs->r_rfl |= X86_EFLAGS_TF;
			return TRUE;
		} else {
			regs->r_pc = (greg_t) tp->ct_orig_pc;
		}
	  	break;

	  case 0xf3: // REPNZ -- need to keep going
	  	if (regs->r_rfl & X86_EFLAGS_ZF && regs->r_rcx) {
			regs->r_pc = (greg_t) tp->ct_instr_buf;
			regs->r_rfl |= X86_EFLAGS_TF;
			return TRUE;
		} else {
			regs->r_pc = (greg_t) tp->ct_orig_pc;
		}
	  	break;

	  case 0xfa: // CLI
	  	/***********************************************/
	  	/*   Notreached   -   we   emulate   this  in  */
	  	/*   cpu_copy_instr  because  we  panic if we  */
	  	/*   single   step.   Not   sure  why  -  but  */
	  	/*   emulation is better anyhow.	       */
	  	/***********************************************/
	  	regs->r_rfl &= ~X86_EFLAGS_IF;
		regs->r_pc = (greg_t) tp->ct_orig_pc;
	  	break;
	  case 0xfb: // STI
	  	/***********************************************/
	  	/*   Notreached   -   we   emulate   this  in  */
	  	/*   cpu_copy_instr  because  we  panic if we  */
	  	/*   single   step.   Not   sure  why  -  but  */
	  	/*   emulation is better anyhow.	       */
	  	/***********************************************/
	  	regs->r_rfl |= X86_EFLAGS_IF;
		regs->r_pc = (greg_t) tp->ct_orig_pc;
	  	break;

	  SEE_BELOW(0xfc, cld);
	  SEE_BELOW(0xfd, std);

	  case 0xff:
	  	/***********************************************/
	  	/*   If  we  execute  a  CALL instruction, we  */
	  	/*   need  to  push  the  original  PC return  */
	  	/*   address, not our buffers!		       */
	  	/***********************************************/
	  	switch (pc[1] & 0xf0) {
		  case 0x10: // call/lcall *(%reg)
		  case 0x50: // call/lcall *nn(%reg)
		  case 0x90: // call/lcall *nn32(%reg)
		  case 0xd0: // call/lcall *%reg
		  	{greg_t *sp = (greg_t *) stack_ptr(regs);
			*sp = (greg_t) tp->ct_orig_pc;
		  	break;
			}

		  case 0x20: // jmp/ljmp *(%reg)
		  case 0x60: // jmp/ljmp *nnn(%reg)
		  case 0xa0: // jmp/ljmp *nnn(%reg)
		  case 0xe0: // jmp/ljmp *%reg
		  	break;
		  default:
		  	goto DEFAULT;
		  }
		break;

	  default:
	  DEFAULT: ;
		regs->r_pc = (greg_t) tp->ct_orig_pc;
//regs->r_rfl = (regs->r_rfl & ~(X86_EFLAGS_TF|X86_EFLAGS_IF)) | (this_cpu->cpuc_eflags & (X86_EFLAGS_IF));
		break;
	  }

	return FALSE;
}
/**********************************************************************/
/*   In  the  single  step  trap handler, get ready to step over the  */
/*   instruction. We copy it to a temp buffer, and set up so we know  */
/*   what to do on return after the trap.			      */
/*   								      */
/*   We  may  need to patch the instruction if it uses %RIP relative  */
/*   addressing mode (which x86-64 will do in the kernel).	      */
/*   								      */
/*   For  an  IRET,  we  cannot  single  step it, so we will need to  */
/*   emulate it instead.					      */
/**********************************************************************/
void
cpu_copy_instr(cpu_core_t *this_cpu, cpu_trap_t *tp, struct pt_regs *regs)
{

/*dtrace_printf("cpu_copy_instr: inslen=%d %02x %02x %02x %02x\n", tp->ct_tinfo.t_inslen,
tp->ct_tinfo.t_opcode,
((unsigned char *) regs->r_pc)[0],
((unsigned char *) regs->r_pc)[1],
((unsigned char *) regs->r_pc)[2]);*/
	/***********************************************/
	/*   Emulate  delicate  instructions. Without  */
	/*   this   we   can   hit  problems  in  the  */
	/*   syscall/interrupt handlers if we try and  */
	/*   single step the instructions.	       */
	/*   Its  also faster and less overhead if we  */
	/*   do this here.			       */
	/***********************************************/
	switch (tp->ct_tinfo.t_opcode) {
	  case 0xfa: // CLI
		regs->r_rfl &= ~X86_EFLAGS_IF;
		this_cpu->cpuc_mode = CPUC_MODE_IDLE;
		return;

	  case 0xfb: // STI
		regs->r_rfl |= X86_EFLAGS_IF;
		this_cpu->cpuc_mode = CPUC_MODE_IDLE;
		return;
	}

	tp->ct_instr_buf[0] = tp->ct_tinfo.t_opcode;
	dtrace_memcpy(&tp->ct_instr_buf[1], 
		(void *) regs->r_pc, 
		tp->ct_tinfo.t_inslen - 1);
	/***********************************************/
	/*   Put   a   NOP   instruction   after  the  */
	/*   instruction we are going to single step.  */
	/*   Some  instructions, like "MOV %CR3,%EAX"  */
	/*   will  step the next instruction also. By  */
	/*   doing  this,  we  regain control even if  */
	/*   this  happens. 0x90 is a NOP in i386 and  */
	/*   amd64.				       */
	/***********************************************/
	tp->ct_instr_buf[tp->ct_tinfo.t_inslen] = 0x90;
	/***********************************************/
	/*   Second  NOP  isnt  needed, but it is for  */
	/*   Xen.  This  may  be  a  cache  coherency  */
	/*   issue.				       */
	/***********************************************/
	tp->ct_instr_buf[tp->ct_tinfo.t_inslen+1] = 0x90;

//printk("step..len=%d %2x %2x\n", this_cpu->cpuc_tinfo.t_inslen, this_cpu->cpuc_instr_buf[0], this_cpu->cpuc_instr_buf[1]);

	/***********************************************/
	/*   Set  where  the next instruction is. The  */
	/*   instruction we hit could be a jump/call,  */
	/*   so  we  will  need to detect that on the  */
	/*   other  side of the single-step to ensure  */
	/*   we dont 'undo' the instruction.	       */
	/***********************************************/
	tp->ct_orig_pc = (unsigned char *) regs->r_pc + 
		tp->ct_tinfo.t_inslen - 1;

	/***********************************************/
	/*   May   need   to   rewrite   our   copied  */
	/*   instruction if we are doing RIP relative  */
	/*   addressing.			       */
	/***********************************************/
	cpu_fix_rel(this_cpu, tp,
		(unsigned char *) regs->r_pc - 1);

	/***********************************************/
	/*   Go  into  single  step mode, and we will  */
	/*   return to our fake buffer.		       */
	/***********************************************/
	tp->ct_eflags = regs->r_rfl;
//printk("origpc=%p instpc=%p rfl=%p\n", regs->r_pc, tp->cpuc_instr_buf, regs->r_rfl);
	regs->r_rfl |= X86_EFLAGS_TF;
	regs->r_rfl &= ~X86_EFLAGS_IF;
	regs->r_pc = (greg_t) tp->ct_instr_buf;
}

/**********************************************************************/
/*   Fix instructions which do %RIP relative addressing, such as MOV  */
/*   or  JMP,  since  our  instruction  trap  buffer is in the wrong  */
/*   place.							      */
/**********************************************************************/
static void
cpu_fix_rel(cpu_core_t *this_cpu, cpu_trap_t *tp, unsigned char *orig_pc)
{
#if defined(__amd64)
	unsigned char *pc = tp->ct_instr_buf;
	int	modrm = tp->ct_tinfo.t_modrm;

	if (modrm >= 0 && (pc[modrm] & 0xc7) == 0x05) {
//dtrace_printf("pc:%p modrm=%d\n", orig_pc, modrm);
		/***********************************************/
		/*   We got one. A common idiom, e.g.	       */
		/*   					       */
		/*   48 8b 05 00 00 00 00 mov 0x0(%rip),%rax   */
		/*   					       */
		/*   Now,  we  need  to  adjust  the relative  */
		/*   addressing   based  on  our  instruction  */
		/*   cache  buffer,  vs the original location  */
		/*   of the instruction.		       */
		/***********************************************/

		/***********************************************/
		/*   We  have  some  complications  here. The  */
		/*   nn(%rip)  offset might be 16 or 32 bits,  */
		/*   but  we  might  be  too  far away in our  */
		/*   buffer to handle the displacement, so we  */
		/*   will  need to map the instruction to use  */
		/*   a long offset.			       */
		/***********************************************/
		unsigned char *target;
		u32 new_target;

//printk("%p disp=%p\n", orig_pc, *(u32 *) (pc + modrm+1));
		target = orig_pc + tp->ct_tinfo.t_inslen
				+ *(s32 *) (&pc[modrm + 1]);
		new_target = target - (pc + tp->ct_tinfo.t_inslen);

//dtrace_printf("%p:%p modrm=%d %02x %02x %02x %02x %02x disp=%p ndisp=%p\n", orig_pc, pc, modrm, pc[0], pc[1], pc[2], pc[4], pc[5], target, new_target);
		*(u32 *) (pc + modrm + 1) = (u32) new_target;
	}
#endif

}
/**********************************************************************/
/*   Skip  leading prefix for an instruction, so we can test what it  */
/*   really is.							      */
/**********************************************************************/
static uchar_t *
cpu_skip_prefix(uchar_t *pc)
{
#if defined(__amd64)
	/***********************************************/
	/*   Watch out for IRETQ (48 CF)	       */
	/***********************************************/
	if (*pc == 0x48)
		pc++;
#endif

	/***********************************************/
	/*   Skip  prefix  bytes,  so we can find the  */
	/*   underlying opcode.			       */
	/***********************************************/
	while (1) {
		switch (*pc) {
		  case 0x26:
		  case 0x2e:
		  case 0x36:
		  case 0x3e:
		  case 0x64:
		  case 0x65:
		  case 0x66:
		  case 0x67:
		  case 0x9b:
		  case 0xf0: // lock
		  case 0xf2: // repnz
		  case 0xf3: // repz
		  	pc++;
			continue;
		  }
		break;
	}

	/***********************************************/
	/*   REX - access to new 8-bit registers.      */
	/***********************************************/
	if (*pc == 0x40)
		pc++;

	return pc;
}

# endif /* defined(__i386) || defined(__amd64) */


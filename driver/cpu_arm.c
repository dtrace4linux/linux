/**********************************************************************/
/*   ARM cpu single stepping.					      */
/**********************************************************************/
# if defined(__arm__)
#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <sys/privregs.h>

# include <asm/cacheflush.h>
# include <asm/traps.h>


# define	DTRACE_ARM_BREAKPOINT_INSTRUCTION       0xe7f001f8
//#define	DTRACE_ARM_BREAKPOINT_INSTRUCTION       0xe1200070

int dtrace_int3_handler(int type, struct pt_regs *regs);

/**********************************************************************/
/*   Code to catch the undefined instructions used by FBT.	      */
/**********************************************************************/
#define	__kprobes __attribute__((__section__(".kprobes.text")))
static int __kprobes 
dtrace_trap_handler(struct pt_regs *regs, unsigned int instr)
{	unsigned long flags;

	local_irq_save(flags);
	dtrace_int3_handler(3, regs);
	local_irq_restore(flags);

	return 0;
}
static struct undef_hook dtrace_arm_break_hook = {
	.instr_mask     = 0xffffffff,
	.instr_val      = DTRACE_ARM_BREAKPOINT_INSTRUCTION,
	.cpsr_mask      = MODE_MASK,
	.cpsr_val       = SVC_MODE,
	.fn             = dtrace_trap_handler,
};

/**********************************************************************/
/*   Called by intr.c (intr_init).				      */
/**********************************************************************/
void
arm_intr_init(void)
{
	void (*register_undef_hook)(struct undef_hook *) = get_proc_addr("register_undef_hook");
	if (register_undef_hook) {
	        register_undef_hook(&dtrace_arm_break_hook);
		printk("dtrace_arm_break_hook registered\n");
		}
}
void
arm_intr_exit(void)
{
	void (*unregister_undef_hook)(struct undef_hook *) = get_proc_addr("unregister_undef_hook");
	if (unregister_undef_hook) {
	        unregister_undef_hook(&dtrace_arm_break_hook);
	}
}

int
cpu_adjust(cpu_core_t *this_cpu, cpu_trap_t *tp, struct pt_regs *regs)
{
	TODO();
	return 0;
}

/**********************************************************************/
/*   On  a breakpoint or undefined instruction, which is an FBT type  */
/*   probe, we want to step past the instruction we patched. On x86,  */
/*   we  use  the  single-step trap to walk past the instruction. On  */
/*   ARM,  we  cannot  do  that  and  have  to  execute the original  */
/*   instruction in a temporary buffer.				      */
/**********************************************************************/
void
cpu_copy_instr(cpu_core_t *this_cpu, cpu_trap_t *tp, struct pt_regs *regs)
{	int	instr = tp->ct_tinfo.t_opcode;
	int	*fbt_get_instr_buf(instr_t *);
	int	*ip;

	this_cpu->cpuc_mode = CPUC_MODE_IDLE;

	/***********************************************/
	/*   Get   the   reserved   area   for   this  */
	/*   instruction. If we cannot find one (race  */
	/*   condition,  or  bug?)  then  just  force  */
	/*   caller to abort.			       */
	/***********************************************/
	if ((ip = fbt_get_instr_buf((instr_t *) regs->ARM_pc)) == NULL) {
		/***********************************************/
		/*   Turn off this probe.		       */
		/***********************************************/
		*(int *) regs->ARM_pc = instr;
		return;
	}

	/***********************************************/
	/*   On  a  LDR/STR  instruction  using  a PC  */
	/*   relative  offset,  we need to access the  */
	/*   original  PC  relative  address, not the  */
	/*   one in the tmp instruction buffer.	       */
	/***********************************************/
	if ((instr & 0x0e000000) == 0x04000000 &&  	// ldr/str
	     instr & 0x100000 &&			// ldr (not str)
	    (instr & 0xf0000) == 0xf0000) {		// r15/pc
		int offset = instr & 0xfff;
	    	int pc_addr = regs->ARM_pc + offset + 8;
		int reg = (instr & 0xf000) >> 12;
		// ldr rN, [pc, #+4] (ip[3])
		ip[0] = (instr & 0xfffff000) + 8;
		// ldr rN, [rN] 
		ip[1] = 0xe5900000 | (reg << 16) | (reg << 12);
		ip[2] = 0xe51ff004; // ldr pc, [pc, #-4] -- jmp
		ip[3] = regs->ARM_pc + 4; 		// .word orig_pc+4
		ip[4] = pc_addr; // .word orig relpc addr
		regs->ARM_pc = (unsigned long) ip;
		return;
	}

	// xxxx101L oooooooo oooooooo oooooooo  - branch
	if ((instr & 0x0e000000) == 0x0a000000) { // b/bl
		int L = instr & 0x01000000;
		int     offset = instr & 0xffffff;                    
		if (offset & 0x800000)                                
			offset |= 0xff000000;
		if (L) {
			regs->ARM_lr = regs->ARM_pc + 4;
		}
		regs->ARM_pc += offset * 4 + 8;
		return;
	}

	// xxxx100P USWLnnnn llllllll llllllll - block data transfer
	if (1 || (instr & 0x0e000000) == 0x08000000) { // push {...}
		ip[0] = instr;
		ip[1] = 0xe51ff004; // ldr pc, [pc, #-4]
		ip[2] = regs->ARM_pc + 4;
		regs->ARM_pc = (unsigned long) ip;
		return;
	}
	printk("emulate unhandled sp:%lx pc:%lx opc=%p regs=%p\n", regs->ARM_sp, regs->ARM_pc, (void *) tp->ct_tinfo.t_opcode, regs);
	regs->ARM_pc += 4;
}
int
dtrace_memcpy_with_error(void *kaddr, void *uaddr, size_t size)
{	size_t r = copy_from_user(kaddr, uaddr, size);

	return r == 0 ? size : 0;
}

/**********************************************************************/
/*   Called  by  fbt/prov_common  to  determine  if  this is a valid  */
/*   probable  instruction.  Since  we  emulate ARM instructions, we  */
/*   cannot allow unsupported probes to be available.		      */
/**********************************************************************/
int
is_probable_instruction(instr_t *insp, int is_entry)
{	int	instr = *insp;

	/***********************************************/
	/*   Ensure  on  a  4-byte  boundary  - avoid  */
	/*   picking  up  some data structures in the  */
	/*   code segment.			       */
	/***********************************************/
	if ((int) insp & 3)
		return FALSE;

	/***********************************************/
	/*   If  its  not a do-always condition code,  */
	/*   it  might  not be a real instruction, eg  */
	/*   __kstrtab_ip_defrag  shows  up like this  */
	/*   on  my  system. (We tend to see an ascii  */
	/*   text  like  instruction,  rather  than a  */
	/*   real instruction).			       */
	/***********************************************/
	if ((instr & 0xf0000000) != 0xe0000000)
		return FALSE;

	if (is_entry) {
		if ((instr & 0x0e000000) == 0x08000000) { // push {...}
			return TRUE;
		}
		if ((instr & 0x0e000000) == 0x0a000000) { // b/bl
			return TRUE;
		}

		if ((instr & 0x0e000000) == 0x04000000) { // ldr/str
			// 0xe59fc00c - ldr r12, [pc, #12]
			int     L = instr & 0x100000;
			int	reg2;

			if (L)
				reg2 = (instr & 0xf0000) >> 16;
			else
				reg2 = (instr & 0x0f000) >> 12;
			/***********************************************/
			/*   Avoid  PC  relative,  since  we will run  */
			/*   from a tmp buf.			       */
			/***********************************************/
			if (reg2 == 15) {
//if (instr == 0xe5900000) return TRUE;
# if 0
{
				static int num;
				int ip[8];
				int offset = instr & 0xffff;
				int pc_addr = (int) insp + offset + 8;
				int reg = (instr & 0xf000) >> 12;
				ip[0] = (instr & 0xffff0000) + (reg << 12) + 8; // ldr rN, [pc, #+4] (ip[3])
				ip[1] = 0xe5900000 | (reg << 16) | (reg << 12);
				ip[2] = 0xe51ff004; // ldr pc, [pc, #-4]
				ip[3] = 0;
				ip[4] = pc_addr; // .word orig relpc addr
				printk("%p:%p is_probable - PC use\n", insp, (void *) instr);
printk("num=%d 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", num++, ip[0], ip[1], ip[2], ip[3], ip[4]);
}
# endif
				return TRUE;
			}
			return TRUE;
		}

		return TRUE;
	}

	/***********************************************/
	/*   Return type instruction.		       */
	/***********************************************/

	/***********************************************/
	/*   pop {..., pc}			       */
	/***********************************************/
	if ((instr & 0x0e000000) == 0x08000000 &&
	    ((instr & 0xf0000) >> 16) == 13 && // sp
	    instr & 0x8000) // pc
		return TRUE;

	/***********************************************/
	/*   bx lr				       */
	/***********************************************/
	if (instr == 0xe12fff1e)
		return TRUE;

	return FALSE;
}

void dtrace_int1(void) {}
void dtrace_int3(void) {}
void dtrace_int_ipi(void) {}
void dtrace_page_fault(void) {}
void dtrace_int_dtrace_ret(void) {}
void dnr1_handler(void) {}
# endif /* defined(__arm__) */


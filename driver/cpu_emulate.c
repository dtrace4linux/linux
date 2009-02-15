/**********************************************************************/
/*   This file contains much of the glue between the Solaris code in  */
/*   dtrace.c  and  the linux kernel. We emulate much of the missing  */
/*   functionality, or map into the kernel.			      */
/*   								      */
/*   Date: Feb 2009  						      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/**********************************************************************/

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <sys/privregs.h>

/**********************************************************************/
/*   On  i386, we need to skip over the faulting instruction. Not so  */
/*   for amd64.							      */
/**********************************************************************/
# if defined(__amd64)
#	define	SKIP_OVER()
# else
#	define	SKIP_OVER() regs->r_pc++
# endif

/**********************************************************************/
/*   For i386, we are looking at this structure...		      */
/**********************************************************************/
	// struct pt_regs:
	//         unsigned long bx;  0		    -1  ss     20
	//         unsigned long cx;  4		    -2  sp     16
	//         unsigned long dx;  8		    -3  flags  12
	//         unsigned long si;  12	    -4  cs     8
	//         unsigned long di;  16	    -5  ip     4
	//         unsigned long bp;  20	      orig_ax  0
	//         unsigned long ax;  24	      fs
	//         unsigned long ds;  28	      es
	// 					      ds
	//         unsigned long es;  32	      ax
	//         unsigned long fs;  36	      bp
	//         /* int  gs; */		      di
	//         unsigned long orig_ax; 40	      si
	//         unsigned long ip;  44	      dx
	//         unsigned long cs;  48	      cx
	//         unsigned long flags; 52	      bx
	//         unsigned long sp;   56
	//         unsigned long ss;
/**********************************************************************/
/*   First part of returning from an interrupt.			      */
/**********************************************************************/
# define 	REGISTER_POP \
                        "mov %0, %%esp\n"	\
                        "pop %%ebx\n"		\
                        "pop %%ecx\n"		\
                        "pop %%edx\n"		\
                        "pop %%esi\n"		\
                        "pop %%edi\n"		\
                        "pop %%ebp\n"		\
                        "pop %%eax\n"		\
                        "pop %%ds\n"		\
                        "pop %%es\n"		\
			"pop %%fs\n"

/**********************************************************************/
/*   Called  from  interrupt  context when we hit one of our patched  */
/*   instructions.  Emulate  the  byte  that  is  missing  so we can  */
/*   continue (after notifying of the probe match).		      */
/**********************************************************************/
void
dtrace_cpu_emulate(int instr, struct pt_regs *regs)
{
	/***********************************************/
	/*   We  patched an instruction so we need to  */
	/*   emulate  what would have happened had we  */
	/*   not done so.			       */
	/***********************************************/
	switch (instr) {
	  case DTRACE_INVOP_PUSHL_EBP:
PRINT_CASE(DTRACE_INVOP_PUSHL_EBP);
/*
printk("ip:%x &regs=%p\n", regs->r_pc, &regs);
printk("eax:%p ebx:%p ecx:%p edx:%p\n", regs->r_rax, regs->r_rbx, regs->r_rcx, regs->r_rdx);
printk("esi:%p edi:%p ebp:%p esp:%p\n", regs->r_rsi, regs->r_rdi, regs->r_rbp, regs);
printk("ds:%04x es:%04x fs:%04x cs:%04x ss:%04x\n", regs->r_ds, regs->r_es, regs->r_fs, regs->r_cs, regs->r_ss);
printk("CODE is: %02x %02x %02x\n", ((unsigned char *) regs->r_pc)[0], ((unsigned char *) regs->r_pc)[1], ((unsigned char *) regs->r_pc)[2]);
dtrace_dump_mem32(regs, 8 * 8);
printk("Stack:\n");
dtrace_dump_mem32(regs->r_sp, 8 * 8);
*/
# if defined(__amd64)
	  	regs->r_rsp -= sizeof(void *);
		((void **) regs->r_rsp)[0] = (void *) regs->r_fp;
# else
		/***********************************************/
		/*   We    are    emulating   a   PUSH   %EBP  */
		/*   instruction.  We need to effect the push  */
		/*   before the invalid opcode trap occurred,  */
		/*   so we need to get into the inner core of  */
		/*   the  kernel  trap  return code. Since we  */
		/*   are  not  modifying  the  kernel, we can  */
		/*   just inline what would have happened had  */
		/*   we returned.			       */
		/*   					       */
		/*   So,  in  the  code below, we pop all the  */
		/*   registers,  but before the IRET, we need  */
		/*   to  execute  the emulation - by adding 1  */
		/*   to  the  EIP and moving the EBP to where  */
		/*   the  stack  is  going  to  point when we  */
		/*   return.				       */
		/***********************************************/
                __asm(
			REGISTER_POP

			"push %%eax\n"
			"mov 8(%%esp), %%eax\n"  // EIP
//			"inc %%eax\n" ; Dont need this now we come in as an INT3 trap
			"mov %%eax,4(%%esp)\n"
			"mov 12(%%esp),%%eax\n"  // CS
			"mov %%eax,8(%%esp)\n"
			"mov 16(%%esp),%%eax\n"  // Flags
			"mov %%eax,12(%%esp)\n"
			"mov %%ebp,16(%%esp)\n"  // emulated push EBP
			"pop %%eax\n"
			"iret\n"
                        :
                        : "a" (regs)
                        );

# endif

		SKIP_OVER();
	  	break;
	  case DTRACE_INVOP_PUSHL_EDI:
PRINT_CASE(DTRACE_INVOP_PUSHL_EDI);
break;
	  	regs->r_rsp -= sizeof(void *);
		((void **) regs->r_rsp)[0] = (void *) regs->r_rdi;
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
# if defined(__amd64)
		regs->r_pc = (greg_t) ((void **) regs->r_rsp)[0];
	  	regs->r_rsp += sizeof(void *);
# else
                __asm(
			REGISTER_POP

			"add $4, %%esp\n"

			/***********************************************/
			/*   Stack diagram:			       */
			//	16  return-addr for RET
			//	12  Flags
			//	8   CS
			//	4   IP
			//	0   tmp ax
			/***********************************************/

			"push %%eax\n"
			"mov 16(%%esp),%%eax\n"  // Execute the POP
			"mov %%eax,4(%%esp)\n"

			"mov 12(%%esp),%%eax\n" // Shuffle the stack
			"mov %%eax,16(%%esp)\n"

			"mov 8(%%esp),%%eax\n"
			"mov %%eax,12(%%esp)\n"

			"mov 4(%%esp),%%eax\n"
			"mov %%eax,8(%%esp)\n"
			"pop %%eax\n"
			"add $4, %%esp\n"
			"iret\n"
                        :
                        : "a" (regs)
                        );
# endif
	  	break;

	  default:
	  	printk("Help me!!!!!!!!!!!!\n");
		break;
	  }
}


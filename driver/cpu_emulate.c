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
# if defined(__amd64)
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
# else /* i386 */
void
dtrace_cpu_emulate(int instr, int opcode, struct pt_regs *regs)
{	int	nn;

/* some debug in case we need it...
printk("ip:%x &regs=%p\n", regs->r_pc, &regs);
printk("eax:%p ebx:%p ecx:%p edx:%p\n", regs->r_rax, regs->r_rbx, regs->r_rcx, regs->r_rdx);
printk("esi:%p edi:%p ebp:%p esp:%p\n", regs->r_rsi, regs->r_rdi, regs->r_rbp, regs);
printk("ds:%04x es:%04x fs:%04x cs:%04x ss:%04x\n", regs->r_ds, regs->r_es, regs->r_fs, regs->r_cs, regs->r_ss);
printk("CODE is: %02x %02x %02x\n", ((unsigned char *) regs->r_pc)[0], ((unsigned char *) regs->r_pc)[1], ((unsigned char *) regs->r_pc)[2]);
dtrace_dump_mem32(regs, 8 * 8);
printk("Stack:\n");
dtrace_dump_mem32(regs->r_sp, 8 * 8);
*/
	/***********************************************/
	/*   We  patched an instruction so we need to  */
	/*   emulate  what would have happened had we  */
	/*   not done so.			       */
	/***********************************************/
	switch (instr) {
	  case DTRACE_INVOP_PUSHL_EBP: // 55
PRINT_CASE(DTRACE_INVOP_PUSHL_EBP);
		/***********************************************/
		/*   We    are    emulating   a   PUSH   %EBP  */
		/*   instruction.  We need to effect the push  */
		/*   before the invalid opcode trap occurred,  */
		/*   so we need to get into the inner core of  */
		/*   the  kernel  trap  return code. Since we  */
		/*   are  not  modifying  the  kernel, we can  */
		/*   just inline what would have happened had  */
		/*   we returned.			       */
		/***********************************************/
                __asm(
			REGISTER_POP

			"push %%eax\n"
			"mov 8(%%esp), %%eax\n"  // EIP
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
	  	break;
	  case DTRACE_INVOP_PUSHL_EBX: // 53
PRINT_CASE(DTRACE_INVOP_PUSHL_EBX);
		/***********************************************/
		/*   We    are    emulating   a   PUSH   %EBP  */
		/*   instruction.  We need to effect the push  */
		/*   before the invalid opcode trap occurred,  */
		/*   so we need to get into the inner core of  */
		/*   the  kernel  trap  return code. Since we  */
		/*   are  not  modifying  the  kernel, we can  */
		/*   just inline what would have happened had  */
		/*   we returned.			       */
		/***********************************************/
                __asm(
			REGISTER_POP

			"push %%eax\n"
			"mov 8(%%esp), %%eax\n"  // EIP
			"mov %%eax,4(%%esp)\n"
			"mov 12(%%esp),%%eax\n"  // CS
			"mov %%eax,8(%%esp)\n"
			"mov 16(%%esp),%%eax\n"  // Flags
			"mov %%eax,12(%%esp)\n"
			"mov %%ebx,16(%%esp)\n"  // emulated push EBP
			"pop %%eax\n"
			"iret\n"
                        :
                        : "a" (regs)
                        );
	  	break;


	  case DTRACE_INVOP_PUSHL_EDI:
PRINT_CASE(DTRACE_INVOP_PUSHL_EDI);
		/***********************************************/
		/*   We    are    emulating   a   PUSH   %EDI  */
		/*   instruction.  We need to effect the push  */
		/*   before the invalid opcode trap occurred,  */
		/*   so we need to get into the inner core of  */
		/*   the  kernel  trap  return code. Since we  */
		/*   are  not  modifying  the  kernel, we can  */
		/*   just inline what would have happened had  */
		/*   we returned.			       */
		/***********************************************/
                __asm(
			REGISTER_POP

			"push %%eax\n"
			"mov 8(%%esp), %%eax\n"  // EIP
			"mov %%eax,4(%%esp)\n"
			"mov 12(%%esp),%%eax\n"  // CS
			"mov %%eax,8(%%esp)\n"
			"mov 16(%%esp),%%eax\n"  // Flags
			"mov %%eax,12(%%esp)\n"
			"mov %%edi,16(%%esp)\n"  // emulated push EDI
			"pop %%eax\n"
			"iret\n"
                        :
                        : "a" (regs)
                        );
	  	break;

	  case DTRACE_INVOP_PUSHL_ESI:
PRINT_CASE(DTRACE_INVOP_PUSHL_ESI);
		/***********************************************/
		/*   We    are    emulating   a   PUSH   %ESI  */
		/*   instruction.  We need to effect the push  */
		/*   before the invalid opcode trap occurred,  */
		/*   so we need to get into the inner core of  */
		/*   the  kernel  trap  return code. Since we  */
		/*   are  not  modifying  the  kernel, we can  */
		/*   just inline what would have happened had  */
		/*   we returned.			       */
		/***********************************************/
                __asm(
			REGISTER_POP

			"push %%eax\n"
			"mov 8(%%esp), %%eax\n"  // EIP
			"mov %%eax,4(%%esp)\n"
			"mov 12(%%esp),%%eax\n"  // CS
			"mov %%eax,8(%%esp)\n"
			"mov 16(%%esp),%%eax\n"  // Flags
			"mov %%eax,12(%%esp)\n"
			"mov %%esi,16(%%esp)\n"  // emulated push ESI
			"pop %%eax\n"
			"iret\n"
                        :
                        : "a" (regs)
                        );
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
	  	break;

	  case DTRACE_INVOP_MOVL_nnn_EAX:
PRINT_CASE(DTRACE_INVOP_MOVL_nnn_EAX);
		regs->r_rax = (greg_t) ((greg_t *) (regs->r_pc + 1))[0];
	  	regs->r_pc += 1 + sizeof(greg_t);
	  	break;

	  case DTRACE_INVOP_SUBL_ESP_nn:
PRINT_CASE(DTRACE_INVOP_SUBL_ESP_nn);
		nn = *(unsigned char *) (regs->r_pc + 1);
		if (nn == 8) {
	                __asm(
				REGISTER_POP
				"add $4, %%esp\n"
				//   Stack diagram:
				//     16  Flags
				//     12   CS
				//      8   IP		-> new flags
				//	4   tmp ax	-> new cs
				//	0   tmp ax	-> new ip

				"add $2,(%%esp)\n" // bump ip
				"push 4(%%esp)\n" // copy cs
				"push 4(%%esp)\n" // copy ip

				"push %%eax\n"
				"mov 20(%%esp),%%eax\n" // now move flags
				"mov %%eax,12(%%esp)\n"
				"pop %%eax\n"

				"iret\n"
	                        :
	                        : "a" (regs)
	                        );
		}
		if (nn == 12) {
	                __asm(
				REGISTER_POP
				"add $4, %%esp\n"
				//   Stack diagram:
				//     24  Flags
				//     20   CS
				//     16   IP
				//     12   tmp ax
				//	8   tmp ax
				//	4   tmp ax
				//	0   tmp ax
				"push %%eax\n"		// Save EAX but move out of harms
				"push %%eax\n"		// way
				"push %%eax\n"		// 
				"push %%eax\n"		// 
				"mov 16(%%esp),%%eax\n"
				"add $2,%%eax\n"          // Bump EIP
				"mov %%eax,4(%%esp)\n"
				"mov 20(%%esp),%%eax\n"
				"mov %%eax,8(%%esp)\n"
				"mov 24(%%esp),%%eax\n"
				"mov %%eax,12(%%esp)\n"
				"pop %%eax\n" // Restore EAX

				"iret\n"
	                        :
	                        : "a" (regs)
	                        );
		}

                __asm(
			REGISTER_POP
			"add $4, %%esp\n"

			/***********************************************/
			// 83 ec NN sub $nn,%esp
			//
			//   Stack diagram:
			//     24  Flags
			//     20   CS
			//     16   IP
			//     12   tmp ax
			//	8   tmp bx
			//	4   tmp cx
			//	0   tmp dx
			/***********************************************/

		/***********************************************/
		/*   WARNING!  This  code  wont  work  if the  */
		/*   first   block   of   PUSH   instructions  */
		/*   overlaps   the  block-of-3  PUSHes  down  */
		/*   below, e.g. SUB $n,ESP where $n <= 12.    */
		/***********************************************/
			"push %%eax\n"		// Save scratch regs
			"push %%ebx\n"
			"push %%ecx\n"
			"push %%edx\n"

			"mov 16(%%esp),%%eax\n"  // Get nnnn from mov instr
			"mov 1(%%eax),%%eax\n"
			"and $0x000000ff,%%eax\n" // EAX is now the offset
			
			"mov 24(%%esp),%%edx\n"	  // Grab Flags/CS/IP
			"mov 20(%%esp),%%ecx\n"	  // CS
			"mov 16(%%esp),%%ebx\n"	  // EIP
			"add $2,%%ebx\n"          // Bump EIP

			"sub %%eax,%%esp\n"	  // Relocate ESP ready for RETI
			"add $28,%%esp\n"
			"push %%edx\n"		  // Push Flags/CS/IP
			"push %%ecx\n"
			"push %%ebx\n"

			"mov -16(%%esp,%%eax,1), %%edx\n" // Restore our temp regs
			"mov -12(%%esp,%%eax,1), %%ecx\n"
			"mov -8(%%esp,%%eax,1), %%ebx\n"
			"mov -4(%%esp,%%eax,1), %%eax\n"

			"iret\n"
                        :
                        : "a" (regs)
                        );
	  	break;

	  default:
	  	printk("Help me!!!!!!!!!!!!\n");
		break;
	  }
}
# endif

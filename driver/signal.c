#include <dtrace_linux.h>
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <asm/pgtable.h>

/**********************************************************************/
/*   The  code  below  is  to  patch the signal delivery code in the  */
/*   kernel.  We need to watch signal delivery in the fasttrap code,  */
/*   else  a  taken-signal will save the address of the temp scratch  */
/*   buffer  and  not the original instruction, causing bad behavior  */
/*   when  returning  from  a  signal  handler  (illegal  opcode, or  */
/*   sigsegv).							      */
/**********************************************************************/

unsigned char *sig_send_ptr;

void	dtrace_safe_synchronous_signal(void);
void	dnr1_handler(struct pt_regs *);

void
init_signal_dummy(void)
{
	__asm(
	FUNCTION(dnr1_handler)
	"push	%r15\n"
	"push	%r14\n"
	"push	%r13\n"
	"push	%r12\n"
	"push	%r11\n"
	"push	%r10\n"
	"push	%r9\n"
	"push	%r8\n"
	"push	%rsi\n"
	"push	%rdi\n"
	"push	%rax\n"
	"push	%rbx\n"
	"push	%rcx\n"
	"push	%rdx\n"

//	"call dtrace_safe_synchronous_signal\n"
	"call sig_send_wrapper\n"

	"pop	%rdx\n"
	"pop	%rcx\n"
	"pop	%rbx\n"
	"pop	%rax\n"
	"pop	%rdi\n"
	"pop	%rsi\n"
	"pop	%r8\n"
	"pop	%r9\n"
	"pop	%r10\n"
	"pop	%r11\n"
	"pop	%r12\n"
	"pop	%r13\n"
	"pop	%r14\n"
	"pop	%r15\n"

	/***********************************************/
	/*   Pretend we never got here.		       */
	/***********************************************/
	"jmp *sig_send_ptr\n"

	END_FUNCTION(dnr1_handler)
	);
}

/**********************************************************************/
/*   Keep  track of what we patch so we can undo them when we unload  */
/*   the driver.						      */
/**********************************************************************/
typedef struct patch_t {
	uintptr_t	p_addr;
	uint32_t	p_val;
	} patch_t;
#define MAX_PATCHES 64
static patch_t ptbl[MAX_PATCHES];
static int pidx;

void
sig_send_wrapper(void)
{
//	printk("hello - wrapper!\n");
	dtrace_safe_synchronous_signal();
}

/**********************************************************************/
/*   We  found  someone  calling  our  target,  so  modify  the call  */
/*   instruction  to  jump  to  our  wrapper,  and track what we are  */
/*   modifying in case we get unloaded.				      */
/**********************************************************************/
static void
sig_send_callback(uint8_t *instr, int size)
{	uintptr_t	addr;
	uintptr_t	target = (uintptr_t) dnr1_handler;

	if (pidx + 1 >= MAX_PATCHES) {
		printk("patch table overflow\n");
		return;
	}

	/***********************************************/
	/*   Make sure we can modify the instruction.  */
	/***********************************************/
	set_page_prot((unsigned long) instr, 1, -1, _PAGE_RW);

	/***********************************************/
	/*   Compute  new  address  (rip-relative for  */
	/*   amd64).				       */
	/***********************************************/
	ptbl[pidx].p_addr = instr + 1;
	ptbl[pidx].p_val = *(uint32_t *) &instr[1];
	pidx++;
#if defined(__amd64)
	addr = (uintptr_t) target - (uintptr_t) (instr + 5);
//printk("cpatch %p:->%p %p\n", instr, target, addr);

	*(uint32_t *) &instr[1] = addr;
#else
	*(uint32_t *) &instr[1] = addr;
#endif
}
void
signal_init(void)
{	

	if ((sig_send_ptr = get_proc_addr("get_signal_to_deliver")) == NULL) {
		printk("init_signal: no function force_sig_info\n");
		return;
	}

	/***********************************************/
	/*   Intercept  calls to the function above -  */
	/*   this  is the place where we are about to  */
	/*   deliver  a  signal  to a process, in the  */
	/*   context  of  the  process itself, and we  */
	/*   may  need to reset the PC to the version  */
	/*   in  the  original  area,  not  the  temp  */
	/*   scratch buffer.			       */
	/***********************************************/
	dtrace_parse_kernel(PARSE_CALL, sig_send_callback, sig_send_ptr);
}
void
signal_fini(void)
{	int	p;

	/***********************************************/
	/*   Patch back any instructions we modified.  */
	/***********************************************/
	for (p = 0; p < pidx; p++) {
		*(uint32_t *) ptbl[p].p_addr = ptbl[p].p_val;
	}
}

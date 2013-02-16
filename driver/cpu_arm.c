/**********************************************************************/
/*   ARM cpu single stepping.					      */
/**********************************************************************/
# if defined(__arm__)
#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <sys/privregs.h>

int
cpu_adjust(cpu_core_t *this_cpu, cpu_trap_t *tp, struct pt_regs *regs)
{
	TODO();
	return 0;
}
void
cpu_copy_instr(cpu_core_t *this_cpu, cpu_trap_t *tp, struct pt_regs *regs)
{
	TODO();
}
int
dtrace_memcpy_with_error(void *kaddr, void *uaddr, size_t size)
{	size_t r = copy_from_user(kaddr, uaddr, size);

	return r == 0 ? size : 0;
}

void dtrace_int1(void) {}
void dtrace_int3(void) {}
void dtrace_int_ipi(void) {}
void dtrace_page_fault(void) {}
void dtrace_int_dtrace_ret(void) {}
void dnr1_handler(void) {}
# endif /* defined(__arm__) */


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
# endif /* defined(__arm__) */


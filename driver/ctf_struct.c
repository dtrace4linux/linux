/**********************************************************************/
/*   This  file  is  used  to  define  kernel  structures  so we can  */
/*   generate /usr/lib/dtrace/linux-`uname -r`.ctf. We dont use this  */
/*   in  the kernel driver, its just a vehicle for translating DWARF  */
/*   debug symbols to ctf format.				      */
/*   								      */
/*   In general, just stack up the #includes - the more the merrier.  */
/*   This  should  allow  lots of structures to be made available to  */
/*   the user.							      */
/*   								      */
/*   Without  this,  SDT  probes  wont  know  how to map the args[n]  */
/*   arguments.							      */
/*   								      */
/*   $Header:$							      */
/**********************************************************************/

#include <linux/swap.h> /* want totalram_pages */
#include <linux/delay.h>
#include <linux/slab.h>
#include <netinet/in.h>
#include <linux/cpumask.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sys.h>
#include <linux/thread_info.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/namei.h>
#include <linux/audit.h>
#include <linux/poll.h>
#include <asm/tlbflush.h>
#include <asm/current.h>
#include <asm/desc.h>

# include "ctf_struct.h"

psinfo_t p;
thread_t t;
dtrace_cpu_t dtrace_curcpu;
int cpu_dr7;
struct task_struct *cur_thread;
int	dtrace_cpu_id;

void
ctf_setup(void)
{
	/***********************************************/
	/*   Set    up    global/externally   visible  */
	/*   pointers  in  ctf_struct.c  so that user  */
	/*   code can access things like cur_thread.   */
	/***********************************************/
	cur_thread = get_current();

	dtrace_curcpu.cpu_id = smp_processor_id();

	dtrace_cpu_id = smp_processor_id();

}

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
/*   $Header: Last edited: 18-Dec-2011 1.1 $ 			      */
/**********************************************************************/

#define cpumask xx_cpumask
#include <linux/swap.h> /* want totalram_pages */
#include <linux/delay.h>
#include <linux/slab.h>
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
#include <linux/user.h>
#include <linux/stat.h>
#include <linux/mount.h>
#include <linux/interrupt.h>
#include <linux/in.h>
#include <linux/icmp.h>
#include <linux/net.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <asm/tlbflush.h>
#include <asm/current.h>
#include <asm/desc.h>
#include "../port.h"
#if defined(HAVE_LINUX_MIGRATE_H)
#  include <linux/migrate.h>
#endif

# include "ctf_struct.h"

/**********************************************************************/
/*   We (probably/do?) need to create an instance of each structure,  */
/*   because  if  we  dont,  we  may  end up seeing a "forward decl"  */
/*   reference when using print.				      */
/*   								      */
/*   Heres a way to look at the structures.			      */
/*   								      */
/*   dtrace -n 'fbt::sys_open:entry				      */
/*       {print(*((struct ipv6_pinfoz *) 0xffffffffa00209c0));}'      */
/*   								      */
/*   Here  I  have  picked  a  random  (but  defined)  address  from  */
/*   /proc/kallsyms,  eg  something in the data segment. I've used a  */
/*   random, high firing probe, to get some output.		      */
/*   								      */
/*   If  the  struct  exists,  we  will  see  a  pretty print of the  */
/*   structure.  If  it  doesnt,  we  will  see  something  like the  */
/*   following:							      */
/*   								      */
/*   2  15906                   sys_open:entry struct ipv6_pinfoz <forward decl> */
/*   								      */
/*   So,  you  need  to  create a concrete instance if the structure  */
/*   shows  up  as  a forward decl. I need to investigate why we get  */
/*   forward  refs  only  -  maybe  ctfconvert  only  picks concrete  */
/*   objects to turn into struct definitions.			      */
/**********************************************************************/
psinfo_t p;
thread_t t;
cpumask_t cpumask;
dtrace_cpu_t dtrace_curcpu;
buf_t 	dt_buf_t;	/* Unused - but need to declare some/anything */
int 	cpu_dr7;
struct task_struct *cur_thread;
int	dtrace_cpu_id;
struct tcp_sock 	tcp_sock;
struct udp_sock 	udp_sock;
struct user		user;
//struct user32		user32; not there in RH4
struct thread_info 	thread_info;
struct icmphdr 		icmphdr;
struct icmp_filter 	icmp_filter;
struct socket		socket;
struct proto_ops	proto_ops;
struct vfsmount		vfsmount;
struct irqaction	irqaction;

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

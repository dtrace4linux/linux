/**********************************************************************/
/*   This  file  contains  the  code for managing interrupts and the  */
/*   IDT.							      */
/*   								      */
/*   Date: December 2011					      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/*   								      */
/*   $Header: Last edited: 05-Feb-2012 1.15 $ 			      */
/**********************************************************************/

#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"

#include <linux/proc_fs.h>
#include <asm/current.h>
#include <asm/segment.h>
#include <asm/desc.h>
#include <sys/privregs.h>
#include <asm/desc.h>
#include <asm/tlbflush.h>
#include <linux/kallsyms.h>
#include <linux/seq_file.h>
#include <sys/trap.h>

/**********************************************************************/
/*   Backwards compat for older kernels.			      */
/**********************************************************************/
#if !defined(store_gdt)
#define store_gdt(ptr) asm volatile("sgdt %0":"=m" (*ptr))
#define store_idt(ptr) asm volatile("sidt %0":"=m" (*ptr))
#endif

struct x86_descriptor {
        unsigned short size;
        unsigned long address;
} __attribute__((packed)) ;

/**********************************************************************/
/*   Define  something  so  the  intr routines know whether to route  */
/*   back to the kernel, ie it is theirs, vs ours (NOTIFY_DONE).      */
/**********************************************************************/
# define	NOTIFY_KERNEL	1

/**********************************************************************/
/*   Pointer to the IDT size/address structure.			      */
/**********************************************************************/
static struct x86_descriptor *idt_descr_ptr;

/**********************************************************************/
/*   We need this to be in an executable page. kzalloc doesnt return  */
/*   us  one  of  these, and havent yet fixed this so we can make it  */
/*   executable,  so  for  now, this will do. As of 3.x kernels, BSS  */
/*   pages  are not executable. We should use a GCC __attribute__ to  */
/*   mark  this  in  an  executable  section  (.text?)  but .text is  */
/*   execute/no-write.    We    change    the    page    perms    in  */
/*   dtrace_linux_init.						      */
/**********************************************************************/
static cpu_core_t	cpu_core_exec[NCPU];
# define THIS_CPU() &cpu_core_exec[cpu_get_id()]

/**********************************************************************/
/*   Pointers to the real interrupt handlers so we can daisy chain.   */
/**********************************************************************/
void *kernel_int1_handler;
void *kernel_int3_handler;
void *kernel_int11_handler;
void *kernel_int13_handler;
void *kernel_double_fault_handler;
void *kernel_page_fault_handler;
void *kernel_int_dtrace_ret_handler;
int ipi_vector = 0xea-1-16; // very temp hack - need to find a free interrupt
void (*kernel_nmi_handler)(void);

/**********************************************************************/
/*   Kernel independent gate definitions.			      */
/**********************************************************************/
# define CPU_GATE_INTERRUPT 	0xE
# define GATE_DEBUG_STACK	0

/**********************************************************************/
/*   Define  the  descriptor table structures. Differing formats for  */
/*   64 and 32 bit, because 64-bit cpu has 64-bit pointers.	      */
/**********************************************************************/
struct gate64 {
        u16 offset_low;
        u16 segment;
        unsigned ist : 3, zero0 : 5, type : 5, dpl : 2, p : 1;
        u16 offset_middle;
        u32 offset_high;
        u32 zero1;
} __attribute__((packed));

struct gate32 {
        u16		base0;
        u16		segment;

	unsigned char	zero;
	unsigned char	flags;
	u16		base1;
} __attribute__((packed));

# if defined(__amd64)
typedef struct gate64 gate_t;

# elif defined(__i386)
typedef struct gate32 gate_t;

# else
#   error "Dont know how to handle GATEs on this cpu"
# endif

/**********************************************************************/
/*   Pointer to the kernels idt table.				      */
/**********************************************************************/
gate_t *idt_table_ptr;

extern int dtrace_printf_disable;

static	const char *(*my_kallsyms_lookup)(unsigned long addr,
                        unsigned long *symbolsize,
                        unsigned long *offset,
                        char **modname, char *namebuf);

/**********************************************************************/
/*   Prototypes.						      */
/**********************************************************************/
int dtrace_double_fault(void);
int dtrace_int1(void);
int dtrace_int3(void);
int dtrace_int11(void);
int dtrace_int13(void);
int dtrace_page_fault(void);
int dtrace_int_ipi(void);
int dtrace_int_nmi(void);
int dtrace_int_dtrace_ret(void);

/**********************************************************************/
/*   Update the IDT table for an interrupt. We just set the function  */
/*   pointer (code which will be in intr_x86-XX.S).		      */
/**********************************************************************/
void
set_idt_entry(int intr, unsigned long func)
{
static gate_t s;

#if defined(__amd64)
	int	type = CPU_GATE_INTERRUPT;
	int	dpl = 3;
	int	seg = __KERNEL_CS;

	dtrace_bzero(&s, sizeof s);

	s.offset_low = PTR_LOW(func);
	s.segment = seg;
        s.ist = GATE_DEBUG_STACK;
        s.p = 1;
        s.dpl = dpl;
        s.zero0 = 0;
        s.zero1 = 0;
        s.type = type;
        s.offset_middle = PTR_MIDDLE(func);
        s.offset_high = PTR_HIGH(func);

#elif defined(__i386)
	/***********************************************/
	/*   We  only care about the address, not the  */
	/*   other  attributes  which  belong  to the  */
	/*   kernel  and  version  of  the kernel. So  */
	/*   dont perturb these (dpl, stack, type).    */
	/***********************************************/
	s = *(gate_t *) &idt_table_ptr[intr];
//	s.segment = __KERNEL_CS;
	s.base0 = (u16) (func & 0xffff);
	s.base1 = (u16) ((func & 0xffff0000) >> 16);
//	int dpl = 3;
//	int type = CPU_GATE_INTERRUPT;
//	s.flags =	0x80 |		// present
//			(dpl << 5) |
//			type;		// CPU_GATE_INTERRUPT

	if (dtrace_here) {
		gate_t *g2 = &idt_table_ptr[intr];
		printk("set_idt_entry intr=%d\n  addr=%p sz=%d\n"
			"  s[0]=%lx %lx fl=%x addr=%04x%04x dpl=%d type=%x\n"
			"  i[0]=%lx %lx fl=%x addr=%04x%04x dpl=%d type=%x\n", 
			intr,
			&idt_table_ptr[intr], (int) sizeof s, 
			((long *) &s)[0], 
			((long *) &s)[1], 
			s.flags, s.base1, s.base0,
			(s.flags >> 5) & 0x3,
			s.flags & 0x1f,
			
			((long *) &idt_table_ptr[intr])[0], 
			((long *) &idt_table_ptr[intr])[1],
			g2->flags, g2->base1, g2->base0, 
			(g2->flags >> 5) & 0x3,
			g2->flags & 0x1f
			);
	}

#else
#  error "set_idt_entry: please help me"
#endif

	idt_table_ptr[intr] = s;
}

/**********************************************************************/
/*   Return the per-cpu area for the current cpu.		      */
/**********************************************************************/
cpu_core_t *
cpu_get_this(void)
{
	return THIS_CPU();
}

/**********************************************************************/
/*   'ipl'  function inside a probe - let us know if interrupts were  */
/*   enabled or not.						      */
/**********************************************************************/
int
dtrace_getipl(void)
{	cpu_core_t *this_cpu = THIS_CPU();

	return this_cpu->cpuc_regs->r_rfl & X86_EFLAGS_IF ? 0 : 1;
}

/**********************************************************************/
/*   Interrupt  handler  for a double fault. (We may not enable this  */
/*   if we dont see a need).					      */
/**********************************************************************/
int 
dtrace_double_fault_handler(int type, struct pt_regs *regs)
{
static int cnt;

dtrace_printf("double-fault[%lu]: CPU:%d PC:%p\n", cnt++, smp_processor_id(), (void *) regs->r_pc-1);
	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Handle  a  single step trap - hopefully ours, as we step past a  */
/*   probe, but, if not, it belongs to someone else.		      */
/**********************************************************************/
int 
dtrace_int1_handler(int type, struct pt_regs *regs)
{	cpu_core_t *this_cpu = THIS_CPU();
	cpu_trap_t	*tp;

//dtrace_printf("int1 PC:%p regs:%p CPU:%d\n", (void *) regs->r_pc-1, regs, smp_processor_id());
	/***********************************************/
	/*   Did  we expect this? If not, back to the  */
	/*   kernel.				       */
	/***********************************************/
	if (this_cpu->cpuc_mode != CPUC_MODE_INT1)
		return NOTIFY_KERNEL;

	/***********************************************/
	/*   Yup - its ours, so we can finish off the  */
	/*   probe.				       */
	/***********************************************/
	tp = &this_cpu->cpuc_trap[0];

	/***********************************************/
	/*   We  may  need to repeat the single step,  */
	/*   e.g. for REP prefix instructions.	       */
	/***********************************************/
	if (cpu_adjust(this_cpu, tp, regs) == FALSE)
		this_cpu->cpuc_mode = CPUC_MODE_IDLE;

	/***********************************************/
	/*   Dont let the kernel know this happened.   */
	/***********************************************/
//printk("int1-end: CPU:%d PC:%p\n", smp_processor_id(), (void *) regs->r_pc-1);
	return NOTIFY_DONE;
}
/**********************************************************************/
/*   Called  from  intr_<cpu>.S -- see if this is a probe for us. If  */
/*   not,  let  kernel  use  the normal notifier chain so kprobes or  */
/*   user land debuggers can have a go.				      */
/**********************************************************************/
unsigned long long cnt_int3_1;
unsigned long long cnt_int3_2;
unsigned long long cnt_int3_3;
int dtrace_int_disable;

void
recint(void)
{
	/***********************************************/
	/*   In  vmware/gdb,  we can put a breakpoint  */
	/*   here  and  decode  the stack to find out  */
	/*   what  nested  us. Just a temporary debug  */
	/*   utility.				       */
	/***********************************************/
	dtrace_printf("");
}
int 
dtrace_int3_handler(int type, struct pt_regs *regs)
{	cpu_core_t *this_cpu = THIS_CPU();
	cpu_trap_t	trap_info;
	cpu_trap_t	*tp;
	int	ret;

//dtrace_printf("#%lu INT3 PC:%p REGS:%p CPU:%d mode:%d\n", cnt_int3_1, regs->r_pc-1, regs, cpu_get_id(), this_cpu->cpuc_mode);
//preempt_disable();
	cnt_int3_1++;

	/***********************************************/
	/*   If  we  decided  to  abort doing probes,  */
	/*   just give up.			       */
	/***********************************************/
	if (dtrace_int_disable)
		goto switch_off;

	/***********************************************/
	/*   Are  we idle, or already single stepping  */
	/*   a probe?				       */
	/***********************************************/
	if (this_cpu->cpuc_mode == CPUC_MODE_IDLE) {
		/***********************************************/
		/*   Is this one of our probes?		       */
		/***********************************************/
		tp = &this_cpu->cpuc_trap[0];
		tp->ct_tinfo.t_doprobe = TRUE;
		/***********************************************/
		/*   Save   original   location   for   debug  */
		/*   purposes in cpu_x86.c		       */
		/***********************************************/
		tp->ct_orig_pc0 = (unsigned char *) regs->r_pc - 1;
		
		/***********************************************/
		/*   Protect  ourselves  in case dtrace_probe  */
		/*   causes a re-entrancy.		       */
		/***********************************************/
		this_cpu->cpuc_mode = CPUC_MODE_INT1;
		this_cpu->cpuc_regs_old = this_cpu->cpuc_regs;
		this_cpu->cpuc_regs = regs;

		/***********************************************/
		/*   Now try for a probe.		       */
		/***********************************************/
//dtrace_printf("CPU:%d ... calling invop\n", cpu_get_id());
		ret = dtrace_invop(regs->r_pc - 1, (uintptr_t *) regs, 
			regs->r_rax, &tp->ct_tinfo);
		if (ret) {
			cnt_int3_2++;
			/***********************************************/
			/*   Let  us  know  on  return  we are single  */
			/*   stepping.				       */
			/***********************************************/
			this_cpu->cpuc_mode = CPUC_MODE_INT1;
			/***********************************************/
			/*   Copy  instruction  in its entirety so we  */
			/*   can step over it.			       */
			/***********************************************/
			cpu_copy_instr(this_cpu, tp, regs);
//preempt_enable_no_resched();
//dtrace_printf("INT3 %p called CPU:%d good finish flags:%x\n", regs->r_pc-1, cpu_get_id(), regs->r_rfl);
			this_cpu->cpuc_regs = this_cpu->cpuc_regs_old;
this_cpu->cpuc_trap[0] = *tp;
			return NOTIFY_DONE;
		}
		this_cpu->cpuc_mode = CPUC_MODE_IDLE;

		/***********************************************/
		/*   Not  fbt/sdt,  so lets see if its a USDT  */
		/*   (user space probe).		       */
		/***********************************************/
//preempt_enable_no_resched();
		if (dtrace_user_probe(3, regs, (caddr_t) regs->r_pc, smp_processor_id())) {
			this_cpu->cpuc_regs = this_cpu->cpuc_regs_old;
			HERE();
			return NOTIFY_DONE;
		}

		/***********************************************/
		/*   Not ours, so let the kernel have it.      */
		/***********************************************/
//dtrace_printf("INT3 %p called CPU:%d hand over\n", regs->r_pc-1, cpu_get_id());
		this_cpu->cpuc_regs = this_cpu->cpuc_regs_old;
		return NOTIFY_KERNEL;
	}

	/***********************************************/
	/*   If   we   get  here,  we  hit  a  nested  */
	/*   breakpoint,  maybe  a  page  fault hit a  */
	/*   probe.  Sort  of  bad  news  really - it  */
	/*   means   the   core   of  dtrace  touched  */
	/*   something  that we shouldnt have. We can  */
	/*   handle  that  by disabling the probe and  */
	/*   logging  the  reason  so we can at least  */
	/*   find evidence of this.		       */
	/*   					       */
	/*   We  need to find the underlying probe so  */
	/*   we can unpatch it.			       */
	/*   					       */
	/*   If  we  trace  page_fault  -  the kernel  */
	/*   handler,  then  we  can  have  fbt_invop  */
	/*   invoke  dtrace_probe,  which in turn can  */
	/*   cause  a  page_fault (PTE syncing rather  */
	/*   than  a  genuine  page  fault). We would  */
	/*   like  to  allow page_fault to be probed,  */
	/*   but  we  need  to  avoid a runaway if we  */
	/*   nest.				       */
	/*   					       */
	/*   To  do  this properly requires we do the  */
	/*   cpu_copy_instr()  code,  like above, and  */
	/*   single  step, but we are possibly single  */
	/*   stepping  already.  Chances  are high we  */
	/*   are  faulting  in the invop/dtrace_probe  */
	/*   code. If we are, we can single step this  */
	/*   nested  trap.  If  not,  then the single  */
	/*   step  code is causing the problem and we  */
	/*   cannot do this without requiring a stack  */
	/*   of int1 areas to step over.	       */
	/***********************************************/
	cnt_int3_3++;
recint();
dtrace_printf("recursive-int[%lu]: PC:%p\n", cnt_int3_3, (void *) regs->r_pc-1);

	/***********************************************/
	/*   We  used  to  set  this  to  disable all  */
	/*   future probes. I dont think we need it -  */
	/*   we will just disable nested probes. This  */
	/*   allows   us   to   have  something  like  */
	/*   page_fault  nest  on us, but not disable  */
	/*   all  of the other probes. Not ideal. See  */
	/*   comment above.			       */
	/***********************************************/
	//dtrace_printf_disable = 1;

switch_off:
//	tp = &this_cpu->cpuc_trap[1];
	tp = &trap_info;
	tp->ct_tinfo.t_doprobe = FALSE;
	ret = dtrace_invop(regs->r_pc - 1, (uintptr_t *) regs, 
		regs->r_rax, &tp->ct_tinfo);
//preempt_enable_no_resched();
	/***********************************************/
	/*   If  we  own  the  breakpoint  area, then  */
	/*   unpatch  the instruction so we will lose  */
	/*   all future control. Better than panicing  */
	/*   the kernel.			       */
	/***********************************************/
	if (ret) {
		((unsigned char *) regs->r_pc)[-1] = tp->ct_tinfo.t_opcode;
		regs->r_pc--;
//preempt_enable();
		return NOTIFY_DONE;
	}

	/***********************************************/
	/*   Not ours, so let the kernel have it. For  */
	/*   example,    someone    else   is   doing  */
	/*   breakpoints,  and  we already fired on a  */
	/*   breakpoint belonging to us.	       */
	/***********************************************/
	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Segment not present interrupt. Not sure if we really care about  */
/*   these, but lets see what happens.				      */
/**********************************************************************/
unsigned long cnt_snp1;
unsigned long cnt_snp2;
int 
dtrace_int11_handler(int type, struct pt_regs *regs)
{
	cnt_snp1++;
	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_NOFAULT)) {
		cnt_snp2++;
		/***********************************************/
		/*   Bad user/D script - set the flag so that  */
		/*   the   invoking   code   can   know  what  */
		/*   happened,  and  propagate  back  to user  */
		/*   space, dismissing the interrupt here.     */
		/***********************************************/
        	DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
	        cpu_core[CPU->cpu_id].cpuc_dtrace_illval = read_cr2_register();
		return NOTIFY_DONE;
		}

	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Detect  us  causing a GPF, before we ripple into a double-fault  */
/*   and  a hung kernel. If we trace something incorrectly, this can  */
/*   happen.  If it does, just disable as much of us as we can so we  */
/*   can diagnose. If its not our GPF, then let the kernel have it.   */
/*   								      */
/*   Additionally, dtrace.c will tell this code that we are about to  */
/*   do  a dangerous probe, e.g. "copyinstr()" from an unvalidatable  */
/*   address.   This   is   allowed   to  trigger  a  GPF,  but  the  */
/*   DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT)  avoids  this  become  a  */
/*   user  core  dump  or  kernel  panic.  Honor  this  bit  setting  */
/*   appropriately for a safe journey through dtrace.		      */
/**********************************************************************/
unsigned long long cnt_gpf1;
unsigned long long cnt_gpf2;
int 
dtrace_int13_handler(int type, struct pt_regs *regs)
{	cpu_core_t *this_cpu = THIS_CPU();

	cnt_gpf1++;

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_NOFAULT)) {
		cnt_gpf2++;
		/***********************************************/
		/*   Bad user/D script - set the flag so that  */
		/*   the   invoking   code   can   know  what  */
		/*   happened,  and  propagate  back  to user  */
		/*   space, dismissing the interrupt here.     */
		/***********************************************/
        	DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
	        cpu_core[CPU->cpu_id].cpuc_dtrace_illval = read_cr2_register();
		regs->r_pc += dtrace_instr_size((uchar_t *) regs->r_pc);
//printk("int13 - gpf - %p\n", read_cr2_register());
		return NOTIFY_DONE;
		}

	/***********************************************/
	/*   If we are idle, it couldnt have been us.  */
	/***********************************************/
	if (this_cpu->cpuc_mode == CPUC_MODE_IDLE)
		return NOTIFY_KERNEL;

	/***********************************************/
	/*   This could be due to a page fault whilst  */
	/*   single stepping. We need to let the real  */
	/*   page fault handler have a chance (but we  */
	/*   prey it wont fire a probe, but it can).   */
	/***********************************************/
	this_cpu->cpuc_regs_old = this_cpu->cpuc_regs;
	this_cpu->cpuc_regs = regs;
	
	dtrace_printf("INT13:GPF %p called\n", regs->r_pc-1);
	dtrace_printf_disable = 1;
	dtrace_int_disable = TRUE;

	this_cpu->cpuc_regs = this_cpu->cpuc_regs_old;
	return NOTIFY_DONE;
}
/**********************************************************************/
/*   Handle  T_DTRACE_RET  interrupt  - invoked by the fasttrap (pid  */
/*   provider) to handle return after handling a user space probe.    */
/**********************************************************************/
unsigned long long cnt_0x80;

int 
dtrace_int_dtrace_ret_handler(int type, struct pt_regs *regs)
{
	cnt_0x80++;
	if (dtrace_user_probe(T_DTRACE_RET, regs, (caddr_t) regs->r_pc, smp_processor_id())) {
		return NOTIFY_DONE;
	}
	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Handle  a  page  fault  - if its us being faulted, else we will  */
/*   pass  on  to  the kernel to handle. We dont care, except we can  */
/*   have  issues  if  a  fault fires whilst we are trying to single  */
/*   step the kernel.						      */
/**********************************************************************/
unsigned long long cnt_pf1;
unsigned long long cnt_pf2;

int 
dtrace_int_page_fault_handler(int type, struct pt_regs *regs)
{
//dtrace_printf("PGF %p called pf%d\n", regs->r_pc-1, cnt_pf1);
	cnt_pf1++;
	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_NOFAULT)) {
		cnt_pf2++;

		/***********************************************/
		/*   Bad user/D script - set the flag so that  */
		/*   the   invoking   code   can   know  what  */
		/*   happened,  and  propagate  back  to user  */
		/*   space, dismissing the interrupt here.     */
		/***********************************************/
        	DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
	        cpu_core[CPU->cpu_id].cpuc_dtrace_illval = read_cr2_register();

		/***********************************************/
		/*   Skip the offending instruction, which is  */
		/*   probably just a MOV instruction.	       */
		/***********************************************/
		regs->r_pc += dtrace_instr_size((uchar_t *) regs->r_pc);
		return NOTIFY_DONE;
		}

	return NOTIFY_KERNEL;
}
/**********************************************************************/
/*   Intercept  SMP  IPI  inter-cpu  interrupts  so we can implement  */
/*   xcall. (Done in intr-x86_XX.S)				      */
/**********************************************************************/
extern unsigned long cnt_ipi1;

/**********************************************************************/
/*   Code to implement /proc/dtrace/idt.			      */
/**********************************************************************/
static void *gdt_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos > GDT_ENTRIES)
		return 0;
	return (void *) (long) (*pos + 1);
}
static void *gdt_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{	long	n = (long) v;
	++*pos;

	return (void *) (n - 2 > GDT_ENTRIES ? NULL : (void *) (n + 1));
}
static void gdt_seq_stop(struct seq_file *seq, void *v)
{
}
/**********************************************************************/
/*   This  code  was  designed  to  help me debug page fault handler  */
/*   issues.  Its  not  complete  (no amd64 output), and the i386 is  */
/*   questionable.						      */
/**********************************************************************/
static int gdt_seq_show(struct seq_file *seq, void *v)
{	int	n = (int) (long) v;
	unsigned long *g;
	unsigned long *gdt_table_ptr;
	struct x86_descriptor desc;

	store_gdt(&desc);
	if (n == 1) {
		seq_printf(seq, "GDT: %p entries=%d\n",
			(void *) desc.address, desc.size);
		seq_printf(seq, "#Ent base limit flags...\n");
		return 0;
	}
	if (n > GDT_ENTRIES + 1)
		return 0;

	gdt_table_ptr = (unsigned long *) desc.address;
	g = &gdt_table_ptr[(n - 2) * 2];
# if defined(__amd64)

# elif defined(__i386)
	{
	unsigned long base0 = (g[0] & 0xffff0000) >> 16;
	unsigned long base1 = (g[1] & 0xff000000) >> 24;
	unsigned long base2 =  g[1] & 0x000000ff;
	unsigned long lim0 = g[1] & 0xffff;
	unsigned long lim1 = (g[0] >> 16) & 0x0f;
	unsigned long lim = (lim1 << 16) | lim0;
	unsigned int access = (g[1] & 0xff00) >> 8;
	unsigned int flags = (g[1] >> 20) & 0x0f;
	unsigned long addr = base0 | (base1 << 16) | (base2 << 24);
	if ((g[0] & 0x8000) == 0) {
		seq_printf(seq, "%02x not-present\n", n - 2);
		return 0;
	}
	if ((flags & 0x08) == 0) {
		lim <<= 12;
		lim |= 0xfff;
	}
	seq_printf(seq, "%02x %s base=%p lim=%08lx %s pr=%d %s %s %s %s %s %s\n",
		n-2,
		(access & (0x08 | 0x02)) == (0x08 | 0x02) ? "CodeRW" :
		(access & (0x08 | 0x02)) == (0x08       ) ? "CodeRO" :
		(access & (0x08 | 0x02)) == (       0x02) ? "DataRW" :
		(access & (0x08 | 0x02)) == (       0000) ? "DataRO" : "????",
		(void *) addr,
		lim,
		access & 0x80 ? " P" : "NP",
		(access & 0x60) >> 5,
		access & 0x08 ? "ex" : "nx",
		access & 0x04 ? " dc" : "ndc",
		access & 0x02 ? "rw" : "ro",
		access & 0x01 ? "ac" : "na",
		flags & 0x08 ? "1b" : "4k",
		flags & 0x04 ? "16" : "32"
		);
	}
# else
# 	error "Please implement /proc/dtrace/gdt"
# endif
	return 0;
}
static struct seq_operations gdt_seq_ops = {
	.start = gdt_seq_start,
	.next = gdt_seq_next,
	.stop = gdt_seq_stop,
	.show = gdt_seq_show,
	};
static int gdt_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &gdt_seq_ops);
}
static const struct file_operations gdt_proc_fops = {
        .owner   = THIS_MODULE,
        .open    = gdt_seq_open,
        .read    = seq_read,
        .llseek  = seq_lseek,
        .release = seq_release,
};

/**********************************************************************/
/*   Code to implement /proc/dtrace/idt.			      */
/**********************************************************************/
static void *idt_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (*pos > IDT_ENTRIES)
		return 0;
	return (void *) (long) (*pos + 1);
}
static void *idt_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{	long	n = (long) v;
	++*pos;

	return (void *) (n - 2 > IDT_ENTRIES ? NULL : (void *) (n + 1));
}
static void idt_seq_stop(struct seq_file *seq, void *v)
{
}
static int idt_seq_show(struct seq_file *seq, void *v)
{	int	n = (int) (long) v;
	gate_t	*g;
	char	*cp;
	unsigned long addr;
	unsigned long size;
	unsigned long offset;
	char	*modname = NULL;
	char	name[KSYM_NAME_LEN];
	struct x86_descriptor desc;
	gate_t	*idt_table_ptr;

	store_idt(&desc);
	if (n == 1) {
		seq_printf(seq, "IDT: %p entries=%d\n",
			(void *) desc.address, desc.size);
		seq_printf(seq, "CR3: %p\n", (void *) read_cr3());
		seq_printf(seq, "#Ent seg:addr dpl type func\n");
		return 0;
	}
	if (n > IDT_ENTRIES + 1)
		return 0;

	idt_table_ptr = (gate_t *) desc.address;
	g = &idt_table_ptr[n - 2];
# if defined(__amd64)
	addr = ((unsigned long) g->offset_high << 32) | 
		((unsigned long) g->offset_middle << 16) |
		g->offset_low;
	cp = (char *) my_kallsyms_lookup(addr,
		&size, &offset, &modname, name);
	seq_printf(seq, "%02x %04x:%p p=%d ist=%d dpl=%d type=%x %s %s:%s\n",
		n-2,
		g->segment,
		(void *) addr, 
		g->p,
		g->ist,
		g->dpl,
		g->type,
		g->type == 0xe ? "intr" :
		g->type == 0xf ? "trap" :
		g->type == 0xc ? "call" :
		g->type == 0x5 ? "task" : "????",
		modname ? modname : "kernel",
		cp ? cp : "");

# elif defined(__i386)
	{int type = g->flags & 0x1f;
	addr = ((unsigned long) g->base1 << 16) | g->base0;
	cp = (char *) my_kallsyms_lookup(addr,
		&size, &offset, &modname, name);
	seq_printf(seq, "%02x %p %04x:%p p=%d dpl=%d type=%x %s %s:%s\n",
		n-2,
		g,
		g->segment,
		(void *) addr, 
		g->flags & 0x80 ? 1 : 0,
		(g->flags >> 5) & 0x3,
		type,
		type == 0xe ? "intr" :
		type == 0xf ? "trap" :
		type == 0xc ? "call" :
		type == 0x5 ? "task" : "????",
		modname ? modname : "kernel",
		cp ? cp : "");
	}
# else
# 	error "Please implement /proc/dtrace/idt"
# endif
	return 0;
}
static struct seq_operations seq_ops = {
	.start = idt_seq_start,
	.next = idt_seq_next,
	.stop = idt_seq_stop,
	.show = idt_seq_show,
	};
static int idt_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_ops);
}
static const struct file_operations idt_proc_fops = {
        .owner   = THIS_MODULE,
        .open    = idt_seq_open,
        .read    = seq_read,
        .llseek  = seq_lseek,
        .release = seq_release,
};

/**********************************************************************/
/*   Saved copies of idt_table[n] for when we get unloaded.	      */
/**********************************************************************/
gate_t saved_double_fault;
gate_t saved_int1;
gate_t saved_int2;
gate_t saved_int3;
gate_t saved_int11;
gate_t saved_int13;
gate_t saved_page_fault;
gate_t saved_ipi;
gate_t saved_int_0x80;

/**********************************************************************/
/*   Utility  function,  based on lookup_address() in the kernel, to  */
/*   walk a page table to find an entry. lookup_address() assumes we  */
/*   are  looking  at  the kernel page table - where an address will  */
/*   exist. We want to lookup in the context of a single process, as  */
/*   we  validate  that the process is mapping our dtrace_page_fault  */
/*   handler.							      */
/**********************************************************************/
#if defined(__i386) && !defined(HAVE_VMALLOC_SYNC_ALL)
static pte_t *
lookup_address_mm(struct mm_struct *mm, unsigned long address, unsigned int *level)
{
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;

        *level = PG_LEVEL_NONE;
	if (mm == NULL)
		return NULL;
	pgd = pgd_offset(mm, address);

        if (pgd_none(*pgd))
               return NULL;

        pud = pud_offset(pgd, address);
        if (pud_none(*pud))
               return NULL;

        *level = PG_LEVEL_1G;
        if (pud_large(*pud) || !pud_present(*pud))
               return (pte_t *)pud;

        pmd = pmd_offset(pud, address);
        if (pmd_none(*pmd))
               return NULL;

        *level = PG_LEVEL_2M;
        if (pmd_large(*pmd) || !pmd_present(*pmd))
               return (pte_t *)pmd;

        *level = PG_LEVEL_4K;

        return pte_offset_kernel(pmd, address);
}
#endif

/**********************************************************************/
/*   This  function  syncs  the page table entry for an address from  */
/*   the  kernel  (swapper_pg_dir)  into the target mm struct (where  */
/*   the  mm  is  the  process we are looking at). This prevents the  */
/*   problem  of the page fault handler causing a runaway page fault  */
/*   escalation.  This  code  is  based on vmalloc_sync_one() in the  */
/*   kernel.							      */
/*   								      */
/*   Maybe  we  can us vmalloc_sync_all() - which appears to only be  */
/*   called if the system is panicing, but I havent checked.	      */
/*   								      */
/*   amd64  has  sync_global_pgds()  function  which  appear  to  do  */
/*   something  similar,  but  looks  like  it is designed to handle  */
/*   pluggable memory.						      */
/**********************************************************************/
#include <asm/page.h>
static inline pmd_t *
sync_swapper_pte(struct mm_struct *mm, unsigned long address)
{
//typedef struct { pgdval_t pgd; } pgd_t;
#if defined(swapper_pg_dir)
	/***********************************************/
	/*   Old kernels, or some compile options may  */
	/*   #define this as NULL.		       */
	/***********************************************/
	return 0;
#else
static pgd_t *swapper_pg_dir;
        unsigned index = pgd_index(address);
	pgd_t	*pgd;
        pgd_t	*pgd_k;
        pud_t	*pud, *pud_k;
        pmd_t	*pmd, *pmd_k;

	/***********************************************/
	/*   If  its  a system process, we can ignore  */
	/*   the scenario.			       */
	/***********************************************/
	if ((pgd = mm->pgd) == NULL)
		return NULL;

	if (swapper_pg_dir == NULL)
		swapper_pg_dir = get_proc_addr("swapper_pg_dir");

        pgd += index;
        pgd_k = swapper_pg_dir + index;

        if (!pgd_present(*pgd_k)) {
               return NULL;
	}

        /*
        * set_pgd(pgd, *pgd_k); here would be useless on PAE
        * and redundant with the set_pmd() on non-PAE. As would
        * set_pud.
        */
        pud = pud_offset(pgd, address);
        pud_k = pud_offset(pgd_k, address);
        if (!pud_present(*pud_k)) {
               return NULL;
	}

        pmd = pmd_offset(pud, address);
        pmd_k = pmd_offset(pud_k, address);
        if (!pmd_present(*pmd_k)) {
               return NULL;
	}

//printk("pmd=%p\n", pmd);
	if (pmd == 0)
		return NULL;
//printk("*pmd=%p\n", *pmd);
        if (!pmd_present(*pmd))
		set_pmd(pmd, *pmd_k);
        else
               BUG_ON(pmd_page(*pmd) != pmd_page(*pmd_k));
//printk("pmd_k=%p\n", pmd_k);

        return pmd_k;
#endif
}

/**********************************************************************/
/*   Unhook the interrupts and remove the /proc/dtrace/idt device.    */
/**********************************************************************/
void
intr_exit(void)
{
	remove_proc_entry("dtrace/idt", 0);
	remove_proc_entry("dtrace/gdt", 0);

	/***********************************************/
	/*   Lose  the  grab  on the interrupt vector  */
	/*   table.				       */
	/***********************************************/
	if (idt_table_ptr == NULL)
		return;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
//	idt_table_ptr[8] = saved_double_fault;
#endif
	idt_table_ptr[1] = saved_int1;
	idt_table_ptr[2] = saved_int2;
	idt_table_ptr[3] = saved_int3;
	idt_table_ptr[11] = saved_int11;
	idt_table_ptr[13] = saved_int13;
	idt_table_ptr[14] = saved_page_fault;
	idt_table_ptr[T_DTRACE_RET] = saved_int_0x80;
	if (ipi_vector)
 		idt_table_ptr[ipi_vector] = saved_ipi;
}


/**********************************************************************/
/*   This    function   implements   the   logic   to   ensure   the  */
/*   dtrace_int_page_fault_handler interrupt routine exists in every  */
/*   processes  page  table.  System procs wont have an mm->pgd, but  */
/*   will  be using the init_mm.pgd. We just need to ensure that all  */
/*   other  procs  can  suffer  a  page  fault once we take over the  */
/*   vector. ("The impossible to debug" bug).			      */
/**********************************************************************/
void
my_vmalloc_sync_all(void)
{
# if defined(__i386) && !defined(HAVE_VMALLOC_SYNC_ALL)
	struct task_struct *t;
	unsigned int	level;
struct map {
	char *name;
	unsigned long addr;
	};
#define	ENTRY(func) {#func, (unsigned long) func}
static struct map tbl[] = {
	ENTRY(dtrace_int_page_fault_handler),
	ENTRY(dtrace_page_fault),
	ENTRY(dtrace_int1_handler),
	ENTRY(dtrace_int3_handler),
	ENTRY(dtrace_int1),
	ENTRY(dtrace_int3),
	};

        if (SHARED_KERNEL_PMD)
               return;

	/***********************************************/
	/*   We  lock  the process table - we wont be  */
	/*   long, but, just in case. This may not be  */
	/*   sufficient,   maybe   we  need  to  stop  */
	/*   scheduling.  But  its only during driver  */
	/*   load.				       */
	/***********************************************/
	rcu_read_lock();
	for_each_process(t) {
		struct mm_struct *mm;
		int	i;
		pte_t *p;
		pmd_t	*pmd;

		if ((mm = t->mm) == NULL)
			continue;
# undef comm
		/***********************************************/
		/*   Make  sure both the page fault interrupt  */
		/*   handler,  and the C companion are in the  */
		/*   page  table.  We need to be very careful  */
		/*   the  page  fault C code does not jump to  */
		/*   other pages which we have not mapped (eg  */
		/*   dtrace_print  etc).  Even  if  we do, we  */
		/*   might  get  away  with it, since the bug  */
		/*   scenario is very obscure.		       */
		/***********************************************/
		for (i = 0; i < (int) (sizeof tbl / sizeof tbl[0]); i++) {
			p = lookup_address_mm(mm, tbl[i].addr, &level);
			if (p == NULL) {
				pmd = sync_swapper_pte(mm, tbl[i].addr);
				printk("lockpf: %s pid%d %s %p\n", tbl[i].name, (int) t->pid, t->comm, pmd);
			}
		}
	}
	rcu_read_unlock();
#endif
}

/**********************************************************************/
/*   Called on loading the module to intercept the various interrupt  */
/*   handlers.							      */
/**********************************************************************/
void
intr_init(void)
{	struct proc_dir_entry *ent;
static	struct x86_descriptor desc1;

	my_kallsyms_lookup = get_proc_addr("kallsyms_lookup");

	/***********************************************/
	/*   cpu_core_exec  is  sitting  in  the  BSS  */
	/*   segment.  We  need to make it executable  */
	/*   for 3.x kernels, so do this here.	       */
	/***********************************************/
	memory_set_rw(cpu_core_exec, sizeof cpu_core_exec / PAGE_SIZE, TRUE);
{	long long *maskp = get_proc_addr("__supported_pte_mask");
	if (maskp)
//printk("__supported_pte_mask=%lx\n", __supported_pte_mask);
		*maskp &= ~_PAGE_NX;
}
	/***********************************************/
	/*   Needed  by assembler trap handler. These  */
	/*   are  the first-level interrupt handlers.  */
	/*   If  we  get  an interrupt which does not  */
	/*   belong  to dtrace, we daisy chain to the  */
	/*   original handler. We should probably get  */
	/*   these   from   the   IDT,   rather  than  */
	/*   hardcoding the names.		       */
	/***********************************************/
	kernel_int1_handler = get_proc_addr("debug");
	kernel_int3_handler = get_proc_addr("int3");
	kernel_int11_handler = get_proc_addr("segment_not_present");
	kernel_int13_handler = get_proc_addr("general_protection");
	kernel_double_fault_handler = get_proc_addr("double_fault");
	kernel_page_fault_handler = get_proc_addr("page_fault");
	kernel_nmi_handler = get_proc_addr("nmi");

	/***********************************************/
	/*   Add  an  /proc/dtrace/idt, so we can see  */
	/*   what it looks like from user space.       */
	/***********************************************/
	ent = create_proc_entry("dtrace/idt", 0444, NULL);
	if (ent)
		ent->proc_fops = &idt_proc_fops;
	ent = create_proc_entry("dtrace/gdt", 0444, NULL);
	if (ent)
		ent->proc_fops = &gdt_proc_fops;

	/***********************************************/
	/*   Lock   the   page   fault  handler  into  */
	/*   everyones MMU. Linux does on-demand sync  */
	/*   from do_page_fault, so we need to ensure  */
	/*   a  pgfault  doesnt  cause a cascade when  */
	/*   the     newly    loaded    page    fault  */
	/*   (dtrace_page_fault                   and  */
	/*   dtrace_int_page_fault_handler)  are  not  */
	/*   visible.				       */
	/***********************************************/
	{ void (*vmalloc_sync_all)(void) = get_proc_addr("vmalloc_sync_all");
	if (vmalloc_sync_all) {
		vmalloc_sync_all();
	} else {
		/***********************************************/
		/*   my_vmalloc_sync_all() is not necessarily  */
		/*   as  good  as the kernel one, or it might  */
		/*   be,  depending  on  when  you  read this  */
		/*   comment.  We wrote a load of code before  */
		/*   we discovered vmalloc_sync_all(), so may  */
		/*   as well use it.			       */
		/***********************************************/
		my_vmalloc_sync_all();
	}
	}

	/***********************************************/
	/*   Now patch the interrupt descriptor table  */
	/*   for  the  interrupts  we  care about. We  */
	/*   need   INT1/INT3  for  FBT  and  related  */
	/*   providers which put breakpoints into the  */
	/*   kernel,  and  we want the INT14(pgfault)  */
	/*   so  we  can  leverage  dtrace's style of  */
	/*   safe memory accesses.		       */
	/*   					       */
	/*   IPI is used for xcall.		       */
	/*   					       */
	/*   The  others are probably not needed, but  */
	/*   can be useful when debugging and getting  */
	/*   double   faults  or  other  segmentation  */
	/*   errors.				       */
	/***********************************************/
	store_idt(&desc1);
	idt_table_ptr = (gate_t *) desc1.address;
	idt_descr_ptr = &desc1;

	/***********************************************/
	/*   Save the original vectors.		       */
	/***********************************************/
	saved_double_fault = idt_table_ptr[8];
	saved_int1 = idt_table_ptr[1];
	saved_int2 = idt_table_ptr[2];
	saved_int3 = idt_table_ptr[3];
	saved_int11 = idt_table_ptr[11];
	saved_int13 = idt_table_ptr[13];
	saved_page_fault = idt_table_ptr[14];
	saved_ipi = idt_table_ptr[ipi_vector];
	saved_int_0x80 = idt_table_ptr[T_DTRACE_RET];

	/***********************************************/
	/*   Now overwrite the vectors.		       */
	/***********************************************/
#if 0 && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
	/***********************************************/
	/*   Double  faults  are  done with a special  */
	/*   register  in  the  CPU.  We  dont really  */
	/*   want/need these, so just ignore for now.  */
	/***********************************************/
	set_idt_entry(8, (unsigned long) dtrace_double_fault);
#endif
	set_idt_entry(1, (unsigned long) dtrace_int1); // single-step
	set_idt_entry(3, (unsigned long) dtrace_int3); // breakpoint
	set_idt_entry(11, (unsigned long) dtrace_int11); //segment_not_present
	set_idt_entry(13, (unsigned long) dtrace_int13); //GPF
	set_idt_entry(14, (unsigned long) dtrace_page_fault);
	set_idt_entry(T_DTRACE_RET, (unsigned long) dtrace_int_dtrace_ret);

	/***********************************************/
	/*   ipi  vector  needed by xcall code if our  */
	/*   'new'  code  is  used.  Turn off for now  */
	/*   since  we havent got the API to allocate  */
	/*   a  free irq. And we dont seem to need it  */
	/*   in xcall.c.			       */
	/***********************************************/
/*
{int *first_v = get_proc_addr("first_system_vector");
set_bit(ipi_vector, used_vectors);
if (*first_v > ipi_vector)
	*first_v = ipi_vector;
}
*/
	if (ipi_vector)
		set_idt_entry(ipi_vector, (unsigned long) dtrace_int_ipi);
	/***********************************************/
	/*   Intercept  NMI.  We  used  to use it for  */
	/*   unblocking  xcall  IPI calls, but now we  */
	/*   must  avoid  NMI  propagating  into  the  */
	/*   kernel  because  we  cannot have a probe  */
	/*   fire from an NMI.			       */
	/***********************************************/
	set_idt_entry(2, (unsigned long) dtrace_int_nmi);

}

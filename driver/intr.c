/**********************************************************************/
/*   This  file  contains  the  code for managing interrupts and the  */
/*   IDT.							      */
/*   								      */
/*   Date: December 2011					      */
/*   Author: Paul D. Fox					      */
/*   								      */
/*   License: CDDL						      */
/*   								      */
/*   $Header: Last edited: 06-Dec-2011 1.12 $ 			      */
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

/**********************************************************************/
/*   Define  something  so  the  intr routines know whether to route  */
/*   back to the kernel, ie it is theirs, vs ours (NOTIFY_DONE).      */
/**********************************************************************/
# define	NOTIFY_KERNEL	1

/**********************************************************************/
/*   Pointer to the IDT size/address structure.			      */
/**********************************************************************/
static struct desc_ptr *idt_descr_ptr;

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
//	s.segment = seg;
	s.base0 = (u16) (func & 0xffff);
	s.base1 = (u16) ((func & 0xffff0000) >> 16);
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
int 
dtrace_int3_handler(int type, struct pt_regs *regs)
{	cpu_core_t *this_cpu = THIS_CPU();
	cpu_trap_t	*tp;
	int	ret;

//dtrace_printf("#%lu INT3 PC:%p REGS:%p CPU:%d mode:%d\n", cnt_int3_1, regs->r_pc-1, regs, cpu_get_id(), this_cpu->cpuc_mode);
//preempt_disable();
	cnt_int3_1++;

	/***********************************************/
	/*   Are  we idle, or already single stepping  */
	/*   a probe?				       */
	/***********************************************/
	if (dtrace_int_disable)
		goto switch_off;

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
	/*   We  need to find the underlying probe so  */
	/*   we can unpatch it.			       */
	/***********************************************/
	cnt_int3_3++;
dtrace_printf("recursive-int[%lu]: CPU:%d PC:%p\n", cnt_int3_3, smp_processor_id(), (void *) regs->r_pc-1);
dtrace_printf_disable = 1;

switch_off:
	tp = &this_cpu->cpuc_trap[1];
	tp->ct_tinfo.t_doprobe = FALSE;
	ret = dtrace_invop(regs->r_pc - 1, (uintptr_t *) regs, 
		regs->r_rax, &tp->ct_tinfo);
//preempt_enable_no_resched();
	if (ret) {
		((unsigned char *) regs->r_pc)[-1] = tp->ct_tinfo.t_opcode;
		regs->r_pc--;
//preempt_enable();
		return NOTIFY_DONE;
	}

	/***********************************************/
	/*   Not ours, so let the kernel have it.      */
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
/*
if (0) {
set_console_on(1);
//dump_stack();
printk("dtrace cpu#%d PGF %p err=%p cr2:%p %02x %02x %02x %02x\n", 
	cpu_get_id(), regs->r_pc, regs->r_trapno, read_cr2_register(), 
	((unsigned char *) regs->r_pc)[0],
	((unsigned char *) regs->r_pc)[1],
	((unsigned char *) regs->r_pc)[2],
	((unsigned char *) regs->r_pc)[3]
	);
}
*/
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

	if (n == 1) {
		seq_printf(seq, "#Ent seg:addr dpl type func\n");
		return 0;
	}
	if (n > IDT_ENTRIES + 1)
		return 0;

	/***********************************************/
	/*   Hack: Only for i386 at present.	       */
	/***********************************************/
	g = &idt_table_ptr[n - 2];
# if defined(__amd64)
	addr = ((unsigned long) g->offset_high << 32) | 
		((unsigned long) g->offset_middle << 16) |
		g->offset_low;
	cp = (char *) my_kallsyms_lookup(addr,
		&size, &offset, &modname, name);
	seq_printf(seq, "%02x %04x:%p p=%d ist=%d dpl=%d type=%x %s:%s\n",
		n-2,
		g->segment,
		(void *) addr, 
		g->p,
		g->ist,
		g->dpl,
		g->type,
		modname ? modname : "kernel",
		cp ? cp : "");

# elif defined(__i386)
	addr = ((unsigned long) g->base1 << 16) | g->base0;
	cp = (char *) my_kallsyms_lookup(addr,
		&size, &offset, &modname, name);
	seq_printf(seq, "%02x %04x:%p p=%d dpl=%d type=%x %s:%s\n",
		n-2,
		g->segment,
		(void *) addr, 
		g->flags & 0x80 ? 1 : 0,
		(g->flags >> 5) & 0x3,
		g->flags & 0x1f,
		modname ? modname : "kernel",
		cp ? cp : "");

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

/**********************************************************************/
/*   Called  by the SMP_CALL_FUNCTION to ensure all CPUs are in sync  */
/*   on the IDT to use.						      */
/**********************************************************************/
static void
reload_idt(void *ptr)
{
	__flush_tlb_all();
	load_idt(ptr);
}

/**********************************************************************/
/*   Unhook the interrupts and remove the /proc/dtrace/idt device.    */
/**********************************************************************/
void
intr_exit(void)
{	char *saved_idt;
static struct desc_ptr idt_descr2;

	remove_proc_entry("dtrace/idt", 0);

	/***********************************************/
	/*   Lose  the  grab  on the interrupt vector  */
	/*   table.				       */
	/***********************************************/
	if (idt_table_ptr == NULL)
		return;

	/***********************************************/
	/*   Flip  all  cpus  to  look away whilst we  */
	/*   unhook the vectors.		       */
	/***********************************************/
	saved_idt = (char *) __get_free_page(GFP_KERNEL);
	memcpy(saved_idt, idt_table_ptr, idt_descr_ptr->size);
	idt_descr2.size = idt_descr_ptr->size;
	idt_descr2.address = (unsigned long) saved_idt;
//	on_each_cpu(reload_idt, &idt_descr2, 1);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
//	idt_table_ptr[8] = saved_double_fault;
#endif
	idt_table_ptr[1] = saved_int1;
	idt_table_ptr[2] = saved_int2;
	idt_table_ptr[3] = saved_int3;
	idt_table_ptr[11] = saved_int11;
	idt_table_ptr[13] = saved_int13;
	idt_table_ptr[14] = saved_page_fault;
	if (ipi_vector)
 		idt_table_ptr[ipi_vector] = saved_ipi;

	/***********************************************/
	/*   Now  get  all the cpus to switch back to  */
	/*   the updated original IDT.		       */
	/***********************************************/
//	on_each_cpu(reload_idt, idt_descr_ptr, 1);

	free_page((unsigned long) saved_idt);
}

void
intr_init(void)
{	struct proc_dir_entry *ent;
	char *saved_idt;
static struct desc_ptr idt_descr2;

	my_kallsyms_lookup = get_proc_addr("kallsyms_lookup");

	/***********************************************/
	/*   cpu_core_exec  is  sitting  in  the  BSS  */
	/*   segment.  We  need to make it executable  */
	/*   for 3.x kernels, so do this here.	       */
	/***********************************************/
	memory_set_rw(cpu_core_exec, sizeof cpu_core_exec / PAGE_SIZE, TRUE);
{printk("__supported_pte_mask=%lx\n", __supported_pte_mask);
__supported_pte_mask &= ~_PAGE_NX;
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
	idt_table_ptr = get_proc_addr("idt_table");
	if (idt_table_ptr == NULL) {
		printk("dtrace: idt_table: not found - cannot patch INT3 handler\n");
		return;
	}

	/***********************************************/
	/*   IDT  needs  to be on a page boundary. We  */
	/*   need  to relocate the IDT away whilst we  */
	/*   make  changes,  then switch back when we  */
	/*   are done.				       */
	/*   					       */
	/*   http://stackoverflow.com/questions/2497919/changing-the-interrupt-descriptor-table				       */
	/***********************************************/
	idt_descr_ptr = get_proc_addr("idt_descr");

	/***********************************************/
	/*   Copy the existing IDT.		       */
	/***********************************************/
	saved_idt = (char *) __get_free_page(GFP_KERNEL);
	memcpy(saved_idt, idt_table_ptr, idt_descr_ptr->size);
	idt_descr2.size = idt_descr_ptr->size;
	idt_descr2.address = (unsigned long) saved_idt;

	/***********************************************/
	/*   Now, all cpus, look away.		       */
	/***********************************************/
	on_each_cpu(reload_idt, &idt_descr2, 1);

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

	/***********************************************/
	/*   Tell  the other cpus its safe to see the  */
	/*   updated IDT.			       */
	/***********************************************/
	on_each_cpu(reload_idt, idt_descr_ptr, 1);

	free_page((unsigned long) saved_idt);
}


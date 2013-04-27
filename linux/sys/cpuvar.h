# if !defined(SYS_CPUVAR_H)
# define	SYS_CPUVAR_H

#include <sys/rwlock.h>

#define CPU_CACHE_COHERENCE_SIZE        64

typedef enum {
        CPU_INIT,
        CPU_CONFIG,
        CPU_UNCONFIG,
        CPU_ON,
        CPU_OFF,
        CPU_CPUPART_IN,
        CPU_CPUPART_OUT
} cpu_setup_t;

/*
 * Flags in the CPU structure.
 *
 * These are protected by cpu_lock (except during creation).
 *
 * Offlined-CPUs have three stages of being offline:
 *
 * CPU_ENABLE indicates that the CPU is participating in I/O interrupts
 * that can be directed at a number of different CPUs.  If CPU_ENABLE
 * is off, the CPU will not be given interrupts that can be sent elsewhere,
 * but will still get interrupts from devices associated with that CPU only,
 * and from other CPUs.
 *
 * CPU_OFFLINE indicates that the dispatcher should not allow any threads
 * other than interrupt threads to run on that CPU.  A CPU will not have
 * CPU_OFFLINE set if there are any bound threads (besides interrupts).
 *
 * CPU_QUIESCED is set if p_offline was able to completely turn idle the
 * CPU and it will not have to run interrupt threads.  In this case it'll
 * stay in the idle loop until CPU_QUIESCED is turned off.
 *
 * CPU_FROZEN is used only by CPR to mark CPUs that have been successfully
 * suspended (in the suspend path), or have yet to be resumed (in the resume
 * case).
 *
 * On some platforms CPUs can be individually powered off.
 * The following flags are set for powered off CPUs: CPU_QUIESCED,
 * CPU_OFFLINE, and CPU_POWEROFF.  The following flags are cleared:
 * CPU_RUNNING, CPU_READY, CPU_EXISTS, and CPU_ENABLE.
 */
#define	CPU_RUNNING	0x001		/* CPU running */
#define	CPU_READY	0x002		/* CPU ready for cross-calls */
#define	CPU_QUIESCED	0x004		/* CPU will stay in idle */
#define	CPU_EXISTS	0x008		/* CPU is configured */
#define	CPU_ENABLE	0x010		/* CPU enabled for interrupts */
#define	CPU_OFFLINE	0x020		/* CPU offline via p_online */
#define	CPU_POWEROFF	0x040		/* CPU is powered off */
#define	CPU_FROZEN	0x080		/* CPU is frozen via CPR suspend */
#define	CPU_SPARE	0x100		/* CPU offline available for use */
#define	CPU_FAULTED	0x200		/* CPU offline diagnosed faulty */


/*
 * The cpu_core structure consists of per-CPU state available in any context.
 * On some architectures, this may mean that the page(s) containing the
 * NCPU-sized array of cpu_core structures must be locked in the TLB -- it
 * is up to the platform to assure that this is performed properly.  Note that
 * the structure is sized to avoid false sharing.
 */
#define CPUC_SIZE               (sizeof (uint16_t) + sizeof (uintptr_t) + \
                                sizeof (kmutex_t))
#define CPUC_PADSIZE            CPU_CACHE_COHERENCE_SIZE - CPUC_SIZE

typedef struct dtrace_cpu cpu_t;
struct dtrace_cpu { /* Avoid name clash with kernel 'cpu' structure. */
        int             cpuid;
        struct cyc_cpu *cpu_cyclic;
        unsigned	cpu_flags;
        unsigned int	cpu_intr_actv;
        uintptr_t       cpu_profile_pc;
        uintptr_t       cpu_profile_upc;
        uintptr_t       cpu_dtrace_caller;      /* DTrace: caller, if any */
        hrtime_t        cpu_dtrace_chillmark;   /* DTrace: chill mark time */
        hrtime_t        cpu_dtrace_chilled;     /* DTrace: total chill time */
        struct cpupart  *cpu_part;              /* partition with this CPU */
	cpu_t		*cpu_next;
	cpu_t		*cpu_next_onln;         /* next online (enabled) CPU */
        cpu_t		*cpu_prev_onln;         /* prev online (enabled) CPU */
	krwlock_t	cpu_ft_lock;		/* fasttrap mutex.	*/
	uintptr_t       cpu_cpcprofile_pc;      /* kernel PC in cpc interrupt */
	uintptr_t       cpu_cpcprofile_upc;     /* user PC in cpc interrupt */
	void		*cpu_cpc_ctx;   /* performance counter context */
};

/**********************************************************************/
/*   Structure  used  when  hitting  a trap instruction, to describe  */
/*   what the instruction looks like (dtrace_invop)		      */
/**********************************************************************/
#if linux
	#if defined(__i386) || defined(__amd64)
	typedef unsigned char instr_t;
	#endif

	# if defined(__arm__)
	typedef unsigned int instr_t;
	# endif
#endif

typedef struct trap_instr_t {
	int		t_doprobe;
	int		t_modrm;
	instr_t		t_opcode;
	unsigned char	t_inslen;
	} trap_instr_t;

/**********************************************************************/
/*   Structure  needed  whilst  we  single step a breakpoint. We may  */
/*   need  more  than one to handle a nested trap (page fault during  */
/*   single step of a probe).					      */
/**********************************************************************/
typedef struct cpu_trap_t {
# define MAX_INSTR_LEN  16
	unsigned char	ct_instr_buf[MAX_INSTR_LEN];
	unsigned char	*ct_orig_pc;
	unsigned char	*ct_orig_pc0;
	unsigned long	ct_eflags;
	trap_instr_t	ct_tinfo;
	} cpu_trap_t;

typedef struct cpu_core {
        uint16_t        cpuc_dtrace_flags;      /* DTrace flags */
	uint32_t        cpuc_dcpc_intr_state;   /* DCPC provider intr state */
	uint8_t		cpuc_probe_level;	/* Avoid reentrancy issues in dtrace_probe */
	uint32_t	cpuc_this_probe;	/* Current probe.	*/
//	spinlock_t	cpuc_spinlock;
//#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 9)
//	/***********************************************/
//	/*   sizeof(kmutex_t)  can  exceed  64 bytes,  */
//	/*   and gcc doesnt like this.		       */
//	/***********************************************/
//        uint8_t         cpuc_pad[CPUC_PADSIZE]; /* padding */
//#endif
        uintptr_t       cpuc_dtrace_illval;     /* DTrace illegal value */
        kmutex_t        cpuc_pid_lock;          /* DTrace pid provider lock */
	/***********************************************/
	/*   Here for single stepping.                 */
	/***********************************************/
	int		cpuc_mode;
	int		cpuc_nmi;	/* Inside a bkpt from an NMI. */
	cpu_trap_t	cpuc_trap[2];

	/***********************************************/
	/*   Make  registers  easily available to the  */
	/*   probe code.			       */
	/***********************************************/
	struct pt_regs	*cpuc_regs;
	struct pt_regs	*cpuc_regs_old; /* Allow for interrupts */
} cpu_core_t;

# define	CPUC_MODE_IDLE	0
# define	CPUC_MODE_INT3	1
# define	CPUC_MODE_INT1	2

extern cpu_core_t *cpu_core;
extern cpu_t	*cpu_table;
extern cpu_t	*cpu_list;
extern kmutex_t	cpu_lock;
extern kmutex_t	mod_lock;
extern cpu_t *curcpu(void);
typedef intptr_t xc_arg_t;
typedef int (*xc_func_t)(xc_arg_t, xc_arg_t, xc_arg_t);


# define	cpu_id	cpuid
//# define	CPU	smp_processor_id()
# define	CPU	(&cpu_table[smp_processor_id()])
# define	CPU_ON_INTR(x) in_interrupt()

# define	cpuset_t	cpumask_t
# define	CPUSET_ZERO(s) memset(&s, 0, sizeof s)
# define	CPUSET_ALL(s)	cpus_setall(s)
# define	CPUSET_ADD(s, cpu) cpu_set(cpu, s)
# define	kpreempt_disable	preempt_disable
# define	kpreempt_enable	preempt_enable
# define	X_CALL_HIPRI	0
# endif

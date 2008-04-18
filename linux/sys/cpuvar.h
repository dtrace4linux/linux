# if !defined(SYS_CPUVAR_H)
# define	SYS_CPUVAR_H

#define CPU_CACHE_COHERENCE_SIZE        64

# define	kmutex_t mutex_t


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
 * The cpu_core structure consists of per-CPU state available in any context.
 * On some architectures, this may mean that the page(s) containing the
 * NCPU-sized array of cpu_core structures must be locked in the TLB -- it
 * is up to the platform to assure that this is performed properly.  Note that
 * the structure is sized to avoid false sharing.
 */
#define CPUC_SIZE               (sizeof (uint16_t) + sizeof (uintptr_t) + \
                                sizeof (kmutex_t))
#define CPUC_PADSIZE            CPU_CACHE_COHERENCE_SIZE - CPUC_SIZE

typedef struct cpu_t {
        int             cpuid;
        struct cyc_cpu *cpu_cyclic;
        unsigned	cpu_flags;
        unsigned int	cpu_intr_actv;
        uintptr_t       cpu_profile_pc;
        uintptr_t       cpu_profile_upc;
        uintptr_t       cpu_dtrace_caller;      /* DTrace: caller, if any */
        hrtime_t        cpu_dtrace_chillmark;   /* DTrace: chill mark time */
        hrtime_t        cpu_dtrace_chilled;     /* DTrace: total chill time */
	struct cpu_t *cpu_next;
} cpu_t;

typedef struct cpu_core {
        uint16_t        cpuc_dtrace_flags;      /* DTrace flags */
        uint8_t         cpuc_pad[CPUC_PADSIZE]; /* padding */
        uintptr_t       cpuc_dtrace_illval;     /* DTrace illegal value */
        kmutex_t        cpuc_pid_lock;          /* DTrace pid provider lock */
} cpu_core_t;

extern cpu_core_t cpu_core[NCPU];
extern cpu_t cpu_table[NCPU];
extern cpu_t	*cpu_list;
extern mutex_t	cpu_lock;
extern mutex_t	mod_lock;
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

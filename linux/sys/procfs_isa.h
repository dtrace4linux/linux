# if !defined(SYS_PROCFS_ISA_H)
# define SYS_PROCFS_ISA_H

/*
 * Possible values of pr_dmodel.
 * This isn't isa-specific, but it needs to be defined here for other reasons.
 */
#define PR_MODEL_UNKNOWN 0
#define PR_MODEL_ILP32  1       /* process data model is ILP32 */
#define PR_MODEL_LP64   2       /* process data model is LP64 */

#define NPRGREG 38
#if defined(_LP64) || defined(_I32LPx)
typedef long            prgreg_t;
#else
typedef int             prgreg_t;
#endif
typedef prgreg_t        prgregset_t[NPRGREG];
typedef prgreg32_t      prgregset32_t[NPRGREG];


/*
 * To determine whether application is running native.
 */
#if defined(_LP64)
#define PR_MODEL_NATIVE PR_MODEL_LP64
#elif defined(_ILP32)
#define PR_MODEL_NATIVE PR_MODEL_ILP32
#else
#error "No DATAMODEL_NATIVE specified"
#endif  /* _LP64 || _ILP32 */

struct fpq {
        unsigned int *fpq_addr;         /* address */
        unsigned int fpq_instr;         /* instruction */
};
struct fq {
        union {                         /* FPU inst/addr queue */
                double whole;
                struct fpq fpq;
        } FQu;
};
/*
 * Floating-point register access (sparc FPU).
 * See <sys/regset.h> for details of interpretation.
 */
#ifdef  __sparcv9
typedef struct prfpregset {
        union {                         /* FPU floating point regs */
                uint32_t pr_regs[32];           /* 32 singles */
                double  pr_dregs[32];           /* 32 doubles */
                long double pr_qregs[16];       /* 16 quads */
        } pr_fr;
        uint64_t pr_filler;
        uint64_t pr_fsr;                /* FPU status register */
        uint8_t pr_qcnt;                /* # of entries in saved FQ */
        uint8_t pr_q_entrysize;         /* # of bytes per FQ entry */
        uint8_t pr_en;                  /* flag signifying fpu in use */
        char    pr_pad[13];             /* ensure sizeof(prfpregset)%16 == 0 */
        struct fq pr_q[16];             /* contains the FQ array */
} prfpregset_t;
#else
typedef struct prfpregset {
        union {                         /* FPU floating point regs */
                uint32_t pr_regs[32];           /* 32 singles */
                double  pr_dregs[16];           /* 16 doubles */
        } pr_fr;
        uint32_t pr_filler;
        uint32_t pr_fsr;                /* FPU status register */
        uint8_t pr_qcnt;                /* # of entries in saved FQ */
        uint8_t pr_q_entrysize;         /* # of bytes per FQ entry */
        uint8_t pr_en;                  /* flag signifying fpu in use */
        struct fq pr_q[32];             /* contains the FQ array */
} prfpregset_t;
#endif  /* __sparcv9 */

# endif

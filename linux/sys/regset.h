# if !defined(SYS_REGSET_H)
# define SYS_REGSET_H 1

/**********************************************************************/
/*   20130204   Only   define  greg_t  for  kernel  code;  otherwise  */
/*   potential   conflict   with   <sys/ucontext.h>  when  compiling  */
/*   ctf_lib.c.							      */
/**********************************************************************/
#if __KERNEL__
#if defined(_LP64) || defined(_I32LPx)
typedef long    greg_t;
#else
typedef int     greg_t;
#endif
#endif /* __KERNEL__ */

# if 0
struct _fpreg { /* structure of a temp real fp register */
        unsigned short significand[4];  /* 64 bit mantissa value */
        unsigned short exponent;        /* 15 bit exponent and sign bit */
};
# endif

#if defined(__amd64)

# if 0
typedef struct fpu {
        union {
                struct fpchip_state {
                        uint16_t cw;
                        uint16_t sw;
                        uint8_t  fctw;
                        uint8_t  __fx_rsvd;
                        uint16_t fop;
                        uint64_t rip;
                        uint64_t rdp;
                        uint32_t mxcsr;
                        uint32_t mxcsr_mask;
                        union {
                                uint16_t fpr_16[5];
                                upad128_t __fpr_pad;
                        } st[8];
                        upad128_t xmm[16];
                        upad128_t __fx_ign2[6];
                        uint32_t status;        /* sw at exception */
                        uint32_t xstatus;       /* mxcsr at exception */
                } fpchip_state;
                uint32_t        f_fpregs[130];
        } fp_reg_set;
} fpregset_t;
# endif

#else   /* __i386 */

/*
 * This definition of the floating point structure is binary
 * compatible with the Intel386 psABI definition, and source
 * compatible with that specification for x87-style floating point.
 * It also allows SSE/SSE2 state to be accessed on machines that
 * possess such hardware capabilities.
 */
# if 0
typedef struct fpu {
        union {
                struct fpchip_state {
                        uint32_t state[27];     /* 287/387 saved state */
                        uint32_t status;        /* saved at exception */
                        uint32_t mxcsr;         /* SSE control and status */
                        uint32_t xstatus;       /* SSE mxcsr at exception */
                        uint32_t __pad[2];      /* align to 128-bits */
                        upad128_t xmm[8];       /* %xmm0-%xmm7 */
                } fpchip_state;
                struct fp_emul_space {          /* for emulator(s) */
                        uint8_t fp_emul[246];
                        uint8_t fp_epad[2];
                } fp_emul_space;
                uint32_t        f_fpregs[95];   /* union of the above */
        } fp_reg_set;
} fpregset_t;
# endif

/*
 * (This structure definition is specified in the i386 ABI supplement)
 */
typedef struct __old_fpu {
        union {
                struct __old_fpchip_state       /* fp extension state */
                {
                        int     state[27];      /* 287/387 saved state */
                        int     status;         /* status word saved at */
                                                /* exception */
                } fpchip_state;
                struct __old_fp_emul_space      /* for emulator(s) */
                {
                        char    fp_emul[246];
                        char    fp_epad[2];
                } fp_emul_space;
                int     f_fpregs[62];           /* union of the above */
        } fp_reg_set;
        long            f_wregs[33];            /* saved weitek state */
} __old_fpregset_t;

#endif  /* __i386 */

typedef int32_t greg32_t;
typedef int64_t greg64_t;

# if defined(__amd64)
# 	define _NGREG	28
#	define R_FP	REG_RBP
# elif defined(__i386)
# 	define _NGREG	19
#	define R_FP	EBP
# elif defined(__arm__)
# 	define _NGREG	18
#	define R_FP	EBP
# else
#	pragma warning("unsupported cpu in sys/regset.h")
# endif
# define NGREG	_NGREG

# if !defined(EFL)
# define	EDI	4
# define	ESI	5
# define	EBP	6
# define	ESP	7
# define	EBX	8
# define	EDX	9
# define	ECX	10
# define	EAX	11
# define	TRAPNO	12
# define	ERR	13
# define	EIP	14
# define	EFL	16
# define	UESP	17
# endif
# define DS 7
# define ES 8
# define FS 9
# define GS 10
# define CS  13
# define SS   16

/* x86-64 */
#define REG_GSBASE      27
#define REG_FSBASE      26
#define REG_DS          25
#define REG_ES          24

#define REG_GS          23
#define REG_FS          22
#define REG_SS          21
#define REG_RSP         20
#define REG_RFL         19
#define REG_CS          18
#define REG_RIP         17
#define REG_ERR         16
#define REG_TRAPNO      15
#define REG_R15         14
#define REG_R14         13
#define REG_R13         12
#define REG_R12         11
#define REG_RBP         10
#define REG_RBX         9
#define REG_R11         8
#define REG_R10         7
#define REG_R9          6
#define REG_R8          5
#define REG_RAX         4
#define REG_RCX         3
#define REG_RDX         2
#define REG_RSI         1
#define REG_RDI         0

/* AMD's FS.base and GS.base MSRs */

#define MSR_AMD_FSBASE  0xc0000100      /* 64-bit base address for %fs */
#define MSR_AMD_GSBASE  0xc0000101      /* 64-bit base address for %gs */

#if defined(__amd64)

#define REG_PC  REG_RIP
#define REG_FP  REG_RBP
#define REG_SP  REG_RSP
#define REG_PS  REG_RFL
#define REG_R0  REG_RAX
#define REG_R1  REG_RDX

#else   /* __i386 */

#define REG_PC  EIP
#define REG_FP  EBP
#define REG_SP  UESP
#define REG_PS  EFL
#define REG_R0  EAX
#define REG_R1  EDX

#endif  /* __i386 */

/**********************************************************************/
/*   ARM definitions.						      */
/**********************************************************************/
# if defined(__arm__)
#	define	R15	15
#	define	R13	13
# endif

# endif

# if !defined(SYS_REGSET_H)
# define SYS_REGSET_H 1

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

# endif

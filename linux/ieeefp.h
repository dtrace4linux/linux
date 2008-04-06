#if defined(__i386) || defined(__amd64)

#if defined(__amd64)
#define _fpstate _fpstate32
#endif

struct _fpstate {               /* saved state info from an exception */
        unsigned int    cw,     /* control word */
                        sw,     /* status word after fnclex-not useful */
                        tag,    /* tag word */
                        ipoff,  /* %eip register */
                        cssel,  /* code segment selector */
                        dataoff, /* data operand address */
                        datasel; /* data operand selector */
        struct _fpreg _st[8];   /* saved register stack */
        unsigned int status;    /* status word saved at exception */
        unsigned int mxcsr;
        unsigned int xstatus;   /* status word saved at exception */
        unsigned int __pad[2];
        unsigned int xmm[8][4];
};

#if defined(__amd64)
#undef  _fpstate
#endif

#endif  /* __i386 || __amd64 */

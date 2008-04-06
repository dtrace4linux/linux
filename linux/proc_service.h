typedef enum {
        PS_OK,          /* generic "call succeeded" */
        PS_ERR,         /* generic error */
        PS_BADPID,      /* bad process handle */
        PS_BADLID,      /* bad lwp identifier */
        PS_BADADDR,     /* bad address */
        PS_NOSYM,       /* p_lookup() could not find given symbol */
        PS_NOFREGS      /* FPU register set not available for given lwp */
} ps_err_e;

#ifdef _LP64
typedef Elf64_Sym       ps_sym_t;
#else
typedef Elf32_Sym       ps_sym_t;
#endif

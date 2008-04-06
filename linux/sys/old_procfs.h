typedef struct prpsinfo {
        char    pr_state;       /* numeric process state (see pr_sname) */
        char    pr_sname;       /* printable character representing pr_state */
        char    pr_zomb;        /* !=0: process terminated but not waited for */
        char    pr_nice;        /* nice for cpu usage */
        uint_t  pr_flag;        /* process flags */
        uid_t   pr_uid;         /* real user id */
        gid_t   pr_gid;         /* real group id */
        pid_t   pr_pid;         /* unique process id */
        pid_t   pr_ppid;        /* process id of parent */
        pid_t   pr_pgrp;        /* pid of process group leader */
        pid_t   pr_sid;         /* session id */
        caddr_t pr_addr;        /* physical address of process */
        size_t  pr_size;        /* size of process image in pages */
        size_t  pr_rssize;      /* resident set size in pages */
        caddr_t pr_wchan;       /* wait addr for sleeping process */
        timestruc_t pr_start;   /* process start time, sec+nsec since epoch */
        timestruc_t pr_time;    /* usr+sys cpu time for this process */
        int     pr_pri;         /* priority, high value is high priority */
        char    pr_oldpri;      /* pre-SVR4, low value is high priority */
        char    pr_cpu;         /* pre-SVR4, cpu usage for scheduling */
        o_dev_t pr_ottydev;     /* short tty device number */
        dev_t   pr_lttydev;     /* controlling tty device (PRNODEV if none) */
        char    pr_clname[PRCLSZ];      /* scheduling class name */
        char    pr_fname[PRFNSZ];       /* last component of execed pathname */
        char    pr_psargs[PRARGSZ];     /* initial characters of arg list */
        short   pr_syscall;     /* system call number (if in syscall) */
        short   pr_fill;
        timestruc_t pr_ctime;   /* usr+sys cpu time for reaped children */
        size_t  pr_bysize;      /* size of process image in bytes */
        size_t  pr_byrssize;    /* resident set size in bytes */
        int     pr_argc;        /* initial argument count */
        char    **pr_argv;      /* initial argument vector */
        char    **pr_envp;      /* initial environment vector */
        int     pr_wstat;       /* if zombie, the wait() status */
                        /* The following percent numbers are 16-bit binary */
                        /* fractions [0 .. 1] with the binary point to the */
                        /* right of the high-order bit (one == 0x8000) */
        ushort_t pr_pctcpu;     /* % of recent cpu time, one or all lwps */
        ushort_t pr_pctmem;     /* % of of system memory used by the process */
        uid_t   pr_euid;        /* effective user id */
        gid_t   pr_egid;        /* effective group id */
        id_t    pr_aslwpid;     /* historical; now always zero */
        char    pr_dmodel;      /* data model of the process */
        char    pr_pad[3];
        int     pr_filler[6];   /* for future expansion */
} prpsinfo_t;

typedef struct prstatus {
        int     pr_flags;       /* Flags (see below) */
        short   pr_why;         /* Reason for process stop (if stopped) */
        short   pr_what;        /* More detailed reason */
        siginfo_t pr_info;      /* Info associated with signal or fault */
        short   pr_cursig;      /* Current signal */
        ushort_t pr_nlwp;       /* Number of lwps in the process */
        sigset_t pr_sigpend;    /* Set of signals pending to the process */
        sigset_t pr_sighold;    /* Set of signals held (blocked) by the lwp */
        struct  sigaltstack pr_altstack; /* Alternate signal stack info */
        struct  sigaction pr_action; /* Signal action for current signal */
        pid_t   pr_pid;         /* Process id */
        pid_t   pr_ppid;        /* Parent process id */
        pid_t   pr_pgrp;        /* Process group id */
        pid_t   pr_sid;         /* Session id */
        timestruc_t pr_utime;   /* Process user cpu time */
        timestruc_t pr_stime;   /* Process system cpu time */
        timestruc_t pr_cutime;  /* Sum of children's user times */
        timestruc_t pr_cstime;  /* Sum of children's system times */
        char    pr_clname[PRCLSZ]; /* Scheduling class name */
        short   pr_syscall;     /* System call number (if in syscall) */
        short   pr_nsysarg;     /* Number of arguments to this syscall */
        long    pr_sysarg[PRSYSARGS]; /* Arguments to this syscall */
        id_t    pr_who;         /* Specific lwp identifier */
        sigset_t pr_lwppend;    /* Set of signals pending to the lwp */
        struct ucontext *pr_oldcontext; /* Address of previous ucontext */
        caddr_t pr_brkbase;     /* Address of the process heap */
        size_t  pr_brksize;     /* Size of the process heap, in bytes */
        caddr_t pr_stkbase;     /* Address of the process stack */
        size_t  pr_stksize;     /* Size of the process stack, in bytes */
        short   pr_processor;   /* processor which last ran this LWP */
        short   pr_bind;        /* processor LWP bound to or PBIND_NONE */
        long    pr_instr;       /* Current instruction */
        prgregset_t pr_reg;     /* General registers */
} prstatus_t;

typedef struct prstatus32 {
        int32_t pr_flags;       /* Flags */
        short   pr_why;         /* Reason for process stop (if stopped) */
        short   pr_what;        /* More detailed reason */
        siginfo32_t pr_info;    /* Info associated with signal or fault */
        short   pr_cursig;      /* Current signal */
        ushort_t pr_nlwp;       /* Number of lwps in the process */
        sigset_t pr_sigpend;    /* Set of signals pending to the process */
        sigset_t pr_sighold;    /* Set of signals held (blocked) by the lwp */
        struct  sigaltstack32 pr_altstack; /* Alternate signal stack info */
        struct  sigaction32 pr_action; /* Signal action for current signal */
        pid32_t pr_pid;         /* Process id */
        pid32_t pr_ppid;        /* Parent process id */
        pid32_t pr_pgrp;        /* Process group id */
        pid32_t pr_sid;         /* Session id */
        timestruc32_t pr_utime; /* Process user cpu time */
        timestruc32_t pr_stime; /* Process system cpu time */
        timestruc32_t pr_cutime; /* Sum of children's user times */
        timestruc32_t pr_cstime; /* Sum of children's system times */
        char    pr_clname[PRCLSZ]; /* Scheduling class name */
        short   pr_syscall;     /* System call number (if in syscall) */
        short   pr_nsysarg;     /* Number of arguments to this syscall */
        int32_t pr_sysarg[PRSYSARGS]; /* Arguments to this syscall */
        id32_t  pr_who;         /* Specific lwp identifier */
        sigset_t pr_lwppend;    /* Set of signals pending to the lwp */
        caddr32_t pr_oldcontext; /* Address of previous ucontext */
        caddr32_t pr_brkbase;   /* Address of the process heap */
        size32_t pr_brksize;    /* Size of the process heap, in bytes */
        caddr32_t pr_stkbase;   /* Address of the process stack */
        size32_t pr_stksize;    /* Size of the process stack, in bytes */
        short   pr_processor;   /* processor which last ran this LWP */
        short   pr_bind;        /* processor LWP bound to or PBIND_NONE */
        int32_t pr_instr;       /* Current instruction */
        prgregset32_t pr_reg;   /* General registers */
} prstatus32_t;

typedef struct prpsinfo32 {
        char    pr_state;       /* numeric process state (see pr_sname) */
        char    pr_sname;       /* printable character representing pr_state */
        char    pr_zomb;        /* !=0: process terminated but not waited for */
        char    pr_nice;        /* nice for cpu usage */
        uint32_t pr_flag;       /* process flags */
        uid32_t pr_uid;         /* real user id */
        gid32_t pr_gid;         /* real group id */
        pid32_t pr_pid;         /* unique process id */
        pid32_t pr_ppid;        /* process id of parent */
        pid32_t pr_pgrp;        /* pid of process group leader */
        pid32_t pr_sid;         /* session id */
        caddr32_t pr_addr;      /* physical address of process */
        size32_t pr_size;       /* size of process image in pages */
        size32_t pr_rssize;     /* resident set size in pages */
        caddr32_t pr_wchan;     /* wait addr for sleeping process */
        timestruc32_t pr_start; /* process start time, sec+nsec since epoch */
        timestruc32_t pr_time;  /* usr+sys cpu time for this process */
        int32_t pr_pri;         /* priority, high value is high priority */
        char    pr_oldpri;      /* pre-SVR4, low value is high priority */
        char    pr_cpu;         /* pre-SVR4, cpu usage for scheduling */
        o_dev_t pr_ottydev;     /* short tty device number */
        dev32_t pr_lttydev;     /* controlling tty device (PRNODEV if none) */
        char    pr_clname[PRCLSZ];      /* scheduling class name */
        char    pr_fname[PRFNSZ];       /* last component of execed pathname */
        char    pr_psargs[PRARGSZ];     /* initial characters of arg list */
        short   pr_syscall;     /* system call number (if in syscall) */
        short   pr_fill;
        timestruc32_t pr_ctime; /* usr+sys cpu time for reaped children */
        size32_t pr_bysize;     /* size of process image in bytes */
        size32_t pr_byrssize;   /* resident set size in bytes */
        int     pr_argc;        /* initial argument count */
        caddr32_t pr_argv;      /* initial argument vector */
        caddr32_t pr_envp;      /* initial environment vector */
        int     pr_wstat;       /* if zombie, the wait() status */
        ushort_t pr_pctcpu;     /* % of recent cpu time, one or all lwps */
        ushort_t pr_pctmem;     /* % of of system memory used by the process */
        uid32_t pr_euid;        /* effective user id */
        gid32_t pr_egid;        /* effective group id */
        id32_t  pr_aslwpid;     /* historical; now always zero */
        char    pr_dmodel;      /* data model of the process */
        char    pr_pad[3];
        int32_t pr_filler[6];   /* for future expansion */
} prpsinfo32_t;


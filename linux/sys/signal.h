# if !defined(SYS_SIGNAL_H)
# define	SYS_SIGNAL_H 1

# include	<linux_types.h>
# include	<sys/procset.h>
# include	<ucontext.h>

# define	ucontext32_t ucontext_t

#ifdef _LP64
#define SI_MAXSZ        256
#define SI_PAD          ((SI_MAXSZ / sizeof (int)) - 4)
#else
#define SI_MAXSZ        128
#define SI_PAD          ((SI_MAXSZ / sizeof (int)) - 3)
#endif
#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
# if !defined(SIGCLD)
#define SIGCLD		17
# endif
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO
#define SIGCANCEL 36    /* reserved signal for thread cancellation */

#define	SIGEMT	65
#define SI_NOINFO       32767   /* no signal information */

#define SI32_MAXSZ      128
#define SI32_PAD        ((SI32_MAXSZ / sizeof (int32_t)) - 3)

/*struct sigaction {
        int sa_flags;
        union {
                void (*_handler)();
                void (*_sigaction)();
        }       _funcptr;
	sigset_t sa_mask;
        int sa_resv[2];
};
*/
typedef struct {
        uint32_t        __sigbits[4];
} sigset32_t;

/*
typedef struct sigaltstack {
        void    *ss_sp;
        size_t  ss_size;
        int     ss_flags;
} stack_t;
*/
typedef struct sigaltstack32 {
        caddr32_t       ss_sp;
        size32_t        ss_size;
        int32_t         ss_flags;
} stack32_t;
struct sigaction32 {
        int32_t         sa_flags;
        union {
                caddr32_t       _handler;
                caddr32_t       _sigaction;
        }       __sigaction_handler;
        sigset32_t      sa_mask;
        int32_t         sa_resv[2];
};
/*#define sa_handler      _funcptr._handler
#define sa_sigaction    _funcptr._sigaction*/

union sigval32 {
        int32_t sival_int;      /* integer value */
        caddr32_t sival_ptr;    /* pointer value */
};

typedef struct siginfo32 {

        int32_t si_signo;                       /* signal from signal.h */
        int32_t si_code;                        /* code from above      */
        int32_t si_errno;                       /* error from errno.h   */

        union {

                int32_t __pad[SI32_PAD];        /* for future growth    */

                struct {                        /* kill(), SIGCLD, siqqueue() */
                        pid32_t __pid;          /* process ID           */
                        union {
                                struct {
                                        uid32_t __uid;
                                        union sigval32  __value;
                                } __kill;
                                struct {
                                        clock32_t __utime;
                                        int32_t __status;
                                        clock32_t __stime;
                                } __cld;
                        } __pdata;
                        id32_t  __ctid;         /* contract ID          */
                        id32_t __zoneid;        /* zone ID              */
                } __proc;

                struct {        /* SIGSEGV, SIGBUS, SIGILL, SIGTRAP, SIGFPE */
                        caddr32_t __addr;       /* faulting address     */
                        int32_t __trapno;       /* illegal trap number  */
                        caddr32_t __pc;         /* instruction address  */
                } __fault;

                struct {                        /* SIGPOLL, SIGXFSZ     */
                /* fd not currently available for SIGPOLL */
                        int32_t __fd;           /* file descriptor      */
                        int32_t __band;
                } __file;

                struct {                        /* SIGPROF */
                        caddr32_t __faddr;      /* last fault address   */
                        timestruc32_t __tstamp; /* real time stamp      */
                        int16_t __syscall;      /* current syscall      */
                        int8_t  __nsysarg;      /* number of arguments  */
                        int8_t  __fault;        /* last fault type      */
                        int32_t __sysarg[8];    /* syscall arguments    */
                        int32_t __mstate[10];   /* see <sys/msacct.h>   */
                } __prof;

                struct {                        /* SI_RCTL */
                        int32_t __entity;       /* type of entity exceeding */
                } __rctl;

        } __data;

} siginfo32_t;

# if 0
union sigval {
        int     sival_int;      /* integer value */
        void    *sival_ptr;     /* pointer value */
};
# endif

# if 0
typedef struct siginfo {                /* pollutes POSIX/XOPEN namespace */
        int     si_signo;                       /* signal from signal.h */
        int     si_code;                        /* code from above      */
        int     si_errno;                       /* error from errno.h   */
        int     si_pad;         /* _LP64 union starts on an 8-byte boundary */
        union {

                int     __pad[SI_PAD];          /* for future growth    */

                struct {                        /* kill(), SIGCLD, siqqueue() */
                        pid_t   __pid;          /* process ID           */
                        union {
                                struct {
                                        uid_t   __uid;
                                        union sigval    __value;
                                } __kill;
                                struct {
                                        clock_t __utime;
                                        int     __status;
                                        clock_t __stime;
                                } __cld;
                        } __pdata;
                        ctid_t  __ctid;         /* contract ID          */
                        zoneid_t __zoneid;      /* zone ID              */
                } __proc;

                struct {        /* SIGSEGV, SIGBUS, SIGILL, SIGTRAP, SIGFPE */
                        void    *__addr;        /* faulting address     */
                        int     __trapno;       /* illegal trap number  */
                        caddr_t __pc;           /* instruction address  */
                } __fault;

                struct {                        /* SIGPOLL, SIGXFSZ     */
                /* fd not currently available for SIGPOLL */
                        int     __fd;           /* file descriptor      */
                        long    __band;
                } __file;

                struct {                        /* SIGPROF */
                        caddr_t __faddr;        /* last fault address   */
                        timestruc_t __tstamp;   /* real time stamp      */
                        short   __syscall;      /* current syscall      */
                        char    __nsysarg;      /* number of arguments  */
                        char    __fault;        /* last fault type      */
                        long    __sysarg[8];    /* syscall arguments    */
                        int     __mstate[10];   /* see <sys/msacct.h>   */
                } __prof;

                struct {                        /* SI_RCTL */
                        int32_t __entity;       /* type of entity exceeding */
                } __rctl;
        } __data;

} siginfo_t;
#define si_pid          __data.__proc.__pid
#define si_utime        __data.__proc.__pdata.__cld.__utime
#define si_stime        __data.__proc.__pdata.__cld.__stime
#define si_status       __data.__proc.__pdata.__cld.__status
#define si_uid          __data.__proc.__pdata.__kill.__uid
#define si_value        __data.__proc.__pdata.__kill.__value
#define si_fd           __data.__file.__fd
#define si_addr         __data.__fault.__addr
#define si_band         __data.__file.__band
# endif

#define si_ctid         __data.__proc.__ctid
#define si_zoneid       __data.__proc.__zoneid
#define si_trapno       __data.__fault.__trapno
#define si_trapafter    __data.__fault.__trapno
#define si_pc           __data.__fault.__pc
#define si_tstamp       __data.__prof.__tstamp
#define si_syscall      __data.__prof.__syscall
#define si_nsysarg      __data.__prof.__nsysarg
#define si_sysarg       __data.__prof.__sysarg
#define si_fault        __data.__prof.__fault
#define si_faddr        __data.__prof.__faddr
#define si_mstate       __data.__prof.__mstate
#define si_entity       __data.__rctl.__entity

# endif

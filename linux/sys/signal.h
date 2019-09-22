/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
 
# if !defined(SYS_SIGNAL_H)
# define	SYS_SIGNAL_H 1

# include	<linux_types.h>
# include	<sys/procset.h>

/**********************************************************************/
/*   Ugly  mess follows. We inline the ARM stuff, because this works  */
/*   for  debian-armel  2.6.32-5-versatile,  but  this  may  not  be  */
/*   accurate  for  other kernels or glibc combinations. Trying this  */
/*   on  Ubuntu  12.04  doesnt,  work  so  we  use  the <ucontext.h>  */
/*   mechanism.							      */
/**********************************************************************/
# if defined(__arm__)
	/***********************************************/
	/*   Following            based            on  */
	/*   /usr/include/sys/procfs.h		       */
	/***********************************************/
	# include	<sys/user.h>
	typedef unsigned long elf_greg_t;
	#define ELF_NGREG (sizeof (struct user_regs) / sizeof(elf_greg_t))
	typedef elf_greg_t elf_gregset_t[ELF_NGREG];
	typedef struct user_fpregs elf_fpregset_t;

# else /* defined(__arm__) */

#   include	<ucontext.h>
# endif 

# define	ucontext32_t ucontext_t

#define SI_FROMUSER(sip)        ((sip)->si_code <= 0)
#define SI_FROMKERNEL(sip)      ((sip)->si_code > 0)
/*  indication whether to queue the signal or not */
#define SI_CANQUEUE(c)  ((c) <= SI_QUEUE)

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
# if !defined(SIGIOT)
#define SIGIOT		 6
# endif
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
# if !defined(SIGIO)
#define SIGIO		29
# endif
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
#define sa_handler32      __sigaction_handler._handler
#define sa_sigaction32    __sigaction_handler._sigaction

union sigval32 {
        int32_t sival_int;      /* integer value */
        caddr32_t sival_ptr;    /* pointer value */
};

/* X/Open requires some more fields with fixed names.  */
# undef si_pid		
# undef si_uid		
# undef si_timerid	
# undef si_overrun	
# undef si_status	
# undef si_utime	
# undef si_stime	
# undef si_value	
# undef si_int		
# undef si_ptr		
# undef si_addr	
# undef si_band	
# undef si_fd		

typedef struct siginfo32
  {
    int si_signo;		/* Signal number.  */
    int si_errno;		/* If non-zero, an errno value associated with
				   this signal, as defined in <errno.h>.  */
    int si_code;		/* Signal code.  */

    union
      {
	int _pad[__SI_PAD_SIZE];

	 /* kill().  */
	struct
	  {
	    __pid_t si_pid;	/* Sending process ID.  */
	    __uid_t si_uid;	/* Real user ID of sending process.  */
	  } _kill;

	/* POSIX.1b timers.  */
	struct
	  {
	    int si_tid;		/* Timer ID.  */
	    int si_overrun;	/* Overrun count.  */
	    sigval_t si_sigval;	/* Signal value.  */
	  } _timer;

	/* POSIX.1b signals.  */
	struct
	  {
	    __pid_t si_pid;	/* Sending process ID.  */
	    __uid_t si_uid;	/* Real user ID of sending process.  */
	    sigval_t si_sigval;	/* Signal value.  */
	  } _rt;

	/* SIGCHLD.  */
	struct
	  {
	    __pid_t si_pid;	/* Which child.  */
	    __uid_t si_uid;	/* Real user ID of sending process.  */
	    int si_status;	/* Exit value or signal.  */
	    __clock_t si_utime;
	    __clock_t si_stime;
	  } _sigchld;

	/* SIGILL, SIGFPE, SIGSEGV, SIGBUS.  */
	struct
	  {
	    void *si_addr;	/* Faulting insn/memory ref.  */
	  } _sigfault;

	/* SIGPOLL.  */
	struct
	  {
	    long int si_band;	/* Band event for SIGPOLL.  */
	    int si_fd;
	  } _sigpoll;
      } _sifields;
  } siginfo32_t;
/* X/Open requires some more fields with fixed names.  */
# define si_pid		_sifields._kill.si_pid
# define si_uid		_sifields._kill.si_uid
# define si_timerid	_sifields._timer.si_tid
# define si_overrun	_sifields._timer.si_overrun
# define si_status	_sifields._sigchld.si_status
# define si_utime	_sifields._sigchld.si_utime
# define si_stime	_sifields._sigchld.si_stime
# define si_value	_sifields._rt.si_sigval
# define si_int		_sifields._rt.si_sigval.sival_int
# define si_ptr		_sifields._rt.si_sigval.sival_ptr
# define si_addr	_sifields._sigfault.si_addr
# define si_band	_sifields._sigpoll.si_band
# define si_fd		_sifields._sigpoll.si_fd


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
#define si_utime        __data.__proc.__pdata.__cld.__utime
#define si_stime        __data.__proc.__pdata.__cld.__stime
#define si_status       __data.__proc.__pdata.__cld.__status
#define si_uid          __data.__proc.__pdata.__kill.__uid
#define si_value        __data.__proc.__pdata.__kill.__value
#define si_fd           __data.__file.__fd
#define si_addr         __data.__fault.__addr
#define si_band         __data.__file.__band
#define si_ctid         __data.__proc.__ctid
# endif

# endif

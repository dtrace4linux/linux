/**********************************************************************/
/*   Generic  barrier  include  file  to protect us from many of the  */
/*   fiddly differences between Solaris and Linux. We define much of  */
/*   whats  in  Solaris - deferring to specific files if it warrants  */
/*   cleaning this mess up.					      */
/**********************************************************************/

# if !defined(LINUX_TYPES_H)
#define	LINUX_TYPES_H 1

#define HERE()	if (dtrace_here) {printk("%s:%s:%d: we are here\n", dtrace_basename(__FILE__), __func__, __LINE__);}
#define DHERE()	dtrace_printf("%s:%s:%d: we are here\n", dtrace_basename(__FILE__), __func__, __LINE__);
#define HERE2() if (dtrace_here) {printk("%s:%s:%d: XYZ we are here2\n", dtrace_basename(__FILE__), __func__, __LINE__);}
#define HERE_WITH_INFO(str)	if (dtrace_here) {printk("%s:%s:%d:%s\n", dtrace_basename(__FILE__), __func__, __LINE__, str);}
#define TODO()	printk("%s:%s:%d: TODO:please fill me in\n", dtrace_basename(__FILE__), __func__, __LINE__)
#define TODO_END()

	/***********************************************/
	/*   Introduce  big  delays  to  test out smp  */
	/*   code.				       */
	/***********************************************/
#define DELAY(n) \
	{static int f = 1; \
	int	i; \
	if (f) { \
		printk("%s(%d): DELAY(%d)\n", __FILE__, __LINE__, n); f = 0; \
	} \
	for (i = 0; i < n; i++) smp_processor_id(); \
	}

# if defined(__i386) && !defined(_LP32)
#	define _LP32
# endif

#define	_INT64_TYPE
#define	_LITTLE_ENDIAN 1

/**********************************************************************/
/*   GCC  doesnt  use  pragmas but uses function attributes. Do this  */
/*   here.							      */
/**********************************************************************/
#define pragma_init	__attribute__((__constructor__))
#define pragma_fini	__attribute__((__destructor__))

/**********************************************************************/
/*   When  trying  to  debug  where we are failing, we convert "goto  */
/*   xxx;"  into this so we can figure out which line we are failing  */
/*   on.							      */
/**********************************************************************/
#define GOTO(x) do {printf("%s(%d): error\n", dtrace_basename(__FILE__), __LINE__); goto x; } while(0)

/**********************************************************************/
/*   In   x86   mode,  kernel  compiled  with  arguments  passed  in  */
/*   registers. Turn it off for some of the assembler code.	      */
/**********************************************************************/
# if defined(__i386)
#	define ATTR__REGPARM0 __attribute__ ((regparm(0)))
# else
#	define ATTR__REGPARM0
# endif

struct modctl;

/**********************************************************************/
/*   The  following include dependencies are horrible, where we take  */
/*   a  bit  of  Linux and Solaris and try to come up with the right  */
/*   set  of  definitions.  I suggest making small changes at a time  */
/*   and validating it will compile before too much surgery. Some of  */
/*   this is likely to be cleaned up in the future.		      */
/**********************************************************************/
# include <linux/version.h>

# if __KERNEL__

	#define modctl module

	# include	<linux/time.h>
	# include	<linux/module.h>
	# undef zone

	# include	<sys/model.h>
	# include	<sys/bitmap.h>
	# include	<sys/processor.h>
	# include	<sys/systm.h>
	# include 	<sys/vmem.h>
	# include 	<linux/sys/cred.h>

	#define	_LARGEFILE_SOURCE	1
	#define	_FILE_OFFSET_BITS	64
	#define 	__USE_LARGEFILE64 1

	# include	<linux/types.h>
	# include	<linux/wait.h>
	# include	<linux/kdev_t.h>
	# include	<linux/version.h>
	# include	<zone.h>
	# if defined(HAVE_INCLUDE_LINUX_MUTEX_H)
	#   include	<linux/mutex.h>
	# endif
	# if defined(HAVE_INCLUDE_LINUX_STACKTRACE_H)
	#   include	<linux/stacktrace.h>
	# endif
	# if defined(HAVE_INCLUDE_ASM_STACKTRACE_H)
	#   include	<asm/stacktrace.h>
	# endif

	#define MUTEX_NOT_HELD(x)	!mutex_is_locked(x)

	#define PS_VM 0x00020000 /* CPU in v8086 mode */

	/***********************************************/
	/*   Linux 2.6.9			       */
	/***********************************************/
	# if !defined(task_pt_regs)
//	#	define task_pt_regs(task) ((struct pt_regs *) (task)->thread.sp0 - 1)
	#	define task_pt_regs(task) ((struct pt_regs *) (task)->thread.rsp0 - 1)
	# endif

	/***********************************************/
	/*   Needed     for     trap    handler    in  */
	/*   dtrace_linux.c			       */
	/***********************************************/
	# if defined(__i386)
	#	define	dotraplinkage
	# else
	#	define	dotraplinkage asmlinkage
	# endif

	#if defined(__amd64)
	#   define	stack_ptr(regs) regs->r_rsp
	# else
	#   define	stack_ptr(regs) &regs->r_rsp
	#endif

# else /* !__KERNEL__ */

	#define	_LARGEFILE_SOURCE	1
	#define	_LARGEFILE64_SOURCE	1
	#define	_FILE_OFFSET_BITS	64
	#define 	__USE_LARGEFILE64 1

	# include "/usr/include/sys/types.h"
	# include	<linux/kdev_t.h>
	# include	<pthread.h>
	# include	<features.h>
	# include	<time.h>

	#define __USE_GNU 1 /* Need Lmid_t type */
	# include	<dlfcn.h>
	# undef __USE_GNU
	
	# include	<sys/time.h>
	# include	<sys/processor.h>
	# include	<sys/systm.h>
	# include 	<sys/vmem.h>

# endif /* __KERNEL__ */

# include 	<sys/regset.h>

#define 	DEFAULTMUTEX PTHREAD_MUTEX_INITIALIZER

// fixme : objfs.h
#define	OBJFS_ROOT	"/system/object"

#define	NPRGREG32 NGREG

#define P2ROUNDUP(x, align)             (-(-(x) & -(align)))

#define FPS_TOP 0x00003800      /* top of stack pointer                 */


typedef unsigned long long hrtime_t;

#define	ABS(x) ((x) < 0 ? -(x) : (x))

/**********************************************************************/
/*   Typedefs for kernel driver building.			      */
/**********************************************************************/
# if __KERNEL__

	# include "../port.h"
	# include <asm/signal.h>
	# include <linux/sched.h>
	#define	SNOCD	0

	#define aston(x) 

	#define	t_did pid
	#define	p_parent parent
	#define klwp_t struct thread_struct
	#define ttolwp(p) (p)->p_task->thread


	#define        MIN(a,b) (((a)<(b))?(a):(b))
	#define UINT8_MAX       (255U)
	#if !defined(USHRT_MAX)
	#  define	USHRT_MAX	0xffff
	#endif
	#define	UINT16_MAX	0xffff
	#define	INT32_MAX	0x7fffffff
	#define	UINT32_MAX	0xffffffff
	#define	INT64_MAX	0x7fffffffffffffffLL
	#define	UINT64_MAX	0xffffffffffffffffLU
	#define INT64_MIN       (-9223372036854775807LL-1)
	typedef struct __dev_info *dev_info_t;
	typedef long	intptr_t;
	typedef unsigned long long off64_t;
	typedef void *taskq_t;
	#define uintptr_t unsigned long
	#define kmem_cache_t struct kmem_cache
	typedef void *kthread_t;

	#define	NBBY	8
	#define	bcmp(a, b, c) memcmp(a, b, c)

	#define	KM_SLEEP GFP_KERNEL
	#define	KM_NOSLEEP GFP_ATOMIC
	#define KM_NORMALPRI 0

	#define	CE_WARN	0
	#define	CE_NOTE	1
	#define	CE_CONT	2
	#define	CE_PANIC	3
	#define	CE_IGNORE	4

	#define	NCPU NR_CPUS
	#define PAGESIZE        PAGE_SIZE
	#define PAGEOFFSET      (PAGESIZE - 1)

	typedef void    *timeout_id_t;

	/**********************************************************************/
	/*   Protoypes.							      */
	/**********************************************************************/
	void cmn_err(int ce, const char *fmt, ...);
	void	*kmem_alloc(size_t, int);

	#define	bcopy(a, b, c) dtrace_memcpy(b, a, c)
	#define	bzero(a, b) dtrace_bzero(a, b)

	# if defined(HAVE_INCLUDE_LINUX_SEMAPHORE_H)
	#   include	<linux/semaphore.h>
	# endif
	# if defined(HAVE_INCLUDE_ASM_SEMAPHORE_H)
	#   include	<asm/semaphore.h>
	# endif
	# if defined(HAVE_INCLUDE_LINUX_SLAB_H)
	#     include <linux/slab.h>
	# endif

/* see cpuvar.h
	#if defined(__i386) || defined(__amd64)
	typedef uchar_t instr_t;
	#endif

	# if defined(__arm__)
	typedef uint32_t instr_t;
	# endif

*/	/***********************************************/
	/*   Define our own private mutexes so we can  */
	/*   avoid  problems  and clashes with kernel  */
	/*   implemented ones. Our mutexes are really  */
	/*   semaphores,  since we cannot use a mutex  */
	/*   inside an interrupt.		       */
	/***********************************************/
	typedef struct mutex mutex_t;
	#define kmutex_t mutex_t
	void mutex_enter(mutex_t *mp);
	void mutex_exit(mutex_t *mp);
	
	# include	<sys/cpuvar_defs.h>
	# include	<sys/cpuvar.h>
# endif /* __KERNEL__ */

/**********************************************************************/
/*   Made into functions so we can monitor memallocations.	      */
/**********************************************************************/
//#define kmem_zalloc(size, flags) kzalloc(size, flags)
//#define kmem_alloc(size, flags) kmalloc(size, flags)
//#define kmem_free(ptr, size) kfree(ptr)

/**********************************************************************/
/*   Definitions for both kernel and non-kernel code.		      */
/**********************************************************************/
typedef short   o_dev_t;                /* old device type      */
typedef unsigned short ushort_t;
typedef unsigned char uchar_t;
# if !defined(__KERNEL__)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
# endif
typedef unsigned int uint_t;
typedef unsigned long ulong_t;
typedef unsigned long long u_longlong_t;
typedef long long longlong_t;
typedef long long offset_t;
typedef unsigned long pc_t;
//typedef ulong_t          Lmid_t;
typedef uint_t lwpid_t;
typedef u_longlong_t    core_content_t;
typedef unsigned long   psaddr_t;
typedef int	ctid_t;                 /* contract ID type     */
typedef int	prgreg32_t;
typedef struct iovec iovec_t;
#define	ino64_t	ino_t
#define	blkcnt64_t blkcnt_t

# if !defined(ENOTSUP)
#define ENOTSUP EOPNOTSUPP
# endif
#define MAXPATHLEN 1024

/**********************************************************************/
/*   Userland - non-kernel definitions.				      */
/**********************************************************************/
# include	<sys/types32.h>
# include	<sys/sysmacros.h>
# if !__KERNEL__

	// Used by Pcore.c
	#define PF_SUNW_FAILURE 0x00100000      /* mapping absent due to failure */
	#define PN_XNUM         0xffff          /* extended program header index */
	#define SHT_SUNW_LDYNSYM        0x6ffffff3

	int mutex_init(pthread_mutex_t *, int, void *);
	#define	mutex_destroy(x)	pthread_mutex_destroy(x)
	#define	mutex_lock(x)		pthread_mutex_lock(x)
	#define	mutex_unlock(x)		pthread_mutex_unlock(x)

	#define printk printf
	#define MIN(a, b) ((a) < (b) ? (a) : (b))
	#define MAX(a, b) ((a) > (b) ? (a) : (b))

	#define PAGESIZE        (sysconf(_SC_PAGESIZE)) /* All the above, for logical */

	#define mutex_t pthread_mutex_t

	struct mutex {
		long xxx;
		};
	#define kmutex_t struct mutex
	#define	NCPU 8 // arbitrary number we should use the real value
	# include	<sys/types.h>
	# include	<stdint.h>
	# include	<unistd.h>
	# include	<sys/stat.h>
	# include	<sys/wait.h>
	# include	<zone.h>

	#define SHT_SUNW_dof            0x6ffffff4
	#define SHT_PROGBITS    1               /* Program specific (private) data */
	#define STT_OBJECT      1               /* Symbol is a data object */

	/***********************************************/
	/*   My SimplyMEPIS system doesnt define this  */
	/*   - so lets see if we can autodetect this.  */
	/***********************************************/
	# if defined(_PTHREAD_DESCR_DEFINED)
	typedef void *pthread_rwlock_t;
	# endif
# endif

# if !defined(roundup)
	#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
# endif

typedef union {
        long double     _q;
        uint32_t        _l[4];
} upad128_t;

#define SIG2STR_MAX     32

#define	B_TRUE	1
#define	B_FALSE	0
# if !defined(TRUE)
#	define	TRUE	1
#	define	FALSE	0
# endif

#define SI_SYSNAME              1       /* return name of operating system */
#define SI_RELEASE              3       /* return release of operating system */

#define SI_PLATFORM             513     /* return platform identifier */
#define SI_ISALIST              514     /* returnsupported isa list */

#define SYS_vfork       119
#define SYS_fork1       143
#define SYS_forkall     2
#define SYS_execve      59
#define SYS_statvfs     103
#define SYS_stat        18
#define SYS_stat64              215
#define	SYS_rename      134
#define SYS_link        9
#define SYS_unlink      10
#define SYS_sigaction   98
#define SYS_xstat       123
#define SYS_lxstat      124
#define SYS_fxstat      125
#define SYS_lstat       88
#define SYS_lstat64             216
#define SYS_fstat64             217
#define SYS_fstat       28
#define SYS_pset                207
#define SYS_munmap      117
#define SYS_open        5
#define SYS_creat       8
#define SYS_fstatvfs    104
#define SYS_zone                227
#define SYS_tasksys     70
#define SYS_waitid     107
#define SYS_close       6
#define SYS_access      33
#define SYS_processor_bind      187
#define SYS_mmap        115
#define SYS_memcntl     131
#define SYS_meminfosys          SYS_lgrpsys
#define SYS_lgrpsys             180
#define SYS_exit        1
#define SYS_uadmin      55
#define SYS_ioctl       54
#define SYS_llseek              175
#define SYS_lseek       19
#define SYS_exec SYS_execve
#define SYS_fcntl       62
#define SYS_rctlsys     74
#define SYS_getitimer           157
#define SYS_setitimer           158
#define SYS_setrlimit64         220
#define SYS_getrlimit64         221
#define SYS_setrlimit   128
#define SYS_getrlimit   129
#define SYS_lwp_exit            160
#define SYS_door                201
#define SYS_getsockname         244
#define SYS_getpeername         243
#define SYS_getsockopt          245
#define	SYS_forksys		999

/* fcntl.h */
#define F_ALLOCSP       10      /* Reserved */
#define F_FREESP        27      /* Free file space */
#define F_SHARE         40      /* Set a file share reservation */
#define F_UNSHARE       41      /* Remove a file share reservation */

/* modctl.h */
#define       MODGETPATH              6
#define       MODGETPATHLEN           14

/* time.h */
/*
 *      Definitions for commonly used resolutions.
 */
#define SEC             1
#define MILLISEC        1000
#define MICROSEC        1000000
#define NANOSEC         1000000000

/* zone.h */
#define GLOBAL_ZONEID   0

typedef struct timespec timestruc_t;

#if !defined(_LP64) && defined(__cplusplus)
typedef ulong_t major_t;        /* major part of device number */
typedef ulong_t minor_t;        /* minor part of device number */
#else
typedef uint_t major_t;
typedef uint_t minor_t;
#endif

#define	makedev		MKDEV
#define	makedevice	MKDEV
#define	getminor(x)	MINOR(x)
#define	getmajor(x)	MAJOR(x)
#define	minor(x)	MINOR(x)
#define	major(x)	MAJOR(x)

typedef struct flock64_32 {
        int16_t l_type;
        int16_t l_whence;
        off64_t l_start;
        off64_t l_len;          /* len == 0 means until end of file */
        int32_t l_sysid;
        pid32_t l_pid;
        int32_t l_pad[4];               /* reserve area */
} flock64_32_t;

/*
 * File share reservation type
 */
typedef struct fshare {
        short   f_access;
        short   f_deny;
        int     f_id;
} fshare_t;

/* time.h */
struct itimerval32 {
        struct  timeval32 it_interval;
        struct  timeval32 it_value;
};

/* sys/socketvar.h */
/*
 * Socket versions. Used by the socket library when calling _so_socket().
 */
#define SOV_STREAM      0       /* Not a socket - just a stream */
#define SOV_DEFAULT     1       /* Select based on so_default_version */
#define SOV_SOCKSTREAM  2       /* Socket plus streams operations */
#define SOV_SOCKBSD     3       /* Socket with no streams operations */
#define SOV_XPG4_2      4       /* Xnet socket */

/* mman.h */
#define _MAP_NEW        0x80000000      /* users should not need to use this */

#define MISYS_MEMINFO           0x0
struct memcntl_mha {
        uint_t          mha_cmd;        /* command(s) */
        uint_t          mha_flags;
        size_t          mha_pagesize;
};

struct memcntl_mha32 {
        uint_t          mha_cmd;        /* command(s) */
        uint_t          mha_flags;
        size32_t        mha_pagesize;
};
#define MC_HAT_ADVISE   7               /* advise hat map size */
typedef struct meminfo {
        const uint64_t *mi_inaddr;      /* array of input addresses */
        const uint_t *mi_info_req;      /* array of types of info requested */
        uint64_t *mi_outdata;           /* array of results are placed */
        uint_t *mi_validity;            /* array of bitwise result codes */
        int mi_info_count;              /* number of pieces of info requested */
} meminfo_t;
typedef struct meminfo32 {
        caddr32_t mi_inaddr;    /* array of input addresses */
        caddr32_t mi_info_req;  /* array of types of information requested */
        caddr32_t mi_outdata;   /* array of results are placed */
        caddr32_t mi_validity;  /* array of bitwise result codes */
        int32_t mi_info_count;  /* number of pieces of information requested */
} meminfo32_t;


/*
 * seg_rw gives the access type for a fault operation
 */
enum seg_rw {
        S_OTHER,                /* unknown or not touched */
        S_READ,                 /* read access attempted */
        S_WRITE,                /* write access attempted */
        S_EXEC,                 /* execution access attempted */
        S_CREATE,               /* create if page doesn't exist */
        S_READ_NOCOW            /* read access, don't do a copy on write */
};

#undef ASSERT
#define	ASSERT(EX)	((void)((EX) || \
			dtrace_assfail(#EX, dtrace_basename(__FILE__), __LINE__)))
extern int dtrace_assfail(const char *, const char *, int);
int	dtrace_mach_aframes(void);

extern int dtrace_here;
extern unsigned long long gethrtime(void);
void *dtrace_casptr(void *target, void *cmp, void *new);
#define	casptr(a, b, c) dtrace_casptr(a, b, c)

#define atomic_add_32(a, b) atomic_add(b, (atomic_t *) (a))
void atomic_add_64(uint64_t *, int n);

char	*dtrace_basename(char *);
void	dtrace_bzero(void *dst, int len);
void	dtrace_memcpy(void *dst, const void *src, int len);

# endif /* LINE_TYPES_H */

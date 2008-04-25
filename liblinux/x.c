# 1 "proc_names.c"
# 1 "/home/fox/src/dtrace/liblinux//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "proc_names.c"
# 10 "proc_names.c"
#pragma ident "@(#)proc_names.c	1.24	04/02/27 SMI"

# 1 "../linux/stdio.h" 1
# 1 "../linux/linux_types.h" 1



struct modctl;
# 24 "../linux/linux_types.h"
# 1 "/usr/include/features.h" 1 3 4
# 335 "/usr/include/features.h" 3 4
# 1 "/usr/include/sys/cdefs.h" 1 3 4
# 360 "/usr/include/sys/cdefs.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 361 "/usr/include/sys/cdefs.h" 2 3 4
# 336 "/usr/include/features.h" 2 3 4
# 359 "/usr/include/features.h" 3 4
# 1 "/usr/include/gnu/stubs.h" 1 3 4



# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 5 "/usr/include/gnu/stubs.h" 2 3 4




# 1 "/usr/include/gnu/stubs-64.h" 1 3 4
# 10 "/usr/include/gnu/stubs.h" 2 3 4
# 360 "/usr/include/features.h" 2 3 4
# 25 "../linux/linux_types.h" 2

# 1 "/usr/include/time.h" 1 3 4
# 31 "/usr/include/time.h" 3 4








# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 214 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 40 "/usr/include/time.h" 2 3 4



# 1 "/usr/include/bits/time.h" 1 3 4
# 44 "/usr/include/time.h" 2 3 4
# 57 "/usr/include/time.h" 3 4
# 1 "/usr/include/bits/types.h" 1 3 4
# 28 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 29 "/usr/include/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;







typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
# 131 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/typesizes.h" 1 3 4
# 132 "/usr/include/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;

typedef int __daddr_t;
typedef long int __swblk_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;

typedef long int __ssize_t;



typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;
# 58 "/usr/include/time.h" 2 3 4



typedef __clock_t clock_t;



# 75 "/usr/include/time.h" 3 4


typedef __time_t time_t;



# 93 "/usr/include/time.h" 3 4
typedef __clockid_t clockid_t;
# 105 "/usr/include/time.h" 3 4
typedef __timer_t timer_t;
# 121 "/usr/include/time.h" 3 4
struct timespec
  {
    __time_t tv_sec;
    long int tv_nsec;
  };








struct tm
{
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;


  long int tm_gmtoff;
  __const char *tm_zone;




};








struct itimerspec
  {
    struct timespec it_interval;
    struct timespec it_value;
  };


struct sigevent;





typedef __pid_t pid_t;








extern clock_t clock (void) __attribute__ ((__nothrow__));


extern time_t time (time_t *__timer) __attribute__ ((__nothrow__));


extern double difftime (time_t __time1, time_t __time0)
     __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern time_t mktime (struct tm *__tp) __attribute__ ((__nothrow__));





extern size_t strftime (char *__restrict __s, size_t __maxsize,
   __const char *__restrict __format,
   __const struct tm *__restrict __tp) __attribute__ ((__nothrow__));

# 229 "/usr/include/time.h" 3 4



extern struct tm *gmtime (__const time_t *__timer) __attribute__ ((__nothrow__));



extern struct tm *localtime (__const time_t *__timer) __attribute__ ((__nothrow__));





extern struct tm *gmtime_r (__const time_t *__restrict __timer,
       struct tm *__restrict __tp) __attribute__ ((__nothrow__));



extern struct tm *localtime_r (__const time_t *__restrict __timer,
          struct tm *__restrict __tp) __attribute__ ((__nothrow__));





extern char *asctime (__const struct tm *__tp) __attribute__ ((__nothrow__));


extern char *ctime (__const time_t *__timer) __attribute__ ((__nothrow__));







extern char *asctime_r (__const struct tm *__restrict __tp,
   char *__restrict __buf) __attribute__ ((__nothrow__));


extern char *ctime_r (__const time_t *__restrict __timer,
        char *__restrict __buf) __attribute__ ((__nothrow__));




extern char *__tzname[2];
extern int __daylight;
extern long int __timezone;




extern char *tzname[2];



extern void tzset (void) __attribute__ ((__nothrow__));



extern int daylight;
extern long int timezone;





extern int stime (__const time_t *__when) __attribute__ ((__nothrow__));
# 312 "/usr/include/time.h" 3 4
extern time_t timegm (struct tm *__tp) __attribute__ ((__nothrow__));


extern time_t timelocal (struct tm *__tp) __attribute__ ((__nothrow__));


extern int dysize (int __year) __attribute__ ((__nothrow__)) __attribute__ ((__const__));
# 327 "/usr/include/time.h" 3 4
extern int nanosleep (__const struct timespec *__requested_time,
        struct timespec *__remaining);



extern int clock_getres (clockid_t __clock_id, struct timespec *__res) __attribute__ ((__nothrow__));


extern int clock_gettime (clockid_t __clock_id, struct timespec *__tp) __attribute__ ((__nothrow__));


extern int clock_settime (clockid_t __clock_id, __const struct timespec *__tp)
     __attribute__ ((__nothrow__));






extern int clock_nanosleep (clockid_t __clock_id, int __flags,
       __const struct timespec *__req,
       struct timespec *__rem);


extern int clock_getcpuclockid (pid_t __pid, clockid_t *__clock_id) __attribute__ ((__nothrow__));




extern int timer_create (clockid_t __clock_id,
    struct sigevent *__restrict __evp,
    timer_t *__restrict __timerid) __attribute__ ((__nothrow__));


extern int timer_delete (timer_t __timerid) __attribute__ ((__nothrow__));


extern int timer_settime (timer_t __timerid, int __flags,
     __const struct itimerspec *__restrict __value,
     struct itimerspec *__restrict __ovalue) __attribute__ ((__nothrow__));


extern int timer_gettime (timer_t __timerid, struct itimerspec *__value)
     __attribute__ ((__nothrow__));


extern int timer_getoverrun (timer_t __timerid) __attribute__ ((__nothrow__));
# 416 "/usr/include/time.h" 3 4

# 27 "../linux/linux_types.h" 2
# 1 "/usr/include/sys/time.h" 1 3 4
# 29 "/usr/include/sys/time.h" 3 4
# 1 "/usr/include/bits/time.h" 1 3 4
# 69 "/usr/include/bits/time.h" 3 4
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
# 30 "/usr/include/sys/time.h" 2 3 4

# 1 "/usr/include/sys/select.h" 1 3 4
# 31 "/usr/include/sys/select.h" 3 4
# 1 "/usr/include/bits/select.h" 1 3 4
# 32 "/usr/include/sys/select.h" 2 3 4


# 1 "/usr/include/bits/sigset.h" 1 3 4
# 24 "/usr/include/bits/sigset.h" 3 4
typedef int __sig_atomic_t;




typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
  } __sigset_t;
# 35 "/usr/include/sys/select.h" 2 3 4



typedef __sigset_t sigset_t;







# 1 "/usr/include/bits/time.h" 1 3 4
# 47 "/usr/include/sys/select.h" 2 3 4


typedef __suseconds_t suseconds_t;





typedef long int __fd_mask;
# 67 "/usr/include/sys/select.h" 3 4
typedef struct
  {






    __fd_mask __fds_bits[1024 / (8 * sizeof (__fd_mask))];


  } fd_set;






typedef __fd_mask fd_mask;
# 99 "/usr/include/sys/select.h" 3 4

# 109 "/usr/include/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 121 "/usr/include/sys/select.h" 3 4
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);



# 32 "/usr/include/sys/time.h" 2 3 4








# 57 "/usr/include/sys/time.h" 3 4
struct timezone
  {
    int tz_minuteswest;
    int tz_dsttime;
  };

typedef struct timezone *__restrict __timezone_ptr_t;
# 73 "/usr/include/sys/time.h" 3 4
extern int gettimeofday (struct timeval *__restrict __tv,
    __timezone_ptr_t __tz) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));




extern int settimeofday (__const struct timeval *__tv,
    __const struct timezone *__tz)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));





extern int adjtime (__const struct timeval *__delta,
      struct timeval *__olddelta) __attribute__ ((__nothrow__));




enum __itimer_which
  {

    ITIMER_REAL = 0,


    ITIMER_VIRTUAL = 1,



    ITIMER_PROF = 2

  };



struct itimerval
  {

    struct timeval it_interval;

    struct timeval it_value;
  };






typedef int __itimer_which_t;




extern int getitimer (__itimer_which_t __which,
        struct itimerval *__value) __attribute__ ((__nothrow__));




extern int setitimer (__itimer_which_t __which,
        __const struct itimerval *__restrict __new,
        struct itimerval *__restrict __old) __attribute__ ((__nothrow__));




extern int utimes (__const char *__file, __const struct timeval __tvp[2])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));



extern int lutimes (__const char *__file, __const struct timeval __tvp[2])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern int futimes (int __fd, __const struct timeval __tvp[2]) __attribute__ ((__nothrow__));
# 191 "/usr/include/sys/time.h" 3 4

# 28 "../linux/linux_types.h" 2

# 1 "../linux/sys/processor.h" 1



typedef unsigned int processorid_t;
# 30 "../linux/linux_types.h" 2
# 1 "../linux/sys/systm.h" 1
# 31 "../linux/linux_types.h" 2
# 1 "../linux/sys/vmem.h" 1
# 24 "../linux/sys/vmem.h"
typedef struct vmem vmem_t;
# 32 "../linux/linux_types.h" 2
# 1 "../linux/sys/cred.h" 1



typedef struct cred {
 int cr_uid;
 int cr_ruid;
 int cr_suid;
 int cr_gid;
 int cr_rgid;
 int cr_sgid;
 } cred_t;

cred_t *CRED(void);
# 33 "../linux/linux_types.h" 2
# 56 "../linux/linux_types.h"
extern int pread(int, void *, int, unsigned long long);
extern int pwrite(int, void *, int, unsigned long long);
# 106 "../linux/linux_types.h"
typedef unsigned long long hrtime_t;




typedef unsigned int mutex_t;
# 171 "../linux/linux_types.h"
typedef short o_dev_t;

typedef unsigned short ushort_t;
typedef unsigned char uchar_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef char * caddr_t;

typedef unsigned int uint_t;
typedef unsigned int ulong_t;

typedef unsigned long long u_longlong_t;
typedef long long longlong_t;
typedef long long offset_t;
typedef unsigned long long rd_agent_t;
typedef unsigned long pc_t;
typedef ulong_t Lmid_t;
typedef uint_t lwpid_t;

typedef u_longlong_t core_content_t;
typedef unsigned long psaddr_t;
typedef int ctid_t;
typedef int zoneid_t;
typedef int prgreg32_t;
typedef struct iovec iovec_t;






# 1 "../linux/sys/types32.h" 1
# 30 "../linux/sys/types32.h"
#pragma ident "@(#)types32.h  1.5     05/06/08 SMI"

# 1 "../linux/sys/int_types.h" 1
# 33 "../linux/sys/types32.h" 2
# 48 "../linux/sys/types32.h"
typedef uint32_t caddr32_t;
typedef int32_t daddr32_t;
typedef int32_t off32_t;
typedef uint32_t ino32_t;
typedef int32_t blkcnt32_t;
typedef uint32_t fsblkcnt32_t;
typedef uint32_t fsfilcnt32_t;
typedef int32_t id32_t;
typedef uint32_t major32_t;
typedef uint32_t minor32_t;
typedef int32_t key32_t;
typedef uint32_t mode32_t;
typedef int32_t uid32_t;
typedef int32_t gid32_t;
typedef uint32_t nlink32_t;
typedef uint32_t dev32_t;
typedef int32_t pid32_t;
typedef uint32_t size32_t;
typedef int32_t ssize32_t;
typedef int32_t time32_t;
typedef int32_t clock32_t;

struct timeval32 {
        time32_t tv_sec;
        int32_t tv_usec;
};

typedef struct timespec32 {
        time32_t tv_sec;
        int32_t tv_nsec;
} timespec32_t;

typedef struct timespec32 timestruc32_t;

typedef struct itimerspec32 {
        struct timespec32 it_interval;
        struct timespec32 it_value;
} itimerspec32_t;
# 206 "../linux/linux_types.h" 2

 struct mutex {
  long xxx;
  };
 typedef unsigned long long loff_t;


# 1 "../linux/sys/types.h" 1



# 1 "../linux/linux_types.h" 1
# 5 "../linux/sys/types.h" 2
# 214 "../linux/linux_types.h" 2
# 1 "/usr/include/stdint.h" 1 3 4
# 27 "/usr/include/stdint.h" 3 4
# 1 "/usr/include/bits/wchar.h" 1 3 4
# 28 "/usr/include/stdint.h" 2 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 29 "/usr/include/stdint.h" 2 3 4
# 37 "/usr/include/stdint.h" 3 4
typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;

typedef long int int64_t;







typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;

typedef unsigned int uint32_t;



typedef unsigned long int uint64_t;
# 66 "/usr/include/stdint.h" 3 4
typedef signed char int_least8_t;
typedef short int int_least16_t;
typedef int int_least32_t;

typedef long int int_least64_t;






typedef unsigned char uint_least8_t;
typedef unsigned short int uint_least16_t;
typedef unsigned int uint_least32_t;

typedef unsigned long int uint_least64_t;
# 91 "/usr/include/stdint.h" 3 4
typedef signed char int_fast8_t;

typedef long int int_fast16_t;
typedef long int int_fast32_t;
typedef long int int_fast64_t;
# 104 "/usr/include/stdint.h" 3 4
typedef unsigned char uint_fast8_t;

typedef unsigned long int uint_fast16_t;
typedef unsigned long int uint_fast32_t;
typedef unsigned long int uint_fast64_t;
# 120 "/usr/include/stdint.h" 3 4
typedef long int intptr_t;


typedef unsigned long int uintptr_t;
# 135 "/usr/include/stdint.h" 3 4
typedef long int intmax_t;
typedef unsigned long int uintmax_t;
# 215 "../linux/linux_types.h" 2
# 1 "/usr/include/unistd.h" 1 3 4
# 28 "/usr/include/unistd.h" 3 4

# 173 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/posix_opt.h" 1 3 4
# 174 "/usr/include/unistd.h" 2 3 4
# 191 "/usr/include/unistd.h" 3 4
typedef __ssize_t ssize_t;





# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 198 "/usr/include/unistd.h" 2 3 4





typedef __gid_t gid_t;




typedef __uid_t uid_t;







typedef __off64_t off_t;




typedef __off64_t off64_t;




typedef __useconds_t useconds_t;
# 245 "/usr/include/unistd.h" 3 4
typedef __socklen_t socklen_t;
# 258 "/usr/include/unistd.h" 3 4
extern int access (__const char *__name, int __type) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 304 "/usr/include/unistd.h" 3 4
extern __off64_t lseek (int __fd, __off64_t __offset, int __whence) __asm__ ("" "lseek64") __attribute__ ((__nothrow__));







extern __off64_t lseek64 (int __fd, __off64_t __offset, int __whence)
     __attribute__ ((__nothrow__));






extern int close (int __fd);






extern ssize_t read (int __fd, void *__buf, size_t __nbytes) ;





extern ssize_t write (int __fd, __const void *__buf, size_t __n) ;
# 384 "/usr/include/unistd.h" 3 4
extern int pipe (int __pipedes[2]) __attribute__ ((__nothrow__)) ;
# 393 "/usr/include/unistd.h" 3 4
extern unsigned int alarm (unsigned int __seconds) __attribute__ ((__nothrow__));
# 405 "/usr/include/unistd.h" 3 4
extern unsigned int sleep (unsigned int __seconds);






extern __useconds_t ualarm (__useconds_t __value, __useconds_t __interval)
     __attribute__ ((__nothrow__));






extern int usleep (__useconds_t __useconds);
# 429 "/usr/include/unistd.h" 3 4
extern int pause (void);



extern int chown (__const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchown (int __fd, __uid_t __owner, __gid_t __group) __attribute__ ((__nothrow__)) ;




extern int lchown (__const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;
# 457 "/usr/include/unistd.h" 3 4
extern int chdir (__const char *__path) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchdir (int __fd) __attribute__ ((__nothrow__)) ;
# 471 "/usr/include/unistd.h" 3 4
extern char *getcwd (char *__buf, size_t __size) __attribute__ ((__nothrow__)) ;
# 484 "/usr/include/unistd.h" 3 4
extern char *getwd (char *__buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__deprecated__)) ;




extern int dup (int __fd) __attribute__ ((__nothrow__)) ;


extern int dup2 (int __fd, int __fd2) __attribute__ ((__nothrow__));


extern char **__environ;







extern int execve (__const char *__path, char *__const __argv[],
     char *__const __envp[]) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 516 "/usr/include/unistd.h" 3 4
extern int execv (__const char *__path, char *__const __argv[])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));



extern int execle (__const char *__path, __const char *__arg, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));



extern int execl (__const char *__path, __const char *__arg, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));



extern int execvp (__const char *__file, char *__const __argv[])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));




extern int execlp (__const char *__file, __const char *__arg, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));




extern int nice (int __inc) __attribute__ ((__nothrow__)) ;




extern void _exit (int __status) __attribute__ ((__noreturn__));





# 1 "/usr/include/bits/confname.h" 1 3 4
# 26 "/usr/include/bits/confname.h" 3 4
enum
  {
    _PC_LINK_MAX,

    _PC_MAX_CANON,

    _PC_MAX_INPUT,

    _PC_NAME_MAX,

    _PC_PATH_MAX,

    _PC_PIPE_BUF,

    _PC_CHOWN_RESTRICTED,

    _PC_NO_TRUNC,

    _PC_VDISABLE,

    _PC_SYNC_IO,

    _PC_ASYNC_IO,

    _PC_PRIO_IO,

    _PC_SOCK_MAXBUF,

    _PC_FILESIZEBITS,

    _PC_REC_INCR_XFER_SIZE,

    _PC_REC_MAX_XFER_SIZE,

    _PC_REC_MIN_XFER_SIZE,

    _PC_REC_XFER_ALIGN,

    _PC_ALLOC_SIZE_MIN,

    _PC_SYMLINK_MAX,

    _PC_2_SYMLINKS

  };


enum
  {
    _SC_ARG_MAX,

    _SC_CHILD_MAX,

    _SC_CLK_TCK,

    _SC_NGROUPS_MAX,

    _SC_OPEN_MAX,

    _SC_STREAM_MAX,

    _SC_TZNAME_MAX,

    _SC_JOB_CONTROL,

    _SC_SAVED_IDS,

    _SC_REALTIME_SIGNALS,

    _SC_PRIORITY_SCHEDULING,

    _SC_TIMERS,

    _SC_ASYNCHRONOUS_IO,

    _SC_PRIORITIZED_IO,

    _SC_SYNCHRONIZED_IO,

    _SC_FSYNC,

    _SC_MAPPED_FILES,

    _SC_MEMLOCK,

    _SC_MEMLOCK_RANGE,

    _SC_MEMORY_PROTECTION,

    _SC_MESSAGE_PASSING,

    _SC_SEMAPHORES,

    _SC_SHARED_MEMORY_OBJECTS,

    _SC_AIO_LISTIO_MAX,

    _SC_AIO_MAX,

    _SC_AIO_PRIO_DELTA_MAX,

    _SC_DELAYTIMER_MAX,

    _SC_MQ_OPEN_MAX,

    _SC_MQ_PRIO_MAX,

    _SC_VERSION,

    _SC_PAGESIZE,


    _SC_RTSIG_MAX,

    _SC_SEM_NSEMS_MAX,

    _SC_SEM_VALUE_MAX,

    _SC_SIGQUEUE_MAX,

    _SC_TIMER_MAX,




    _SC_BC_BASE_MAX,

    _SC_BC_DIM_MAX,

    _SC_BC_SCALE_MAX,

    _SC_BC_STRING_MAX,

    _SC_COLL_WEIGHTS_MAX,

    _SC_EQUIV_CLASS_MAX,

    _SC_EXPR_NEST_MAX,

    _SC_LINE_MAX,

    _SC_RE_DUP_MAX,

    _SC_CHARCLASS_NAME_MAX,


    _SC_2_VERSION,

    _SC_2_C_BIND,

    _SC_2_C_DEV,

    _SC_2_FORT_DEV,

    _SC_2_FORT_RUN,

    _SC_2_SW_DEV,

    _SC_2_LOCALEDEF,


    _SC_PII,

    _SC_PII_XTI,

    _SC_PII_SOCKET,

    _SC_PII_INTERNET,

    _SC_PII_OSI,

    _SC_POLL,

    _SC_SELECT,

    _SC_UIO_MAXIOV,

    _SC_IOV_MAX = _SC_UIO_MAXIOV,

    _SC_PII_INTERNET_STREAM,

    _SC_PII_INTERNET_DGRAM,

    _SC_PII_OSI_COTS,

    _SC_PII_OSI_CLTS,

    _SC_PII_OSI_M,

    _SC_T_IOV_MAX,



    _SC_THREADS,

    _SC_THREAD_SAFE_FUNCTIONS,

    _SC_GETGR_R_SIZE_MAX,

    _SC_GETPW_R_SIZE_MAX,

    _SC_LOGIN_NAME_MAX,

    _SC_TTY_NAME_MAX,

    _SC_THREAD_DESTRUCTOR_ITERATIONS,

    _SC_THREAD_KEYS_MAX,

    _SC_THREAD_STACK_MIN,

    _SC_THREAD_THREADS_MAX,

    _SC_THREAD_ATTR_STACKADDR,

    _SC_THREAD_ATTR_STACKSIZE,

    _SC_THREAD_PRIORITY_SCHEDULING,

    _SC_THREAD_PRIO_INHERIT,

    _SC_THREAD_PRIO_PROTECT,

    _SC_THREAD_PROCESS_SHARED,


    _SC_NPROCESSORS_CONF,

    _SC_NPROCESSORS_ONLN,

    _SC_PHYS_PAGES,

    _SC_AVPHYS_PAGES,

    _SC_ATEXIT_MAX,

    _SC_PASS_MAX,


    _SC_XOPEN_VERSION,

    _SC_XOPEN_XCU_VERSION,

    _SC_XOPEN_UNIX,

    _SC_XOPEN_CRYPT,

    _SC_XOPEN_ENH_I18N,

    _SC_XOPEN_SHM,


    _SC_2_CHAR_TERM,

    _SC_2_C_VERSION,

    _SC_2_UPE,


    _SC_XOPEN_XPG2,

    _SC_XOPEN_XPG3,

    _SC_XOPEN_XPG4,


    _SC_CHAR_BIT,

    _SC_CHAR_MAX,

    _SC_CHAR_MIN,

    _SC_INT_MAX,

    _SC_INT_MIN,

    _SC_LONG_BIT,

    _SC_WORD_BIT,

    _SC_MB_LEN_MAX,

    _SC_NZERO,

    _SC_SSIZE_MAX,

    _SC_SCHAR_MAX,

    _SC_SCHAR_MIN,

    _SC_SHRT_MAX,

    _SC_SHRT_MIN,

    _SC_UCHAR_MAX,

    _SC_UINT_MAX,

    _SC_ULONG_MAX,

    _SC_USHRT_MAX,


    _SC_NL_ARGMAX,

    _SC_NL_LANGMAX,

    _SC_NL_MSGMAX,

    _SC_NL_NMAX,

    _SC_NL_SETMAX,

    _SC_NL_TEXTMAX,


    _SC_XBS5_ILP32_OFF32,

    _SC_XBS5_ILP32_OFFBIG,

    _SC_XBS5_LP64_OFF64,

    _SC_XBS5_LPBIG_OFFBIG,


    _SC_XOPEN_LEGACY,

    _SC_XOPEN_REALTIME,

    _SC_XOPEN_REALTIME_THREADS,


    _SC_ADVISORY_INFO,

    _SC_BARRIERS,

    _SC_BASE,

    _SC_C_LANG_SUPPORT,

    _SC_C_LANG_SUPPORT_R,

    _SC_CLOCK_SELECTION,

    _SC_CPUTIME,

    _SC_THREAD_CPUTIME,

    _SC_DEVICE_IO,

    _SC_DEVICE_SPECIFIC,

    _SC_DEVICE_SPECIFIC_R,

    _SC_FD_MGMT,

    _SC_FIFO,

    _SC_PIPE,

    _SC_FILE_ATTRIBUTES,

    _SC_FILE_LOCKING,

    _SC_FILE_SYSTEM,

    _SC_MONOTONIC_CLOCK,

    _SC_MULTI_PROCESS,

    _SC_SINGLE_PROCESS,

    _SC_NETWORKING,

    _SC_READER_WRITER_LOCKS,

    _SC_SPIN_LOCKS,

    _SC_REGEXP,

    _SC_REGEX_VERSION,

    _SC_SHELL,

    _SC_SIGNALS,

    _SC_SPAWN,

    _SC_SPORADIC_SERVER,

    _SC_THREAD_SPORADIC_SERVER,

    _SC_SYSTEM_DATABASE,

    _SC_SYSTEM_DATABASE_R,

    _SC_TIMEOUTS,

    _SC_TYPED_MEMORY_OBJECTS,

    _SC_USER_GROUPS,

    _SC_USER_GROUPS_R,

    _SC_2_PBS,

    _SC_2_PBS_ACCOUNTING,

    _SC_2_PBS_LOCATE,

    _SC_2_PBS_MESSAGE,

    _SC_2_PBS_TRACK,

    _SC_SYMLOOP_MAX,

    _SC_STREAMS,

    _SC_2_PBS_CHECKPOINT,


    _SC_V6_ILP32_OFF32,

    _SC_V6_ILP32_OFFBIG,

    _SC_V6_LP64_OFF64,

    _SC_V6_LPBIG_OFFBIG,


    _SC_HOST_NAME_MAX,

    _SC_TRACE,

    _SC_TRACE_EVENT_FILTER,

    _SC_TRACE_INHERIT,

    _SC_TRACE_LOG,


    _SC_LEVEL1_ICACHE_SIZE,

    _SC_LEVEL1_ICACHE_ASSOC,

    _SC_LEVEL1_ICACHE_LINESIZE,

    _SC_LEVEL1_DCACHE_SIZE,

    _SC_LEVEL1_DCACHE_ASSOC,

    _SC_LEVEL1_DCACHE_LINESIZE,

    _SC_LEVEL2_CACHE_SIZE,

    _SC_LEVEL2_CACHE_ASSOC,

    _SC_LEVEL2_CACHE_LINESIZE,

    _SC_LEVEL3_CACHE_SIZE,

    _SC_LEVEL3_CACHE_ASSOC,

    _SC_LEVEL3_CACHE_LINESIZE,

    _SC_LEVEL4_CACHE_SIZE,

    _SC_LEVEL4_CACHE_ASSOC,

    _SC_LEVEL4_CACHE_LINESIZE,



    _SC_IPV6 = _SC_LEVEL1_ICACHE_SIZE + 50,

    _SC_RAW_SOCKETS

  };


enum
  {
    _CS_PATH,


    _CS_V6_WIDTH_RESTRICTED_ENVS,



    _CS_GNU_LIBC_VERSION,

    _CS_GNU_LIBPTHREAD_VERSION,


    _CS_LFS_CFLAGS = 1000,

    _CS_LFS_LDFLAGS,

    _CS_LFS_LIBS,

    _CS_LFS_LINTFLAGS,

    _CS_LFS64_CFLAGS,

    _CS_LFS64_LDFLAGS,

    _CS_LFS64_LIBS,

    _CS_LFS64_LINTFLAGS,


    _CS_XBS5_ILP32_OFF32_CFLAGS = 1100,

    _CS_XBS5_ILP32_OFF32_LDFLAGS,

    _CS_XBS5_ILP32_OFF32_LIBS,

    _CS_XBS5_ILP32_OFF32_LINTFLAGS,

    _CS_XBS5_ILP32_OFFBIG_CFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LDFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LIBS,

    _CS_XBS5_ILP32_OFFBIG_LINTFLAGS,

    _CS_XBS5_LP64_OFF64_CFLAGS,

    _CS_XBS5_LP64_OFF64_LDFLAGS,

    _CS_XBS5_LP64_OFF64_LIBS,

    _CS_XBS5_LP64_OFF64_LINTFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_CFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LDFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LIBS,

    _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V6_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LIBS,

    _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V6_LP64_OFF64_CFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LIBS,

    _CS_POSIX_V6_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS

  };
# 555 "/usr/include/unistd.h" 2 3 4


extern long int pathconf (__const char *__path, int __name)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern long int fpathconf (int __fd, int __name) __attribute__ ((__nothrow__));


extern long int sysconf (int __name) __attribute__ ((__nothrow__));



extern size_t confstr (int __name, char *__buf, size_t __len) __attribute__ ((__nothrow__));




extern __pid_t getpid (void) __attribute__ ((__nothrow__));


extern __pid_t getppid (void) __attribute__ ((__nothrow__));




extern __pid_t getpgrp (void) __attribute__ ((__nothrow__));
# 591 "/usr/include/unistd.h" 3 4
extern __pid_t __getpgid (__pid_t __pid) __attribute__ ((__nothrow__));
# 600 "/usr/include/unistd.h" 3 4
extern int setpgid (__pid_t __pid, __pid_t __pgid) __attribute__ ((__nothrow__));
# 617 "/usr/include/unistd.h" 3 4
extern int setpgrp (void) __attribute__ ((__nothrow__));
# 634 "/usr/include/unistd.h" 3 4
extern __pid_t setsid (void) __attribute__ ((__nothrow__));







extern __uid_t getuid (void) __attribute__ ((__nothrow__));


extern __uid_t geteuid (void) __attribute__ ((__nothrow__));


extern __gid_t getgid (void) __attribute__ ((__nothrow__));


extern __gid_t getegid (void) __attribute__ ((__nothrow__));




extern int getgroups (int __size, __gid_t __list[]) __attribute__ ((__nothrow__)) ;
# 667 "/usr/include/unistd.h" 3 4
extern int setuid (__uid_t __uid) __attribute__ ((__nothrow__));




extern int setreuid (__uid_t __ruid, __uid_t __euid) __attribute__ ((__nothrow__));




extern int seteuid (__uid_t __uid) __attribute__ ((__nothrow__));






extern int setgid (__gid_t __gid) __attribute__ ((__nothrow__));




extern int setregid (__gid_t __rgid, __gid_t __egid) __attribute__ ((__nothrow__));




extern int setegid (__gid_t __gid) __attribute__ ((__nothrow__));
# 723 "/usr/include/unistd.h" 3 4
extern __pid_t fork (void) __attribute__ ((__nothrow__));






extern __pid_t vfork (void) __attribute__ ((__nothrow__));





extern char *ttyname (int __fd) __attribute__ ((__nothrow__));



extern int ttyname_r (int __fd, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2))) ;



extern int isatty (int __fd) __attribute__ ((__nothrow__));





extern int ttyslot (void) __attribute__ ((__nothrow__));




extern int link (__const char *__from, __const char *__to)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2))) ;
# 769 "/usr/include/unistd.h" 3 4
extern int symlink (__const char *__from, __const char *__to)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern ssize_t readlink (__const char *__restrict __path,
    char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2))) ;
# 792 "/usr/include/unistd.h" 3 4
extern int unlink (__const char *__name) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 801 "/usr/include/unistd.h" 3 4
extern int rmdir (__const char *__path) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));



extern __pid_t tcgetpgrp (int __fd) __attribute__ ((__nothrow__));


extern int tcsetpgrp (int __fd, __pid_t __pgrp_id) __attribute__ ((__nothrow__));






extern char *getlogin (void);







extern int getlogin_r (char *__name, size_t __name_len) __attribute__ ((__nonnull__ (1)));




extern int setlogin (__const char *__name) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 837 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/getopt.h" 1 3 4
# 59 "/usr/include/getopt.h" 3 4
extern char *optarg;
# 73 "/usr/include/getopt.h" 3 4
extern int optind;




extern int opterr;



extern int optopt;
# 152 "/usr/include/getopt.h" 3 4
extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       __attribute__ ((__nothrow__));
# 838 "/usr/include/unistd.h" 2 3 4







extern int gethostname (char *__name, size_t __len) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));






extern int sethostname (__const char *__name, size_t __len)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;



extern int sethostid (long int __id) __attribute__ ((__nothrow__)) ;





extern int getdomainname (char *__name, size_t __len)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;
extern int setdomainname (__const char *__name, size_t __len)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;





extern int vhangup (void) __attribute__ ((__nothrow__));


extern int revoke (__const char *__file) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;







extern int profil (unsigned short int *__sample_buffer, size_t __size,
     size_t __offset, unsigned int __scale)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));





extern int acct (__const char *__name) __attribute__ ((__nothrow__));



extern char *getusershell (void) __attribute__ ((__nothrow__));
extern void endusershell (void) __attribute__ ((__nothrow__));
extern void setusershell (void) __attribute__ ((__nothrow__));





extern int daemon (int __nochdir, int __noclose) __attribute__ ((__nothrow__)) ;






extern int chroot (__const char *__path) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;



extern char *getpass (__const char *__prompt) __attribute__ ((__nonnull__ (1)));
# 923 "/usr/include/unistd.h" 3 4
extern int fsync (int __fd);






extern long int gethostid (void);


extern void sync (void) __attribute__ ((__nothrow__));




extern int getpagesize (void) __attribute__ ((__nothrow__)) __attribute__ ((__const__));




extern int getdtablesize (void) __attribute__ ((__nothrow__));
# 952 "/usr/include/unistd.h" 3 4
extern int truncate (__const char *__file, __off64_t __length) __asm__ ("" "truncate64") __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;







extern int truncate64 (__const char *__file, __off64_t __length)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;
# 973 "/usr/include/unistd.h" 3 4
extern int ftruncate (int __fd, __off64_t __length) __asm__ ("" "ftruncate64") __attribute__ ((__nothrow__)) ;






extern int ftruncate64 (int __fd, __off64_t __length) __attribute__ ((__nothrow__)) ;
# 990 "/usr/include/unistd.h" 3 4
extern int brk (void *__addr) __attribute__ ((__nothrow__)) ;





extern void *sbrk (intptr_t __delta) __attribute__ ((__nothrow__));
# 1011 "/usr/include/unistd.h" 3 4
extern long int syscall (long int __sysno, ...) __attribute__ ((__nothrow__));
# 1037 "/usr/include/unistd.h" 3 4
extern int lockf (int __fd, int __cmd, __off64_t __len) __asm__ ("" "lockf64") ;






extern int lockf64 (int __fd, int __cmd, __off64_t __len) ;
# 1065 "/usr/include/unistd.h" 3 4
extern int fdatasync (int __fildes);
# 1103 "/usr/include/unistd.h" 3 4

# 216 "../linux/linux_types.h" 2
# 1 "../linux/sys/stat.h" 1




# 1 "/usr/include/sys/stat.h" 1
# 46 "/usr/include/sys/stat.h"
typedef __dev_t dev_t;
# 59 "/usr/include/sys/stat.h"
typedef __ino64_t ino_t;





typedef __mode_t mode_t;




typedef __nlink_t nlink_t;
# 105 "/usr/include/sys/stat.h"


# 1 "/usr/include/bits/stat.h" 1 3 4
# 43 "/usr/include/bits/stat.h" 3 4
struct stat
  {
    __dev_t st_dev;




    __ino_t st_ino;







    __nlink_t st_nlink;
    __mode_t st_mode;

    __uid_t st_uid;
    __gid_t st_gid;

    int pad0;

    __dev_t st_rdev;




    __off_t st_size;



    __blksize_t st_blksize;

    __blkcnt_t st_blocks;
# 88 "/usr/include/bits/stat.h" 3 4
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 103 "/usr/include/bits/stat.h" 3 4
    long int __unused[3];
# 112 "/usr/include/bits/stat.h" 3 4
  };



struct stat64
  {
    __dev_t st_dev;

    __ino64_t st_ino;
    __nlink_t st_nlink;
    __mode_t st_mode;






    __uid_t st_uid;
    __gid_t st_gid;

    int pad0;
    __dev_t st_rdev;
    __off_t st_size;





    __blksize_t st_blksize;
    __blkcnt64_t st_blocks;







    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 164 "/usr/include/bits/stat.h" 3 4
    long int __unused[3];



  };
# 108 "/usr/include/sys/stat.h" 2
# 217 "/usr/include/sys/stat.h"
extern int stat (__const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "stat64") __attribute__ ((__nothrow__))

     __attribute__ ((__nonnull__ (1, 2)));
extern int fstat (int __fd, struct stat *__buf) __asm__ ("" "fstat64") __attribute__ ((__nothrow__))
     __attribute__ ((__nonnull__ (2)));






extern int stat64 (__const char *__restrict __file,
     struct stat64 *__restrict __buf) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
extern int fstat64 (int __fd, struct stat64 *__buf) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));
# 265 "/usr/include/sys/stat.h"
extern int lstat (__const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "lstat64") __attribute__ ((__nothrow__))


     __attribute__ ((__nonnull__ (1, 2)));





extern int lstat64 (__const char *__restrict __file,
      struct stat64 *__restrict __buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));





extern int chmod (__const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));





extern int lchmod (__const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));




extern int fchmod (int __fd, __mode_t __mode) __attribute__ ((__nothrow__));
# 309 "/usr/include/sys/stat.h"
extern __mode_t umask (__mode_t __mask) __attribute__ ((__nothrow__));
# 318 "/usr/include/sys/stat.h"
extern int mkdir (__const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 333 "/usr/include/sys/stat.h"
extern int mknod (__const char *__path, __mode_t __mode, __dev_t __dev)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 347 "/usr/include/sys/stat.h"
extern int mkfifo (__const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 408 "/usr/include/sys/stat.h"
extern int __fxstat (int __ver, int __fildes, struct stat *__stat_buf) __asm__ ("" "__fxstat64") __attribute__ ((__nothrow__))

     __attribute__ ((__nonnull__ (3)));
extern int __xstat (int __ver, __const char *__filename, struct stat *__stat_buf) __asm__ ("" "__xstat64") __attribute__ ((__nothrow__))

     __attribute__ ((__nonnull__ (2, 3)));
extern int __lxstat (int __ver, __const char *__filename, struct stat *__stat_buf) __asm__ ("" "__lxstat64") __attribute__ ((__nothrow__))

     __attribute__ ((__nonnull__ (2, 3)));
extern int __fxstatat (int __ver, int __fildes, __const char *__filename, struct stat *__stat_buf, int __flag) __asm__ ("" "__fxstatat64") __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4)));
# 430 "/usr/include/sys/stat.h"
extern int __fxstat64 (int __ver, int __fildes, struct stat64 *__stat_buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3)));
extern int __xstat64 (int __ver, __const char *__filename,
        struct stat64 *__stat_buf) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2, 3)));
extern int __lxstat64 (int __ver, __const char *__filename,
         struct stat64 *__stat_buf) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2, 3)));
extern int __fxstatat64 (int __ver, int __fildes, __const char *__filename,
    struct stat64 *__stat_buf, int __flag)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4)));

extern int __xmknod (int __ver, __const char *__path, __mode_t __mode,
       __dev_t *__dev) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2, 4)));

extern int __xmknodat (int __ver, int __fd, __const char *__path,
         __mode_t __mode, __dev_t *__dev)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 5)));
# 532 "/usr/include/sys/stat.h"

# 6 "../linux/sys/stat.h" 2




typedef __blkcnt64_t blkcnt_t;

typedef __blksize_t blksize_t;






struct stat64_32 {
        dev32_t st_dev;
        int32_t st_pad1[3];
        ino_t st_ino;
        mode32_t st_mode;
        nlink32_t st_nlink;
        uid32_t st_uid;
        gid32_t st_gid;
        dev32_t st_rdev;
        int32_t st_pad2[2];
        off64_t st_size;
        timestruc32_t st_atim;
        timestruc32_t st_mtim;
        timestruc32_t st_ctim;
        int32_t st_blksize;
        blkcnt_t st_blocks;
        char st_fstype[16];
        int32_t st_pad4[8];
};





struct stat64;
# 217 "../linux/linux_types.h" 2
# 1 "/usr/include/sys/wait.h" 1 3 4
# 29 "/usr/include/sys/wait.h" 3 4


# 1 "/usr/include/signal.h" 1 3 4
# 31 "/usr/include/signal.h" 3 4


# 1 "/usr/include/bits/sigset.h" 1 3 4
# 104 "/usr/include/bits/sigset.h" 3 4
extern int __sigismember (__const __sigset_t *, int);
extern int __sigaddset (__sigset_t *, int);
extern int __sigdelset (__sigset_t *, int);
# 34 "/usr/include/signal.h" 2 3 4







typedef __sig_atomic_t sig_atomic_t;

# 58 "/usr/include/signal.h" 3 4
# 1 "/usr/include/bits/signum.h" 1 3 4
# 59 "/usr/include/signal.h" 2 3 4
# 75 "/usr/include/signal.h" 3 4
typedef void (*__sighandler_t) (int);




extern __sighandler_t __sysv_signal (int __sig, __sighandler_t __handler)
     __attribute__ ((__nothrow__));
# 90 "/usr/include/signal.h" 3 4


extern __sighandler_t signal (int __sig, __sighandler_t __handler)
     __attribute__ ((__nothrow__));
# 104 "/usr/include/signal.h" 3 4

# 117 "/usr/include/signal.h" 3 4
extern int kill (__pid_t __pid, int __sig) __attribute__ ((__nothrow__));






extern int killpg (__pid_t __pgrp, int __sig) __attribute__ ((__nothrow__));




extern int raise (int __sig) __attribute__ ((__nothrow__));




extern __sighandler_t ssignal (int __sig, __sighandler_t __handler)
     __attribute__ ((__nothrow__));
extern int gsignal (int __sig) __attribute__ ((__nothrow__));




extern void psignal (int __sig, __const char *__s);
# 153 "/usr/include/signal.h" 3 4
extern int __sigpause (int __sig_or_mask, int __is_sig);
# 181 "/usr/include/signal.h" 3 4
extern int sigblock (int __mask) __attribute__ ((__nothrow__)) __attribute__ ((__deprecated__));


extern int sigsetmask (int __mask) __attribute__ ((__nothrow__)) __attribute__ ((__deprecated__));


extern int siggetmask (void) __attribute__ ((__nothrow__)) __attribute__ ((__deprecated__));
# 201 "/usr/include/signal.h" 3 4
typedef __sighandler_t sig_t;
# 212 "/usr/include/signal.h" 3 4
# 1 "/usr/include/bits/siginfo.h" 1 3 4
# 25 "/usr/include/bits/siginfo.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 26 "/usr/include/bits/siginfo.h" 2 3 4







typedef union sigval
  {
    int sival_int;
    void *sival_ptr;
  } sigval_t;
# 51 "/usr/include/bits/siginfo.h" 3 4
typedef struct siginfo
  {
    int si_signo;
    int si_errno;

    int si_code;

    union
      {
 int _pad[((128 / sizeof (int)) - 4)];


 struct
   {
     __pid_t si_pid;
     __uid_t si_uid;
   } _kill;


 struct
   {
     int si_tid;
     int si_overrun;
     sigval_t si_sigval;
   } _timer;


 struct
   {
     __pid_t si_pid;
     __uid_t si_uid;
     sigval_t si_sigval;
   } _rt;


 struct
   {
     __pid_t si_pid;
     __uid_t si_uid;
     int si_status;
     __clock_t si_utime;
     __clock_t si_stime;
   } _sigchld;


 struct
   {
     void *si_addr;
   } _sigfault;


 struct
   {
     long int si_band;
     int si_fd;
   } _sigpoll;
      } _sifields;
  } siginfo_t;
# 129 "/usr/include/bits/siginfo.h" 3 4
enum
{
  SI_ASYNCNL = -60,

  SI_TKILL = -6,

  SI_SIGIO,

  SI_ASYNCIO,

  SI_MESGQ,

  SI_TIMER,

  SI_QUEUE,

  SI_USER,

  SI_KERNEL = 0x80

};



enum
{
  ILL_ILLOPC = 1,

  ILL_ILLOPN,

  ILL_ILLADR,

  ILL_ILLTRP,

  ILL_PRVOPC,

  ILL_PRVREG,

  ILL_COPROC,

  ILL_BADSTK

};


enum
{
  FPE_INTDIV = 1,

  FPE_INTOVF,

  FPE_FLTDIV,

  FPE_FLTOVF,

  FPE_FLTUND,

  FPE_FLTRES,

  FPE_FLTINV,

  FPE_FLTSUB

};


enum
{
  SEGV_MAPERR = 1,

  SEGV_ACCERR

};


enum
{
  BUS_ADRALN = 1,

  BUS_ADRERR,

  BUS_OBJERR

};


enum
{
  TRAP_BRKPT = 1,

  TRAP_TRACE

};


enum
{
  CLD_EXITED = 1,

  CLD_KILLED,

  CLD_DUMPED,

  CLD_TRAPPED,

  CLD_STOPPED,

  CLD_CONTINUED

};


enum
{
  POLL_IN = 1,

  POLL_OUT,

  POLL_MSG,

  POLL_ERR,

  POLL_PRI,

  POLL_HUP

};
# 273 "/usr/include/bits/siginfo.h" 3 4
typedef struct sigevent
  {
    sigval_t sigev_value;
    int sigev_signo;
    int sigev_notify;

    union
      {
 int _pad[((64 / sizeof (int)) - 4)];



 __pid_t _tid;

 struct
   {
     void (*_function) (sigval_t);
     void *_attribute;
   } _sigev_thread;
      } _sigev_un;
  } sigevent_t;






enum
{
  SIGEV_SIGNAL = 0,

  SIGEV_NONE,

  SIGEV_THREAD,


  SIGEV_THREAD_ID = 4

};
# 213 "/usr/include/signal.h" 2 3 4



extern int sigemptyset (sigset_t *__set) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern int sigfillset (sigset_t *__set) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern int sigaddset (sigset_t *__set, int __signo) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern int sigdelset (sigset_t *__set, int __signo) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern int sigismember (__const sigset_t *__set, int __signo)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 246 "/usr/include/signal.h" 3 4
# 1 "/usr/include/bits/sigaction.h" 1 3 4
# 25 "/usr/include/bits/sigaction.h" 3 4
struct sigaction
  {


    union
      {

 __sighandler_t sa_handler;

 void (*sa_sigaction) (int, siginfo_t *, void *);
      }
    __sigaction_handler;







    __sigset_t sa_mask;


    int sa_flags;


    void (*sa_restorer) (void);
  };
# 247 "/usr/include/signal.h" 2 3 4


extern int sigprocmask (int __how, __const sigset_t *__restrict __set,
   sigset_t *__restrict __oset) __attribute__ ((__nothrow__));






extern int sigsuspend (__const sigset_t *__set) __attribute__ ((__nonnull__ (1)));


extern int sigaction (int __sig, __const struct sigaction *__restrict __act,
        struct sigaction *__restrict __oact) __attribute__ ((__nothrow__));


extern int sigpending (sigset_t *__set) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));






extern int sigwait (__const sigset_t *__restrict __set, int *__restrict __sig)
     __attribute__ ((__nonnull__ (1, 2)));






extern int sigwaitinfo (__const sigset_t *__restrict __set,
   siginfo_t *__restrict __info) __attribute__ ((__nonnull__ (1)));






extern int sigtimedwait (__const sigset_t *__restrict __set,
    siginfo_t *__restrict __info,
    __const struct timespec *__restrict __timeout)
     __attribute__ ((__nonnull__ (1)));



extern int sigqueue (__pid_t __pid, int __sig, __const union sigval __val)
     __attribute__ ((__nothrow__));
# 304 "/usr/include/signal.h" 3 4
extern __const char *__const _sys_siglist[65];
extern __const char *__const sys_siglist[65];


struct sigvec
  {
    __sighandler_t sv_handler;
    int sv_mask;

    int sv_flags;

  };
# 328 "/usr/include/signal.h" 3 4
extern int sigvec (int __sig, __const struct sigvec *__vec,
     struct sigvec *__ovec) __attribute__ ((__nothrow__));



# 1 "/usr/include/bits/sigcontext.h" 1 3 4
# 26 "/usr/include/bits/sigcontext.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 27 "/usr/include/bits/sigcontext.h" 2 3 4

struct _fpreg
{
  unsigned short significand[4];
  unsigned short exponent;
};

struct _fpxreg
{
  unsigned short significand[4];
  unsigned short exponent;
  unsigned short padding[3];
};

struct _xmmreg
{
  __uint32_t element[4];
};
# 109 "/usr/include/bits/sigcontext.h" 3 4
struct _fpstate
{

  __uint16_t cwd;
  __uint16_t swd;
  __uint16_t ftw;
  __uint16_t fop;
  __uint64_t rip;
  __uint64_t rdp;
  __uint32_t mxcsr;
  __uint32_t mxcr_mask;
  struct _fpxreg _st[8];
  struct _xmmreg _xmm[16];
  __uint32_t padding[24];
};

struct sigcontext
{
  unsigned long r8;
  unsigned long r9;
  unsigned long r10;
  unsigned long r11;
  unsigned long r12;
  unsigned long r13;
  unsigned long r14;
  unsigned long r15;
  unsigned long rdi;
  unsigned long rsi;
  unsigned long rbp;
  unsigned long rbx;
  unsigned long rdx;
  unsigned long rax;
  unsigned long rcx;
  unsigned long rsp;
  unsigned long rip;
  unsigned long eflags;
  unsigned short cs;
  unsigned short gs;
  unsigned short fs;
  unsigned short __pad0;
  unsigned long err;
  unsigned long trapno;
  unsigned long oldmask;
  unsigned long cr2;
  struct _fpstate * fpstate;
  unsigned long __reserved1 [8];
};
# 334 "/usr/include/signal.h" 2 3 4


extern int sigreturn (struct sigcontext *__scp) __attribute__ ((__nothrow__));






# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 344 "/usr/include/signal.h" 2 3 4




extern int siginterrupt (int __sig, int __interrupt) __attribute__ ((__nothrow__));

# 1 "/usr/include/bits/sigstack.h" 1 3 4
# 26 "/usr/include/bits/sigstack.h" 3 4
struct sigstack
  {
    void *ss_sp;
    int ss_onstack;
  };



enum
{
  SS_ONSTACK = 1,

  SS_DISABLE

};
# 50 "/usr/include/bits/sigstack.h" 3 4
typedef struct sigaltstack
  {
    void *ss_sp;
    int ss_flags;
    size_t ss_size;
  } stack_t;
# 351 "/usr/include/signal.h" 2 3 4
# 359 "/usr/include/signal.h" 3 4
extern int sigstack (struct sigstack *__ss, struct sigstack *__oss)
     __attribute__ ((__nothrow__)) __attribute__ ((__deprecated__));



extern int sigaltstack (__const struct sigaltstack *__restrict __ss,
   struct sigaltstack *__restrict __oss) __attribute__ ((__nothrow__));
# 388 "/usr/include/signal.h" 3 4
# 1 "/usr/include/bits/pthreadtypes.h" 1 3 4
# 23 "/usr/include/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 24 "/usr/include/bits/pthreadtypes.h" 2 3 4
# 50 "/usr/include/bits/pthreadtypes.h" 3 4
typedef unsigned long int pthread_t;


typedef union
{
  char __size[56];
  long int __align;
} pthread_attr_t;



typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;
# 76 "/usr/include/bits/pthreadtypes.h" 3 4
typedef union
{
  struct __pthread_mutex_s
  {
    int __lock;
    unsigned int __count;
    int __owner;

    unsigned int __nusers;



    int __kind;

    int __spins;
    __pthread_list_t __list;
# 101 "/usr/include/bits/pthreadtypes.h" 3 4
  } __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;




typedef union
{
  struct
  {
    int __lock;
    unsigned int __futex;
    __extension__ unsigned long long int __total_seq;
    __extension__ unsigned long long int __wakeup_seq;
    __extension__ unsigned long long int __woken_seq;
    void *__mutex;
    unsigned int __nwaiters;
    unsigned int __broadcast_seq;
  } __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;



typedef unsigned int pthread_key_t;



typedef int pthread_once_t;





typedef union
{

  struct
  {
    int __lock;
    unsigned int __nr_readers;
    unsigned int __readers_wakeup;
    unsigned int __writer_wakeup;
    unsigned int __nr_readers_queued;
    unsigned int __nr_writers_queued;
    int __writer;
    int __shared;
    unsigned long int __pad1;
    unsigned long int __pad2;


    unsigned int __flags;
  } __data;
# 187 "/usr/include/bits/pthreadtypes.h" 3 4
  char __size[56];
  long int __align;
} pthread_rwlock_t;

typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;





typedef volatile int pthread_spinlock_t;




typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;
# 389 "/usr/include/signal.h" 2 3 4
# 1 "/usr/include/bits/sigthread.h" 1 3 4
# 31 "/usr/include/bits/sigthread.h" 3 4
extern int pthread_sigmask (int __how,
       __const __sigset_t *__restrict __newmask,
       __sigset_t *__restrict __oldmask)__attribute__ ((__nothrow__));


extern int pthread_kill (pthread_t __threadid, int __signo) __attribute__ ((__nothrow__));
# 390 "/usr/include/signal.h" 2 3 4






extern int __libc_current_sigrtmin (void) __attribute__ ((__nothrow__));

extern int __libc_current_sigrtmax (void) __attribute__ ((__nothrow__));




# 32 "/usr/include/sys/wait.h" 2 3 4
# 1 "/usr/include/sys/resource.h" 1 3 4
# 25 "/usr/include/sys/resource.h" 3 4
# 1 "/usr/include/bits/resource.h" 1 3 4
# 33 "/usr/include/bits/resource.h" 3 4
enum __rlimit_resource
{

  RLIMIT_CPU = 0,



  RLIMIT_FSIZE = 1,



  RLIMIT_DATA = 2,



  RLIMIT_STACK = 3,



  RLIMIT_CORE = 4,






  __RLIMIT_RSS = 5,



  RLIMIT_NOFILE = 7,
  __RLIMIT_OFILE = RLIMIT_NOFILE,




  RLIMIT_AS = 9,



  __RLIMIT_NPROC = 6,



  __RLIMIT_MEMLOCK = 8,



  __RLIMIT_LOCKS = 10,



  __RLIMIT_SIGPENDING = 11,



  __RLIMIT_MSGQUEUE = 12,





  __RLIMIT_NICE = 13,




  __RLIMIT_RTPRIO = 14,


  __RLIMIT_NLIMITS = 15,
  __RLIM_NLIMITS = __RLIMIT_NLIMITS


};
# 129 "/usr/include/bits/resource.h" 3 4
typedef __rlim64_t rlim_t;


typedef __rlim64_t rlim64_t;


struct rlimit
  {

    rlim_t rlim_cur;

    rlim_t rlim_max;
  };


struct rlimit64
  {

    rlim64_t rlim_cur;

    rlim64_t rlim_max;
 };



enum __rusage_who
{

  RUSAGE_SELF = 0,



  RUSAGE_CHILDREN = -1

};


# 1 "/usr/include/bits/time.h" 1 3 4
# 167 "/usr/include/bits/resource.h" 2 3 4


struct rusage
  {

    struct timeval ru_utime;

    struct timeval ru_stime;

    long int ru_maxrss;


    long int ru_ixrss;

    long int ru_idrss;

    long int ru_isrss;


    long int ru_minflt;

    long int ru_majflt;

    long int ru_nswap;


    long int ru_inblock;

    long int ru_oublock;

    long int ru_msgsnd;

    long int ru_msgrcv;

    long int ru_nsignals;



    long int ru_nvcsw;


    long int ru_nivcsw;
  };







enum __priority_which
{
  PRIO_PROCESS = 0,

  PRIO_PGRP = 1,

  PRIO_USER = 2

};
# 26 "/usr/include/sys/resource.h" 2 3 4


typedef __id_t id_t;




# 43 "/usr/include/sys/resource.h" 3 4
typedef int __rlimit_resource_t;
typedef int __rusage_who_t;
typedef int __priority_which_t;
# 55 "/usr/include/sys/resource.h" 3 4
extern int getrlimit (__rlimit_resource_t __resource, struct rlimit *__rlimits) __asm__ ("" "getrlimit64") __attribute__ ((__nothrow__));






extern int getrlimit64 (__rlimit_resource_t __resource,
   struct rlimit64 *__rlimits) __attribute__ ((__nothrow__));
# 74 "/usr/include/sys/resource.h" 3 4
extern int setrlimit (__rlimit_resource_t __resource, __const struct rlimit *__rlimits) __asm__ ("" "setrlimit64") __attribute__ ((__nothrow__));







extern int setrlimit64 (__rlimit_resource_t __resource,
   __const struct rlimit64 *__rlimits) __attribute__ ((__nothrow__));




extern int getrusage (__rusage_who_t __who, struct rusage *__usage) __attribute__ ((__nothrow__));





extern int getpriority (__priority_which_t __which, id_t __who) __attribute__ ((__nothrow__));



extern int setpriority (__priority_which_t __which, id_t __who, int __prio)
     __attribute__ ((__nothrow__));


# 33 "/usr/include/sys/wait.h" 2 3 4





# 1 "/usr/include/bits/waitflags.h" 1 3 4
# 39 "/usr/include/sys/wait.h" 2 3 4
# 63 "/usr/include/sys/wait.h" 3 4
typedef union
  {
    union wait *__uptr;
    int *__iptr;
  } __WAIT_STATUS __attribute__ ((__transparent_union__));
# 80 "/usr/include/sys/wait.h" 3 4
# 1 "/usr/include/bits/waitstatus.h" 1 3 4
# 65 "/usr/include/bits/waitstatus.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 37 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/endian.h" 1 3 4
# 38 "/usr/include/endian.h" 2 3 4
# 66 "/usr/include/bits/waitstatus.h" 2 3 4

union wait
  {
    int w_status;
    struct
      {

 unsigned int __w_termsig:7;
 unsigned int __w_coredump:1;
 unsigned int __w_retcode:8;
 unsigned int:16;







      } __wait_terminated;
    struct
      {

 unsigned int __w_stopval:8;
 unsigned int __w_stopsig:8;
 unsigned int:16;






      } __wait_stopped;
  };
# 81 "/usr/include/sys/wait.h" 2 3 4
# 102 "/usr/include/sys/wait.h" 3 4
typedef enum
{
  P_ALL,
  P_PID,
  P_PGID
} idtype_t;
# 116 "/usr/include/sys/wait.h" 3 4
extern __pid_t wait (__WAIT_STATUS __stat_loc);
# 139 "/usr/include/sys/wait.h" 3 4
extern __pid_t waitpid (__pid_t __pid, int *__stat_loc, int __options);



# 1 "/usr/include/bits/siginfo.h" 1 3 4
# 25 "/usr/include/bits/siginfo.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 26 "/usr/include/bits/siginfo.h" 2 3 4
# 144 "/usr/include/sys/wait.h" 2 3 4
# 155 "/usr/include/sys/wait.h" 3 4
extern int waitid (idtype_t __idtype, __id_t __id, siginfo_t *__infop,
     int __options);





struct rusage;






extern __pid_t wait3 (__WAIT_STATUS __stat_loc, int __options,
        struct rusage * __usage) __attribute__ ((__nothrow__));




extern __pid_t wait4 (__pid_t __pid, __WAIT_STATUS __stat_loc, int __options,
        struct rusage *__usage) __attribute__ ((__nothrow__));




# 218 "../linux/linux_types.h" 2
# 1 "../linux/zone.h" 1
# 219 "../linux/linux_types.h" 2
# 228 "../linux/linux_types.h"
typedef union {
        long double _q;
        uint32_t _l[4];
} upad128_t;
# 327 "../linux/linux_types.h"
typedef struct timespec timestruc_t;





typedef uint_t major_t;
typedef uint_t minor_t;


typedef struct flock64_32 {
        int16_t l_type;
        int16_t l_whence;
        off64_t l_start;
        off64_t l_len;
        int32_t l_sysid;
        pid32_t l_pid;
        int32_t l_pad[4];
} flock64_32_t;




typedef struct fshare {
        short f_access;
        short f_deny;
        int f_id;
} fshare_t;


struct itimerval32 {
        struct timeval32 it_interval;
        struct timeval32 it_value;
};
# 376 "../linux/linux_types.h"
struct memcntl_mha {
        uint_t mha_cmd;
        uint_t mha_flags;
        size_t mha_pagesize;
};

struct memcntl_mha32 {
        uint_t mha_cmd;
        uint_t mha_flags;
        size32_t mha_pagesize;
};

typedef struct meminfo {
        const uint64_t *mi_inaddr;
        const uint_t *mi_info_req;
        uint64_t *mi_outdata;
        uint_t *mi_validity;
        int mi_info_count;
} meminfo_t;
typedef struct meminfo32 {
        caddr32_t mi_inaddr;
        caddr32_t mi_info_req;
        caddr32_t mi_outdata;
        caddr32_t mi_validity;
        int32_t mi_info_count;
} meminfo32_t;
# 2 "../linux/stdio.h" 2
# 1 "/usr/include/stdio.h" 1
# 30 "/usr/include/stdio.h"




# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 35 "/usr/include/stdio.h" 2
# 45 "/usr/include/stdio.h"
struct _IO_FILE;



typedef struct _IO_FILE FILE;





# 65 "/usr/include/stdio.h"
typedef struct _IO_FILE __FILE;
# 75 "/usr/include/stdio.h"
# 1 "/usr/include/libio.h" 1 3 4
# 32 "/usr/include/libio.h" 3 4
# 1 "/usr/include/_G_config.h" 1 3 4
# 15 "/usr/include/_G_config.h" 3 4
# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 16 "/usr/include/_G_config.h" 2 3 4




# 1 "/usr/include/wchar.h" 1 3 4
# 78 "/usr/include/wchar.h" 3 4
typedef struct
{
  int __count;
  union
  {

    unsigned int __wch;



    char __wchb[4];
  } __value;
} __mbstate_t;
# 21 "/usr/include/_G_config.h" 2 3 4

typedef struct
{
  __off_t __pos;
  __mbstate_t __state;
} _G_fpos_t;
typedef struct
{
  __off64_t __pos;
  __mbstate_t __state;
} _G_fpos64_t;
# 53 "/usr/include/_G_config.h" 3 4
typedef int _G_int16_t __attribute__ ((__mode__ (__HI__)));
typedef int _G_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int _G_uint16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int _G_uint32_t __attribute__ ((__mode__ (__SI__)));
# 33 "/usr/include/libio.h" 2 3 4
# 53 "/usr/include/libio.h" 3 4
# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stdarg.h" 1 3 4
# 43 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 54 "/usr/include/libio.h" 2 3 4
# 170 "/usr/include/libio.h" 3 4
struct _IO_jump_t; struct _IO_FILE;
# 180 "/usr/include/libio.h" 3 4
typedef void _IO_lock_t;





struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;



  int _pos;
# 203 "/usr/include/libio.h" 3 4
};


enum __codecvt_result
{
  __codecvt_ok,
  __codecvt_partial,
  __codecvt_error,
  __codecvt_noconv
};
# 271 "/usr/include/libio.h" 3 4
struct _IO_FILE {
  int _flags;




  char* _IO_read_ptr;
  char* _IO_read_end;
  char* _IO_read_base;
  char* _IO_write_base;
  char* _IO_write_ptr;
  char* _IO_write_end;
  char* _IO_buf_base;
  char* _IO_buf_end;

  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;



  int _flags2;

  __off_t _old_offset;



  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];



  _IO_lock_t *_lock;
# 319 "/usr/include/libio.h" 3 4
  __off64_t _offset;
# 328 "/usr/include/libio.h" 3 4
  void *__pad1;
  void *__pad2;
  void *__pad3;
  void *__pad4;
  size_t __pad5;

  int _mode;

  char _unused2[15 * sizeof (int) - 4 * sizeof (void *) - sizeof (size_t)];

};


typedef struct _IO_FILE _IO_FILE;


struct _IO_FILE_plus;

extern struct _IO_FILE_plus _IO_2_1_stdin_;
extern struct _IO_FILE_plus _IO_2_1_stdout_;
extern struct _IO_FILE_plus _IO_2_1_stderr_;
# 364 "/usr/include/libio.h" 3 4
typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);







typedef __ssize_t __io_write_fn (void *__cookie, __const char *__buf,
     size_t __n);







typedef int __io_seek_fn (void *__cookie, __off64_t *__pos, int __w);


typedef int __io_close_fn (void *__cookie);
# 416 "/usr/include/libio.h" 3 4
extern int __underflow (_IO_FILE *);
extern int __uflow (_IO_FILE *);
extern int __overflow (_IO_FILE *, int);
# 458 "/usr/include/libio.h" 3 4
extern int _IO_getc (_IO_FILE *__fp);
extern int _IO_putc (int __c, _IO_FILE *__fp);
extern int _IO_feof (_IO_FILE *__fp) __attribute__ ((__nothrow__));
extern int _IO_ferror (_IO_FILE *__fp) __attribute__ ((__nothrow__));

extern int _IO_peekc_locked (_IO_FILE *__fp);





extern void _IO_flockfile (_IO_FILE *) __attribute__ ((__nothrow__));
extern void _IO_funlockfile (_IO_FILE *) __attribute__ ((__nothrow__));
extern int _IO_ftrylockfile (_IO_FILE *) __attribute__ ((__nothrow__));
# 488 "/usr/include/libio.h" 3 4
extern int _IO_vfscanf (_IO_FILE * __restrict, const char * __restrict,
   __gnuc_va_list, int *__restrict);
extern int _IO_vfprintf (_IO_FILE *__restrict, const char *__restrict,
    __gnuc_va_list);
extern __ssize_t _IO_padn (_IO_FILE *, int, __ssize_t);
extern size_t _IO_sgetn (_IO_FILE *, void *, size_t);

extern __off64_t _IO_seekoff (_IO_FILE *, __off64_t, int, int);
extern __off64_t _IO_seekpos (_IO_FILE *, __off64_t, int);

extern void _IO_free_backup_area (_IO_FILE *) __attribute__ ((__nothrow__));
# 76 "/usr/include/stdio.h" 2
# 89 "/usr/include/stdio.h"




typedef _G_fpos64_t fpos_t;



typedef _G_fpos64_t fpos64_t;
# 141 "/usr/include/stdio.h"
# 1 "/usr/include/bits/stdio_lim.h" 1 3 4
# 142 "/usr/include/stdio.h" 2



extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;









extern int remove (__const char *__filename) __attribute__ ((__nothrow__));

extern int rename (__const char *__old, __const char *__new) __attribute__ ((__nothrow__));









# 177 "/usr/include/stdio.h"
extern FILE *tmpfile (void) __asm__ ("" "tmpfile64") ;






extern FILE *tmpfile64 (void) ;



extern char *tmpnam (char *__s) __attribute__ ((__nothrow__)) ;





extern char *tmpnam_r (char *__s) __attribute__ ((__nothrow__)) ;
# 206 "/usr/include/stdio.h"
extern char *tempnam (__const char *__dir, __const char *__pfx)
     __attribute__ ((__nothrow__)) __attribute__ ((__malloc__)) ;








extern int fclose (FILE *__stream);




extern int fflush (FILE *__stream);

# 231 "/usr/include/stdio.h"
extern int fflush_unlocked (FILE *__stream);
# 245 "/usr/include/stdio.h"

# 262 "/usr/include/stdio.h"
extern FILE *fopen (__const char *__restrict __filename, __const char *__restrict __modes) __asm__ ("" "fopen64")

  ;
extern FILE *freopen (__const char *__restrict __filename, __const char *__restrict __modes, FILE *__restrict __stream) __asm__ ("" "freopen64")


  ;







extern FILE *fopen64 (__const char *__restrict __filename,
        __const char *__restrict __modes) ;
extern FILE *freopen64 (__const char *__restrict __filename,
   __const char *__restrict __modes,
   FILE *__restrict __stream) ;




extern FILE *fdopen (int __fd, __const char *__modes) __attribute__ ((__nothrow__)) ;
# 306 "/usr/include/stdio.h"



extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__));





extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__));








extern int fprintf (FILE *__restrict __stream,
      __const char *__restrict __format, ...);




extern int printf (__const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      __const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg);




extern int vprintf (__const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));





extern int snprintf (char *__restrict __s, size_t __maxlen,
       __const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));

# 400 "/usr/include/stdio.h"





extern int fscanf (FILE *__restrict __stream,
     __const char *__restrict __format, ...) ;




extern int scanf (__const char *__restrict __format, ...) ;

extern int sscanf (__const char *__restrict __s,
     __const char *__restrict __format, ...) __attribute__ ((__nothrow__));
# 443 "/usr/include/stdio.h"

# 506 "/usr/include/stdio.h"





extern int fgetc (FILE *__stream);
extern int getc (FILE *__stream);





extern int getchar (void);

# 530 "/usr/include/stdio.h"
extern int getc_unlocked (FILE *__stream);
extern int getchar_unlocked (void);
# 541 "/usr/include/stdio.h"
extern int fgetc_unlocked (FILE *__stream);











extern int fputc (int __c, FILE *__stream);
extern int putc (int __c, FILE *__stream);





extern int putchar (int __c);

# 574 "/usr/include/stdio.h"
extern int fputc_unlocked (int __c, FILE *__stream);







extern int putc_unlocked (int __c, FILE *__stream);
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream);


extern int putw (int __w, FILE *__stream);








extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     ;






extern char *gets (char *__s) ;

# 655 "/usr/include/stdio.h"





extern int fputs (__const char *__restrict __s, FILE *__restrict __stream);





extern int puts (__const char *__s);






extern int ungetc (int __c, FILE *__stream);






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream) ;




extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s) ;

# 708 "/usr/include/stdio.h"
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream) ;
extern size_t fwrite_unlocked (__const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream) ;








extern int fseek (FILE *__stream, long int __off, int __whence);




extern long int ftell (FILE *__stream) ;




extern void rewind (FILE *__stream);

# 752 "/usr/include/stdio.h"
extern int fseeko (FILE *__stream, __off64_t __off, int __whence) __asm__ ("" "fseeko64");


extern __off64_t ftello (FILE *__stream) __asm__ ("" "ftello64");








# 777 "/usr/include/stdio.h"
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos) __asm__ ("" "fgetpos64");

extern int fsetpos (FILE *__stream, __const fpos_t *__pos) __asm__ ("" "fsetpos64");









extern int fseeko64 (FILE *__stream, __off64_t __off, int __whence);
extern __off64_t ftello64 (FILE *__stream) ;
extern int fgetpos64 (FILE *__restrict __stream, fpos64_t *__restrict __pos);
extern int fsetpos64 (FILE *__stream, __const fpos64_t *__pos);




extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__)) ;

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__)) ;




extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__)) ;
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__)) ;








extern void perror (__const char *__s);






# 1 "/usr/include/bits/sys_errlist.h" 1 3 4
# 27 "/usr/include/bits/sys_errlist.h" 3 4
extern int sys_nerr;
extern __const char *__const sys_errlist[];
# 825 "/usr/include/stdio.h" 2




extern int fileno (FILE *__stream) __attribute__ ((__nothrow__)) ;




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__)) ;
# 844 "/usr/include/stdio.h"
extern FILE *popen (__const char *__command, __const char *__modes) ;





extern int pclose (FILE *__stream);





extern char *ctermid (char *__s) __attribute__ ((__nothrow__));
# 884 "/usr/include/stdio.h"
extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__)) ;


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__));
# 914 "/usr/include/stdio.h"

# 2 "../linux/stdio.h" 2
# 13 "proc_names.c" 2

# 1 "/usr/include/string.h" 1 3 4
# 28 "/usr/include/string.h" 3 4





# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 34 "/usr/include/string.h" 2 3 4




extern void *memcpy (void *__restrict __dest,
       __const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));


extern void *memmove (void *__dest, __const void *__src, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));






extern void *memccpy (void *__restrict __dest, __const void *__restrict __src,
        int __c, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));





extern void *memset (void *__s, int __c, size_t __n) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern int memcmp (__const void *__s1, __const void *__s2, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern void *memchr (__const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));

# 82 "/usr/include/string.h" 3 4


extern char *strcpy (char *__restrict __dest, __const char *__restrict __src)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncpy (char *__restrict __dest,
        __const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));


extern char *strcat (char *__restrict __dest, __const char *__restrict __src)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncat (char *__restrict __dest, __const char *__restrict __src,
        size_t __n) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcmp (__const char *__s1, __const char *__s2)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern int strncmp (__const char *__s1, __const char *__s2, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcoll (__const char *__s1, __const char *__s2)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern size_t strxfrm (char *__restrict __dest,
         __const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));

# 130 "/usr/include/string.h" 3 4
extern char *strdup (__const char *__s)
     __attribute__ ((__nothrow__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));
# 165 "/usr/include/string.h" 3 4


extern char *strchr (__const char *__s, int __c)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));

extern char *strrchr (__const char *__s, int __c)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));

# 181 "/usr/include/string.h" 3 4



extern size_t strcspn (__const char *__s, __const char *__reject)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern size_t strspn (__const char *__s, __const char *__accept)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strpbrk (__const char *__s, __const char *__accept)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strstr (__const char *__haystack, __const char *__needle)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));



extern char *strtok (char *__restrict __s, __const char *__restrict __delim)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));




extern char *__strtok_r (char *__restrict __s,
    __const char *__restrict __delim,
    char **__restrict __save_ptr)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2, 3)));

extern char *strtok_r (char *__restrict __s, __const char *__restrict __delim,
         char **__restrict __save_ptr)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2, 3)));
# 240 "/usr/include/string.h" 3 4


extern size_t strlen (__const char *__s)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));

# 254 "/usr/include/string.h" 3 4


extern char *strerror (int __errnum) __attribute__ ((__nothrow__));

# 270 "/usr/include/string.h" 3 4
extern int strerror_r (int __errnum, char *__buf, size_t __buflen) __asm__ ("" "__xpg_strerror_r") __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));
# 294 "/usr/include/string.h" 3 4
extern void __bzero (void *__s, size_t __n) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));



extern void bcopy (__const void *__src, void *__dest, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bzero (void *__s, size_t __n) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern int bcmp (__const void *__s1, __const void *__s2, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern char *index (__const char *__s, int __c)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));


extern char *rindex (__const char *__s, int __c)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));



extern int ffs (int __i) __attribute__ ((__nothrow__)) __attribute__ ((__const__));
# 331 "/usr/include/string.h" 3 4
extern int strcasecmp (__const char *__s1, __const char *__s2)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strncasecmp (__const char *__s1, __const char *__s2, size_t __n)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 354 "/usr/include/string.h" 3 4
extern char *strsep (char **__restrict __stringp,
       __const char *__restrict __delim)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
# 432 "/usr/include/string.h" 3 4

# 15 "proc_names.c" 2


# 1 "/usr/include/alloca.h" 1 3 4
# 25 "/usr/include/alloca.h" 3 4
# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 26 "/usr/include/alloca.h" 2 3 4







extern void *alloca (size_t __size) __attribute__ ((__nothrow__));






# 18 "proc_names.c" 2
# 1 "/usr/include/errno.h" 1 3 4
# 32 "/usr/include/errno.h" 3 4




# 1 "/usr/include/bits/errno.h" 1 3 4
# 25 "/usr/include/bits/errno.h" 3 4
# 1 "/usr/include/linux/errno.h" 1 3 4



# 1 "/usr/include/asm/errno.h" 1 3 4
# 1 "/usr/include/asm-generic/errno.h" 1 3 4



# 1 "/usr/include/asm-generic/errno-base.h" 1 3 4
# 5 "/usr/include/asm-generic/errno.h" 2 3 4
# 1 "/usr/include/asm/errno.h" 2 3 4
# 5 "/usr/include/linux/errno.h" 2 3 4
# 26 "/usr/include/bits/errno.h" 2 3 4
# 43 "/usr/include/bits/errno.h" 3 4
extern int *__errno_location (void) __attribute__ ((__nothrow__)) __attribute__ ((__const__));
# 37 "/usr/include/errno.h" 2 3 4
# 59 "/usr/include/errno.h" 3 4

# 19 "proc_names.c" 2
# 1 "../libproc/common/libproc.h" 1
# 28 "../libproc/common/libproc.h"
#pragma ident "@(#)libproc.h	1.45	04/11/01 SMI"


# 1 "/usr/include/stdlib.h" 1 3 4
# 33 "/usr/include/stdlib.h" 3 4
# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 326 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 3 4
typedef int wchar_t;
# 34 "/usr/include/stdlib.h" 2 3 4


# 96 "/usr/include/stdlib.h" 3 4


typedef struct
  {
    int quot;
    int rem;
  } div_t;



typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;



# 140 "/usr/include/stdlib.h" 3 4
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__)) ;




extern double atof (__const char *__nptr)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern int atoi (__const char *__nptr)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern long int atol (__const char *__nptr)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





__extension__ extern long long int atoll (__const char *__nptr)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





extern double strtod (__const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;

# 182 "/usr/include/stdlib.h" 3 4


extern long int strtol (__const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;

extern unsigned long int strtoul (__const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;




__extension__
extern long long int strtoq (__const char *__restrict __nptr,
        char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;

__extension__
extern unsigned long long int strtouq (__const char *__restrict __nptr,
           char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;





__extension__
extern long long int strtoll (__const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;

__extension__
extern unsigned long long int strtoull (__const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;

# 311 "/usr/include/stdlib.h" 3 4
extern char *l64a (long int __n) __attribute__ ((__nothrow__)) ;


extern long int a64l (__const char *__s)
     __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;
# 327 "/usr/include/stdlib.h" 3 4
extern long int random (void) __attribute__ ((__nothrow__));


extern void srandom (unsigned int __seed) __attribute__ ((__nothrow__));





extern char *initstate (unsigned int __seed, char *__statebuf,
   size_t __statelen) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));



extern char *setstate (char *__statebuf) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));







struct random_data
  {
    int32_t *fptr;
    int32_t *rptr;
    int32_t *state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t *end_ptr;
  };

extern int random_r (struct random_data *__restrict __buf,
       int32_t *__restrict __result) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));

extern int srandom_r (unsigned int __seed, struct random_data *__buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));

extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
   size_t __statelen,
   struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2, 4)));

extern int setstate_r (char *__restrict __statebuf,
         struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));






extern int rand (void) __attribute__ ((__nothrow__));

extern void srand (unsigned int __seed) __attribute__ ((__nothrow__));




extern int rand_r (unsigned int *__seed) __attribute__ ((__nothrow__));







extern double drand48 (void) __attribute__ ((__nothrow__));
extern double erand48 (unsigned short int __xsubi[3]) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern long int lrand48 (void) __attribute__ ((__nothrow__));
extern long int nrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern long int mrand48 (void) __attribute__ ((__nothrow__));
extern long int jrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));


extern void srand48 (long int __seedval) __attribute__ ((__nothrow__));
extern unsigned short int *seed48 (unsigned short int __seed16v[3])
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern void lcong48 (unsigned short int __param[7]) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));





struct drand48_data
  {
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    unsigned long long int __a;
  };


extern int drand48_r (struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
extern int erand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));


extern int lrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
extern int nrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));


extern int mrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
extern int jrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));


extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));

extern int seed48_r (unsigned short int __seed16v[3],
       struct drand48_data *__buffer) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));

extern int lcong48_r (unsigned short int __param[7],
        struct drand48_data *__buffer)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));









extern void *malloc (size_t __size) __attribute__ ((__nothrow__)) __attribute__ ((__malloc__)) ;

extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__)) __attribute__ ((__malloc__)) ;










extern void *realloc (void *__ptr, size_t __size)
     __attribute__ ((__nothrow__)) __attribute__ ((__warn_unused_result__));

extern void free (void *__ptr) __attribute__ ((__nothrow__));




extern void cfree (void *__ptr) __attribute__ ((__nothrow__));
# 502 "/usr/include/stdlib.h" 3 4
extern void *valloc (size_t __size) __attribute__ ((__nothrow__)) __attribute__ ((__malloc__)) ;




extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;




extern void abort (void) __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));



extern int atexit (void (*__func) (void)) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));





extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));






extern void exit (int __status) __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));

# 543 "/usr/include/stdlib.h" 3 4


extern char *getenv (__const char *__name) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;




extern char *__secure_getenv (__const char *__name)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;





extern int putenv (char *__string) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));





extern int setenv (__const char *__name, __const char *__value, int __replace)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));


extern int unsetenv (__const char *__name) __attribute__ ((__nothrow__));






extern int clearenv (void) __attribute__ ((__nothrow__));
# 583 "/usr/include/stdlib.h" 3 4
extern char *mktemp (char *__template) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;
# 597 "/usr/include/stdlib.h" 3 4
extern int mkstemp (char *__template) __asm__ ("" "mkstemp64")
     __attribute__ ((__nonnull__ (1))) ;





extern int mkstemp64 (char *__template) __attribute__ ((__nonnull__ (1))) ;
# 614 "/usr/include/stdlib.h" 3 4
extern char *mkdtemp (char *__template) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;
# 640 "/usr/include/stdlib.h" 3 4





extern int system (__const char *__command) ;

# 662 "/usr/include/stdlib.h" 3 4
extern char *realpath (__const char *__restrict __name,
         char *__restrict __resolved) __attribute__ ((__nothrow__)) ;






typedef int (*__compar_fn_t) (__const void *, __const void *);









extern void *bsearch (__const void *__key, __const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;



extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));



extern int abs (int __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)) ;
extern long int labs (long int __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)) ;












extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__)) __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__)) __attribute__ ((__const__)) ;

# 727 "/usr/include/stdlib.h" 3 4
extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *gcvt (double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3))) ;




extern char *qecvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qfcvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3))) ;




extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4, 5)));

extern int qecvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qfcvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (3, 4, 5)));







extern int mblen (__const char *__s, size_t __n) __attribute__ ((__nothrow__)) ;


extern int mbtowc (wchar_t *__restrict __pwc,
     __const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__)) ;


extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__)) ;



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   __const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__));

extern size_t wcstombs (char *__restrict __s,
   __const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__));








extern int rpmatch (__const char *__response) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1))) ;
# 832 "/usr/include/stdlib.h" 3 4
extern int posix_openpt (int __oflag) ;
# 867 "/usr/include/stdlib.h" 3 4
extern int getloadavg (double __loadavg[], int __nelem)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
# 883 "/usr/include/stdlib.h" 3 4

# 32 "../libproc/common/libproc.h" 2

# 1 "/usr/include/fcntl.h" 1 3 4
# 30 "/usr/include/fcntl.h" 3 4




# 1 "/usr/include/bits/fcntl.h" 1 3 4
# 25 "/usr/include/bits/fcntl.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 26 "/usr/include/bits/fcntl.h" 2 3 4
# 158 "/usr/include/bits/fcntl.h" 3 4
struct flock
  {
    short int l_type;
    short int l_whence;




    __off64_t l_start;
    __off64_t l_len;

    __pid_t l_pid;
  };


struct flock64
  {
    short int l_type;
    short int l_whence;
    __off64_t l_start;
    __off64_t l_len;
    __pid_t l_pid;
  };
# 225 "/usr/include/bits/fcntl.h" 3 4

# 254 "/usr/include/bits/fcntl.h" 3 4

# 35 "/usr/include/fcntl.h" 2 3 4
# 76 "/usr/include/fcntl.h" 3 4
extern int fcntl (int __fd, int __cmd, ...);
# 88 "/usr/include/fcntl.h" 3 4
extern int open (__const char *__file, int __oflag, ...) __asm__ ("" "open64")
     __attribute__ ((__nonnull__ (1)));





extern int open64 (__const char *__file, int __oflag, ...) __attribute__ ((__nonnull__ (1)));
# 133 "/usr/include/fcntl.h" 3 4
extern int creat (__const char *__file, __mode_t __mode) __asm__ ("" "creat64") __attribute__ ((__nonnull__ (1)));






extern int creat64 (__const char *__file, __mode_t __mode) __attribute__ ((__nonnull__ (1)));
# 180 "/usr/include/fcntl.h" 3 4
extern int posix_fadvise (int __fd, __off64_t __offset, __off64_t __len, int __advise) __asm__ ("" "posix_fadvise64") __attribute__ ((__nothrow__));







extern int posix_fadvise64 (int __fd, __off64_t __offset, __off64_t __len,
       int __advise) __attribute__ ((__nothrow__));
# 201 "/usr/include/fcntl.h" 3 4
extern int posix_fallocate (int __fd, __off64_t __offset, __off64_t __len) __asm__ ("" "posix_fallocate64");







extern int posix_fallocate64 (int __fd, __off64_t __offset, __off64_t __len);
# 220 "/usr/include/fcntl.h" 3 4

# 34 "../libproc/common/libproc.h" 2
# 1 "/usr/include/nlist.h" 1 3 4
# 55 "/usr/include/nlist.h" 3 4
struct nlist
{
  char *n_name;
  long int n_value;
  short int n_scnum;
  unsigned short int n_type;
  char n_sclass;
  char n_numaux;
};







extern int nlist (__const char *__filename, struct nlist *__nl);
# 35 "../libproc/common/libproc.h" 2
# 1 "../linux/door.h" 1




typedef unsigned long long door_ptr_t;
typedef unsigned long long door_id_t;
typedef unsigned int door_attr_t;




typedef struct door_info {
        pid_t di_target;
        door_ptr_t di_proc;
        door_ptr_t di_data;
        door_attr_t di_attributes;
        door_id_t di_uniquifier;
        int di_resv[4];
} door_info_t;
# 36 "../libproc/common/libproc.h" 2
# 1 "/usr/include/gelf.h" 1 3 4
# 53 "/usr/include/gelf.h" 3 4
# 1 "/usr/include/libelf.h" 1 3 4
# 56 "/usr/include/libelf.h" 3 4
# 1 "/usr/include/elf.h" 1 3 4
# 25 "/usr/include/elf.h" 3 4







typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;


typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;


typedef uint64_t Elf32_Xword;
typedef int64_t Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;


typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;


typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;


typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;


typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;






typedef struct
{
  unsigned char e_ident[(16)];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
  unsigned char e_ident[(16)];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;
# 267 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct
{
  Elf64_Word sh_name;
  Elf64_Word sh_type;
  Elf64_Xword sh_flags;
  Elf64_Addr sh_addr;
  Elf64_Off sh_offset;
  Elf64_Xword sh_size;
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Xword sh_addralign;
  Elf64_Xword sh_entsize;
} Elf64_Shdr;
# 375 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Section st_shndx;
} Elf32_Sym;

typedef struct
{
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Section st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
} Elf64_Sym;




typedef struct
{
  Elf32_Half si_boundto;
  Elf32_Half si_flags;
} Elf32_Syminfo;

typedef struct
{
  Elf64_Half si_boundto;
  Elf64_Half si_flags;
} Elf64_Syminfo;
# 488 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
} Elf32_Rel;






typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
} Elf64_Rel;



typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
  Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
  Elf64_Sxword r_addend;
} Elf64_Rela;
# 533 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;

typedef struct
{
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;
# 615 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Sword d_tag;
  union
    {
      Elf32_Word d_val;
      Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct
{
  Elf64_Sxword d_tag;
  union
    {
      Elf64_Xword d_val;
      Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;
# 782 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Half vd_version;
  Elf32_Half vd_flags;
  Elf32_Half vd_ndx;
  Elf32_Half vd_cnt;
  Elf32_Word vd_hash;
  Elf32_Word vd_aux;
  Elf32_Word vd_next;

} Elf32_Verdef;

typedef struct
{
  Elf64_Half vd_version;
  Elf64_Half vd_flags;
  Elf64_Half vd_ndx;
  Elf64_Half vd_cnt;
  Elf64_Word vd_hash;
  Elf64_Word vd_aux;
  Elf64_Word vd_next;

} Elf64_Verdef;
# 824 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Word vda_name;
  Elf32_Word vda_next;

} Elf32_Verdaux;

typedef struct
{
  Elf64_Word vda_name;
  Elf64_Word vda_next;

} Elf64_Verdaux;




typedef struct
{
  Elf32_Half vn_version;
  Elf32_Half vn_cnt;
  Elf32_Word vn_file;

  Elf32_Word vn_aux;
  Elf32_Word vn_next;

} Elf32_Verneed;

typedef struct
{
  Elf64_Half vn_version;
  Elf64_Half vn_cnt;
  Elf64_Word vn_file;

  Elf64_Word vn_aux;
  Elf64_Word vn_next;

} Elf64_Verneed;
# 871 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Word vna_hash;
  Elf32_Half vna_flags;
  Elf32_Half vna_other;
  Elf32_Word vna_name;
  Elf32_Word vna_next;

} Elf32_Vernaux;

typedef struct
{
  Elf64_Word vna_hash;
  Elf64_Half vna_flags;
  Elf64_Half vna_other;
  Elf64_Word vna_name;
  Elf64_Word vna_next;

} Elf64_Vernaux;
# 905 "/usr/include/elf.h" 3 4
typedef struct
{
  uint32_t a_type;
  union
    {
      uint32_t a_val;



    } a_un;
} Elf32_auxv_t;

typedef struct
{
  uint64_t a_type;
  union
    {
      uint64_t a_val;



    } a_un;
} Elf64_auxv_t;
# 983 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
} Elf32_Nhdr;

typedef struct
{
  Elf64_Word n_namesz;
  Elf64_Word n_descsz;
  Elf64_Word n_type;
} Elf64_Nhdr;
# 1044 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Xword m_value;
  Elf32_Word m_info;
  Elf32_Word m_poffset;
  Elf32_Half m_repeat;
  Elf32_Half m_stride;
} Elf32_Move;

typedef struct
{
  Elf64_Xword m_value;
  Elf64_Xword m_info;
  Elf64_Xword m_poffset;
  Elf64_Half m_repeat;
  Elf64_Half m_stride;
} Elf64_Move;
# 1390 "/usr/include/elf.h" 3 4
typedef union
{
  struct
    {
      Elf32_Word gt_current_g_value;
      Elf32_Word gt_unused;
    } gt_header;
  struct
    {
      Elf32_Word gt_g_value;
      Elf32_Word gt_bytes;
    } gt_entry;
} Elf32_gptab;



typedef struct
{
  Elf32_Word ri_gprmask;
  Elf32_Word ri_cprmask[4];
  Elf32_Sword ri_gp_value;
} Elf32_RegInfo;



typedef struct
{
  unsigned char kind;

  unsigned char size;
  Elf32_Section section;

  Elf32_Word info;
} Elf_Options;
# 1466 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Word hwp_flags1;
  Elf32_Word hwp_flags2;
} Elf_Options_Hw;
# 1619 "/usr/include/elf.h" 3 4
typedef struct
{
  Elf32_Word l_name;
  Elf32_Word l_time_stamp;
  Elf32_Word l_checksum;
  Elf32_Word l_version;
  Elf32_Word l_flags;
} Elf32_Lib;

typedef struct
{
  Elf64_Word l_name;
  Elf64_Word l_time_stamp;
  Elf64_Word l_checksum;
  Elf64_Word l_version;
  Elf64_Word l_flags;
} Elf64_Lib;
# 1650 "/usr/include/elf.h" 3 4
typedef Elf32_Addr Elf32_Conflict;
# 2643 "/usr/include/elf.h" 3 4

# 57 "/usr/include/libelf.h" 2 3 4



typedef enum
{
  ELF_T_BYTE,
  ELF_T_ADDR,
  ELF_T_DYN,
  ELF_T_EHDR,
  ELF_T_HALF,
  ELF_T_OFF,
  ELF_T_PHDR,
  ELF_T_RELA,
  ELF_T_REL,
  ELF_T_SHDR,
  ELF_T_SWORD,
  ELF_T_SYM,
  ELF_T_WORD,
  ELF_T_XWORD,
  ELF_T_SXWORD,
  ELF_T_VDEF,
  ELF_T_VDAUX,
  ELF_T_VNEED,
  ELF_T_VNAUX,
  ELF_T_NHDR,
  ELF_T_SYMINFO,
  ELF_T_MOVE,
  ELF_T_LIB,
  ELF_T_GNUHASH,
  ELF_T_AUXV,

  ELF_T_NUM
} Elf_Type;






typedef struct
{
  void *d_buf;
  Elf_Type d_type;
  unsigned int d_version;
  size_t d_size;
  off64_t d_off;
  size_t d_align;
} Elf_Data;



typedef enum
{
  ELF_C_NULL,
  ELF_C_READ,
  ELF_C_RDWR,
  ELF_C_WRITE,
  ELF_C_CLR,
  ELF_C_SET,
  ELF_C_FDDONE,

  ELF_C_FDREAD,


  ELF_C_READ_MMAP,
  ELF_C_RDWR_MMAP,
  ELF_C_WRITE_MMAP,
  ELF_C_READ_MMAP_PRIVATE,

  ELF_C_EMPTY,

  ELF_C_NUM
} Elf_Cmd;



enum
{
  ELF_F_DIRTY = 0x1,

  ELF_F_LAYOUT = 0x4,

  ELF_F_PERMISSIVE = 0x8

};



typedef enum
{
  ELF_K_NONE,
  ELF_K_AR,
  ELF_K_COFF,
  ELF_K_ELF,

  ELF_K_NUM
} Elf_Kind;



typedef struct
{
  char *ar_name;
  time_t ar_date;
  uid_t ar_uid;
  gid_t ar_gid;
  mode_t ar_mode;
  off64_t ar_size;
  char *ar_rawname;
} Elf_Arhdr;



typedef struct
{
  char *as_name;
  size_t as_off;
  unsigned long int as_hash;
} Elf_Arsym;



typedef struct Elf Elf;


typedef struct Elf_Scn Elf_Scn;







extern Elf *elf_begin (int __fildes, Elf_Cmd __cmd, Elf *__ref);


  extern Elf *elf_clone (Elf *__elf, Elf_Cmd __cmd);


extern Elf *elf_memory (char *__image, size_t __size);


extern Elf_Cmd elf_next (Elf *__elf);


extern int elf_end (Elf *__elf);


extern off64_t elf_update (Elf *__elf, Elf_Cmd __cmd);


extern Elf_Kind elf_kind (Elf *__elf) __attribute__ ((__pure__));


extern off64_t elf_getbase (Elf *__elf);



extern char *elf_getident (Elf *__elf, size_t *__nbytes);


extern Elf32_Ehdr *elf32_getehdr (Elf *__elf);

extern Elf64_Ehdr *elf64_getehdr (Elf *__elf);


extern Elf32_Ehdr *elf32_newehdr (Elf *__elf);

extern Elf64_Ehdr *elf64_newehdr (Elf *__elf);


extern Elf32_Phdr *elf32_getphdr (Elf *__elf);

extern Elf64_Phdr *elf64_getphdr (Elf *__elf);


extern Elf32_Phdr *elf32_newphdr (Elf *__elf, size_t __cnt);

extern Elf64_Phdr *elf64_newphdr (Elf *__elf, size_t __cnt);



extern Elf_Scn *elf_getscn (Elf *__elf, size_t __index);


extern Elf_Scn *elf32_offscn (Elf *__elf, Elf32_Off __offset);

extern Elf_Scn *elf64_offscn (Elf *__elf, Elf64_Off __offset);


extern size_t elf_ndxscn (Elf_Scn *__scn);


extern Elf_Scn *elf_nextscn (Elf *__elf, Elf_Scn *__scn);


extern Elf_Scn *elf_newscn (Elf *__elf);





extern int elf_getshnum (Elf *__elf, size_t *__dst);






extern int elf_getshstrndx (Elf *__elf, size_t *__dst);



extern Elf32_Shdr *elf32_getshdr (Elf_Scn *__scn);

extern Elf64_Shdr *elf64_getshdr (Elf_Scn *__scn);



extern unsigned int elf_flagelf (Elf *__elf, Elf_Cmd __cmd,
     unsigned int __flags);

extern unsigned int elf_flagehdr (Elf *__elf, Elf_Cmd __cmd,
      unsigned int __flags);

extern unsigned int elf_flagphdr (Elf *__elf, Elf_Cmd __cmd,
      unsigned int __flags);

extern unsigned int elf_flagscn (Elf_Scn *__scn, Elf_Cmd __cmd,
     unsigned int __flags);

extern unsigned int elf_flagdata (Elf_Data *__data, Elf_Cmd __cmd,
      unsigned int __flags);

extern unsigned int elf_flagshdr (Elf_Scn *__scn, Elf_Cmd __cmd,
      unsigned int __flags);




extern Elf_Data *elf_getdata (Elf_Scn *__scn, Elf_Data *__data);


extern Elf_Data *elf_rawdata (Elf_Scn *__scn, Elf_Data *__data);


extern Elf_Data *elf_newdata (Elf_Scn *__scn);




extern Elf_Data *elf_getdata_rawchunk (Elf *__elf,
           off64_t __offset, size_t __size,
           Elf_Type __type);



extern char *elf_strptr (Elf *__elf, size_t __index, size_t __offset);



extern Elf_Arhdr *elf_getarhdr (Elf *__elf);


extern off64_t elf_getaroff (Elf *__elf);


extern size_t elf_rand (Elf *__elf, size_t __offset);


extern Elf_Arsym *elf_getarsym (Elf *__elf, size_t *__narsyms);



extern int elf_cntl (Elf *__elf, Elf_Cmd __cmd);


extern char *elf_rawfile (Elf *__elf, size_t *__nbytes);





extern size_t elf32_fsize (Elf_Type __type, size_t __count,
      unsigned int __version)
       __attribute__ ((__const__));

extern size_t elf64_fsize (Elf_Type __type, size_t __count,
      unsigned int __version)
       __attribute__ ((__const__));




extern Elf_Data *elf32_xlatetom (Elf_Data *__dest, const Elf_Data *__src,
     unsigned int __encode);

extern Elf_Data *elf64_xlatetom (Elf_Data *__dest, const Elf_Data *__src,
     unsigned int __encode);



extern Elf_Data *elf32_xlatetof (Elf_Data *__dest, const Elf_Data *__src,
     unsigned int __encode);

extern Elf_Data *elf64_xlatetof (Elf_Data *__dest, const Elf_Data *__src,
     unsigned int __encode);




extern int elf_errno (void);





extern const char *elf_errmsg (int __error);



extern unsigned int elf_version (unsigned int __version);


extern void elf_fill (int __fill);


extern unsigned long int elf_hash (const char *__string)
       __attribute__ ((__pure__));


extern unsigned long int elf_gnu_hash (const char *__string)
       __attribute__ ((__pure__));



extern long int elf32_checksum (Elf *__elf);

extern long int elf64_checksum (Elf *__elf);
# 54 "/usr/include/gelf.h" 2 3 4
# 65 "/usr/include/gelf.h" 3 4
typedef Elf64_Half GElf_Half;


typedef Elf64_Word GElf_Word;
typedef Elf64_Sword GElf_Sword;


typedef Elf64_Xword GElf_Xword;
typedef Elf64_Sxword GElf_Sxword;


typedef Elf64_Addr GElf_Addr;


typedef Elf64_Off GElf_Off;



typedef Elf64_Ehdr GElf_Ehdr;


typedef Elf64_Shdr GElf_Shdr;




typedef Elf64_Section GElf_Section;


typedef Elf64_Sym GElf_Sym;



typedef Elf64_Syminfo GElf_Syminfo;


typedef Elf64_Rel GElf_Rel;


typedef Elf64_Rela GElf_Rela;


typedef Elf64_Phdr GElf_Phdr;


typedef Elf64_Dyn GElf_Dyn;



typedef Elf64_Verdef GElf_Verdef;


typedef Elf64_Verdaux GElf_Verdaux;


typedef Elf64_Verneed GElf_Verneed;


typedef Elf64_Vernaux GElf_Vernaux;



typedef Elf64_Versym GElf_Versym;



typedef Elf64_auxv_t GElf_auxv_t;



typedef Elf64_Nhdr GElf_Nhdr;



typedef Elf64_Move GElf_Move;



typedef Elf64_Lib GElf_Lib;
# 171 "/usr/include/gelf.h" 3 4
extern int gelf_getclass (Elf *__elf);





extern size_t gelf_fsize (Elf *__elf, Elf_Type __type, size_t __count,
     unsigned int __version);


extern GElf_Ehdr *gelf_getehdr (Elf *__elf, GElf_Ehdr *__dest);


extern int gelf_update_ehdr (Elf *__elf, GElf_Ehdr *__src);


extern unsigned long int gelf_newehdr (Elf *__elf, int __class);


extern Elf_Scn *gelf_offscn (Elf *__elf, GElf_Off __offset);


extern GElf_Shdr *gelf_getshdr (Elf_Scn *__scn, GElf_Shdr *__dst);


extern int gelf_update_shdr (Elf_Scn *__scn, GElf_Shdr *__src);


extern GElf_Phdr *gelf_getphdr (Elf *__elf, int __ndx, GElf_Phdr *__dst);


extern int gelf_update_phdr (Elf *__elf, int __ndx, GElf_Phdr *__src);


extern unsigned long int gelf_newphdr (Elf *__elf, size_t __phnum);




extern Elf_Data *gelf_xlatetom (Elf *__elf, Elf_Data *__dest,
    const Elf_Data *__src, unsigned int __encode);



extern Elf_Data *gelf_xlatetof (Elf *__elf, Elf_Data *__dest,
    const Elf_Data *__src, unsigned int __encode);



extern GElf_Rel *gelf_getrel (Elf_Data *__data, int __ndx, GElf_Rel *__dst);


extern GElf_Rela *gelf_getrela (Elf_Data *__data, int __ndx, GElf_Rela *__dst);


extern int gelf_update_rel (Elf_Data *__dst, int __ndx, GElf_Rel *__src);


extern int gelf_update_rela (Elf_Data *__dst, int __ndx, GElf_Rela *__src);



extern GElf_Sym *gelf_getsym (Elf_Data *__data, int __ndx, GElf_Sym *__dst);


extern int gelf_update_sym (Elf_Data *__data, int __ndx, GElf_Sym *__src);




extern GElf_Sym *gelf_getsymshndx (Elf_Data *__symdata, Elf_Data *__shndxdata,
       int __ndx, GElf_Sym *__sym,
       Elf32_Word *__xshndx);



extern int gelf_update_symshndx (Elf_Data *__symdata, Elf_Data *__shndxdata,
     int __ndx, GElf_Sym *__sym,
     Elf32_Word __xshndx);




extern GElf_Syminfo *gelf_getsyminfo (Elf_Data *__data, int __ndx,
          GElf_Syminfo *__dst);



extern int gelf_update_syminfo (Elf_Data *__data, int __ndx,
    GElf_Syminfo *__src);



extern GElf_Dyn *gelf_getdyn (Elf_Data *__data, int __ndx, GElf_Dyn *__dst);


extern int gelf_update_dyn (Elf_Data *__dst, int __ndx, GElf_Dyn *__src);



extern GElf_Move *gelf_getmove (Elf_Data *__data, int __ndx, GElf_Move *__dst);


extern int gelf_update_move (Elf_Data *__data, int __ndx,
        GElf_Move *__src);



extern GElf_Lib *gelf_getlib (Elf_Data *__data, int __ndx, GElf_Lib *__dst);


extern int gelf_update_lib (Elf_Data *__data, int __ndx, GElf_Lib *__src);




extern GElf_Versym *gelf_getversym (Elf_Data *__data, int __ndx,
        GElf_Versym *__dst);


extern int gelf_update_versym (Elf_Data *__data, int __ndx,
          GElf_Versym *__src);



extern GElf_Verneed *gelf_getverneed (Elf_Data *__data, int __offset,
          GElf_Verneed *__dst);


extern int gelf_update_verneed (Elf_Data *__data, int __offset,
    GElf_Verneed *__src);


extern GElf_Vernaux *gelf_getvernaux (Elf_Data *__data, int __offset,
          GElf_Vernaux *__dst);


extern int gelf_update_vernaux (Elf_Data *__data, int __offset,
    GElf_Vernaux *__src);



extern GElf_Verdef *gelf_getverdef (Elf_Data *__data, int __offset,
        GElf_Verdef *__dst);


extern int gelf_update_verdef (Elf_Data *__data, int __offset,
          GElf_Verdef *__src);



extern GElf_Verdaux *gelf_getverdaux (Elf_Data *__data, int __offset,
          GElf_Verdaux *__dst);


extern int gelf_update_verdaux (Elf_Data *__data, int __offset,
    GElf_Verdaux *__src);



extern GElf_auxv_t *gelf_getauxv (Elf_Data *__data, int __ndx,
      GElf_auxv_t *__dst);


extern int gelf_update_auxv (Elf_Data *__data, int __ndx, GElf_auxv_t *__src);





extern size_t gelf_getnote (Elf_Data *__data, size_t __offset,
       GElf_Nhdr *__result,
       size_t *__name_offset, size_t *__desc_offset);



extern long int gelf_checksum (Elf *__elf);
# 37 "../libproc/common/libproc.h" 2
# 1 "../linux/proc_service.h" 1
typedef enum {
        PS_OK,
        PS_ERR,
        PS_BADPID,
        PS_BADLID,
        PS_BADADDR,
        PS_NOSYM,
        PS_NOFREGS
} ps_err_e;


typedef Elf64_Sym ps_sym_t;
# 38 "../libproc/common/libproc.h" 2
# 1 "../linux/rtld_db.h" 1
# 15 "../linux/rtld_db.h"
typedef enum {
        RD_ERR,
        RD_OK,
        RD_NOCAPAB,
        RD_DBERR,
        RD_NOBASE,
        RD_NODYNAM,
        RD_NOMAPS
} rd_err_e;





typedef enum {
        RD_NONE = 0,
        RD_PREINIT,
        RD_POSTINIT,
        RD_DLACTIVITY
} rd_event_e;




typedef enum {
        RD_NOTIFY_BPT,
        RD_NOTIFY_AUTOBPT,
        RD_NOTIFY_SYSCALL
} rd_notify_e;




typedef struct rd_notify {
        rd_notify_e type;
        union {
                psaddr_t bptaddr;
                long syscallno;
        } u;
} rd_notify_t;




typedef enum {
        RD_NOSTATE = 0,
        RD_CONSISTENT,
        RD_ADD,
        RD_DELETE
} rd_state_e;

typedef struct rd_event_msg {
        rd_event_e type;
        union {
                rd_state_e state;
        } u;
} rd_event_msg_t;




typedef struct rd_loadobj {
        psaddr_t rl_nameaddr;
        unsigned rl_flags;
        psaddr_t rl_base;
        psaddr_t rl_data_base;
        Lmid_t rl_lmident;
        psaddr_t rl_refnameaddr;


        psaddr_t rl_plt_base;
        unsigned rl_plt_size;

        psaddr_t rl_bend;
        psaddr_t rl_padstart;
        psaddr_t rl_padend;
        psaddr_t rl_dynamic;

        unsigned long rl_tlsmodid;
} rd_loadobj_t;

typedef int rl_iter_f(const rd_loadobj_t *, void *);
# 39 "../libproc/common/libproc.h" 2
# 1 "../linux/procfs.h" 1
# 1 "../linux/sys/procfs.h" 1




# 1 "../linux/sys/procfs_isa.h" 1
# 14 "../linux/sys/procfs_isa.h"
typedef long prgreg_t;



typedef prgreg_t prgregset_t[38];
typedef prgreg32_t prgregset32_t[38];
# 37 "../linux/sys/procfs_isa.h"
typedef uchar_t instr_t;


struct fpq {
        unsigned int *fpq_addr;
        unsigned int fpq_instr;
};
struct fq {
        union {
                double whole;
                struct fpq fpq;
        } FQu;
};
# 70 "../linux/sys/procfs_isa.h"
typedef struct prfpregset {
        union {
                uint32_t pr_regs[32];
                double pr_dregs[16];
        } pr_fr;
        uint32_t pr_filler;
        uint32_t pr_fsr;
        uint8_t pr_qcnt;
        uint8_t pr_q_entrysize;
        uint8_t pr_en;
        struct fq pr_q[32];
} prfpregset_t;
# 6 "../linux/sys/procfs.h" 2

# 1 "../linux/sys/signal.h" 1




# 1 "../linux/sys/procset.h" 1
# 6 "../linux/sys/signal.h" 2
# 1 "/usr/include/ucontext.h" 1 3 4
# 27 "/usr/include/ucontext.h" 3 4
# 1 "/usr/include/sys/ucontext.h" 1 3 4
# 24 "/usr/include/sys/ucontext.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 25 "/usr/include/sys/ucontext.h" 2 3 4
# 33 "/usr/include/sys/ucontext.h" 3 4
typedef long int greg_t;





typedef greg_t gregset_t[23];
# 94 "/usr/include/sys/ucontext.h" 3 4
struct _libc_fpxreg
{
  unsigned short int significand[4];
  unsigned short int exponent;
  unsigned short int padding[3];
};

struct _libc_xmmreg
{
  __uint32_t element[4];
};

struct _libc_fpstate
{

  __uint16_t cwd;
  __uint16_t swd;
  __uint16_t ftw;
  __uint16_t fop;
  __uint64_t rip;
  __uint64_t rdp;
  __uint32_t mxcsr;
  __uint32_t mxcr_mask;
  struct _libc_fpxreg _st[8];
  struct _libc_xmmreg _xmm[16];
  __uint32_t padding[24];
};


typedef struct _libc_fpstate *fpregset_t;


typedef struct
  {
    gregset_t gregs;

    fpregset_t fpregs;
    unsigned long __reserved1 [8];
} mcontext_t;


typedef struct ucontext
  {
    unsigned long int uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
    __sigset_t uc_sigmask;
    struct _libc_fpstate __fpregs_mem;
  } ucontext_t;
# 28 "/usr/include/ucontext.h" 2 3 4




extern int getcontext (ucontext_t *__ucp) __attribute__ ((__nothrow__));


extern int setcontext (__const ucontext_t *__ucp) __attribute__ ((__nothrow__));



extern int swapcontext (ucontext_t *__restrict __oucp,
   __const ucontext_t *__restrict __ucp) __attribute__ ((__nothrow__));







extern void makecontext (ucontext_t *__ucp, void (*__func) (void),
    int __argc, ...) __attribute__ ((__nothrow__));


# 7 "../linux/sys/signal.h" 2
# 69 "../linux/sys/signal.h"
typedef struct {
        uint32_t __sigbits[4];
} sigset32_t;
# 80 "../linux/sys/signal.h"
typedef struct sigaltstack32 {
        caddr32_t ss_sp;
        size32_t ss_size;
        int32_t ss_flags;
} stack32_t;
struct sigaction32 {
        int32_t sa_flags;
        union {
                caddr32_t _handler;
                caddr32_t _sigaction;
        } __sigaction_handler;
        sigset32_t sa_mask;
        int32_t sa_resv[2];
};



union sigval32 {
        int32_t sival_int;
        caddr32_t sival_ptr;
};

typedef struct siginfo32 {

        int32_t si_signo;
        int32_t si_code;
        int32_t si_errno;

        union {

                int32_t __pad[((128 / sizeof (int32_t)) - 3)];

                struct {
                        pid32_t __pid;
                        union {
                                struct {
                                        uid32_t __uid;
                                        union sigval32 __value;
                                } __kill;
                                struct {
                                        clock32_t __utime;
                                        int32_t __status;
                                        clock32_t __stime;
                                } __cld;
                        } __pdata;
                        id32_t __ctid;
                        id32_t __zoneid;
                } __proc;

                struct {
                        caddr32_t __addr;
                        int32_t __trapno;
                        caddr32_t __pc;
                } __fault;

                struct {

                        int32_t __fd;
                        int32_t __band;
                } __file;

                struct {
                        caddr32_t __faddr;
                        timestruc32_t __tstamp;
                        int16_t __syscall;
                        int8_t __nsysarg;
                        int8_t __fault;
                        int32_t __sysarg[8];
                        int32_t __mstate[10];
                } __prof;

                struct {
                        int32_t __entity;
                } __rctl;

        } __data;

} siginfo32_t;
# 8 "../linux/sys/procfs.h" 2
# 1 "../linux/sys/priv.h" 1



typedef struct priv_impl_info {
        uint32_t priv_headersize;
        uint32_t priv_flags;
        uint32_t priv_nsets;
        uint32_t priv_setsize;
        uint32_t priv_max;
        uint32_t priv_infosize;
        uint32_t priv_globalinfosize;
} priv_impl_info_t;
# 9 "../linux/sys/procfs.h" 2
# 1 "../linux/sys/fault.h" 1
# 34 "../linux/sys/fault.h"
#pragma ident "@(#)fault.h    1.15    05/06/08 SMI"
# 63 "../linux/sys/fault.h"
typedef struct {
        unsigned int word[4];
} fltset_t;
# 10 "../linux/sys/procfs.h" 2
# 127 "../linux/sys/procfs.h"
typedef struct {
        unsigned int word[16];
} sysset_t;

typedef int psetid_t;
typedef id_t taskid_t;
typedef id_t poolid_t;
typedef id_t projid_t;
typedef uint32_t priv_chunk_t;

struct fpq32 {
        caddr32_t fpq_addr;
        uint32_t fpq_instr;
};

struct fq32 {
        union {
                double whole;
                struct fpq32 fpq;
        } FQu;
};

typedef struct prfpregset32 {
        union {
                uint32_t pr_regs[32];
                double pr_dregs[16];
        } pr_fr;
        uint32_t pr_filler;
        uint32_t pr_fsr;
        uint8_t pr_qcnt;
        uint8_t pr_q_entrysize;
        uint8_t pr_en;
        struct fq32 pr_q[32];
} prfpregset32_t;





typedef struct prmap {
        uintptr_t pr_vaddr;
        size_t pr_size;
        char pr_mapname[64];
        offset_t pr_offset;
        int pr_mflags;
        int pr_pagesize;
        int pr_shmid;
        int pr_filler[1];
} prmap_t;







typedef struct lwpstatus32 {
        int pr_flags;
        id32_t pr_lwpid;
        short pr_why;
        short pr_what;
        short pr_cursig;
        short pr_pad1;
        siginfo32_t pr_info;
        sigset_t pr_lwppend;
        sigset_t pr_lwphold;
        struct sigaction32 pr_action;
        stack32_t pr_altstack;
        caddr32_t pr_oldcontext;
        short pr_syscall;
        short pr_nsysarg;
        int pr_errno;
        int32_t pr_sysarg[8];
        int32_t pr_rval1;
        int32_t pr_rval2;
        char pr_clname[8];
        timestruc32_t pr_tstamp;
        timestruc32_t pr_utime;
        timestruc32_t pr_stime;
        int pr_filler[11 - 2 * sizeof (timestruc32_t) / sizeof (int)];
        int pr_errpriv;
        caddr32_t pr_ustack;
        uint32_t pr_instr;
        prgregset32_t pr_reg;
        prfpregset32_t pr_fpreg;
} lwpstatus32_t;

typedef struct lwpstatus {
 int pr_flags;
 id_t pr_lwpid;
 short pr_why;
 short pr_what;
 short pr_cursig;
 short pr_pad1;
 siginfo_t pr_info;
 sigset_t pr_lwppend;
 sigset_t pr_lwphold;
 struct sigaction pr_action;
 stack_t pr_altstack;
 uintptr_t pr_oldcontext;
 short pr_syscall;
 short pr_nsysarg;
 int pr_errno;
 long pr_sysarg[8];
 long pr_rval1;
 long pr_rval2;
 char pr_clname[8];
 timestruc_t pr_tstamp;
 timestruc_t pr_utime;
 timestruc_t pr_stime;
 int pr_filler[11 - 2 * sizeof (timestruc_t) / sizeof (int)];
 int pr_errpriv;
 uintptr_t pr_ustack;
 ulong_t pr_instr;
 prgregset_t pr_reg;
 prfpregset_t pr_fpreg;
} lwpstatus_t;




typedef struct pstatus32 {
        int pr_flags;
        int pr_nlwp;
        pid32_t pr_pid;
        pid32_t pr_ppid;
        pid32_t pr_pgid;
        pid32_t pr_sid;
        id32_t pr_aslwpid;
        id32_t pr_agentid;
        sigset_t pr_sigpend;
        caddr32_t pr_brkbase;
        size32_t pr_brksize;
        caddr32_t pr_stkbase;
        size32_t pr_stksize;
        timestruc32_t pr_utime;
        timestruc32_t pr_stime;
        timestruc32_t pr_cutime;
        timestruc32_t pr_cstime;
        sigset_t pr_sigtrace;
        fltset_t pr_flttrace;
        sysset_t pr_sysentry;
        sysset_t pr_sysexit;
        char pr_dmodel;
        char pr_pad[3];
        id32_t pr_taskid;
        id32_t pr_projid;
        int pr_nzomb;
        id32_t pr_zoneid;
        int pr_filler[15];
        lwpstatus32_t pr_lwp;
} pstatus32_t;

typedef struct pstatus {
        int pr_flags;
        int pr_nlwp;
        pid_t pr_pid;
        pid_t pr_ppid;
        pid_t pr_pgid;
        pid_t pr_sid;
        id_t pr_aslwpid;
        id_t pr_agentid;
        sigset_t pr_sigpend;
        uintptr_t pr_brkbase;
        size_t pr_brksize;
        uintptr_t pr_stkbase;
        size_t pr_stksize;
        timestruc_t pr_utime;
        timestruc_t pr_stime;
        timestruc_t pr_cutime;
        timestruc_t pr_cstime;
        sigset_t pr_sigtrace;
        fltset_t pr_flttrace;
        sysset_t pr_sysentry;
        sysset_t pr_sysexit;
        char pr_dmodel;
        char pr_pad[3];
        taskid_t pr_taskid;
        projid_t pr_projid;
        int pr_nzomb;
        zoneid_t pr_zoneid;
        int pr_filler[15];
        lwpstatus_t pr_lwp;
} pstatus_t;







typedef struct lwpsinfo32 {
        int pr_flag;
        id32_t pr_lwpid;
        caddr32_t pr_addr;
        caddr32_t pr_wchan;
        char pr_stype;
        char pr_state;
        char pr_sname;
        char pr_nice;
        short pr_syscall;
        char pr_oldpri;
        char pr_cpu;
        int pr_pri;



        ushort_t pr_pctcpu;
        ushort_t pr_pad;
        timestruc32_t pr_start;
        timestruc32_t pr_time;
        char pr_clname[8];
        char pr_name[16];
        processorid_t pr_onpro;
        processorid_t pr_bindpro;
        psetid_t pr_bindpset;
        int pr_filler[5];
} lwpsinfo32_t;


typedef struct lwpsinfo {
        int pr_flag;
        id_t pr_lwpid;
        uintptr_t pr_addr;
        uintptr_t pr_wchan;
        char pr_stype;
        char pr_state;
        char pr_sname;
        char pr_nice;
        short pr_syscall;
        char pr_oldpri;
        char pr_cpu;
        int pr_pri;



        ushort_t pr_pctcpu;
        ushort_t pr_pad;
        timestruc_t pr_start;
        timestruc_t pr_time;
        char pr_clname[8];
        char pr_name[16];
        processorid_t pr_onpro;
        processorid_t pr_bindpro;
        psetid_t pr_bindpset;
        int pr_filler[5];
} lwpsinfo_t;





typedef struct psinfo32 {
        int pr_flag;
        int pr_nlwp;
        pid32_t pr_pid;
        pid32_t pr_ppid;
        pid32_t pr_pgid;
        pid32_t pr_sid;
        uid32_t pr_uid;
        uid32_t pr_euid;
        gid32_t pr_gid;
        gid32_t pr_egid;
        caddr32_t pr_addr;
        size32_t pr_size;
        size32_t pr_rssize;
        size32_t pr_pad1;
        dev32_t pr_ttydev;
        ushort_t pr_pctcpu;
        ushort_t pr_pctmem;
        timestruc32_t pr_start;
        timestruc32_t pr_time;
        timestruc32_t pr_ctime;
        char pr_fname[16];
        char pr_psargs[80];
        int pr_wstat;
        int pr_argc;
        caddr32_t pr_argv;
        caddr32_t pr_envp;
        char pr_dmodel;
        char pr_pad2[3];
        id32_t pr_taskid;
        id32_t pr_projid;
        int pr_nzomb;
        id32_t pr_poolid;
        id32_t pr_zoneid;
        id32_t pr_contract;
        int pr_filler[1];
        lwpsinfo32_t pr_lwp;
} psinfo32_t;




typedef struct psinfo {
        int pr_flag;
        int pr_nlwp;
        pid_t pr_pid;
        pid_t pr_ppid;
        pid_t pr_pgid;
        pid_t pr_sid;
        uid_t pr_uid;
        uid_t pr_euid;
        gid_t pr_gid;
        gid_t pr_egid;
        uintptr_t pr_addr;
        size_t pr_size;
        size_t pr_rssize;
        size_t pr_pad1;
        dev_t pr_ttydev;



        ushort_t pr_pctcpu;
        ushort_t pr_pctmem;
        timestruc_t pr_start;
        timestruc_t pr_time;
        timestruc_t pr_ctime;
        char pr_fname[16];
        char pr_psargs[80];
        int pr_wstat;
        int pr_argc;
        uintptr_t pr_argv;
        uintptr_t pr_envp;
        char pr_dmodel;
        char pr_pad2[3];
        taskid_t pr_taskid;
        projid_t pr_projid;
        int pr_nzomb;
        poolid_t pr_poolid;
        zoneid_t pr_zoneid;
        id_t pr_contract;
        int pr_filler[1];
        lwpsinfo_t pr_lwp;
} psinfo_t;




typedef struct prcred {
        uid_t pr_euid;
        uid_t pr_ruid;
        uid_t pr_suid;
        gid_t pr_egid;
        gid_t pr_rgid;
        gid_t pr_sgid;
        int pr_ngroups;
        gid_t pr_groups[1];
} prcred_t;




typedef struct prpriv {
        uint32_t pr_nsets;
        uint32_t pr_setsize;
        uint32_t pr_infosize;
        priv_chunk_t pr_sets[1];
} prpriv_t;




typedef struct prwatch {
        uintptr_t pr_vaddr;
        size_t pr_size;
        int pr_wflags;
        int pr_pad;
} prwatch_t;

typedef struct {
        long sys_rval1;
        long sys_rval2;
} sysret_t;







typedef struct prheader {
        long pr_nent;
        long pr_entsize;
} prheader_t;




typedef struct priovec {
        void *pio_base;
        size_t pio_len;
        off_t pio_offset;
} priovec_t;
# 1 "../linux/procfs.h" 2
# 40 "../libproc/common/libproc.h" 2
# 1 "../linux/rctl.h" 1
typedef struct rctlblk rctlblk_t;
# 41 "../libproc/common/libproc.h" 2
# 1 "../libctf/libctf.h" 1
# 43 "../libctf/libctf.h"
#pragma ident "@(#)libctf.h	1.6	05/06/08 SMI"

# 1 "../uts/common/sys/ctf_api.h" 1
# 26 "../uts/common/sys/ctf_api.h"
#pragma ident "@(#)ctf_api.h	1.1	03/09/02 SMI"



# 1 "/usr/include/linux/elf.h" 1 3 4



# 1 "/usr/include/linux/types.h" 1 3 4




# 1 "/usr/include/linux/posix_types.h" 1 3 4



# 1 "/usr/include/linux/stddef.h" 1 3 4
# 5 "/usr/include/linux/posix_types.h" 2 3 4
# 36 "/usr/include/linux/posix_types.h" 3 4
typedef struct {
 unsigned long fds_bits [(1024/(8 * sizeof(unsigned long)))];
} __kernel_fd_set;


typedef void (*__kernel_sighandler_t)(int);


typedef int __kernel_key_t;
typedef int __kernel_mqd_t;

# 1 "/usr/include/asm/posix_types.h" 1 3 4



# 1 "/usr/include/asm/posix_types_64.h" 1 3 4
# 10 "/usr/include/asm/posix_types_64.h" 3 4
typedef unsigned long __kernel_ino_t;
typedef unsigned int __kernel_mode_t;
typedef unsigned long __kernel_nlink_t;
typedef long __kernel_off_t;
typedef int __kernel_pid_t;
typedef int __kernel_ipc_pid_t;
typedef unsigned int __kernel_uid_t;
typedef unsigned int __kernel_gid_t;
typedef unsigned long __kernel_size_t;
typedef long __kernel_ssize_t;
typedef long __kernel_ptrdiff_t;
typedef long __kernel_time_t;
typedef long __kernel_suseconds_t;
typedef long __kernel_clock_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef int __kernel_daddr_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;


typedef long long __kernel_loff_t;


typedef struct {
 int val[2];
} __kernel_fsid_t;

typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;
typedef __kernel_uid_t __kernel_uid32_t;
typedef __kernel_gid_t __kernel_gid32_t;

typedef unsigned long __kernel_old_dev_t;
# 5 "/usr/include/asm/posix_types.h" 2 3 4
# 48 "/usr/include/linux/posix_types.h" 2 3 4
# 6 "/usr/include/linux/types.h" 2 3 4
# 1 "/usr/include/asm/types.h" 1 3 4





typedef unsigned short umode_t;






typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;







typedef __signed__ long long __s64;
typedef unsigned long long __u64;
# 7 "/usr/include/linux/types.h" 2 3 4
# 153 "/usr/include/linux/types.h" 3 4
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;

typedef __u64 __le64;
typedef __u64 __be64;

typedef __u16 __sum16;
typedef __u32 __wsum;


struct ustat {
 __kernel_daddr_t f_tfree;
 __kernel_ino_t f_tinode;
 char f_fname[6];
 char f_fpack[6];
};
# 5 "/usr/include/linux/elf.h" 2 3 4
# 1 "/usr/include/linux/elf-em.h" 1 3 4
# 6 "/usr/include/linux/elf.h" 2 3 4
# 1 "/usr/include/asm/elf.h" 1 3 4







# 1 "/usr/include/asm/ptrace.h" 1 3 4



# 1 "/usr/include/asm/ptrace-abi.h" 1 3 4
# 5 "/usr/include/asm/ptrace.h" 2 3 4
# 35 "/usr/include/asm/ptrace.h" 3 4
struct pt_regs {
 unsigned long r15;
 unsigned long r14;
 unsigned long r13;
 unsigned long r12;
 unsigned long rbp;
 unsigned long rbx;

 unsigned long r11;
 unsigned long r10;
 unsigned long r9;
 unsigned long r8;
 unsigned long rax;
 unsigned long rcx;
 unsigned long rdx;
 unsigned long rsi;
 unsigned long rdi;
 unsigned long orig_rax;


 unsigned long rip;
 unsigned long cs;
 unsigned long eflags;
 unsigned long rsp;
 unsigned long ss;

};
# 9 "/usr/include/asm/elf.h" 2 3 4
# 1 "/usr/include/asm/user.h" 1 3 4



# 1 "/usr/include/asm/user_64.h" 1 3 4




# 1 "/usr/include/asm/page.h" 1 3 4



# 1 "/usr/include/asm/page_64.h" 1 3 4



# 1 "/usr/include/linux/const.h" 1 3 4
# 5 "/usr/include/asm/page_64.h" 2 3 4
# 5 "/usr/include/asm/page.h" 2 3 4
# 6 "/usr/include/asm/user_64.h" 2 3 4
# 50 "/usr/include/asm/user_64.h" 3 4
struct user_i387_struct {
 unsigned short cwd;
 unsigned short swd;
 unsigned short twd;
 unsigned short fop;
 __u64 rip;
 __u64 rdp;
 __u32 mxcsr;
 __u32 mxcsr_mask;
 __u32 st_space[32];
 __u32 xmm_space[64];
 __u32 padding[24];
};




struct user_regs_struct {
 unsigned long r15,r14,r13,r12,rbp,rbx,r11,r10;
 unsigned long r9,r8,rax,rcx,rdx,rsi,rdi,orig_rax;
 unsigned long rip,cs,eflags;
 unsigned long rsp,ss;
   unsigned long fs_base, gs_base;
 unsigned long ds,es,fs,gs;
};




struct user{


  struct user_regs_struct regs;

  int u_fpvalid;

  int pad0;
  struct user_i387_struct i387;

  unsigned long int u_tsize;
  unsigned long int u_dsize;
  unsigned long int u_ssize;
  unsigned long start_code;
  unsigned long start_stack;



  long int signal;
  int reserved;
  int pad1;
  struct user_pt_regs * u_ar0;

  struct user_i387_struct* u_fpstate;
  unsigned long magic;
  char u_comm[32];
  unsigned long u_debugreg[8];
  unsigned long error_code;
  unsigned long fault_address;
};
# 5 "/usr/include/asm/user.h" 2 3 4
# 10 "/usr/include/asm/elf.h" 2 3 4
# 1 "/usr/include/asm/auxvec.h" 1 3 4
# 11 "/usr/include/asm/elf.h" 2 3 4

typedef unsigned long elf_greg_t;


typedef elf_greg_t elf_gregset_t[(sizeof (struct user_regs_struct) / sizeof(elf_greg_t))];

typedef struct user_i387_struct elf_fpregset_t;
# 7 "/usr/include/linux/elf.h" 2 3 4

struct file;
# 18 "/usr/include/linux/elf.h" 3 4
typedef __u32 Elf32_Addr;
typedef __u16 Elf32_Half;
typedef __u32 Elf32_Off;
typedef __s32 Elf32_Sword;
typedef __u32 Elf32_Word;


typedef __u64 Elf64_Addr;
typedef __u16 Elf64_Half;
typedef __s16 Elf64_SHalf;
typedef __u64 Elf64_Off;
typedef __s32 Elf64_Sword;
typedef __u32 Elf64_Word;
typedef __u64 Elf64_Xword;
typedef __s64 Elf64_Sxword;
# 125 "/usr/include/linux/elf.h" 3 4
typedef struct dynamic{
  Elf32_Sword d_tag;
  union{
    Elf32_Sword d_val;
    Elf32_Addr d_ptr;
  } d_un;
} Elf32_Dyn;

typedef struct {
  Elf64_Sxword d_tag;
  union {
    Elf64_Xword d_val;
    Elf64_Addr d_ptr;
  } d_un;
} Elf64_Dyn;
# 148 "/usr/include/linux/elf.h" 3 4
typedef struct elf32_rel {
  Elf32_Addr r_offset;
  Elf32_Word r_info;
} Elf32_Rel;

typedef struct elf64_rel {
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
} Elf64_Rel;

typedef struct elf32_rela{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
  Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct elf64_rela {
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
  Elf64_Sxword r_addend;
} Elf64_Rela;

typedef struct elf32_sym{
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Half st_shndx;
} Elf32_Sym;

typedef struct elf64_sym {
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Half st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
} Elf64_Sym;




typedef struct elf32_hdr{
  unsigned char e_ident[16];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct elf64_hdr {
  unsigned char e_ident[16];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;







typedef struct elf32_phdr{
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;

typedef struct elf64_phdr {
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;
# 287 "/usr/include/linux/elf.h" 3 4
typedef struct {
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct elf64_shdr {
  Elf64_Word sh_name;
  Elf64_Word sh_type;
  Elf64_Xword sh_flags;
  Elf64_Addr sh_addr;
  Elf64_Off sh_offset;
  Elf64_Xword sh_size;
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Xword sh_addralign;
  Elf64_Xword sh_entsize;
} Elf64_Shdr;
# 361 "/usr/include/linux/elf.h" 3 4
typedef struct elf32_note {
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
} Elf32_Nhdr;


typedef struct elf64_note {
  Elf64_Word n_namesz;
  Elf64_Word n_descsz;
  Elf64_Word n_type;
} Elf64_Nhdr;
# 384 "/usr/include/linux/elf.h" 3 4
extern Elf64_Dyn _DYNAMIC [];
# 394 "/usr/include/linux/elf.h" 3 4
static __inline__ int elf_coredump_extra_notes_size(void) { return 0; }
static __inline__ int elf_coredump_extra_notes_write(struct file *file,
   loff_t *foffset) { return 0; }
# 31 "../uts/common/sys/ctf_api.h" 2




# 1 "../uts/common/sys/ctf.h" 1
# 94 "../uts/common/sys/ctf.h"
typedef struct ctf_preamble {
 ushort_t ctp_magic;
 uchar_t ctp_version;
 uchar_t ctp_flags;
} ctf_preamble_t;

typedef struct ctf_header {
 ctf_preamble_t cth_preamble;
 uint_t cth_parlabel;
 uint_t cth_parname;
 uint_t cth_lbloff;
 uint_t cth_objtoff;
 uint_t cth_funcoff;
 uint_t cth_typeoff;
 uint_t cth_stroff;
 uint_t cth_strlen;
} ctf_header_t;







typedef struct ctf_header_v1 {
 ctf_preamble_t cth_preamble;
 uint_t cth_objtoff;
 uint_t cth_funcoff;
 uint_t cth_typeoff;
 uint_t cth_stroff;
 uint_t cth_strlen;
} ctf_header_v1_t;
# 138 "../uts/common/sys/ctf.h"
typedef struct ctf_lblent {
 uint_t ctl_label;
 uint_t ctl_typeidx;
} ctf_lblent_t;

typedef struct ctf_stype {
 uint_t ctt_name;
 ushort_t ctt_info;
 union {
  ushort_t _size;
  ushort_t _type;
 } _u;
} ctf_stype_t;
# 160 "../uts/common/sys/ctf.h"
typedef struct ctf_type {
 uint_t ctt_name;
 ushort_t ctt_info;
 union {
  ushort_t _size;
  ushort_t _type;
 } _u;
 uint_t ctt_lsizehi;
 uint_t ctt_lsizelo;
} ctf_type_t;
# 297 "../uts/common/sys/ctf.h"
typedef struct ctf_array {
 ushort_t cta_contents;
 ushort_t cta_index;
 uint_t cta_nelems;
} ctf_array_t;
# 313 "../uts/common/sys/ctf.h"
typedef struct ctf_member {
 uint_t ctm_name;
 ushort_t ctm_type;
 ushort_t ctm_offset;
} ctf_member_t;

typedef struct ctf_lmember {
 uint_t ctlm_name;
 ushort_t ctlm_type;
 ushort_t ctlm_pad;
 uint_t ctlm_offsethi;
 uint_t ctlm_offsetlo;
} ctf_lmember_t;






typedef struct ctf_enum {
 uint_t cte_name;
 int cte_value;
} ctf_enum_t;
# 36 "../uts/common/sys/ctf_api.h" 2
# 46 "../uts/common/sys/ctf_api.h"
typedef struct ctf_file ctf_file_t;
typedef long ctf_id_t;






typedef struct ctf_sect {
 const char *cts_name;
 ulong_t cts_type;
 ulong_t cts_flags;
 const void *cts_data;
 size_t cts_size;
 size_t cts_entsize;
 off64_t cts_offset;
} ctf_sect_t;






typedef struct ctf_encoding {
 uint_t cte_format;
 uint_t cte_offset;
 uint_t cte_bits;
} ctf_encoding_t;

typedef struct ctf_membinfo {
 ctf_id_t ctm_type;
 ulong_t ctm_offset;
} ctf_membinfo_t;

typedef struct ctf_arinfo {
 ctf_id_t ctr_contents;
 ctf_id_t ctr_index;
 uint_t ctr_nelems;
} ctf_arinfo_t;

typedef struct ctf_funcinfo {
 ctf_id_t ctc_return;
 uint_t ctc_argc;
 uint_t ctc_flags;
} ctf_funcinfo_t;

typedef struct ctf_lblinfo {
 ctf_id_t ctb_typeidx;
} ctf_lblinfo_t;
# 129 "../uts/common/sys/ctf_api.h"
typedef int ctf_visit_f(const char *, ctf_id_t, ulong_t, int, void *);
typedef int ctf_member_f(const char *, ctf_id_t, ulong_t, void *);
typedef int ctf_enum_f(const char *, int, void *);
typedef int ctf_type_f(ctf_id_t, void *);
typedef int ctf_label_f(const char *, const ctf_lblinfo_t *, void *);

extern ctf_file_t *ctf_bufopen(const ctf_sect_t *, const ctf_sect_t *,
    const ctf_sect_t *, int *);
extern ctf_file_t *ctf_fdopen(int, int *);
extern ctf_file_t *ctf_open(const char *, int *);
extern ctf_file_t *ctf_create(int *);
extern void ctf_close(ctf_file_t *);

extern ctf_file_t *ctf_parent_file(ctf_file_t *);
extern const char *ctf_parent_name(ctf_file_t *);

extern int ctf_import(ctf_file_t *, ctf_file_t *);
extern int ctf_setmodel(ctf_file_t *, int);
extern int ctf_getmodel(ctf_file_t *);

extern int ctf_errno(ctf_file_t *);
extern const char *ctf_errmsg(int);
extern int ctf_version(int);

extern int ctf_func_info(ctf_file_t *, ulong_t, ctf_funcinfo_t *);
extern int ctf_func_args(ctf_file_t *, ulong_t, uint_t, ctf_id_t *);

extern ctf_id_t ctf_lookup_by_name(ctf_file_t *, const char *);
extern ctf_id_t ctf_lookup_by_symbol(ctf_file_t *, ulong_t);

extern ctf_id_t ctf_type_resolve(ctf_file_t *, ctf_id_t);
extern char *ctf_type_name(ctf_file_t *, ctf_id_t, char *, size_t);
extern ssize_t ctf_type_size(ctf_file_t *, ctf_id_t);
extern ssize_t ctf_type_align(ctf_file_t *, ctf_id_t);
extern int ctf_type_kind(ctf_file_t *, ctf_id_t);
extern ctf_id_t ctf_type_reference(ctf_file_t *, ctf_id_t);
extern ctf_id_t ctf_type_pointer(ctf_file_t *, ctf_id_t);
extern int ctf_type_encoding(ctf_file_t *, ctf_id_t, ctf_encoding_t *);
extern int ctf_type_visit(ctf_file_t *, ctf_id_t, ctf_visit_f *, void *);
extern int ctf_type_cmp(ctf_file_t *, ctf_id_t, ctf_file_t *, ctf_id_t);
extern int ctf_type_compat(ctf_file_t *, ctf_id_t, ctf_file_t *, ctf_id_t);

extern int ctf_member_info(ctf_file_t *, ctf_id_t, const char *,
    ctf_membinfo_t *);
extern int ctf_array_info(ctf_file_t *, ctf_id_t, ctf_arinfo_t *);

extern const char *ctf_enum_name(ctf_file_t *, ctf_id_t, int);
extern int ctf_enum_value(ctf_file_t *, ctf_id_t, const char *, int *);

extern const char *ctf_label_topmost(ctf_file_t *);
extern int ctf_label_info(ctf_file_t *, const char *, ctf_lblinfo_t *);

extern int ctf_member_iter(ctf_file_t *, ctf_id_t, ctf_member_f *, void *);
extern int ctf_enum_iter(ctf_file_t *, ctf_id_t, ctf_enum_f *, void *);
extern int ctf_type_iter(ctf_file_t *, ctf_type_f *, void *);
extern int ctf_label_iter(ctf_file_t *, ctf_label_f *, void *);

extern ctf_id_t ctf_add_array(ctf_file_t *, uint_t, const ctf_arinfo_t *);
extern ctf_id_t ctf_add_const(ctf_file_t *, uint_t, ctf_id_t);
extern ctf_id_t ctf_add_enum(ctf_file_t *, uint_t, const char *);
extern ctf_id_t ctf_add_float(ctf_file_t *, uint_t,
    const char *, const ctf_encoding_t *);
extern ctf_id_t ctf_add_forward(ctf_file_t *, uint_t, const char *, uint_t);
extern ctf_id_t ctf_add_function(ctf_file_t *, uint_t,
    const ctf_funcinfo_t *, const ctf_id_t *);
extern ctf_id_t ctf_add_integer(ctf_file_t *, uint_t,
    const char *, const ctf_encoding_t *);
extern ctf_id_t ctf_add_pointer(ctf_file_t *, uint_t, ctf_id_t);
extern ctf_id_t ctf_add_type(ctf_file_t *, ctf_file_t *, ctf_id_t);
extern ctf_id_t ctf_add_typedef(ctf_file_t *, uint_t, const char *, ctf_id_t);
extern ctf_id_t ctf_add_restrict(ctf_file_t *, uint_t, ctf_id_t);
extern ctf_id_t ctf_add_struct(ctf_file_t *, uint_t, const char *);
extern ctf_id_t ctf_add_union(ctf_file_t *, uint_t, const char *);
extern ctf_id_t ctf_add_volatile(ctf_file_t *, uint_t, ctf_id_t);

extern int ctf_add_enumerator(ctf_file_t *, ctf_id_t, const char *, int);
extern int ctf_add_member(ctf_file_t *, ctf_id_t, const char *, ctf_id_t);

extern int ctf_set_array(ctf_file_t *, ctf_id_t, const ctf_arinfo_t *);

extern int ctf_update(ctf_file_t *);
extern int ctf_discard(ctf_file_t *);
extern int ctf_write(ctf_file_t *, int);
# 46 "../libctf/libctf.h" 2
# 54 "../libctf/libctf.h"
extern int _libctf_debug;
# 42 "../libproc/common/libproc.h" 2

# 1 "../linux/sys/statvfs.h" 1



# 1 "/usr/include/sys/statvfs.h" 1
# 26 "/usr/include/sys/statvfs.h"
# 1 "/usr/include/bits/statvfs.h" 1 3 4
# 29 "/usr/include/bits/statvfs.h" 3 4
struct statvfs
  {
    unsigned long int f_bsize;
    unsigned long int f_frsize;
# 41 "/usr/include/bits/statvfs.h" 3 4
    __fsblkcnt64_t f_blocks;
    __fsblkcnt64_t f_bfree;
    __fsblkcnt64_t f_bavail;
    __fsfilcnt64_t f_files;
    __fsfilcnt64_t f_ffree;
    __fsfilcnt64_t f_favail;

    unsigned long int f_fsid;



    unsigned long int f_flag;
    unsigned long int f_namemax;
    int __f_spare[6];
  };


struct statvfs64
  {
    unsigned long int f_bsize;
    unsigned long int f_frsize;
    __fsblkcnt64_t f_blocks;
    __fsblkcnt64_t f_bfree;
    __fsblkcnt64_t f_bavail;
    __fsfilcnt64_t f_files;
    __fsfilcnt64_t f_ffree;
    __fsfilcnt64_t f_favail;
    unsigned long int f_fsid;



    unsigned long int f_flag;
    unsigned long int f_namemax;
    int __f_spare[6];
  };




enum
{
  ST_RDONLY = 1,

  ST_NOSUID = 2
# 109 "/usr/include/bits/statvfs.h" 3 4
};
# 27 "/usr/include/sys/statvfs.h" 2
# 39 "/usr/include/sys/statvfs.h"
typedef __fsblkcnt64_t fsblkcnt_t;



typedef __fsfilcnt64_t fsfilcnt_t;





# 57 "/usr/include/sys/statvfs.h"
extern int statvfs (__const char *__restrict __file, struct statvfs *__restrict __buf) __asm__ ("" "statvfs64") __attribute__ ((__nothrow__))


     __attribute__ ((__nonnull__ (1, 2)));





extern int statvfs64 (__const char *__restrict __file,
        struct statvfs64 *__restrict __buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
# 78 "/usr/include/sys/statvfs.h"
extern int fstatvfs (int __fildes, struct statvfs *__buf) __asm__ ("" "fstatvfs64") __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));






extern int fstatvfs64 (int __fildes, struct statvfs64 *__buf)
     __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (2)));



# 5 "../linux/sys/statvfs.h" 2

typedef struct statvfs64 statvfs_t;
typedef struct statvfs statvfs32_t;
# 44 "../libproc/common/libproc.h" 2
# 1 "../linux/sys/auxv.h" 1



typedef struct
{
        int a_type;
        union {
                long a_val;

                void *a_ptr;



                void (*a_fcn)();
        } a_un;
} auxv_t;

typedef struct {
        int32_t a_type;
        union {
                int32_t a_val;
                caddr32_t a_ptr;
                caddr32_t a_fcn;
        } a_un;
} auxv32_t;
# 45 "../libproc/common/libproc.h" 2

# 1 "/usr/include/sys/socket.h" 1 3 4
# 26 "/usr/include/sys/socket.h" 3 4


# 1 "/usr/include/sys/uio.h" 1 3 4
# 26 "/usr/include/sys/uio.h" 3 4



# 1 "/usr/include/bits/uio.h" 1 3 4
# 44 "/usr/include/bits/uio.h" 3 4
struct iovec
  {
    void *iov_base;
    size_t iov_len;
  };
# 30 "/usr/include/sys/uio.h" 2 3 4
# 40 "/usr/include/sys/uio.h" 3 4
extern ssize_t readv (int __fd, __const struct iovec *__iovec, int __count);
# 50 "/usr/include/sys/uio.h" 3 4
extern ssize_t writev (int __fd, __const struct iovec *__iovec, int __count);


# 29 "/usr/include/sys/socket.h" 2 3 4

# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 31 "/usr/include/sys/socket.h" 2 3 4





# 1 "/usr/include/bits/socket.h" 1 3 4
# 30 "/usr/include/bits/socket.h" 3 4
# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stddef.h" 1 3 4
# 31 "/usr/include/bits/socket.h" 2 3 4

# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/limits.h" 1 3 4
# 11 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/limits.h" 3 4
# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/syslimits.h" 1 3 4






# 1 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/limits.h" 1 3 4
# 122 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/limits.h" 3 4
# 1 "/usr/include/limits.h" 1 3 4
# 145 "/usr/include/limits.h" 3 4
# 1 "/usr/include/bits/posix1_lim.h" 1 3 4
# 153 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/local_lim.h" 1 3 4
# 36 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/linux/limits.h" 1 3 4
# 37 "/usr/include/bits/local_lim.h" 2 3 4
# 154 "/usr/include/bits/posix1_lim.h" 2 3 4
# 146 "/usr/include/limits.h" 2 3 4



# 1 "/usr/include/bits/posix2_lim.h" 1 3 4
# 150 "/usr/include/limits.h" 2 3 4
# 123 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/limits.h" 2 3 4
# 8 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/syslimits.h" 2 3 4
# 12 "/usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/limits.h" 2 3 4
# 33 "/usr/include/bits/socket.h" 2 3 4
# 42 "/usr/include/bits/socket.h" 3 4
enum __socket_type
{
  SOCK_STREAM = 1,


  SOCK_DGRAM = 2,


  SOCK_RAW = 3,

  SOCK_RDM = 4,

  SOCK_SEQPACKET = 5,


  SOCK_PACKET = 10



};
# 147 "/usr/include/bits/socket.h" 3 4
# 1 "/usr/include/bits/sockaddr.h" 1 3 4
# 29 "/usr/include/bits/sockaddr.h" 3 4
typedef unsigned short int sa_family_t;
# 148 "/usr/include/bits/socket.h" 2 3 4


struct sockaddr
  {
    sa_family_t sa_family;
    char sa_data[14];
  };
# 167 "/usr/include/bits/socket.h" 3 4
struct sockaddr_storage
  {
    sa_family_t ss_family;
    __uint64_t __ss_align;
    char __ss_padding[(128 - (2 * sizeof (__uint64_t)))];
  };



enum
  {
    MSG_OOB = 0x01,

    MSG_PEEK = 0x02,

    MSG_DONTROUTE = 0x04,






    MSG_CTRUNC = 0x08,

    MSG_PROXY = 0x10,

    MSG_TRUNC = 0x20,

    MSG_DONTWAIT = 0x40,

    MSG_EOR = 0x80,

    MSG_WAITALL = 0x100,

    MSG_FIN = 0x200,

    MSG_SYN = 0x400,

    MSG_CONFIRM = 0x800,

    MSG_RST = 0x1000,

    MSG_ERRQUEUE = 0x2000,

    MSG_NOSIGNAL = 0x4000,

    MSG_MORE = 0x8000,


    MSG_CMSG_CLOEXEC = 0x40000000



  };




struct msghdr
  {
    void *msg_name;
    socklen_t msg_namelen;

    struct iovec *msg_iov;
    size_t msg_iovlen;

    void *msg_control;
    size_t msg_controllen;




    int msg_flags;
  };


struct cmsghdr
  {
    size_t cmsg_len;




    int cmsg_level;
    int cmsg_type;

    __extension__ unsigned char __cmsg_data [];

  };
# 273 "/usr/include/bits/socket.h" 3 4
extern struct cmsghdr *__cmsg_nxthdr (struct msghdr *__mhdr,
          struct cmsghdr *__cmsg) __attribute__ ((__nothrow__));
# 300 "/usr/include/bits/socket.h" 3 4
enum
  {
    SCM_RIGHTS = 0x01


    , SCM_CREDENTIALS = 0x02


  };



struct ucred
{
  pid_t pid;
  uid_t uid;
  gid_t gid;
};


# 1 "/usr/include/asm/socket.h" 1 3 4



# 1 "/usr/include/asm/sockios.h" 1 3 4
# 5 "/usr/include/asm/socket.h" 2 3 4
# 321 "/usr/include/bits/socket.h" 2 3 4



struct linger
  {
    int l_onoff;
    int l_linger;
  };
# 37 "/usr/include/sys/socket.h" 2 3 4




struct osockaddr
  {
    unsigned short int sa_family;
    unsigned char sa_data[14];
  };




enum
{
  SHUT_RD = 0,

  SHUT_WR,

  SHUT_RDWR

};
# 101 "/usr/include/sys/socket.h" 3 4
extern int socket (int __domain, int __type, int __protocol) __attribute__ ((__nothrow__));





extern int socketpair (int __domain, int __type, int __protocol,
         int __fds[2]) __attribute__ ((__nothrow__));


extern int bind (int __fd, __const struct sockaddr * __addr, socklen_t __len)
     __attribute__ ((__nothrow__));


extern int getsockname (int __fd, struct sockaddr *__restrict __addr,
   socklen_t *__restrict __len) __attribute__ ((__nothrow__));
# 125 "/usr/include/sys/socket.h" 3 4
extern int connect (int __fd, __const struct sockaddr * __addr, socklen_t __len);



extern int getpeername (int __fd, struct sockaddr *__restrict __addr,
   socklen_t *__restrict __len) __attribute__ ((__nothrow__));






extern ssize_t send (int __fd, __const void *__buf, size_t __n, int __flags);






extern ssize_t recv (int __fd, void *__buf, size_t __n, int __flags);






extern ssize_t sendto (int __fd, __const void *__buf, size_t __n,
         int __flags, __const struct sockaddr * __addr,
         socklen_t __addr_len);
# 162 "/usr/include/sys/socket.h" 3 4
extern ssize_t recvfrom (int __fd, void *__restrict __buf, size_t __n,
    int __flags, struct sockaddr *__restrict __addr,
    socklen_t *__restrict __addr_len);







extern ssize_t sendmsg (int __fd, __const struct msghdr *__message,
   int __flags);






extern ssize_t recvmsg (int __fd, struct msghdr *__message, int __flags);





extern int getsockopt (int __fd, int __level, int __optname,
         void *__restrict __optval,
         socklen_t *__restrict __optlen) __attribute__ ((__nothrow__));




extern int setsockopt (int __fd, int __level, int __optname,
         __const void *__optval, socklen_t __optlen) __attribute__ ((__nothrow__));





extern int listen (int __fd, int __n) __attribute__ ((__nothrow__));
# 210 "/usr/include/sys/socket.h" 3 4
extern int accept (int __fd, struct sockaddr *__restrict __addr,
     socklen_t *__restrict __addr_len);







extern int shutdown (int __fd, int __how) __attribute__ ((__nothrow__));




extern int sockatmark (int __fd) __attribute__ ((__nothrow__));







extern int isfdtype (int __fd, int __fdtype) __attribute__ ((__nothrow__));
# 241 "/usr/include/sys/socket.h" 3 4

# 47 "../libproc/common/libproc.h" 2
# 1 "/usr/include/sys/utsname.h" 1 3 4
# 28 "/usr/include/sys/utsname.h" 3 4


# 1 "/usr/include/bits/utsname.h" 1 3 4
# 31 "/usr/include/sys/utsname.h" 2 3 4
# 49 "/usr/include/sys/utsname.h" 3 4
struct utsname
  {

    char sysname[65];


    char nodename[65];


    char release[65];

    char version[65];


    char machine[65];






    char __domainname[65];


  };
# 82 "/usr/include/sys/utsname.h" 3 4
extern int uname (struct utsname *__name) __attribute__ ((__nothrow__));



# 48 "../libproc/common/libproc.h" 2
# 1 "../linux/sys/corectl.h" 1
# 49 "../libproc/common/libproc.h" 2

# 1 "../linux/sys/sysi86.h" 1





struct ssd {
        unsigned int sel;
        unsigned int bo;
        unsigned int ls;
        unsigned int acc1;
        unsigned int acc2;
};
# 51 "../libproc/common/libproc.h" 2
# 62 "../libproc/common/libproc.h"
struct ps_prochandle;




struct ps_lwphandle;

extern int _libproc_debug;
# 144 "../libproc/common/libproc.h"
typedef struct {
 long arg_value;
 void *arg_object;
 char arg_type;
 char arg_inout;
 ushort_t arg_size;
} argdes_t;
# 170 "../libproc/common/libproc.h"
extern struct ps_prochandle *Pcreate(const char *, char *const *,
    int *, char *, size_t);
extern struct ps_prochandle *Pxcreate(const char *, char *const *,
    char *const *, int *, char *, size_t);

extern const char *Pcreate_error(int);

extern struct ps_prochandle *Pgrab(pid_t, int, int *);
extern struct ps_prochandle *Pgrab_core(const char *, const char *, int, int *);
extern struct ps_prochandle *Pfgrab_core(int, const char *, int *);
extern struct ps_prochandle *Pgrab_file(const char *, int *);
extern const char *Pgrab_error(int);

extern int Preopen(struct ps_prochandle *);
extern void Prelease(struct ps_prochandle *, int);
extern void Pfree(struct ps_prochandle *);

extern int Pasfd(struct ps_prochandle *);
extern int Pctlfd(struct ps_prochandle *);
extern int Pcreate_agent(struct ps_prochandle *);
extern void Pdestroy_agent(struct ps_prochandle *);
extern int Pstopstatus(struct ps_prochandle *, long, uint_t);
extern int Pwait(struct ps_prochandle *, uint_t);
extern int Pstop(struct ps_prochandle *, uint_t);
extern int Pdstop(struct ps_prochandle *);
extern int Pstate(struct ps_prochandle *);
extern const psinfo_t *Ppsinfo(struct ps_prochandle *);
extern const pstatus_t *Pstatus(struct ps_prochandle *);
extern int Pcred(struct ps_prochandle *, prcred_t *, int);
extern int Psetcred(struct ps_prochandle *, const prcred_t *);
extern ssize_t Ppriv(struct ps_prochandle *, prpriv_t *, size_t);
extern int Psetpriv(struct ps_prochandle *, prpriv_t *);
extern void *Pprivinfo(struct ps_prochandle *);
extern int Psetzoneid(struct ps_prochandle *, zoneid_t);
extern int Pgetareg(struct ps_prochandle *, int, prgreg_t *);
extern int Pputareg(struct ps_prochandle *, int, prgreg_t);
extern int Psetrun(struct ps_prochandle *, int, int);
extern ssize_t Pread(struct ps_prochandle *, void *, size_t, uintptr_t);
extern ssize_t Pread_string(struct ps_prochandle *, char *, size_t, uintptr_t);
extern ssize_t Pwrite(struct ps_prochandle *, const void *, size_t, uintptr_t);
extern int Pclearsig(struct ps_prochandle *);
extern int Pclearfault(struct ps_prochandle *);
extern int Psetbkpt(struct ps_prochandle *, uintptr_t, ulong_t *);
extern int Pdelbkpt(struct ps_prochandle *, uintptr_t, ulong_t);
extern int Pxecbkpt(struct ps_prochandle *, ulong_t);
extern int Psetwapt(struct ps_prochandle *, const prwatch_t *);
extern int Pdelwapt(struct ps_prochandle *, const prwatch_t *);
extern int Pxecwapt(struct ps_prochandle *, const prwatch_t *);
extern int Psetflags(struct ps_prochandle *, long);
extern int Punsetflags(struct ps_prochandle *, long);
extern int Psignal(struct ps_prochandle *, int, int);
extern int Pfault(struct ps_prochandle *, int, int);
extern int Psysentry(struct ps_prochandle *, int, int);
extern int Psysexit(struct ps_prochandle *, int, int);
extern void Psetsignal(struct ps_prochandle *, const sigset_t *);
extern void Psetfault(struct ps_prochandle *, const fltset_t *);
extern void Psetsysentry(struct ps_prochandle *, const sysset_t *);
extern void Psetsysexit(struct ps_prochandle *, const sysset_t *);

extern void Psync(struct ps_prochandle *);
extern int Psyscall(struct ps_prochandle *, sysret_t *,
   int, uint_t, argdes_t *);
extern int Pisprocdir(struct ps_prochandle *, const char *);




extern struct ps_lwphandle *Lgrab(struct ps_prochandle *, lwpid_t, int *);
extern const char *Lgrab_error(int);

extern struct ps_prochandle *Lprochandle(struct ps_lwphandle *);
extern void Lfree(struct ps_lwphandle *);

extern int Lctlfd(struct ps_lwphandle *);
extern int Lwait(struct ps_lwphandle *, uint_t);
extern int Lstop(struct ps_lwphandle *, uint_t);
extern int Ldstop(struct ps_lwphandle *);
extern int Lstate(struct ps_lwphandle *);
extern const lwpsinfo_t *Lpsinfo(struct ps_lwphandle *);
extern const lwpstatus_t *Lstatus(struct ps_lwphandle *);
extern int Lgetareg(struct ps_lwphandle *, int, prgreg_t *);
extern int Lputareg(struct ps_lwphandle *, int, prgreg_t);
extern int Lsetrun(struct ps_lwphandle *, int, int);
extern int Lclearsig(struct ps_lwphandle *);
extern int Lclearfault(struct ps_lwphandle *);
extern int Lxecbkpt(struct ps_lwphandle *, ulong_t);
extern int Lxecwapt(struct ps_lwphandle *, const prwatch_t *);
extern void Lsync(struct ps_lwphandle *);

extern int Lstack(struct ps_lwphandle *, stack_t *);
extern int Lmain_stack(struct ps_lwphandle *, stack_t *);
extern int Lalt_stack(struct ps_lwphandle *, stack_t *);




extern int pr_open(struct ps_prochandle *, const char *, int, mode_t);
extern int pr_creat(struct ps_prochandle *, const char *, mode_t);
extern int pr_close(struct ps_prochandle *, int);
extern int pr_access(struct ps_prochandle *, const char *, int);
extern int pr_door_info(struct ps_prochandle *, int, struct door_info *);
extern void *pr_mmap(struct ps_prochandle *,
   void *, size_t, int, int, int, off_t);
extern void *pr_zmap(struct ps_prochandle *,
   void *, size_t, int, int);
extern int pr_munmap(struct ps_prochandle *, void *, size_t);
extern int pr_memcntl(struct ps_prochandle *,
   caddr_t, size_t, int, caddr_t, int, int);
extern int pr_meminfo(struct ps_prochandle *, const uint64_t *addrs,
   int addr_count, const uint_t *info, int info_count,
   uint64_t *outdata, uint_t *validity);
extern int pr_sigaction(struct ps_prochandle *,
   int, const struct sigaction *, struct sigaction *);
extern int pr_getitimer(struct ps_prochandle *,
   int, struct itimerval *);
extern int pr_setitimer(struct ps_prochandle *,
   int, const struct itimerval *, struct itimerval *);
extern int pr_ioctl(struct ps_prochandle *, int, int, void *, size_t);
extern int pr_fcntl(struct ps_prochandle *, int, int, void *);
extern int pr_stat(struct ps_prochandle *, const char *, struct stat *);
extern int pr_lstat(struct ps_prochandle *, const char *, struct stat *);
extern int pr_fstat(struct ps_prochandle *, int, struct stat *);
extern int pr_stat64(struct ps_prochandle *, const char *,
   struct stat64 *);
extern int pr_lstat64(struct ps_prochandle *, const char *,
   struct stat64 *);
extern int pr_fstat64(struct ps_prochandle *, int, struct stat64 *);
extern int pr_statvfs(struct ps_prochandle *, const char *, statvfs_t *);
extern int pr_fstatvfs(struct ps_prochandle *, int, statvfs_t *);
extern projid_t pr_getprojid(struct ps_prochandle *Pr);
extern taskid_t pr_gettaskid(struct ps_prochandle *Pr);
extern taskid_t pr_settaskid(struct ps_prochandle *Pr, projid_t project,
   int flags);
extern zoneid_t pr_getzoneid(struct ps_prochandle *Pr);
extern int pr_getrctl(struct ps_prochandle *,
   const char *, rctlblk_t *, rctlblk_t *, int);
extern int pr_setrctl(struct ps_prochandle *,
   const char *, rctlblk_t *, rctlblk_t *, int);
extern int pr_getrlimit(struct ps_prochandle *,
   int, struct rlimit *);
extern int pr_setrlimit(struct ps_prochandle *,
   int, const struct rlimit *);






extern int pr_lwp_exit(struct ps_prochandle *);
extern int pr_exit(struct ps_prochandle *, int);
extern int pr_waitid(struct ps_prochandle *,
   idtype_t, id_t, siginfo_t *, int);
extern off_t pr_lseek(struct ps_prochandle *, int, off_t, int);
extern offset_t pr_llseek(struct ps_prochandle *, int, offset_t, int);
extern int pr_rename(struct ps_prochandle *, const char *, const char *);
extern int pr_link(struct ps_prochandle *, const char *, const char *);
extern int pr_unlink(struct ps_prochandle *, const char *);
extern int pr_getpeername(struct ps_prochandle *,
   int, struct sockaddr *, socklen_t *);
extern int pr_getsockname(struct ps_prochandle *,
   int, struct sockaddr *, socklen_t *);
extern int pr_getsockopt(struct ps_prochandle *,
   int, int, int, void *, int *);
extern int pr_processor_bind(struct ps_prochandle *,
   idtype_t, id_t, int, int *);
extern int pr_pset_bind(struct ps_prochandle *,
   int, idtype_t, id_t, int *);




extern int Plwp_getregs(struct ps_prochandle *, lwpid_t, prgregset_t);
extern int Plwp_setregs(struct ps_prochandle *, lwpid_t, const prgregset_t);

extern int Plwp_getfpregs(struct ps_prochandle *, lwpid_t, prfpregset_t *);
extern int Plwp_setfpregs(struct ps_prochandle *, lwpid_t,
    const prfpregset_t *);
# 363 "../libproc/common/libproc.h"
extern int Pldt(struct ps_prochandle *, struct ssd *, int);
extern int proc_get_ldt(pid_t, struct ssd *, int);


extern int Plwp_getpsinfo(struct ps_prochandle *, lwpid_t, lwpsinfo_t *);

extern int Plwp_stack(struct ps_prochandle *, lwpid_t, stack_t *);
extern int Plwp_main_stack(struct ps_prochandle *, lwpid_t, stack_t *);
extern int Plwp_alt_stack(struct ps_prochandle *, lwpid_t, stack_t *);




typedef int proc_lwp_f(void *, const lwpstatus_t *);
extern int Plwp_iter(struct ps_prochandle *, proc_lwp_f *, void *);




typedef int proc_lwp_all_f(void *, const lwpstatus_t *, const lwpsinfo_t *);
extern int Plwp_iter_all(struct ps_prochandle *, proc_lwp_all_f *, void *);




typedef int proc_walk_f(psinfo_t *, lwpsinfo_t *, void *);
extern int proc_walk(proc_walk_f *, void *, int);







extern int proc_lwp_in_set(const char *, lwpid_t);
extern int proc_lwp_range_valid(const char *);
# 428 "../libproc/common/libproc.h"
extern int Plookup_by_name(struct ps_prochandle *,
    const char *, const char *, GElf_Sym *);

extern int Plookup_by_addr(struct ps_prochandle *,
    uintptr_t, char *, size_t, GElf_Sym *);

typedef struct prsyminfo {
 const char *prs_object;
 const char *prs_name;
 Lmid_t prs_lmid;
 uint_t prs_id;
 uint_t prs_table;
} prsyminfo_t;

extern int Pxlookup_by_name(struct ps_prochandle *,
    Lmid_t, const char *, const char *, GElf_Sym *, prsyminfo_t *);

extern int Pxlookup_by_addr(struct ps_prochandle *,
    uintptr_t, char *, size_t, GElf_Sym *, prsyminfo_t *);

typedef int proc_map_f(void *, const prmap_t *, const char *);

extern int Pmapping_iter(struct ps_prochandle *, proc_map_f *, void *);
extern int Pobject_iter(struct ps_prochandle *, proc_map_f *, void *);

extern const prmap_t *Paddr_to_map(struct ps_prochandle *, uintptr_t);
extern const prmap_t *Paddr_to_text_map(struct ps_prochandle *, uintptr_t);
extern const prmap_t *Pname_to_map(struct ps_prochandle *, const char *);
extern const prmap_t *Plmid_to_map(struct ps_prochandle *,
    Lmid_t, const char *);

extern const rd_loadobj_t *Paddr_to_loadobj(struct ps_prochandle *, uintptr_t);
extern const rd_loadobj_t *Pname_to_loadobj(struct ps_prochandle *,
    const char *);
extern const rd_loadobj_t *Plmid_to_loadobj(struct ps_prochandle *,
    Lmid_t, const char *);

extern ctf_file_t *Paddr_to_ctf(struct ps_prochandle *, uintptr_t);
extern ctf_file_t *Pname_to_ctf(struct ps_prochandle *, const char *);

extern char *Pplatform(struct ps_prochandle *, char *, size_t);
extern int Puname(struct ps_prochandle *, struct utsname *);
extern char *Pzonename(struct ps_prochandle *, char *, size_t);

extern char *Pexecname(struct ps_prochandle *, char *, size_t);
extern char *Pobjname(struct ps_prochandle *, uintptr_t, char *, size_t);
extern int Plmid(struct ps_prochandle *, uintptr_t, Lmid_t *);

typedef int proc_env_f(void *, struct ps_prochandle *, uintptr_t, const char *);
extern int Penv_iter(struct ps_prochandle *, proc_env_f *, void *);
extern char *Pgetenv(struct ps_prochandle *, const char *, char *, size_t);
extern long Pgetauxval(struct ps_prochandle *, int);
extern const auxv_t *Pgetauxvec(struct ps_prochandle *);





typedef int proc_sym_f(void *, const GElf_Sym *, const char *);
typedef int proc_xsym_f(void *, const GElf_Sym *, const char *,
    const prsyminfo_t *);

extern int Psymbol_iter(struct ps_prochandle *,
    const char *, int, int, proc_sym_f *, void *);
extern int Psymbol_iter_by_addr(struct ps_prochandle *,
    const char *, int, int, proc_sym_f *, void *);
extern int Psymbol_iter_by_name(struct ps_prochandle *,
    const char *, int, int, proc_sym_f *, void *);

extern int Psymbol_iter_by_lmid(struct ps_prochandle *,
    Lmid_t, const char *, int, int, proc_sym_f *, void *);

extern int Pxsymbol_iter(struct ps_prochandle *,
    Lmid_t, const char *, int, int, proc_xsym_f *, void *);
# 529 "../libproc/common/libproc.h"
extern rd_agent_t *Prd_agent(struct ps_prochandle *);







extern void Pupdate_maps(struct ps_prochandle *);
extern void Pupdate_syms(struct ps_prochandle *);
# 552 "../libproc/common/libproc.h"
extern void Preset_maps(struct ps_prochandle *);






extern const char *Ppltdest(struct ps_prochandle *, uintptr_t);




extern int Pissyscall_prev(struct ps_prochandle *, uintptr_t, uintptr_t *);




typedef int proc_stack_f(void *, prgregset_t, uint_t, const long *);

extern int Pstack_iter(struct ps_prochandle *,
    const prgregset_t, proc_stack_f *, void *);
# 586 "../libproc/common/libproc.h"
extern void Perror_printf(struct ps_prochandle *P, const char *format, ...);
extern void Pcreate_callback(struct ps_prochandle *);





extern void proc_unctrl_psinfo(psinfo_t *);
# 605 "../libproc/common/libproc.h"
extern struct ps_prochandle *proc_arg_grab(const char *, int, int, int *);
extern struct ps_prochandle *proc_arg_xgrab(const char *, const char *, int,
    int, int *, const char **);
extern pid_t proc_arg_psinfo(const char *, int, psinfo_t *, int *);
extern pid_t proc_arg_xpsinfo(const char *, int, psinfo_t *, int *,
    const char **);





extern int proc_get_auxv(pid_t, auxv_t *, int);
extern int proc_get_cred(pid_t, prcred_t *, int);
extern prpriv_t *proc_get_priv(pid_t);
extern int proc_get_psinfo(pid_t, psinfo_t *);
extern int proc_get_status(pid_t, pstatus_t *);
# 629 "../libproc/common/libproc.h"
extern char *proc_fltname(int, char *, size_t);
extern char *proc_signame(int, char *, size_t);
extern char *proc_sysname(int, char *, size_t);





extern int proc_str2flt(const char *, int *);
extern int proc_str2sig(const char *, int *);
extern int proc_str2sys(const char *, int *);







extern char *proc_fltset2str(const fltset_t *, const char *, int,
    char *, size_t);
extern char *proc_sigset2str(const sigset_t *, const char *, int,
    char *, size_t);
extern char *proc_sysset2str(const sysset_t *, const char *, int,
    char *, size_t);

extern int Pgcore(struct ps_prochandle *, const char *, core_content_t);
extern int Pfgcore(struct ps_prochandle *, int, core_content_t);
extern core_content_t Pcontent(struct ps_prochandle *);






extern char *proc_str2fltset(const char *, const char *, int, fltset_t *);
extern char *proc_str2sigset(const char *, const char *, int, sigset_t *);
extern char *proc_str2sysset(const char *, const char *, int, sysset_t *);






extern int proc_str2content(const char *, core_content_t *);
extern int proc_content2str(core_content_t, char *, size_t);






extern int proc_initstdio(void);
extern int proc_flushstdio(void);
extern int proc_finistdio(void);
# 20 "proc_names.c" 2

static const char *
rawfltname(int flt)
{
 const char *name;

 switch (flt) {
 case 1: name = "FLTILL"; break;
 case 2: name = "FLTPRIV"; break;
 case 3: name = "FLTBPT"; break;
 case 4: name = "FLTTRACE"; break;
 case 5: name = "FLTACCESS"; break;
 case 6: name = "FLTBOUNDS"; break;
 case 7: name = "FLTIOVF"; break;
 case 8: name = "FLTIZDIV"; break;
 case 9: name = "FLTFPE"; break;
 case 10: name = "FLTSTACK"; break;
 case 11: name = "FLTPAGE"; break;
 case 12: name = "FLTWATCH"; break;
 case 13: name = "FLTCPCOVF"; break;
 default: name = ((void *)0); break;
 }

 return (name);
}





char *
proc_fltname(int flt, char *buf, size_t bufsz)
{
 const char *name = rawfltname(flt);
 size_t len;

 if (bufsz == 0)
  return (((void *)0));

 if (name != ((void *)0)) {
  len = strlen(name);
  (void) strncpy(buf, name, bufsz);
 } else {
  len = snprintf(buf, bufsz, "FLT#%d", flt);
 }

 if (len >= bufsz)
  buf[bufsz-1] = '\0';

 return (buf);
}

int sig2str(int sig, char *buf)
{
 strcpy(buf, strsignal(sig));
 return 1;
}




char *
proc_signame(int sig, char *buf, size_t bufsz)
{
 char name[32 +4];
 size_t len;

 if (bufsz == 0)
  return (((void *)0));


 (void) strcpy(name, "SIG");

 if (sig2str(sig, name+3) == 0) {
  len = strlen(name);
  (void) strncpy(buf, name, bufsz);
 } else {
  len = snprintf(buf, bufsz, "SIG#%d", sig);
 }

 if (len >= bufsz)
  buf[bufsz-1] = '\0';

 return (buf);
}

static const char *const systable[] = {
 ((void *)0),
 "_exit",
 "forkall",
 "read",
 "write",
 "open",
 "close",
 "wait",
 "creat",
 "link",
 "unlink",
 "exec",
 "chdir",
 "time",
 "mknod",
 "chmod",
 "chown",
 "brk",
 "stat",
 "lseek",
 "getpid",
 "mount",
 "umount",
 "setuid",
 "getuid",
 "stime",
 "ptrace",
 "alarm",
 "fstat",
 "pause",
 "utime",
 "stty",
 "gtty",
 "access",
 "nice",
 "statfs",
 "sync",
 "kill",
 "fstatfs",
 "pgrpsys",
 ((void *)0),
 "dup",
 "pipe",
 "times",
 "profil",
 "plock",
 "setgid",
 "getgid",
 "signal",
 "msgsys",
 "sysi86",
 "acct",
 "shmsys",
 "semsys",
 "ioctl",
 "uadmin",
 "uexch",
 "utssys",
 "fdsync",
 "execve",
 "umask",
 "chroot",
 "fcntl",
 "ulimit",


 ((void *)0),
 ((void *)0),
 ((void *)0),
 ((void *)0),
 ((void *)0),
 ((void *)0),

 "tasksys",
 "acctctl",
 "exacctsys",
 "getpagesizes",
 "rctlsys",
 "issetugid",
 "fsat",
 "lwp_park",
 "sendfilev",
 "rmdir",
 "mkdir",
 "getdents",
 "privsys",
 ((void *)0),
 "sysfs",
 "getmsg",
 "putmsg",
 "poll",
 "lstat",
 "symlink",
 "readlink",
 "setgroups",
 "getgroups",
 "fchmod",
 "fchown",
 "sigprocmask",
 "sigsuspend",
 "sigaltstack",
 "sigaction",
 "sigpending",
 "context",
 "evsys",
 "evtrapret",
 "statvfs",
 "fstatvfs",
 ((void *)0),
 "nfssys",
 "waitid",
 "sigsendsys",
 "hrtsys",
 "acancel",
 "async",
 "priocntlsys",
 "pathconf",
 "mincore",
 "mmap",
 "mprotect",
 "munmap",
 "fpathconf",
 "vfork",
 "fchdir",
 "readv",
 "writev",
 "xstat",
 "lxstat",
 "fxstat",
 "xmknod",
 "clocal",
 "setrlimit",
 "getrlimit",
 "lchown",
 "memcntl",
 "getpmsg",
 "putpmsg",
 "rename",
 "uname",
 "setegid",
 "sysconfig",
 "adjtime",
 "systeminfo",
 ((void *)0),
 "seteuid",
 "vtrace",
 "fork1",
 "sigtimedwait",
 "lwp_info",
 "yield",
 "lwp_sema_wait",
 "lwp_sema_post",
 "lwp_sema_trywait",
 ((void *)0),
 ((void *)0),
 "modctl",
 "fchroot",
 "utimes",
 "vhangup",
 "gettimeofday",
 "getitimer",
 "setitimer",
 "lwp_create",
 "lwp_exit",
 "lwp_suspend",
 "lwp_continue",
 "lwp_kill",
 "lwp_self",
 "lwp_sigmask",
 ((void *)0),
 "lwp_wait",
 "lwp_mutex_unlock",
 "lwp_mutex_lock",
 "lwp_cond_wait",
 "lwp_cond_signal",
 "lwp_cond_broadcast",
 "pread",
 "pwrite",
 "llseek",
 "inst_sync",
 ((void *)0),
 "kaio",
 "cpc",
 "lgrpsys",
 "rusagesys",
 "portfs",
 "pollsys",
 ((void *)0),
 "acl",
 "auditsys",
 "processor_bind",
 "processor_info",
 "p_online",
 "sigqueue",
 "clock_gettime",
 "clock_settime",
 "clock_getres",
 "timer_create",
 "timer_delete",
 "timer_settime",
 "timer_gettime",
 "timer_getoverrun",
 "nanosleep",
 "facl",
 "door",
 "setreuid",
 "setregid",
 "install_utrap",
 "signotify",
 "schedctl",
 "pset",
 ((void *)0),
 "resolvepath",
 "lwp_mutex_timedlock",
 "lwp_sema_timedwait",
 "lwp_rwlock_sys",
 "getdents64",
 "mmap64",
 "stat64",
 "lstat64",
 "fstat64",
 "statvfs64",
 "fstatvfs64",
 "setrlimit64",
 "getrlimit64",
 "pread64",
 "pwrite64",
 "creat64",
 "open64",
 "rpcmod",
 "zone",
 "autofssys",
 ((void *)0),
 "so_socket",
 "so_socketpair",
 "bind",
 "listen",
 "accept",
 "connect",
 "shutdown",
 "recv",
 "recvfrom",
 "recvmsg",
 "send",
 "sendmsg",
 "sendto",
 "getpeername",
 "getsockname",
 "getsockopt",
 "setsockopt",
 "sockconfig",
 "ntp_gettime",
 "ntp_adjtime",
 "lwp_mutex_unlock",
 "lwp_mutex_trylock",
 "lwp_mutex_init",
 "cladm",
 ((void *)0),
 "umount2"
};
# 375 "proc_names.c"
char *
proc_sysname(int sys, char *buf, size_t bufsz)
{
 const char *name;
 size_t len;

 if (bufsz == 0)
  return (((void *)0));

 if (sys >= 0 && sys < (sizeof (systable) / sizeof (systable[0])))
  name = systable[sys];
 else
  name = ((void *)0);

 if (name != ((void *)0)) {
  len = strlen(name);
  (void) strncpy(buf, name, bufsz);
 } else {
  len = snprintf(buf, bufsz, "SYS#%d", sys);
 }

 if (len >= bufsz)
  buf[bufsz-1] = '\0';

 return (buf);
}




int
proc_str2flt(const char *str, int *fltnum)
{
 char *next;
 int i;

 i = strtol(str, &next, 0);
 if (i > 0 && i <= (32 * sizeof (fltset_t) / sizeof (uint32_t)) && *next == '\0') {
  *fltnum = i;
  return (0);
 }

 for (i = 1; i <= (32 * sizeof (fltset_t) / sizeof (uint32_t)); i++) {
  const char *s = rawfltname(i);

  if (s && (strcasecmp(s, str) == 0 ||
      strcasecmp(s + 3, str) == 0)) {
   *fltnum = i;
   return (0);
  }
 }

 return (-1);
}

int str2sig(char *str, int signum)
{
 printf("str2sig %s %d called\n", str, signum);
 return 0;
}





int
proc_str2sig(const char *str, int *signum)
{
 if (strncasecmp(str, "SIG", 3) == 0)
  str += 3;

 return (str2sig(str, signum));
}





int
proc_str2sys(const char *str, int *sysnum)
{
 char *next;
 int i;

 i = strtol(str, &next, 0);
 if (i > 0 && i <= (32 * sizeof (sysset_t) / sizeof (uint32_t)) && *next == '\0') {
  *sysnum = i;
  return (0);
 }

 for (i = 1; i < (sizeof (systable) / sizeof (systable[0])); i++) {
  if (systable[i] != ((void *)0) && strcmp(systable[i], str) == 0) {
   *sysnum = i;
   return (0);
  }
 }

 return (-1);
}
# 482 "proc_names.c"
char *
proc_fltset2str(const fltset_t *set, const char *delim, int m,
 char *buf, size_t len)
{
 char name[32], *p = buf;
 size_t n;
 int i;

 if (buf == ((void *)0) || len < 1) {
  (*__errno_location ()) = 22;
  return (((void *)0));
 }

 buf[0] = '\0';

 for (i = 1; i <= (32 * sizeof (fltset_t) / sizeof (uint32_t)); i++) {
  if (((((unsigned)((i)-1) < 32*sizeof (*(set))/sizeof (uint32_t)) && (((uint32_t *)(set))[((i)-1)/32] & (1U<<(((i)-1)%32)))) != 0) ^ (m == 0)) {
   (void) proc_fltname(i, name, sizeof (name));

   if (buf[0] != '\0')
    n = snprintf(p, len, "%s%s", delim, name);
   else
    n = snprintf(p, len, "%s", name);

   if (n != strlen(p)) {
    (*__errno_location ()) = 36;
    return (((void *)0));
   }
   len -= n;
   p += n;
  }
 }
 return (buf);
}






char *
proc_sigset2str(const sigset_t *set, const char *delim, int m,
 char *buf, size_t len)
{
 char name[32], *p = buf;
 size_t n;
 int i;

 if (buf == ((void *)0) || len < 1) {
  (*__errno_location ()) = 22;
  return (((void *)0));
 }

 m = (m != 0);
 buf[0] = '\0';





 for (i = 1; i < 65; i++) {
  if (sigismember(set, i) == m) {
   (void) sig2str(i, name);

   if (buf[0] != '\0')
    n = snprintf(p, len, "%s%s", delim, name);
   else
    n = snprintf(p, len, "%s", name);

   if (n != strlen(p)) {
    (*__errno_location ()) = 36;
    return (((void *)0));
   }

   len -= n;
   p += n;
  }
 }

 return (buf);
}





char *
proc_sysset2str(const sysset_t *set, const char *delim, int m,
 char *buf, size_t len)
{
 char name[32], *p = buf;
 size_t n;
 int i;

 if (buf == ((void *)0) || len < 1) {
  (*__errno_location ()) = 22;
  return (((void *)0));
 }

 buf[0] = '\0';

 for (i = 1; i <= (32 * sizeof (sysset_t) / sizeof (uint32_t)); i++) {
  if (((((unsigned)((i)-1) < 32*sizeof (*(set))/sizeof (uint32_t)) && (((uint32_t *)(set))[((i)-1)/32] & (1U<<(((i)-1)%32)))) != 0) ^ (m == 0)) {
   (void) proc_sysname(i, name, sizeof (name));

   if (buf[0] != '\0')
    n = snprintf(p, len, "%s%s", delim, name);
   else
    n = snprintf(p, len, "%s", name);

   if (n != strlen(p)) {
    (*__errno_location ()) = 36;
    return (((void *)0));
   }
   len -= n;
   p += n;
  }
 }
 return (buf);
}
# 612 "proc_names.c"
char *
proc_str2fltset(const char *s, const char *delim, int m, fltset_t *set)
{
 char *p, *q, *t = __builtin_alloca (strlen(s) + 1);
 int flt;

 if (m) {
  { register int _i_ = sizeof (*(set))/sizeof (uint32_t); while (_i_) ((uint32_t *)(set))[--_i_] = (uint32_t)0; };
 } else {
  { register int _i_ = sizeof (*(set))/sizeof (uint32_t); while (_i_) ((uint32_t *)(set))[--_i_] = (uint32_t)0xFFFFFFFF; };
 }

 (void) strcpy(t, s);

 for (p = strtok_r(t, delim, &q); p != ((void *)0);
     p = strtok_r(((void *)0), delim, &q)) {
  if (proc_str2flt(p, &flt) == -1) {
   (*__errno_location ()) = 22;
   return ((char *)s + (p - t));
  }
  if (m)
   ((void)(((unsigned)((flt)-1) < 32*sizeof (*(set))/sizeof (uint32_t)) ? (((uint32_t *)(set))[((flt)-1)/32] |= (1U<<(((flt)-1)%32))) : 0));
  else
   ((void)(((unsigned)((flt)-1) < 32*sizeof (*(set))/sizeof (uint32_t)) ? (((uint32_t *)(set))[((flt)-1)/32] &= ~(1U<<(((flt)-1)%32))) : 0));
 }
 return (((void *)0));
}






char *
proc_str2sigset(const char *s, const char *delim, int m, sigset_t *set)
{
 char *p, *q, *t = __builtin_alloca (strlen(s) + 1);
 int sig;

 if (m) {
  { register int _i_ = sizeof (*(set))/sizeof (uint32_t); while (_i_) ((uint32_t *)(set))[--_i_] = (uint32_t)0; };
 } else {
  { register int _i_ = sizeof (*(set))/sizeof (uint32_t); while (_i_) ((uint32_t *)(set))[--_i_] = (uint32_t)0xFFFFFFFF; };
 }

 (void) strcpy(t, s);

 for (p = strtok_r(t, delim, &q); p != ((void *)0);
     p = strtok_r(((void *)0), delim, &q)) {
  if (proc_str2sig(p, &sig) == -1) {
   (*__errno_location ()) = 22;
   return ((char *)s + (p - t));
  }
  if (m)
   ((void)(((unsigned)((sig)-1) < 32*sizeof (*(set))/sizeof (uint32_t)) ? (((uint32_t *)(set))[((sig)-1)/32] |= (1U<<(((sig)-1)%32))) : 0));
  else
   ((void)(((unsigned)((sig)-1) < 32*sizeof (*(set))/sizeof (uint32_t)) ? (((uint32_t *)(set))[((sig)-1)/32] &= ~(1U<<(((sig)-1)%32))) : 0));
 }
 return (((void *)0));
}






char *
proc_str2sysset(const char *s, const char *delim, int m, sysset_t *set)
{
 char *p, *q, *t = __builtin_alloca (strlen(s) + 1);
 int sys;

 if (m) {
  { register int _i_ = sizeof (*(set))/sizeof (uint32_t); while (_i_) ((uint32_t *)(set))[--_i_] = (uint32_t)0; };
 } else {
  { register int _i_ = sizeof (*(set))/sizeof (uint32_t); while (_i_) ((uint32_t *)(set))[--_i_] = (uint32_t)0xFFFFFFFF; };
 }

 (void) strcpy(t, s);

 for (p = strtok_r(t, delim, &q); p != ((void *)0);
     p = strtok_r(((void *)0), delim, &q)) {
  if (proc_str2sys(p, &sys) == -1) {
   (*__errno_location ()) = 22;
   return ((char *)s + (p - t));
  }
  if (m)
   ((void)(((unsigned)((sys)-1) < 32*sizeof (*(set))/sizeof (uint32_t)) ? (((uint32_t *)(set))[((sys)-1)/32] |= (1U<<(((sys)-1)%32))) : 0));
  else
   ((void)(((unsigned)((sys)-1) < 32*sizeof (*(set))/sizeof (uint32_t)) ? (((uint32_t *)(set))[((sys)-1)/32] &= ~(1U<<(((sys)-1)%32))) : 0));
 }
 return (((void *)0));
}

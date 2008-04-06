# if !defined(SYS_PROCSET_H)
# define	SYS_PROCSET_H

/*
 *      The following defines the values for an identifier type.  It
 *      specifies the interpretation of an id value.  An idtype and
 *      id together define a simple set of processes.
 */
# if 0 /* <sys/wait.h> */
typedef enum
#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
        idtype          /* pollutes XPG4.2 namespace */
#endif
                {
        P_PID,          /* A process identifier.                */
        P_PPID,         /* A parent process identifier.         */
        P_PGID,         /* A process group (job control group)  */
                        /* identifier.                          */
        P_SID,          /* A session identifier.                */
        P_CID,          /* A scheduling class identifier.       */
        P_UID,          /* A user identifier.                   */
        P_GID,          /* A group identifier.                  */
        P_ALL,          /* All processes.                       */
        P_LWPID,        /* An LWP identifier.                   */
        P_TASKID,       /* A task identifier.                   */
        P_PROJID,       /* A project identifier.                */
        P_POOLID,       /* A pool identifier.                   */
        P_ZONEID,       /* A zone identifier.                   */
        P_CTID,         /* A (process) contract identifier.     */
        P_CPUID,        /* CPU identifier.                      */
        P_PSETID        /* Processor set identifier             */
} idtype_t;
# endif

# endif

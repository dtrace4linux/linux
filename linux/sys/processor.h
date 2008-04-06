# if !defined(PROCESSOR_H)
# define PROCESSOR_H

typedef unsigned int processorid_t;

/*
 * Flags and return values for p_online(2), and pi_state for processor_info(2).
 * These flags are *not* for in-kernel examination of CPU states.
 * See <sys/cpuvar.h> for appropriate informational functions.
 */
#define P_OFFLINE       0x0001  /* processor is offline, as quiet as possible */
#define P_ONLINE        0x0002  /* processor is online */
#define P_STATUS        0x0003  /* value passed to p_online to request status */
#define P_FAULTED       0x0004  /* processor is offline, in faulted state */
#define P_POWEROFF      0x0005  /* processor is powered off */
#define P_NOINTR        0x0006  /* processor is online, but no I/O interrupts */
#define P_SPARE         0x0007  /* processor is offline, can be reactivated */
#define P_BAD           P_FAULTED       /* unused but defined by USL */
#define P_FORCED        0x10000000      /* force processor offline */

#endif

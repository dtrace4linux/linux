/*
 * Flags for taskq_dispatch. TQ_SLEEP/TQ_NOSLEEP should be same as
 * KM_SLEEP/KM_NOSLEEP.
 */
#define TQ_SLEEP        0x00    /* Can block for memory */
#define TQ_NOSLEEP      0x01    /* cannot block for memory; may fail */
#define TQ_NOQUEUE      0x02    /* Do not enqueue if can't dispatch */
#define TQ_NOALLOC      0x04    /* cannot allocate memory; may fail */

#define maxclsyspri 1

/**********************************************************************/
/*   taskq  interface for the probes garbage collection triggered in  */
/*   fasttrap.							      */
/**********************************************************************/
typedef uintptr_t taskqid_t;
typedef int pri_t;
typedef void (task_func_t)(void *);

taskqid_t taskq_dispatch(taskq_t *tq, task_func_t func, void *arg, uint_t flags);
taskq_t * taskq_create(const char *name, int nthreads, pri_t pri, int minalloc,        
    int maxalloc, uint_t flags);
void taskq_destroy(taskq_t *tq);



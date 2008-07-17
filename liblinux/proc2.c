/**********************************************************************/
/*   Missing Linux functions compared to Solaris. We need prototypes  */
/*   in  scope to make sure we behave. Stuff migrated from proc.c as  */
/*   we fill them in.						      */
/*   Author: Paul Fox						      */
/*   Date: May 2008						      */
/**********************************************************************/

# include	<time.h>
# include	<sys/time.h>
# include	<pthread.h>
#include "Pcontrol.h"
#include "libproc.h"

int lx_read_stat(struct ps_prochandle *P, pstatus_t *pst)
{	int	fd;
	char	buf[4096];
	char	cmd[1024];
	char	state[1024];
	int	n;
	long	pid, ppid, pgid, sid, tty_nr, tty_pgrp;
	long	flags;
	long	min_flt, cmin_flt, maj_flt, cmaj_flt;
	long	utime, stime, cutime, cstime;
	long	priority, nice, num_threads, it_real_value;
	long	start_time, vsize, rss, rsslim;
	long	start_code, end_code, start_stack, esp, eip;
	long	pending, blocked, sigign, sigcatch;
	long	wchan, zero1, zero2, exit_signal;
	long	cpu, rt_priority, policy;

HERE(); printf("Help: reading /stat/ structure.\n");
	memset(pst, 0, sizeof *pst);
	sprintf(buf, "/proc/%d/stat", P->pid);
	if ((fd = open(buf, O_RDONLY)) < 0)
		return -1;
	n = read(fd, buf, sizeof buf);

	sscanf(buf, "%ld %s %s %ld %ld %ld %ld "
		"%ld %ld %ld %ld "	 // flt
		"%ld %ld %ld %ld " // utime
		"%ld %ld %ld %ld " // priority
		"%ld %ld %ld %ld " // start_time
		"%ld %ld %ld %ld %ld" // start_code
		"%ld %ld %ld %ld " // pending
		"%ld %ld %ld %ld " // wchan
		"%ld %ld %ld"	 // cpu
		,

		&pid, cmd, state, &ppid, &pgid, &tty_nr, &tty_pgrp,
		&min_flt, &cmin_flt, &maj_flt, &cmaj_flt,
		&utime, &stime, &cutime, &cstime,
		&priority, &nice, &num_threads, &it_real_value,
		&start_time, &vsize, &rss, &rsslim,
		&start_code, &end_code, &start_stack, &esp, &eip,
		&pending, &blocked, &sigign, &sigcatch,
		&wchan, &zero1, &zero2, &exit_signal,
		&cpu, &rt_priority, &policy);

	pst->pr_pid = pid;
	pst->pr_ppid = ppid;
	pst->pr_sid = sid;
	pst->pr_nlwp = num_threads;
	return 0;
}

unsigned long long 
gethrtime()
{
	struct timespec sp;
	int ret;
	long long v;

	ret = clock_gettime(CLOCK_REALTIME, &sp);
# if 0
#ifdef CLOCK_MONOTONIC_HR
	ret = clock_gettime(CLOCK_MONOTONIC_HR, &sp);
#else
	ret = clock_gettime(CLOCK_MONOTONIC, &sp);
#endif
# endif

	if (ret)
		return 0;

	v = 1000000000LL;
	v *= sp.tv_sec;
	v += sp.tv_nsec;

	return v; 
}

/**********************************************************************/
/*   Linux doesnt support zones (well, it can do, but not quite).     */
/**********************************************************************/
int
getzoneid() 
{
	return 0;
}
/**********************************************************************/
/*   Memory  barrier  used  by  atomic  cas instructions. We need to  */
/*   implement  this if we are to be reliable in an SMP environment.  */
/*   systrace.c calls us   					      */
/**********************************************************************/
void
membar_enter(void)
{
}
void
membar_producer(void)
{
}
/**********************************************************************/
/*   Initialise  a  mutex using pthreads lib. We map solaris mutex_t  */
/*   to a pthread_mutex_t.					      */
/**********************************************************************/
int
mutex_init(pthread_mutex_t *m, int flags1, void *ptr)
{
	if (ptr) {
		printf("%s: mutex_init called with non-null ibc\n", __func__);
	}
	return pthread_mutex_init(m, NULL);
}

int
pthread_cond_reltimedwait_np(pthread_cond_t *cvp, pthread_mutex_t *mp,
        struct timespec *reltime)
{	struct timespec ts;
	ts = *reltime;
	ts.tv_sec += time(NULL);
//	sleep(1);
	return pthread_cond_timedwait(cvp, mp, &ts);
}

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

int
pthread_cond_reltimedwait_np(pthread_cond_t *cvp, pthread_mutex_t *mp,
        struct timespec *reltime)
{	struct timespec ts;
	ts = *reltime;
	ts.tv_sec += time(NULL);
//	sleep(1);
	return pthread_cond_timedwait(cvp, mp, &ts);
}

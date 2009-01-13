/* Simple thread test - am having problems proving dtrace
thread/posix locking works; could be a vmware issue... */
# include <stdio.h>
# include <pthread.h>
# include <signal.h>

pthread_mutex_t m;
static void
thread(void *arg)
{
	printf("2: in thread...about to lock\n");
	pthread_mutex_lock(&m);
	printf("2: in thread .. succeeded lock\n");
	pause();
}

int main(int argc, char **argv)
{	pthread_t	tid;
	pthread_attr_t a;
	sigset_t nset, oset;

	pthread_mutex_init(&m, NULL);
	printf("1: main creating thread\n");
	pthread_mutex_lock(&m);

	(void) pthread_attr_init(&a);
	(void) pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	(void) sigfillset(&nset);
	(void) sigdelset(&nset, SIGABRT);	/* unblocked for assert() */
	(void) sigdelset(&nset, SIGUSR1);	/* see dt_proc_destroy() */
	(void) pthread_sigmask(SIG_SETMASK, &nset, &oset);
	pthread_create(&tid, NULL, thread, NULL);
	printf("1: thread created\n");
	pause();
}

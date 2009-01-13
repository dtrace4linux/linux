/* Simple ptrace test - use PTRACE_ATTACH in a thread
to see if it works; dtrace is having trouble attaching
to a forked/execed proc.
*/
# include <stdio.h>
# include <sys/ptrace.h>
# include <pthread.h>
# include <signal.h>

int	pid;
pthread_mutex_t m;

static void
thread(void *arg)
{
	if (fork()) return;
	if (ptrace(PTRACE_ATTACH, pid, 0, 0) < 0) {
		perror("PTRACE_ATTACH");
	}
	printf("about to do PTRACE_CONT \n");
	if (ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
		perror("PTRACE_CONT");
	}
	printf("PTRACE_CONT executed\n");
	pause();
}

int main(int argc, char **argv)
{	pthread_t	tid;
	pthread_attr_t a;
	sigset_t nset, oset;

	pid = fork();
	if (pid == 0) {
		if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
			perror("PTRACE_TRACEME");
		}
		printf("execing...\n");
		execlp("/bin/date", "/bin/date", NULL);
	}
//thread(0);
	pthread_mutex_init(&m, NULL);
	printf("1: main creating thread\n");
	pthread_mutex_lock(&m);

	(void) pthread_attr_init(&a);
//	(void) pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
	(void) sigfillset(&nset);
	(void) sigdelset(&nset, SIGABRT);	/* unblocked for assert() */
	(void) sigdelset(&nset, SIGUSR1);	/* see dt_proc_destroy() */
//	(void) pthread_sigmask(SIG_SETMASK, &nset, &oset);
	pthread_create(&tid, &a, thread, NULL);
	printf("1: thread created\n");
	pause();
}

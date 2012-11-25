#include <stdio.h>
#include <time.h>
#include <signal.h>

volatile int cnt;

void usr1_handler()
{
	cnt++;
}
int main(int argc, char **argv)
{	int	pid;
	time_t	t0 = time(NULL);

	signal(SIGUSR1, usr1_handler);
	if ((pid = fork()) == 0) {
		/***********************************************/
		/*   Child gets the machine gun.	       */
		/***********************************************/
		while (1) {
			int ret = kill(pid, SIGUSR1);
			if (ret < 0)
				exit(0);
		}
	}
	while (1) {
		time_t t1 = time(NULL);
		char	buf[BUFSIZ];

		if (t1 != t0) {
			printf("%d: count=%d\n", getpid(), cnt);
			cnt = 0;
			t0 = t1;
		}
	}
}

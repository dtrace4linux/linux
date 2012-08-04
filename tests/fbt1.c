/* Simple do-nothing executable, so we can debug pid provider
without a lot of action/noise. */
#include <stdio.h>

void do_nothing(int n);
void do_nothing2(void);

int main(int argc, char **argv)
{	int	n = 99;

	while (1) {
		char *cp = malloc(1234);
		free(cp);
		printf("pid: %d %p ", (int) getpid(), &cp);
		fflush(stdout);
		system("uptime");
		system("head /proc/dtrace/fasttrap");
		sleep(1);

		if (argc > 1) {
			char buf[1024];
			printf("Ready: ");
			fgets(buf, sizeof buf, stdin);
			do_nothing2();
		} else
			do_nothing(n++);
	}
}
/**********************************************************************/
/*   Functions  which  are not called, so we can probe them, and not  */
/*   actually invoke a probe. Used for debugging.		      */
/**********************************************************************/
void
do_nothing(int n)
{
	if ((n & 3) == 0) 
		system("df");
}
void
do_nothing2()
{
	if (getpid() & 1)
		printf("something %x\n", getpid() + 3 * getpid());
	else
		printf("something else %x\n", getpid() % 3);
		

	system("df");
}

/* Simple do-nothing executable, so we can debug pid provider
without a lot of action/noise. */
#include <stdio.h>

void do_nothing2(void);

int main(int argc, char **argv)
{
	while (1) {
		char *cp = malloc(1234);
		free(cp);
		printf("pid: %d %p ", (int) getpid(), &cp);
		fflush(stdout);
		system("uptime");
		system("cat /proc/dtrace/fasttrap");
		sleep(1);

		if (argc > 1) {
			char buf[1024];
			printf("Ready: ");
			fgets(buf, sizeof buf, stdin);
			do_nothing2();
		}
	}
}
/**********************************************************************/
/*   Functions  which  are not called, so we can probe them, and not  */
/*   actually invoke a probe. Used for debugging.		      */
/**********************************************************************/
void
do_nothing()
{
	while (1) {
		system("df");
		}
}
void
do_nothing2()
{
	system("df");
}

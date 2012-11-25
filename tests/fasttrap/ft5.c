#include <stdio.h>
#include <signal.h>

volatile unsigned cnt;
int	tick;

void alarm_handler()
{
	printf("%d tick: %2d %u\n", getpid(), tick++, cnt);
}
int main(int argc, char **argv)
{
	while (1) {
		int	old_tick = tick;
		signal(SIGALRM, alarm_handler);
		alarm(1);
		for (cnt = 0; ; cnt++) {
			if (tick != old_tick)
				break;
		}
	}
}

# include <stdio.h>
# include <sys/sdt.h>

int main(int argc, char **argv)
{	int	n;

	/***********************************************/
	/*   Invoke shlib function.		       */
	/***********************************************/
	fred();

	for (n = 0; ; n++) {
		printf("PID:%d %d: here on line %d\n", getpid(), n, __LINE__);
		DTRACE_PROBE1(simple, saw__line, 0x1234);
		printf("PID:%d here on line %d\n", getpid(), __LINE__);
		DTRACE_PROBE1(simple, saw__word, 0x87654321);
		printf("PID:%d here on line %d\n", getpid(), __LINE__);
		DTRACE_PROBE1(simple, saw__word, 0xdeadbeef);
		printf("PID:%d here on line %d\n", getpid(), __LINE__);
		sleep(2);
		}
}

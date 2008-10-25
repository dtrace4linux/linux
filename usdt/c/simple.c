# include <stdio.h>
# include <sys/sdt.h>

int main(int argc, char **argv)
{	int	n;

	/***********************************************/
	/*   Invoke shlib function.		       */
	/***********************************************/
	fred();

	for (n = 0; ; n++) {
		printf("%d: here on line %d\n", n, __LINE__);
		DTRACE_PROBE1(simple, saw__line, 0x1234);
		printf("here on line %d\n", __LINE__);
		DTRACE_PROBE1(simple, saw__word, 0x87654321);
		printf("here on line %d\n", __LINE__);
		DTRACE_PROBE1(simple, saw__word, 0xdeadbeef);
		printf("here on line %d\n", __LINE__);
		sleep(1);
		}
}

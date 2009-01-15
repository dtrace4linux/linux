# include <stdio.h>
# include <sys/sdt.h>

int end_main(void);

int main(int argc, char **argv)
{	int	n;
	unsigned long crc;
	unsigned char *cp;

	/***********************************************/
	/*   Invoke shlib function.		       */
	/***********************************************/
	fred();

	for (n = 0; ; n++) {
		/***********************************************/
		/*   compute  checksum  for  main() so we can  */
		/*   watch if we get patched.		       */
		/***********************************************/
		crc = 0;
		for (cp = (unsigned char *) main; cp != (unsigned char *) end_main; cp++)
			crc += *cp;

		printf("PID:%d %d: here on line %d: crc=%08lx\n", getpid(), n, __LINE__, crc);
		DTRACE_PROBE1(simple, saw__line, 0x1234);
		printf("PID:%d here on line %d\n", getpid(), __LINE__);
		DTRACE_PROBE1(simple, saw__word, 0x87654321);
		printf("PID:%d here on line %d\n", getpid(), __LINE__);
		DTRACE_PROBE1(simple, saw__word, 0xdeadbeef);
		printf("PID:%d here on line %d\n", getpid(), __LINE__);
		sleep(2);
		}
}
int end_main(void)
{
	return 0;
}

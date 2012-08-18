#include <math.h>
#include <stdio.h>
#include <signal.h>

void sig()
{
	printf("inside a signal\n");
}

int main(int argc,char **argv)
{	char	buf[BUFSIZ];
	double x = (int) argv;

	signal(SIGINT,sig);

	printf("PID %d .. press enter when ready\n", getpid());
	fgets(buf, sizeof buf, stdin);

	printf("%f\n", sin(x));

	printf("PID %d .. press enter when ready\n", getpid());
	/* run thru the rtld exit() code.... */
}
int main2()
{	char	buf[100000];
	char	*cp = buf;
	void (*func)() = buf;

	*cp++ = 0x90;
	*cp++ = 0xc3;
	*cp++ = 0x00;
	*cp++ = 0x00;

	func();
	printf("hello world\n");
}
int main3()
{
	asm(".byte 0x42, 0xff, 0x24, 0xf5, 0x10, 0x5d, 0x51, 0x00\n");
}

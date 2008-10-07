# include <stdio.h>
# include <sys/sdt.h>

int main(int argc, char **argv)
{
	DTRACE_PROBE1(simple, saw__line, 0x1234);
	DTRACE_PROBE1(simple, saw__word, 0x87654321);
}

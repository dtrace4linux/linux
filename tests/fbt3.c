# include <stdio.h>

void fred()
{
	printf("this is fred\n");
}
void (*func) = fred;
int main(int argc, char **argv)
{	char	buf[BUFSIZ];

	printf("pid %d ready: ", getpid());
	fgets(buf, sizeof buf, stdin);
	asm("mov $fred,%edi\n");
	asm("call *%edi\n");

	asm("mov $func,%edx\n");
	asm("call *(%edx)\n");
}

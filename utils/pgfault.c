/**********************************************************************/
/*   Try and induce page faulting system calls on the kernel.	      */
/**********************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>

void
int_handler()
{
	printf("Interrupt!\n");

}
int main()
{	

	signal(SIGSEGV, SIG_IGN);
	int fd = open("/dev/zero", O_RDONLY);
	printf("fd = %d\n", fd);
	char *cp = mmap(NULL, 1024 * 1024, PROT_READ, MAP_PRIVATE, fd, 0);
	printf("addr = %p\n", cp);

	while (1) {
		printf("cp=%p %x\n", cp, *cp);
		cp += 8192;
		int fd = open(cp, O_RDONLY);
		if (fd > 0)
			close(fd);
	}
}

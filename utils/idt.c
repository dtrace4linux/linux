# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>

static char buf[64];

int main()
{	void *idtr;
	int	fd, ret;

	asm ("sidt %0" : "=m" (idtr));
	idtr = (unsigned long) idtr & ~0xfff;

	printf("idt=%p\n", idtr);
	fd = open64("/dev/kmem", O_RDWR);
	lseek64(fd, idtr, SEEK_SET);
	ret = read(fd, buf, sizeof buf);
	if (ret < 0) {
		perror("read");
		exit(1);
	}
}

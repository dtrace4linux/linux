#define _LARGEFILE64_SOURCE
       
# include <stdio.h>
# include <stdlib.h>
# include <fcntl.h>
# include <unistd.h>
# include <sys/types.h>

int main(int argc, char **argv)
{	
	off64_t addr;
	char	buf[4096];
	int	n, ret;
	int	fd;
	char	*device = "/dev/kmem";

	sscanf(argv[1], "%llx", &addr);
	if (argc > 2) {
		device = argv[2];
		}
	printf("addr: %llx\n", addr);

	fd = open64(device, O_RDONLY | O_LARGEFILE);
	if (fd < 0) {
		perror(device);
		exit(1);
		}
	if (lseek64(fd, addr, SEEK_SET) == -1) {
		perror("lseek64");
		exit(1);
		}
	n = read(fd, buf, sizeof buf);
	printf("n=%d\n", n);
	if (n < 0)
		perror("read");

}

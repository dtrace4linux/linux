/*
Tool to get old_rsp from the running kernel. Some kernels
dont export this in /proc/kallsyms or /boot/System.map. You
cannot get it from the kernel headers, so we can only
get it from the syscalls that we are intercepting.

Paul Fox Feb 2015
*/

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <fcntl.h>

int main(int argc, char **argv)
{	int	fd, i, n;
	unsigned long offset;
	FILE	*fp;
	unsigned char	buf[BUFSIZ];
	unsigned char	*proc;
	unsigned long ptr;
	char	*name = "stub_execve";
	int	debug = 0;

	snprintf(buf, sizeof buf,
		"grep %s /proc/kallsyms",
		name);

	if ((fp = popen(buf, "r")) == NULL) {
		perror("grep");
		exit(1);
	}

	if (fgets(buf, sizeof buf, fp) == NULL) {
		fprintf(stderr, "Failed to find %s\n", name);
		exit(1);
	}
	pclose(fp);

	sscanf(buf, "%lx", &ptr);
	if (debug)
		printf("%s=0x%lx\n", name, ptr);

	if ((fd = open64("/proc/kcore", O_RDONLY)) < 0) {
		perror("/proc/kcore");
		exit(1);
	}

	/***********************************************/
	/*   Not sure why we have to do this - but we  */
	/*   reflect  the  virtual addr to a physical  */
	/*   one.				       */
	/***********************************************/
	ptr &= ~0xffff800000000000L;
	if (lseek64(fd, ptr, SEEK_SET) == -1) {
		perror("lseek");
		exit(1);
	}

	if ((n = read(fd, buf, sizeof buf)) != sizeof buf) {
		fprintf(stderr, "Failed to read buffer from /proc/kcore\n");
		exit(1);
	}

	for (i = 0; i < n; i++) {
		// mov %gs:nnnn,%r11
		if (memcmp(buf + i, "\x65\x4c\x8b\x1c\x25", 5) == 0) {
			printf("%x\n",
				*(int *) (buf + i + 5));
			exit(0);
		}
	}
	fprintf(stderr, "Failed to find offset\n");
	exit(1);
}

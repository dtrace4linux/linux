# include <stdio.h>
# include <stdlib.h>
# include <fcntl.h>
# include <errno.h>
# include <ctype.h>
# include <sys/ptrace.h>

int main(int argc, char **argv)
{	int	ret;
	char	buf[1024];
	int	fd;
	void	*addr = main;
	char	*name = "/proc/self/mem";
	int	size = sizeof buf;
	int	do_attach = 0;
	int	pid = -1;
	
	if (argc > 1 && strcmp(argv[1], "-attach") == 0) {
		do_attach = 1;
		argv++, argc--;
	}
	if (argc > 1) {
		name = argv[1];
		if (isdigit(*name)) {
			pid = atoi(argv[1]);
			sprintf(buf, "/proc/%s/mem", argv[1]);
			name = buf;
		}
		argv++, argc--;
	}

	if (argc > 1) {
		addr = (char *) strtoul(argv[1], NULL, 0);
		argv++, argc--;
	}

	if (argc > 1) {
		size = atoi(argv[1]);
		argv++, argc--;
	}

	if (do_attach) {
		ret = ptrace(PTRACE_ATTACH, pid, 0, 0);
		if (ret < 0) {
			perror("ptrace(PTRACE_ATTACH)");
			exit(1);
		}
	}

	printf("open: %s, offset %p, readlen=%d\n", name, addr, size);

	if ((fd = open64(name, O_RDONLY)) < 0) {
		perror("name");
		exit(1);
	}

# if defined(__i386)
	/***********************************************/
	/*   Avoid  sign-extension  issues in top 2GB  */
	/*   of addr space.			       */
	/***********************************************/
	ret = pread64(fd, buf, size, (unsigned long long) (unsigned long) addr);
# else
	ret = pread64(fd, buf, size, addr);
# endif
	printf("ret=%d\n", ret);
	if (ret < 0)
		perror("read");

	if (do_attach) {
		ret = ptrace(PTRACE_DETACH, pid, 0, 0);
		if (ret < 0) {
			perror("ptrace(PTRACE_DETACH)");
			exit(1);
		}
	}
}

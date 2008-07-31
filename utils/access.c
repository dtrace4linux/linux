# include <stdio.h>
# include <errno.h>

int main()
{	int	ret;
	int	i;
	char	buf[1024];
	memset(buf, 'a', sizeof buf);
	for (i = 0; ; i++) {
		printf("pid=%d %d: doing access\n", getpid(), i);
		if ((ret = access("/tmp/xxxx", 2)) < 0) {
			printf("access ret=%d ", ret);
			perror("");
		}
		sleep(5);
	}

}

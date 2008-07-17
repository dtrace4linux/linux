# include <stdio.h>
# include <errno.h>

int main()
{	int	ret;
	int	i;

	for (i = 0; ; i++) {
		printf("%d: doing access\n", i);
		if ((ret = access("/tmp/xxxx", 2)) < 0) {
			printf("access ret=%d ", ret);
			perror("");
		}
		sleep(5);
	}

}

# include <stdio.h>

int main()
{
	int	pid = getpid();
	int	p;

	printf("mypid=%d\n", pid);
	while (1) {
		if ((p = getpid(&p)) != pid) {
			printf("oops: pid=%d vs getpid=%d\n", pid, p);
		}
		time(NULL);
	}

}

# include <stdio.h>
# include <time.h>

int main()
{
	int	uid = getuid();
	int	p;
	unsigned long n = 0;
	time_t t = time(NULL);

	printf("myuid=%d\n", uid);
	while (1) {
		if ((p = getuid()) != uid) {
			printf("oops: uid=%d vs getuid=%d\n", uid, p);
		}
		n++;
		if (t != time(NULL)) {
			printf("counted: %lu\n", n);
			n = 0;
			t = time(NULL);
		}
	}

}

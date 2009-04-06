# include <pthread.h>

void thread_func(void *arg)
{	int	ret = iopl(3);

	printf("thread iopl=%d\n", ret);

}
int main(int argc, char **argv)
{
	int	pl = argc == 1 ? 3 : atoi(argv[1]);
	int	ret;
	pthread_t thr;

	pthread_create(&thr, NULL, thread_func, NULL);

	ret = iopl(pl);
	printf("iopl(%d) = %d\n", pl, ret);
	pause();
}

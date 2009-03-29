// test dtrace -n ::clone: 
// which is invoked from fork.

int main(int argc, char **argv)
{
	int	ret;
	sleep(5);
	printf("forking...\n");
	ret = fork();
	printf("ret=%d\n", ret);
}

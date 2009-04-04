int main()
{	char *argv[2];
	int	ret;

	sleep(5);
	printf("execing...\n");
	argv[0] = "/bin/date";
	argv[1] = 0;
	ret = execvp(argv[0], argv);
	printf("ret=%d\n", ret);
}


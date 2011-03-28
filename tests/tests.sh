# 20110329 Validate pgfault handler intercepts bad addresses on
#          the bogus value from a return
root build/dtrace -n '
BEGIN {
cnt = 0;
}
	syscall::open*:
	{
	printf("%s", stringof(arg0));
	cnt++;
	}
	tick-5s
	/cnt > 100 /
	{
	exit(0);
	}
'

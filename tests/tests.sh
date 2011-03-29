# 20110329 Validate pgfault handler intercepts bad addresses on
#          the bogus value from a return
root build/dtrace -n '
BEGIN {
cnt = 0;
}
	syscall::open*:
	{
	printf("%d %d %s %s", pid, ppid, execname, stringof(arg0));
	cnt++;
	}
	tick-5s
	/cnt > 1000 /
	{
	exit(0);
	}
'

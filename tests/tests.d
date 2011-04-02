name: systrace-stringof-bad
note:
	20110329 Validate pgfault handler intercepts bad addresses on
	the bogus value from a return
d:
	BEGIN {
	cnt = 0;
	}
	syscall::open*:
	{
	printf("%d %d %s %s", pid, ppid, execname, stringof(arg0));
	cnt++;
	}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s
	/cnt > 1000 /
	{
	exit(0);
	}

name: fbt-a
note: Do stuff to measure fbt heavy duty access.
d:
	fbt::a*:
	{
	cnt++;
	}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s
	{
	exit(0);
	}

name: io-1
note: check io provider isnt causing page faults.
d:
	io::: { cnt++;}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s {
		exit(0);
	}

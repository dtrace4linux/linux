##################################################################
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
	/cnt > ${loop} /
	{
	exit(0);
	}

##################################################################
name: systrace-stringof-bad2
note:
	Use some arg to generate a page fault - systrace-stringof-bad
	may not generate a page fault depending on the value of arg0
	when returning from the function.
d:
	BEGIN {
		cnt = 0;
	}
	syscall::open*: {
		printf("bad2 %d %d %s %s %s %s", pid, ppid, execname, 
			stringof(arg0), stringof(arg1), stringof(arg2));
		cnt++;
	}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s /cnt > ${loop} /
	{
		exit(0);
	}
##################################################################
name: systrace-stringof-bad3
note:
	Be more dastardly trying to trigger a fault in dtrace_getarg -
	just do stringof any of the args of any syscalls.
d:
	BEGIN {
		cnt = 0;
	}
	syscall::: {
		printf("bad2 %d %d %s %s %s %s", pid, ppid, execname, 
			stringof(arg0), stringof(arg1), stringof(arg2));
		cnt++;
	}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s /cnt > ${loop} /
	{
		exit(0);
	}
##################################################################
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

##################################################################
name: copyinstr-1
note: Simple version of copyinstr-1
d:
	syscall::open*:entry { 
		cnt++;
		printf("%s",execname);
	}
	tick-5s
	{
	exit(0);
	}
##################################################################
name: copyinstr-2
note: Validate copyinstr isnt generate badaddr messages
d:
	syscall::open*:entry { 
		cnt++;
		printf("%s %s",execname,copyinstr(arg0)); 
	}
	tick-5s
	{
	exit(0);
	}


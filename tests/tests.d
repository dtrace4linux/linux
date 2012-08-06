##################################################################
# These tests are designed to validate core functionality in dtrace
# on the installed operating system, such as bad address handling,
# and various probe functions. We dont really care about the output -
# other than some form of forward progress.
##################################################################
name: systrace-stringof-bad
note:
	20110329 Validate pgfault handler intercepts bad addresses on
	the bogus value from a return
d:
	BEGIN {
	cnt = 0;
	tstart = timestamp;
	}
	syscall::open*:
	{
		this->pid = pid;
		this->ppid = ppid;
		this->execname = execname;
		this->arg0 = stringof(arg0);
		cnt++;
	}
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s { exit(0); }

##################################################################
name: systrace-stringof-bad2
note:
	Use some arg to generate a page fault - systrace-stringof-bad
	may not generate a page fault depending on the value of arg0
	when returning from the function.
d:
	BEGIN {
		cnt = 0;
		tstart = timestamp;
	}
	syscall::open*: {
		this->pid = pid;
		this->ppid = ppid;
		this->execname = execname;
		this->arg0 = stringof(arg0);
		this->arg1 = stringof(arg1);
		this->arg2 = stringof(arg2);
		cnt++;
	}
	syscall::open*: /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s { exit(0); }

##################################################################
name: systrace-stringof-bad3
note:
	Be more dastardly trying to trigger a fault in dtrace_getarg -
	just do stringof any of the args of any syscalls.
d:
	BEGIN {
		cnt = 0;
		tstart = timestamp;
	}
	syscall::: {
		this->pid = pid;
		this->ppid = ppid;
		this->execname = execname;
		this->arg0 = stringof(arg0);
		this->arg1 = stringof(arg1);
		this->arg2 = stringof(arg2);
		cnt++;
	}
	syscall::: /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s { exit(0); }
##################################################################
name: systrace-stringof-bad4
note:
	Turn off the stringof, but otherwise the same as 
	systrace-stringof-bad3
d:
	BEGIN {
		cnt = 0;
		tstart = timestamp;
	}
	syscall::: {
		this->pid = pid;
		this->ppid = ppid;
		this->execname = execname;
		cnt++;
	}
	syscall::: /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s { exit(0); }
##################################################################
name: high-profile1
note:
	Lots of ticks to try and induce interrupt stacking/xcall
	issues.
d:
	BEGIN {
		cnt = 0;
		tstart = timestamp;
	}
	syscall::: {
		this->pid = pid;
		this->ppid = ppid;
		this->execname = execname;
		this->arg0 = stringof(arg0);
		this->arg1 = stringof(arg1);
		this->arg2 = stringof(arg2);
		cnt++;
	}
	syscall::: /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-5000 { }
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s { exit(0); }
##################################################################
name: high-profile2
note:
	Lots of ticks but no probes on syscalls.
	issues.
d:
	BEGIN {
		cnt = 0;
	}
	tick-5000 { cnt++; }
	tick-1s { printf("count so far: %d", cnt); }
	tick-10s { exit(0); }
##################################################################
name: fbt-a
note: Do stuff to measure fbt heavy duty access.
d:
	BEGIN {
		tstart = timestamp;
	}
	fbt::a*:
	{
	cnt++;
	}
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s { exit(0); }

##################################################################
name: fbt-abc
note: Do more stuff to measure fbt heavy duty access.
d:
	fbt::a*:
	{
	cnt++;
	}
	tick-1s { printf("count so far: %d", cnt); }
	tick-10s { exit(0); }
##################################################################
name: io-1
note: check io provider isnt causing page faults.
d:
	BEGIN {
		tstart = timestamp;
	}
	io::: { cnt++;}
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1s { printf("count so far: %d", cnt); }
	tick-5s { exit(0); }

##################################################################
name: execname-1
note: Simple use of execname
d:
	BEGIN {
		tstart = timestamp;
	}
	syscall::open*:entry { 
		cnt++;
		printf("%s",execname);
	}
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-5s { exit(0); }
##################################################################
name: copyinstr-1
note: Validate copyinstr isnt generating badaddr messages
d:
	BEGIN {
		tstart = timestamp;
	}
	syscall::open*:entry { 
		cnt++;
		printf("%s %s",execname,copyinstr(arg0)); 
	}
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-5s { exit(0); }
##################################################################
name: badptr-1
note: Simple badptr test (thanks Nigel Smith)
d:
	BEGIN
	{
	        x = (int *)NULL;
	        y = *x;
	        trace(y);
		exit(0);
	}
	tick-1s { exit(0); }
##################################################################
name: profile-1
note: Check we dont lose a rare timer in the midst of lots of timers.
d:
	int cnt;
	tick-1s { 
		printf("got %d * 1mS ticks\n", cnt);
		exit(0); 
		}
	tick-1ms { cnt++; }
##################################################################
name: profile-2
note: Check we dont lose a rare timer in the midst of lots of timers.
d:
	int cnt;
	tick-1s { 
		printf("got %d * tick-5000 ticks\n", cnt);
		exit(0); 
		}
	tick-5000 { cnt++; }
##################################################################
name: profile-3
note: Check we dont lose a rare timer in the midst of lots of timers.
d:
	int cnt_1ms, cnt_1s;
	tick-1ms { cnt_1ms++; } 
	tick-1s { cnt_1s++; 
		printf("tick-1ms=%d tick-1s=%d", cnt_1ms, cnt_1s);
		}
	tick-5s { 
		printf("the end: got %d + %d\n", cnt_1ms, cnt_1s);
		exit(0); 
		}
##################################################################
name: profile-4
note: Check we dont lose a rare timer in the midst of lots of timers.
d:
	BEGIN {
		tstart = timestamp;
		}
	int cnt_1ms, cnt_1s;
	fbt::a*: {}
	tick-1ms { cnt_1ms++; } 
	tick-1ms /timestamp - tstart > 5 * 1000 * 1000 * 1000 / {exit(0);}
	tick-1s { cnt_1s++; 
		printf("tick-1ms=%d tick-1s=%d", cnt_1ms, cnt_1s);
		}
	tick-5s { 
		printf("the end: got %d + %d\n", cnt_1ms, cnt_1s);
		exit(0); 
		}
##################################################################
name: profile-5
note: Check we dont lose a rare timer in the midst of lots of timers.
d:
	int cnt_1ms, cnt_1s, cnt_fbt;
	fbt::: {cnt_fbt++;}
	tick-1ms { cnt_1ms++; } 
	tick-1s { cnt_1s++; 
		printf("tick-1ms=%d tick-1s=%d fbt=%d", cnt_1ms, cnt_1s, cnt_fbt);
		}
	tick-5s { 
		printf("the end: got %d + %d, fbt=%d\n", cnt_1ms, cnt_1s, cnt_fbt);
		exit(0); 
		}
##################################################################
name: profile-6
note: Lots of severe timers whilst doing heavy fbt stuff. Attempt to
	track for race conditions in timer destruction.
d:
	fbt:::{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;} tick-1ms{cnt++;}
	tick-5s { 
		exit(0); 
		}

##################################################################
name: quantize-1
note: Some random quantize invocations
d:
	syscall:::entry { self->t = timestamp; }
	syscall:::return { 
		@s[probefunc] = quantize(timestamp - self->t); 
		self->t = 0;
		}
	tick-5s { 
		exit(0); 
		}
##################################################################
name: quantize-2
note: Some random quantize invocations
d:
	syscall:::entry { self->t = timestamp; }
	syscall:::return { 
		@s[probefunc] = lquantize(timestamp - self->t, 0, 100000, 200); 
		self->t = 0;
		}
	tick-5s { 
		exit(0); 
		}

##################################################################
name: BEGIN-fbt:a-exec-time
note: Test from Nigel Smith
d:
	fbt:kernel:a*: {} 
	dtrace:::BEGIN { exit(0); }
##################################################################
name: BEGIN-fbt:exec-time
note: Test from Nigel Smith
d:
	fbt:kernel:: {} 
	dtrace:::BEGIN { exit(0); }
##################################################################
name: fbt-BEGIN-exit
note: Variant of bug report from Nigel Smith
d:
	fbt:kernel:: BEGIN {exit(0); } 

##################################################################
name: tcp-1
note: Check tcp probe. Ideally need to invoke TCP activity, but this will 
	just let us show we dont crash the kernel.
d:
	tcp::: {printf("%s", execname); }
	tick-5s { 
		exit(0); 
		}

##################################################################
name: vminfo-1
note: Check vminfo does something
d:
	vminfo::: {printf("%s", execname); }
	tick-5s { 
		exit(0); 
		}

##################################################################
name: tick-1
note: Some basic check of time sanity, e.g. num milliseconds in a second
d:
	int cnt_1ms, cnt_1s;
	tick-1ms {
	        cnt_1ms++;
	        printf("%d %d.%09d", cnt_1ms, timestamp / 1000000000, 
			timestamp % (1000000000));
	        }
	tick-1s { cnt_1s++;
	        printf("tick-1ms=%d tick-1s=%d", cnt_1ms, cnt_1s);
		cnt_1ms = 0;
	        }
	tick-5s {
	        printf("the end: got %d + %d\n", cnt_1ms, cnt_1s);
	        exit(0);
	        }

##################################################################
name: tick-2
note: Make sure timers are not counting after we terminate dtrace.
	Cant check that in the test itself, but this is designed to be
	nasty.
d:
	int cnt_1ms;
	tick-1ms { cnt_1ms++;}
	tick-2ms { cnt_1ms++;}
	tick-3ms { cnt_1ms++;}
	tick-4ms { cnt_1ms++;}
	tick-5ms { cnt_1ms++;}
	tick-6ms { cnt_1ms++;}
	tick-7ms { cnt_1ms++;}
	tick-8ms { cnt_1ms++;}
	tick-9ms { cnt_1ms++;}
	tick-10ms { cnt_1ms++;}
	tick-5s { 
		exit(0); 
		}

##################################################################
name: tick-3
note: 1s worth of 1m ticks.
d:
	int cnt_1ms;
	tick-1ms {
	        cnt_1ms++;
	        }
	tick-1s {
	        printf("cnt_1ms=%d in 1 second", cnt_1ms);
		cnt_1ms = 0;
	        }
	tick-5s {
		exit(0);
	        }

##################################################################
name:	instr-1
note:	Some checks on the instruction provider. Helps us to determine
	if the single step works properly, and recursion. Much more
	invasive than fbt (because there are more probes).
d:
	instr::a*-j*: { cnt++;}
	tick-1ms {exit(0);}

##################################################################
name:	llquantize-1
note:	test llquantize parsing works
d:
	tick-1ms
	{
		/*
		 * A log-linear quantization with factor of 10 ranging from magnitude
		 * 0 to magnitude 6 (inclusive) with twenty steps per magnitude
		 */
		@ = llquantize(i++, 10, 0, 6, 20);
	}
	tick-1ms
	/i == 1500/
	{
		exit(0);
	}

##################################################################
name:	ctf-print-1
note:	demonstrate printing a structure based on the ctf data description.
	We use an arbitrary (stupid) symbol (this is a code symbol).
d:
	BEGIN {
		print(*((struct file *)&`dtrace_match_nonzero)); 
		exit(0);
	}
##################################################################
name:	page_fault
note:	See if we can probe this successfully
d:
	fbt::page_fault:{printf("%s", execname);}
	tick-5s: { exit(0); }

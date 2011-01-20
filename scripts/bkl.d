/* Script to monitor calls to the BKL - Big Kernel Lock. */

#pragma D option quiet
int waiting, tcnt;
long time_start;
long time_tot;

fbt::lock_kernel:entry
{
	time_start = timestamp;
	waiting++;
	tcnt++;
	@num[execname] = count();
}
fbt::unlock_kernel:entry
{
	time_tot += waiting ? timestamp - time_start : 0;
	waiting > 0 ? waiting-- : 0;
}
tick-1sec
{
	printf("Locks: %4d Total: %4d  Time/per lock: %4dus  Lock time: %5dus\n", 
		waiting, tcnt, time_tot / tcnt, time_tot);
	printa(@num);
	clear(@num);
}
END {
	printf("Locks acquired: %d\n", tcnt);
}

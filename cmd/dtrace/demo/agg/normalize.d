#pragma D option quiet

BEGIN
{
	/*
	 * Get the start time, in nanoseconds.
	 */
	start = timestamp;
}

syscall:::entry
{
	@func[execname] = count();
}

END
{
	/*
	 * Normalize the aggregation based on the number of seconds we have
	 * been running.  (There are 1,000,000,000 nanoseconds in one second.)
	 */	
	normalize(@func, (timestamp - start) / 1000000000);
}

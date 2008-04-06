profile-5000
{
	/*
	 * We divide by 1,000,000 to convert nanoseconds to milliseconds, and
	 * then we take the value mod 10 to get the current millisecond within
	 * a 10 millisecond window.  On platforms that do not support truly
	 * arbitrary resolution profile probes, all of the profile-5000 probes
	 * will fire on roughly the same millisecond.  On platforms that
	 * support a truly arbitrary resolution, the probe firings will be
	 * evenly distributed across the milliseconds.
	 */
	@ms = lquantize((timestamp / 1000000) % 10, 0, 10, 1);
}

tick-1sec
/i++ >= 10/
{
	exit(0);
}

BEGIN
{
	start = timestamp;
}

/*
 * ... other tracing actions...
 */

END
{
	printf("total time: %d secs", (timestamp - start) / 1000000000);
}

/*
 * Count off and report the number of seconds elapsed
 */
dtrace:::BEGIN
{
	i = 0;
}

profile:::tick-1sec
{
	i = i + 1;
	trace(i);
}

dtrace:::END
{
	trace(i);
}

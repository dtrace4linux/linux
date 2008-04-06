#pragma D option quiet

BEGIN
{
	start = timestamp;
}

syscall:::entry
{
	@func[execname] = count();
}

tick-10sec
{
	normalize(@func, (timestamp - start) / 1000000000);
	printa(@func);
}

#pragma D option quiet

BEGIN
{
	last = timestamp;
}

syscall:::entry
{
	@func[execname] = count();
}

tick-10sec
{
	normalize(@func, (timestamp - last) / 1000000000);
	printa(@func);
	clear(@func);
	last = timestamp;
}

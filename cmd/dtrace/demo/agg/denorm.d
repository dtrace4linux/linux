#pragma D option quiet

BEGIN
{
	start = timestamp;
}

syscall:::entry
{
	@func[execname] = count();
}

END
{
	this->seconds = (timestamp - start) / 1000000000;
	printf("Ran for %d seconds.\n", this->seconds);

	printf("Per-second rate:\n");
	normalize(@func, this->seconds);
	printa(@func);

	printf("\nRaw counts:\n");
	denormalize(@func);
	printa(@func);
}

#pragma D option quiet

io:::start
{
	@[args[1]->dev_statname, execname, pid] = sum(args[0]->b_bcount);
}

END
{
	printf("%10s %20s %10s %15s\n", "DEVICE", "APP", "PID", "BYTES");
	printa("%10s %20s %10d %15@d\n", @);
}

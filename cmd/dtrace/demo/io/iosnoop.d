#pragma D option quiet

BEGIN
{
	printf("%10s %58s %2s\n", "DEVICE", "FILE", "RW");
}

io:::start
{
	printf("%10s %58s %2s\n", args[1]->dev_statname,
	    args[2]->fi_pathname, args[0]->b_flags & B_READ ? "R" : "W");
}

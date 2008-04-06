#pragma D option quiet

proc:::signal-send
{
	@[execname, stringof(args[1]->pr_fname), args[2]] = count();
}

END
{
	printf("%20s %20s %12s %s\n",
	    "SENDER", "RECIPIENT", "SIG", "COUNT");
	printa("%20s %20s %12d %@d\n", @);
}

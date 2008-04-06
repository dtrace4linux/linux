#pragma D option quiet

io:::start
/args[0]->b_flags & B_WRITE/
{
	@[execname, args[2]->fi_dirname] = count();
}

END
{
	printf("%20s %51s %5s\n", "WHO", "WHERE", "COUNT");
	printa("%20s %51s %5@d\n", @);
}

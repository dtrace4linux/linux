#pragma D option quiet

proc:::exec
{
        self->parent = execname;
}

proc:::exec-success
/self->parent != NULL/
{
	@[self->parent, execname] = count();
	self->parent = NULL;
}

proc:::exec-failure
/self->parent != NULL/
{
	self->parent = NULL;
}

END
{
	printf("%-20s %-20s %s\n", "WHO", "WHAT", "COUNT");
	printa("%-20s %-20s %@d\n", @);
}

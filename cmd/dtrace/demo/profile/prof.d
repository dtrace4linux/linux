#pragma D option quiet

profile-97
/pid != 0/
{
	@proc[pid, execname] = count();
}

END
{
	printf("%-8s %-40s %s\n", "PID", "CMD", "COUNT");
	printa("%-8d %-40s %@d\n", @proc);
}

#pragma D option quiet

BEGIN
{
	start = timestamp;
}

sched:::change-pri
/args[1]->pr_pid == $1 && args[0]->pr_lwpid == $2/
{
	printf("%d %d\n", timestamp - start, args[2]);
}

tick-1sec
/++n == 5/
{
	exit(0);
}

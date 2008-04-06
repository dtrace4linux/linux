#pragma D option bufpolicy=ring
#pragma D option bufsize=16k

syscall:::entry
/execname == $1/
{
	trace(timestamp);
}

syscall::rexit:entry
{
	exit(0);
}

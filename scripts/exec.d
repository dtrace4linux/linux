dtrace -n '
#pragma D option quiet
syscall::exec*:entry
{
	printf("%d %s: %s\n", pid, execname, copyinstr(arg0));
}
'

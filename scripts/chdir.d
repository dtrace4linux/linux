dtrace -n '
#pragma D option quiet
syscall::chdir:entry
{
	printf("%d %s: cd %s\n", pid, execname, copyinstr(arg0));
}
'


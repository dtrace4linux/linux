dtrace -n '
#pragma D option quiet
syscall::open*:entry
{
        printf("%d %s: %s %s\n", pid, execname, probefunc, copyinstr(arg0));
}
'

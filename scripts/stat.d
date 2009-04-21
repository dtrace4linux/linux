dtrace -n '
#pragma D option quiet
syscall::lstat*:entry,
syscall::stat*:entry
{
        printf("%d %s: %s %s\n", pid, execname, probefunc, copyinstr(arg0));
}
'

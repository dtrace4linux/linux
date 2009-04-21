dtrace -n '
#pragma D option quiet
syscall::unlink*:entry,
syscall::rmdir*:entry,
syscall::remove*:entry
{
        printf("%d %s: %s %s\n", pid, execname, probefunc, copyinstr(arg0));
}
'

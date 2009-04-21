dtrace -n '
#pragma D option quiet
syscall::open*:entry,
syscall::chdir*:entry,
syscall::chmod*:entry,
syscall::chown*:entry,
syscall::unlink*:entry,
syscall::rmdir*:entry,
syscall::mkdir*:entry,
syscall::remove*:entry
{
        printf("%d %s: %s %s\n", pid, execname, probefunc, copyinstr(arg0));
}
'

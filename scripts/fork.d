dtrace -n '
#pragma D option quiet
syscall::fork*:entry,
syscall::vfork*:entry,
syscall::clone*:entry
{
        printf("%d %s: %s %s\n", pid, execname, probefunc, copyinstr(arg0));
}
'

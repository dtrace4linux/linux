syscall::read:entry,
syscall::write:entry
/pid == $1/
{
	ts[probefunc] = timestamp;
}

syscall::read:return,
syscall::write:return
/pid == $1 && ts[probefunc] != 0/
{
	printf("%d nsecs", timestamp - ts[probefunc]);
}

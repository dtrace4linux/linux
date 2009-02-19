#pragma D option quiet

syscall::mkdir:entry
{
	self->filename = copyinstr(arg0);
}
syscall::mkdir:return
/arg0 < 0/
{
	printf("%d: %s mkdir %s error %d\n", pid, execname, self->filename, errno)
}
syscall::mkdir:return
/arg0 == 0/
{
	printf("%d: %s mkdir %s\n", pid, execname, self->filename)
}



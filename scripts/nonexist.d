#pragma D option quiet

syscall::open:entry
{
	self->filename = copyinstr(arg0);
}
syscall::open:return
{
/*	printf("%s Failed open %x\n", execname, arg0);*/
	printf("%d: %s Failed open %x %s\n", pid, execname, arg0, self->filename);
}

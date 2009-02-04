#pragma D option quiet

syscall::open:entry
{
	self->filename = copyinstr(arg0);
}
syscall::open:return
{
/*	printf("%s Failed open %x\n", execname, arg0);*/
	printf("%s Failed open %x %s\n", execname, arg0, self->filename);
}

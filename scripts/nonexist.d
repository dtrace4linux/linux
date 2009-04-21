#pragma D option quiet

syscall::open:entry
{
	self->filename = copyinstr(arg0);
}
syscall::open:return
/arg0 < 0/
{
/*	printf("%s Failed open %x\n", execname, arg0);*/
	printf("%d: %s : %s\n", 
		pid, execname, 
		self->filename);
}

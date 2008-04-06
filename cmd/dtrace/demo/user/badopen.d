syscall::open:entry
/pid == $1/
{
	self->path = copyinstr(arg0);
}

syscall::open:return
/self->path != NULL && arg1 == -1/
{
	printf("open for '%s' failed", self->path);
	ustack();
}

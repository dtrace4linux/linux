syscall:::entry
/execname == "find"/
{
	self->syscall = probefunc;
	self->insys = 1;
}

sysinfo:::xcalls
/execname == "find"/
{
	@[self->insys ? self->syscall : "<none>"] = count();
}

syscall:::return
/self->insys/
{
	self->insys = 0;
	self->syscall = NULL;
}

syscall::write:entry
{
	self->ts = timestamp;
}

syscall::write:return
/self->ts/
{
	@time[execname] = avg(timestamp - self->ts);
	self->ts = 0;
}

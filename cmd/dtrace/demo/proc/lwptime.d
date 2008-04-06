proc:::lwp-start
/tid != 1/
{
	self->start = timestamp;
}

proc:::lwp-exit
/self->start/
{
	@[execname] = quantize(timestamp - self->start);
	self->start = 0;
}

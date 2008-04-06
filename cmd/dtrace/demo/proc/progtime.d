proc:::start
{
	self->start = timestamp;
}

proc:::exit
/self->start/
{
	@[execname] = quantize(timestamp - self->start);
	self->start = 0;
}

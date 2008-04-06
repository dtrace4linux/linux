sched:::on-cpu
{
	self->ts = timestamp;
}

sched:::off-cpu
/self->ts/
{
	@[cpu] = quantize(timestamp - self->ts);
	self->ts = 0;
}

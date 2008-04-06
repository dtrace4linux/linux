sched:::enqueue
{
	self->ts = timestamp;
}

sched:::dequeue
/self->ts/
{
	@[args[2]->cpu_id] = quantize(timestamp - self->ts);
	self->ts = 0;
}

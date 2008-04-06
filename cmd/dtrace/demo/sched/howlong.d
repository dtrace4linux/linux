sched:::off-cpu
/curlwpsinfo->pr_state == SSLEEP/
{
	self->cpu = cpu;
	self->ts = timestamp;
}

sched:::on-cpu
/self->ts/
{
	@[self->cpu == cpu ?
	    "sleep time, no CPU migration" : "sleep time, CPU migration"] =
	    lquantize((timestamp - self->ts) / 1000000, 0, 500, 25);
	self->ts = 0;
	self->cpu = 0;
}

sched:::enqueue
{
	this->len = qlen[args[2]->cpu_id]++;
	@[args[2]->cpu_id] = lquantize(this->len, 0, 100);
}

sched:::dequeue
/qlen[args[2]->cpu_id]/
{
	qlen[args[2]->cpu_id]?;
}

sched:::tick,
sched:::enqueue
{
	@[probename] = lquantize((timestamp / 1000000) % 10, 0, 10);
}

uint64_t last[int];

sched:::tick
/last[cpu]/
{
	@[cpu] = min(timestamp - last[cpu]);
}

sched:::tick
{
	last[cpu] = timestamp;
}

#pragma D option quiet

sched:::preempt
{
	self->preempt = 1;
}

sched:::remain-cpu
/self->preempt/
{
	self->preempt = 0;
}

sched:::off-cpu
/self->preempt/
{
	/*
	 * If we were told to preempt ourselves, see who we ended up giving
	 * the CPU to.
	 */
	@[stringof(args[1]->pr_fname), args[0]->pr_pri, execname,
	    curlwpsinfo->pr_pri] = count();
	self->preempt = 0;
}

END
{
	printf("%30s %3s %30s %3s %5s\n", "PREEMPTOR", "PRI",
	    "PREEMPTED", "PRI", "#");
	printa("%30s %3d %30s %3d %5@d\n", @);
}

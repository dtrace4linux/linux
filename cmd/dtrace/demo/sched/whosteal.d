#pragma D option quiet

sched:::dequeue
/args[2]->cpu_id != -1 && cpu != args[2]->cpu_id &&
    (curlwpsinfo->pr_flag & PR_IDLE)/
{
	@[stringof(args[1]->pr_fname), args[2]->cpu_id] =
	    lquantize(cpu, 0, 100);
}

END
{
	printa("%s stolen from CPU %d by:\n%@d\n", @);
}

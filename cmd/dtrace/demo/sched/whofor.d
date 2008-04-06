#pragma D option quiet

sched:::sleep
/!(curlwpsinfo->pr_flag & PR_ISSYS) && curlwpsinfo->pr_stype == SOBJ_CV/
{
	bedtime[curlwpsinfo->pr_addr] = timestamp;
}

sched:::wakeup
/bedtime[args[0]->pr_addr]/
{
	@[stringof(args[1]->pr_fname), execname] =
	    quantize(timestamp - bedtime[args[0]->pr_addr]);
	bedtime[args[0]->pr_addr] = 0;
}

END
{
	printa("%s sleeping on %s:\n%@d\n", @);
}

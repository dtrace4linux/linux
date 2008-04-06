sched:::change-pri
{
	@[stringof(args[0]->pr_clname)] =
	    lquantize(args[2] - args[0]->pr_pri, -50, 50, 5);
}

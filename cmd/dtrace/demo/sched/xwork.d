self int last;

sched:::wakeup
/self->last && args[0]->pr_stype == SOBJ_CV/
{
	@[stringof(args[1]->pr_fname)] = sum(vtimestamp - self->last);
	self->last = 0;
}

sched:::wakeup
/execname == "Xsun" && self->last == 0/
{
	self->last = vtimestamp;
}

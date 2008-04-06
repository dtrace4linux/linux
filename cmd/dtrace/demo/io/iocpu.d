#pragma D option quiet

sched:::on-cpu
/execname == "soffice.bin"/
{
	self->on = vtimestamp;
}

sched:::off-cpu
/self->on/
{
	@time["<on cpu>"] = sum(vtimestamp - self->on);
	self->on = 0;
}

io:::wait-start
/execname == "soffice.bin"/
{
	self->wait = timestamp;
}

io:::wait-done
/self->wait/
{
	@io[args[2]->fi_name] = sum(timestamp - self->wait);
	@time["<I/O wait>"] = sum(timestamp - self->wait);
	self->wait = 0;
}

END
{
	printf("Time breakdown (milliseconds):\n");
	normalize(@time, 1000000);
	printa("  %-50s %15@d\n", @time);

	printf("\nI/O wait breakdown (milliseconds):\n");
	normalize(@io, 1000000);
	printa("  %-50s %15@d\n", @io);
}

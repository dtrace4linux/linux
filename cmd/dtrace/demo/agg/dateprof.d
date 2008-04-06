syscall::exec:return,
syscall::exece:return
/execname == "date"/
{
	self->start = vtimestamp;
}

syscall:::entry
/self->start/
{
	/*
	 * We linearly quantize on the current virtual time minus our
	 * process's start time.  We divide by 1000 to yield microseconds
	 * rather than nanoseconds.  The range runs from 0 to 10 milliseconds
	 * in steps of 100 microseconds; we expect that no date(1) process
	 * will take longer than 10 milliseconds to complete.
	 */
	@["system calls over time"] =
	    lquantize((vtimestamp - self->start) / 1000, 0, 10000, 100);
}

syscall::rexit:entry
/self->start/
{
	self->start = 0;
}

vminfo:::maj_fault,
vminfo:::zfod,
vminfo:::as_fault
/execname == "soffice.bin" && start == 0/
{
	/*
	 * This is the first time that a vminfo probe has been hit; record
	 * our initial timestamp.
	 */
	start = timestamp;
}

vminfo:::maj_fault,
vminfo:::zfod,
vminfo:::as_fault
/execname == "soffice.bin"/
{
	/*
	 * Aggregate on the probename, and lquantize() the number of seconds
	 * since our initial timestamp.  (There are 1,000,000,000 nanoseconds
	 * in a second.)  We assume that the script will be terminated before
	 * 60 seconds elapses.
	 */
	@[probename] =
	    lquantize((timestamp - start) / 1000000000, 0, 60);
}

syscall::write:entry
{
	@fds[execname] = lquantize(arg0, 0, 100, 1);
}

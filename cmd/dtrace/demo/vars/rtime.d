syscall::read:entry
{
	self->t = timestamp;
}

syscall::read:return
/self->t != 0/
{
	printf("%d/%d spent %d nsecs in read(2)\n",
	    pid, tid, timestamp - self->t);
	
	/*
	 * We're done with this thread-local variable; assign zero to it to
	 * allow the DTrace runtime to reclaim the underlying storage.
	 */
	self->t = 0;
}

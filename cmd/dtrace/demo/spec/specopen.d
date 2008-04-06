#!/usr/sbin/dtrace -Fs

syscall::open:entry,
syscall::open64:entry
{
	/*
	 * The call to speculation() creates a new speculation.  If this fails,
	 * dtrace(1M) will generate an error message indicating the reason for
	 * the failed speculation(), but subsequent speculative tracing will be
	 * silently discarded.
	 */
	self->spec = speculation();
	speculate(self->spec);

	/*
	 * Because this printf() follows the speculate(), it is being 
	 * speculatively traced; it will only appear in the data buffer if the
	 * speculation is subsequently commited.
	 */
	printf("%s", stringof(copyinstr(arg0)));
}

fbt:::
/self->spec/
{
	/*
	 * A speculate() with no other actions speculates the default action:
	 * tracing the EPID.
	 */
	speculate(self->spec);
}

syscall::open:return,
syscall::open64:return
/self->spec/
{
	/*
	 * To balance the output with the -F option, we want to be sure that
	 * every entry has a matching return.  Because we speculated the
	 * open entry above, we want to also speculate the open return.
	 * This is also a convenient time to trace the errno value.
	 */
	speculate(self->spec);
	trace(errno);
}

syscall::open:return,
syscall::open64:return
/self->spec && errno != 0/
{
	/*
	 * If errno is non-zero, we want to commit the speculation.
	 */
	commit(self->spec);
	self->spec = 0;
}

syscall::open:return,
syscall::open64:return
/self->spec && errno == 0/
{
	/*
	 * If errno is not set, we discard the speculation.
	 */
	discard(self->spec);
	self->spec = 0;
}

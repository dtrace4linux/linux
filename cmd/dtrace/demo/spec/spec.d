syscall::open:entry
{
	self->spec = speculation();
}

syscall:::
/self->spec/
{
	speculate(self->spec);
	printf("this is speculative");
}

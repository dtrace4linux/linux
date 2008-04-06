pid$1::$2:entry
{
	self->spec = speculation();
	speculate(self->spec);
	printf("%x %x %x %x %x", arg0, arg1, arg2, arg3, arg4);
}

pid$1::$2:
/self->spec/
{
	speculate(self->spec);
}

pid$1::$2:return
/self->spec && arg1 == 0/
{
	discard(self->spec);
	self->spec = 0;
}

pid$1::$2:return
/self->spec && arg1 != 0/
{
	commit(self->spec);
	self->spec = 0;
}

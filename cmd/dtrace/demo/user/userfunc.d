pid$1::$2:entry
{
	self->trace = 1;
}

pid$1::$2:return
/self->trace/
{
	self->trace = 0;
}

pid$1:::entry,
pid$1:::return
/self->trace/
{
}

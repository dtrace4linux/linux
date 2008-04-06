fbt::iprbattach:entry
{
	self->trace = 1;
}

fbt:::
/self->trace/
{}

fbt::iprbattach:return
{
	self->trace = 0;
}

fbt::delay:entry,
fbt::drv_usecwait:entry
{
	self->in = timestamp
}

fbt::delay:return,
fbt::drv_usecwait:return
/self->in/
{
	@snoozers[stack()] = quantize(timestamp - self->in);
	self->in = 0;
}

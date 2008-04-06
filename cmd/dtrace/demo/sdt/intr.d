interrupt-start
{
	self->ts = vtimestamp;
}

interrupt-complete
/self->ts/
{
	this->devi = (struct dev_info *)arg0;
	@[stringof(`devnamesp[this->devi->devi_major].dn_name),
	    this->devi->devi_instance] = quantize(vtimestamp - self->ts);
}

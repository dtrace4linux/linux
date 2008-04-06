pid$1:libkstat:kstat_data_lookup:entry
{
	self->ksname = arg1;
}

pid$1:libkstat:kstat_data_lookup:return
/self->ksname != NULL && arg1 != NULL/
{
	this->ksp = (kstat_named_t *) copyin(arg1, sizeof (kstat_named_t));
	printf("%s has ui64 value %u\n",
	    copyinstr(self->ksname), this->ksp->value.ui64);
}

pid$1:libkstat:kstat_data_lookup:return
/self->ksname != NULL && arg1 == NULL/
{
	self->ksname = NULL;
}

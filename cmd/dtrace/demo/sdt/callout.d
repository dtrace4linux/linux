#pragma D option quiet

sdt:::callout-start
{
	@callouts[((callout_t *)arg0)->c_func] = count();
}

tick-1sec
{
	printa("%40a %10@d\n", @callouts);
	clear(@callouts);
}

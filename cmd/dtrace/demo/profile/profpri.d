profile-1001
/pid == $1/
{
	@proc[execname] = lquantize(curlwpsinfo->pr_pri, 0, 100, 10);
}

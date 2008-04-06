lockstat:::adaptive-acquire
/execname == "date"/
{
	@locks["adaptive"] = count();
}

lockstat:::spin-acquire
/execname == "date"/
{
	@locks["spin"] = count();
}

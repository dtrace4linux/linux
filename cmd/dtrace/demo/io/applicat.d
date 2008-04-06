io:::start
/execname == "soffice.bin" && args[2]->fi_name == "applicat.rdb"/
{
	@ = lquantize(args[2]->fi_offset != -1 ?
	    args[2]->fi_offset / (1000 * 1024) : -1, 0, 1000);
}

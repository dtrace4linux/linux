fbt::putnext:entry
{
	@calls[stringof(args[0]->q_qinfo->qi_minfo->mi_idname)] = count();
}

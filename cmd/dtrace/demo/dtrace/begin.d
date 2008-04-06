BEGIN
{
	prot[0] = "---";
	prot[1] = "r--";
	prot[2] = "-w-";
	prot[3] = "rw-";
	prot[4] = "--x";
	prot[5] = "r-x";
	prot[6] = "-wx";
	prot[7] = "rwx";
}

syscall::mmap:entry
{
	printf("mmap with prot = %s", prot[arg2 & 0x7]);
}

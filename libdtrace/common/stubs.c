int
gettaskid()
{
	return 0;
}
int
getprojid()
{
	return 0;
}
int
_lwp_kill(int tid, int sig)
{
	return 0;
}
void *
Paddr_to_map(void *p, unsigned long addr)
{
	printf("stubs.c:Paddr_to_map called -- help!\n");
	return 0;
}

/**********************************************************************/
/*   Program  which  attempts  to  exercise  all  system  calls. The  */
/*   program doesnt do anything useful, but should allow a degree of  */
/*   validation that all syscalls in 32 or 64 kernels will not cause  */
/*   problems.							      */
/**********************************************************************/
# include <stdio.h>
#include <signal.h>

void int_handler()
{
	printf("SIGINT invoked\n");
}
void segv_handler(int sig)
{
	printf("SIGSEGV invoked\n");
}
int main(int argc, char **argv)
{	int	x = 0;

	x += getpid();
	x += getppid();
	x += getuid();
	x += getgid();
	x += setsid();
	x += seteuid();
	x += setegid();
	lseek(0, 0, -1);
	kill(0, 0);
	signal(99, 0);
	signal(SIGINT, int_handler);
	signal(SIGSEGV, segv_handler);
//	*(int *) 0 = 0;
	pipe(0);
	munmap(0, 0);
	mincore(0, 0);
	shmget(0);
	shmat(0);
	dup(-1);
	dup2(-1, -1);
	shmctl(0, 0, 0, 0);
	alarm(9999);
	bind(0, 0, 0);
	socket(0, 0, 0);
	accept(0, 0, 0);
	listen(0);
	shutdown(0);
	getsockname(0, 0, 0);
	getpeername(0, 0, 0);
	truncate(0, 0);
	ftruncate(0, 0);
	if (vfork() == 0)
		exit(0);
	x = opendir("/", 0, 0);
	readdir(x, 0, 0);
	closedir(x);
	chroot("/");
	sigaction(0, 0, 0);
	sigprocmask(0, 0, 0);
	sigreturn(0);
	x += open("/nothing", 0);
	x += chdir("/nothing");
	x += mknod("/nothing/nothing", 0);
	x += ioctl();
	execve("/nothing", NULL, NULL);
	x += close(-2);
	if (fork() == 0)
		exit(0);
	clone();
	brk(0);
	sbrk(0);
	mmap(0, 0, 0, 0, 0);
	uname(0);
	getcwd(0, 0);
	iopl(3);
	unlink("/nothing");
	rmdir("/nothing");
	chmod(0, 0);
	modify_ldt(0);

}

/**********************************************************************/
/*   Program  which  attempts  to  exercise  all  system  calls. The  */
/*   program doesnt do anything useful, but should allow a degree of  */
/*   validation that all syscalls in 32 or 64 kernels will not cause  */
/*   problems.							      */
/**********************************************************************/
# include <stdio.h>
# include <fcntl.h>
# include <stdlib.h>
# include <string.h>
# include <signal.h>
# include <errno.h>
/*# include <sys/vm86.h>*/

static int cnt;
static int line;

static void trace(int line, char *msg);

void alarm_handler()
{
}
/**********************************************************************/
/*   Do something with signals. Dont wait for too long.		      */
/**********************************************************************/
void
do_signals()
{
	sigset_t oldset;
	sigset_t set;
	int	pid;

	trace(__LINE__, "starting do_signals");
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &set, &oldset);
	signal(SIGALRM, alarm_handler);
	pid = getpid();
	if (fork() == 0) {
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 20000;
		select(FD_SETSIZE, NULL, NULL, NULL, &tv);
		kill(pid, SIGCONT);
		exit(0);
		}
	alarm(1);
	getpid();
	trace(__LINE__, "do_signals:calling sigsuspend");
	sigsuspend(&set);
}
void
do_slow()
{	int	pid = getpid();

	if (fork())
		return;

	sync();
	exit(0);
}

void sigchld()
{
}
int
clone_func(void *arg)
{
	return 0;
}
void int_handler()
{
	printf("SIGINT invoked\n");
}
void segv_handler(int sig)
{
	printf("PID:%d SIGSEGV#%d invoked, at syscalls.c:%d\n", getpid(), cnt++, line);
	if (cnt > 10) {
		printf("Something is wrong. We shouldnt be segfaulting whilst dtrace isactive\n");
		exit(0);
	}
}
void sigusr1()
{
#if __i386
	syscall(119, 0, 0, 0); /* sigreturn */
#endif
}
static void
trace(int line, char *msg)
{	char	buf[1024];

	sprintf(buf, "/tracing:/line %d - %s", line, msg);
	open(buf, O_RDONLY);
}

int main(int argc, char **argv)
{	int	x = 0;
	char	*args[10];

	setuid(2);

	signal(SIGCHLD, sigchld);
	do_signals();

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

	line = __LINE__;
	poll(-1, 0, 0);
	signal(SIGSEGV, SIG_IGN);
//	ppoll(-1, -1, -1, 0);
	signal(SIGSEGV, SIG_DFL);
	sched_yield();
	readv(-1, 0, 0, 0);
	writev(-1, 0, 0, 0);
	msync(0, 0, 0);
	fsync(-1);
	fdatasync(-1);
	semget(0, 0, 0);
	semctl(0, 0, 0);
	uselib(NULL);
	pivot_root(0, 0);
	personality(-1);
	setfsuid(-1);
	flock(-1, 0);
	shmdt(0, 0, 0);
	times(0);
	mremap(0, 0, 0, 0, 0);
	madvise(0, 0, 0);
	fchown(-1, 0, 0);
	lchown(0, 0, 0);
	setreuid();
	setregid();
	link("/nonexistant", "/also-nonexistant");

	do_slow();

	symlink("/nothing", "/");
	rename("/", "/");
	mkdir("/junk/stuff////0", 0777);
	geteuid();
	getsid();
	getpgid();
	getresuid();
	getresgid();
	getpgid();
	ptrace(-1, 0, 0, 0);
	semop(0, 0, 0);
	capget(0, 0);

	line = __LINE__;
	gettimeofday(0, 0);
	settimeofday(0, 0);
	dup(-1);
	dup2(-1, -1);
	shmctl(0, 0, 0, 0);
	execve("/bin/nothing", "/bin/nothing", 0);
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
	line = __LINE__;
	if (vfork() == 0)
		exit(0);
	line = __LINE__;
	x = opendir("/", 0, 0);
	line = __LINE__;
	readdir(x, 0, 0);
	line = __LINE__;
	closedir(x);
	line = __LINE__;
	chroot("/");
	line = __LINE__;
	sigaction(0, 0, 0);
	line = __LINE__;
	sigprocmask(0, 0, 0);
	x += open("/nothing", 0);
	x += chdir("/nothing");
	x += mknod("/nothing/nothing", 0);
	x += ioctl();
	execve("/nothing", NULL, NULL);
	line = __LINE__;
	x += close(-2);
	line = __LINE__;
	if (fork() == 0)
		exit(0);
	line = __LINE__;
	clone(clone_func, 0, 0, 0);
	line = __LINE__;
	brk(0);
	sbrk(0);
	line = __LINE__;
	mmap(0, 0, 0, 0, 0);
	line = __LINE__;
	uname(0);
	line = __LINE__;
	getcwd(0, 0);
	line = __LINE__;
	iopl(3);
	ioperm(0, 0, 0);
	mount(0, 0, 0, 0, 0);
	umount(0, 0);
	umount(0, 0, 0);
	swapon(0, 0);
	swapoff(0);
	sethostname(0);
	line = __LINE__;
	time(NULL);
	unlink("/nothing");
	line = __LINE__;
	rmdir("/nothing");
	chmod(0, 0);
	line = __LINE__;
# if defined(__i386) || defined(__amd64)
	modify_ldt(0);
# endif

	stat("/doing-nice", 0);
	nice(0);

	args[0] = "/bin/df";
	args[1] = "-l";
	args[2] = NULL;
	close(1);
	open("/dev/null", O_WRONLY);
	/***********************************************/
	/*   Some  syscalls  arent  available  direct  */
	/*   from  libc,  so get them here. We mostly  */
	/*   care  about  the  ones which have caused  */
	/*   implementation   difficulty  and  kernel  */
	/*   crashes - eventually we can be complete.  */
	/***********************************************/
	line = __LINE__;
	open("/system-dependent-syscalls-follow", 0);
	line = __LINE__;
	if (fork() == 0)
		exit(0);

	{int status;
	while (wait(&status) >= 0)
		;
	}

	sigaltstack(0, 0);

	/*vm86(0, 0);*/

	/***********************************************/
	/*   Some syscalls arent directly accessible,  */
	/*   e.g. legacy.			       */
	/***********************************************/
#if defined(__x86_64__)
	trace(__LINE__, "x64 syscalls");
	syscall(174, 0, 0, 0); // create_module
	syscall(176, 0, 0, 0); // delete_module
	syscall(178, 0, 0, 0); // query_module
#else
	trace(__LINE__, "x32 syscalls");
	syscall(0, 0, 0, 0); // restart_syscall
	syscall(34, 0, 0, 0); // nice
	syscall(59, 0, 0, 0); // oldolduname	
	syscall(109, 0, 0, 0); // olduname	
	if (fork() == 0)
		syscall(1, 0, 0, 0); // exit
#endif
	line = __LINE__;
	execve("/bin/df", args, NULL);

	fprintf(stderr, "Error: should not get here -- %s\n", strerror(errno));

	exit(1);
}

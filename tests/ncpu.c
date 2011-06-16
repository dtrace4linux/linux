#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#define __USE_GNU
#include <sched.h>

int	ncpu;
int	*data;

int worker(int cpuid)
{
	/***********************************************/
	/*   Ensure   we   are  running  on  the  cpu  */
	/*   specified.				       */
	/***********************************************/
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpuid, &mask);
	if (sched_setaffinity(getpid(), 64, &mask) < 0) {
		perror("sched_setaffinity");
//		exit(1);
	}

	/***********************************************/
	/*   Now we are locked to a cpu.	       */
	/***********************************************/

	/***********************************************/
	/*   Wait  for  master  to  give  us the "go"  */
	/*   signal.				       */
	/***********************************************/
	while (data[cpuid] == 0)
		;

	/***********************************************/
	/*   Let master know we saw it.		       */
	/***********************************************/
	data[cpuid] = 2;

	/***********************************************/
	/*   Wait for master to see our response.      */
	/***********************************************/
	while (data[cpuid] == 2)
		;
	exit(0);
}

int main(int argc, char **argv)
{	int	fd;
	FILE	*fp;
	char	buf[BUFSIZ];
	int	i;

	/***********************************************/
	/*   Find out how many cpus on this box.       */
	/***********************************************/
	fp = fopen("/proc/cpuinfo", "r");
	while (fgets(buf, sizeof buf, fp)) {
		if (strncmp(buf, "processor\t:", 11) == 0)
			ncpu++;
	}
	fclose(fp);

	if (argc > 1)
		ncpu = atoi(argv[1]);

	/***********************************************/
	/*   Create  a  shared memory segment to work  */
	/*   with.				       */
	/***********************************************/
	fd = open("/tmp/shm", O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		perror("/tmp/shm");
		exit(1);
	}
	for (i = 0; i < ncpu * sizeof(int); i++) {
		char c = 0;
		write(fd, &c, 1);
	}
	data = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	printf("mmap=%p\n", data);

	/***********************************************/
	/*   Now create one "thread" per cpu.	       */
	/***********************************************/
	memset(data, 0, sizeof(int) * ncpu);
	for (i = 0; i < ncpu; i++) {
		if (fork() == 0) {
			worker(i);
		}
	}

	/***********************************************/
	/*   Tell children to go ahead.		       */
	/***********************************************/
	for (i = 0; i < ncpu; i++) {
		data[i] = 1;
	}

	for (i = 0; i < ncpu; i++) {
		while (data[i] != 2)
			;
		printf("CPU%d at 2\n", i);
	}

	/***********************************************/
	/*   Let children exit.			       */
	/***********************************************/
	for (i = 0; i < ncpu; i++) {
		data[i] = 3;
	}

	/***********************************************/
	/*   Wait for kids to die.		       */
	/***********************************************/
	int status;
	while (wait(&status) >= 0)
		;
	exit(0);
}

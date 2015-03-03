/*
Tool to get old_rsp from the running kernel. Some kernels
dont export this in /proc/kallsyms or /boot/System.map. You
cannot get it from the kernel headers, so we can only
get it from the syscalls that we are intercepting.

Paul Fox Feb 2015
*/

#define _LARGEFILE64_SOURCE
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <fcntl.h>

static int	debug = 0;

static int do_read(char *fname, unsigned long ptr, int remap);

int main(int argc, char **argv)
{	FILE	*fp;
	unsigned char	buf[BUFSIZ];
	unsigned long ptr;
	int	ret;
	char	*name = "stub_execve";

//printf("18\n");
//exit(0);
	if (argc > 1 && strcmp(argv[1], "-d") == 0)
		debug = 1;
	snprintf(buf, sizeof buf,
		"grep %s /proc/kallsyms",
		name);

	if ((fp = popen(buf, "r")) == NULL) {
		perror("grep");
		exit(1);
	}

	if (fgets(buf, sizeof buf, fp) == NULL) {
		fprintf(stderr, "Failed to find %s\n", name);
		exit(1);
	}
	pclose(fp);

	sscanf(buf, "%lx", &ptr);
	if (debug)
		printf("%s=0x%lx\n", name, ptr);

	/***********************************************/
	/*   Centos  5.6  /proc/kcore  is  broken and  */
	/*   unusable. So we try our optional driver.  */
	/***********************************************/
	if (do_read("/proc/kcore", ptr, 1))
		exit(0);

	/***********************************************/
	/*   Centos  5.6 wont let us seek to a kernel  */
	/*   address,  so  truncate  the  number, and  */
	/*   hack   it   back  in  in  the  kmem_read  */
	/*   function.				       */
	/***********************************************/

	/***********************************************/
	/*   Load the driver if we got this far.       */
	/***********************************************/
	snprintf(buf, sizeof buf, "/sbin/insmod build/driver-kmem/dtrace_kmem.ko");
	system(buf);
	ptr &= 0xffffffff;
	ret = do_read("/proc/dtrace_kmem", ptr, 0);
	system("/sbin/rmmod dtrace_kmem");
	if (ret)
		exit(0);
	fprintf(stderr, "Failed to find offset\n");
	exit(1);
}

static int do_read(char *fname, unsigned long ptr, int remap)
{	int	fd, i, n;
	unsigned char	buf[BUFSIZ];

	if ((fd = open64(fname, O_RDONLY | O_LARGEFILE)) < 0)
		return 0;

	/***********************************************/
	/*   Not sure why we have to do this - but we  */
	/*   reflect  the  virtual addr to a physical  */
	/*   one.				       */
	/***********************************************/
	if (remap)
		ptr &= ~0xffff800000000000L;
	if (lseek64(fd, ptr, SEEK_SET) == -1) {
		perror("lseek");
		return 0;
	}

	if ((n = read(fd, buf, sizeof buf)) != sizeof buf) {
		close(fd);
		if (debug)
			fprintf(stderr, "Failed to read buffer from %s, n=%d\n", fname, n);
		return 0;
	}

	for (i = 0; i < n; i++) {
		// mov %gs:nnnn,%r11
		if (memcmp(buf + i, "\x65\x4c\x8b\x1c\x25", 5) == 0) {
			printf("%x\n",
				*(int *) (buf + i + 5));
			exit(0);
		}
	}
	return 0;
}

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include "../include/ctl.h"

char	buf[2 * 1024 * 1024];

static void
dump_mem(char *start, char *mem, char *memend)
{	char	*cp;
	int	i;

        for (cp = mem; cp < memend; cp += 16) {
                printf("%04lx: ", cp - start);
                for (i = 0; i < 16 && cp + i < memend; i++) {
                        printf("%02x ", cp[i] & 0xff);
                }
                printf("\n");
        }
}

int main(int argc, char **argv)
{	int	arg_index = 1;
	int	ret, cmd;
	int	fd;
	ctl_mem_t ctl;

	if (argc < 5) {
		printf("usage: ctl [rd | wr] pid addr len\n");
		exit(1);
	}

	fd = open("/dev/dtrace_ctl", O_RDWR);
	if (fd < 0) {
		perror("/dev/dtrace_ctl");
		exit(1);
		}

	if (strcmp(argv[arg_index++], "rd") == 0) {
		ctl.c_pid = atoi(argv[arg_index++]);
		cmd = CTLIOC_RDMEM;
		ctl.c_dst = buf;
		ctl.c_src = strtoul(argv[arg_index++], NULL, 0);
	} else {
		ctl.c_pid = atoi(argv[arg_index++]);
		cmd = CTLIOC_WRMEM;
		ctl.c_src = buf;
		ctl.c_dst = strtoul(argv[arg_index++], NULL, 0);
	}
	ctl.c_len = atoi(argv[arg_index++]);

	printf("pid=%d src=%p dst=%p len=%d\n",
		ctl.c_pid, ctl.c_src, ctl.c_dst, ctl.c_len);
	if ((ret = ioctl(fd, cmd, &ctl)) < 0) {
		perror("ioctl");
		exit(1);
		}
	printf("ret=%d\n", ret);
	dump_mem(buf, buf, buf + ret);
}

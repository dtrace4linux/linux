#include <stdio.h>

int
cpu_online(int cpu)
{	char	buf[BUFSIZ];
	char	*cp;
	FILE	*fp;
	int	n;

	if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
		return -1;
	while (fgets(buf, sizeof buf, fp)) {
		if (strncmp(buf, "processor\t:", 11) != 0)
			continue;
		cp = buf + 12;
		n = atoi(cp);
		if (n == cpu) {
			fclose(fp);
			return 1;
		}
		if (n >= cpu) {
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return -1;
}

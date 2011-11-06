#define NULL	0

int
libc_strcmp(const char *p1, const char *p2)
{
	while (1) {
		int	ch1 = *p1++;
		int	ch2 = *p2++;
		int	ret = ch2 - ch1;
		if (ret == 0 && ch1 == 0)
			return 0;
		if (ret)
			return ret;
	}
}
int 
libc_strlen(const char *str)
{	int	len = 0;

	while (*str++)
		len++;
	return len;
}
int 
libc_strncmp(const char *s1, const char *s2, int len)
{
	int	c1, c2;

	if (s1 == s2 || len == 0)
		return (0);

	while (len > 0) {
		c1 = *s1++;
		c2 = *s2++;
		if (c1 != c2)
			return c1 - c2;
		len--;
	}

	return (0);
}

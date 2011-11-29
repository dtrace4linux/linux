/*
gcc -DUSERMODE -Dprintk=printf -DDIS_TEXT -I. -o /tmp/instr ../tests/instr.c instr_size.c dis_tables.c
*/
#include <stdio.h>
#include <string.h>

#define uint_t unsigned int
typedef unsigned long size_t;
typedef unsigned char uchar_t;
typedef unsigned long long uint64_t;

#include	"dis_tables.h"


# define	DATAMODEL_LP64 2
# define	DATAMODEL_NATIVE 2

uchar_t *dtrace_instr_modrm(uchar_t *instr);

static int
dtrace_dis_get_byte(void *p)
{
	int ret;
	uchar_t **instr = p;

	ret = **instr;
	*instr += 1;

	return (ret);
}

int
lookup(dis86_t *x, uint64_t addr, char *buf, size_t len)
{
	*buf = '\0';
	return 0;
}
int main(int argc, char **argv)
{	char	buf[32];
	char	ibuf[256];
	char	*bufp = buf;
	char	*bp;
	int	i, ch;
	dis86_t	x;
	uint_t mode = SIZE64;
	int	model = DATAMODEL_LP64;

	if (argc > 1 && strcmp(argv[1], "-64") == 0)
		argc--, argv++;
	else if (argc > 1 && strcmp(argv[1], "-32") == 0) {
		argc--, argv++;
		mode = SIZE32;
		model = DATAMODEL_NATIVE;
		}
	memset(buf, 0, sizeof buf);
	for (i = 1; i < argc; i++) {
		sscanf(argv[i], "%x", &ch);
		buf[i-1] = ch;
		}
	dtrace_instr_dump("", buf);

	mode = (model == DATAMODEL_LP64) ? SIZE64 : SIZE32;

	memset(&x, 0, sizeof x);
	x.d86_data = (void **)&bufp;
	x.d86_get_byte = dtrace_dis_get_byte;
	x.d86_check_func = NULL;
	x.d86_sym_lookup = (void *) lookup;
	x.d86_sprintf_func = snprintf;

	dtrace_disx86(&x, mode);

	dtrace_disx86_str(&x, mode, 0, ibuf, sizeof ibuf);
	printf("  %s\n", ibuf);
	bp = dtrace_instr_modrm(buf);
	printf("  modrm: %d\n", (int) (bp - buf));
}
size_t 
strlcat(char *dst, const char *src, size_t len)
{	int	i;

	while (*dst)
		dst++, len--;

	for (i = 0; *src && i < len; i++)
		*dst++ = *src++;
	*dst = '\0';
	return i;
}


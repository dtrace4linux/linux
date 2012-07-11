
#include <stdlib.h>

int
gmatch(const char *s, const char *p)
{
	return !fnmatch(p, s, 0);
}

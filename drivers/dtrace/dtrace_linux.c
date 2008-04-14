#include "dtrace_linux.h"

int	panic_quiesce;

void *
kmem_alloc(size_t size, int flags)
{
	return kmalloc(size, flags);
}
void
kmem_free(void *ptr, int size)
{
	kfree(ptr);
}

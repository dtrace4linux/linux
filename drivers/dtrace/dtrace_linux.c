#include "dtrace_linux.h"

int	panic_quiesce;
sol_proc_t	*curthread;

void
debug_enter(int arg)
{
	printk("%s(%d): %s\n", __FILE__, __LINE__, __func__);
}
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

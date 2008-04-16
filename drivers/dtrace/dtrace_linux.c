#include "dtrace_linux.h"
#include <sys/dtrace.h>

uintptr_t	_userlimit = 0x7fffffff;
uintptr_t kernelbase;

kmutex_t        cpu_lock;
int	panic_quiesce;
sol_proc_t	*curthread;

void
vcmn_err(int ce, const char *fmt, va_list adx)
{
        char buf[256];

        switch (ce) {
        case CE_CONT:
                snprintf(buf, sizeof(buf), "Solaris(cont): %s\n", fmt);
                break;
        case CE_NOTE:
                snprintf(buf, sizeof(buf), "Solaris: NOTICE: %s\n", fmt);
                break;
        case CE_WARN:
                snprintf(buf, sizeof(buf), "Solaris: WARNING: %s\n", fmt);
                break;
        case CE_PANIC:
                snprintf(buf, sizeof(buf), "Solaris(panic): %s\n", fmt);
                break;
        case CE_IGNORE:
                break;
        default:
                panic("Solaris: unknown severity level");
        }
        if (ce == CE_PANIC)
                panic(buf);
        if (ce != CE_IGNORE)
                printk(buf, adx);
}

void
cmn_err(int type, const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vcmn_err(type, fmt, ap);
        va_end(ap);
}
void
debug_enter(int arg)
{
	printk("%s(%d): %s\n", __FILE__, __LINE__, __func__);
}
static void
dtrace_sync_func(void)
{
}

void
dtrace_sync(void)
{
        dtrace_xcall(DTRACE_CPUALL, (dtrace_xcall_t)dtrace_sync_func, NULL);
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
int
priv_policy_only(const cred_t *a, int b, int c)
{
        return 0;
}
/*
void
dtrace_copy(uintptr_t src, uintptr_t dest, size_t size)
{
	memcpy(dest, src, size);
}
void
dtrace_copystr(uintptr_t uaddr, uintptr_t kaddr, size_t size)
{
	strlcpy(uaddr, kaddr, size);
}

*/

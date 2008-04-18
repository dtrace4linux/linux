#include "dtrace_linux.h"
#include <sys/dtrace.h>
#include <cpumask.h>

uintptr_t	_userlimit = 0x7fffffff;
uintptr_t kernelbase = 0; //_stext;
cpu_core_t cpu_core[CONFIG_NR_CPUS];
cpu_t cpu_table[NCPU];
mutex_t	mod_lock;

kmutex_t        cpu_lock;
int	panic_quiesce;
sol_proc_t	*curthread;

cred_t *
CRED()
{	static cred_t c;

	return &c;
}
hrtime_t
dtrace_gethrtime()
{	struct timeval tv;

	do_gettimeofday(&tv);
	return tv.tv_sec * 1000 * 1000 * 1000 + tv.tv_usec * 1000;
}
uint64_t
dtrace_gethrestime()
{
	return dtrace_gethrtime();
}
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
static int
dtrace_xcall_func(dtrace_xcall_t func, void *arg)
{
        (*func)(arg);

        return (0);
}
/*ARGSUSED*/
void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t func, void *arg)
{
	cpuset_t set;

	CPUSET_ZERO(set);

	if (cpu == DTRACE_CPUALL) {
		CPUSET_ALL(set);
	} else {
		CPUSET_ADD(set, cpu);
	}

	kpreempt_disable();
	smp_call_function_mask(set, func, arg, TRUE);
/*
	xc_sync((xc_arg_t)func, (xc_arg_t)arg, 0, X_CALL_HIPRI, set,
		(xc_func_t)dtrace_xcall_func);
*/
	kpreempt_enable();
}

void *
kmem_alloc(size_t size, int flags)
{
	return kmalloc(size, flags);
}
void *
kmem_zalloc(size_t size, int flags)
{	void *ptr = kmalloc(size, flags);

	if (ptr)
		memset(ptr, 0, size);
	return ptr;
}
void
kmem_free(void *ptr, int size)
{
	kfree(ptr);
}
void *
vmem_alloc(vmem_t *hdr, size_t s, int flags)
{
	return kmem_cache_alloc(hdr, flags);
}

void
vmem_free(vmem_t *hdr, void *ptr, size_t s)
{
	kmem_cache_free(hdr, ptr);
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

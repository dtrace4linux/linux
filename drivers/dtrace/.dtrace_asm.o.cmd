cmd_/home/fox/src/dtrace/drivers/dtrace/dtrace_asm.o := gcc -Wp,-MD,/home/fox/src/dtrace/drivers/dtrace/.dtrace_asm.o.d  -nostdinc -isystem /usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include -D__KERNEL__ -Iinclude  -include include/linux/autoconf.h -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Os  -mtune=generic -m64 -mno-red-zone -mcmodel=kernel -pipe -Wno-sign-compare -fno-asynchronous-unwind-tables -funit-at-a-time -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -maccumulate-outgoing-args -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -fstack-protector -fomit-frame-pointer -g  -fno-stack-protector -Wdeclaration-after-statement -Wno-pointer-sign   -I/home/fox/src/dtrace/drivers/dtrace/../.. -I/home/fox/src/dtrace/drivers/dtrace/../../sys -I/home/fox/src/dtrace/drivers/dtrace/../../linux -I/home/fox/src/dtrace/drivers/dtrace/../../common/ctf -I/home/fox/src/dtrace/drivers/dtrace/../../uts/common -I/home/fox/src/dtrace/drivers/dtrace/../include -D_KERNEL  -DMODULE -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(dtrace_asm)"  -D"KBUILD_MODNAME=KBUILD_STR(dtracedrv)" -c -o /home/fox/src/dtrace/drivers/dtrace/dtrace_asm.o /home/fox/src/dtrace/drivers/dtrace/dtrace_asm.c

deps_/home/fox/src/dtrace/drivers/dtrace/dtrace_asm.o := \
  /home/fox/src/dtrace/drivers/dtrace/dtrace_asm.c \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/linux_types.h \
  /home/fox/src/dtrace/drivers/dtrace/../include/features.h \
  /home/fox/src/dtrace/drivers/dtrace/../../sys/time.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbd.h) \
    $(wildcard include/config/lsf.h) \
    $(wildcard include/config/resources/64bit.h) \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/forced/inlining.h) \
  include/linux/compiler-gcc.h \
  include/asm/posix_types.h \
    $(wildcard include/config/x86/32.h) \
  include/asm/posix_types_64.h \
  include/asm/types.h \
    $(wildcard include/config/x86/64.h) \
    $(wildcard include/config/highmem64g.h) \
  include/linux/cache.h \
    $(wildcard include/config/smp.h) \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/spinlock/sleep.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/numa.h) \
  /usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stdarg.h \
  include/linux/linkage.h \
  include/asm/linkage.h \
  include/asm/linkage_64.h \
  include/linux/bitops.h \
  include/asm/bitops.h \
  include/asm/bitops_64.h \
  include/asm/alternative.h \
  include/asm/alternative_64.h \
    $(wildcard include/config/paravirt.h) \
  include/asm/cpufeature.h \
    $(wildcard include/config/x86/invlpg.h) \
  include/asm/required-features.h \
    $(wildcard include/config/x86/minimum/cpu/family.h) \
    $(wildcard include/config/math/emulation.h) \
    $(wildcard include/config/x86/pae.h) \
    $(wildcard include/config/x86/cmov.h) \
    $(wildcard include/config/x86/use/3dnow.h) \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/ext2-non-atomic.h \
  include/asm-generic/bitops/le.h \
  include/asm/byteorder.h \
    $(wildcard include/config/x86/bswap.h) \
  include/linux/byteorder/little_endian.h \
  include/linux/byteorder/swab.h \
  include/linux/byteorder/generic.h \
  include/asm-generic/bitops/minix.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
  include/asm/cache.h \
    $(wildcard include/config/x86/l1/cache/shift.h) \
    $(wildcard include/config/x86/vsmp.h) \
  include/linux/seqlock.h \
  include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  include/linux/thread_info.h \
  include/asm/thread_info.h \
  include/asm/thread_info_64.h \
    $(wildcard include/config/debug/stack/usage.h) \
  include/asm/page.h \
  include/asm/page_64.h \
    $(wildcard include/config/physical/start.h) \
    $(wildcard include/config/flatmem.h) \
  include/linux/const.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
    $(wildcard include/config/out/of/line/pfn/to/page.h) \
  include/asm-generic/page.h \
  include/asm/pda.h \
    $(wildcard include/config/cc/stackprotector.h) \
  include/asm/mmsegment.h \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/poison.h \
  include/linux/prefetch.h \
  include/asm/processor.h \
  include/asm/processor_64.h \
    $(wildcard include/config/mpsc.h) \
    $(wildcard include/config/mcore2.h) \
  include/asm/segment.h \
  include/asm/segment_64.h \
  include/asm/sigcontext.h \
  include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  include/asm/msr.h \
  include/asm/msr-index.h \
  include/linux/errno.h \
  include/asm/errno.h \
  include/asm-generic/errno.h \
  include/asm-generic/errno-base.h \
  include/asm/current.h \
  include/asm/current_64.h \
  include/asm/system.h \
  include/asm/system_64.h \
    $(wildcard include/config/ia32/emulation.h) \
  include/asm/cmpxchg.h \
  include/asm/cmpxchg_64.h \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
    $(wildcard include/config/x86.h) \
  include/asm/irqflags.h \
  include/asm/irqflags_64.h \
  include/asm/processor-flags.h \
  include/asm/percpu.h \
  include/asm/percpu_64.h \
  include/linux/personality.h \
  include/linux/cpumask.h \
    $(wildcard include/config/hotplug/cpu.h) \
  include/linux/bitmap.h \
  include/linux/string.h \
  include/asm/string.h \
  include/asm/string_64.h \
  include/linux/stringify.h \
  include/linux/bottom_half.h \
  include/linux/spinlock_types.h \
  include/asm/spinlock_types.h \
  include/linux/lockdep.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/generic/hardirqs.h) \
    $(wildcard include/config/prove/locking.h) \
  include/asm/spinlock.h \
  include/asm/spinlock_64.h \
  include/asm/atomic.h \
  include/asm/atomic_64.h \
  include/asm-generic/atomic.h \
  include/asm/rwlock.h \
  include/linux/spinlock_api_smp.h \
  /home/fox/src/dtrace/drivers/dtrace/../../sys/time.h \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/sys/processor.h \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/sys/systm.h \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/sys/vmem.h \
  /home/fox/src/dtrace/drivers/dtrace/../../sys/types.h \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/sys/cred.h \
  /home/fox/src/dtrace/drivers/dtrace/../include/stdint.h \
  /home/fox/src/dtrace/drivers/dtrace/../../sys/unistd.h \
  include/asm/unistd.h \
  include/asm/unistd_64.h \
  /home/fox/src/dtrace/drivers/dtrace/../../sys/wait.h \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/zone.h \
  /home/fox/src/dtrace/drivers/dtrace/../include/sys/cpuvar_defs.h \
  include/asm/signal.h \
  include/linux/time.h \
  include/asm-generic/signal.h \
  /home/fox/src/dtrace/drivers/dtrace/../../sys/sched.h \
    $(wildcard include/config/sched/debug.h) \
    $(wildcard include/config/no/hz.h) \
    $(wildcard include/config/detect/softlockup.h) \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/keys.h) \
    $(wildcard include/config/bsd/process/acct.h) \
    $(wildcard include/config/taskstats.h) \
    $(wildcard include/config/audit.h) \
    $(wildcard include/config/inotify/user.h) \
    $(wildcard include/config/posix/mqueue.h) \
    $(wildcard include/config/fair/user/sched.h) \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/schedstats.h) \
    $(wildcard include/config/task/delay/acct.h) \
    $(wildcard include/config/fair/group/sched.h) \
    $(wildcard include/config/blk/dev/io/trace.h) \
    $(wildcard include/config/sysvipc.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/rt/mutexes.h) \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/task/xacct.h) \
    $(wildcard include/config/cpusets.h) \
    $(wildcard include/config/cgroups.h) \
    $(wildcard include/config/futex.h) \
    $(wildcard include/config/compat.h) \
    $(wildcard include/config/fault/injection.h) \
  include/asm/param.h \
    $(wildcard include/config/hz.h) \
  include/linux/capability.h \
  include/linux/timex.h \
  include/asm/timex.h \
    $(wildcard include/config/x86/elan.h) \
  include/asm/tsc.h \
    $(wildcard include/config/x86/tsc.h) \
    $(wildcard include/config/x86/generic.h) \
  include/linux/jiffies.h \
  include/linux/calc64.h \
  include/asm/div64.h \
  include/asm-generic/div64.h \
  include/linux/rbtree.h \
  include/linux/nodemask.h \
    $(wildcard include/config/highmem.h) \
  include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  include/linux/mm_types.h \
    $(wildcard include/config/mmu.h) \
  include/linux/auxvec.h \
  include/asm/auxvec.h \
  include/linux/prio_tree.h \
  include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  include/linux/rwsem-spinlock.h \
  include/linux/completion.h \
  include/linux/wait.h \
  include/asm/mmu.h \
  include/linux/mutex.h \
  include/asm/semaphore.h \
  include/asm/semaphore_64.h \
  include/asm/ptrace.h \
  include/asm/ptrace-abi.h \
  include/asm/cputime.h \
  include/asm-generic/cputime.h \
  include/linux/smp.h \
  include/asm/smp.h \
  include/asm/smp_64.h \
  include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/acpi/hotplug/memory.h) \
  include/asm/mpspec.h \
  include/asm/mpspec_64.h \
    $(wildcard include/config/acpi.h) \
  include/asm/apic.h \
  include/asm/apic_64.h \
    $(wildcard include/config/x86/good/apic.h) \
  include/linux/pm.h \
    $(wildcard include/config/pm/sleep.h) \
  include/linux/delay.h \
  include/asm/delay.h \
  include/asm/fixmap.h \
  include/asm/fixmap_64.h \
  include/asm/apicdef.h \
  include/asm/apicdef_64.h \
  include/asm/vsyscall.h \
    $(wildcard include/config/generic/time.h) \
  include/asm/io_apic.h \
  include/asm/io_apic_64.h \
  include/linux/sem.h \
  include/linux/ipc.h \
  include/asm/ipcbuf.h \
  include/linux/kref.h \
  include/asm/sembuf.h \
  include/linux/signal.h \
  include/asm/siginfo.h \
  include/asm-generic/siginfo.h \
  include/linux/securebits.h \
  include/linux/fs_struct.h \
  include/linux/pid.h \
  include/linux/rcupdate.h \
  include/linux/percpu.h \
  include/linux/slab.h \
    $(wildcard include/config/slab/debug.h) \
    $(wildcard include/config/slub.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/debug/slab.h) \
    $(wildcard include/config/slabinfo.h) \
  include/linux/gfp.h \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
  include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/arch/populates/node/map.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/need/node/memmap/size.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/nodes/span/other/nodes.h) \
    $(wildcard include/config/holes/in/zone.h) \
  include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/linux/memory_hotplug.h \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
  include/linux/notifier.h \
  include/linux/srcu.h \
  include/linux/topology.h \
    $(wildcard include/config/sched/smt.h) \
    $(wildcard include/config/sched/mc.h) \
  include/asm/topology.h \
  include/asm/topology_64.h \
    $(wildcard include/config/acpi/numa.h) \
  include/asm-generic/topology.h \
  include/asm/mmzone.h \
  include/asm/mmzone_64.h \
    $(wildcard include/config/numa/emu.h) \
  include/asm/sparsemem.h \
  include/asm/sparsemem_64.h \
  include/linux/slub_def.h \
    $(wildcard include/config/slub/debug.h) \
  include/linux/workqueue.h \
  include/linux/timer.h \
    $(wildcard include/config/timer/stats.h) \
  include/linux/ktime.h \
    $(wildcard include/config/ktime/scalar.h) \
  include/linux/kobject.h \
  include/linux/sysfs.h \
  include/linux/proportions.h \
  include/linux/percpu_counter.h \
  include/linux/seccomp.h \
    $(wildcard include/config/seccomp.h) \
  include/asm/seccomp.h \
  include/asm/seccomp_64.h \
  include/linux/unistd.h \
  include/asm/ia32_unistd.h \
  include/linux/futex.h \
  include/linux/sched.h \
    $(wildcard include/config/utrace.h) \
    $(wildcard include/config/ptrace.h) \
  include/linux/rtmutex.h \
    $(wildcard include/config/debug/rt/mutexes.h) \
  include/linux/plist.h \
    $(wildcard include/config/debug/pi/list.h) \
  include/linux/param.h \
  include/linux/resource.h \
  include/asm/resource.h \
  include/asm-generic/resource.h \
  include/linux/hrtimer.h \
    $(wildcard include/config/high/res/timers.h) \
  include/linux/task_io_accounting.h \
    $(wildcard include/config/task/io/accounting.h) \
  include/linux/aio.h \
  include/linux/aio_abi.h \
  include/linux/uio.h \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/sys/cpuvar.h \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/sys/types32.h \
  /home/fox/src/dtrace/drivers/dtrace/../../linux/sys/int_types.h \

/home/fox/src/dtrace/drivers/dtrace/dtrace_asm.o: $(deps_/home/fox/src/dtrace/drivers/dtrace/dtrace_asm.o)

$(deps_/home/fox/src/dtrace/drivers/dtrace/dtrace_asm.o):

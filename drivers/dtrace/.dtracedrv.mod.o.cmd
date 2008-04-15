cmd_/home/fox/src/dtrace/drivers/dtrace/dtracedrv.mod.o := gcc -Wp,-MD,/home/fox/src/dtrace/drivers/dtrace/.dtracedrv.mod.o.d  -nostdinc -isystem /usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include -D__KERNEL__ -Iinclude  -include include/linux/autoconf.h -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Os  -mtune=generic -m64 -mno-red-zone -mcmodel=kernel -pipe -Wno-sign-compare -fno-asynchronous-unwind-tables -funit-at-a-time -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -maccumulate-outgoing-args -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -fstack-protector -fomit-frame-pointer -g  -fno-stack-protector -Wdeclaration-after-statement -Wno-pointer-sign    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(dtracedrv.mod)"  -D"KBUILD_MODNAME=KBUILD_STR(dtracedrv)" -DMODULE -c -o /home/fox/src/dtrace/drivers/dtrace/dtracedrv.mod.o /home/fox/src/dtrace/drivers/dtrace/dtracedrv.mod.c

deps_/home/fox/src/dtrace/drivers/dtrace/dtracedrv.mod.o := \
  /home/fox/src/dtrace/drivers/dtrace/dtracedrv.mod.c \
  include/linux/module.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/module/unload.h) \
    $(wildcard include/config/kallsyms.h) \
    $(wildcard include/config/markers.h) \
    $(wildcard include/config/sysfs.h) \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/forced/inlining.h) \
  include/linux/compiler-gcc.h \
  include/linux/poison.h \
  include/linux/prefetch.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbd.h) \
    $(wildcard include/config/lsf.h) \
    $(wildcard include/config/resources/64bit.h) \
  include/linux/posix_types.h \
  include/asm/posix_types.h \
    $(wildcard include/config/x86/32.h) \
  include/asm/posix_types_64.h \
  include/asm/types.h \
    $(wildcard include/config/x86/64.h) \
    $(wildcard include/config/highmem64g.h) \
  include/asm/processor.h \
  include/asm/processor_64.h \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/x86/vsmp.h) \
    $(wildcard include/config/mpsc.h) \
    $(wildcard include/config/mcore2.h) \
  include/asm/segment.h \
  include/asm/segment_64.h \
  include/asm/cache.h \
    $(wildcard include/config/x86/l1/cache/shift.h) \
  include/asm/page.h \
  include/asm/page_64.h \
    $(wildcard include/config/physical/start.h) \
    $(wildcard include/config/flatmem.h) \
  include/linux/const.h \
  include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  include/asm-generic/bug.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
    $(wildcard include/config/out/of/line/pfn/to/page.h) \
  include/asm-generic/page.h \
  include/asm/sigcontext.h \
  include/asm/cpufeature.h \
    $(wildcard include/config/x86/invlpg.h) \
  include/linux/bitops.h \
  include/asm/bitops.h \
  include/asm/bitops_64.h \
  include/asm/alternative.h \
  include/asm/alternative_64.h \
    $(wildcard include/config/paravirt.h) \
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
  include/asm/required-features.h \
    $(wildcard include/config/x86/minimum/cpu/family.h) \
    $(wildcard include/config/math/emulation.h) \
    $(wildcard include/config/x86/pae.h) \
    $(wildcard include/config/x86/cmov.h) \
    $(wildcard include/config/x86/use/3dnow.h) \
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
  include/asm/pda.h \
    $(wildcard include/config/cc/stackprotector.h) \
  include/linux/cache.h \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/spinlock/sleep.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/numa.h) \
  /usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.2.0/include/stdarg.h \
  include/linux/linkage.h \
  include/asm/linkage.h \
  include/asm/linkage_64.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
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
    $(wildcard include/config/debug/lock/alloc.h) \
  include/asm/processor-flags.h \
  include/asm/mmsegment.h \
  include/asm/percpu.h \
  include/asm/percpu_64.h \
  include/linux/personality.h \
  include/linux/cpumask.h \
    $(wildcard include/config/hotplug/cpu.h) \
  include/linux/bitmap.h \
  include/linux/string.h \
  include/asm/string.h \
  include/asm/string_64.h \
  include/linux/stat.h \
  include/asm/stat.h \
  include/linux/time.h \
  include/linux/seqlock.h \
  include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/preempt.h) \
  include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  include/linux/thread_info.h \
  include/asm/thread_info.h \
  include/asm/thread_info_64.h \
    $(wildcard include/config/debug/stack/usage.h) \
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
  include/linux/kmod.h \
    $(wildcard include/config/kmod.h) \
  include/linux/elf.h \
  include/linux/elf-em.h \
  include/asm/elf.h \
  include/asm/ptrace.h \
  include/asm/ptrace-abi.h \
  include/asm/user.h \
  include/asm/user_64.h \
  include/asm/auxvec.h \
  include/linux/kobject.h \
    $(wildcard include/config/hotplug.h) \
  include/linux/sysfs.h \
  include/linux/kref.h \
  include/linux/wait.h \
  include/linux/moduleparam.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/ppc64.h) \
  include/linux/init.h \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/acpi/hotplug/memory.h) \
  include/linux/marker.h \
  include/asm/local.h \
  include/asm/local_64.h \
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
    $(wildcard include/config/highmem.h) \
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
  include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  include/linux/nodemask.h \
  include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/linux/memory_hotplug.h \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
  include/linux/notifier.h \
  include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
  include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  include/linux/rwsem-spinlock.h \
  include/linux/srcu.h \
  include/linux/topology.h \
    $(wildcard include/config/sched/smt.h) \
    $(wildcard include/config/sched/mc.h) \
  include/linux/smp.h \
  include/asm/smp.h \
  include/asm/smp_64.h \
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
  include/linux/jiffies.h \
  include/linux/calc64.h \
  include/asm/div64.h \
  include/asm-generic/div64.h \
  include/linux/timex.h \
    $(wildcard include/config/no/hz.h) \
  include/asm/param.h \
    $(wildcard include/config/hz.h) \
  include/asm/timex.h \
    $(wildcard include/config/x86/elan.h) \
  include/asm/tsc.h \
    $(wildcard include/config/x86/tsc.h) \
    $(wildcard include/config/x86/generic.h) \
  include/asm/module.h \
  include/asm/module_64.h \
  include/linux/vermagic.h \
  include/linux/utsrelease.h \

/home/fox/src/dtrace/drivers/dtrace/dtracedrv.mod.o: $(deps_/home/fox/src/dtrace/drivers/dtrace/dtracedrv.mod.o)

$(deps_/home/fox/src/dtrace/drivers/dtrace/dtracedrv.mod.o):

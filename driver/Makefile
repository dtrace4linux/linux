
dtracedrv-objs := \
	ctf_hash.o \
	ctf_lookup.o \
	ctf_mod.o \
	ctf_open.o \
	ctf_struct.o \
	ctf_subr.o \
	ctf_types.o \
	ctf_util.o \
	ctl.o \
	cpu_x86.o \
	cyclic_linux.o \
	dis_tables.o \
	mutex.o \
	dtrace.o \
	dtrace_asm.o \
	dtrace_isa.o \
	dtrace_linux.o \
	dtrace_subr.o \
	dwarf.o \
	fasttrap.o \
	fasttrap_isa.o \
	fasttrap_linux.o \
	fbt_linux.o \
	instr_linux.o \
	intr_x86-32.o \
	intr_x86-64.o \
	libc.o \
	missing.o \
	printf.o \
	profile.o \
	prov_common.o \
	sdt_linux.o \
	sdt_subr.o \
	systrace.o \
	tcp.o \
	toxic.o \
	instr_size.o \
	uncompress.o \
	vminfo.o \
	x_call.o
ifdef BUILD_i386
	###############################################
	#   Kernel  may  be  using  -regparm, and we  #
	#   cannot  call the funcs in libgcc.a since  #
	#   they  are  the wrong calling convention.  #
	#   Use  divmod64.c  to  trampoline into the  #
	#   real functions.			      #
	###############################################
	dtracedrv-objs += divmod64.o
	LDFLAGS += --wrap __moddi3 
	LDFLAGS += --wrap __udivdi3 
	LDFLAGS += --wrap __umoddi3 
	LDFLAGS += --wrap __divdi3 
	dtracedrv-objs += ../libgcc.a
endif

obj-m := dtracedrv.o

H	= dtrace_linux.h dtrace_proto.h
EXTRA_CFLAGS += -g \
	-I$(M) \
	-I$(M)/../.. \
	-I$(M)/../../include \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-D_KERNEL -DCTF_OLD_VERSIONS $(PTR32)

######################################################################
#   Handle  include  files  which  exist  or  moved  around amongst  #
#   different kernel releases.					     #
#   20110415 We are moving this to tools/mkport.pl and build/port.h  #
#   since OpenSuse is proving challenging to keep nice and simple.   #
######################################################################
EXTRA_CFLAGS += $(shell test -f include/asm/kdebug.h && echo -DHAVE_INCLUDE_ASM_KDEBUG_H)
EXTRA_CFLAGS += $(shell test -f include/linux/kdebug.h && echo -DHAVE_INCLUDE_LINUX_KDEBUG_H)
EXTRA_CFLAGS += $(shell test -f include/linux/hrtimer.h && echo -DHAVE_INCLUDE_LINUX_HRTIMER_H)
EXTRA_CFLAGS += $(shell test -f include/linux/mutex.h && echo -DHAVE_INCLUDE_LINUX_MUTEX_H)
EXTRA_CFLAGS += $(shell test -f include/asm/mutex.h && echo -DHAVE_INCLUDE_ASM_MUTEX_H)
EXTRA_CFLAGS += $(shell test -f include/linux/stacktrace.h && echo -DHAVE_INCLUDE_LINUX_STACKTRACE_H)
EXTRA_CFLAGS += $(shell test -f include/asm/stacktrace.h && echo -DHAVE_INCLUDE_ASM_STACKTRACE_H)
EXTRA_CFLAGS += $(shell test -f include/linux/semaphore.h && echo -DHAVE_INCLUDE_LINUX_SEMAPHORE_H)
EXTRA_CFLAGS += $(shell test -f include/asm/semaphore.h && echo -DHAVE_INCLUDE_ASM_SEMAPHORE_H)
EXTRA_CFLAGS += $(shell test -f include/asm/msr-index.h && echo -DHAVE_INCLUDE_ASM_MSR_INDEX_H)
EXTRA_CFLAGS += $(shell grep -s atomic.*count include/*/semaphore*.h >/dev/null && echo -DHAVE_SEMAPHORE_ATOMIC_COUNT)

clean:
	rm -rf .tmp_versions *.a *.o *.ko *.bak .*.d .*.cmd *.mod.c Module.* modules.order

unl:
	setuid root rmmod dtracedrv

echo:
	echo $(EXTRA_CFLAGS)

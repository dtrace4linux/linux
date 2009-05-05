
# no longer needed.
#	cpu_32bit.o
#	cpu_64bit.o
dtracedrv-objs := \
	ctf_subr.o \
	ctf_types.o \
	ctf_hash.o \
	ctf_util.o \
	ctf_mod.o \
	ctf_open.o \
	ctf_lookup.o \
	ctl.o \
	cyclic_linux.o \
	dis_tables.o \
	dtrace.o \
	dtrace_asm.o \
	dtrace_isa.o \
	dtrace_linux.o \
	dtrace_subr.o \
	fasttrap.o \
	fasttrap_isa.o \
	fasttrap_linux.o \
	fbt_linux.o \
	intr_x86-32.o \
	intr_x86-64.o \
	linux_pure.o \
	missing.o \
	profile.o \
	sdt_linux.o \
	sdt_subr.o \
	systrace.o \
	toxic.o \
	instr_size.o \
	uncompress.o
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
EXTRA_CFLAGS += \
	-I$(M) \
	-I$(M)/../.. \
	-I$(M)/../../include \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-D_KERNEL -DCTF_OLD_VERSIONS $(PTR32)
clean:
	rm -rf .tmp_versions *.a *.o *.ko *.bak .*.d .*.cmd *.mod.c Module.* modules.order

unl:
	setuid root rmmod dtracedrv

echo:
	echo $(EXTRA_CFLAGS)

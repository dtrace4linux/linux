dtracedrv-objs := \
	ctf_subr.o \
	ctf_types.o \
	ctf_hash.o \
	ctf_util.o \
	ctf_mod.o \
	ctf_open.o \
	ctf_lookup.o \
	cyclic.o \
	dis_tables.o \
	dtrace.o \
	dtrace_asm.o \
	dtrace_isa.o \
	dtrace_linux.o \
	dtrace_subr.o \
	fasttrap_linux.o \
	fbt_linux.o \
	sdt_linux.o \
	sdt_subr.o \
	instr_size.o \
	uncompress.o

obj-m := dtracedrv.o

H	= dtrace_linux.h dtrace_proto.h

EXTRA_CFLAGS += \
	-I$(M) \
	-I$(M)/../.. \
	-I$(M)/../include \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-D_KERNEL -DCTF_OLD_VERSIONS

clean:
	rm -rf .tmp_versions *.a *.o *.ko *.bak .*.d .*.cmd *.mod.c Module.symvers

unl:
	setuid root rmmod dtracedrv

echo:
	echo $(EXTRA_CFLAGS)

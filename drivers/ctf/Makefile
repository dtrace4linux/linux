obj-m := ctf.o

ctf-objs = \
	ctf_subr.o \
	ctf_types.o \
	ctf_hash.o \
	ctf_util.o \
	ctf_mod.o \
	ctf_open.o \
	ctf_lookup.o \
	uncompress.o

EXTRA_CFLAGS += \
	-I$(M)/../.. \
	-I$(M)/../include \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-I$(M)/../dtrace \
	-D_KERNEL -DCTF_OLD_VERSIONS

clean:
	rm -rf .tmp_versions *.o *.ko *.bak .*.d .*.cmd *.mod.c Module.symvers


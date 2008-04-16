dtracedrv-objs := \
	dtrace.o \
	dtrace_asm.o \
	dtrace_isa.o \
	dtrace_linux.o \
	dtrace_subr.o

obj-m += dtracedrv.o

EXTRA_CFLAGS += \
	-I$(M)/../.. \
	-I$(M)/../../sys \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-I$(M)/../include \
	-D_KERNEL
#CFLAGS += -I$(M)/../../common/ctf -I$(M)/../../uts/common/sys -D_KERNEL

dtracedrv.o: $(dtracedrv-objs)
	$(LD) -r -o $@ $(dtrace-objs)

clean:
	rm -f *.o *.ko *.bak .*.cmd *.mod.c

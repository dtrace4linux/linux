#! /bin/make
#
# Dont run this directly - use the make-me script

dtracedrv-objs := \
	dtrace.o \
	dtrace_asm.o \
	dtrace_isa.o \
	dtrace_linux.o \
	dtrace_subr.o

obj-m += dtracedrv.o

EXTRA_CFLAGS += \
	-I$(M)/../.. \
	-I$(M)/../include \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-D_KERNEL
#CFLAGS += -I$(M)/../../common/ctf -I$(M)/../../uts/common/sys -D_KERNEL
dtracedrv.o: $(dtracedrv-objs)
	$(LD) -r -o $@ $(dtrace-objs)

$(dtracedrv-objs): dtrace_linux.h $(M)/../../linux/linux_types.h

clean:
	rm -f *.o *.ko *.bak .*.d .*.cmd *.mod.c
ins:
	setuid root insmod dtracedrv.ko
rm unl:
	setuid root rmmod dtracedrv


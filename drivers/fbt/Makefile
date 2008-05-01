#! /bin/make
#
# Dont run this directly - use the make-me script

fbt-objs := \
	dtrace_subr.o \
	fbt_linux.o

obj-m += fbt.o

EXTRA_CFLAGS += \
	-I../dtrace \
	-I$(M)/../.. \
	-I$(M)/../include \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-D_KERNEL
#CFLAGS += -I$(M)/../../common/ctf -I$(M)/../../uts/common/sys -D_KERNEL
fbt.o: $(fbt-objs)
	$(LD) -r -o $@ $(fbt-objs)

$(fbt-objs): $(M)/../../linux/linux_types.h

clean:
	rm -f *.o *.ko *.bak .*.d .*.cmd *.mod.c
ins:
	sync ; sync
	-setuid root rmmod fbt
	sync ; sync
	setuid root insmod fbt.ko
unl:
	setuid root rmmod fbt


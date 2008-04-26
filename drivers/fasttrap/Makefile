#! /bin/make
#
# Dont run this directly - use the make-me script

fasttrap-objs := \
	fasttrap_linux.o

obj-m += fasttrap.o

EXTRA_CFLAGS += \
	-I../dtrace \
	-I$(M)/../.. \
	-I$(M)/../include \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-D_KERNEL
#CFLAGS += -I$(M)/../../common/ctf -I$(M)/../../uts/common/sys -D_KERNEL
fasttrap.o: $(fasttrap-objs)
	$(LD) -r -o $@ $(dtrace-objs)

$(fasttrap-objs): $(M)/../../linux/linux_types.h

clean:
	rm -f *.o *.ko *.bak .*.d .*.cmd *.mod.c
ins:
	sync ; sync
	setuid root insmod fasttrap.ko
unl:
	setuid root rmmod fasttrap


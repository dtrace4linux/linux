obj-m += dtrace.o
dtrace-objs := dtrace.o dtrace_linux.o

EXTRA_CFLAGS += \
	-I$(M)/../.. \
	-I$(M)/../../sys \
	-I$(M)/../../linux \
	-I$(M)/../../common/ctf \
	-I$(M)/../../uts/common \
	-I$(M)/../include \
	-D_KERNEL
#CFLAGS += -I$(M)/../../common/ctf -I$(M)/../../uts/common/sys -D_KERNEL

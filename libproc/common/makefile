CC=gcc -g
CPPFLAGS += -I../common -I../../common/ctf -I../../uts/common/ \
	-I../../linux -I../lib/libproc/common -I../libctf/common -DCTF_OLD_VERSIONS
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64

LIB = libproc.a

$(LIB): \
	$(LIB)(P32ton.o) \
	$(LIB)(Pcontrol.o) \
	$(LIB)(Pcore.o) \
	$(LIB)(Pexecname.o) \
	$(LIB)(Pgcore.o) \
	$(LIB)(Pidle.o) \
	$(LIB)(Plwpregs.o) \
	$(LIB)(Pscantext.o) \
	$(LIB)(Pservice.o) \
	$(LIB)(Pstack.o) \
	$(LIB)(Psymtab.o) \
	$(LIB)(Psyscall.o) \
	$(LIB)(Putil.o) \
	$(LIB)(pr_door.o) \
	$(LIB)(pr_exit.o) \
	$(LIB)(pr_fcntl.o) \
	$(LIB)(pr_getitimer.o) \
	$(LIB)(pr_getrctl.o) \
	$(LIB)(pr_getrlimit.o) \
	$(LIB)(pr_getsockname.o) \
	$(LIB)(pr_ioctl.o) \
	$(LIB)(pr_lseek.o) \
	$(LIB)(pr_memcntl.o) \
	$(LIB)(pr_meminfo.o) \
	$(LIB)(pr_mmap.o) \
	$(LIB)(pr_open.o) \
	$(LIB)(pr_pbind.o) \
	$(LIB)(pr_rename.o) \
	$(LIB)(pr_sigaction.o) \
	$(LIB)(pr_stat.o) \
	$(LIB)(pr_statvfs.o) \
	$(LIB)(pr_tasksys.o) \
	$(LIB)(pr_waitid.o) \
	$(LIB)(proc_arg.o) \
	$(LIB)(proc_get_info.o) \
	$(LIB)(proc_names.o) \
	$(LIB)(proc_set.o) \
	$(LIB)(proc_stdio.o) \

#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
# ident	"@(#)Makefile.com	1.17	04/09/28 SMI"
#
# lib/libproc/Makefile.com

LIBRARY = libproc.a
VERS = .1

CMNOBJS =	\
	P32ton.o	\
	Pcontrol.o	\
	Pcore.o		\
	Pexecname.o	\
	Pgcore.o	\
	Pidle.o		\
	Pisprocdir.o	\
	Plwpregs.o	\
	Pservice.o	\
	Psymtab.o	\
	Pscantext.o	\
	Pstack.o	\
	Psyscall.o	\
	Putil.o		\
	pr_door.o	\
	pr_exit.o	\
	pr_fcntl.o	\
	pr_getitimer.o	\
	pr_getrctl.o	\
	pr_getrlimit.o	\
	pr_getsockname.o \
	pr_ioctl.o	\
	pr_lseek.o	\
	pr_memcntl.o	\
	pr_meminfo.o	\
	pr_mmap.o	\
	pr_open.o	\
	pr_pbind.o	\
	pr_rename.o	\
	pr_sigaction.o	\
	pr_stat.o	\
	pr_statvfs.o	\
	pr_tasksys.o	\
	pr_waitid.o	\
	proc_get_info.o	\
	proc_names.o	\
	proc_arg.o	\
	proc_set.o	\
	proc_stdio.o

ISAOBJS =	\
	Pisadep.o

OBJECTS = $(CMNOBJS) $(ISAOBJS)

# include library definitions
include ../../Makefile.lib
include ../../Makefile.rootfs

SRCS =		$(CMNOBJS:%.o=../common/%.c) $(ISAOBJS:%.o=%.c)

LIBS =		$(DYNLIB) $(LINTLIB)
LDLIBS +=	-lrtld_db -lelf -lctf -lc
$(LINTLIB) :=	SRCS = $(SRCDIR)/$(LINTSRC)

SRCDIR =	../common
MAPDIR =	../spec/$(TRANSMACH)
SPECMAPFILE =	$(MAPDIR)/mapfile

CFLAGS +=	$(CCVERBOSE)
CPPFLAGS +=	-I$(SRCDIR)

.KEEP_STATE:

all: $(LIBS)

lint: lintcheck

# include library targets
include ../../Makefile.targ

objs/%.o pics/%.o: %.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

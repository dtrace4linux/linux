#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)Makefile.com	1.6	04/08/02 SMI"

PROG= lockstat
OBJS= lockstat.o sym.o
SRCS= $(OBJS:%.o=../%.c)

include ../../Makefile.cmd

LDLIBS += -lelf -lkstat -ldtrace -lrt
CFLAGS += $(CCVERBOSE)
CFLAGS64 += $(CCVERBOSE)
LINTFLAGS += -xerroff=E_SEC_SPRINTF_UNBOUNDED_COPY
LINTFLAGS64 += -xerroff=E_SEC_SPRINTF_UNBOUNDED_COPY

FILEMODE= 0555
GROUP= bin

CLEANFILES += $(OBJS)

.KEEP_STATE:

all: $(PROG)

$(PROG):	$(OBJS)
	$(LINK.c) -o $(PROG) $(OBJS) $(LDLIBS)
	$(POST_PROCESS)

clean:
	-$(RM) $(CLEANFILES)

lint:	lint_SRCS

%.o:    ../%.c
	$(COMPILE.c) $<

include ../../Makefile.targ

#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
# ident	"@(#)Makefile.com	1.3	04/09/28 SMI"
#

PROG = intrstat
OBJS = intrstat.o
SRCS = $(OBJS:%.o=../%.c)

include ../../Makefile.cmd

CFLAGS += $(CCVERBOSE)
CFLAGS64 += $(CCVERBOSE)
LDLIBS += -ldtrace -lrt

STRIPFLAG =
FILEMODE = 0555
GROUP = bin

CLEANFILES += $(OBJS)

.KEEP_STATE:

all: $(PROG)

$(PROG): $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LDLIBS)
	$(POST_PROCESS) ; $(STRIP_STABS)

clean:
	-$(RM) $(CLEANFILES)

lint: lint_SRCS

%.o: ../%.c
	$(COMPILE.c) $<

include ../../Makefile.targ

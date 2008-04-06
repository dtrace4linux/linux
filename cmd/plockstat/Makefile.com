#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)Makefile.com	1.1	04/08/31 SMI"

PROG = plockstat
OBJS = plockstat.o
SRCS = $(OBJS:%.o=../%.c)

include ../../Makefile.cmd

LDLIBS += -ldtrace -lproc

CLEANFILES += $(OBJS)

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

#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)Makefile	1.2	04/02/03 SMI"
#

PROG= isaexec

include ../Makefile.cmd

CFLAGS += $(CCVERBOSE)

.KEEP_STATE:

all: $(PROG)

install: all $(ROOTLIBPROG)

clean:

lint:
	$(LINT.c) $(PROG).c $(LDLIBS)

include ../Makefile.targ

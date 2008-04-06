#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)Makefile	1.1	03/09/02 SMI"

include ../Makefile.lib

HDRS = dtrace.h
HDRDIR = common

SUBDIRS = $(MACH) 
$(BUILD64)SUBDIRS += $(MACH64)

all := TARGET = all
clean := TARGET = clean
clobber := TARGET = clobber
install := TARGET = install
lint := TARGET = lint
yydebug := TARGET = yydebug

.KEEP_STATE:

all clean clobber: spec .WAIT $(SUBDIRS)

install: install_h spec .WAIT $(SUBDIRS)

lint yydebug: $(SUBDIRS)

$(ROOTHDRS) := GROUP = bin

install_h: $(ROOTHDRS)

check: $(CHECKHDRS)

spec $(SUBDIRS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

FRC:

include ../Makefile.targ

#
# Cross-reference customization: build the cross-reference only over the
# source directories, and ignore Makefiles and machine-generated source.
#
XRDIRS = common i386 sparc sparcv9
XRDEL = dt_lex.c dt_grammar.c Makefile*

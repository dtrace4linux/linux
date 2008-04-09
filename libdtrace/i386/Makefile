#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)Makefile	1.6	04/11/11 SMI"

ASFLAGS += $(AS_PICFLAGS) -P -D_ASM

MAPDIR = ../spec/i386
MACHOBJS = dis_tables.o dt_asmsubr.o
DLIBSRCS = regs.d

include ../Makefile.com

SRCS += $(SRC)/common/dis/i386/dis_tables.c dt_asmsubr.s
CPPFLAGS += -I$(SRC)/common/dis/i386

LINTFLAGS += -erroff=E_BAD_PTR_CAST_ALIGN

CLEANFILES += regs.sed regs.d

%.sed: %.sed.in
	$(COMPILE.cpp) -D_KERNEL $< | \
	tr -d ' ' | tr '@' ' ' | tr '"' '@' | grep '^s/' > $@

../$(MACH)/regs.d: regs.sed regs.d.in
	sed -f regs.sed < regs.d.in > $@

pics/%.o: $(SRC)/common/dis/i386/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

install yydebug: all $(ROOTLIBS) $(ROOTLINKS) $(ROOTLINT) \
	$(ROOTDLIBS) $(ROOTDOBJS)

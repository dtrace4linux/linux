#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)Makefile	1.2	04/02/03 SMI"

UTSBASE	= ../..

MODULE		= systrace
OBJECTS		= $(SYSTRACE_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(SYSTRACE_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
ROOTLINK	= $(ROOT_DTRACE_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/common/dtrace

include $(UTSBASE)/sparc/Makefile.sparc

ALL_TARGET	= $(BINARY) $(SRC_CONFILE)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE) $(ROOTLINK) $(ROOT_CONFFILE)

CFLAGS		+= $(CCVERBOSE)
LDFLAGS		+= -dy -Ndrv/dtrace

.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clobber:	$(CLOBBER_DEPS)

lint:		$(LINT_DEPS)

modlintlib:	$(MODLINTLIB_DEPS)

clean.lint:	$(CLEAN_LINT_DEPS)

install:	$(INSTALL_DEPS)

$(ROOTLINK):	$(ROOT_DTRACE_DIR) $(ROOTMODULE)
	-$(RM) $@; ln $(ROOTMODULE) $@

include $(UTSBASE)/sparc/Makefile.targ

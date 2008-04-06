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

MODULE		= ctf
OBJECTS		= $(CTF_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(CTF_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_MISC_DIR)/$(MODULE)

include $(UTSBASE)/sparc/Makefile.sparc

ALL_TARGET	= $(BINARY)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE)

CFLAGS		+= $(CCVERBOSE)
CPPFLAGS	+= -I$(SRC)/common/ctf -DCTF_OLD_VERSIONS
LDFLAGS		+= -Breduce -M$(UTSBASE)/common/ctf/mapfile -dy -Nmisc/zmod

.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clobber:	$(CLOBBER_DEPS)

lint:		$(LINT_DEPS)

modlintlib:	$(MODLINTLIB_DEPS)

clean.lint:	$(CLEAN_LINT_DEPS)

install:	$(INSTALL_DEPS)

include $(UTSBASE)/sparc/Makefile.targ

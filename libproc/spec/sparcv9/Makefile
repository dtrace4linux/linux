#
#ident	"@(#)Makefile	1.1	99/01/25 SMI"
#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
# lib/libproc/spec/sparcv9/Makefile

.KEEP_STATE:

include	../Makefile.targ

# Add arch specific objects here
OBJECTS	+=

include	$(SRC)/lib/Makefile.lib
include	$(SRC)/lib/Makefile.lib.64

# Uncomment the following if the linker complains
#sparcv9_C_PICFLAGS  = -K PIC

include	$(SRC)/lib/Makefile.spec

install: $(ROOTABILIB64)

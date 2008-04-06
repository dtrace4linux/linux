#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)Makefile	1.5	99/03/24 SMI"
#
# lib/libproc/Makefile

include ../Makefile.lib

SUBDIRS=	$(MACH)
$(BUILD64)SUBDIRS += $(MACH64)

all :=		TARGET= all
clean :=	TARGET= clean
clobber :=	TARGET= clobber
delete :=	TARGET= delete
install :=	TARGET= install
lint :=		TARGET= lint
_msg :=		TARGET= _msg
package :=	TARGET= package

LIBRARY= 	libproc.a
TEXT_DOMAIN=	SUNW_OST_OSLIB
XGETFLAGS=	-a
POFILE=		$(LIBRARY:.a=.po)
POFILES=	generic.po

SED=	sed
GREP=	grep
CP=	cp

# definitions for install_h target
HDRS=		libproc.h
ROOTHDRDIR=	$(ROOT)/usr/include
ROOTHDRS=	$(HDRS:%=$(ROOTHDRDIR)/%)
CHECKHDRS=	$(HDRS:%.h=common/%.check)

$(ROOTHDRS) :=  FILEMODE= 644

# install rule for install_h target
$(ROOTHDRDIR)/%: common/%
	$(INS.file)

.KEEP_STATE:

all clean clobber delete install package: spec .WAIT $(SUBDIRS)

lint: $(SUBDIRS)

install_h: $(ROOTHDRS)

check: $(CHECKHDRS)

spec $(SUBDIRS):	FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

_msg:	$(MSGDOMAIN) $(POFILE)
	$(RM) $(MSGDOMAIN)/$(POFILE)
	$(CP) $(POFILE) $(MSGDOMAIN)

$(POFILE):	.WAIT $(POFILES)
	$(RM) $@
	$(CAT) $(POFILES) > $@

$(POFILES):
	$(RM) messages.po
	$(XGETTEXT) $(XGETFLAGS) *.[ch]*
	$(SED) -e '/^# msg/d' -e '/^domain/d' messages.po > $@
	$(RM) messages.po

FRC:

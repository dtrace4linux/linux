#!/bin/sh
#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)mksignal.sh	1.1	03/09/02 SMI"

echo "\
/*\n\
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.\n\
 *\n\
 * The contents of this file are subject to the terms of the\n\
 * Common Development and Distribution License, Version 1.0 only.\n\
 * See the file usr/src/LICENSING.NOTICE in this distribution or\n\
 * http://www.opensolaris.org/license/ for details.\n\
 */\n\
\n\
#pragma ident\t\"@(#)mksignal.sh\t1.1\t03/09/02 SMI\"\n"

pattern='^#define[	 ]*_*\(SIG[A-Z0-9]*\)[	 ]\{1,\}\([A-Z0-9]*\).*$'
replace='inline int \1 = \2;@#pragma D binding "1.0" \1'

sed -n "s/$pattern/$replace/p;/SIGRTMAX/q" | tr '@' '\n'

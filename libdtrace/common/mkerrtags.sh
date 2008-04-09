#!/bin/sh
#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# See the file usr/src/LICENSING.NOTICE in this distribution or
# http://www.opensolaris.org/license/ for details.
#
#ident	"@(#)mkerrtags.sh	1.1	03/09/02 SMI"

echo "
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident \"@(#)mkerrtags.sh\t1.1\t03/09/02 SMI\"

#include <dt_errtags.h>

static const char *const _dt_errtags[] = {"

pattern='^	\(D_[A-Z0-9_]*\),*'
replace='	"\1",'

sed -n "s/$pattern/$replace/p" || exit 1

echo "
};

static const int _dt_ntag = sizeof (_dt_errtags) / sizeof (_dt_errtags[0]);

const char *
dt_errtag(dt_errtag_t tag)
{
	return (_dt_errtags[(tag > 0 && tag < _dt_ntag) ? tag : 0]);
}"

exit 0

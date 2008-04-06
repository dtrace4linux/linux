/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#ifndef	_FASTTRAP_ISA_H
#define	_FASTTRAP_ISA_H

#pragma ident	"@(#)fasttrap_isa.h	1.3	04/11/07 SMI"

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define	FASTTRAP_MAX_INSTR_SIZE		15

#define	FASTTRAP_INSTR			0xcc

#define	FASTTRAP_SUNWDTRACE_SIZE	64

typedef	uint8_t		fasttrap_instr_t;

typedef struct fasttrap_machtp {
	uint8_t		ftmt_instr[FASTTRAP_MAX_INSTR_SIZE]; /* orig. instr. */
	uint8_t		ftmt_size;	/* instruction size */
#ifdef __amd64
	uint8_t		ftmt_ripmode;	/* %rip-relative handling mode */
	uint8_t		ftmt_modrm;	/* saved modrm byte */
#endif
	uint8_t		ftmt_type;	/* emulation type */
	uint8_t		ftmt_code;	/* branch condition */
	uint8_t		ftmt_base;	/* branch base */
	uint8_t		ftmt_index;	/* branch index */
	uint8_t		ftmt_scale;	/* branch scale */
	uintptr_t	ftmt_dest;	/* destination of control flow */
} fasttrap_machtp_t;

#define	ftt_instr	ftt_mtp.ftmt_instr
#ifdef __amd64
#define	ftt_ripmode	ftt_mtp.ftmt_ripmode
#define	ftt_modrm	ftt_mtp.ftmt_modrm
#endif
#define	ftt_size	ftt_mtp.ftmt_size
#define	ftt_type	ftt_mtp.ftmt_type
#define	ftt_code	ftt_mtp.ftmt_code
#define	ftt_base	ftt_mtp.ftmt_base
#define	ftt_index	ftt_mtp.ftmt_index
#define	ftt_scale	ftt_mtp.ftmt_scale
#define	ftt_dest	ftt_mtp.ftmt_dest

#define	FASTTRAP_T_COMMON	0x00	/* common case -- no emulation */
#define	FASTTRAP_T_JCC		0x01	/* near and far conditional jumps */
#define	FASTTRAP_T_LOOP		0x02	/* loop instructions */
#define	FASTTRAP_T_JCXZ		0x03	/* jump if %ecx/%rcx is zero */
#define	FASTTRAP_T_JMP		0x04	/* relative jump */
#define	FASTTRAP_T_CALL		0x05	/* near call (and link) */
#define	FASTTRAP_T_RET		0x06	/* ret */
#define	FASTTRAP_T_RET16	0x07	/* ret <imm16> */

/*
 * For performance rather than correctness.
 */
#define	FASTTRAP_T_PUSHL_EBP	0x10	/* pushl %ebp (for function entry) */

#define	FASTTRAP_RIP_1		0x1
#define	FASTTRAP_RIP_2		0x2
#define	FASTTRAP_RIP_X		0x4

#define	FASTTRAP_AFRAMES		3
#define	FASTTRAP_RETURN_AFRAMES		4
#define	FASTTRAP_ENTRY_AFRAMES		3
#define	FASTTRAP_OFFSET_AFRAMES		3

#ifdef	__cplusplus
}
#endif

#endif	/* _FASTTRAP_ISA_H */

/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)dt_isadep.c	1.4	04/09/01 SMI"

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

#include <dt_impl.h>
#include <dt_pid.h>

/*ARGSUSED*/
int
dt_pid_create_entry_probe(struct ps_prochandle *P, dtrace_hdl_t *dtp,
    fasttrap_probe_spec_t *ftp, const GElf_Sym *symp)
{
	ftp->ftps_type = DTFTP_ENTRY;
	ftp->ftps_pc = (uintptr_t)symp->st_value;
	ftp->ftps_size = (size_t)symp->st_size;
	ftp->ftps_noffs = 1;
	ftp->ftps_offs[0] = 0;

	if (ioctl(dtp->dt_ftfd, FASTTRAPIOC_MAKEPROBE, ftp) != 0) {
		dt_dprintf("fasttrap probe creation ioctl failed: %s\n",
		    strerror(errno));
		return (dt_set_errno(dtp, errno));
	}

	return (1);
}

int
dt_pid_create_return_probe(struct ps_prochandle *P, dtrace_hdl_t *dtp,
    fasttrap_probe_spec_t *ftp, const GElf_Sym *symp, uint64_t *stret)
{

	uint32_t *text;
	int i;
	int srdepth = 0;

	if ((text = malloc(symp->st_size + 4)) == NULL) {
		dt_dprintf("mr sparkle: malloc() failed\n");
		return (DT_PROC_ERR);
	}

	if (Pread(P, text, symp->st_size, symp->st_value) != symp->st_size) {
		dt_dprintf("mr sparkle: Pread() failed\n");
		free(text);
		return (DT_PROC_ERR);
	}

	/*
	 * Leave a dummy instruction in the last slot to simplify edge
	 * conditions.
	 */
	text[symp->st_size / 4] = 0;

	ftp->ftps_type = DTFTP_RETURN;
	ftp->ftps_pc = symp->st_value;
	ftp->ftps_size = symp->st_size;
	ftp->ftps_noffs = 0;

	for (i = 0; i < symp->st_size / 4; i++) {
		/*
		 * If we encounter an existing tracepoint, query the
		 * kernel to find out the instruction that was
		 * replaced at this spot.
		 */
		while (text[i] == FASTTRAP_INSTR) {
			fasttrap_instr_query_t instr;

			instr.ftiq_pid = Pstatus(P)->pr_pid;
			instr.ftiq_pc = symp->st_value + i * 4;

			if (ioctl(dtp->dt_ftfd, FASTTRAPIOC_GETINSTR,
			    &instr) != 0) {

				if (errno == ESRCH || errno == ENOENT) {
					if (Pread(P, &text[i], 4,
					    instr.ftiq_pc) != 4) {
						dt_dprintf("mr sparkle: "
						    "Pread() failed\n");
						free(text);
						return (DT_PROC_ERR);
					}
					continue;
				}

				free(text);
				dt_dprintf("mr sparkle: getinstr query "
				    "failed: %s\n", strerror(errno));
				return (DT_PROC_ERR);
			}

			text[i] = instr.ftiq_instr;
			break;
		}

		/* save */
		if ((text[i] & 0xc1f80000) == 0x81e00000) {
			srdepth++;
			continue;
		}

		/* restore */
		if ((text[i] & 0xc1f80000) == 0x81e80000) {
			srdepth--;
			continue;
		}

		if (srdepth > 0) {
			/* ret */
			if (text[i] == 0x81c7e008)
				goto is_ret;

			/* return */
			if (text[i] == 0x81cfe008)
				goto is_ret;

			/* call or jmpl w/ restore in the slot */
			if (((text[i] & 0xc0000000) == 0x40000000 ||
			    (text[i] & 0xc1f80000) == 0x81c00000) &&
			    (text[i + 1] & 0xc1f80000) == 0x81e80000)
				goto is_ret;

			/* call to one of the stret routines */
			if ((text[i] & 0xc0000000) == 0x40000000) {
				int32_t	disp = text[i] << 2;
				uint64_t dest = ftp->ftps_pc + i * 4 + disp;

				dt_dprintf("dest = %llx\n", (u_longlong_t)dest);

				if (dest == stret[0] || dest == stret[1] ||
				    dest == stret[2] || dest == stret[3])
					goto is_ret;
			}
		} else {
			/* external call */
			if ((text[i] & 0xc0000000) == 0x40000000) {
				int32_t dst = text[i] << 2;

				dst += i * 4;

				if ((uintptr_t)dst >= (uintptr_t)symp->st_size)
					goto is_ret;
			}

			/* jmpl into %g0 -- this includes the retl pseudo op */
			if ((text[i] & 0xfff80000) == 0x81c00000)
				goto is_ret;

			/* external branch -- possible return site */
			if ((text[i] & 0xc0000000) == 0) {
				int32_t dst;

				switch ((text[i] >> 22) & 0x7) {
				case 1:		/* BPcc */
					dst = text[i] & 0x7ffff;
					dst <<= 13;
					dst >>= 11;
					break;
				case 2:		/* Bicc */
					dst = text[i] & 0x3fffff;
					dst <<= 10;
					dst >>= 8;
					break;
				case 3:		/* BPr */
					dst = (((text[i]) >> 6) & 0xc000) |
					    ((text[i]) & 0x3fff);
					dst <<= 16;
					dst >>= 14;
					break;
				case 5:		/* FBPfcc */
					dst = text[i] & 0x7ffff;
					dst <<= 13;
					dst >>= 11;
					break;
				case 6:
					dst = text[i] & 0x3fffff;
					dst <<= 10;
					dst >>= 8;
					break;
				default:
					continue;
				}

				dst += i * 4;

				if ((uintptr_t)dst >= (uintptr_t)symp->st_size)
					goto is_ret;
			}
		}

		continue;
is_ret:
		i++;
		dt_dprintf("return at offset %x\n", i * 4);
		ftp->ftps_offs[ftp->ftps_noffs++] = i * 4;
	}

	free(text);
	if (ftp->ftps_noffs > 0) {
		if (ioctl(dtp->dt_ftfd, FASTTRAPIOC_MAKEPROBE, ftp) != 0) {
			dt_dprintf("fasttrap probe creation ioctl failed: %s\n",
			    strerror(errno));
			return (dt_set_errno(dtp, errno));
		}
	}


	return (ftp->ftps_noffs);
}

/*ARGSUSED*/
int
dt_pid_create_offset_probe(struct ps_prochandle *P, dtrace_hdl_t *dtp,
    fasttrap_probe_spec_t *ftp, const GElf_Sym *symp, ulong_t off)
{
	if (off & 0x3)
		return (DT_PROC_ALIGN);

	ftp->ftps_type = DTFTP_OFFSETS;
	ftp->ftps_pc = (uintptr_t)symp->st_value;
	ftp->ftps_size = (size_t)symp->st_size;
	ftp->ftps_noffs = 1;
	ftp->ftps_offs[0] = off;

	if (ioctl(dtp->dt_ftfd, FASTTRAPIOC_MAKEPROBE, ftp) != 0) {
		dt_dprintf("fasttrap probe creation ioctl failed: %s\n",
		    strerror(errno));
		return (dt_set_errno(dtp, errno));
	}

	return (1);
}

/*ARGSUSED*/
int
dt_pid_create_glob_offset_probes(struct ps_prochandle *P, dtrace_hdl_t *dtp,
    fasttrap_probe_spec_t *ftp, const GElf_Sym *symp, const char *pattern)
{
	ulong_t i;

	ftp->ftps_type = DTFTP_OFFSETS;
	ftp->ftps_pc = (uintptr_t)symp->st_value;
	ftp->ftps_size = (size_t)symp->st_size;
	ftp->ftps_noffs = 0;

	/*
	 * If we're matching against everything, just iterate through each
	 * instruction in the function, otherwise look for matching offset
	 * names by constructing the string and comparing it against the
	 * pattern.
	 */
	if (strcmp("*", pattern) == 0) {
		for (i = 0; i < symp->st_size; i += 4) {
			ftp->ftps_offs[ftp->ftps_noffs++] = i;
		}
	} else {
		char name[sizeof (i) * 2 + 1];

		for (i = 0; i < symp->st_size; i += 4) {
			(void) sprintf(name, "%lx", i);
			if (gmatch(name, pattern))
				ftp->ftps_offs[ftp->ftps_noffs++] = i;
		}
	}

	if (ioctl(dtp->dt_ftfd, FASTTRAPIOC_MAKEPROBE, ftp) != 0) {
		dt_dprintf("fasttrap probe creation ioctl failed: %s\n",
		    strerror(errno));
		return (dt_set_errno(dtp, errno));
	}

	return (ftp->ftps_noffs);
}

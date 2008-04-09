/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#pragma ident	"@(#)Pidle.c	1.3	04/08/16 SMI"

#include <linux_types.h>
#include <stdlib.h>
#include <libelf.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/sysmacros.h>

#include "Pcontrol.h"

static ssize_t
Pread_idle(struct ps_prochandle *P, void *buf, size_t n, uintptr_t addr)
{
	size_t resid = n;

	while (resid > 0) {
		map_info_t *mp;
		uintptr_t mapoff;
		ssize_t len;
		off64_t off;

		if ((mp = Paddr2mptr(P, addr)) == NULL)
			break;

		mapoff = addr - mp->map_pmap.pr_vaddr;
		len = MIN(resid, mp->map_pmap.pr_size - mapoff);
		off = mp->map_offset + mapoff;

		if ((len = pread64(P->asfd, buf, len, off)) <= 0)
			break;

		resid -= len;
		addr += len;
		buf = (char *)buf + len;
	}

	return (n - resid);
}

/*ARGSUSED*/
static ssize_t
Pwrite_idle(struct ps_prochandle *P, const void *buf, size_t n, uintptr_t addr)
{
	errno = EIO;
	return (-1);
}

static const ps_rwops_t P_idle_ops = {
	Pread_idle,
	Pwrite_idle
};

static int
idle_add_mapping(struct ps_prochandle *P, GElf_Phdr *php, file_info_t *fp)
{
	map_info_t *mp;
	int i;

	dprintf("mapping base %llx filesz %llu memsz %llu offset %llu\n",
	    (u_longlong_t)php->p_vaddr, (u_longlong_t)php->p_filesz,
	    (u_longlong_t)php->p_memsz, (u_longlong_t)php->p_offset);

	for (i = 0; i < P->map_count; i++)
		if (php->p_vaddr < P->mappings[i].map_pmap.pr_vaddr)
			break;

	(void) memmove(P->mappings + i, P->mappings + i + 1,
	    (P->map_count - i) * sizeof (map_info_t));
	mp = &P->mappings[i];
	P->map_count++;

	mp->map_offset = php->p_offset;
	mp->map_file = fp;
	if (fp->file_map == NULL)
		fp->file_map = mp;
	fp->file_ref++;

	mp->map_pmap.pr_vaddr = (uintptr_t)php->p_vaddr;
	mp->map_pmap.pr_size = php->p_filesz;
	(void) strncpy(mp->map_pmap.pr_mapname, fp->file_pname,
	    sizeof (mp->map_pmap.pr_mapname));
	mp->map_pmap.pr_offset = php->p_offset;

	mp->map_pmap.pr_mflags = 0;
	if (php->p_flags & PF_R)
		mp->map_pmap.pr_mflags |= MA_READ;
	if (php->p_flags & PF_W)
		mp->map_pmap.pr_mflags |= MA_WRITE;
	if (php->p_flags & PF_X)
		mp->map_pmap.pr_mflags |= MA_EXEC;

	mp->map_pmap.pr_pagesize = 0;
	mp->map_pmap.pr_shmid = -1;

	return (0);
}

struct ps_prochandle *
Pgrab_file(const char *fname, int *perr)
{
	struct ps_prochandle *P = NULL;
	GElf_Ehdr ehdr;
	Elf *elf = NULL;
	file_info_t *fp = NULL;
	int fd;
	int i;

	if ((fd = open64(fname, O_RDONLY)) < 0) {
		dprintf("couldn't open file");
		*perr = (errno == ENOENT) ? G_NOEXEC : G_STRANGE;
		return (NULL);
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		dprintf("libproc ELF version is more recent than libelf");
		*perr = G_ELF;
		goto err;
	}

	if ((P = calloc(1, sizeof (struct ps_prochandle))) == NULL) {
		*perr = G_STRANGE;
		goto err;
	}

	(void) mutex_init(&P->proc_lock, USYNC_THREAD, NULL);
	P->state = PS_IDLE;
	P->pid = (pid_t)-1;
	P->asfd = fd;
	P->ctlfd = -1;
	P->statfd = -1;
	P->agentctlfd = -1;
	P->agentstatfd = -1;
	P->info_valid = -1;
	P->ops = &P_idle_ops;
	Pinitsym(P);

	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		*perr = G_ELF;
		return (NULL);
	}

	/*
	 * Construct a file_info_t that corresponds to this file.
	 */
	if ((fp = calloc(1, sizeof (file_info_t))) == NULL) {
		*perr = G_STRANGE;
		goto err;
	}

	if ((fp->file_lo = calloc(1, sizeof (rd_loadobj_t))) == NULL) {
		*perr = G_STRANGE;
		goto err;
	}

	if (*fname == '/') {
		(void) strncpy(fp->file_pname, fname, sizeof (fp->file_pname));
	} else {
		size_t sz;

		if (getcwd(fp->file_pname, sizeof (fp->file_pname) - 1) ==
		    NULL) {
			*perr = G_STRANGE;
			goto err;
		}

		sz = strlen(fp->file_pname);
		(void) snprintf(&fp->file_pname[sz],
		    sizeof (fp->file_pname) - sz, "/%s", fname);
	}

	fp->file_fd = fd;
	fp->file_lo->rl_lmident = LM_ID_BASE;
	fp->file_lname = strdup(fp->file_pname);
	fp->file_lbase = strdup(strrchr(fp->file_pname, '/') + 1);

	P->execname = strdup(fp->file_pname);

	P->num_files++;
	list_link(fp, &P->file_head);

	if (gelf_getehdr(elf, &ehdr) == NULL) {
		*perr = G_STRANGE;
		goto err;
	}

	dprintf("Pgrab_file: ehdr.e_phnum = %d\n", ehdr.e_phnum);

	/*
	 * Sift through the program headers making the relevant maps.
	 */
	P->mappings = malloc(ehdr.e_phnum * sizeof (map_info_t));
	for (i = 0; i < ehdr.e_phnum; i++) {
		GElf_Phdr phdr, *php;

		if ((php = gelf_getphdr(elf, i, &phdr)) == NULL) {
			*perr = G_STRANGE;
			goto err;
		}

		if (php->p_type != PT_LOAD)
			continue;

		if (idle_add_mapping(P, php, fp) != 0) {
			*perr = G_STRANGE;
			goto err;
		}
	}

	(void) elf_end(elf);

	P->map_exec = fp->file_map;

	P->status.pr_flags = PR_STOPPED;
	P->status.pr_nlwp = 0;
	P->status.pr_pid = (pid_t)-1;
	P->status.pr_ppid = (pid_t)-1;
	P->status.pr_pgid = (pid_t)-1;
	P->status.pr_sid = (pid_t)-1;
	P->status.pr_taskid = (taskid_t)-1;
	P->status.pr_projid = (projid_t)-1;
	switch (ehdr.e_ident[EI_CLASS]) {
	case ELFCLASS32:
		P->status.pr_dmodel = PR_MODEL_ILP32;
		break;
	case ELFCLASS64:
		P->status.pr_dmodel = PR_MODEL_LP64;
		break;
	default:
		*perr = G_FORMAT;
		goto err;
	}

	/*
	 * The file and map lists are complete, and will never need to be
	 * adjusted.
	 */
	P->info_valid = 1;

	return (P);
err:
	(void) close(fd);
	if (P != NULL)
		Pfree(P);
	if (elf != NULL)
		(void) elf_end(elf);
	return (NULL);
}

/**********************************************************************/
/*   This file contains code for static probes. Under Linux, we take  */
/*   a  different  strategy  to Solaris/Apple - since we dont molest  */
/*   the kernel code directly. (This is still supported - sort of).   */
/*   								      */
/*   Author: Paul D. Fox					      */
/*   $Header: Last edited: 11-Jun-2011 1.2 $ 			      */
/**********************************************************************/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

//#pragma ident	"@(#)sdt.c	1.6	06/03/24 SMI"

#include <dtrace_linux.h>
#include <linux/miscdevice.h>
#include <sys/modctl.h>
#include <sys/dtrace.h>
#include <sys/stack.h>
#include <sys/sdt.h>
#include <sys/sdt_impl.h>
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"
#include <linux/mount.h>
#include <linux/kallsyms.h>

# define regs pt_regs
#include <sys/stack.h>
#include <sys/frame.h>
#include <sys/privregs.h>

#include "ctf_struct.h"

#define	SDT_ADDR2NDX(addr)	((((uintptr_t)(addr)) >> 4) & sdt_probetab_mask)
#define	SDT_PROBETAB_SIZE	0x1000		/* 4k entries -- 16K total */

# if defined(sun)
static int			sdt_verbose = 0;
# endif
static sdt_probe_t		**sdt_probetab;
static int			sdt_probetab_size;
static int			sdt_probetab_mask;

/**********************************************************************/
/*   Easy handles to the specific providers we want to process.	      */
/**********************************************************************/
static sdt_provider_t *io_prov;

/**********************************************************************/
/*   Needed to map a mountpoint to a printable name.		      */
/**********************************************************************/
extern char *(*dentry_path_fn)(struct dentry *, char *, int);

int io_prov_sdt(pf_info_t *infp, uint8_t *instr, int size, int modrm);

/**********************************************************************/
/*   Go hunting for the static io:: provider slots.		      */
/**********************************************************************/
static int
io_prov_entry(pf_info_t *infp, uint8_t *instr, int size, int modrm)
{
	sdt_probe_t *sdp;
	sdt_provider_t *prov;
	uint8_t *offset;
	char	*name;

printk("io_prov_entry called %s:%s\n", infp->modname, infp->name);

	for (prov = sdt_providers; prov->sdtp_prefix != NULL; prov++) {
		if (strcmp(prov->sdtp_name, infp->modname) == 0)
			break;
	}
	name = kstrdup(infp->name, KM_SLEEP);
	sdp = kmem_zalloc(sizeof (sdt_probe_t), KM_SLEEP);
	sdp->sdp_id = dtrace_probe_create(prov->sdtp_id,
			    infp->func, NULL, name, 0, sdp);
	sdp->sdp_name = name;
	sdp->sdp_namelen = strlen(name);
	sdp->sdp_inslen = size;
	sdp->sdp_modrm = modrm;
	sdp->sdp_provider = prov;
	sdp->sdp_flags = infp->flags;
	sdp->sdp_entry = TRUE;

	/***********************************************/
	/*   Add the entry to the hash table.	       */
	/***********************************************/
	offset = instr;
	sdp->sdp_hashnext =
	    sdt_probetab[SDT_ADDR2NDX(offset)];
	sdt_probetab[SDT_ADDR2NDX(offset)] = sdp;

	sdp->sdp_patchval = PATCHVAL;
	sdp->sdp_patchpoint = (uint8_t *)offset;
	sdp->sdp_savedval = *sdp->sdp_patchpoint;

	infp->retptr = NULL;
	return 1;
}
/**********************************************************************/
/*   Handle creation of a return probe.				      */
/**********************************************************************/
static int
io_prov_return(pf_info_t *infp, uint8_t *instr, int size)
{
	sdt_probe_t *sdp;
	sdt_provider_t *prov;
	uint8_t *offset;
	char	*name;
	sdt_probe_t *retsdt = infp->retptr;

printk("io_prov_return called %s:%s %p  sz=%x\n", infp->modname, infp->name, instr, size);

	for (prov = sdt_providers; prov->sdtp_prefix != NULL; prov++) {
		if (strcmp(prov->sdtp_name, infp->modname) == 0)
			break;
	}
	name = kstrdup(infp->name2, KM_SLEEP);
	sdp = kmem_zalloc(sizeof (sdt_probe_t), KM_SLEEP);
	/***********************************************/
	/*   Daisy chain the return exit points so we  */
	/*   dont  end  up firing all of them when we  */
	/*   return from the probe.		       */
	/***********************************************/
	if (retsdt == NULL) {
		sdp->sdp_id = dtrace_probe_create(prov->sdtp_id,
			    infp->func, NULL, name, 0, sdp);
		infp->retptr = sdp;
	} else {
		retsdt->sdp_next = sdp;
		sdp->sdp_id = retsdt->sdp_id;
	}
	sdp->sdp_name = name;
	sdp->sdp_namelen = strlen(name);
	sdp->sdp_inslen = size;
	sdp->sdp_provider = prov;
	sdp->sdp_flags = infp->flags;
	sdp->sdp_entry = FALSE;

	/***********************************************/
	/*   Add the entry to the hash table.	       */
	/***********************************************/
	offset = instr;
	sdp->sdp_hashnext =
	    sdt_probetab[SDT_ADDR2NDX(offset)];
	sdt_probetab[SDT_ADDR2NDX(offset)] = sdp;

	sdp->sdp_patchval = PATCHVAL;
	sdp->sdp_patchpoint = (uint8_t *)offset;
	sdp->sdp_savedval = *sdp->sdp_patchpoint;
	return 1;
}
static void
io_prov_create(char *func, char *name, int flags)
{
	pf_info_t	inf;
	uint8_t		*start;
	int		size;

	if (dtrace_function_size(func, &start, &size) == 0) {
		printk("sdt_linux: cannot locate %s\n", func);
		return;
	}

	memset(&inf, 0, sizeof inf);
	inf.modname = "io";
	inf.func = func;
	inf.name = name;
	inf.name2 = "done";

	inf.func_entry = io_prov_entry;
	inf.func_return = io_prov_return;
	inf.func_sdt = io_prov_sdt;
	inf.flags = flags;

	dtrace_parse_function(&inf, start, start + size);

}
/**********************************************************************/
/*   Function called from dtrace_linux.c:dtrace_linux_init after the  */
/*   various  symtab hooks are setup so we can find the functions we  */
/*   are after.							      */
/**********************************************************************/
void
io_prov_init(void)
{
	io_prov_create("do_sync_read", "start", 1);
	io_prov_create("do_sync_write", "start", 0);
}

/**********************************************************************/
/*   This is called when we hit an SDT breakpoint.		      */
/**********************************************************************/

/**********************************************************************/
/*   Convert the args do do_sync_read/do_sync_write to an exportable  */
/*   structure so that D scripts can access the buf_t structure.      */
/**********************************************************************/
static buf_t *
create_buf_t(struct file *file, void *uaddr, size_t len, long long offset)
{
	/***********************************************/
	/*   BUG ALERT! We need per-cpu copies of the  */
	/*   static's  below  -  will fix this later.  */
	/*   Without  this,  one  cpu  may  overwrite  */
	/*   anothers version of the data.	       */
	/***********************************************/
static buf_t finfo;
static char buf[1024];
static char buf2[1024];
	char *name;
	char *fname;
	char *mntname = NULL;

	memset(&finfo, 0, sizeof finfo);
	finfo.f.fi_offset = offset;
	finfo.b.b_addr = uaddr;
	finfo.b.b_bcount = len;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
	fname = d_path(&file->f_path, buf, sizeof buf);
#else
        fname = d_path(file->f_dentry, file->f_vfsmnt, buf, sizeof buf);
#endif
	if (IS_ERR(fname)) {
		fname = "(unknown)";
	}
	name = strrchr(fname, '/');
	dtrace_memcpy(buf2, "<unknown>", 10);
	if (fname && name) {
		dtrace_memcpy(buf2, fname, name - fname);
		buf2[name - fname] = '\0';
	}

	/***********************************************/
	/*   Problem with older (2.6.9 kernel).	       */
	/***********************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
	if (dentry_path_fn) {
		static char buf3[1024];
		mntname = dentry_path_fn(file->f_vfsmnt->mnt_mountpoint, buf3, sizeof buf3);
	}
#endif

	finfo.f.fi_dirname = buf2;
	finfo.f.fi_name = name ? name + 1 : "<none>";
	finfo.f.fi_pathname = fname ? fname : "<unknown>";
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
	finfo.f.fi_fs = file->f_vfsmnt->mnt_devname;
#endif
	finfo.f.fi_mount = mntname ? mntname : "<unknown>";

	finfo.d.dev_major = -1;
	finfo.d.dev_minor = -1;
	finfo.d.dev_instance = -1;
	if (file->f_mapping && file->f_mapping->host) {
		finfo.d.dev_major = getmajor(file->f_mapping->host->i_rdev);
		finfo.d.dev_minor = getminor(file->f_mapping->host->i_rdev);
		finfo.d.dev_instance = file->f_mapping->host->i_rdev;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
	finfo.d.dev_pathname = (char *) file->f_vfsmnt->mnt_devname;
#endif
	finfo.d.dev_statname = mntname;

	return &finfo;
}
/*ARGSUSED*/
static int
sdt_invop(uintptr_t addr, uintptr_t *stack, uintptr_t eax, trap_instr_t *tinfo)
{
	uintptr_t stack0, stack1, stack2, stack3, stack4;
	sdt_probe_t *sdt = sdt_probetab[SDT_ADDR2NDX(addr)];
	struct pt_regs *regs;

	for (; sdt != NULL; sdt = sdt->sdp_hashnext) {
//printk("sdt_invop %p %p\n", sdt->sdp_patchpoint, addr);
		if (sdt->sdp_enabled && (uintptr_t)sdt->sdp_patchpoint == addr) {
			tinfo->t_opcode = sdt->sdp_savedval;
			tinfo->t_inslen = sdt->sdp_inslen;
			tinfo->t_modrm = sdt->sdp_modrm;
			/***********************************************/
			/*   Dont fire probe if this is unsafe.	       */
			/***********************************************/
			if (!tinfo->t_doprobe)
				return (DTRACE_INVOP_NOP);
			/*
			 * When accessing the arguments on the stack, we must
			 * protect against accessing beyond the stack.  We can
			 * safely set NOFAULT here -- we know that interrupts
			 * are already disabled.
			 */
			regs = (struct pt_regs *) stack;
			DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
			stack0 = regs->c_arg0;
			stack1 = regs->c_arg1;
			stack2 = regs->c_arg2;
			stack3 = regs->c_arg3;
			stack4 = regs->c_arg4;

			/***********************************************/
			/*   Not  sure  if  this  is re-entrant safe.  */
			/*   Might   need   a   per-cpu   buffer   to  */
			/*   write/read from.			       */
			/***********************************************/

			/***********************************************/
			/*   Dont  do this for the return probe - the  */
			/*   arguments  are  going  to be junk and we  */
			/*   will  hang/panic  the  kernel.  At  some  */
			/*   point  we  need  something better than a  */
			/*   entry/return  indicator -- maybe an enum  */
			/*   type.				       */
			/***********************************************/
			if (sdt->sdp_entry) {
				stack0 = (uintptr_t) create_buf_t((struct file *) stack0, 
					(void *) stack1,  /* uaddr */
					(size_t) stack2,  /* size */
					(long long) stack3 /* offset */);
			}

			DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
			    CPU_DTRACE_BADADDR);
//printk("probe %p: %p %p %p %p %p\n", &addr, stack0, stack1, stack2, stack3, stack4);
			dtrace_probe(sdt->sdp_id, stack0, stack0, stack0, 0, 0);
//			dtrace_probe(sdt->sdp_id, stack0, stack1,
//			    stack2, stack3, stack4);

			return (DTRACE_INVOP_NOP);
		}
	}
//printk("none in invop for dsdt\n");

	return (0);
}

/*ARGSUSED*/
static void
sdt_provide_module(void *arg, struct modctl *ctl)
{
# if defined(sun)
	struct module *mp = ctl->mod_mp;
	char *modname = ctl->mod_modname;
	sdt_probedesc_t *sdpd;
	sdt_probe_t *sdp, *old;
	sdt_provider_t *prov;

	/*
	 * One for all, and all for one:  if we haven't yet registered all of
	 * our providers, we'll refuse to provide anything.
	 */
	for (prov = sdt_providers; prov->sdtp_name != NULL; prov++) {
		if (prov->sdtp_id == DTRACE_PROVNONE)
			return;
	}

	if (mp->sdt_nprobes != 0 || (sdpd = mp->sdt_probes) == NULL)
		return;

	for (sdpd = mp->sdt_probes; sdpd != NULL; sdpd = sdpd->sdpd_next) {
		char *name = sdpd->sdpd_name, *func, *nname;
		int i, j, len;
		sdt_provider_t *prov;
		ulong_t offs;
		dtrace_id_t id;

		for (prov = sdt_providers; prov->sdtp_prefix != NULL; prov++) {
			char *prefix = prov->sdtp_prefix;

			if (strncmp(name, prefix, strlen(prefix)) == 0) {
				name += strlen(prefix);
				break;
			}
		}

		nname = kmem_alloc(len = strlen(name) + 1, KM_SLEEP);

		for (i = 0, j = 0; name[j] != '\0'; i++) {
			if (name[j] == '_' && name[j + 1] == '_') {
				nname[i] = '-';
				j += 2;
			} else {
				nname[i] = name[j++];
			}
		}

		nname[i] = '\0';

		sdp = kmem_zalloc(sizeof (sdt_probe_t), KM_SLEEP);
		sdp->sdp_loadcnt = ctl->mod_loadcnt;
		sdp->sdp_ctl = ctl;
		sdp->sdp_name = nname;
		sdp->sdp_namelen = len;
		sdp->sdp_provider = prov;

		func = kobj_searchsym(mp, sdpd->sdpd_offset, &offs);

		if (func == NULL)
			func = "<unknown>";

		/*
		 * We have our provider.  Now create the probe.
		 */
		if ((id = dtrace_probe_lookup(prov->sdtp_id, modname,
		    func, nname)) != DTRACE_IDNONE) {
			old = dtrace_probe_arg(prov->sdtp_id, id);
			ASSERT(old != NULL);

			sdp->sdp_next = old->sdp_next;
			sdp->sdp_id = id;
			old->sdp_next = sdp;
		} else {
			sdp->sdp_id = dtrace_probe_create(prov->sdtp_id,
			    modname, func, nname, 3, sdp);

			mp->sdt_nprobes++;
		}

		sdp->sdp_hashnext =
		    sdt_probetab[SDT_ADDR2NDX(sdpd->sdpd_offset)];
		sdt_probetab[SDT_ADDR2NDX(sdpd->sdpd_offset)] = sdp;

		sdp->sdp_patchval = PATCHVAL;
		sdp->sdp_patchpoint = (uint8_t *)sdpd->sdpd_offset;
		sdp->sdp_savedval = *sdp->sdp_patchpoint;
	}
# endif
}

/*ARGSUSED*/
static void
sdt_destroy(void *arg, dtrace_id_t id, void *parg)
{
	sdt_probe_t *sdp = parg, *old, *last, *hash;
	int ndx;

# if defined(sun)
	struct modctl *ctl = sdp->sdp_ctl;
	if (ctl != NULL && ctl->mod_loadcnt == sdp->sdp_loadcnt) {
		if ((ctl->mod_loadcnt == sdp->sdp_loadcnt &&
		    ctl->mod_loaded)) {
			((struct module *)(ctl->mod_mp))->sdt_nprobes--;
		}
	}
# endif

	while (sdp != NULL) {
		old = sdp;

		/*
		 * Now we need to remove this probe from the sdt_probetab.
		 */
		ndx = SDT_ADDR2NDX(sdp->sdp_patchpoint);
		last = NULL;
		hash = sdt_probetab[ndx];

		while (hash != sdp) {
			ASSERT(hash != NULL);
			last = hash;
			hash = hash->sdp_hashnext;
		}

		if (last != NULL) {
			last->sdp_hashnext = sdp->sdp_hashnext;
		} else {
			sdt_probetab[ndx] = sdp->sdp_hashnext;
		}

		kmem_free(sdp->sdp_name, sdp->sdp_namelen);
		sdp = sdp->sdp_next;
		kmem_free(old, sizeof (sdt_probe_t));
	}
}

/*ARGSUSED*/
static int
sdt_enable(void *arg, dtrace_id_t id, void *parg)
{
	sdt_probe_t *sdp = parg;

# if defined(sun)
	ctl->mod_nenabled++;
	{struct modctl *ctl = sdp->sdp_ctl;
	/*
	 * If this module has disappeared since we discovered its probes,
	 * refuse to enable it.
	 */
	if (!ctl->mod_loaded) {
		if (sdt_verbose) {
			cmn_err(CE_NOTE, "sdt is failing for probe %s "
			    "(module %s unloaded)",
			    sdp->sdp_name, ctl->mod_modname);
		}
		return 0;
	}

	/*
	 * Now check that our modctl has the expected load count.  If it
	 * doesn't, this module must have been unloaded and reloaded -- and
	 * we're not going to touch it.
	 */
	if (ctl->mod_loadcnt != sdp->sdp_loadcnt) {
		if (sdt_verbose) {
			cmn_err(CE_NOTE, "sdt is failing for probe %s "
			    "(module %s reloaded)",
			    sdp->sdp_name, ctl->mod_modname);
		}
		return 0;
	}
	}
# endif

	while (sdp != NULL) {
		/***********************************************/
		/*   Kernel  code  wil be write protected, so  */
		/*   try and unprotect it.		       */
		/***********************************************/
		sdp->sdp_enabled = TRUE;
		if (memory_set_rw(sdp->sdp_patchpoint, 1, TRUE))
			*sdp->sdp_patchpoint = sdp->sdp_patchval;
		sdp = sdp->sdp_next;
	}
	return 0;
}

/*ARGSUSED*/
static void
sdt_disable(void *arg, dtrace_id_t id, void *parg)
{
	sdt_probe_t *sdp = parg;

# if 0
	ctl->mod_nenabled--;
	{
	struct modctl *ctl = sdp->sdp_ctl;
	if (!ctl->mod_loaded || ctl->mod_loadcnt != sdp->sdp_loadcnt)
		return;
	}
# endif

	while (sdp != NULL) {
		sdp->sdp_enabled = FALSE;
		/***********************************************/
		/*   Memory  should  be  writable,  but if we  */
		/*   failed  in  the  sdt_enable  code,  e.g.  */
		/*   because   we  failed  to  unprotect  the  */
		/*   memory  page,  then dont try and unpatch  */
		/*   something we didnt patch.		       */
		/***********************************************/
		if (*sdp->sdp_patchpoint == sdp->sdp_patchval) {
			*sdp->sdp_patchpoint = sdp->sdp_savedval;
		}
		sdp = sdp->sdp_next;
	}
}

/*ARGSUSED*/
uint64_t
sdt_getarg(void *arg, dtrace_id_t id, void *parg, int argno, int aframes)
{
	uintptr_t val;
	struct frame *fp = (struct frame *)dtrace_getfp();
	uintptr_t *stack = NULL;
	int i;
#if defined(__amd64)
	/*
	 * A total of 6 arguments are passed via registers; any argument with
	 * index of 5 or lower is therefore in a register.
	 */
	int inreg = 5;
#endif

/*	for (i = 1; i <= aframes+2; i++) {
		fp = (struct frame *)(fp->fr_savfp);
printk("stk %d: %p: %p %p %p %p\n%p %p %p\n", i, fp, ((long *) fp)[0], ((long *) fp)[1], ((long *) fp)[2], ((long *) fp)[3], 
((long *) fp)[4],
((long *) fp)[5],
((long *) fp)[6],
((long *) fp)[7]
);
	}
fp = dtrace_getfp();
*/

	for (i = 1; i <= aframes; i++) {
//printk("i=%d fp=%p aframes=%d pc:%p\n", i, fp, aframes, fp->fr_savpc);
		fp = (struct frame *)(fp->fr_savfp);
#if defined(linux)
		/***********************************************/
		/*   Temporary hack - not sure which stack we  */
		/*   have here and it is faultiing us.	       */
		/***********************************************/
		if (fp == NULL)
			return 0;
#endif

		if (fp->fr_savpc == (pc_t)dtrace_invop_callsite) {
#if !defined(__amd64)
			/*
			 * If we pass through the invalid op handler, we will
			 * use the pointer that it passed to the stack as the
			 * second argument to dtrace_invop() as the pointer to
			 * the stack.
			 */
			stack = ((uintptr_t **)&fp[1])[1];
#else
			/*
			 * In the case of amd64, we will use the pointer to the
			 * regs structure that was pushed when we took the
			 * trap.  To get this structure, we must increment
			 * beyond the frame structure.  If the argument that
			 * we're seeking is passed on the stack, we'll pull
			 * the true stack pointer out of the saved registers
			 * and decrement our argument by the number of
			 * arguments passed in registers; if the argument
			 * we're seeking is passed in regsiters, we can just
			 * load it directly.
			 */
			struct regs *rp = (struct regs *)((uintptr_t)&fp[1] +
			    sizeof (uintptr_t));

			if (argno <= inreg) {
				stack = (uintptr_t *)&rp->r_rdi;
			} else {
				stack = (uintptr_t *)(rp->r_rsp);
				argno -= (inreg + 1);
			}
#endif
			goto load;
		}
	}
//printk("stack %d: %p %p %p %p\n", i, ((long *) fp)[0], ((long *) fp)[1], ((long *) fp)[2], ((long *) fp)[3], ((long *) fp)[4]);

	/*
	 * We know that we did not come through a trap to get into
	 * dtrace_probe() -- the provider simply called dtrace_probe()
	 * directly.  As this is the case, we need to shift the argument
	 * that we're looking for:  the probe ID is the first argument to
	 * dtrace_probe(), so the argument n will actually be found where
	 * one would expect to find argument (n + 1).
	 */
	argno++;

#if defined(__amd64)
	if (argno <= inreg) {
		/*
		 * This shouldn't happen.  If the argument is passed in a
		 * register then it should have been, well, passed in a
		 * register...
		 */
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	}

	argno -= (inreg + 1);
#endif
	stack = (uintptr_t *)&fp[1];

load:
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	val = stack[argno];
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);
printk("sdt_getarg#%d: %lx aframes=%d\n", argno, val, aframes);

	return (val);
}

static dtrace_pops_t sdt_pops = {
	NULL,
	sdt_provide_module,
	sdt_enable,
	sdt_disable,
	NULL,
	NULL,
	sdt_getargdesc,
	sdt_getarg,
	NULL,
	sdt_destroy
};

/*ARGSUSED*/
static int
sdt_attach(void)
{
	sdt_provider_t *prov;
# if defined(sun)

	if (ddi_create_minor_node(devi, "sdt", S_IFCHR,
	    0, DDI_PSEUDO, NULL) == DDI_FAILURE) {
		cmn_err(CE_NOTE, "/dev/sdt couldn't create minor node");
		ddi_remove_minor_node(devi, NULL);
		return (DDI_FAILURE);
	}

	ddi_report_dev(devi);
	sdt_devi = devi;
# endif

	if (sdt_probetab_size == 0)
		sdt_probetab_size = SDT_PROBETAB_SIZE;

	sdt_probetab_mask = sdt_probetab_size - 1;
	sdt_probetab =
	    kmem_zalloc(sdt_probetab_size * sizeof (sdt_probe_t *), KM_SLEEP);
	dtrace_invop_add(sdt_invop);

	for (prov = sdt_providers; prov->sdtp_name != NULL; prov++) {
		if (dtrace_register(prov->sdtp_name, prov->sdtp_attr,
		    DTRACE_PRIV_KERNEL, NULL,
		    &sdt_pops, prov, &prov->sdtp_id) != 0) {
			cmn_err(CE_WARN, "failed to register sdt provider %s",
			    prov->sdtp_name);
		}

		if (strcmp(prov->sdtp_name, "io") == 0) {
			io_prov = prov;
		}
	}

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
sdt_detach(void)
{
	sdt_provider_t *prov;

	for (prov = sdt_providers; prov->sdtp_name != NULL; prov++) {
		if (prov->sdtp_id != DTRACE_PROVNONE) {
			if (dtrace_unregister(prov->sdtp_id) != 0)
				return (DDI_FAILURE);

			prov->sdtp_id = DTRACE_PROVNONE;
		}
	}

	dtrace_invop_remove(sdt_invop);
	kmem_free(sdt_probetab, sdt_probetab_size * sizeof (sdt_probe_t *));

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
sdt_open(struct inode *inode, struct file *file)
{
	return (0);
}

static const struct file_operations sdt_fops = {
	.owner = THIS_MODULE,
        .open = sdt_open,
};

static struct miscdevice sdt_dev = {
        MISC_DYNAMIC_MINOR,
        "sdt",
        &sdt_fops
};

static int initted;

int sdt_init(void)
{	int	ret;

	ret = misc_register(&sdt_dev);
	if (ret) {
		printk(KERN_WARNING "sdt: Unable to register misc device\n");
		return ret;
		}

	sdt_attach();

	dtrace_printf("sdt loaded: /dev/sdt now available\n");

	initted = 1;

	return 0;
}
void sdt_exit(void)
{
	if (initted) {
		sdt_detach();
		misc_deregister(&sdt_dev);
	}

/*	printk(KERN_WARNING "sdt driver unloaded.\n");*/
}

/**********************************************************************/
/*   Dynamic SDT traces based on discovery from the loaded kernel or  */
/*   modules.							      */
/*   								      */
/*   Placed  at  end  of  file,  since  we need the ref to sdt_pops,  */
/*   above.							      */
/**********************************************************************/
#define MAX_DYN_SDT 32
static sdt_provider_t sdt_dyn_providers[MAX_DYN_SDT];

int
io_prov_sdt(pf_info_t *infp, uint8_t *instr, int size, int modrm)
{
	sdt_probe_t *sdp;
	sdt_provider_t *prov;
	uint8_t *offset;
	char	*name;
	char	namebuf[KSYM_NAME_LEN];
	char	provname[64];
	char	modname[64];
	char	probename[64];
	char	funcname[64];
	int	sz;
	char	*cp, *cp1;
	unsigned long addr = (unsigned long) instr + 5 + *(int32_t *) (instr+1);

static dtrace_pattr_t sdt_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

printk("io_prov_sdt called %s:%s\n", infp->modname, infp->name);

	/***********************************************/
	/*   Map   the   target   of   the  SDT  call  */
	/*   instruction into a symbol name so we can  */
	/*   extract the provider and probe name.      */
	/***********************************************/
	sz = get_proc_name(addr, namebuf);
//	printk("io_prov_sdt: func=%s\n", namebuf);
	if (strncmp(namebuf, "__dtrace_", 9) != 0)
		return 1;
	cp1 = namebuf + 9;
	for (cp = provname; cp < &provname[sizeof provname - 1]; cp++) {
		if (strncmp(cp1, "___", 3) == 0)
			break;
		*cp = *cp1++;
	}
	*cp = '\0';
	cp1 += 3;
	for (cp = modname; cp < &modname[sizeof modname - 1]; cp++) {
		if (strncmp(cp1, "___", 3) == 0)
			break;
		*cp = *cp1++;
	}
	*cp = '\0';
	cp1 += 3;
	for (cp = probename; cp < &probename[sizeof probename - 1]; cp++) {
		if (strncmp(cp1, "___", 3) == 0)
			break;
		*cp = *cp1++;
	}
	*cp = '\0';
	cp1 += 3;
	for (cp = funcname; cp < &funcname[sizeof funcname - 1]; cp++) {
		if (strncmp(cp1, "___", 3) == 0)
			break;
		*cp = *cp1++;
	}
	*cp = '\0';
	cp1 += 3;
printk("io_prov_sdt: func=%s %s:%s:%s:%s\n", namebuf, provname, modname, probename, funcname);

	/***********************************************/
	/*   Try and find an existing version of this  */
	/*   provider,  else  allocate  a new one and  */
	/*   register it.			       */
	/***********************************************/
	for (prov = sdt_dyn_providers; prov->sdtp_prefix != NULL; prov++) {
		if (strcmp(prov->sdtp_name, provname) == 0)
			break;
	}
	/***********************************************/
	/*   If  not  already  in the table, register  */
	/*   the provider.			       */
	/***********************************************/
	if (prov->sdtp_prefix == NULL) {
		extern mutex_t dtrace_provider_lock;

		if (prov - sdt_dyn_providers >= MAX_DYN_SDT - 1) {
			printk("out of table space: %d slots available\n", MAX_DYN_SDT);
			return 1;
			}
		prov->sdtp_name = kstrdup(provname, KM_SLEEP);
		prov->sdtp_attr = &sdt_attr;

		/***********************************************/
		/*   This  is  ugly  - we are already holding  */
		/*   the    lock    so    we    cannot   call  */
		/*   dtrace_register.  But  module loading is  */
		/*   rare,  so  any  race conditions shouldnt  */
		/*   exist to allow a re-entrancy problem.     */
		/***********************************************/
		mutex_exit(&dtrace_provider_lock);
		if (dtrace_register(prov->sdtp_name, prov->sdtp_attr,
		    DTRACE_PRIV_KERNEL, NULL,
		    &sdt_pops, prov, &prov->sdtp_id) != 0) {
			mutex_enter(&dtrace_provider_lock);
			cmn_err(CE_WARN, "failed to register sdt provider %s",
			    prov->sdtp_name);
			return 1;
		}
		mutex_enter(&dtrace_provider_lock);
	}

	name = kstrdup(probename, KM_SLEEP);
	sdp = kmem_zalloc(sizeof (sdt_probe_t), KM_SLEEP);
	sdp->sdp_id = dtrace_probe_create(prov->sdtp_id,
			    infp->modname, probename, funcname, 0, sdp);
	sdp->sdp_name = name;
	sdp->sdp_namelen = strlen(name);
	sdp->sdp_inslen = size;
	sdp->sdp_modrm = modrm;
	sdp->sdp_provider = prov;
	sdp->sdp_flags = infp->flags;
	sdp->sdp_entry = FALSE;

	/***********************************************/
	/*   Add the entry to the hash table.	       */
	/***********************************************/
	offset = instr;
	sdp->sdp_hashnext =
	    sdt_probetab[SDT_ADDR2NDX(offset)];
	sdt_probetab[SDT_ADDR2NDX(offset)] = sdp;

	sdp->sdp_patchval = PATCHVAL;
	sdp->sdp_patchpoint = (uint8_t *)offset;
	sdp->sdp_savedval = *sdp->sdp_patchpoint;

	infp->retptr = NULL;
	return 1;
}


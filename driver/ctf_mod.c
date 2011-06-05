/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file usr/src/LICENSING.NOTICE in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

//#pragma ident	"@(#)ctf_mod.c	1.1	03/09/02 SMI"

#include <dtrace_linux.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>

# define ctf_open Xctf_open
# define ctf_write Xctf_write
#include <ctf_impl.h>
# undef ctf_open
# undef ctf_write

MODULE_AUTHOR("Paul Fox");
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_DESCRIPTION("DTRACE CTF Driver");

int ctf_leave_compressed = 0;

void dtrace_printf(char *, ...);

/*ARGSUSED*/
void *
ctf_zopen(int *errp)
{
        return ((void *)1); /* zmod is always loaded because we depend on it */
}

/*ARGSUSED*/
const void *
ctf_sect_mmap(ctf_sect_t *sp, int fd)
{
	return (MAP_FAILED); /* we don't support this in the kernel */
}

/*ARGSUSED*/
void
ctf_sect_munmap(const ctf_sect_t *sp)
{
	/* we don't support this in the kernel */
}

/*ARGSUSED*/
ctf_file_t *
ctf_fdopen(int fd, int *errp)
{
	return (ctf_set_open_errno(errp, ENOTSUP));
}

# if 0
/*ARGSUSED*/
ctf_file_t *
ctf_open(const char *filename, int *errp)
{
	return (ctf_set_open_errno(errp, ENOTSUP));
}
# endif
/*ARGSUSED*/
ssize_t
ctf_write(struct file *fp, const char __user *buf, size_t len, loff_t *off)
{
	return -ENOTSUP;
}

int
ctf_version(int version)
{
	ASSERT(version > 0 && version <= CTF_VERSION);

	if (version > 0)
		_libctf_version = MIN(CTF_VERSION, version);

	return (_libctf_version);
}

# if 0
static int ctf_ioctl(struct inode *inode, struct file *file,
                     unsigned int cmd, unsigned long arg)
{
        return -EINVAL;
}
# endif
/*ARGSUSED*/
int
ctf_open(struct inode *ip, struct file *fp)
{
# if 1
	return 0;
# else
	ctf_sect_t ctfsect, symsect, strsect;
	ctf_file_t *fp = NULL;
	int err;

	if (error == NULL)
		error = &err;

	ctfsect.cts_name = ".SUNW_ctf";
	ctfsect.cts_type = SHT_PROGBITS;
	ctfsect.cts_flags = SHF_ALLOC;
	ctfsect.cts_data = mp->ctfdata;
	ctfsect.cts_size = mp->ctfsize;
	ctfsect.cts_entsize = 1;
	ctfsect.cts_offset = 0;

	symsect.cts_name = ".symtab";
	symsect.cts_type = SHT_SYMTAB;
	symsect.cts_flags = 0;
	symsect.cts_data = mp->symtbl;
	symsect.cts_size = mp->symhdr->sh_size;
#ifdef _LP64
	symsect.cts_entsize = sizeof (Elf64_Sym);
#else
	symsect.cts_entsize = sizeof (Elf32_Sym);
#endif
	symsect.cts_offset = 0;

	strsect.cts_name = ".strtab";
	strsect.cts_type = SHT_STRTAB;
	strsect.cts_flags = 0;
	strsect.cts_data = mp->strings;
	strsect.cts_size = mp->strhdr->sh_size;
	strsect.cts_entsize = 1;
	strsect.cts_offset = 0;

	ASSERT(MUTEX_HELD(&mod_lock));

	if ((fp = ctf_bufopen(&ctfsect, &symsect, &strsect, error)) == NULL)
		return (NULL);

	if (!ctf_leave_compressed && (caddr_t)fp->ctf_base != mp->ctfdata) {
		/*
		 * We must have just uncompressed the CTF data.  To avoid
		 * others having to pay the (substantial) cost of decompressing
		 * the data, we're going to substitute the uncompressed version
		 * for the compressed version.  Note that this implies that the
		 * first CTF consumer will induce memory impact on the system
		 * (but in the name of performance of future CTF consumers).
		 */
		kobj_set_ctf(mp, (caddr_t)fp->ctf_base, fp->ctf_size);
		fp->ctf_data.cts_data = fp->ctf_base;
		fp->ctf_data.cts_size = fp->ctf_size;
	}

	return (fp);
# endif
}
static const struct file_operations ctf_fops = {
        .write = ctf_write,
/*	.ioctl = ctf_ioctl,*/
        .open = ctf_open,
};

static struct miscdevice ctf_dev = {
        MISC_DYNAMIC_MINOR,
        "ctf",
        &ctf_fops
};

static int initted;

int
ctf_init(void)
{	int	ret;

	ret = misc_register(&ctf_dev);
	if (ret) {
		printk(KERN_WARNING "ctf: Unable to register misc device\n");
		return ret;
		}
	dtrace_printf("ctf driver loaded /proc/ctf and /dev/ctf now available\n");
	initted = TRUE;

	return 0;
}
void ctf_exit(void)
{
/*	printk(KERN_WARNING "ctf driver unloaded.\n");*/
	if (initted) {
		misc_deregister(&ctf_dev);
	}
}

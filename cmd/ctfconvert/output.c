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

#pragma ident	"@(#)output.c	1.16	06/08/22 SMI"

/*
 * Routines for preparing tdata trees for conversion into CTF data, and
 * for placing the resulting data into an output file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include <unistd.h>

#include "ctftools.h"
#include "list.h"
#include "memory.h"
#include "traverse.h"
#include "symbol.h"

#if defined(__APPLE__)
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/mman.h>

/* Prototype taken from <libiberty/demangle.h> (which won't #include cleanly) */
extern char *cplus_demangle(const char *, int);

static char 
*demangleSymbolCString(const char *mangled)
 {
     if(mangled[0]!='_') return NULL;
     if(mangled[1]=='_') mangled++; // allow either __Z or _Z prefix
     if(mangled[1]!='Z') return NULL;
     return cplus_demangle(mangled, 0);
 }

static GElf_Sym *
gelf_getsym_macho(Elf_Data * data, int ndx, int nent, GElf_Sym * dst, const char *base) 
{
	const struct nlist *nsym = ((const struct nlist *)(data->d_buf)) + ndx;
	const char *name = base + nsym->n_un.n_strx;
	char *tmp;

	if (0 == nsym->n_un.n_strx) // iff a null, "", name.
		name = "null name"; // return NULL;

	if ((tmp = demangleSymbolCString(name)))
		name = tmp;

	if ('_' == name[0])
		name++; // Lop off omnipresent underscore to match DWARF convention

	dst->st_name = (GElf_Sxword)(name - base);
	dst->st_value = nsym->n_value;
	dst->st_size = 0;
	dst->st_info = GELF_ST_INFO((STB_GLOBAL), (STT_NOTYPE));
	dst->st_other = 0;
	dst->st_shndx = SHN_MACHO; /* Mark underlying file as Mach-o */
	
	if (nsym->n_type & N_STAB) {
	
		switch(nsym->n_type) {
		case N_FUN:
			dst->st_info = GELF_ST_INFO((STB_GLOBAL), (STT_FUNC));
			break;
		case N_GSYM:
			dst->st_info = GELF_ST_INFO((STB_GLOBAL), (STT_OBJECT));
			break;
		default:
			break;
		}
		
	} else if ((N_ABS | N_EXT) == (nsym->n_type & (N_TYPE | N_EXT)) ||
		(N_SECT | N_EXT) == (nsym->n_type & (N_TYPE | N_EXT))) {

		dst->st_info = GELF_ST_INFO((STB_GLOBAL), (nsym->n_desc)); 
	} else if ((N_UNDF | N_EXT) == (nsym->n_type & (N_TYPE | N_EXT)) &&
				nsym->n_sect == NO_SECT) {
		dst->st_info = GELF_ST_INFO((STB_GLOBAL), (STT_OBJECT)); /* Common */
	} 
		
	return dst;
}

static GElf_Sym *
gelf_getsym_macho_64(Elf_Data * data, int ndx, int nent, GElf_Sym * dst, const char *base)
{
	const struct nlist_64 *nsym = ((const struct nlist_64 *)(data->d_buf)) + ndx;
	const char *name = base + nsym->n_un.n_strx;
	char *tmp;

	if (0 == nsym->n_un.n_strx) // iff a null, "", name.
		name = "null name"; // return NULL;

	if ((tmp = demangleSymbolCString(name)))
		name = tmp;

	if ('_' == name[0])
		name++; // Lop off omnipresent underscore to match DWARF convention

	dst->st_name = (GElf_Sxword)(name - base);
	dst->st_value = nsym->n_value;
	dst->st_size = 0;
	dst->st_info = GELF_ST_INFO((STB_GLOBAL), (STT_NOTYPE));
	dst->st_other = 0;
	dst->st_shndx = SHN_MACHO_64; /* Mark underlying file as Mach-o 64 */
	
	if (nsym->n_type & N_STAB) {
	
		switch(nsym->n_type) {
		case N_FUN:
			dst->st_info = GELF_ST_INFO((STB_GLOBAL), (STT_FUNC));
			break;
		case N_GSYM:
			dst->st_info = GELF_ST_INFO((STB_GLOBAL), (STT_OBJECT));
			break;
		default:
			break;
		}
		
	} else if ((N_ABS | N_EXT) == (nsym->n_type & (N_TYPE | N_EXT)) ||
		(N_SECT | N_EXT) == (nsym->n_type & (N_TYPE | N_EXT))) {

		dst->st_info = GELF_ST_INFO((STB_GLOBAL), (nsym->n_desc)); 
	} else if ((N_UNDF | N_EXT) == (nsym->n_type & (N_TYPE | N_EXT)) &&
				nsym->n_sect == NO_SECT) {
		dst->st_info = GELF_ST_INFO((STB_GLOBAL), (STT_OBJECT)); /* Common */
	} 
		
	return dst;
}
#endif /* __APPLE__ */

typedef struct iidesc_match {
	int iim_fuzzy;
	iidesc_t *iim_ret;
	char *iim_name;
	char *iim_file;
	uchar_t iim_bind;
} iidesc_match_t;

static int
burst_iitypes(void *data, void *arg)
{
	iidesc_t *ii = data;
	iiburst_t *iiburst = arg;

	switch (ii->ii_type) {
	case II_GFUN:
	case II_SFUN:
	case II_GVAR:
	case II_SVAR:
		if (!(ii->ii_flags & IIDESC_F_USED))
			return (0);
		break;
	default:
		break;
	}

	ii->ii_dtype->t_flags |= TDESC_F_ISROOT;
	(void) iitraverse_td(ii, iiburst->iib_tdtd);
	return (1);
}

/*ARGSUSED1*/
static int
save_type_by_id(tdesc_t *tdp, tdesc_t **tdpp, void *private)
{
	iiburst_t *iiburst = private;

	/*
	 * Doing this on every node is horribly inefficient, but given that
	 * we may be suppressing some types, we can't trust nextid in the
	 * tdata_t.
	 */
	if (tdp->t_id > iiburst->iib_maxtypeid)
		iiburst->iib_maxtypeid = tdp->t_id;

	slist_add(&iiburst->iib_types, tdp, tdesc_idcmp);

	return (1);
}

static tdtrav_cb_f burst_types_cbs[] = {
	NULL,
	save_type_by_id,	/* intrinsic */
	save_type_by_id,	/* pointer */
	save_type_by_id,	/* array */
	save_type_by_id,	/* function */
	save_type_by_id,	/* struct */
	save_type_by_id,	/* union */
	save_type_by_id,	/* enum */
	save_type_by_id,	/* forward */
	save_type_by_id,	/* typedef */
	tdtrav_assert,		/* typedef_unres */
	save_type_by_id,	/* volatile */
	save_type_by_id,	/* const */
	save_type_by_id		/* restrict */
};


static iiburst_t *
iiburst_new(tdata_t *td, int max)
{
	iiburst_t *iiburst = xcalloc(sizeof (iiburst_t));
	iiburst->iib_td = td;
	iiburst->iib_funcs = xcalloc(sizeof (iidesc_t *) * max);
	iiburst->iib_nfuncs = 0;
	iiburst->iib_objts = xcalloc(sizeof (iidesc_t *) * max);
	iiburst->iib_nobjts = 0;
	return (iiburst);
}

static void
iiburst_types(iiburst_t *iiburst)
{
	tdtrav_data_t tdtd;

	tdtrav_init(&tdtd, &iiburst->iib_td->td_curvgen, NULL, burst_types_cbs,
	    NULL, (void *)iiburst);

	iiburst->iib_tdtd = &tdtd;

	(void) hash_iter(iiburst->iib_td->td_iihash, burst_iitypes, iiburst);
}

static void
iiburst_free(iiburst_t *iiburst)
{
	free(iiburst->iib_funcs);
	free(iiburst->iib_objts);
	list_free(iiburst->iib_types, NULL, NULL);
	free(iiburst);
}

/*
 * See if this iidesc matches the ELF symbol data we pass in.
 *
 * A fuzzy match is where we have a local symbol matching the name of a
 * global type description. This is common when a mapfile is used for a
 * DSO, but we don't accept it by default.
 *
 * A weak fuzzy match is when a weak symbol was resolved and matched to
 * a global type description.
 */
static int
matching_iidesc(iidesc_t *iidesc, iidesc_match_t *match)
{
	if (streq(iidesc->ii_name, match->iim_name) == 0)
		return (0);

	switch (iidesc->ii_type) {
	case II_GFUN:
	case II_GVAR:
		if (match->iim_bind == STB_GLOBAL) {
			match->iim_ret = iidesc;
			return (-1);
		} else if (match->iim_fuzzy && match->iim_ret == NULL) {
			match->iim_ret = iidesc;
			/* continue to look for strong match */
			return (0);
		}
		break;
	case II_SFUN:
	case II_SVAR:
		if (match->iim_bind == STB_LOCAL &&
		    match->iim_file != NULL &&
		    streq(iidesc->ii_owner, match->iim_file)) {
			match->iim_ret = iidesc;
			return (-1);
		}
		break;
	}
	return (0);
}

static iidesc_t *
find_iidesc(tdata_t *td, iidesc_match_t *match)
{
	match->iim_ret = NULL;
	iter_iidescs_by_name(td, match->iim_name,
	    (int (*)())matching_iidesc, match);
	return (match->iim_ret);
}

/*
 * If we have a weak symbol, attempt to find the strong symbol it will
 * resolve to.  Note: the code where this actually happens is in
 * sym_process() in cmd/sgs/libld/common/syms.c
 *
 * Finding the matching symbol is unfortunately not trivial.  For a
 * symbol to be a candidate, it must:
 *
 * - have the same type (function, object)
 * - have the same value (address)
 * - have the same size
 * - not be another weak symbol
 * - belong to the same section (checked via section index)
 *
 * If such a candidate is global, then we assume we've found it.  The
 * linker generates the symbol table such that the curfile might be
 * incorrect; this is OK for global symbols, since find_iidesc() doesn't
 * need to check for the source file for the symbol.
 *
 * We might have found a strong local symbol, where the curfile is
 * accurate and matches that of the weak symbol.  We assume this is a
 * reasonable match.
 *
 * If we've got a local symbol with a non-matching curfile, there are
 * two possibilities.  Either this is a completely different symbol, or
 * it's a once-global symbol that was scoped to local via a mapfile.  In
 * the latter case, curfile is likely inaccurate since the linker does
 * not preserve the needed curfile in the order of the symbol table (see
 * the comments about locally scoped symbols in libld's update_osym()).
 * As we can't tell this case from the former one, we use this symbol
 * iff no other matching symbol is found.
 *
 * What we really need here is a SUNW section containing weak<->strong
 * mappings that we can consume.
 */
static int
check_for_weak(GElf_Sym *weak, char const *weakfile,
    Elf_Data *data, int nent, Elf_Data *strdata,
    GElf_Sym *retsym, char **curfilep)
{
	char *curfile = NULL;
	char *tmpfile;
	GElf_Sym tmpsym;
	int candidate = 0;
	int i;

	if (GELF_ST_BIND(weak->st_info) != STB_WEAK)
		return (0);

	for (i = 0; i < nent; i++) {
		GElf_Sym sym;
		uchar_t type;

		if (gelf_getsym(data, i, &sym) == NULL)
			continue;

		type = GELF_ST_TYPE(sym.st_info);

		if (type == STT_FILE)
			curfile = (char *)strdata->d_buf + sym.st_name;

		if (GELF_ST_TYPE(weak->st_info) != type ||
		    weak->st_value != sym.st_value)
			continue;

		if (weak->st_size != sym.st_size)
			continue;

		if (GELF_ST_BIND(sym.st_info) == STB_WEAK)
			continue;

		if (sym.st_shndx != weak->st_shndx)
			continue;

		if (GELF_ST_BIND(sym.st_info) == STB_LOCAL &&
		    (curfile == NULL || weakfile == NULL ||
		    strcmp(curfile, weakfile) != 0)) {
			candidate = 1;
			tmpfile = curfile;
			tmpsym = sym;
			continue;
		}

		*curfilep = curfile;
		*retsym = sym;
		return (1);
	}

	if (candidate) {
		*curfilep = tmpfile;
		*retsym = tmpsym;
		return (1);
	}

	return (0);
}

/*
 * When we've found the underlying symbol's type description
 * for a weak symbol, we need to copy it and rename it to match
 * the weak symbol. We also need to add it to the td so it's
 * handled along with the others later.
 */
static iidesc_t *
copy_from_strong(tdata_t *td, GElf_Sym *sym, iidesc_t *strongdesc,
    const char *weakname, const char *weakfile)
{
	iidesc_t *new = iidesc_dup_rename(strongdesc, weakname, weakfile);
	uchar_t type = GELF_ST_TYPE(sym->st_info);

	switch (type) {
	case STT_OBJECT:
		new->ii_type = II_GVAR;
		break;
	case STT_FUNC:
		new->ii_type = II_GFUN;
		break;
	}

	hash_add(td->td_iihash, new);

	return (new);
}

/*
 * Process the symbol table of the output file, associating each symbol
 * with a type description if possible, and sorting them into functions
 * and data, maintaining symbol table order.
 */
static iiburst_t *
sort_iidescs(Elf *elf, const char *file, tdata_t *td, int fuzzymatch,
    int dynsym)
{
	iiburst_t *iiburst;
	Elf_Scn *scn;
	GElf_Shdr shdr;
	Elf_Data *data, *strdata;
	int i, stidx;
	int nent;
	iidesc_match_t match;

	match.iim_fuzzy = fuzzymatch;
	match.iim_file = NULL;

	if ((stidx = findelfsecidx(elf, file,
	    dynsym ? ".dynsym" : ".symtab")) < 0)
#if !defined(__APPLE__)
		terminate("%s: Can't open symbol table\n", file);
#else
        terminate(""); /* missing symbol table is most likely an empty binary,
                        * produce no output, but also don't warn the user. */
#endif
	scn = elf_getscn(elf, stidx);
	data = elf_getdata(scn, NULL);
	gelf_getshdr(scn, &shdr);
	nent = shdr.sh_size / shdr.sh_entsize;

#if !defined(__APPLE__)
	scn = elf_getscn(elf, shdr.sh_link);
	strdata = elf_getdata(scn, NULL);
#else
	if (SHN_MACHO !=  shdr.sh_link && SHN_MACHO_64 !=  shdr.sh_link) { 
		scn = elf_getscn(elf, shdr.sh_link);
		strdata = elf_getdata(scn, NULL);
	} else {
		/* Underlying file is Mach-o */
		int dir_idx;

		if ((dir_idx = findelfsecidx(elf, file, ".dir_str_table")) < 0 || 
		    (scn = elf_getscn(elf, dir_idx)) == NULL ||
		    (strdata = elf_getdata(scn, NULL)) == NULL)
			terminate("%s: Can't open direct string table\n", file);
	}
#endif /* __APPLE__ */

	iiburst = iiburst_new(td, nent);

#if !defined(__APPLE__)
	for (i = 0; i < nent; i++) {
		GElf_Sym sym;
		iidesc_t **tolist;
		GElf_Sym ssym;
		iidesc_match_t smatch;
		int *curr;
		iidesc_t *iidesc;

		if (gelf_getsym(data, i, &sym) == NULL)
			elfterminate(file, "Couldn't read symbol %d", i);

		match.iim_name = (char *)strdata->d_buf + sym.st_name;
		match.iim_bind = GELF_ST_BIND(sym.st_info);

		switch (GELF_ST_TYPE(sym.st_info)) {
		case STT_FILE:
			match.iim_file = match.iim_name;
			continue;
		case STT_OBJECT:
			tolist = iiburst->iib_objts;
			curr = &iiburst->iib_nobjts;
			break;
		case STT_FUNC:
			tolist = iiburst->iib_funcs;
			curr = &iiburst->iib_nfuncs;
			break;
		default:
			continue;
		}

		if (ignore_symbol(&sym, match.iim_name))
			continue;

		iidesc = find_iidesc(td, &match);

		if (iidesc != NULL) {
			tolist[*curr] = iidesc;
			iidesc->ii_flags |= IIDESC_F_USED;
			(*curr)++;
			continue;
		}

		if (!check_for_weak(&sym, match.iim_file, data, nent, strdata,
		    &ssym, &smatch.iim_file)) {
			(*curr)++;
			continue;
		}

		smatch.iim_fuzzy = fuzzymatch;
		smatch.iim_name = (char *)strdata->d_buf + ssym.st_name;
		smatch.iim_bind = GELF_ST_BIND(ssym.st_info);

		debug(3, "Weak symbol %s resolved to %s\n", match.iim_name,
		    smatch.iim_name);

		iidesc = find_iidesc(td, &smatch);

		if (iidesc != NULL) {
			tolist[*curr] = copy_from_strong(td, &sym,
			    iidesc, match.iim_name, match.iim_file);
			tolist[*curr]->ii_flags |= IIDESC_F_USED;
		}

		(*curr)++;
	}
#else
	for (i = 0; i < nent; i++) {
		GElf_Sym sym;
		iidesc_t **tolist;
		int *curr;
		iidesc_t *iidesc;

		if (SHN_MACHO == shdr.sh_link) {
			if (gelf_getsym_macho(data, i, nent, &sym, (const char *)strdata->d_buf) == NULL)
				elfterminate(file, "Couldn't read symbol %d", i);
		} else if (SHN_MACHO_64 == shdr.sh_link) {
			if (gelf_getsym_macho_64(data, i, nent, &sym, (const char *)strdata->d_buf) == NULL)
				elfterminate(file, "Couldn't read symbol %d", i);
		}

		match.iim_name = (char *)strdata->d_buf + sym.st_name;
		match.iim_bind = GELF_ST_BIND(sym.st_info);
		
		switch (GELF_ST_TYPE(sym.st_info)) {
		case STT_FILE:
			match.iim_file = match.iim_name;
			continue;
		case STT_OBJECT:
			tolist = iiburst->iib_objts;
			curr = &iiburst->iib_nobjts;
			break;
		case STT_FUNC:
			tolist = iiburst->iib_funcs;
			curr = &iiburst->iib_nfuncs;
			break;
		default:
			continue;
		}

		if (ignore_symbol(&sym, match.iim_name))
			continue;

		iidesc = find_iidesc(td, &match);

		if (iidesc != NULL) {
			tolist[*curr] = iidesc;
			iidesc->ii_flags |= IIDESC_F_USED;
			(*curr)++;
			continue;
		}

		if (ignore_symbol(&sym, match.iim_name))
			continue;

#warning FIXME: deal with weak bindings.

		(*curr)++;
	}	
#endif /* __APPLE__ */

	/*
	 * Stabs are generated for every function declared in a given C source
	 * file.  When converting an object file, we may encounter a stab that
	 * has no symbol table entry because the optimizer has decided to omit
	 * that item (for example, an unreferenced static function).  We may
	 * see iidescs that do not have an associated symtab entry, and so
	 * we do not write records for those functions into the CTF data.
	 * All others get marked as a root by this function.
	 */
	iiburst_types(iiburst);

	/*
	 * By not adding some of the functions and/or objects, we may have
	 * caused some types that were referenced solely by those
	 * functions/objects to be suppressed.  This could cause a label,
	 * generated prior to the evisceration, to be incorrect.  Find the
	 * highest type index, and change the label indicies to be no higher
	 * than this value.
	 */
	tdata_label_newmax(td, iiburst->iib_maxtypeid);

	return (iiburst);
}

#if !defined(__APPLE__)
#if defined(linux)
void
#else
static void
#endif
write_file(Elf *src, const char *srcname, Elf *dst, const char *dstname,
    caddr_t ctfdata, size_t ctfsize, int flags)
{
	GElf_Ehdr sehdr, dehdr;
	Elf_Scn *sscn, *dscn;
	Elf_Data *sdata, *ddata;
	GElf_Shdr shdr;
	GElf_Word symtab_type;
	int symtab_idx = -1;
	off_t new_offset = 0;
	off_t ctfnameoff = 0;
	int dynsym = (flags & CTF_USE_DYNSYM);
	int keep_stabs = (flags & CTF_KEEP_STABS);
	int *secxlate;
	int srcidx, dstidx;
	int curnmoff = 0;
	int changing = 0;
	int pad;
	int i;

	if (gelf_newehdr(dst, gelf_getclass(src)) == NULL)
		elfterminate(dstname, "Cannot copy ehdr to temp file");
	gelf_getehdr(src, &sehdr);
	memcpy(&dehdr, &sehdr, sizeof (GElf_Ehdr));
	gelf_update_ehdr(dst, &dehdr);

	symtab_type = dynsym ? SHT_DYNSYM : SHT_SYMTAB;

	/*
	 * Neither the existing stab sections nor the SUNW_ctf sections (new or
	 * existing) are SHF_ALLOC'd, so they won't be in areas referenced by
	 * program headers.  As such, we can just blindly copy the program
	 * headers from the existing file to the new file.
	 */
	if (sehdr.e_phnum != 0) {
		(void) elf_flagelf(dst, ELF_C_SET, ELF_F_LAYOUT);
		if (gelf_newphdr(dst, sehdr.e_phnum) == NULL)
			elfterminate(dstname, "Cannot make phdrs in temp file");

		for (i = 0; i < sehdr.e_phnum; i++) {
			GElf_Phdr phdr;

			gelf_getphdr(src, i, &phdr);
			gelf_update_phdr(dst, i, &phdr);
		}
	}

	secxlate = xmalloc(sizeof (int) * sehdr.e_shnum);
	for (srcidx = dstidx = 0; srcidx < sehdr.e_shnum; srcidx++) {
		Elf_Scn *scn = elf_getscn(src, srcidx);
		GElf_Shdr shdr;
		char *sname;

		gelf_getshdr(scn, &shdr);
		sname = elf_strptr(src, sehdr.e_shstrndx, shdr.sh_name);
		if (sname == NULL) {
			elfterminate(srcname, "Can't find string at %u",
			    shdr.sh_name);
		}

		if (strcmp(sname, CTF_ELF_SCN_NAME) == 0) {
			secxlate[srcidx] = -1;
		} else if (!keep_stabs &&
		    (strncmp(sname, ".stab", 5) == 0 ||
		    strncmp(sname, ".debug", 6) == 0 ||
		    strncmp(sname, ".rel.debug", 10) == 0 ||
		    strncmp(sname, ".rela.debug", 11) == 0)) {
			secxlate[srcidx] = -1;
		} else if (dynsym && shdr.sh_type == SHT_SYMTAB) {
			/*
			 * If we're building CTF against the dynsym,
			 * we'll rip out the symtab so debuggers aren't
			 * confused.
			 */
			secxlate[srcidx] = -1;
		} else {
			secxlate[srcidx] = dstidx++;
			curnmoff += strlen(sname) + 1;
		}

		new_offset = (off_t)dehdr.e_phoff;
	}

	for (srcidx = 1; srcidx < sehdr.e_shnum; srcidx++) {
		char *sname;

		sscn = elf_getscn(src, srcidx);
		gelf_getshdr(sscn, &shdr);

		if (secxlate[srcidx] == -1) {
			changing = 1;
			continue;
		}

		dscn = elf_newscn(dst);

		/*
		 * If this file has program headers, we need to explicitly lay
		 * out sections.  If none of the sections prior to this one have
		 * been removed, then we can just use the existing location.  If
		 * one or more sections have been changed, then we need to
		 * adjust this one to avoid holes.
		 */
		if (changing && sehdr.e_phnum != 0) {
			pad = new_offset % shdr.sh_addralign;

			if (pad)
				new_offset += shdr.sh_addralign - pad;
			shdr.sh_offset = new_offset;
		}

		shdr.sh_link = secxlate[shdr.sh_link];

		if (shdr.sh_type == SHT_REL || shdr.sh_type == SHT_RELA)
			shdr.sh_info = secxlate[shdr.sh_info];

		sname = elf_strptr(src, sehdr.e_shstrndx, shdr.sh_name);
		if (sname == NULL) {
			elfterminate(srcname, "Can't find string at %u",
			    shdr.sh_name);
		}
		if ((sdata = elf_getdata(sscn, NULL)) == NULL)
			elfterminate(srcname, "Cannot get sect %s data", sname);
		if ((ddata = elf_newdata(dscn)) == NULL)
			elfterminate(dstname, "Can't make sect %s data", sname);
		bcopy(sdata, ddata, sizeof (Elf_Data));

		if (srcidx == sehdr.e_shstrndx) {
			char seclen = strlen(CTF_ELF_SCN_NAME);

			ddata->d_buf = xmalloc(ddata->d_size + shdr.sh_size +
			    seclen + 1);
			bcopy(sdata->d_buf, ddata->d_buf, shdr.sh_size);
			strcpy((caddr_t)ddata->d_buf + shdr.sh_size,
			    CTF_ELF_SCN_NAME);
			ctfnameoff = (off_t)shdr.sh_size;
			shdr.sh_size += seclen + 1;
			ddata->d_size += seclen + 1;

			if (sehdr.e_phnum != 0)
				changing = 1;
		}

		if (shdr.sh_type == symtab_type && shdr.sh_entsize != 0) {
			int nsym = shdr.sh_size / shdr.sh_entsize;

			symtab_idx = secxlate[srcidx];

			ddata->d_buf = xmalloc(shdr.sh_size);
			bcopy(sdata->d_buf, ddata->d_buf, shdr.sh_size);

			for (i = 0; i < nsym; i++) {
				GElf_Sym sym;
				short newscn;

				(void) gelf_getsym(ddata, i, &sym);

				if (sym.st_shndx >= SHN_LORESERVE)
					continue;

				if ((newscn = secxlate[sym.st_shndx]) !=
				    sym.st_shndx) {
					sym.st_shndx =
					    (newscn == -1 ? 1 : newscn);

					gelf_update_sym(ddata, i, &sym);
				}
			}
		}

		if (gelf_update_shdr(dscn, &shdr) == NULL)
			elfterminate(dstname, "Cannot update sect %s", sname);

		new_offset = (off_t)shdr.sh_offset;
		if (shdr.sh_type != SHT_NOBITS)
			new_offset += shdr.sh_size;
	}

	if (symtab_idx == -1) {
		terminate("%s: Cannot find %s section\n", srcname,
		    dynsym ? "SHT_DYNSYM" : "SHT_SYMTAB");
	}

	/* Add the ctf section */
	dscn = elf_newscn(dst);
	gelf_getshdr(dscn, &shdr);
	shdr.sh_name = ctfnameoff;
	shdr.sh_type = SHT_PROGBITS;
	shdr.sh_size = ctfsize;
	shdr.sh_link = symtab_idx;
	shdr.sh_addralign = 4;
	if (changing && sehdr.e_phnum != 0) {
		pad = new_offset % shdr.sh_addralign;

		if (pad)
			new_offset += shdr.sh_addralign - pad;

		shdr.sh_offset = new_offset;
		new_offset += shdr.sh_size;
	}

	ddata = elf_newdata(dscn);
	ddata->d_buf = ctfdata;
	ddata->d_size = ctfsize;
	ddata->d_align = shdr.sh_addralign;

	gelf_update_shdr(dscn, &shdr);

	/* update the section header location */
	if (sehdr.e_phnum != 0) {
		size_t align = gelf_fsize(dst, ELF_T_ADDR, 1, EV_CURRENT);
		size_t r = new_offset % align;

		if (r)
			new_offset += align - r;

		dehdr.e_shoff = new_offset;
	}

	/* commit to disk */
	dehdr.e_shstrndx = secxlate[sehdr.e_shstrndx];
	gelf_update_ehdr(dst, &dehdr);
	if (elf_update(dst, ELF_C_WRITE) < 0)
		elfterminate(dstname, "Cannot finalize temp file");

	free(secxlate);
}
#else
#include "decl.h"
static void
write_file_64(Elf *src, const char *srcname, Elf *dst, const char *dstname,
    caddr_t ctfdata, size_t ctfsize, int flags); /* Forward reference. */

static void
fill_ctf_segments(struct segment_command *seg, struct section *sect, uint32_t vmaddr, size_t size, uint32_t offset, int swap)
{
	struct segment_command tmpseg = {
		LC_SEGMENT, 
		sizeof(struct segment_command) + sizeof(struct section),
		SEG_CTF,
		vmaddr,
		0, /* Do not map. Do not reserve virtual address range. */
		offset,
		size,
		VM_PROT_READ,
		VM_PROT_READ,
		1,
		0
	};

	struct section tmpsect = {
		SECT_CTF,
		SEG_CTF,
		vmaddr,
		size,
		offset,
		0, /* byte aligned */
		0,
		0,
		0,
		0, 
		0
	};
	
	if (swap) {
		__swap_segment_command(&tmpseg);
		__swap_section(&tmpsect);
	}

	*seg = tmpseg;
	*sect = tmpsect;
}

void
write_file(Elf *src, const char *srcname, Elf *dst, const char *dstname,
    caddr_t ctfdata, size_t ctfsize, int flags)
{
	struct mach_header hdr, *mh = (struct mach_header *)src->ed_image;
	struct segment_command ctfseg_command;
	struct section ctf_sect; 
	struct segment_command *curcmd, *ctfcmd;
	int fd, cmdsleft, swap = (MH_CIGAM == mh->magic);
	size_t sz;
	char *p;
	uint32_t ctf_vmaddr = 0, t;

	if (ELFCLASS64 == src->ed_class) {
		write_file_64(src, srcname, dst, dstname, ctfdata, ctfsize, flags);
		return;
	}
		
	/* Swap mach header to host order so we can do arithmetic */
	if (swap) { 
		hdr = *mh;
		mh = &hdr;
		__swap_mach_header(mh);
	}
	
	/* Get a pristine instance of the source mach-o */
	if ((fd = open(srcname, O_RDONLY)) < 0)
		terminate("%s: Cannot open for re-reading", srcname);
		
	sz = (size_t)lseek(fd, (off_t)0, SEEK_END);
	
	p = mmap((char *)0, sz, PROT_READ, MAP_PRIVATE, fd, (off_t)0);
	if ((char *)-1 == p)
		terminate("%s: Cannot mmap for re-reading", srcname);
		
	if ((MH_MAGIC != ((struct mach_header *)p)->magic) && 
			(MH_CIGAM != ((struct mach_header *)p)->magic))
		terminate("%s: is not a thin (single architecture) mach-o binary.\n", srcname);

	/* Iterate through load commands looking for CTF data */
	ctfcmd = NULL;
	cmdsleft = mh->ncmds;
	curcmd = (struct segment_command *) (p + sizeof(struct mach_header));
	
	if (cmdsleft < 1)
		terminate("%s: Has no load commands.\n", srcname);
		
	while (cmdsleft-- > 0) {
		int size = curcmd->cmdsize;
		uint32_t thecmd = curcmd->cmd;

		if (swap) {
			SWAP32(size);
			SWAP32(thecmd);
		}
		
		if (LC_SEGMENT == thecmd) {
			uint32_t vmaddr = curcmd->vmaddr;
			uint32_t vmsize = curcmd->vmsize;
			if (swap) {
				SWAP32(vmaddr);
				SWAP32(vmsize);
			}
			t = vmaddr + vmsize;
			if (t > ctf_vmaddr)
				ctf_vmaddr = t;
		}
			
		if ((LC_SEGMENT == thecmd) && (!strcmp(curcmd->segname, SEG_CTF))) {
			ctfcmd = curcmd;
		}
		
		curcmd = (struct segment_command *) (((char *)curcmd) + size);
	}

	ctf_vmaddr = (ctf_vmaddr + getpagesize() - 1) & (~(getpagesize() - 1)); // page aligned

	if (ctfcmd) {
		/* CTF segment command exists: overwrite it */
		fill_ctf_segments(&ctfseg_command, &ctf_sect, 
			((struct segment_command *)ctfcmd)->vmaddr, ctfsize, sz /* file offset */, swap);

		write(dst->ed_fd, p, sz); // byte-for-byte copy of input mach-o file
		write(dst->ed_fd, ctfdata, ctfsize); // append CTF 
		
		lseek(dst->ed_fd, (off_t)((char *)ctfcmd - p), SEEK_SET);
		write(dst->ed_fd, &ctfseg_command, sizeof(ctfseg_command)); // lay down CTF_SEG
		write(dst->ed_fd, &ctf_sect, sizeof(ctf_sect)); // lay down CTF_SECT
	} else { 
		int cmdlength, dataoffset, datalength;
		int ctfhdrsz = (sizeof(ctfseg_command) + sizeof(ctf_sect));

		cmdlength = mh->sizeofcmds; // where to write CTF seg/sect
		dataoffset = sizeof(*mh) + mh->sizeofcmds; // where all real data starts
		datalength = src->ed_imagesz - dataoffset;

		/* Add one segment command to header */
		mh->sizeofcmds += ctfhdrsz; 
		mh->ncmds++;
		/* 
		 * Chop the first ctfhdrsz bytes out of the generic data so
		 * that all the internal offsets stay the same
		 * (required for ELF parsing)
		 * FIXME: This isn't pretty.
		 */
		dataoffset += ctfhdrsz;
		datalength -= ctfhdrsz;

		fill_ctf_segments(&ctfseg_command, &ctf_sect, ctf_vmaddr, ctfsize, 
			(sizeof(*mh) + cmdlength + ctfhdrsz + datalength) /* file offset */, 
			swap);

		if (swap) {
			__swap_mach_header(mh);
		}

		write(dst->ed_fd, mh, sizeof(*mh));
		write(dst->ed_fd, p + sizeof(*mh), cmdlength); 
		write(dst->ed_fd, &ctfseg_command, sizeof(ctfseg_command));
		write(dst->ed_fd, &ctf_sect, sizeof(ctf_sect));
		write(dst->ed_fd, p + dataoffset, datalength); 
		write(dst->ed_fd, ctfdata, ctfsize);
	}
	
	(void)munmap(p, sz);
	(void)close(fd);
	
	return;
}

static void
fill_ctf_segments_64(struct segment_command_64 *seg, struct section_64 *sect, uint64_t vmaddr, size_t size, uint32_t offset, int swap)
{
	struct segment_command_64 tmpseg = {
		LC_SEGMENT_64,
		sizeof(struct segment_command_64) + sizeof(struct section_64),
		SEG_CTF,
		vmaddr,
		0, /* Do not map. Do not reserve virtual address range. */
		offset,
		size,
		VM_PROT_READ,
		VM_PROT_READ,
		1,
		0
	};

	struct section_64 tmpsect = {
		SECT_CTF,
		SEG_CTF,
		vmaddr,
		size,
		offset,
		0, /* byte aligned */
		0,
		0,
		0,
		0, 
		0
	};
	
	if (swap) {
		__swap_segment_command_64(&tmpseg);
		__swap_section_64(&tmpsect);
	}

	*seg = tmpseg;
	*sect = tmpsect;
}

static void
write_file_64(Elf *src, const char *srcname, Elf *dst, const char *dstname,
    caddr_t ctfdata, size_t ctfsize, int flags)
{
	struct mach_header_64 hdr, *mh = (struct mach_header_64 *)src->ed_image;
	struct segment_command_64 ctfseg_command;
	struct section_64 ctf_sect; 
	struct segment_command_64 *curcmd, *ctfcmd;
	int fd, cmdsleft, swap = (MH_CIGAM_64 == mh->magic);
	size_t sz;
	char *p;
	uint64_t ctf_vmaddr = 0, t;

	/* Swap mach header to host order so we can do arithmetic */
	if (swap) { 
		hdr = *mh;
		mh = &hdr;
		__swap_mach_header_64(mh);
	}
	
	/* Get a pristine instance of the source mach-o */
	if ((fd = open(srcname, O_RDONLY)) < 0)
		terminate("%s: Cannot open for re-reading", srcname);
		
	sz = (size_t)lseek(fd, (off_t)0, SEEK_END);
	
	p = mmap((char *)0, sz, PROT_READ, MAP_PRIVATE, fd, (off_t)0);
	if ((char *)-1 == p)
		terminate("%s: Cannot mmap for re-reading", srcname);
		
	if ((MH_MAGIC_64 != ((struct mach_header *)p)->magic) && 
			(MH_CIGAM_64 != ((struct mach_header *)p)->magic))
		terminate("%s: is not a thin (single architecture) mach-o binary.\n", srcname);

	/* Iterate through load commands looking for CTF data */
	ctfcmd = NULL;
	cmdsleft = mh->ncmds;
	curcmd = (struct segment_command_64 *) (p + sizeof(struct mach_header_64));
	
	if (cmdsleft < 1)
		terminate("%s: Has no load commands.\n", srcname);
		
	while (cmdsleft-- > 0) {
		int size = curcmd->cmdsize;
		uint32_t thecmd = curcmd->cmd;
		
		if (swap) {
			SWAP32(size);
			SWAP32(thecmd);
		}
		
		if (LC_SEGMENT_64 == thecmd) {
			uint64_t vmaddr = curcmd->vmaddr;
			uint64_t vmsize = curcmd->vmsize;
			if (swap) {
				SWAP64(vmaddr);
				SWAP64(vmsize);
			}
			t = vmaddr + vmsize;
			if (t > ctf_vmaddr)
				ctf_vmaddr = t;
		}

		if ((LC_SEGMENT_64 == thecmd) && (!strcmp(curcmd->segname, SEG_CTF))) {
			ctfcmd = curcmd;
		}
		
		curcmd = (struct segment_command_64 *) (((char *)curcmd) + size);
	}
	
	ctf_vmaddr = (ctf_vmaddr + getpagesize() - 1) & (~(getpagesize() - 1)); // page aligned

	if (ctfcmd) {
		/* CTF segment command exists: overwrite it */
		fill_ctf_segments_64(&ctfseg_command, &ctf_sect, 
			((struct segment_command_64 *)curcmd)->vmaddr, ctfsize, sz /* file offset */, swap);

		write(dst->ed_fd, p, sz); // byte-for-byte copy of input mach-o file
		write(dst->ed_fd, ctfdata, ctfsize); // append CTF 
		
		lseek(dst->ed_fd, (off_t)((char *)ctfcmd - p), SEEK_SET);
		write(dst->ed_fd, &ctfseg_command, sizeof(ctfseg_command)); // lay down CTF_SEG
		write(dst->ed_fd, &ctf_sect, sizeof(ctf_sect)); // lay down CTF_SECT
	} else { 
		int cmdlength, dataoffset, datalength;
		int ctfhdrsz = (sizeof(ctfseg_command) + sizeof(ctf_sect));

		cmdlength = mh->sizeofcmds; // where to write CTF seg/sect
		dataoffset = sizeof(*mh) + mh->sizeofcmds; // where all real data starts
		datalength = src->ed_imagesz - dataoffset;

		/* Add one segment command to header */
		mh->sizeofcmds += ctfhdrsz; 
		mh->ncmds++;
		/* 
		 * Chop the first ctfhdrsz bytes out of the generic data so
		 * that all the internal offsets stay the same
		 * (required for ELF parsing)
		 * FIXME: This isn't pretty.
		 */
		dataoffset += ctfhdrsz;
		datalength -= ctfhdrsz;

		fill_ctf_segments_64(&ctfseg_command, &ctf_sect, ctf_vmaddr, ctfsize, 
			(sizeof(*mh) + cmdlength + ctfhdrsz + datalength) /* file offset */, 
			swap);

		if (swap) {
			__swap_mach_header_64(mh);
		}

		write(dst->ed_fd, mh, sizeof(*mh));
		write(dst->ed_fd, p + sizeof(*mh), cmdlength); 
		write(dst->ed_fd, &ctfseg_command, sizeof(ctfseg_command));
		write(dst->ed_fd, &ctf_sect, sizeof(ctf_sect));
		write(dst->ed_fd, p + dataoffset, datalength); 
		write(dst->ed_fd, ctfdata, ctfsize);
	}
	
	(void)munmap(p, sz);
	(void)close(fd);
	
	return;
}

#endif /* __APPLE__ */

static caddr_t
make_ctf_data(tdata_t *td, Elf *elf, const char *file, size_t *lenp, int flags)
{
	iiburst_t *iiburst;
	caddr_t data;

	iiburst = sort_iidescs(elf, file, td, flags & CTF_FUZZY_MATCH,
	    flags & CTF_USE_DYNSYM);
#if !defined(__APPLE__)
	data = ctf_gen(iiburst, lenp, flags & CTF_COMPRESS);
#else
	data = ctf_gen(iiburst, lenp, flags & (CTF_COMPRESS | CTF_BYTESWAP));
#endif /* __APPLE__ */

	iiburst_free(iiburst);

	return (data);
}

void
write_ctf(tdata_t *td, const char *curname, const char *newname, int flags)
{
	struct stat st;
	Elf *elf = NULL;
	Elf *telf = NULL;
	caddr_t data;
	size_t len;
	int fd = -1;
	int tfd = -1;

	(void) elf_version(EV_CURRENT);
	if ((fd = open(curname, O_RDONLY)) < 0 || fstat(fd, &st) < 0)
		terminate("%s: Cannot open for re-reading", curname);
	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
		elfterminate(curname, "Cannot re-read");

	if ((tfd = open(newname, O_RDWR | O_CREAT | O_TRUNC, st.st_mode)) < 0)
		terminate("Cannot open temp file %s for writing", newname);
	if ((telf = elf_begin(tfd, ELF_C_WRITE, NULL)) == NULL)
		elfterminate(curname, "Cannot write");

#if defined(__APPLE__)
	/*
	 * If the caller has advised CTF_BYTESWAP but the target is the same
	 * byte order as this processor then clear CTF_BYTESWAP. Otherwise CTF_BYTESWAP
	 * stays lit and the output (typically from ctfmerge) is swapped to final form.
	 */
	if (ELFCLASS32 == elf->ed_class) {
		struct mach_header *mh = (struct mach_header *)elf->ed_image;
		if ((flags & CTF_BYTESWAP) && (MH_CIGAM != mh->magic))
			flags &= ~CTF_BYTESWAP;

	data = make_ctf_data(td, elf, curname, &len, flags);
	write_file(elf, curname, telf, newname, data, len, flags);
	} else if (ELFCLASS64 == elf->ed_class) {
		struct mach_header_64 *mh_64 = (struct mach_header_64 *)elf->ed_image;
		if ((flags & CTF_BYTESWAP) && (MH_CIGAM_64 != mh_64->magic))
			flags &= ~CTF_BYTESWAP;

		data = make_ctf_data(td, elf, curname, &len, flags);
		write_file_64(elf, curname, telf, newname, data, len, flags);
	} else
		terminate("%s: Unknown ed_class", curname);
#else
	data = make_ctf_data(td, elf, curname, &len, flags);
	write_file(elf, curname, telf, newname, data, len, flags);
#endif /* __APPLE__ */
	free(data);

	elf_end(telf);
	elf_end(elf);
	(void) close(fd);
	(void) close(tfd);
}

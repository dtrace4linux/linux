# if !defined(SYS_AUXV_H)
# define SYS_AUXV_H 1

typedef struct
{
        int     a_type;
        union {
                long    a_val;
#ifdef __STDC__
                void    *a_ptr;
#else
                char    *a_ptr;
#endif
                void    (*a_fcn)();
        } a_un;
} auxv_t;

typedef struct {
        int32_t a_type;
        union   {
                int32_t a_val;
                caddr32_t a_ptr;
                caddr32_t a_fcn;
        } a_un;
} auxv32_t;

/*
 * The following aux vector provides a pointer to a null-terminated
 * path name, a copy of the path name passed to the exec() system
 * call but that has had all symlinks resolved (see resolvepath(2)).
 */
#define AT_SUN_EXECNAME 2014    /* exec() path name */

#define AT_SUN_MMU      2015    /* mmu module name */
#define AT_SUN_LDDATA   2016    /* dynamic linkers data segment */

#define AT_SUN_AUXFLAGS 2017    /* AF_SUN_ flags passed from the kernel */

#define AT_NULL         0
#define AT_IGNORE       1
#define AT_EXECFD       2
#define AT_PHDR         3       /* &phdr[0] */
#define AT_PHENT        4       /* sizeof(phdr[0]) */
#define AT_PHNUM        5       /* # phdr entries */
#define AT_PAGESZ       6       /* getpagesize(2) */
#define AT_BASE         7       /* ld.so base addr */
#define AT_FLAGS        8       /* processor flags */
#define AT_ENTRY        9       /* a.out entry point */

# endif

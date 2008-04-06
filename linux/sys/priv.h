# if !defined(SYS_PRIV_H)
# define SYS_PRIV_H

typedef struct priv_impl_info {
        uint32_t        priv_headersize;        /* sizeof (priv_impl_info) */
        uint32_t        priv_flags;             /* additional flags */
        uint32_t        priv_nsets;             /* number of priv sets */
        uint32_t        priv_setsize;           /* size in priv_chunk_t */
        uint32_t        priv_max;               /* highest actual valid priv */
        uint32_t        priv_infosize;          /* Per proc. additional info */
        uint32_t        priv_globalinfosize;    /* Per system info */
} priv_impl_info_t;

#define PRIV_IMPL_INFO_SIZE(p) \
                        ((p)->priv_headersize + (p)->priv_globalinfosize)

#define PRIV_PRPRIV_INFO_OFFSET(p) \
                (sizeof (*(p)) + \
                ((p)->pr_nsets * (p)->pr_setsize - 1) * sizeof (priv_chunk_t))

#define PRIV_PRPRIV_SIZE(p) \
                (PRIV_PRPRIV_INFO_OFFSET(p) + (p)->pr_infosize)

# endif

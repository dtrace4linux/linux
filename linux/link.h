# if !defined(LINK_H)
# define LINK_H

# include "/usr/include/link.h"

typedef enum {
        RD_NONE = 0,            /* no event */
        RD_PREINIT,             /* the Initial rendezvous before .init */
        RD_POSTINIT,            /* the Second rendezvous after .init */
        RD_DLACTIVITY           /* a dlopen or dlclose has happened */
} rd_event_e;

# endif

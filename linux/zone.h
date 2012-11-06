# if !defined(_ZONE_H)
# define _ZONE_H
/*
 * The zone support infrastructure uses the zone name as a component
 * of unix domain (AF_UNIX) sockets, which are limited to 108 characters
 * in length, so ZONENAME_MAX is limited by that.
 */
#define ZONENAME_MAX            64
#define ZONE_ATTR_ROOT          1

typedef struct {
	int zone_id;
	} zone_t;
typedef int zoneid_t;

# endif

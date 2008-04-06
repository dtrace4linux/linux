/*
 * The zone support infrastructure uses the zone name as a component
 * of unix domain (AF_UNIX) sockets, which are limited to 108 characters
 * in length, so ZONENAME_MAX is limited by that.
 */
#define ZONENAME_MAX            64

/*
 * XX64 Do we need to support this for 64-bit apps?
 *
 * request structure passed by user
 */
struct ssd {
        unsigned int    sel;   /* descriptor selector */
        unsigned int    bo;    /* segment base or gate offset */
        unsigned int    ls;    /* segment limit or gate selector */
        unsigned int    acc1;  /* access byte 5 */
        unsigned int    acc2;  /* access bits in byte 6 or gate count */
};

/*
 * 32-bit Solaris device major/minor sizes.
 */
#define NBITSMAJOR32    14
#define NBITSMINOR32    18
#define MAXMAJ32        0x3ffful        /* SVR4 max major value */
#define MAXMIN32        0x3fffful       /* SVR4 max minor value */

#ifdef _LP64

#define NBITSMAJOR64    32      /* # of major device bits in 64-bit Solaris */
#define NBITSMINOR64    32      /* # of minor device bits in 64-bit Solaris */
#define MAXMAJ64        0xfffffffful    /* max major value */
#define MAXMIN64        0xfffffffful    /* max minor value */

#define NBITSMAJOR      NBITSMAJOR64
#define NBITSMINOR      NBITSMINOR64
#define MAXMAJ          MAXMAJ64
#define MAXMIN          MAXMIN64

#else /* !_LP64 */

#define NBITSMAJOR      NBITSMAJOR32
#define NBITSMINOR      NBITSMINOR32
#define MAXMAJ          MAXMAJ32
#define MAXMIN          MAXMIN32

#endif /* !_LP64 */

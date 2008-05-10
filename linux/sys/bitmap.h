# if !defined(SYS_BITMAP_H)
# define SYS_BITMAP_H

#ifdef _LP64
#define BT_ULSHIFT      6 /* log base 2 of BT_NBIPUL, to extract word index */
#define BT_ULSHIFT32    5 /* log base 2 of BT_NBIPUL, to extract word index */
#else
#define BT_ULSHIFT      5 /* log base 2 of BT_NBIPUL, to extract word index */
#endif

#define BT_NBIPUL       (1 << BT_ULSHIFT)       /* n bits per ulong_t */
#define BT_ULMASK       (BT_NBIPUL - 1)         /* to extract bit index */

#ifdef _LP64
#define BT_NBIPUL32     (1 << BT_ULSHIFT32)     /* n bits per ulong_t */
#define BT_ULMASK32     (BT_NBIPUL32 - 1)       /* to extract bit index */
#define BT_ULMAXMASK    0xffffffffffffffff      /* used by bt_getlowbit */
#else
#define BT_ULMAXMASK    0xffffffff
#endif

/*
 * These are public macros
 *
 * BT_BITOUL == n bits to n ulong_t's
 */
/*
 * word in map
 */
#define BT_WIM(bitmap, bitindex) \
        ((bitmap)[(bitindex) >> BT_ULSHIFT])
/*
 * bit in word
 */
#define BT_BIW(bitindex) \
        (1UL << ((bitindex) & BT_ULMASK))
	
#define BT_BITOUL(nbits) \
        (((nbits) + BT_NBIPUL - 1l) / BT_NBIPUL)
#define BT_SIZEOFMAP(nbits) \
        (BT_BITOUL(nbits) * sizeof (ulong_t))
#define BT_TEST(bitmap, bitindex) \
        ((BT_WIM((bitmap), (bitindex)) & BT_BIW(bitindex)) ? 1 : 0)
#define BT_SET(bitmap, bitindex) \
        { BT_WIM((bitmap), (bitindex)) |= BT_BIW(bitindex); }
#define BT_CLEAR(bitmap, bitindex) \
        { BT_WIM((bitmap), (bitindex)) &= ~BT_BIW(bitindex); }

# endif

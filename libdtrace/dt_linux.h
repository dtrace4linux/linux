#if !defined(dt_linux_h_INCLUDE)
#define	dt_linux_h_INCLUDE

#if defined(_LP64)
#  define	BITS_PER_LONG	64
#  define	BITS_LOG2_LONG	6
#else
#  define	BITS_PER_LONG	32
#  define	BITS_LOG2_LONG	5
#endif
#define	BT_ULMASK	((1 << BITS_LOG2_LONG) - 1)
#define BT_ULSHIFT	BITS_LOG2_LONG

#define	BT_TEST(bitmap, bitindex) \
	((bitmap)[(bitindex) >> BITS_LOG2_LONG] & (1 << ((bitindex) & (BITS_PER_LONG-1))) ? 1 : 0)

#define strdupa(str) ({int len = strlen(str) + 1; \
		char *cp = alloca(len); \
		strcpy(cp, str); \
		cp; \
		})

#endif /* !defined(dt_linux_h_INCLUDE) */

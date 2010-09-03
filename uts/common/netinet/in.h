/**********************************************************************/
/*   $Header: Last edited: 31-Aug-2010 1.1 $ 			      */
/**********************************************************************/
# if !defined(SYS_IN_H)
# define SYS_IN_H

#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define AF_INET6        26              /* Internet Protocol, Version 6 */

#if 0
#define	INET_ADDRSTRLEN		16	/* max len IPv4 addr in ascii dotted */
					/* decimal notation. */
#define	INET6_ADDRSTRLEN	46	/* max len of IPv6 addr in ascii */
					/* standard colon-hex notation. */
#endif

struct in6_addr {
        union {
                /*
                 * Note: Static initalizers of "union" type assume
                 * the constant on the RHS is the type of the first member
                 * of union.
                 * To make static initializers (and efficient usage) work,
                 * the order of members exposed to user and kernel view of
                 * this data structure is different.
                 * User environment sees specified uint8_t type as first
                 * member whereas kernel sees most efficient type as
                 * first member.
                 */
#ifdef _KERNEL
                uint32_t        _S6_u32[4];     /* IPv6 address */
                uint8_t         _S6_u8[16];     /* IPv6 address */
#else
                uint8_t         _S6_u8[16];     /* IPv6 address */
                uint32_t        _S6_u32[4];     /* IPv6 address */
#endif
                uint32_t        __S6_align;     /* Align on 32 bit boundary */
        } _S6_un;
};
#define s6_addr         _S6_un._S6_u8

#ifdef _KERNEL
#define s6_addr8        _S6_un._S6_u8
#define s6_addr32       _S6_un._S6_u32
#endif

typedef struct in6_addr in6_addr_t;

/* Exclude loopback and unspecified address */
#ifdef _BIG_ENDIAN
#define	IN6_IS_ADDR_V4COMPAT(addr) \
	(((addr)->_S6_un._S6_u32[2] == 0) && \
	((addr)->_S6_un._S6_u32[1] == 0) && \
	((addr)->_S6_un._S6_u32[0] == 0) && \
	!((addr)->_S6_un._S6_u32[3] == 0) && \
	!((addr)->_S6_un._S6_u32[3] == 0x00000001))

#else /* _BIG_ENDIAN */
#define	IN6_IS_ADDR_V4COMPAT(addr) \
	(((addr)->_S6_un._S6_u32[2] == 0) && \
	((addr)->_S6_un._S6_u32[1] == 0) && \
	((addr)->_S6_un._S6_u32[0] == 0) && \
	!((addr)->_S6_un._S6_u32[3] == 0) && \
	!((addr)->_S6_un._S6_u32[3] == 0x01000000))
#endif /* _BIG_ENDIAN */


#ifdef _BIG_ENDIAN
#define	IN6_IS_ADDR_V4MAPPED(addr) \
	(((addr)->_S6_un._S6_u32[2] == 0x0000ffff) && \
	((addr)->_S6_un._S6_u32[1] == 0) && \
	((addr)->_S6_un._S6_u32[0] == 0))
#else  /* _BIG_ENDIAN */
#define	IN6_IS_ADDR_V4MAPPED(addr) \
	(((addr)->_S6_un._S6_u32[2] == 0xffff0000U) && \
	((addr)->_S6_un._S6_u32[1] == 0) && \
	((addr)->_S6_un._S6_u32[0] == 0))
#endif /* _BIG_ENDIAN */

# endif

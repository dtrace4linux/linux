/*
 * Definitions for corectl() system call.
 */

/* subcodes */
#define CC_SET_OPTIONS          1
#define CC_GET_OPTIONS          2
#define CC_SET_GLOBAL_PATH      3
#define CC_GET_GLOBAL_PATH      4
#define CC_SET_PROCESS_PATH     5
#define CC_GET_PROCESS_PATH     6
#define CC_SET_GLOBAL_CONTENT   7
#define CC_GET_GLOBAL_CONTENT   8
#define CC_SET_PROCESS_CONTENT  9
#define CC_GET_PROCESS_CONTENT  10
#define CC_SET_DEFAULT_PATH     11
#define CC_GET_DEFAULT_PATH     12
#define CC_SET_DEFAULT_CONTENT  13
#define CC_GET_DEFAULT_CONTENT  14

/* options */
#define CC_GLOBAL_PATH          0x01    /* enable global core files */
#define CC_PROCESS_PATH         0x02    /* enable per-process core files */
#define CC_GLOBAL_SETID         0x04    /* allow global setid core files */
#define CC_PROCESS_SETID        0x08    /* allow per-process setid core files */
#define CC_GLOBAL_LOG           0x10    /* log global core dumps to syslog */

/* all of the above */
#define CC_OPTIONS      \
        (CC_GLOBAL_PATH | CC_PROCESS_PATH | \
        CC_GLOBAL_SETID | CC_PROCESS_SETID | CC_GLOBAL_LOG)

/* contents */
#define CC_CONTENT_STACK        0x0001ULL /* process stack */
#define CC_CONTENT_HEAP         0x0002ULL /* process heap */

/* MAP_SHARED file mappings */
#define CC_CONTENT_SHFILE       0x0004ULL /* file-backed shared mapping */
#define CC_CONTENT_SHANON       0x0008ULL /* anonymous shared mapping */

/* MAP_PRIVATE file mappings */
#define CC_CONTENT_TEXT         0x0010ULL /* read/exec file mappings */
#define CC_CONTENT_DATA         0x0020ULL /* writable file mappings */
#define CC_CONTENT_RODATA       0x0040ULL /* read-only file mappings */
#define CC_CONTENT_ANON         0x0080ULL /* anonymous mappings (MAP_ANON) */

#define CC_CONTENT_SHM          0x0100ULL /* System V shared memory */
#define CC_CONTENT_ISM          0x0200ULL /* intimate shared memory */
#define CC_CONTENT_DISM         0x0400ULL /* dynamic intimate shared memory */

#define CC_CONTENT_CTF          0x0800ULL /* CTF data */
#define CC_CONTENT_SYMTAB       0x1000ULL /* symbol table */

#define CC_CONTENT_ALL          0x1fffULL
#define CC_CONTENT_NONE         0ULL
#define CC_CONTENT_DEFAULT      (CC_CONTENT_STACK | CC_CONTENT_HEAP | \
        CC_CONTENT_ISM | CC_CONTENT_DISM | CC_CONTENT_SHM | \
        CC_CONTENT_SHANON | CC_CONTENT_TEXT | CC_CONTENT_DATA | \
        CC_CONTENT_RODATA | CC_CONTENT_ANON | CC_CONTENT_CTF)
#define CC_CONTENT_INVALID      (-1ULL)

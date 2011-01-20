/**********************************************************************/
/*   Wrapper  around  sys/types.h to get the right stuff defined for  */
/*   user and kernel land.					      */
/**********************************************************************/
# if !defined(SYS_STAT_H)
# define	SYS_STAT_H

# include	<linux_types.h>
# include	"/usr/include/sys/stat.h"

#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif
#define _ST_FSTYPSZ 16          /* array size for file system type name */

struct stat64_32 {
        dev32_t         st_dev;
        int32_t         st_pad1[3];
        ino64_t         st_ino;
        mode32_t        st_mode;
        nlink32_t       st_nlink;
        uid32_t         st_uid;
        gid32_t         st_gid;
        dev32_t         st_rdev;
        int32_t         st_pad2[2];
        off64_t         st_size;
        timestruc32_t   st_atim;
        timestruc32_t   st_mtim;
        timestruc32_t   st_ctim;
        int32_t         st_blksize;
        blkcnt64_t      st_blocks;
        char            st_fstype[_ST_FSTYPSZ];
        int32_t         st_pad4[8];
};

#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

struct stat64;

# endif

/**********************************************************************/
/*   Licensed  according to the current DTrace license. This file is  */
/*   Linux specific and is a mapping from kernel to D script.	      */
/*   $Header: Last edited: 30-Aug-2010 1.1 $ 			      */
/**********************************************************************/

/*#pragma D depends_on module unix*/
/*#pragma D depends_on provider io*/

inline int B_BUSY = 0x0000;
#pragma D binding "1.0" B_BUSY
inline int B_DONE = 0x0001;
#pragma D binding "1.0" B_DONE
inline int B_ERROR = 0x0002;
#pragma D binding "1.0" B_ERROR
inline int B_PAGEIO = 0x0004;
#pragma D binding "1.0" B_PAGEIO
inline int B_PHYS = 0x0008;
#pragma D binding "1.0" B_PHYS
inline int B_READ = 0x0010;
#pragma D binding "1.0" B_READ
inline int B_WRITE = 0x0020;
#pragma D binding "1.0" B_WRITE
inline int B_ASYNC = 0x0040;
#pragma D binding "1.0" B_ASYNC

/*
 * The following inline constants can be used to examine fi_oflags when using
 * the fds[] array or a translated fileinfo_t.  Note that the various open
 * flags behave as a bit-field *except* for O_RDONLY, O_WRONLY, and O_RDWR.
 * To test the open mode, you write code similar to that used with the fcntl(2)
 * F_GET[X]FL command, such as: if ((fi_oflags & O_ACCMODE) == O_WRONLY).
 */
inline int O_ACCMODE = 0x0003;
#pragma D binding "1.1" O_ACCMODE

inline int O_RDONLY = 0x0000;
#pragma D binding "1.1" O_RDONLY
inline int O_WRONLY = 0x0001;
#pragma D binding "1.1" O_WRONLY
inline int O_RDWR = 0x0002;
#pragma D binding "1.1" O_RDWR

inline int O_APPEND = 0x0400;
#pragma D binding "1.1" O_APPEND
inline int O_EXCL = 0x0080;
#pragma D binding "1.1" O_EXCL
inline int O_CREAT = 0x0040;
#pragma D binding "1.1" O_CREAT
inline int O_LARGEFILE = 0x8000;
#pragma D binding "1.1" O_LARGEFILE
inline int O_NOCTTY = 0x0100;
#pragma D binding "1.1" O_NOCTTY
inline int O_NONBLOCK = 0x800;
#pragma D binding "1.1" O_NONBLOCK
inline int O_NDELAY = 0x800;
#pragma D binding "1.1" O_NDELAY
inline int O_SYNC = 0x1000;
#pragma D binding "1.1" O_SYNC
inline int O_TRUNC = 0x0200;
#pragma D binding "1.1" O_TRUNC

/**********************************************************************/
/*   The  following  maps  from  the  kernel view of the io provider  */
/*   structures,  to the D view. Where there is a conflict, we use a  */
/*   k_  prefix  to  denote  the  kernel  side.  (See  the  code  in  */
/*   sdt_linux.c to see what/why/where this happens).		      */
/*   								      */
/*   In   Solaris/MacOS,   the   kernel   contains   CTF   structure  */
/*   definitions,  so  you  wont  see the kernel side definitions in  */
/*   their  file. But we dont control the way a kernel is built, and  */
/*   have  to  annotate  here. At a later date, we may provide a CTF  */
/*   compiler  to  avoid D scripts having to parse these definitions  */
/*   at run time.						      */
/**********************************************************************/

typedef char *caddr_t;
typedef int dev_t;
typedef struct bufinfo {
	int b_flags;			/* buffer status */
	size_t b_bcount;		/* number of bytes */
	caddr_t b_addr;			/* buffer address */
	uint64_t b_lblkno;		/* block # on device */
	uint64_t b_blkno;		/* expanded block # on device */
	size_t b_resid;			/* # of bytes not transferred */
	size_t b_bufsize;		/* size of allocated buffer */
	caddr_t b_iodone;		/* I/O completion routine */
	int b_error;			/* expanded error field */
	dev_t b_edev;			/* extended device */
} bufinfo_t;

typedef struct devinfo {                                                      
	int dev_major;                  /* major number */                    
	int dev_minor;                  /* minor number */                    
	int dev_instance;               /* instance number */                 
	char *dev_name;                /* name of device */                   
	char *dev_statname;            /* name of device + instance/minor */  
	char *dev_pathname;            /* pathname of device */               
} devinfo_t;

typedef struct k_fileinfo {
	char *fi_name;
	char *fi_dirname;
	char *fi_pathname;
	long long fi_offset;		/* offset within file */
	char *fi_fs;
	char *fi_mount;
} k_fileinfo_t;

typedef struct fileinfo {
	string fi_name;
	string fi_dirname;
	string fi_pathname;
	long long fi_offset;		/* offset within file */
	string fi_fs;
	string fi_mount;
} fileinfo_t;

typedef struct buf {
	bufinfo_t b;
	devinfo_t d;
	k_fileinfo_t f;
        } buf_t;

#pragma D binding "1.0" translator
translator fileinfo_t < buf_t *B > {
	fi_name	   = stringof(B->f.fi_name);
	fi_dirname = stringof(B->f.fi_dirname);
	fi_pathname= stringof(B->f.fi_pathname);
	fi_fs 	   = stringof(B->f.fi_fs);
	fi_mount   = stringof(B->f.fi_mount);
	fi_offset  = B->f.fi_offset;
};

/*
inline fileinfo_t fds[int fd] = xlate <fileinfo_t> (
    fd >= 0 && fd < curthread->t_procp->p_user.u_finfo.fi_nfiles ?
    curthread->t_procp->p_user.u_finfo.fi_list[fd].uf_file : NULL);

#pragma D attributes Stable/Stable/Common fds
#pragma D binding "1.1" fds
*/


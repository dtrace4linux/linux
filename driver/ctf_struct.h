
/**********************************************************************/
/*   Following  structures  are  the public interface for io:::start  */
/*   and io:::done.						      */
/**********************************************************************/
typedef struct bufinfo_t {
        int b_flags;                    /* flags */
        size_t b_bcount;                /* number of bytes */
        caddr_t b_addr;                 /* buffer address */
        uint64_t b_blkno;               /* expanded block # on device */
        uint64_t b_lblkno;              /* block # on device */
        size_t b_resid;                 /* # of bytes not transferred */
        size_t b_bufsize;               /* size of allocated buffer */ 
        caddr_t b_iodone;               /* I/O completion routine */
        int b_error;                    /* expanded error field */
        dev_t b_edev;                   /* extended device */
	} bufinfo_t;

typedef struct devinfo {
        int dev_major;                  /* major number */
        int dev_minor;                  /* minor number */
        int dev_instance;               /* instance number */
        char *dev_name;                /* name of device */
        char *dev_statname;            /* name of device + instance/minor */
        char *dev_pathname;            /* pathname of device */
} devinfo_t;

typedef struct fileinfo_t {
	char	*fi_name;
	char	*fi_dirname;
	char	*fi_pathname;
	long long fi_offset;
	const char *fi_fs;
	char *fi_mount;
} fileinfo_t;

typedef struct public_buf_t {
	bufinfo_t	b;
	devinfo_t	d;
	fileinfo_t	f;
	} public_buf_t;



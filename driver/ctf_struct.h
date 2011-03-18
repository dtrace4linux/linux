
/**********************************************************************/
/*   Following  structures  are  the public interface for io:::start  */
/*   and io:::done.						      */
/**********************************************************************/
typedef struct kbufinfo {
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
	} kbufinfo_t;

typedef struct kdevinfo {
	int dev_major;                  /* major number */
        int dev_minor;                  /* minor number */
        int dev_instance;               /* instance number */
        char *dev_name;                /* name of device */
        char *dev_statname;            /* name of device + instance/minor */
        char *dev_pathname;            /* pathname of device */
} kdevinfo_t;

typedef struct kfileinfo {
	char	*fi_name;
	char	*fi_dirname;
	char	*fi_pathname;
	long long fi_offset;
	const char *fi_fs;
	char *fi_mount;
} kfileinfo_t;

typedef struct buf {
	struct kbufinfo		b;
	struct kdevinfo		d;
	struct kfileinfo	f;
	} buf_t;

/**********************************************************************/
/*   For procfs support.					      */
/**********************************************************************/
typedef struct psinfo_t {
	int	pr_nlwp;
	int	pr_pid;
	int	pr_ppid;
	int	pr_pgid;
	char	*pr_psargs;
	} psinfo_t;
typedef struct thread_t {
	int	pr_projid;
	int	xxx;
	} thread_t;

typedef struct dtrace_cpu_t {
	int	cpu_id;
	} dtrace_cpu_t;

/**********************************************************************/
/*   This  file  is  used  to  define  kernel  structures  so we can  */
/*   generate /usr/lib/dtrace/linux-`uname -r`.ctf. We dont use this  */
/*   in  the kernel driver, its just a vehicle for translating DWARF  */
/*   debug symbols to ctf format.				      */
/*   								      */
/*   In general, just stack up the #includes - the more the merrier.  */
/*   This  should  allow  lots of structures to be made available to  */
/*   the user.							      */
/*   								      */
/*   Without  this,  SDT  probes  wont  know  how to map the args[n]  */
/*   arguments.							      */
/*   								      */
/*   $Header:$							      */
/**********************************************************************/

#include <linux/swap.h> /* want totalram_pages */
#include <linux/delay.h>
#include <linux/slab.h>
#include <netinet/in.h>
#include <linux/cpumask.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sys.h>
#include <linux/thread_info.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/namei.h>
#include <linux/audit.h>
#include <linux/poll.h>
#include <asm/tlbflush.h>
#include <asm/current.h>
#include <asm/desc.h>

/**********************************************************************/
/*   Following  structures  are  the public interface for io:::start  */
/*   and io:::done.						      */
/**********************************************************************/
typedef struct bufinfo_t {
        int b_flags;                    /* flags */
        int b_bcount;                /* number of bytes */
        int b_addr;                 /* buffer address */
        int b_blkno;               /* expanded block # on device */
        int b_lblkno;              /* block # on device */
        int b_resid;                 /* # of bytes not transferred */
        int b_bufsize;               /* size of allocated buffer */ 
        int b_iodone;               /* I/O completion routine */
        int b_error;                    /* expanded error field */
        int b_edev;                   /* extended device */
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
	char *fi_fs;
	char *fi_mount;
} fileinfo_t;

typedef struct public_buf_t {
	bufinfo_t	b;
	devinfo_t	d;
	fileinfo_t	f;
	} public_buf_t;


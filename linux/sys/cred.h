# if !defined(SYS_CRED_H)
# define SYS_CRED_H

/**********************************************************************/
/*   Linux  2.6.29 introduces 2.6.29, so we need to be careful since  */
/*   the  struct  isnt compatible with the Solaris one. We need this  */
/*   as  a container for the identity of a process, so we can simply  */
/*   map  from ours to theirs where we can, else define our own cred  */
/*   struct for older kernels.					      */
/**********************************************************************/
# define cr_ruid	euid
# define cr_rgid	egid
# define cr_uid		uid
# define cr_gid		gid
# define cr_sgid	sgid
# define cr_suid	suid

/**********************************************************************/
/*   Linux  2.6.29  introduces <linux/cred.h> so we will have a name  */
/*   clash and struct mem clash.				      */
/**********************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
struct cred {
	int	cr_uid;
	int	cr_ruid;
	int	cr_suid;
	int	cr_gid;
	int	cr_rgid;
	int	cr_sgid;
	struct zone *cr_zone;
	};
# endif

typedef struct cred cred_t;

cred_t *CRED(void);
int priv_policy_only(const cred_t *a, int priv, int allzone);
#define crgetzoneid(p) 0
#define crfree(cred)
#define crgetgid(cred)  ((cred)->cr_gid)
#define crgetuid(cred)  ((cred)->cr_uid)

//# define	curthread current_thread_info()->task

# endif

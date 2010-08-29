# include <stdio.h>

/**********************************************************************/
/*   Stubs which we have yet to implement. Just log we got called so  */
/*   we can make porting progress and know what we forgot to do.      */
/**********************************************************************/

int	_libproc_debug;		/* set non-zero to enable debugging printfs */
int	blockable_sigs;	/* signals to block when we need to be safe */

getzonenamebyid() { printf("%s\n", __func__); }

int
OBJFS_MODID(int x) 
{
	return x;
}
__priv_free_info() { printf("proc-stub:%s\n", __func__); }
_rw_read_held() { printf("proc-stub:%s\n", __func__); }
_rw_write_held() { printf("proc-stub:%s\n", __func__); }
//dt_pid_create_entry_probe() { printf("proc-stub:%s\n", __func__); }
//dt_pid_create_glob_offset_probes() { printf("proc-stub:%s\n", __func__); }
//dt_pid_create_offset_probe() { printf("proc-stub:%s\n", __func__); }
//dt_pid_create_return_probe() { printf("proc-stub:%s\n", __func__); }

int modctl() { printf("proc-stub:%s\n", __func__); return -1; }
int p_online(int processorid, int flag) 
{
//	printf("proc-stub:%s: processorid=%d flag=%d\n", __func__, processorid, flag); 
	return 2; // P_ONLINE
}


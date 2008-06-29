
/**********************************************************************/
/*   Stubs which we have yet to implement. Just log we got called so  */
/*   we can make porting progress and know what we forgot to do.      */
/**********************************************************************/

int	_libproc_debug;		/* set non-zero to enable debugging printfs */
int	blockable_sigs;	/* signals to block when we need to be safe */

Pcreate() { printf("proc-stub:%s\n", __func__); }
Pcreate_error() { printf("proc-stub:%s\n", __func__); }
Pctlfd() { printf("proc-stub:%s\n", __func__); }
Pdelbkpt() { printf("proc-stub:%s\n", __func__); }
Pfault() { printf("proc-stub:%s\n", __func__); }
Pgrab() { printf("proc-stub:%s\n", __func__); }
Pgrab_error() { printf("proc-stub:%s\n", __func__); }
Plmid() { printf("proc-stub:%s\n", __func__); }
Plmid_to_map() { printf("proc-stub:%s\n", __func__); }
Plookup_by_addr() { printf("proc-stub:%s\n", __func__); }
Pname_to_map() { printf("proc-stub:%s\n", __func__); }
Pobject_iter() { printf("proc-stub:%s\n", __func__); }
Pobjname() { printf("proc-stub:%s\n", __func__); }
Ppltdest() { printf("proc-stub:%s\n", __func__); }
Ppsinfo() { printf("proc-stub:%s\n", __func__); }
Prd_agent() { printf("proc-stub:%s\n", __func__); }
Pread() { printf("proc-stub:%s\n", __func__); }
Prelease() { printf("proc-stub:%s\n", __func__); }
Preopen() { printf("proc-stub:%s\n", __func__); }
Preset_maps() { printf("proc-stub:%s\n", __func__); }
Psetbkpt() { printf("proc-stub:%s\n", __func__); }
Psetflags() { printf("proc-stub:%s\n", __func__); }
Psetrun() { printf("proc-stub:%s\n", __func__); }
Pstate() { printf("proc-stub:%s\n", __func__); }
Pstatus() { printf("proc-stub:%s\n", __func__); }
Pstopstatus() { printf("proc-stub:%s\n", __func__); }
OBJFS_MODID(int x) 
{
	printf("proc-stub:%s\n", __func__);
	return 0;
}
Psymbol_iter_by_addr() { printf("proc-stub:%s\n", __func__); }
Psync() { printf("proc-stub:%s\n", __func__); }
Psysentry() { printf("proc-stub:%s\n", __func__); }
Psysexit() { printf("proc-stub:%s\n", __func__); }
Punsetflags() { printf("proc-stub:%s\n", __func__); }
Pupdate_maps() { printf("proc-stub:%s\n", __func__); }
Pupdate_syms() { printf("proc-stub:%s\n", __func__); }
Pxecbkpt() { printf("proc-stub:%s\n", __func__); }
Pxlookup_by_name() { printf("proc-stub:%s\n", __func__); }
_mutex_held() { printf("proc-stub:%s\n", __func__); }
_rw_read_held() { printf("proc-stub:%s\n", __func__); }
_rw_write_held() { printf("proc-stub:%s\n", __func__); }
dt_pid_create_entry_probe() { printf("proc-stub:%s\n", __func__); }
dt_pid_create_glob_offset_probes() { printf("proc-stub:%s\n", __func__); }
dt_pid_create_offset_probe() { printf("proc-stub:%s\n", __func__); }
dt_pid_create_return_probe() { printf("proc-stub:%s\n", __func__); }
fork1() { printf("proc-stub:%s\n", __func__); }

int modctl() { printf("proc-stub:%s\n", __func__); return -1; }
int p_online(int processorid, int flag) 
{
//	printf("proc-stub:%s: processorid=%d flag=%d\n", __func__, processorid, flag); 
	return 2; // P_ONLINE
}
pr_close() { printf("proc-stub:%s\n", __func__); }
pr_ioctl() { printf("proc-stub:%s\n", __func__); }
pr_open() { printf("proc-stub:%s\n", __func__); }
rd_errstr() { printf("proc-stub:%s\n", __func__); }
rd_event_addr() { printf("proc-stub:%s\n", __func__); }
rd_event_enable() { printf("proc-stub:%s\n", __func__); }
rd_event_getmsg() { printf("proc-stub:%s\n", __func__); }
rd_init() { printf("proc-stub:%s\n", __func__); }


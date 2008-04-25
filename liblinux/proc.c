int	_libproc_debug;		/* set non-zero to enable debugging printfs */
int	blockable_sigs;	/* signals to block when we need to be safe */

Pcreate() { printf("%s\n", __func__); }
Pcreate_error() { printf("%s\n", __func__); }
Pctlfd() { printf("%s\n", __func__); }
Pdelbkpt() { printf("%s\n", __func__); }
Pfault() { printf("%s\n", __func__); }
Pgrab() { printf("%s\n", __func__); }
Pgrab_error() { printf("%s\n", __func__); }
Plmid() { printf("%s\n", __func__); }
Plmid_to_map() { printf("%s\n", __func__); }
Plookup_by_addr() { printf("%s\n", __func__); }
Pname_to_map() { printf("%s\n", __func__); }
Pobject_iter() { printf("%s\n", __func__); }
Pobjname() { printf("%s\n", __func__); }
Ppltdest() { printf("%s\n", __func__); }
Ppsinfo() { printf("%s\n", __func__); }
Prd_agent() { printf("%s\n", __func__); }
Pread() { printf("%s\n", __func__); }
Prelease() { printf("%s\n", __func__); }
Preopen() { printf("%s\n", __func__); }
Preset_maps() { printf("%s\n", __func__); }
Psetbkpt() { printf("%s\n", __func__); }
Psetflags() { printf("%s\n", __func__); }
Psetrun() { printf("%s\n", __func__); }
Pstate() { printf("%s\n", __func__); }
Pstatus() { printf("%s\n", __func__); }
Pstopstatus() { printf("%s\n", __func__); }
OBJFS_MODID(int x) 
{
	printf("%s\n", __func__);
	return 0;
}
Psymbol_iter_by_addr() { printf("%s\n", __func__); }
Psync() { printf("%s\n", __func__); }
Psysentry() { printf("%s\n", __func__); }
Psysexit() { printf("%s\n", __func__); }
Punsetflags() { printf("%s\n", __func__); }
Pupdate_maps() { printf("%s\n", __func__); }
Pupdate_syms() { printf("%s\n", __func__); }
Pxecbkpt() { printf("%s\n", __func__); }
Pxlookup_by_name() { printf("%s\n", __func__); }
_mutex_held() { printf("%s\n", __func__); }
_rw_read_held() { printf("%s\n", __func__); }
_rw_write_held() { printf("%s\n", __func__); }
dt_pid_create_entry_probe() { printf("%s\n", __func__); }
dt_pid_create_glob_offset_probes() { printf("%s\n", __func__); }
dt_pid_create_offset_probe() { printf("%s\n", __func__); }
dt_pid_create_return_probe() { printf("%s\n", __func__); }
dt_pragma() { printf("%s\n", __func__); }
fork1() { printf("%s\n", __func__); }
gethrtime() { printf("%s\n", __func__); }
int
getzoneid() 
{
	return 0;
}
int modctl() { printf("%s\n", __func__); return -1; }
p_online() { printf("%s\n", __func__); }
pr_close() { printf("%s\n", __func__); }
pr_ioctl() { printf("%s\n", __func__); }
pr_open() { printf("%s\n", __func__); }
pthread_cond_reltimedwait_np() { printf("%s\n", __func__); }
rd_errstr() { printf("%s\n", __func__); }
rd_event_addr() { printf("%s\n", __func__); }
rd_event_enable() { printf("%s\n", __func__); }
rd_event_getmsg() { printf("%s\n", __func__); }
rd_init() { printf("%s\n", __func__); }


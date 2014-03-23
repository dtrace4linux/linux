#include <sys/wait.h>
#include <sys/lwp.h>
#include <strings.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <dt_proc.h>
#include <dt_pid.h>
#include <dt_impl.h>
# include <sys/ptrace.h>

typedef struct dt_proc_control_data {
	dtrace_hdl_t *dpcd_hdl;			/* DTrace handle */
	dt_proc_t *dpcd_proc;			/* proccess to control */
} dt_proc_control_data_t;

void *
find_r_debug(int pid)
{	char	buf[512];
	char	buf1[512];
	FILE	*fp, *fp1;
	int	ret;
	char	*r_debug = 0;

	printf("%s\n", __func__);
	snprintf(buf, sizeof buf, "/proc/%d/maps", pid);
	if ((fp = fopen(buf, "r")) == NULL) {
		return RD_ERR;
	}
	while (fgets(buf, sizeof buf, fp) != NULL) {
		char	*addr_str;
		char	*perms;
		char	*name;
		char	*size;
		char	*dev;
		char	*ino;

		if (strstr(buf, "[stack]") != NULL)
			break;

		buf[strlen(buf)-1] = '\0';
		addr_str = strtok(buf, " ");
		perms = strtok(NULL, " ");
		size = strtok(NULL, " ");
		dev = strtok(NULL, " ");
		ino = strtok(NULL, " ");

		r_debug = strtol(buf, NULL, 16) + 0x100;
//printf("%s\n", buf);
//printf("rd_loadobj_iter: %s %p %s\n", lib, addr, buf);
	}
	fclose(fp);
	return r_debug;

	while (fgets(buf, sizeof buf, fp) != NULL) {
		char	*addr_str;
		long	addr;
		long	addr1;
		char	*perms;
		char	*lib;
		char	*name;
		char	*size;
		char	*dev;
		char	*ino;

		if (strcmp(buf + strlen(buf) - 4, ".so\n") != 0)
			continue;
		buf[strlen(buf)-1] = '\0';
		lib = strrchr(buf, ' ');
		if (lib == NULL)
			continue;
		lib++;
		addr_str = strtok(buf, " ");
		perms = strtok(NULL, " ");
		size = strtok(NULL, " ");
		dev = strtok(NULL, " ");
		ino = strtok(NULL, " ");
		if (perms == NULL || strchr(perms, 'x') == NULL)
			continue;

		if ((name = strtok(NULL, " ")) == NULL)
			continue;

		if (strncmp(name, "/lib/ld-", 8) != 0 &&
		    strncmp(name, "/lib64/ld-", 10) != 0)
		    	continue;

		addr = strtol(addr_str, NULL, 16);

		snprintf(buf1, sizeof buf1, "readelf -s %s| grep r_debug | awk '{print $2}'", name);
		if ((fp1 = popen(buf1, "r")) == NULL)
			continue;
		fgets(buf, sizeof buf, fp1);
		pclose(fp1);
		addr1 = strtol(buf, NULL, 16);

		r_debug = addr + addr1;
		break;

//printf("rd_loadobj_iter: %s %p %s\n", lib, addr, buf);
	}
	fclose(fp);

	return r_debug;
}

void pp(int pid, long addr)
{	int	i;

	for (i = 0; i < 80; i++) {
		long	v = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
		long	v1 = ptrace(PTRACE_PEEKTEXT, pid, addr+8, 0);
		long	v2 = ptrace(PTRACE_PEEKTEXT, pid, addr+16, 0);
		long	v3 = ptrace(PTRACE_PEEKTEXT, pid, addr+24, 0);
		printf("%p: %016lx %016lx %016lx %016lx\n", addr, v, v1, v2, v3);
		addr += 32;
		}
}
void *
dt_proc_control_linux(void *arg)
{
	int	status;
	int	ret;
	dt_proc_control_data_t *datap = arg;
	struct r_debug *r_debug;
	void	*r_brk;
	long	val;
	dt_proc_t *dpr = datap->dpcd_proc;
	dtrace_hdl_t *dtp = datap->dpcd_hdl;

	do_ptrace(__func__, PTRACE_ATTACH, dpr->dpr_pid, 0, 0);
	ret = waitpid(dpr->dpr_pid, &status, 0);
printf("waitpid %d return st=%x\n", dpr->dpr_pid, status);
	/***********************************************/
	/*   Find _r_debug in /lib/ld.so.	       */
	/***********************************************/
	r_debug = find_r_debug(dpr->dpr_pid);
	r_brk = (void *) ((long) r_debug +
			((char *) &r_debug->r_brk - (char *) r_debug));
printf("r_debug=%p\n", r_brk);

	/***********************************************/
	/*   Single  step  process  til  we  get  the  */
	/*   r_debug area.			       */
	/***********************************************/
printf("[step] stepping til we get _r_debug\n");
	while (1) {
		ret = do_ptrace(__func__, PTRACE_SINGLESTEP, dpr->dpr_pid, 0, 0);
		if (ret < 0) {
			perror("ptrace");
			break;
		}

		ret = waitpid(dpr->dpr_pid, &status, 0);
		val = ptrace(PTRACE_PEEKTEXT, dpr->dpr_pid, (long) r_brk, 0, 0);
		if (val) {
			r_brk = val;
			break;
		}
	}
printf("r_brk=%p\n", r_brk);
//sleep(5);
//pp(dpr->dpr_pid, val);

	/***********************************************/
	/*   Put a breakpoint at the r_brk function -  */
	/*   this is a no-op, but tells us that a new  */
	/*   shlib got loaded.			       */
	/***********************************************/
	int instr = do_ptrace(__func__, PTRACE_PEEKTEXT, dpr->dpr_pid, r_brk, 0);
printf("instr=%x\n", instr);
	instr = 0xcc000000 | (instr & 0xffffff);
printf("instr=%x\n", instr);
        instr = 0xc3cccccc;
	do_ptrace(__func__, PTRACE_POKEDATA, dpr->dpr_pid, r_brk, instr);
char buf[256];
sprintf(buf, "cat /proc/%d/maps", dpr->dpr_pid);
system(buf);
Pupdate_syms(dpr->dpr_proc);
if (dt_pid_create_probes_module(dtp, dpr) != 0)
	dt_proc_notify(dtp, dtp->dt_procs, dpr,
	    dpr->dpr_errmsg);
printf("\n\n");

//dpr->dpr_stop |= DT_PROC_STOP_IDLE;
////dpr->dpr_done = B_TRUE;
//(void) pthread_cond_broadcast(&dpr->dpr_cv);
//while(1) sleep(1);
//(void) pthread_mutex_unlock(&dpr->dpr_lock);
	/***********************************************/
	/*   Let process run.			       */
	/***********************************************/
	while (!dpr->dpr_quit) {
		ret = do_ptrace(__func__, PTRACE_CONT, dpr->dpr_pid, 0, 0);
		if (ret < 0) {
			perror("ptrace");
			break;
		}

		ret = waitpid(dpr->dpr_pid, &status, 0);
printf("waitpid %d return ret=%d st=%x\n", dpr->dpr_pid, ret, status);
		if (WIFEXITED(status)) {
			printf("exited.....\n");
			break;
		}
		if (status & 0xff00) {
			int sig = WSTOPSIG(status);
			printf("signal %s %d\n", strsignal(WSTOPSIG(status)), sig);
			if (sig == SIGTRAP) {
//dpr->dpr_stop |= DT_PROC_STOP_IDLE;
//dpr->dpr_done = B_TRUE;
//(void) pthread_cond_broadcast(&dpr->dpr_cv);
				system(buf);
				Pupdate_syms(dpr->dpr_proc);
				if (dt_pid_create_probes_module(dtp, dpr) != 0)
					dt_proc_notify(dtp, dtp->dt_procs, dpr,
					    dpr->dpr_errmsg);
			}
		}
		if (WIFSTOPPED(status)) {
			printf("stopped....\n");
		}
	}

	/***********************************************/
	/*   Its all over...			       */
	/***********************************************/
	(void) pthread_mutex_lock(&dpr->dpr_lock);

//	dt_proc_bpdestroy(dpr, B_TRUE);
	dpr->dpr_done = B_TRUE;
	dpr->dpr_tid = 0;

	(void) pthread_cond_broadcast(&dpr->dpr_cv);
	(void) pthread_mutex_unlock(&dpr->dpr_lock);
}

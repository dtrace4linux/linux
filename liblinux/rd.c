#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <rtld_db.h>

struct ps_prochandle;

#include "Pcontrol.h"
#include "Putil.h"

/*
 * Mutex to protect global data
 */
mutex_t	glob_mutex = DEFAULTMUTEX;
int	rtld_db_version = RD_VERSION1;
int	rtld_db_logging = 0;
char	rtld_db_helper_path[MAXPATHLEN];

rd_err_e
rd_init(int version)
{
	if ((version < RD_VERSION1) ||
	    (version > RD_VERSION))
		return (RD_NOCAPAB);
	rtld_db_version = version;
//	LOG(ps_plog(MSG_ORIG(MSG_DB_RDINIT), rtld_db_version));

	return (RD_OK);
}
void
rd_delete(rd_agent_t *rap)
{
	printf("rd_delete:%s\n", __func__);
	free(rap);
}
rd_agent_t *
rd_new(struct ps_prochandle *php) 
{	rd_agent_t *rap;

return NULL;
	rap = (rd_agent_t *) calloc(sizeof *rap, 1);

	if (rap == NULL)
		return NULL;

	rap->rd_psp = php;
	(void) mutex_init(&rap->rd_mutex, USYNC_THREAD, 0);
	printf("rd_new:%s\n", __func__); 
	return rap;
}
rd_err_e
rd_loadobj_iter(rd_agent_t *rap, rl_iter_f *cb, void *client_data)
{
	printf("%s\n", __func__);
}
rd_err_e
rd_get_dyns(rd_agent_t *rap, psaddr_t addr, void **dynpp, size_t *dynpp_sz)
{
	printf("%s\n", __func__);
	return RD_ERR;
}
void
rd_log(const int on_off)
{
	(void) mutex_lock(&glob_mutex);
	rtld_db_logging = on_off;
	(void) mutex_unlock(&glob_mutex);
}
rd_errstr()
{
	printf("proc-stub:%s\n", __func__);
}
rd_event_addr()
{
	printf("proc-stub:%s\n", __func__);
}
rd_event_enable()
{
	printf("proc-stub:%s\n", __func__);
}
rd_event_getmsg()
{
	printf("proc-stub:%s\n", __func__);
}


/**********************************************************************/
/*   This  file  contains  pure  linux  stuff - no #defines or other  */
/*   things which complicate us with the rest of the dtrace code.     */
/**********************************************************************/

#include <linux/miscdevice.h>
#include <linux/smp.h>
#include <linux/gfp.h>
#include <linux/kdev_t.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/fs.h>
#include <linux/device.h>

char *
linux_get_proc_comm(void)
{
	return current->comm;
}


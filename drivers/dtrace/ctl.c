/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/**********************************************************************/
/*   This file contains a thin /proc/$pid/ctl interface (procfs).     */
/**********************************************************************/

#include <dtrace_linux.h>
#include <linux/miscdevice.h>
#include <sys/modctl.h>
#include <sys/dtrace.h>
#include <sys/stack.h>


/*ARGSUSED*/
static int
ctl_open(dev_t *devp, int flag, int otyp, cred_t *cred_p)
{
	return (0);
}

static const struct file_operations ctl_fops = {
        .open = ctl_open,
};

static struct miscdevice ctl_dev = {
        MISC_DYNAMIC_MINOR,
        "dtrace_ctl",
        &ctl_fops
};

int ctl_init(void)
{	int	ret;

	ret = misc_register(&ctl_dev);
	if (ret) {
		printk(KERN_WARNING "ctl: Unable to register misc device\n");
		return ret;
		}

	printk(KERN_WARNING "ctl loaded: /dev/dtrace_ctl now available\n");

	return 0;
}
void ctl_exit(void)
{
	printk(KERN_WARNING "ctl driver unloaded.\n");
	misc_deregister(&ctl_dev);
}


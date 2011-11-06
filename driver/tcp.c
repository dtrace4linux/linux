/**********************************************************************/
/*                                                                    */
/*  File:          tcp.c                                              */
/*  Author:        P. D. Fox                                          */
/*  Created:       6 Nov 2011                                         */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  Description:  TCP provider implementation                         */
/*--------------------------------------------------------------------*/
/*  $Header: Last edited: 06-Nov-2011 1.1 $ */
/**********************************************************************/

#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include <dtrace_linux.h>
#include <sys/privregs.h>
#include <sys/dtrace_impl.h>
#include <linux/sys.h>
#undef comm /* For 2.6.36 and above - conflict with perf_event.h */
#include <sys/dtrace.h>
#include <dtrace_proto.h>

/**********************************************************************/
/*   Initialise the tcp provider probes.			      */
/**********************************************************************/
void tcp_init()
{
	prcom_add_function("tcp:::state-change", "tcp_set_state");
}

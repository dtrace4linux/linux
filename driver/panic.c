/**********************************************************************/
/*                                                                    */
/*  File:          panic.c                                            */
/*  Author:        P. D. Fox                                          */
/*  Created:       29 Oct 2011                                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*   Description: Optional debug code to handle a kernel panic.	      */
/*   We might want to try and salvage or dump diags.		      */
/*--------------------------------------------------------------------*/
/*  $Header: Last edited: 29-Oct-2011 1.1 $ */
/**********************************************************************/

#include "dtrace_linux.h"

int 
dtrace_kernel_panic(struct notifier_block *this, unsigned long event, void *ptr)
{
	printk("Dtrace might have panic'ed kernel. Sorry.\n");
	return NOTIFY_DONE;
}


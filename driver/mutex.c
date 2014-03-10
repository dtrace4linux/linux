/**********************************************************************/
/*                                                                    */
/*  File:          mutex.c                                            */
/*  Author:        P. D. Fox                                          */
/*  Created:       13 Nov 2011                                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  Description:  Mutex implementations                               */
/*   We  support  two  styles  of mutexes: those called from the top  */
/*   half  of the kernel, and run with interrupts enabled, and those  */
/*   called from probe context.					      */
/*   								      */
/*   Upper  half  mutexes  are  accessed via mutex_enter/mutex_exit.  */
/*   Lower half via mutex_enter/mutex_exit.			      */
/*   								      */
/*   The  same  mutex  may be called by either function, but must be  */
/*   limited to the appropriate half of the kernel.		      */
/*   								      */
/*   Without  this,  we  may  end  up  causing kernel diagnostics or  */
/*   deadlock.   mutex_enter   will  grab  the  mutex,  and  disable  */
/*   interrupts during the grab, but re-enable after the grab.	      */
/*   								      */
/*   mutex_enter will leave interrupts disabled.		      */
/*   								      */
/*   We  need  to avoid the spin locks and semaphores/mutexes of the  */
/*   kernel,   because   they   do   preemption   advice,   and  the  */
/*   WARN_ON/BUG_ON  macros in the kernel can fire, e.g. on kmalloc,  */
/*   when allocating memory if the irqs_disabled() function disagree  */
/*   with the allocation flags.					      */
/*--------------------------------------------------------------------*/
/*  $Header: Last edited: 04-Dec-2011 1.3 $ 			      */
/**********************************************************************/

#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include <sys/dtrace.h>
#include <dtrace_proto.h>

unsigned long cnt_mtx1;
unsigned long cnt_mtx2;
unsigned long cnt_mtx3;

/**********************************************************************/
/*   Lock mutex                                                       */
/**********************************************************************/
void
mutex_enter(mutex_t *mp)
{
	cnt_mtx1++;

	mutex_lock(mp);
}


/**********************************************************************/
/*   Release mutex                             			      */
/**********************************************************************/
void
mutex_exit(mutex_t *mp)
{
	mutex_unlock(mp);
}

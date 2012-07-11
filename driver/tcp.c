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
#include <net/tcp.h>

static int
tcp_connect_established(dtrace_id_t id, struct pt_regs *regs)
{	struct sock *sk = (struct sock *) regs->c_arg0;
	int	state = regs->c_arg2;

//printk("tcp_connect_established %d -> %d\n", sk->sk_state, state);

	return sk->sk_state == TCP_SYN_SENT && state == TCP_ESTABLISHED ? TRUE : FALSE;
}
static int
tcp_connect_refused(dtrace_id_t id, struct pt_regs *regs)
{	struct sock *sk = (struct sock *) regs->c_arg0;
	int	state = regs->c_arg2;

//printk("tcp_connect_refused st=%d\n", sk->sk_state);
	switch (sk->sk_state) {
	  case TCP_SYN_SENT:
	  	return TRUE;
	  default:
	  	return FALSE;
	}
}
static int
tcp_connect_request(dtrace_id_t id, struct pt_regs *regs)
{	int	state = regs->c_arg2;

//printk("tcp_connect_request\n");
	return state == TCP_SYN_SENT ? TRUE : FALSE;
}
/**********************************************************************/
/*   Connect to a non-existant host...handle refusal here.	      */
/**********************************************************************/
static int
tcp_connect_refused2(dtrace_id_t id, struct pt_regs *regs)
{	struct sock *sk = (struct sock *) regs->c_arg0;
	int	state = regs->c_arg2;

	if (state == TCP_CLOSE && 
	    sk->sk_state != TCP_FIN_WAIT2) {
//printk("tcp_connect_refused2 %d\n", sk->sk_state);
		return TRUE;
		}
	return FALSE;
}
static int
tcp_accept_established(dtrace_id_t id, struct pt_regs *regs)
{	struct sock *sk = (struct sock *) regs->c_arg0;
	int	state = regs->c_arg2;

//printk("tcp_accept_established %d -> %d\n", sk->sk_state, state);
	if (state == TCP_ESTABLISHED && 
	    sk->sk_state == TCP_SYN_RECV) {
		return TRUE;
		}
	return FALSE;
}
static int
tcp_recv(dtrace_id_t id, struct pt_regs *regs)
{
	return 1;
}
static int
tcp_send(dtrace_id_t id, struct pt_regs *regs)
{
	return 1;
}

static int
udp_recv(dtrace_id_t *id, struct pt_regs *regs)
{
	return 1;
}
static int
udp_send(dtrace_id_t *id, struct pt_regs *regs)
{
	return 1;
}
/**********************************************************************/
/*   Initialise the tcp provider probes.			      */
/**********************************************************************/
void 
tcp_init(void)
{
	prcom_add_function("tcp:::state-change", "tcp_set_state");
	prcom_add_callback("tcp:::connect-established", "tcp_set_state", tcp_connect_established);
	prcom_add_callback("tcp:::connect-request", "tcp_set_state", tcp_connect_request);

	/***********************************************/
	/*   Two  scenarios  for refusal - host is up  */
	/*   and port is not open, or host is not up,  */
	/*   and connection is similuated/close.       */
	/***********************************************/
	prcom_add_callback("tcp:::connect-refused", "tcp_reset", tcp_connect_refused);
	prcom_add_callback("tcp:::connect-refused", "tcp_set_state", tcp_connect_refused2);

	prcom_add_callback("tcp:::accept-established", "tcp_set_state", tcp_accept_established);
	prcom_add_callback("tcp:::send", "tcp_event_data_sent", tcp_send);
	prcom_add_callback("tcp:::send", "tcp_event_new_data_sent", tcp_send);
	prcom_add_callback("tcp:::receive", "tcp_event_data_recv", tcp_recv);

	/***********************************************/
	/*   UDP.				       */
	/***********************************************/
	prcom_add_callback("udp:::receive", "udp_queue_rcv_skb", udp_recv);
	prcom_add_callback("udp:::send", "udp_sendmsg", udp_send);

}

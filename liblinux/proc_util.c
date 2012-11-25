/*-
 * Copyright (c) 2008 John Birrell (jb@freebsd.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include "Pcontrol.h"

#if !defined(PTRACE_SETOPTIONS)
#define PTRACE_SETOPTIONS   0x420
#endif

int
proc_clearflags(struct ps_prochandle *phdl, int mask)
{
	if (phdl == NULL)
		return (EINVAL);

//printf("HERE:%s\n", __func__);
	phdl->p_flags &= ~mask;

	return (0);
}

int
proc_continue(struct ps_prochandle *phdl)
{
	if (phdl == NULL)
		return (EINVAL);

//printf("HERE:%s phdl=%p\n", __func__, phdl);
	while (do_ptrace(__func__, PTRACE_CONT, phdl->pid, (caddr_t)(uintptr_t) 0, 0) != 0) {
//		fprintf(stderr, "Error: pid=%d ", phdl->pid);
//		perror("ptrace(PT_CONTINUE)");
		if (do_ptrace(__func__, PTRACE_ATTACH, phdl->pid, 0, 0) < 0) {
			perror("ptrace(PTRACE_ATTACH)");
		}
		break;
		return (errno);
	}

	phdl->p_status = PS_RUN;

	return (0);
}

int
proc_detach(struct ps_prochandle *phdl)
{
	if (phdl == NULL)
		return (EINVAL);

//printf("HERE:%s\n", __func__);
	if (do_ptrace(__func__, PT_DETACH, phdl->pid, 0, 0) != 0)
		return (errno);

	return (0);
}

int
proc_getflags(struct ps_prochandle *phdl)
{
	if (phdl == NULL)
		return (-1);

	return(phdl->p_flags);
}

int
proc_setflags(struct ps_prochandle *phdl, int mask)
{
	if (phdl == NULL)
		return (EINVAL);

//printf("HERE:%s\n", __func__);
	phdl->p_flags |= mask;

	return (0);
}

int
proc_state(struct ps_prochandle *phdl)
{
	if (phdl == NULL)
		return (-1);

//printf("HERE:%s status=%d\n", __func__, phdl->p_status);
	return (phdl->p_status);
}

int
proc_wait(struct ps_prochandle *phdl)
{	struct stat sbuf;
	char	buf[128];
	int status = 0;

	if (phdl == NULL)
		return (EINVAL);

//printf("HERE:%s\n", __func__);
	if (waitpid(phdl->pid, &status, 0) < 0)
		return -1;
	snprintf(buf, sizeof buf, "/proc/%d", phdl->pid);
	if (stat(buf, &sbuf) < 0) {
		phdl->p_status = PS_DEAD;
		}
	else
		phdl->p_status = PS_STOP;
	return status;
}

pid_t
proc_getpid(struct ps_prochandle *phdl)
{
	if (phdl == NULL)
		return (-1);

	return (phdl->pid);
}
int
do_kill(char *func, int pid, int sig)
{	int	ret = kill(pid, sig);

	if (getenv("DTRACE_KILL")) {
		printf("%s(): kill(%d, %d %s) := %d\n", 
			func, pid, sig, 
			sys_siglist[sig], ret);
	}
	return ret;
}
int
do_ptrace(char *func, int req, int pid, void *addr, void *data)
{	int	ret = ptrace(req, pid, addr, data);
	char	*str;
	char	buf[32];

	if (getenv("DTRACE_PTRACE") == NULL)
		return ret;
	str = 
		req == PTRACE_TRACEME ? "PTRACE_TRACEME" :
		req == PTRACE_CONT ? "PTRACE_CONT" :
		req == PTRACE_DETACH ? "PTRACE_DETACH" :
		req == PTRACE_ATTACH ? "PTRACE_ATTACH" :
		req == PTRACE_SETOPTIONS ? "PTRACE_SETOPTIONS" :
			NULL;
	if (str == NULL) {
		str = buf;
		snprintf(buf, sizeof buf, "0x%x", req);
	}

	printf("[%d] %s(): ptrace(%s, %d, %p, %p) := %d ",
		getpid(),
		func,
		str,
		pid, addr, data, ret);
	fflush(stdout);
	if (ret == -1)
		perror("");
	else
		printf("\n");
	return ret;
}

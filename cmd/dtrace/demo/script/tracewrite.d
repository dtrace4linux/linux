#!/usr/sbin/dtrace -s

syscall::write:entry
/pid == $1/
{
}

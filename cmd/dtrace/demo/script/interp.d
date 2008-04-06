#!/usr/sbin/dtrace -s

BEGIN
{
	trace("hello");
	exit(0);
}

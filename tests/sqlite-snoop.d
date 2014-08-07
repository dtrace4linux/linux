#!/usr/sbin/dtrace -Zs

/*
 
sqlite-snoop.d
 
Author: Damian Carrillo
Usage: sudo ./libsqlite_snoop.d <pid>
Date: 2012-04-03
https://gist.github.com/damiancarrillo/2395146
 
*/
 
#pragma D option quiet
#pragma D option switchrate=10hz
 
dtrace:::BEGIN
{
printf("%-8s %6s %s\n", "TIME(ms)", "Q(ms)", "QUERY");
timezero = timestamp;
}
 
pid$1:libsqlite3*:sqlite3_prepare*:entry
{
self->query = copyinstr(arg1);
self->start = timestamp;
}
 
pid$1:libsqlite3*:sqlite3_prepare*:entry
/self->start/
{
this->time = (timestamp - self->start) / 1000000;
this->now = (timestamp - timezero) / 1000000;
printf("%-8d %6d %s\n", this->now, this->time, self->query);
self->start = 0; self->query = 0;
}
 
pid$1:libsqlite3*:sqlite3_exec*:entry
{
self->query = copyinstr(arg1);
self->start = timestamp;
}
 
pid$1:libsqlite3*:sqlite3_exec*:entry
/self->start/
{
this->time = (timestamp - self->start) / 1000000;
this->now = (timestamp - timezero) / 1000000;
printf("%-8d %6d %s\n", this->now, this->time, self->query);
self->start = 0; self->query = 0;
}

struct callinfo {
	uint64_t ts;      /* timestamp of last syscall entry */
	uint64_t elapsed; /* total elapsed time in nanoseconds */
	uint64_t calls;   /* number of calls made */
	size_t maxbytes;  /* maximum byte count argument */
};

struct callinfo i[string];	/* declare i as an associative array */

syscall::read:entry, syscall::write:entry
/pid == $1/
{
	i[probefunc].ts = timestamp;
	i[probefunc].calls++;
	i[probefunc].maxbytes = arg2 > i[probefunc].maxbytes ?
		arg2 : i[probefunc].maxbytes;
}

syscall::read:return, syscall::write:return
/i[probefunc].ts != 0 && pid == $1/
{
	i[probefunc].elapsed += timestamp - i[probefunc].ts;
}

END
{
	printf("        calls  max bytes  elapsed nsecs\n");
	printf("------  -----  ---------  -------------\n");
	printf("  read  %5d  %9d  %d\n",
	    i["read"].calls, i["read"].maxbytes, i["read"].elapsed);
	printf(" write  %5d  %9d  %d\n",
	    i["write"].calls, i["write"].maxbytes, i["write"].elapsed);
}

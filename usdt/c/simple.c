/**********************************************************************/
/*   Simple demonstration of probes embedded into an executable.      */
/**********************************************************************/
# include <stdio.h>
# include <sys/sdt.h>
# include <time.h>

/**********************************************************************/
/*   Following structs are just for debugging.			      */
/**********************************************************************/
#define	DOF_ID_SIZE	16	/* total size of dofh_ident[] in bytes */

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef struct dof_hdr {
	uint8_t dofh_ident[DOF_ID_SIZE]; /* identification bytes (see below) */
	uint32_t dofh_flags;		/* file attribute flags (if any) */
	uint32_t dofh_hdrsize;		/* size of file header in bytes */
	uint32_t dofh_secsize;		/* size of section header in bytes */
	uint32_t dofh_secnum;		/* number of section headers */
	uint64_t dofh_secoff;		/* file offset of section headers */
	uint64_t dofh_loadsz;		/* file size of loadable portion */
	uint64_t dofh_filesz;		/* file size of entire DOF file */
	uint64_t dofh_pad;		/* reserved for future use */
} dof_hdr_t;

typedef struct dof_sec {
	uint32_t dofs_type;	/* section type (see below) */
	uint32_t dofs_align;	/* section data memory alignment */
	uint32_t dofs_flags;	/* section flags (if any) */
	uint32_t dofs_entsize;	/* size of section entry (if table) */
	uint64_t dofs_offset;	/* offset of section data within file */
	uint64_t dofs_size;	/* size of section data in bytes */
} dof_sec_t;

extern dof_hdr_t __SUNW_dof;
dof_sec_t	*secp;

int end_main(void);

static void
dump_dof_section()
{	int	n;

	/***********************************************/
	/*   For debugging, dump the DOF header which  */
	/*   describes   the  probes  in  this  file.  */
	/*   Normal apps dont need this.	       */
	/***********************************************/
	printf("__SUNW_dof header:\n");
	printf("dofh_flags     %08x\n", __SUNW_dof.dofh_flags);
	printf("dofh_hdrsize   %08x\n", __SUNW_dof.dofh_hdrsize);
	printf("dofh_secsize   %08x\n", __SUNW_dof.dofh_secsize);
	printf("dofh_secnum    %08x\n", __SUNW_dof.dofh_secnum);
	printf("dofh_secoff    %p\n", (void *) __SUNW_dof.dofh_secoff);
	printf("dofh_loadsz    %p\n", (void *) __SUNW_dof.dofh_loadsz);
	printf("dofh_filesz    %p\n", (void *) __SUNW_dof.dofh_filesz);
	secp = (dof_sec_t *) (&__SUNW_dof + 1);
	for (n = 0; n < __SUNW_dof.dofh_secnum; secp++, n++) {
		printf("%d: %04x %04x %04x %04x %08x %08llx\n", n,
			secp->dofs_type,
			secp->dofs_align,
			secp->dofs_flags,
			secp->dofs_entsize,
			secp->dofs_offset,
			(long long) secp->dofs_size);
	}
}

int main(int argc, char **argv)
{	int	n;
	unsigned long crc;
	unsigned char *cp;

	/***********************************************/
	/*   Invoke shlib function.		       */
	/***********************************************/
	fred();

	dump_dof_section();

	for (n = 0; ; n++) {
		char	buf[64];
		time_t	t = time(NULL);
		struct tm *tm = localtime(&t);

		strftime(buf, sizeof buf, "%H:%M:%S", tm);

		/***********************************************/
		/*   compute  checksum  for  main() so we can  */
		/*   watch if we get patched.		       */
		/***********************************************/
		crc = 0;
		for (cp = (unsigned char *) main; cp != (unsigned char *) end_main; cp++)
			crc += *cp;

		printf("%s PID:%d %d: here on line %d: crc=%08lx\n", buf, getpid(), n, __LINE__, crc);
		DTRACE_PROBE1(simple, saw__line, 0x1234);
		printf("%s PID:%d here on line %d\n", buf, getpid(), __LINE__);
		DTRACE_PROBE1(simple, saw__word, 0x87654321);
		printf("%s PID:%d here on line %d\n", buf, getpid(), __LINE__);
		DTRACE_PROBE1(simple, saw__word, 0xdeadbeef);
		printf("%s PID:%d here on line %d\n", buf, getpid(), __LINE__);
		sleep(2);
		}
}
int end_main(void)
{
	return 0;
}

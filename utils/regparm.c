/*

Simple code to test the 64-bit div operators in GCC for i386 cpu targets.

First command is how the kernel gets compiled demonstrating bad
div/mod instructions.

gcc -g -mregparm=3 -O2 -freg-struct-return -mpreferred-stack-boundary=2  \
	-march=i586 -mtune=generic -o x.bad regparm.c drivers/dtrace/divmod64.c

This example shows how we can work around this.

gcc -I/usr/src/linux/include \
	-Wl,--wrap,__moddi3 \
	-Wl,--wrap,__udivdi3 \
	-Wl,--wrap,__umoddi3 \
	-Wl,--wrap,__divdi3 \
	-g -mregparm=3 -O2 -freg-struct-return -mpreferred-stack-boundary=2  \
	-march=i586 -mtune=generic -o x.good \
	regparm.c ../drivers/dtrace/divmod64.c

Plain ol' gcc works for user space:

gcc -o x.good regparm.c

Paul Fox
June 2008

*/

# include <unistd.h>
# define uint32_t	unsigned int
# define uint64_t	unsigned long long

typedef struct dof_sec {
        uint32_t dofs_type;     /* section type (see below) */
        uint32_t dofs_align;    /* section data memory alignment */
        uint32_t dofs_flags;    /* section flags (if any) */
        uint32_t dofs_entsize;  /* size of section entry (if table) */
        uint64_t dofs_offset;   /* offset of section data within file */
        uint64_t dofs_size;     /* size of section data in bytes */
} dof_sec_t;

dof_sec_t s;
dof_sec_t *d;

unsigned long long size = 8;
unsigned int entsize = 4;
void
fred()
{
	s.dofs_size = 8;
	s.dofs_entsize = 8;
	d = &s;
}
int main()
{
	fred();
//        printf("sizeof intptr_t=%d\n", sizeof(intptr_t));
//	printf("sizeof long=%d\n", sizeof(long));
	if (d->dofs_entsize != 0 &&
	   (d->dofs_size % d->dofs_entsize) != 0) {
	   	write(1, "bug\n", 4);
	} else 
		write(1, "ok\n", 3);

	printf("mod=%d\n", (size % entsize) != 0);
	printf("div=%d\n", size / entsize);
}


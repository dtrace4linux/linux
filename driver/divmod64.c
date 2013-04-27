/**********************************************************************/
/*   This  code allows i386 code to do 64-bit divides in the kernel.  */
/*   Based  on  an  idea  by IBM about wrapping the calls to the GCC  */
/*   generated  64-bit  operations.  We  can  almost  use  this from  */
/*   userspace  -  see utils/regarm.c for an example of compiling to  */
/*   test this.							      */
/*   								      */
/*   Paul Fox June 2008						      */
/**********************************************************************/

# include <linux/kernel.h>
# include <asm/div64.h>

# define DEBUG 0

# if !defined(__KERNEL__)
# define printk printf
# endif

# if defined(__arm__)
	// See arm_divmod64.S
# endif /* defined(__arm__) */

long long
__wrap___divdi3(long long a, long long b)
{	long long a1 = a;
	long long b1 = b;

	do_div(a1, b1);

# if DEBUG
	printk("__wrap___divdi3: %lld / %lld = %lld\n", a, b, a1);
# endif
	return a1;
}

unsigned long long
__wrap___udivdi3(unsigned long long a, unsigned long long b)
{	unsigned long long a1 = a;
	unsigned long long b1 = b;

	do_div(a1, b1);
# if DEBUG
	printk("__wrap___udivdi3: %llu / %llu = %llu\n", a, b, a1);
# endif
	return a1;
}

long long
__wrap___moddi3(long long a, long long b)
{	long long a1 = a;
	long long b1 = b;

	long long ret = do_div(a1, b1);
# if DEBUG
	printk("__wrap___moddi3: %lld %% %lld = %lld\n", a, b, ret);
# endif
	return ret;
}

unsigned long long
__wrap___umoddi3(unsigned long long a, unsigned long long b)
{	unsigned long long a1 = a;
	unsigned long long b1 = b;

	unsigned long long ret = do_div(a1, b1);
# if DEBUG
	printk("__wrap___umoddi3: %llu %% %llu = %llu\n", a, b, ret);
# endif
	return ret;
}

/**********************************************************************/
/*   On  ARM/libgcc.a  may invoke raise. Avoid causing any damage if  */
/*   this happens, e.g. divide by zero.				      */
/**********************************************************************/
int
raise(int sig)
{
	return 0;
}


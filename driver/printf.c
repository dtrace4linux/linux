/**********************************************************************/
/*   File  containing  dtrace_printf  and  related  debug functions.  */
/*   These  functions  rely  on  no kernel code and write to a trace  */
/*   buffer  (/proc/dtrace/trace), so we can call them safely during  */
/*   critical regions of the kernel.				      */
/*   								      */
/*   Author: Paul D. Fox					      */
/*   Date: December 2011					      */
/*   								      */
/*   License: CDDL						      */
/**********************************************************************/

#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include "dtrace_linux.h"
#include <sys/dtrace_impl.h>
#include "dtrace_proto.h"

/**********************************************************************/
/*   This  doesnt really do anything, but we might use it to dynamic  */
/*   switch print mode.						      */
/**********************************************************************/
int dtrace_printk;
module_param(dtrace_printk, int, 0);

/**********************************************************************/
/*   dtrace_printf() buffer and state.				      */
/**********************************************************************/
#define	LOG_BUFSIZ (64 * 1024)
char	dtrace_buf[LOG_BUFSIZ];
int	dbuf_i;
int	dtrace_printf_disable;
const int log_bufsiz = LOG_BUFSIZ; /* So dtrace_linux.c can use it. */

/**********************************************************************/
/*   Routine with zero outside dependencies, and hence callable from  */
/*   any context, to format a hrtime_t time difference.		      */
/**********************************************************************/
char *
hrtime_str(hrtime_t s)
{	int	s1 = s / (1000 * 1000 * 1000);
	int	s2 = s % (1000 * 1000 * 1000);
	int	i;
static char buf[44];
static char tmp[44];
	char	*bp = buf;

	for (i = 0; ; ) {
		tmp[i++] = (s1 % 10) + '0';
		s1 /= 10;
		if (s1 == 0)
			break;
	}
	for (; --i >= 0; ) {
		*bp++ = tmp[i];
	}
	
	*bp++ = '.';

	for (i = 0; i < 9; ) {
		tmp[i++] = (s2 % 10) + '0';
		s2 /= 10;
	}
	for (; --i >= 0; ) {
		*bp++ = tmp[i];
	}
	*bp = '\0';
	return buf;
}

/**********************************************************************/
/*   Convert  all  our printks to the internal dtrace_printf. If you  */
/*   change  the  name  of  this  function, you can revert to kernel  */
/*   printk()s,  but  might  suffer deadlock in the printk() locking  */
/*   regime.							      */
/**********************************************************************/
asmlinkage int
printk(const char *fmt, ...)
{	va_list	ap;

	va_start(ap, fmt);
	dtrace_vprintf(fmt, ap);
	va_end(ap);
	return 0;
}

int 
dtrace_kernel_panic(struct notifier_block *this, unsigned long event, void *ptr)
{
	printk("Dtrace might have panic'ed kernel. Sorry.\n");
	return NOTIFY_DONE;
}

/**********************************************************************/
/*   Internal logging mechanism for dtrace. Avoid calls to printk if  */
/*   we are in dangerous territory).				      */
/**********************************************************************/
volatile int dtrace_printf_lock = -1;
# define ADDCH(ch) {dtrace_buf[dbuf_i] = ch; dbuf_i = (dbuf_i + 1) % LOG_BUFSIZ;}
#define	MAX_DIGITS 40 /* In case of buggy divmod64.c */
static	char	tmp[48];

void
dtrace_printf(const char *fmt, ...)
{
	va_list	ap;
	va_start(ap, fmt);
	dtrace_vprintf(fmt, ap);
	va_end(ap);
}
static void
dtrace_printf_int(int n)
{	int	i;

	for (i = 0; ; ) {
		tmp[i++] = (n % 10) + '0';
		n /= 10;
		if (n == 0)
			break;
	}
	for (; --i >= 0; ) {
		ADDCH(tmp[i]);
	}
}
void
dtrace_vprintf(const char *fmt, va_list ap)
{	short	ch;
	unsigned long long n;
	unsigned long sec, nsec;
	char	*cp;
	short	i;
	short	l_mode;
	short	zero;
	short	width;
	hrtime_t hrt;
	static char digits[] = "0123456789abcdef";
	static hrtime_t	hrt0;

# if 0
	/***********************************************/
	/*   Temp: dont wrap buffer - because we want  */
	/*   to see first entries.		       */
	/***********************************************/
	if (dbuf_i >= LOG_BUFSIZ - 2048)
		goto exit;
# endif
	if (dtrace_printf_disable)
		goto exit;

	preempt_disable_notrace();

	hrt = dtrace_gethrtime();

	/***********************************************/
	/*   Try  and  avoid intermingled output from  */
	/*   the  other  cpus.  Dont  do  this  if we  */
	/*   interrupt our own cpu.		       */
	/***********************************************/
	while (dtrace_printf_lock >= 0 && dtrace_printf_lock != smp_processor_id())
		;
	/***********************************************/
	/*   Allow a blank string - dont generate any  */
	/*   output.  We  often  use this to add some  */
	/*   slowdown  to paths of code. We will wait  */
	/*   for  any  locks,  but not grab the lock,  */
	/*   just for a bit more entropy.	       */
	/***********************************************/
	if (*fmt == '\0')
		goto exit;
	dtrace_printf_lock = smp_processor_id();

	/***********************************************/
	/*   Add in timestamp.			       */
	/***********************************************/
	if (hrt0 == 0)
		hrt0 = hrt;
	if (hrt) {
		hrt -= hrt0;
		sec = (unsigned long) (hrt / (1000 * 1000 * 1000));
		nsec = (unsigned long) (hrt % (1000 * 1000 * 1000));
		for (i = 0; ; ) {
			tmp[i++] = (sec % 10) + '0';
			sec /= 10;
			if (sec == 0)
				break;
		}
		for (; --i >= 0; ) {
			ADDCH(tmp[i]);
		}
		ADDCH('.');
		for (i = 0; i < 9; ) {
			tmp[i++] = (nsec % 10) + '0';
			nsec /= 10;
		}
		for (; --i >= 0; ) {
			ADDCH(tmp[i]);
		}
		ADDCH(' ');
	}
	/***********************************************/
	/*   Add the current CPU.		       */
	/***********************************************/
	ADDCH('#');
	dtrace_printf_int(smp_processor_id());
	ADDCH(' ');
	dtrace_printf_int(get_current()->pid);
	if (irqs_disabled()) {
		ADDCH('-');
	} else {
		ADDCH(':');
	}

	while ((ch = *fmt++) != '\0') {
		if (ch != '%') {
			ADDCH(ch);
			continue;
		}

		zero = ' ';
		width = -1;

		if ((ch = *fmt++) == '\0')
			break;
		if (ch == '0')
			zero = '0';
		while (ch >= '0' && ch <= '9') {
			if (width < 0)
				width = ch - '0';
			else
				width = 10 * width + ch - '0';
			if ((ch = *fmt++) == '\0')
				break;
		}
		l_mode = FALSE;
		if (ch == '*') {
			width = (int) va_arg(ap, int);
			if ((ch = *fmt++) == '\0')
				break;
		}
		if (ch == '.') {
			if ((ch = *fmt++) == '\0')
				break;
		}
		if (ch == '*') {
			width = (int) va_arg(ap, int);
			if ((ch = *fmt++) == '\0')
				break;
		}
		if (ch == 'l') {
			l_mode = TRUE;
			if ((ch = *fmt++) == '\0')
				break;
			if (ch == 'l') {
				l_mode++;
				if ((ch = *fmt++) == '\0')
			  		break;
			}
		}
		switch (ch) {
		  case 'c':
		  	ch = (char) va_arg(ap, int);
			ADDCH(ch);
		  	break;
		  case 'd':
		  case 'u':
			if (l_mode) {
			  	n = va_arg(ap, unsigned long);
			} else {
			  	n = va_arg(ap, unsigned int);
			}
			if (ch == 'd' && (long) n < 0) {
				ADDCH('-');
				n = -n;
			}
			for (i = 0; i < MAX_DIGITS; i++) {
				tmp[i] = '0' + (n % 10);
				n /= 10;
				if (n == 0)
					break;
			}
			while (i >= 0)
				ADDCH(tmp[i--]);
		  	break;
		  case 'p':
#if defined(__i386) || defined(__arm__)
		  	width = 8;
#else
		  	width = 16;
#endif
			zero = '0';
			l_mode = TRUE;
			// fallthru...
		  case 'x':
			if (l_mode) {
			  	n = va_arg(ap, unsigned long);
			} else {
			  	n = va_arg(ap, unsigned int);
			}
			for (i = 0; ; ) {
				tmp[i++] = digits[(n & 0xf)];
				n >>= 4;
				if (n == 0)
					break;
			}
			width -= i;
			while (width-- > 0)
				ADDCH(zero);
			while (--i >= 0)
				ADDCH(tmp[i]);
		  	break;
		  case 's':
		  	cp = va_arg(ap, char *);
			if (cp == NULL)
				cp = "(null)";
			while (*cp) {
				ADDCH(*cp++);
				if (width >= 0) {
					if (--width < 0)
						break;
				}
			}
		  	break;
		  }
	}

	dtrace_printf_lock = -1;

exit:
	preempt_enable_no_resched_notrace();
}

/**********************************************************************/
/*   Utility  routine  for debugging, mostly not needed. Turn on all  */
/*   writes  to the console - may be needed when debugging low level  */
/*   interrupts which crash the box.   				      */
/**********************************************************************/
void
set_console_on(int flag)
{	int	mode = flag ? 7 : 0;
static	int first_time = TRUE;
static	int *console_printk;
	
	if (first_time) {
		console_printk = get_proc_addr("console_printk");
		first_time = FALSE;
	}

	if (console_printk)
		console_printk[0] = mode;
}

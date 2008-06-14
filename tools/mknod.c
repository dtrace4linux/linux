# include <errno.h>
# include <sys/types.h>
# include <sys/stat.h>

int main()
{	int	ret;
	int	i;

	/***********************************************/
	/*   Strange  libc  bug  here  --  remove the  */
	/*   getpid  and watch the first mknod() fail  */
	/*   -  not because of EEXIST, but because it  */
	/*   is never actually executed!	       */
	/***********************************************/
	getpid();
	for (i = 0; ; i++) {
		if (mknod("xxx", 0644, 0) < 0) {
			if (errno != EEXIST)
				perror("mknod(xxx)");
			}
		}
}

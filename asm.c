// some test code to get the validate_ptr working.

#define __copy_user(to,from,size)     \
do {         			      \
 int __d0, __d1;       		      \
 __asm__ __volatile__(      	      \
  "0: rep; movsl\n"     	      \
  " movl %3,%0\n"     		      \
  "1: rep; movsb\n"     	      \
  "2:\n"       			      \
  ".section .fixup,\"ax\"\n"          \
  "3: lea 0(%3,%0,4),%0\n"    	      \
  " jmp 2b\n"     		      \
  ".previous\n"      		      \
  ".section __ex_table,\"a\"\n"       \
  " .align 4\n"     		      \
  " .long 0b,3b\n"     		      \
  " .long 1b,2b\n"     		      \
  ".previous"      		      \
  : "=&c"(size), "=&D" (__d0), "=&S" (__d1)  		 \
  : "r"(size & 3), "0"(size / 4), "1"(to), "2"(from) 	 \
  : "memory");      					 \
} while (0)

#define __validate_ptr(ptr, ret)        \
 __asm__ __volatile__(      	      \
  "  mov $1, %0\n" 		      \
  "0: mov (%1), %1\n"                \
  "2:\n"       			      \
  ".section .fixup,\"ax\"\n"          \
  "3: mov $0, %0\n"    	              \
  " jmp 2b\n"     		      \
  ".previous\n"      		      \
  ".section __ex_table,\"a\"\n"       \
  " .align 8\n"     		      \
  " .quad 0b,3b\n"     		      \
  ".previous"      		      \
  : "=&a" (ret) 		      \
  : "c" (ptr) 	                      \
  )

int
validate_ptr(void *ptr)
{	int	ret;

	__validate_ptr(ptr, ret);
	return ret;
}

int glob_ptr;
int *xyzzy = &glob_ptr;

int main(int argc, char **argv)
{	char	src[10];
	char	dst[10];
	int	size = sizeof(src);
	int	ret;

//	__copy_user(dst, src, size);
	if (*xyzzy)
		printf("hello\n");
	getpid(&xyzzy);
	ret = validate_ptr(xyzzy);

	printf("ret=%d\n", ret);
}

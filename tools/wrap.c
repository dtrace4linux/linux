#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

typedef struct wrap_t {
        char    *wr_name;
        void    *(*wr_func)();
        void    *(*wr_wrapper)();
        char    wr_buf[256];
        char    wr_save[32];
        } wrap_t;

wrap_t  wr;

char    wr_call0[1024];

void wrap_mgr(void);

# define FUNCTION(x)                    \
        ".text\n"                       \
        ".globl " #x "\n"               \
        ".type   " #x ", @function\n"   \
        #x ":\n"
#define DEF_VAR(x) \
        ".global " #x"\n" \
        ".type " #x ", @object\n" \
        #x ":\n"
# define END_FUNCTION(x) \
        ".size " #x ", .-" #x "\n"

extern int template_code();
extern int tpl_offset0;
extern int tpl_offset0_end;
extern int tpl_offset1;
extern int tpl_offset2;
extern int tpl_size;
void
dummy_template_code()
{
        asm(
                FUNCTION(template_code)
                "push %rax\n"
                DEF_VAR(tpl_offset0)
                "mov $0x12345678123456, %rax\n"
                "jmp *%rax\n"
                DEF_VAR(tpl_offset0_end)

                /***********************************************/
                /*   Above  jumps  to here, where we call the  */
                /*   real code and then flyback home.          */
                /***********************************************/
                ".align 8\n"
                "push %rdi\n"
                "push %rsi\n"
                "push %rbp\n"
                "push %rbx\n"
                "push %rdx\n"
                "push %rcx\n"
                "push %rax\n"
                "push %r8\n"
                "push %r9\n"

                DEF_VAR(tpl_offset1)
                "mov $0x12345678123456, %rax\n"
                "call *%rax\n"

                "pop %r9\n"
                "pop %r8\n"
                "pop %rax\n"
                "pop %rcx\n"
                "pop %rdx\n"
                "pop %rbx\n"
                "pop %rbp\n"
                "pop %rsi\n"
                "pop %rdi\n"

"push %rax\n"
                DEF_VAR(tpl_offset2)
"mov $0x1234567812345678,%rax\n"
"xchg %rax,(%rsp)\n"
"ret\n"

                DEF_VAR(tpl_size)
                END_FUNCTION(template_code)
                );
}
void
wrap(wrap_t *wr)
{       int     size = (int) &tpl_size - (int) &template_code;
        int     off0 = (int) &tpl_offset0 - (int) &template_code + 2;
        int     off0_size = (int) &tpl_offset0_end - (int) &tpl_offset0;
        int     off1 = (int) &tpl_offset1 - (int) &template_code + 2;
        int     off2 = (int) &tpl_offset2 - (int) &template_code + 2;
        int     ret;

#define pgaddr(x) (((unsigned long) (x)) & ~(PAGE_SIZE - 1))
        ret = mprotect((void *) pgaddr(wr->wr_func), 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
        ret = mprotect((void *) pgaddr(wr->wr_buf), 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

#define STORE4(addr, val) *(unsigned int *) (addr) = (unsigned int) (val)
#define STORE8(addr, val) *(unsigned long *) (addr) = (unsigned long) (val)

        memcpy(wr->wr_buf, template_code, size);
        STORE8(&wr->wr_buf[off0], wr->wr_func);
        STORE8(&wr->wr_buf[off1], wr->wr_wrapper);
        STORE8(&wr->wr_buf[off2], (unsigned char *) wr->wr_func + 8);

        memcpy(wr->wr_func, wr->wr_buf, off0_size);
        printf("func %p is wrapped\n", wr->wr_func);
}
void
wrap2(unsigned char *func, unsigned char *wr_func)
{       int     w0, w1;

        /***********************************************/
        /*   Dispatch orig->tramp                      */
        /***********************************************/
        w0 = 0;
        wr_call0[w0++] = 0xe9; // jmp
        *(unsigned int *) (wr_call0 + w0) = (
                int) wr_call0 + 8 - 5 - (int) func;
        w0 += 4;
        wr_call0[w0++] = 0x90; // nop
        wr_call0[w0++] = 0x90; // nop
        wr_call0[w0++] = 0x90; // nop

        /***********************************************/
        /*   Trampoline.                               */
        /***********************************************/
        w1 = w0;
        wr_call0[w1++] = 0x57; // push %rdi
        wr_call0[w1++] = 0x56; // push %rsi
        wr_call0[w1++] = 0x55; // push %rbp
        wr_call0[w1++] = 0x53; // push %rbx
        wr_call0[w1++] = 0x52; // push %rdx
        wr_call0[w1++] = 0x51; // push %rcx
        wr_call0[w1++] = 0x50; // push %rax
        wr_call0[w1++] = 0x41; wr_call0[w1++] = 0x51; // push %r9
        wr_call0[w1++] = 0x41; wr_call0[w1++] = 0x50; // push %r8

#if 0
        wr_call0[w1++] = 0xe8; // call
        *(unsigned int *) (wr_call0 + w1) = 
                                (int) wr_func - 5 - 
                                (int) (wr_call0 + w1 - 1);
        w1 += 4;
#else
        wr_call0[w1++] = 0x48; // mov wr_func, %rax
        wr_call0[w1++] = 0xb8;
        *(void **) &wr_call0[w1] = wr_func;
        w1 += 8;
        wr_call0[w1++] = 0xff; // call *%rax
        wr_call0[w1++] = 0xd0;
        
#endif
        wr_call0[w1++] = 0x41; wr_call0[w1++] = 0x58; // pop %r9
        wr_call0[w1++] = 0x41; wr_call0[w1++] = 0x59; // pop %r8
        wr_call0[w1++] = 0x58; // pop %rax
        wr_call0[w1++] = 0x59; // pop %rcx
        wr_call0[w1++] = 0x5a; // pop %rdx
        wr_call0[w1++] = 0x5b; // pop %rbx
        wr_call0[w1++] = 0x5d; // pop %rbp
        wr_call0[w1++] = 0x5e; // pop %rsi
        wr_call0[w1++] = 0x5f; // pop %rdi

        memcpy(&wr_call0[w1], func, 8);
        w1 += 8;
        wr_call0[w1++] = 0xe9; // jmp
        *(unsigned int *) (wr_call0 + w1) =
                                (int) (func + 8) -
                                (int) (wr_call0 + w1 + 4);
        w1 += 4;

        memcpy(func, wr_call0, w0);
        printf("func %p is wrapped\n", func);

}
void
wrap_mgr()
{
}
typedef int (*func_t)();
func_t tbl[10];
void (*xx)() = 0x1234567812345678;
void dummy(int n)
{
        tbl[n]();
        xx();
        asm("call *%rax\n");
        asm("add $8, %rsp\n");
        asm("call 0x12345678\n");
        asm("push %rdi\n");
        asm("pop %rdi\n");
        asm("push %r9\n");
        asm("push %r8\n");
        asm("pop %r8\n");
        asm("pop %r9\n");
}
void
wrapped_fred(int a, int b)
{
        printf("wrapped fred(%d, %d)\n",a, b);
}
//////////////////////////////////////////
void
fred(int n, int a, int b, int c, int d, int e, int f)
{
        printf("fred: n=%d n=%d a=%d %d %d %d %d %d\n", n, n * 3, a, b, c, d, e, f);

}
int main(int argc, char **argv)
{       int     i;

        wr.wr_name = "fred";
        wr.wr_func = fred;
        wr.wr_wrapper = wrapped_fred;
        wrap(&wr);

        for (i = 0; i < 10; i++)
                fred(i, 100, 101, 102, 103, 104, 105);
}
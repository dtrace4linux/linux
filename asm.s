	.file	"asm.c"
	.text
.globl validate_ptr
	.type	validate_ptr, @function
validate_ptr:
.LFB2:
	pushq	%rbp
.LCFI0:
	movq	%rsp, %rbp
.LCFI1:
	movq	%rdi, -24(%rbp)
	leaq	-24(%rbp), %rcx
#APP
	  mov 1, %eax
0: mov (%rcx), %rcx
2:
.section .fixup,"ax"
3: mov 0, %eax
 jmp 2b
.previous
.section __ex_table,"a"
 .align 8
 .quad 0b,2b
.previous
#NO_APP
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	leave
	ret
.LFE2:
	.size	validate_ptr, .-validate_ptr
	.section	.rodata
.LC0:
	.string	"ret=%d\n"
	.text
.globl main
	.type	main, @function
main:
.LFB3:
	pushq	%rbp
.LCFI2:
	movq	%rsp, %rbp
.LCFI3:
	subq	$64, %rsp
.LCFI4:
	movl	%edi, -52(%rbp)
	movq	%rsi, -64(%rbp)
	movl	$10, -8(%rbp)
	movl	$xyzzy, %edi
	movl	$0, %eax
	call	getpid
	movq	xyzzy(%rip), %rdi
	call	validate_ptr
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %esi
	movl	$.LC0, %edi
	movl	$0, %eax
	call	printf
	leave
	ret
.LFE3:
	.size	main, .-main
	.comm	glob_ptr,4,4
	.comm	xyzzy,8,8
	.section	.eh_frame,"a",@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1
.LSCIE1:
	.long	0x0
	.byte	0x1
	.string	"zR"
	.uleb128 0x1
	.sleb128 -8
	.byte	0x10
	.uleb128 0x1
	.byte	0x3
	.byte	0xc
	.uleb128 0x7
	.uleb128 0x8
	.byte	0x90
	.uleb128 0x1
	.align 8
.LECIE1:
.LSFDE1:
	.long	.LEFDE1-.LASFDE1
.LASFDE1:
	.long	.LASFDE1-.Lframe1
	.long	.LFB2
	.long	.LFE2-.LFB2
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI0-.LFB2
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI1-.LCFI0
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE1:
.LSFDE3:
	.long	.LEFDE3-.LASFDE3
.LASFDE3:
	.long	.LASFDE3-.Lframe1
	.long	.LFB3
	.long	.LFE3-.LFB3
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI2-.LFB3
	.byte	0xe
	.uleb128 0x10
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI3-.LCFI2
	.byte	0xd
	.uleb128 0x6
	.align 8
.LEFDE3:
	.ident	"GCC: (GNU) 4.2.0"
	.section	.note.GNU-stack,"",@progbits

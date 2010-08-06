
Code taken out of dtrace_isa.c until we are ready to reinstate.

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
# define read_pda(x) percpu_read(x)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
bos = sp = current->thread.usersp;
#else
bos = sp = read_pda(oldrsp);
#endif
//bos = sp = (unsigned long *) task_pt_regs(current)->sp;
//bos = sp = read_pda(oldrsp);
//unsigned long pc = ((unsigned long *) read_pda(kernelstack))[0];
unsigned long pc = ((unsigned long *) sp)[0];
printk("rsp %p pc=%p\n", sp, pc);

//dtrace_dump_mem64(&current->thread, 64);
//dtrace_dump_mem64(sp, 128);
	/***********************************************/
	/*   Find  base  of  the  code  area  for ELF  */
	/*   header.				       */
	/***********************************************/
int loop = 0;
        while (pcstack < pcstack_end &&
               sp >= bos) {
	       	struct vm_area_struct *vm = find_vma(current->mm, pc);
		int	cfa_offset;
char	dw[200];
int do_dwarf_phdr(char *, char *);
int dw_find_ret_addr(char *, unsigned long, int *);

printk("loop=%d sp:%p pc=%p\n", loop++, sp, pc);
		if (vm == NULL) {
/*printk("no vm for sp: %p pc=%p\n", pc);*/
			break;
		}

		/***********************************************/
		/*   Work  out where the .eh_frame section is  */
		/*   in memory.				       */
		/***********************************************/
char *ptr = vm->vm_start;
printk("pc=%p sp=%p vmtart=%p elf=%02x %02x %02x %02x\n", pc, sp[0], vm->vm_start, ptr[0], ptr[1], ptr[2], ptr[3]);
printk("sp[0] %p %p %p\n", sp[0], sp[1], sp[2]);
printk("sp[3] %p %p %p\n", sp[3], sp[4], sp[5]);
		if (do_dwarf_phdr((char *) vm->vm_start, &dw) < 0) {
			printk("sorry - no phdr\n");
			break;
		}

		/***********************************************/
		/*   Now  process  the  CFA machinery to find  */
		/*   where  the next return address is on the  */
		/*   stack (relative to where we are).	       */
		/***********************************************/
		if (dw_find_ret_addr(dw, pc, &cfa_offset) == 0) {
			printk("sorry..\n");
			break;
		}
//cfa_offset = cfa_offset+8;
		printk("vm=%p %p: %p cfa_offset=%d\n", vm, vm->vm_start, *(long *) vm->vm_start, cfa_offset);
		*pcstack++ = sp[0];
		sp = (char *) sp + cfa_offset - 8;
		if (*sp < 4096)
			sp++;
printk("CHECK: %p %p %p\n", sp[0], sp[1], sp[2]);
		pc = *sp;
		continue;
                if (validate_ptr(sp))
                        *pcstack++ = sp[0];
                sp++;
	}


#ifndef _SYS_VMEM_H
#define _SYS_VMEM_H

//# ident   "@(#)vmem.h     1.14    07/05/11 SMI"

#ifdef  __cplusplus
extern "C" {
#endif


/*
 * Per-allocation flags
 */
#define VM_SLEEP        0x00000000      /* same as KM_SLEEP */
#define VM_NOSLEEP      0x00000001      /* same as KM_NOSLEEP */
#define VM_PANIC        0x00000002      /* same as KM_PANIC */
#define VM_PUSHPAGE     0x00000004      /* same as KM_PUSHPAGE */
#define VM_KMFLAGS      0x000000ff      /* flags that must match KM_* flags */

#define VM_BESTFIT      0x00000100
#define VM_FIRSTFIT     0x00000200
#define VM_NEXTFIT      0x00000400

typedef struct vmem vmem_t;

#ifdef  __cplusplus
}
#endif

#endif

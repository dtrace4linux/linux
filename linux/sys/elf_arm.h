#ifndef _SYS_ELF_ARM_H
#define	_SYS_ELF_ARM_H

#ifdef	__cplusplus
extern "C" {
#endif

#define R_ARM_NONE		0	/* No reloc */
#define R_ARM_PC24		1	/* PC relative 26 bit branch */
#define R_ARM_ABS32		2	/* Direct 32 bit  */
#define R_ARM_REL32		3	/* PC relative 32 bit */
#define R_ARM_PC13		4
#define R_ARM_ABS16		5	/* Direct 16 bit */
#define R_ARM_ABS12		6	/* Direct 12 bit */
#define R_ARM_THM_ABS5		7
#define R_ARM_ABS8		8	/* Direct 8 bit */
#define R_ARM_SBREL32		9
#define R_ARM_THM_PC22		10
#define R_ARM_THM_PC8		11
#define R_ARM_AMP_VCALL9	12
#define R_ARM_SWI24		13
#define R_ARM_THM_SWI8		14
#define R_ARM_XPC25		15
#define R_ARM_THM_XPC22		16
#define R_ARM_TLS_DTPMOD32	17	/* ID of module containing symbol */
#define R_ARM_TLS_DTPOFF32	18	/* Offset in TLS block */
#define R_ARM_TLS_TPOFF32	19	/* Offset in static TLS block */
#define R_ARM_COPY		20	/* Copy symbol at runtime */
#define R_ARM_GLOB_DAT		21	/* Create GOT entry */
#define R_ARM_JUMP_SLOT		22	/* Create PLT entry */
#define R_ARM_RELATIVE		23	/* Adjust by program base */
#define R_ARM_GOTOFF		24	/* 32 bit offset to GOT */
#define R_ARM_GOTPC		25	/* 32 bit PC relative offset to GOT */
#define R_ARM_GOT32		26	/* 32 bit GOT entry */
#define R_ARM_PLT32		27	/* 32 bit PLT address */
#define R_ARM_CALL 		28
#define R_ARM_ALU_PCREL_7_0	32
#define R_ARM_ALU_PCREL_15_8	33
#define R_ARM_ALU_PCREL_23_15	34
#define R_ARM_LDR_SBREL_11_0	35
#define R_ARM_ALU_SBREL_19_12	36
#define R_ARM_ALU_SBREL_27_20	37
#define R_ARM_GNU_VTENTRY	100
#define R_ARM_GNU_VTINHERIT	101
#define R_ARM_THM_PC11		102	/* thumb unconditional branch */
#define R_ARM_THM_PC9		103	/* thumb conditional branch */
#define R_ARM_TLS_GD32		104	/* PC-rel 32 bit for global dynamic
					   thread local data */
#define R_ARM_TLS_LDM32		105	/* PC-rel 32 bit for local dynamic
					   thread local data */
#define R_ARM_TLS_LDO32		106	/* 32 bit offset relative to TLS
					   block */
#define R_ARM_TLS_IE32		107	/* PC-rel 32 bit for GOT entry of
					   static TLS block offset */
#define R_ARM_TLS_LE32		108	/* 32 bit offset relative to static
					   TLS block */
#define R_ARM_RXPC25		249
#define R_ARM_RSBREL32		250
#define R_ARM_THM_RPC22		251
#define R_ARM_RREL32		252
#define R_ARM_RABS22		253
#define R_ARM_RPC24		254
#define R_ARM_RBASE		255
/* Keep this the last entry.  */
#define R_ARM_NUM		256

#if 0
#define	ELF_386_MAXPGSZ		0x10000	/* maximum page size */

#define	SHF_ORDERED	0x40000000
#define	SHF_EXCLUDE	0x80000000

#define	SHN_BEFORE	0xff00
#define	SHN_AFTER	0xff01
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ELF_ARM_H */

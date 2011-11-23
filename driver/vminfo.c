/**********************************************************************/
/*                                                                    */
/*  File:          vminfo.c                                           */
/*  Author:        P. D. Fox                                          */
/*  Created:       26 Oct 2011                                        */
/*                                                                    */
/*  Copyright (c) 2011, Foxtrot Systems Ltd                           */
/*                All Rights Reserved.                                */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  Description:  vminfo - virtual memory provider                    */
/*   This  provider  is an interface to the /proc/vmstats data - for  */
/*   each  statistic,  we  provide  a  probe  which  intercepts  the  */
/*   increment of that data item.				      */
/*--------------------------------------------------------------------*/
/*  $Header: Last edited: 26-Oct-2011 1.1 $ */
/**********************************************************************/

#include <linux/mm.h>
# undef zone
# define zone linux_zone
#include <dtrace_linux.h>
#include <sys/privregs.h>
#include <sys/dtrace_impl.h>
#include <linux/sys.h>
#undef comm /* For 2.6.36 and above - conflict with perf_event.h */
#include <sys/dtrace.h>
#include <linux/vmalloc.h>
#include <dtrace_proto.h>

#define MAX_LOCATORS 99
typedef struct find_sdt_t {
	void	*f_offset;
	char	*f_name;
	dtrace_provider_id_t f_id;
	} find_sdt_t;
static find_sdt_t sdt_locator[MAX_LOCATORS];
static int	sdt_locator_cnt;

/**********************************************************************/
/*   Compile up a list of instructions we are going to intercept. As  */
/*   we  disassemble the kernel, we will match against these entries  */
/*   to know which ones go with which probe.			      */
/**********************************************************************/
static void
sdt_add_locator(void *instr, char *name)
{
	if (sdt_locator_cnt >= MAX_LOCATORS) {
		printk("sdt_locator table too small, please resize\n");
		return;
	}

//	printk("sdt_add_locator: %p %s\n", instr, name);
	sdt_locator[sdt_locator_cnt].f_offset = instr;
	sdt_locator[sdt_locator_cnt].f_name = name;
	sdt_locator_cnt++;
}

/**********************************************************************/
/*   As   we   disassemble   the  kernel,  call  back  for  relevant  */
/*   instructions.  At the moment dtrace_parse_kernel calls back for  */
/*   the  instructions  (64b  only)  we are interested in. Later, we  */
/*   will need to either hint to it what we want or filter here.      */
/**********************************************************************/
static void
vminfo_instr_callback(uint8_t *instr, int size)
{	int	i;
	int	offset = *(int *) (instr + 5);

	for (i = 0; i < sdt_locator_cnt; i++) {
		if (sdt_locator[i].f_offset == (void *) (long) offset) {
//			printk("callback: %p name=%s\n", instr, sdt_locator[i].f_name);
			prcom_add_instruction(sdt_locator[i].f_name, instr);
			return;
		}
	}
//	printk("callback: %p offset=%x\n", instr, offset);
}
/**********************************************************************/
/*   Routine  to  set  up  the  traps  for  the  increments  on  the  */
/*   vm_event_item  structure.  <linux/vmstat.h>  contains  the enum  */
/*   definitions,  but, since these are enums, we cannot directly do  */
/*   ifdef's to protect ourselves for older kernels.		      */
/**********************************************************************/
void vminfo_init(void)
{
	/***********************************************/
	/*   Avoid  build  errors on earlier kernels.  */
	/*   Need  to figure out what vm_event_addr()  */
	/*   maps to. Be conservative for now.	       */
	/***********************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38) // temporary

#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)
#	define	vm_event_addr(x) &vm_event_states.event[x]
#  else
#	define	vm_event_addr(x) &__get_cpu_var(vm_event_states).event[x]
#  endif
	sdt_add_locator(vm_event_addr(PGPGIN), "vminfo:::pgpgin");
	sdt_add_locator(vm_event_addr(PGPGOUT), "vminfo:::pgpgout");
	sdt_add_locator(vm_event_addr(PSWPIN), "vminfo:::pswpin");
	sdt_add_locator(vm_event_addr(PSWPOUT), "vminfo:::pswpout");

	sdt_add_locator(vm_event_addr(PGALLOC_NORMAL), "vminfo:::pgalloc");
	sdt_add_locator(vm_event_addr(PGALLOC_MOVABLE), "vminfo:::pgalloc");

	sdt_add_locator(vm_event_addr(PGFREE), "vminfo:::pgfree");
	sdt_add_locator(vm_event_addr(PGACTIVATE), "vminfo:::pgactivate");
	sdt_add_locator(vm_event_addr(PGDEACTIVATE), "vminfo:::pgdeactivate");
	sdt_add_locator(vm_event_addr(PGFAULT), "vminfo:::pgfault");
	sdt_add_locator(vm_event_addr(PGMAJFAULT), "vminfo:::pgmajfault");
	sdt_add_locator(vm_event_addr(PGREFILL_NORMAL), "vminfo:::pgrefill");
	sdt_add_locator(vm_event_addr(PGREFILL_MOVABLE), "vminfo:::pgrefill");
	sdt_add_locator(vm_event_addr(PGSTEAL_NORMAL), "vminfo:::pgsteal");
	sdt_add_locator(vm_event_addr(PGSTEAL_MOVABLE), "vminfo:::pgsteal");

	sdt_add_locator(vm_event_addr(PGSCAN_KSWAPD_NORMAL), "vminfo:::pgscan_kswapd");
	sdt_add_locator(vm_event_addr(PGSCAN_KSWAPD_MOVABLE), "vminfo:::pgscan_kswapd");

	sdt_add_locator(vm_event_addr(PGSCAN_DIRECT_NORMAL), "vminfo:::pgscan_direct");
	sdt_add_locator(vm_event_addr(PGSCAN_DIRECT_MOVABLE), "vminfo:::pgscan_direct");

#ifdef CONFIG_NUMA
	sdt_add_locator(vm_event_addr(PGSCAN_ZONE_RECLAIM_FAILED), "vminfo:::pgscan_zone_reclaim_failed");
#endif
	sdt_add_locator(vm_event_addr(PGINODESTEAL), "vminfo:::pginodesteal");
	sdt_add_locator(vm_event_addr(SLABS_SCANNED), "vminfo:::slabs_scanned");
	sdt_add_locator(vm_event_addr(KSWAPD_STEAL), "vminfo:::kswapd_steal");
	sdt_add_locator(vm_event_addr(KSWAPD_INODESTEAL), "vminfo:::kswapd_inodesteal");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)
	sdt_add_locator(vm_event_addr(KSWAPD_LOW_WMARK_HIT_QUICKLY), "vminfo:::kswapd_low_wmark_hit_quickly");
	sdt_add_locator(vm_event_addr(KSWAPD_HIGH_WMARK_HIT_QUICKLY), "vminfo:::kswapd_high_wmark_hit_quickly");
	sdt_add_locator(vm_event_addr(KSWAPD_SKIP_CONGESTION_WAIT), "vminfo:::kswapd_skip_congestion_wait");
#endif
	sdt_add_locator(vm_event_addr(PAGEOUTRUN), "vminfo:::pageoutrun");
	sdt_add_locator(vm_event_addr(ALLOCSTALL), "vminfo:::allocstall");
	sdt_add_locator(vm_event_addr(PGROTATED), "vminfo:::pgrotated");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	sdt_add_locator(vm_event_addr(COMPACTBLOCKS), "vminfo:::compactblocks");
	sdt_add_locator(vm_event_addr(COMPACTPAGES), "vminfo:::compactpages");
	sdt_add_locator(vm_event_addr(COMPACTPAGEFAILED), "vminfo:::compactpagefailed");
	sdt_add_locator(vm_event_addr(COMPACTSTALL), "vminfo:::compactstall");
	sdt_add_locator(vm_event_addr(COMPACTFAIL), "vminfo:::compactfail");
	sdt_add_locator(vm_event_addr(COMPACTSUCCESS), "vminfo:::compactsuccess");
#endif

#ifdef CONFIG_HUGETLB_PAGE
	sdt_add_locator(vm_event_addr(HTLB_BUDDY_PGALLOC), "vminfo:::htlb_buddy_pgalloc");
	sdt_add_locator(vm_event_addr(HTLB_BUDDY_PGALLOC_FAIL), "vminfo:::htlb_buddy_pgalloc_fail");
#endif

	sdt_add_locator(vm_event_addr(UNEVICTABLE_PGCULLED), "vminfo:::unevictable_pgculled");
	sdt_add_locator(vm_event_addr(UNEVICTABLE_PGSCANNED), "vminfo:::unevictable_pgscanned");
	sdt_add_locator(vm_event_addr(UNEVICTABLE_PGRESCUED), "vminfo:::unevictable_pgrescued");
	sdt_add_locator(vm_event_addr(UNEVICTABLE_PGMLOCKED), "vminfo:::unevictable_pgmlocked");
	sdt_add_locator(vm_event_addr(UNEVICTABLE_PGMUNLOCKED), "vminfo:::unevictable_pgmunlocked");
	sdt_add_locator(vm_event_addr(UNEVICTABLE_PGCLEARED), "vminfo:::unevictable_pgcleared");
	sdt_add_locator(vm_event_addr(UNEVICTABLE_PGSTRANDED), "vminfo:::unevictable_pgstranded");
	sdt_add_locator(vm_event_addr(UNEVICTABLE_MLOCKFREED), "vminfo:::unevictable_mlockfreed");

#if defined(CONFIG_TRANSPARENT_HUGEPAGE) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	sdt_add_locator(vm_event_addr(THP_FAULT_ALLOC), "vminfo:::thp_fault_alloc");
	sdt_add_locator(vm_event_addr(THP_FAULT_FALLBACK), "vminfo:::thp_fault_fallback");
	sdt_add_locator(vm_event_addr(THP_COLLAPSE_ALLOC), "vminfo:::thp_collapse_alloc");
	sdt_add_locator(vm_event_addr(THP_COLLAPSE_ALLOC_FAILED), "vminfo:::thp_collapse_alloc_failed");
	sdt_add_locator(vm_event_addr(THP_SPLIT), "vminfo:::thp_split");
# endif
	dtrace_parse_kernel(vminfo_instr_callback);
# endif /* if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38) */

}

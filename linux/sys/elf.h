/*
 *      Known values for note entry types (e_type == ET_CORE)
 */

#define NT_PRSTATUS     1       /* prstatus_t   <sys/old_procfs.h>      */
#define NT_PRFPREG      2       /* prfpregset_t <sys/old_procfs.h>      */
#define NT_PRPSINFO     3       /* prpsinfo_t   <sys/old_procfs.h>      */
#define NT_PRXREG       4       /* prxregset_t  <sys/procfs.h>          */
#define NT_PLATFORM     5       /* string from sysinfo(SI_PLATFORM)     */
#define NT_AUXV         6       /* auxv_t array <sys/auxv.h>            */
#define NT_GWINDOWS     7       /* gwindows_t   SPARC only              */
#define NT_ASRS         8       /* asrset_t     SPARC V9 only           */
#define NT_LDT          9       /* ssd array    <sys/sysi86.h> IA32 only */
#define NT_PSTATUS      10      /* pstatus_t    <sys/procfs.h>          */
#define NT_PSINFO       13      /* psinfo_t     <sys/procfs.h>          */
#define NT_PRCRED       14      /* prcred_t     <sys/procfs.h>          */
#define NT_UTSNAME      15      /* struct utsname <sys/utsname.h>       */
#define NT_LWPSTATUS    16      /* lwpstatus_t  <sys/procfs.h>          */
#define NT_LWPSINFO     17      /* lwpsinfo_t   <sys/procfs.h>          */
#define NT_PRPRIV       18      /* prpriv_t     <sys/procfs.h>          */
#define NT_PRPRIVINFO   19      /* priv_impl_info_t <sys/priv.h>        */
#define NT_CONTENT      20      /* core_content_t <sys/corectl.h>       */
#define NT_ZONENAME     21      /* string from getzonenamebyid(3C)      */


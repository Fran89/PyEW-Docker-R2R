
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: reboot_mss_ew.h 3762 2010-01-02 00:39:51Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2010/01/02 00:39:51  kress
 *     bookeeping on gcf2ew.  reboot_mss and reboot_mss_ew now both work in linux.
 *
 *     Revision 1.3  2004/06/25 18:27:27  dietz
 *     modified to work with TYPE_TRACEBUF2 and location code
 *
 *     Revision 1.2  2001/04/27 00:57:27  kohler
 *     New parameter: Logout
 *
 *     Revision 1.1  2001/04/26 17:50:05  kohler
 *     Initial revision
 *
 *
 *
 */
/******************************************************************
 *                      File reboot_mss_ew.h                      *
 ******************************************************************/

#include <time.h>
#include <trace_buf.h>

typedef struct {
  char   sta[TRACE2_STA_LEN];   /* Station name */
  char   comp[TRACE2_CHAN_LEN]; /* Component */
  char   net[TRACE2_NET_LEN];   /* Network */
  char   loc[TRACE2_LOC_LEN];   /* Location */
  char   mss_ip[16];            /* IP address of MSS100 */
  char   mss_apwd[16];          /* initial access password of MSS100 */
  char   mss_ppwd[16];          /* Priviledged user password of MSS100 */
  time_t gapStart;              /* time of previous message for this SCNL */
  int    reboot_active;         /* 1 if reboot active; 0 otherwise */
#ifdef _WINNT
   HANDLE hProcess;              /* Process handle */
#endif
#ifdef _SOLARIS
   pid_t  pid;                   /* Process id */
#endif
#ifdef _LINUX
   pid_t  pid;                   /* Process id */
#endif
} SCNL;

typedef struct {
   char InRing[MAX_RING_STR];   /* Name of ring containing tracebuf messages */
   char MyModName[MAX_MOD_STR]; /* module name */
   long InKey;                  /* Key to ring where waveforms live */
   int  nSCNL;                  /* Number of SCNL's in the config file */
   int  HeartbeatInt;           /* Heartbeat interval in seconds */
   int  RebootGap;              /* Gaps longer than this value (sec) cause an MSS reboot */
   char ProgName[80];           /* Name of stand-alone reboot program */
   int  Logout;                 /* If 0, reboot MSS.  If 1, logout MSS100 serial port */
} GPARM;


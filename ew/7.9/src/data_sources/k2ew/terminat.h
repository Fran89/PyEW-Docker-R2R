/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: terminat.h 796 2001-10-19 18:22:57Z kohler $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2001/10/19 18:22:57  kohler
 *     Updated comments.  No code change.
 *
 *     Revision 1.5  2000/11/07 19:39:14  kohler
 *     Defined new symbol: K2STAT_GPSLOCK
 *
 *     Revision 1.4  2000/08/30 17:34:00  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.3  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.2  2000/05/16 23:39:16  lombard
 *     bug fixes, removed OutputThread Keepalive, added OnBattery alarm
 *     made alarms report only once per occurence
 *
 *     Revision 1.1  2000/05/04 23:48:43  lombard
 *     Initial revision
 *
 *
 *
 */
/*  terminat.h:  Header file for 'terminat.c' -- 3/17/99 -- [ET] */

#ifndef TERMINAT
#define TERMINAT

/* status/termination codes which map into 'k2ew.desc': */
#define K2TERM_UNKNOWN 0          /* unknown termination code */
#define K2TERM_K2_STARTUP 1       /* Error during K2 configuration/startup */
#define K2TERM_K2_COMMERR 2       /* Error or timeout reading from K2 */
#define K2TERM_K2_CIRBUFF 3       /* Error writing to K2 circular data buffer */
#define K2TERM_SIG_TRAP 4         /* SIGNAL caused k2ew to terminate */
#define K2TERM_EW_PUTMSG 5        /* EW tport_putmsg() failed */
#define K2TERM_EW_TERM 6          /* EW TERMINATE received */
#define K2TERM_NUMENTS 6          /* number of possible termination codes */

#define K2STAT_ON_BATT 7          /* lost external power, K2 on battery */
#define K2STAT_OFF_BATT 8         /* External power restored */
#define K2STAT_LOW_BATT 9         /* low battery voltage */
#define K2STAT_LOW_EXT 10         /* low external voltage */
#define K2STAT_OK_EXT 11          /* External voltage OK */
#define K2STAT_HW_FAULT 12        /* K2 hardware fault */
#define K2STAT_HWFLT_CLR 13       /* hardware fault is clear */
#define K2STAT_LOW_DISK 14        /* low free disk space */
#define K2STAT_BAD_DISK 15        /* Disk R/W error or disk reports `not ready' */
#define K2STAT_OK_DISK 16         /* Disk status now OK */
#define K2STAT_LOW_TEMP 17        /* low temperature */
#define K2STAT_HIGH_TEMP 18       /* high temperature */
#define K2STAT_OK_TEMP 19         /* temperature within limits */
#define K2STAT_RESTART 20         /* K2 has restarted */
#define K2STAT_GPSLOCK 21         /* GPS not synched to satellites */

/* set code and message for 'statmgr' on exit */
void k2ew_enter_exitmsg(int termcode,const char *fmtstr,...);
void k2ew_exit(int);        /* exit program; with 'statmgr' msg & cleanup */

#endif

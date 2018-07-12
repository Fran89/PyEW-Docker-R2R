/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2ewmain.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.41  2007/12/18 13:36:03  paulf
 *     version 2.43 which improves modem handling for new kmi firmware on k2 instruments
 *
 *     Revision 1.40  2007/05/15 23:29:24  dietz
 *     Changed version to 2.42 to reflect previous Windows Service-friendly mods
 *
 *     Revision 1.39  2007/05/10 00:14:50  dietz
 *     wrapped all SIGBREAK statements in #ifdef SIGBREAK for solaris
 *
 *     Revision 1.38  2007/05/09 23:56:27  dietz
 *     Added a CtrlHandler (Windows only) to catch/ignore user logoff events
 *     so that k2ew console windows will survive user logouts when Earthworm
 *     is started with startstop_service. Also modified to log a descriptive
 *     message instead of a numeric value for handled signals.
 *
 *     Revision 1.37  2005/09/15 13:56:19  friberg
 *     version 2.41 of k2ew has logging messages cleaned up
 *
 *     Revision 1.36  2005/07/27 19:28:49  friberg
 *     2.40 changes for ForceBlockMode and comm stats
 *
 *     Revision 1.35  2005/05/27 15:04:59  friberg
 *     new version 2.39 fixes restart file issue with netcode and location codes
 *
 *     Revision 1.34  2005/03/26 00:17:46  kohler
 *     Version 2.38.  Added capability to get network code values from the K2
 *     headers.  The "Network" parameter in the config file is now optional.
 *     WMK 3/25/05
 *
 *     Revision 1.33  2005/03/25 00:38:01  kohler
 *     Now logging chan/net/loc codes from K2 header.  WMK
 *
 *     Revision 1.32  2005/01/04 22:03:18  friberg
 *     comments for version 2.33 fixed
 *
 *     Revision 1.31  2004/06/04 16:53:44  lombard
 *     Tweaked some startup message formats.
 *
 *     Revision 1.30  2004/06/03 23:30:00  lombard
 *     Added printing of LocationNames during startup
 *
 *     Revision 1.29  2004/06/03 16:54:01  lombard
 *     Updated for location code (SCNL) and TYPE_TRACEBUF2 messages.
 *
 *     Revision 1.28  2003/08/22 20:12:13  friberg
 *     added check for terminate flag to redo_socket in k2c_tcp.c
 *     to prevent k2ew_tcp from taking too long to exit when in this
 *     function call and a terminate signal arrives.
 *
 *     Revision 1.27  2003/07/17 19:23:27  friberg
 *     Fixed a status message to include network and station name for the k2
 *     from which the error originated.
 *
 *     Revision 1.26  2003/06/06 01:24:13  lombard
 *     Changed to version 2.34: fix for byte alignment problem in extended status
 *     structure.
 *
 *     Revision 1.25  2003/05/29 13:33:40  friberg
 *     see release notes, added new directive InjectInfo and cleaned
 *     up exit messages at startup
 *
 *     Revision 1.24  2003/05/15 00:42:45  lombard
 *     Changed to version 2.31: fixed handling of channel map.
 *
 *     Revision 1.23  2002/05/06 18:29:15  kohler
 *     Changed version from 2.29 to 2.30.
 *
 *     Revision 1.22  2002/04/30 18:15:44  lucky
 *     Fixed version string to 2.29
 *
 *     Revision 1.21  2002/01/30 14:28:31  friberg
 *     added robust comm recovery for time out case
 *
 *     Revision 1.20  2002/01/17 22:02:32  kohler
 *     Changed version to 2.27
 *
 *     Revision 1.19  2002/01/17 21:57:20  kohler
 *     Changed type of g_mt_working and g_ot_working to volatile.
 *
 *     Revision 1.18  2001/10/19 18:23:49  kohler
 *     Changed version string to "2.26".  Updated comments.
 *
 *     Revision 1.17  2001/10/16 22:03:56  friberg
 *     Upgraded to version 2.25
 *
 *     Revision 1.16  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.14  2001/05/23 00:19:28  kohler
 *     New config parameter: HeaderFile
 *
 *     Revision 1.13  2001/05/08 23:16:14  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or g_pidval.
 *
 *     Revision 1.12  2001/05/08 00:13:07  kohler
 *     Minor logging changes.
 *
 *     Revision 1.11  2001/04/23 20:18:53  friberg
 *     Added station name remapping using StationID parameter.
 *
 *     Revision 1.10  2000/11/28 01:12:20  kohler
 *     Cosmetic changes to log file format.
 *
 *     Revision 1.9  2000/11/07 19:41:29  kohler
 *     Added new global variable: gcfg_gpslock_alarm
 *
 *     Revision 1.8  2000/08/30 17:34:00  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.7  2000/08/12 18:10:04  lombard
 *     Fixed bug in cb_check_waits that caused circular buffer overflows
 *     Added cb_dumb_buf() to dump buffer indexes to file for debugging
 *
 *     Revision 1.6  2000/07/28 22:36:10  lombard
 *     Moved heartbeats to separate thread; added DontQuick command; removed
 *     redo_com() since it doesn't do any good; other minor bug fixes
 *
 *     Revision 1.5  2000/07/03 18:00:37  lombard
 *     Added code to limit age of waiting packets; stops circ buffer overflows
 *     Added and Deleted some config params.
 *     Added check of K2 station name against restart file station name.
 *     See ChangeLog for complete list.
 *
 *     Revision 1.4  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.3  2000/05/16 23:39:16  lombard
 *     bug fixes, removed OutputThread Keepalive, added OnBattery alarm
 *     made alarms report only once per occurence
 *
 *     Revision 1.2  2000/05/09 23:59:07  lombard
 *     Added restart mechanism
 *
 *     Revision 1.1  2000/05/04 23:48:19  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2ewmain.c:  Main (top-level) module for K2-to-Earthworm          */
/*                                                                    */
/*     k2ew = K2-to-Earthworm Interface Module                        */
/*                                                                    */
/*     COPYRIGHT 1998, 1999: Instrumental Software Technologies, Inc. */
/*     ALL RIGHTS RESERVED. Please contact ISTI for use of this code. */
/*     It is free to all academic institutions and federal government */
/*     agencies.                                                      */
/*                                                                    */
/*     Author:  Eric Thomas                                           */
/*     Contact: support@isti.com                                      */
/*                                                                    */
/*   3/29/99 -- [ET]  Version 1.00 release                            */
/*   3/15/00 -- Pete Lombard: mods to allow socket and Unix serial    */
/*                comms.                                              */
/*                                                                    */
/*   3/22/2001 -- Paul Friberg Version 2.11 - added in Station Name   */
/*              remapping feature(what is it with the month of March?) */
/*
     7/22/2001 -- [ET]  Version 2.20:  Improved handling of resend
         requests (more focus on oldest waiting packet); added commands
         "MaxBlkResends", "MaxReqPending", "ResumeReqVal",  "WaitResendVal",
         "RestartComm"; improved debug log output messages; added logging of
         K2 channel names; changed "# of packets lost" value so that it is
         only the total after a restart (to be consistent with the "# of
         packets received OK" total); added "Program start time", "# of
         packet retries", "Packet error rating" and "Time of last output
         seq err" to the summary listings; added logging of "in-process"
         summary listing at each status output interval.

     7/23/2001 -- [ET]  Version 2.22:  Changed default value for
         'MaxReqPending' command from 10 to 6 (as per recommendation
         of Dennis Pumphrey).

      8/7/2001 -- [ET]  Version 2.23:  Added "ChannelNames" and
         "InvPolFlags" parameters.

      8/8/2001 -- [ET]  Version 2.24:  Changed so that the "ChannelNames"
         remapping occurs even when a restart file is used; made handling
         of station names in the code more straightforward.

     8/22/2001 -- [ET]  Version 2.25:  Fixed "# of packets lost" total
         displayed so that it works correctly after a restart file has
         been used.

     10/19/2001 -- Will Kohler  Version 2.26:  If a pcdrv_raw_read or
         pcdrv_raw_write error message is received from the K2, the
         program now sends a K2STAT_BAD_DISK message to statmgr.

     01/17/2002 -- Will Kohler  Version 2.27:  Changed types of variables
         g_mt_working and g_ot_working to "volatile".

      1/29/2002 -- [ET]  Version 2.28:  Improved recovery after timeout by
         adding call to 'k2mi_init_blkmde()' to attempt to restore block
         mode in the K2 (needed if modem took over K2's I/O).

      5/6/2002  -- Will Kohler  Version 2.30:  Added a line to function
         k2c_init_io() in file k2c_tcp.c so that heartbeats are sent to
         statmgr while k2ew is attempting to make a socket connection to
         the K2.

      5/13/2003 -- [PNL] Version 2.31: Changed parsing of the channel bitmask 
        from STREAM_K2_RW_PARMS.TxChanMap  instead of 
	MISC_RW_PARMS.channel_bitmap, which is the acquisition mask.
	Added ability to handle 40-byte extended status packet in addition
	to old 12-byte extended status packet.

      05/26/2003 Paul Friberg Version 2.32: cleaned up GPS lock error 
	message to echo network and station name (previously it was impossible 
	to tag this message to any particular K2). Also added network code to 
	all error messages emanating from a K2. They now all read: 
		"K2 <NN-SSSS>: error/warn message"
        All changes made in k2misc.c                    

      05/27/2003 Paul Friberg Version 2.33: added in k2 info packets for
	status monitoring of K2's. Only tested on Solaris. New config
	item in .d file is InjectInfo (defaults to OFF if not present).
	This new feature is for use in SeisNetWatch to monitor parameters
	of K2s. K2 parameters and status packets are injected into the
	wavering as TYPE_K2INFO and this message should be configured as
	a local message in the earthworm.d file.
	
	Also fixed first three calls to k2ew_enter_exitmsg() which sent out
	status messages without the station name...because comms were not
	yet established with the K2. I added the config file name in there
	since that usually has some station specific moniker in it.


      06/05/2003 Pete Lombard Version 2.34: added code to handle byte
	alignment problems with extended status packet, in k2misc.c	

      07/17/2003 Paul Friberg Version 2.35: fixed a message about 
	write and read errors that was getting sent to the status
	manager without any information about the K2 network/station
	name that it was corresponding to.

      08/22/2003 Paul Friberg Version 2.36: modified the redo_socket()
	call to more properly obey the SIGTERM signal. This was hanging
	up the restart caused by startstop_solaris and causing k2ew_tcp
	under Solaris to go into a Zombie state. The fix was to look
	at the g_terminate flag and if set, exit with an error. Previously
	the program would continue on for another try if the g_dontquit
	flag was set!

      06/02/2004 Pete Lombard Version 2.37: added support for location
        code (SCNL) and TYPE_TRACEBUF2 messages. Location codes are
	specified in the LocationNames config parameter, similar to
	ChannelNames. Added LCFlag to control the action taken on missing
	location or channel names.

      3/25/05 Will Kohler Version 2.38: K2ew will now, optionally,
        obtain network code from the K2 headers, rather than from the
        configuration file.  For this to work, network codes need to
        be entered into the K2 using Altus File Assistant. A different
        network code may be entered for each stream.  The K2 must be
        running application firmware version 3.02 or later.
        The "Network" parameter, in the config file, is now optional.

	2005-05-27 Paul Friberg Version 2.39: fixed previous version for issues
        with the restart file. If the restart file was being used, then
        the above network code assignment was not taking. The change 
        was to add network and location code to the line, instead of 
        just recording the Channel name in the restart file. The old
        Channel identifier is retired and is replaced by NCL. If
        the restart file Channel line is encountered, it will cause 
        k2ew to exit (this will alert to problems when k2ew is 
        upgraded to this new version). The fix will be to remove any
        restart files before upgrading.
        THIS VERSION ONLY COMPILED on SOLARIS by Paul Friberg
	Will Kohler tested the above version on Windows

	2005-06-01 Paul Friberg Version 2.40: added in a new directive
	ForceBlockMode to wrest control from the modem for K2s configured
	with both a modem and a serial data stream feed. This feature is
	off by default, but can be turned on by setting ForceBlockMode 1
	inside the configuration file.  Also added new communications
	statistics message for pickup by k2ewagent.

	2005-09-15 Paul Friberg Version 2.41: cleaned up all of the
	exit messages that could get emailed to a user. The messages now
	all indicate the station name or the config filename so that 
	the K2 having the error can be identified.

	2007-12-17 Paul Friberg Version 2.43: new dual feed k2 with
	modems improvements. This requires a new firmware upgrade
	from KMI that was completed in August of 2007.

	2015-12-10 Paul Friberg Version 2.44: modified SNW injection to
        send actual SNW messages as strings as injection messages. The
        prior versions sent raw K2 packets.
 
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <earthworm.h>       /* Earthworm main include file */
#include <time.h>
#include <transport.h>       /* Earthworm shared-memory transport routines */
#include "time_ew.h"         /* Earthworm time conversion routines */
#include "k2comif.h"         /* K2 communication interface routines */
#include "k2pktio.h"         /* K2 packet send/receive routines */
#include "k2pktman.h"        /* K2 packet management and SDS functions */
#include "byteswap.h"        /* integer byte-swap macros */
#include "k2ewerrs.h"        /* K2-to-Earthworm error codes */
#include "k2cirbuf.h"        /* K2 circular buffer routines */
#include "k2misc.h"          /* miscellaneous K2 functions */
#include "k2info.h"          /* K2 info functions */
#include "k2ewrstrt.h"
#include "getconfig.h"       /* command configuration file processing fn */
#include "outptthrd.h"       /* 'output' thread function declaration */
#include "terminat.h"        /* program termination defines and functions */
#include "heartbt.h"         /* 'heartbeat' thread function declaration */
#include "glbvars.h"         /* externs to globals & 'K2_MAX_STREAMS' define */

#define K2EW_VERSION_STR "2.45 2016-07-07" /* version string for program */

#define K2_STMDEBUG_FLG 0       /* 1 for stream input debug display enabled */

#define K2_NUM_RETRIES    3     /* # of times to retry failed commands */
#define K2_PING_COUNT     10    /* # of pings to send/recv for test */
/* #define K2_WAITFOR_MS     5000  * max # of msec to wait for start of data */
/* #define K2_CBENTS_PERSTM  2000  * # of circ buff entries per stream */
/* #define K2_PKTAHEAD_LIM   30    * max # of packets "ahead" allowed */
/* #define K2_MAX_WAITENTS   30    * max # of "waiting" entries allowed */
/* #define K2_MAX_BLKRESENDS 2     * max # resend requests per block */
/* #define K2_WAIT_RESENDVAL 50    * # data seq between resend reqs (was 20) */
/* #define K2_MAX_REQ_PEND   3     * max number of pending resend requests */

/* macro calculates, for the given packet stream and data sequence numbers, */
/*  an index value relative to the current packet position */
#define K2M_PCKT_RELIDX(stm,dseq) \
   (((int32_t)(dseq)-(int32_t)cur_dataseq)*g_numstms+(unsigned char)(stm)-cur_stmnum)

/* Internal function prototypes */
void k2ew_signal_hdlr(int signum);/* function to handle "signal" events */
void k2ew_signal_string(int signum, char *str, int len); 
                                  /* provide useful string for signal logging */
void k2ew_log_cfgparams(void);    /* output config values to log file */

static int k2ew_signal_val=-1;    /* initialize value from signal function */

char g_progname_str[]="K2EW";     /* global program name string */
int g_terminate_flg = 0;          /* = 1 to initiate program termination */
int g_numstms = 0;                /* number of data streams coming in */
int g_smprate = 0;                /* sample rate (& entries per SDS pkt) */
char g_k2_stnid[STN_ID_LENGTH+1];      /* station-ID string read from K2 */
char g_stnid[STN_ID_LENGTH+1];         /* station-ID string in use */
uint32_t g_pktout_okcount=0L;     /* # of packets sent to Earthworm OK */
uint32_t g_seq_errcnt=0L;         /* number of output sequence errors */
uint32_t g_skip_errcnt=0L;        /* number of packets skipped */
uint32_t g_trk_dataseq=0L;        /* output thread tracked data seqence */
unsigned char g_trk_stmnum=(unsigned char)0; /* outputthread tracked stream */
time_t g_seq_errtime = (time_t)0; /* time of last output sequence error */
int g_validrestart_flag = 0;      /* 1 if restart file is valid */
int g_extstatus_avail = 1;        /* set to 0 if extended status fails */



SHM_INFO g_tport_region;          /* transport region struct for Earthworm */
MSG_LOGO g_heartbeat_logo;        /* Transport logo for heartbeat messages */
MSG_LOGO g_error_logo;            /* Transport logo for error messages */
unsigned char g_heart_ltype = 0;  /* logo type for heartbeat msgs */
unsigned char g_error_ltype = 0;  /* logo type for error messages */
unsigned char g_trace_ltype = 0;  /* logo type value for trace messages */
unsigned char g_instid_val = 0;   /* installation ID value from/for EW */
volatile int g_mt_working = 1;    /* =1 says main thread is working */
volatile int g_ot_working = 0;    /* =1 says output thread is working */
unsigned g_output_threadid=(unsigned)-1;    /* ID for output thread */
unsigned g_hrtbt_threadid=(unsigned)-1;     /* ID for heartbeat thread */
pid_t g_pidval = 0;                 /* Our PID for heartbeat messages */
int g_wait_count = 0;             /* number of "wait" blocks in buffer */
int g_req_pend = 0;               /* number of resend requests pending */

/* array of channel ID strings, 1 for each logical stream expected */
char g_stmids_arr[K2_MAX_STREAMS][CHANNEL_ID_LENGTH+1];
/* array of network code strings, 1 for each logical stream expected */
char g_netcode_arr[K2_MAX_STREAMS][K2_NETBUFF_SIZE+2];

/* global configuration variables set in 'getconfig.c': */
s_gen_io gen_io;                       /* general IO params structure */
char gcfg_module_name[MAXNAMELEN+2]="";     /* module name for k2ew */
unsigned char gcfg_module_idnum=0;     /* module id for k2ew */
char gcfg_ring_name[MAXNAMELEN+2]="";  /* specified name of ring buffer */
int32_t gcfg_ring_key=0L;              /* key to ring buffer k2ew dumps data */
int gcfg_heartbeat_itvl = 30;          /* heartbeat interval (secs) */
int gcfg_logfile_flgval=1;             /* output to logfile enable flag */
char gcfg_network_buff[K2_NETBUFF_SIZE+2]="";  /* network name buffer */
int gcfg_base_pinno = 0;               /* pin number for stream 0 */
int gcfg_status_itvl = 1800;           /* status interval (seconds) */
int gcfg_commtimeout_itvl = 5000;      /* communication timeout (millisecs) */
int gcfg_pktwait_lim = 60;             /* Maximum time to wait for packet */
int gcfg_max_blkresends = 4;           /* max # resend requests per block */
int gcfg_max_reqpending = 6;           /* max # of pending resend requests */
int gcfg_resume_reqval = 2;            /* # blks needed to resume requests */
int gcfg_wait_resendval = 20;          /* # data seq between resend reqs */
int gcfg_restart_commflag = 0;         /* 1 to restart comm after timeout */
int gcfg_ext_status = 0;               /* flag to get extended status msgs */
int gcfg_on_batt = 0;                  /* flag for sending `on battery' msg */
int gcfg_battery_alarm = -1;           /* battery low-voltage alarm, 0.1 V */
int gcfg_extern_alarm = -1;            /* External low-voltage alarm, 0.1 V; not used */
double gcfg_disk_alarm[2] = {-1.0, -1.0}; /* disk A space low alarm, bytes */
int gcfg_hwstat_alarm = 0;             /* hardware status alarm flag */
int gcfg_templo_alarm = -1000;         /* low temperature alarm, 0.1 deg C */
int gcfg_temphi_alarm = 1000;          /* high temperature alarm, 0.1 deg C */
double gcfg_gpslock_alarm = -1.0;      /* report if no GPS synch for this many hours */
char gcfg_restart_filename[MAXNAMELEN+2] = "";   /* Restart file name */
char gcfg_header_filename[MAXNAMELEN+2]  = "";   /* K2 header file name */
int gcfg_restart_maxage = 0;           /* Maximum age of restart file in sec */
char gcfg_stnid_remap[STN_ID_LENGTH+1];  /* remapping value for station-ID */
                   /* array of "ChannelNames" remapping name strings: */
char *gcfg_chnames_arr[K2_MAX_STREAMS];
                   /* array of invert-polarity flags; 1 per channel: */
int gcfg_invpol_arr[K2_MAX_STREAMS];
                   /* array of "LocationNames": */
char *gcfg_locnames_arr[K2_MAX_STREAMS];
int gcfg_lc_flag = 0;                  /* =1 to use "--" if no loc code for K2 chan;
					  =2 to die if no loc code for K2 chan */
int gcfg_dont_quit = 0;                /* =0 if we should quit on timeout */
int gcfg_inject_info = 0;	       /* inject K2INFO packets into ring  if == 1 */
int gcfg_force_blkmde = 0;	       /* force a K2 into block mode at startup, for troublesome modem connected K2's */
int gcfg_debug = 0;                    /* k2ew debug level */


/* Put whole function here to avoid having to ifdef a prototype. */
#ifdef _WINNT
BOOL CtrlHandler( DWORD fdwCtrlType )
{
  switch( fdwCtrlType )
  {
    case CTRL_LOGOFF_EVENT:  /* intercept user logoff events with no action */
      return(TRUE);          /* because we want k2ew to keep running!       */

    default:
      return(FALSE);         /* pass other events to other signal handlers */
  }
}
#endif /* _WINNT */

/**************************************************************************
 * main                                                                   *
 **************************************************************************/

int main(int argc,char **argv)
{
  static int count;
  static int rc,cb_rc;
  static int ext_size;
  static int numacq_chan;
  static int dcount,wait_resenditvl,numticks,idnum,resendcnt,rr_resendcnt,
                                             resume_numpend,rerequest_idnum;
  static int no_req = 0, timeout_logged = 0;
  static int ch_cfg_err = 0;
  static uint32_t cur_dataseq,dataseq,rr_dataseq,msk,strmmsk,
                  acqmsk, rstrt_dataseq =0, last_sec = 0;
  static int32_t idx,c;
  static uint32_t pktin_okcount,missing_errcnt,unexpect_errcnt,
                  request_errcnt,resync_errcnt,packet_errcnt,
                  receive_errcnt,retried_pckcnt,last_pktin_okcount,
                  last_pktout_okcount,last_seq_errcnt,
                  last_retried_pckcnt,last_missing_errcnt,comm_retries;
  static unsigned char seqnum,cur_stmnum,stmnum,rr_stmnum,
                       rstrt_stmnum = 0;
  static K2_HEADER k2hdr;
  static struct STATUS_INFO k2stat;
  static struct EXT2_STATUS_INFO k2extstat;
  static struct StrDataHdr datahdr;
  static int32_t databuff[K2PM_MIN_DBUFSIZ+2];
  static time_t prog_start_time, timevar, last_stat_time = (time_t)0; 
  static time_t last_force_block, last_attempted_force_block, now;
  static char msg_txt[180];
  static struct tm tmblk;


  time(&prog_start_time);         /* save program start time */
  gcfg_stnid_remap[0]='\0';
  g_k2_stnid[0] ='\0';
  last_force_block = 0;
  last_attempted_force_block = 0;

  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s configfile\n VERSION %s\n", argv[0], K2EW_VERSION_STR);
    exit (EW_FAILURE);
  }

  if (get_config(argv[1]) == -1)  /* process configuration file */
  {
    fprintf(stderr, "k2ew: error processing config file <%s>\n", argv[1]);
    return 1;    /* if error then exit program (error reported in function) */
  }

         /* calculate number of pending waiting blocks that must be  */
         /*  gotten down to before resend requests are resumed after */
         /*  'MaxReqPending' has been reached:                       */
  if((resume_numpend=gcfg_max_reqpending-gcfg_resume_reqval) < 0)
    resume_numpend = 0;           /* if negative then make it zero */

  /* initialize output log file */
  logit_init(argv[1],(short)gcfg_module_idnum, 1024, gcfg_logfile_flgval);
  logit("et","K2-to-Earthworm Module, Version %s\n",K2EW_VERSION_STR);
  logit("et","with SCNL and TRACEBUF2 messages\n");
  logit("et","Processed configuration file <%s>\n",argv[1]);

  k2ew_log_cfgparams();      /* output configuration values to log file */

  /* attach to Earthworm shared memory ring buffer */
  /*  (fn will display error message and abort if error) */
  tport_attach(&g_tport_region,gcfg_ring_key);
  logit("et","Attached to ring key:  %ld\n",gcfg_ring_key);

  /* setup logo type values for Eartworm messages */
  if(GetType("TYPE_ERROR",&g_error_ltype) != 0 ||
     GetType("TYPE_HEARTBEAT",&g_heart_ltype) != 0 ||
     GetType("TYPE_TRACEBUF2",&g_trace_ltype) != 0)
  {      /* error fetching logo type values; show error message and abort */
    sprintf (msg_txt, "Error fetching logo type values for EW messages: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                 /* exit program with code value */
  }


  /* get installation ID value */
  if(GetLocalInst(&g_instid_val) != 0)
  {      /* error fetching installation ID value; show error message & abort */
    sprintf (msg_txt, "Error fetching installation ID value: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                 /* exit program with code value */
  }


  /* setup things for heartbeat messages */
  g_pidval = getpid();
  g_heartbeat_logo.type = g_heart_ltype;
  g_heartbeat_logo.mod = gcfg_module_idnum;
  g_heartbeat_logo.instid = g_instid_val;

  /* setup things for error messages */
  g_error_logo.type = g_error_ltype;
  g_error_logo.mod = gcfg_module_idnum;
  g_error_logo.instid = g_instid_val;

  g_mt_working = 1;
  /* startup the heartbeat thread */
  if (StartThread(k2ew_heartbeat_fn, (unsigned)0, &g_hrtbt_threadid) == -1)
  {
    sprintf (msg_txt, "Unable to startup heartbeat thread--aborting: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                 /* exit program with code value */
  }

  if (gcfg_restart_filename[0] != '\0')
    k2ew_read_rsfile(&g_validrestart_flag, &rstrt_dataseq, &rstrt_stmnum);
  if (gcfg_debug > 1)
    logit("et", "restart flag: %d\n", g_validrestart_flag);

  /* Initialize IO to the K2: sockets or serial comms */
  if ( (rc = k2c_init_io( &gen_io, gcfg_commtimeout_itvl )) != K2R_NO_ERROR)
  {      /* comms initialization failed */
    sprintf (msg_txt, "Unable to initialize IO port: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                  /* exit program with code value */
  }
  if (gcfg_debug > 0)
    logit("et","Initialized IO port\n");

  /* get comm link to K2 going */
  if ( gcfg_force_blkmde ) 
  {
     if(k2mi_force_blkmde() < K2R_NO_ERROR)
     {      /* error code returned; show error message and exit program */
       sprintf (msg_txt, "Unable to force block mode with K2: %s",argv[1]);
       k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
       k2ew_exit(0);                  /* exit program with code value */
     }
     logit("et","Forced block mode with K2\n");
     time(&last_force_block);
  }

  seqnum = (unsigned char)0;       /* initialize K2 packet sequence number */
  if(k2mi_init_blkmde(&seqnum) < K2R_NO_ERROR)
  {      /* error code returned; show error message and exit program */
    sprintf (msg_txt, "Unable to establish communications link with K2: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                  /* exit program with code value */
  }
  logit("et","Established communications link with K2\n");

  /*  if (init_rc != K2R_POSITIVE)
      g_validrestart_flag = 0;  */   /* Commented out: try using any K2
                                         response for valid restart */

  if (g_validrestart_flag == 0)
  {
    /* stop streaming and acquiring */
    logit("et", "Initializing K2 stream control...\n");
    count = K2_NUM_RETRIES;        /* initialize retry count */
    while( (rc = k2pm_stop_streaming(&seqnum, 1)) < K2R_NO_ERROR)
    {      /* loop while function fails */
      if (--count <= 0)
      {         /* too many attempts; indicate failure */
        sprintf (msg_txt, "Unable to initialize K2 stream control: %s",argv[1]);
        k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
        k2ew_exit(0);                /* exit program with code value */
      }
    }
    if (gcfg_debug > 0)
      logit("et","Initialized K2 stream control\n");

    /* test COM link */
    logit("et", "Testing K2 communications link...");
    count = K2_NUM_RETRIES;        /* initialize retry count */
    while ( (rc = k2mi_ping_test(K2_PING_COUNT,&seqnum)) != K2R_NO_ERROR)
    {      /* loop while ping test fails */
      if (--count <= 0)
      {         /* too many attempts; indicate failure */
        sprintf (msg_txt, "Error testing K2 communications links: %s",argv[1]);
        k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
        k2ew_exit(0);                /* exit program with code value */
      }
    }
    logit("e", "OK\n");

    /* get K2 header parameters; needed to configure this program */
    if (gcfg_debug > 0)
       logit("et", "Reading K2 header parameters...\n");
    count = K2_NUM_RETRIES;        /* initialize retry count */
    while ( (rc = k2mi_get_params(&k2hdr, &seqnum)) != K2R_POSITIVE)
    {      /* loop while read command fails */
      if (--count <= 0)
      {         /* too many attempts; indicate failure */
        sprintf (msg_txt, "Error reading K2 parameters: %s",argv[1]);
        k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
        k2ew_exit(0);                /* exit program with code value */
      }
    }

    /* get K2 status */
    if (gcfg_debug > 0)
      logit("et", "Reading K2 status...\n");
    count = K2_NUM_RETRIES;        /* initialize retry count */
    while ( (rc = k2mi_get_status(&k2stat,&seqnum)) != K2R_POSITIVE)
    {      /* loop while read command fails */
      if (--count <= 0)
      {         /* too many attempts; signal failure */
        sprintf (msg_txt, "Error reading K2 status: %s",argv[1]);
        k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
        k2ew_exit(0);                /* exit program with code value */
      }
    }
    /* get K2 extended status */
    if (gcfg_ext_status != 0)
    {
      logit("et", "Reading K2 extended status...\n");
      count = 1;        /* initialize retry count */
      while ( (rc = k2mi_get_extstatus(&k2extstat, &seqnum, &ext_size)) 
	      != K2R_POSITIVE)
      {      /* loop while read command fails */
        if (gcfg_debug > 3)
          logit("e", "k2mi_get_extstatus returned %d\n", rc);
        if (--count <= 0)
        {         /* too many attempts; signal failure */
          if (rc == K2R_NO_ERROR)
          {
            logit("et", "Extended status not available from K2\n");
            g_extstatus_avail = 0;
            break;
          }
          sprintf (msg_txt, "Error reading K2 extended status: %s",argv[1]);
          k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
          k2ew_exit(0);                /* exit program with code value */
        }
      }
    }
    k2mi_report_params(&k2hdr);
    last_stat_time = timevar;

    /* get aquisition channel count and sample rate */
    numacq_chan = BYTESWAP_UINT16(k2hdr.rwParms.misc.nchannels);
    g_smprate = BYTESWAP_UINT16(k2hdr.rwParms.stream.SampleRate);
    /* save copy of station-ID string received from K2 */
    strncpy(g_k2_stnid,k2hdr.rwParms.misc.stnID,sizeof(g_k2_stnid)-1);
    g_k2_stnid[sizeof(g_k2_stnid)-1] = '\0';     /* make sure of null */
        /* 2001.081 remapping of station id now possible from config */
    if (strlen(gcfg_stnid_remap) > 0 &&
                                   strcmp(g_k2_stnid,gcfg_stnid_remap) != 0)
    {    /* different station name given in config file */
      logit("et","Original K2 Station ID = \"%s\"\n",g_k2_stnid);
      logit("et","K2 Station ID remapped to \"%s\"\n",gcfg_stnid_remap);
      strncpy(g_stnid,gcfg_stnid_remap,sizeof(g_stnid)-1);
      g_stnid[sizeof(g_stnid)-1] = '\0';    /* make sure null terminated */
    }
    else      /* no station name remap */
    {                   /* put received name into use: */
      strncpy(g_stnid,g_k2_stnid,sizeof(g_stnid)-1);
      g_stnid[sizeof(g_stnid)-1] = '\0';    /* make sure null terminated */
    }

    /* save streaming and acquisition channel bitmap masks */
    strmmsk = BYTESWAP_UINT32(k2hdr.rwParms.stream.TxChanMap);
    acqmsk = BYTESWAP_UINT32(k2hdr.rwParms.misc.channel_bitmap);

    /* Determine which channel and network codes to stuff */
    /* into the EW headers */
    stmnum = (unsigned char)0;          /* initialize stream number */
    msk = (uint32_t)1;                  /* initialize bit mask position */
    for(idnum=0; (unsigned char)idnum<K2_MAX_STREAMS; ++idnum)
    {      /* for each possible stream */
      if((msk & strmmsk) != (uint32_t)0)
      {    /* channel is selected as a logical stream */

        /* fill in 'g_stmids_arr[][]' array with channel ID strings from */
        /*  the K2 (or substitute ID strings for any in the K2 that are  */
        /*  empty); one channel ID entry for each logical stream in use  */
        if(k2hdr.rwParms.channel[idnum].id[0] != '\0')
        {       /* channel ID in K2 is not blank; copy it over */
          strncpy(g_stmids_arr[stmnum], k2hdr.rwParms.channel[idnum].id,
                  sizeof(g_stmids_arr[0])-1);
          /* make sure string is NULL terminated */
          g_stmids_arr[stmnum][sizeof(g_stmids_arr[0])-1] = '\0';
        }
        else    /* channel ID in K2 is blank; generate substitute ID string */
          sprintf(g_stmids_arr[stmnum], "C%02d",idnum + 1);

        /* Fill in the g_netcode_arr[][] array with the network code from */
        /* the config file, if any.  If no netcode was specified in the   */
        /* config file, use netcodes from the K2 headers.                 */
        if ( *gcfg_network_buff != '\0' )
        {       /* Network code is specified in the config file; copy it over */
          strcpy(g_netcode_arr[stmnum], gcfg_network_buff);
	  logit("et", "Using Network code %s from .d config file for stream %d\n", g_netcode_arr[stmnum], stmnum);
        }
        else    /* No network code in the config file; copy from K2 headers */
        {
          strncpy(g_netcode_arr[stmnum], k2hdr.rwParms.channel[idnum].networkcode,
                  sizeof(g_netcode_arr[0])-1);
          /* make sure string is NULL terminated */
          g_netcode_arr[stmnum][sizeof(g_netcode_arr[0])-1] = '\0';
	  logit("et", "Using Network code %s from K2 for stream %d\n", g_netcode_arr[stmnum], stmnum);
        }

	/* The above looks weird, but apparently it is correct. One would  *
	 * think that g_stmids_arr[] should be indexed by idnum so that    *
	 * "channel numbers" and "stream numbers" would match. But the K2  *
	 * doesn't work that way. The first channel configured for         *
	 * streaming is stream #0; the second is stream #1, etc.,          *
	 * independent of the channel numbers. PNL 6/2/2004                */

        if(++stmnum >= (unsigned char)numacq_chan)
          break;        /* if ID for last stream filled then exit loop */
      }
      msk <<= 1;        /* shift to bit position for next channel */
    }
    /* Remember the number of streaming channels */
    g_numstms = stmnum;
    
    while(stmnum < K2_MAX_STREAMS)          /* fill in remaining entries */
      strcpy(g_stmids_arr[stmnum++],"???"); /*  with dummy strings */

    /* show parameter and status information */
    logit("et","K2 Station ID = \"%s\"\n",g_stnid);
    msk = (uint32_t)1;            /* initialize bit mask position */
    for ( idnum = 0; idnum < K2_MAX_STREAMS; idnum++ )
    {      /* for each possible stream */
       if((msk & strmmsk) != (uint32_t)0)
       {    /* channel is selected as a logical stream */
           logit( "et", "K2 Chan.Net.Loc Code: %s.%s.%s (channel %d)\n",
                 k2hdr.rwParms.channel[idnum].id,
                 k2hdr.rwParms.channel[idnum].networkcode,
                 k2hdr.rwParms.channel[idnum].locationcode, idnum );
       }
       msk <<= 1;        /* shift to bit position for next channel */
    }
    logit("et","K2 Instrument Code = %hu\n",
          (unsigned short)(k2hdr.roParms.instrumentCode));
    logit("et","K2 Header Version = %hu.%02hu\n",(unsigned short)
          BYTESWAP_UINT16(k2hdr.roParms.headerVersion) / (unsigned short)100,
          (unsigned short)
          BYTESWAP_UINT16(k2hdr.roParms.headerVersion) % (unsigned short)100);
    logit("et","K2 Serial Number = %hu\n",
          (unsigned short)BYTESWAP_UINT16(k2hdr.rwParms.misc.serialNumber));
    logit("et","K2 Site ID = \"%s\"\n",k2hdr.rwParms.misc.siteID);
    logit("et","K2 SDS Timeout = %hd\n",
          (short)BYTESWAP_UINT16(k2hdr.rwParms.stream.Timeout));
    logit("et","K2 Number of Acquired Channels = %d; Streaming Channels = %d\n",
	  numacq_chan, g_numstms);
    logit("et","K2 Channel Acquisition Bitmap = %04lXH\n",acqmsk);
    logit("et","K2 Channel Streaming Bitmap = %04lXH\n",strmmsk);

    k2mi_report_status(&k2stat);
    if(gcfg_ext_status && g_extstatus_avail)
      k2mi_report_extstatus(&k2extstat, ext_size);

    /* Initialize packet numbers if we don't read them from restart file */
    g_trk_stmnum = cur_stmnum = (unsigned char)0;
    g_trk_dataseq = cur_dataseq = (uint32_t)0;
  }
  else
  {   /* Set parameters from restart file */
    logit("et","Using stream parameters from restart file\n");
        /* 2001.081 remapping of station id now possible from config */
    if (gcfg_stnid_remap != NULL && strlen(gcfg_stnid_remap) > 0 &&
                                   strcmp(g_k2_stnid,gcfg_stnid_remap) != 0)
    {    /* different station name given in config file */
      logit("et","Original K2 Station ID = \"%s\"\n",g_k2_stnid);
      logit("et","K2 Station ID remapped to \"%s\"\n",gcfg_stnid_remap);
      strncpy(g_stnid,gcfg_stnid_remap,sizeof(g_stnid)-1);
      g_stnid[sizeof(g_stnid)-1] = '\0';    /* make sure null terminated */
    }
    else      /* no station name remap */
    {                   /* put received name into use: */
      strncpy(g_stnid,g_k2_stnid,sizeof(g_stnid)-1);
      g_stnid[sizeof(g_stnid)-1] = '\0';    /* make sure null terminated */
      logit("et","K2 Station ID = \"%s\"\n",g_stnid);
    }
    logit("et","K2 Number of Streams = %d\n",g_numstms);

    /* dataseq and stmnum have been incremented from the last tracebuf message
     * by output thread before writing restart file, so these numbers here
     * are the numbers for the next expected packet */
    g_trk_dataseq = cur_dataseq = rstrt_dataseq;
    g_trk_stmnum = cur_stmnum = rstrt_stmnum;
  }

  if(g_numstms < 1 || (unsigned char)g_numstms > K2_MAX_STREAMS)
  {      /* stream count out of range; show error message and abort program */
    sprintf (msg_txt, "Illegal number of K2 streams--aborting: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                  /* exit program with code value */
  }
  if(g_smprate < 1)
  {      /* sample rate out of range; show error message and abort program */
    sprintf (msg_txt, "Illegal K2 sample rate--aborting: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                  /* exit program with code value */
  }

  /* setup the k2info logo if necessary */
  if (gcfg_inject_info == 1) 
  {
    if (k2info_init() == -1)
    {
      sprintf (msg_txt, "Error fetching logo type values for K2INFO messages: %s",argv[1]);
      k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
      k2ew_exit(0);                 /* exit program with code value */
    }
  }

  /* substitute in any "ChannelNames" items that were given; *
   * check the LocationNames                                 */
  for(idnum=0; idnum<g_numstms; ++idnum)
  {      /* for each channel (stream) */
    if(gcfg_chnames_arr[idnum] != NULL &&
       gcfg_chnames_arr[idnum][0] != '\0') {
	/* there is a configured channel name for this stream */
      if (strncmp(g_stmids_arr[idnum],gcfg_chnames_arr[idnum],
		  CHANNEL_ID_LENGTH) != 0)
      {    
	  logit("et","Renaming channel (stream %d) from \"%s\" to \"%s\"\n",
		idnum,g_stmids_arr[idnum],gcfg_chnames_arr[idnum]);
	  /* copy in new channel name: */
	  strncpy(g_stmids_arr[idnum],gcfg_chnames_arr[idnum], 
		  CHANNEL_ID_LENGTH);
	  /* make sure string is NULL terminated: */
	  g_stmids_arr[idnum][CHANNEL_ID_LENGTH] = '\0';
      }
    }
    else   /* there is no configured channel name for this stream */
    {
	logit("et", "No configured name for channel \"%s\" (stream %d)\n",
	      g_stmids_arr[idnum], idnum);
	ch_cfg_err++;
    }
    if (gcfg_locnames_arr[idnum] != NULL &&
	gcfg_locnames_arr[idnum][0] != '\0')
    {   /* there is a location code configured for this stream */
	logit("et", "Using location code \"%s\" for channel \"%s\" (stream %d)\n",
	      gcfg_locnames_arr[idnum], g_stmids_arr[idnum], idnum);
    }
    else 
    {
	if (gcfg_lc_flag == 1)
	{
	    logit("et", "Using default location code \"%s\" for channel \"%s\" (stream %d)\n",
		  LOC_NULL_STRING, g_stmids_arr[idnum], idnum);
	    gcfg_locnames_arr[idnum] = strdup(LOC_NULL_STRING);
	}
	else if (gcfg_lc_flag == 2)
        {
	    logit("et", "No location code configured for channel \"%s\" (stream %d)\n",
		  g_stmids_arr[idnum], idnum);
	    ch_cfg_err++;
	}
    }
  }
  if (ch_cfg_err && gcfg_lc_flag == 2) {
      sprintf (msg_txt, "Error in channel/location name configuration: %s",argv[1]);
      k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
      k2ew_exit(0);                 /* exit program with code value */
  }

  for ( stmnum = 0; stmnum < g_numstms; stmnum++ )
  {    /* for each channel name; log name */
    logit("et","Using Chan.Net.Loc Code: ");
    logit("e","%s.%s.%s",g_stmids_arr[stmnum],
                         g_netcode_arr[stmnum],
                         gcfg_locnames_arr[stmnum]);
    logit( "e", " (stream %d)\n", stmnum );
  }

  rc = 0;          /* check if any channels (streams) are to be inverted */
  for(stmnum=0; stmnum<(unsigned char)g_numstms; ++stmnum)
  {      /* for each invert-polarity flag */
    if(gcfg_invpol_arr[stmnum] != 0)
    {    /* invert-polarity flag is set*/
      rc = 1;      /* indicate that at least one flag set */
      break;       /* exit loop */
    }
  }
  if(rc != 0)
  { /* at least one invert-polarity flag is set */
    logit("et","Inverted Channels = ");
    rc = 0;        /* initialize flag for first item */
    for(stmnum=0; stmnum<(unsigned char)g_numstms; ++stmnum)
    {    /* for each channel (stream) */
      if(gcfg_invpol_arr[stmnum] != 0)
      {  /* invert-polarity flag for channel (stream) is set */
        if(rc == 0)          /* if first one then */
          rc = 1;            /* set flag */
        else                 /* if not first one then */
          logit("e",", ");   /* put in separator */
        logit("e","%s.%s",g_stmids_arr[stmnum], gcfg_locnames_arr[stmnum]);
      }
    }
    logit("e","\n");         /* put in line terminator */
  }

  /* calculate retry interval for waiting packets */
  wait_resenditvl = g_numstms * gcfg_wait_resendval;

  /* initialize circular buffer to twice the WaitTime per stream */
  if ( (rc = k2cb_init_buffer(g_numstms * gcfg_pktwait_lim * 2,
                                                g_smprate)) != K2R_NO_ERROR)
  {      /* error code returned; show error message and abort program */
    sprintf (msg_txt, "Error initializing circular buffer: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                 /* exit program */
  }

  if (g_validrestart_flag == 0)
  {
    /* start acquisition and streaming */
    logit("et", "Starting K2 acquisition and streaming...\n");
    count = K2_NUM_RETRIES;        /* initialize retry count */
    while ( (rc = k2pm_start_streaming(&seqnum, 1)) != K2R_POSITIVE)
    {      /* loop while function fails */
      if (--count <= 0)
      {         /* too many attempts; indicate failure */
        sprintf (msg_txt, "Error starting K2 serial data stream output: %s",argv[1]);
        k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
        k2ew_exit(0);               /* exit program with code value */
      }
    }
    if (gcfg_debug > 0)
      logit("et","Started K2 acquisition and serial data stream output\n");
  }

  /* wait for start of data */
  if (k2c_rcvflg_tout(gcfg_commtimeout_itvl) == K2R_ERROR)
  {
    sprintf (msg_txt, "No stream data seen from K2 on startup: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);               /* exit program with code value */
  }

  /* install handler function for various "signal" events */
  signal(SIGINT,k2ew_signal_hdlr);     /* <Ctrl-C> interrupt */
  signal(SIGTERM,k2ew_signal_hdlr);    /* program termination request */
#ifdef SIGBREAK
  signal(SIGBREAK,k2ew_signal_hdlr);   /* keyboard break (sent by logoff & "X") */
#endif
  signal(SIGABRT,k2ew_signal_hdlr);    /* abnormal termination */
  signal(SIGFPE,k2ew_signal_hdlr);     /* arithmetic error */
#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);            /* Ignore SIGPIPE if we have it */
#endif

  /* install handler to intercept/ignore the Windows CTRL_LOGOFF_EVENT */
#ifdef _WINNT 
  SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ); 
#endif /* _WINNT */

  logit("et","Beginning K2 data processing...\n");

  /* startup the output thread */
  if (StartThread(k2ew_outputthread_fn, (unsigned)0, &g_output_threadid) == -1)
  {
    sprintf (msg_txt, "Unable to startup output thread--aborting: %s",argv[1]);
    k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
    k2ew_exit(0);                 /* exit program with code value */
  }

  /* read data blocks */
  k2ew_signal_val = -1;      /* initialize signal ID from handler function */
  g_pktout_okcount =         /* initialize packets-delivered-OK count */
    g_seq_errcnt =             /* initialize output sequence error count */
    missing_errcnt =           /* initialize "missing packets" error count */
    unexpect_errcnt =          /* initialize "unexpected packet" error count */
    g_skip_errcnt =            /* initialize "packet skipped" error count */
    request_errcnt =           /* initialize "request resend" error count */
    resync_errcnt =            /* initialize "resync" error count */
    packet_errcnt =            /* initialize "packet data" error count */
    receive_errcnt =           /* initialize "received data" error count */
    retried_pckcnt =           /* initialize "# of packet retries" count */
    pktin_okcount =            /* initialize packets-received-OK count */
    last_pktin_okcount =       /* init 'last' packets-received-OK count */
    last_pktout_okcount =      /* init 'last' packets-sent-OK count */
    last_seq_errcnt =          /* init 'last' sequence error count */
    last_retried_pckcnt =      /* init 'last' "# of packet retries" count */
    comm_retries =      	/* init # of communications retries count */
    last_missing_errcnt = 0L;  /* init 'last' missing packets error count */
  g_req_pend = 0;              /* initialize pending request count */
  cb_rc = K2R_NO_ERROR;      /* initialize circular buffer fn return code */
  rerequest_idnum = K2CB_DBFIDX_NONE;       /* init re-request ID number */
  g_terminate_flg = 0;       /* initialize program-terminate flag */

  /********************** Main Packet Loop ****************************/
  do          /* loop while reading serial data stream packets */
  {
    time(&timevar);

         /* Request a status report from K2, if it's time */
    if(gcfg_status_itvl > 0 && difftime(timevar,last_stat_time) >
                                                   (double)gcfg_status_itvl)
    {
                                  /* send request for status packet */
      if((rc=k2mi_req_status(&seqnum)) != K2R_NO_ERROR)
        break;          /* if send failed then exit loop (and program) */

      if(g_extstatus_avail &&     /* if OK then send req for ext status */
                           (rc=k2mi_req_extstatus(&seqnum)) != K2R_NO_ERROR)
      {  /* sending of request failed */
        break;          /* exit loop (and program) */
      }

      /* Get K2 params for status report, and to check K2 station *
       * name against restart-file station name.                  */
      if ( (rc = k2mi_req_params(&seqnum)) != K2R_NO_ERROR)
        break;

      if(last_stat_time > 0)
      {  /* not the first time through */
        uint32_t okcnt,rtycnt;
              /* log "in-process" summary stats */
        logit("et","In-Process Summary:\n");
        gmtime_ew(&prog_start_time,&tmblk);      /* conv & log start time */
        logit("e","Program start time:  %4d%02d%02d_UTC_%02d:%02d:%02d\n",
                (tmblk.tm_year+TM_YEAR_CORR),(tmblk.tm_mon+1),tmblk.tm_mday,
                                   tmblk.tm_hour,tmblk.tm_min,tmblk.tm_sec);
        logit("e","Count totals:\n-------------\n");
        logit("e","# of packets received OK    :  %lu\n",pktin_okcount);
        logit("e","# of packets sent to EW     :  %lu\n",g_pktout_okcount);
        logit("e","# of output sequence errors :  %lu\n",g_seq_errcnt);
        if(g_seq_errtime > (time_t)0)
        {     /* last output sequence error time has been filled in */
          gmtime_ew(&g_seq_errtime,&tmblk);                /* convert time */
          logit("e","Time of last output seq err :  "      /* log GM time */
                    "%4d%02d%02d_UTC_%02d:%02d:%02d\n",
                (tmblk.tm_year+TM_YEAR_CORR),(tmblk.tm_mon+1),tmblk.tm_mday,
                                   tmblk.tm_hour,tmblk.tm_min,tmblk.tm_sec);
        }
        logit("e","Number of packet retries    :  %lu\n",retried_pckcnt);
        logit("e","Packet error rating         :  %1.3lf%%\n",
                                                        ((pktin_okcount!=0)?
                           (double)retried_pckcnt/pktin_okcount*100.0:0.0));
        logit("e","Missing packet(s) events    :  %lu\n",missing_errcnt);
        logit("e","Number of packets skipped   :  %lu\n",g_skip_errcnt);
        logit("e","Request resend failures     :  %lu\n",request_errcnt);
        logit("e","Number of stream resyncs    :  %lu\n",resync_errcnt);
        logit("e","Number of comm timeouts     :  %lu\n",comm_retries);
        logit("e","Since last summary:\n------------------\n");
        logit("e","# of packets received OK    :  %lu\n",
                                  (okcnt=pktin_okcount-last_pktin_okcount));
        logit("e","# of packets sent to EW     :  %lu\n",
                                      g_pktout_okcount-last_pktout_okcount);
        logit("e","# of output sequence errors :  %lu\n",
                                              g_seq_errcnt-last_seq_errcnt);
        logit("e","Number of packet retries    :  %lu\n",
                               (rtycnt=retried_pckcnt-last_retried_pckcnt));
        logit("e","Packet error rating         :  %1.3lf%%\n",((okcnt!=0)?
                                           (double)rtycnt/okcnt*100.0:0.0));
        logit("e","Missing packet(s) events    :  %lu\n",
                                        missing_errcnt-last_missing_errcnt);
        last_pktin_okcount = pktin_okcount;           /* set 'last' vars */
        last_pktout_okcount = g_pktout_okcount;       /*  for next time  */
        last_seq_errcnt = g_seq_errcnt;
        last_retried_pckcnt = retried_pckcnt;
        last_missing_errcnt = missing_errcnt;
	/* if we are injecting status info for k2ewagent, populate a comm info struct and send it */
	if (gcfg_inject_info==1)
        {
	    struct COMM_INFO ci;
	    ci.program_start_time=prog_start_time;
	    ci.last_info_time=last_stat_time;
	    time(&ci.comm_info_time);
	    ci.total_pkt_received = pktin_okcount;
	    ci.total_pkt_sent = g_pktout_okcount;
	    ci.total_pkt_output_seqerr = g_seq_errcnt;
	    ci.last_output_seqerr_time = g_seq_errtime;
	    ci.total_pkt_retries = retried_pckcnt;
	    ci.total_pkt_skipped = g_skip_errcnt;
	    ci.total_pkt_request_resend_failure = request_errcnt;
	    ci.total_comm_retries = comm_retries;
	    ci.total_pkt_missing_err = missing_errcnt;
	    ci.total_error_rating = (float) ((pktin_okcount!=0)?(float)retried_pckcnt/pktin_okcount*100.0:0.0);
	    ci.last_pkt_received = okcnt;
	    ci.last_pkt_sent = g_pktout_okcount-last_pktout_okcount;
	    ci.last_pkt_retries = rtycnt;
	    ci.last_error_rating = (float) ((okcnt!=0)?(float)rtycnt/okcnt*100.0:0.0);
            k2info_send(K2INFO_TYPE_COMM, (char *) &ci);
	}
      }

      last_stat_time = timevar;
    }

    if((rc=k2pm_get_stmblk(&datahdr,databuff,&dcount,1)) == K2R_POSITIVE)
    {    /* SDS block retrieved OK */
      /* Tell the world, maybe */
      if(timeout_logged)
      {
        logit("et", "Resumed communicating with K2\n");
        timeout_logged = 0;
      }
      if(datahdr.StreamNum >= (unsigned char)g_numstms)
      {       /* received stream number is out of range */
        logit("et", "Received stream number (%d) out of range (1 - %d)\n",
              (int)datahdr.StreamNum, g_numstms);
        rc = K2ERR_BAD_STMNUM;         /* setup error code */
      }
      else
      {
        if (dcount != g_smprate)
        {     /* received data count mismatches */
          logit("et", "Received datacount (%d) not expected (%d)\n",
                dcount, g_smprate);
          rc = K2ERR_BAD_DATACNT;      /* setup error code */
        }
      }
    }
         /* check if terminate flag set by signal handler */
    if(g_terminate_flg)      /* if terminate flag set then */
      break;                 /* exit loop (and program) */

    /* stream data block received; now deal with it */
    if (rc == K2R_POSITIVE )
    {    /* SDS block received and checked */
      if (gcfg_debug > 2)
      {  /* logging level enabled; show message */
        logit("et", "Received:  stm#=%hu, dseq=%lu, secs=%lu, ms=%hu\n",
                       (unsigned short)(datahdr.StreamNum), datahdr.DataSeq,
                                            datahdr.Seconds, datahdr.Msecs);
      }

      if(gcfg_invpol_arr[datahdr.StreamNum] != 0)
      {  /* invert polarity flag for channel (stream) is enabled */
        for(count=0; count<dcount; ++count)
        {     /* for each data value in array */
          databuff[count] *= -1;       /* invert polarity of data value */
        }
      }

      if(datahdr.StreamNum == cur_stmnum && datahdr.DataSeq == cur_dataseq)
      {  /* packet stream and data sequence numbers match expected values */
                   /* enter received data block into circular buffer */
        if((cb_rc=k2cb_block_in(datahdr.StreamNum, datahdr.DataSeq,
                                  datahdr.Seconds, datahdr.Msecs, databuff))
                                                            != K2R_NO_ERROR)
        {          /* error code returned */
          break;             /* exit loop (and program) */
        }
        ++pktin_okcount;          /* increment packet-received-OK count */
        /* increment current stream (and data sequence) numbers */
        if(++cur_stmnum >= (unsigned char)g_numstms)
        {          /* increment puts it past last stream number */
          cur_stmnum = (unsigned char)0;  /* wrap-around to first stream number */
          ++cur_dataseq;               /* increment data sequence number */
        }
        last_sec = datahdr.Seconds; /* Save the packet time for later */
      }
      else
      {     /* stream and data sequence numbers do not match "current" */
        if ( (idx = K2M_PCKT_RELIDX(datahdr.StreamNum, datahdr.DataSeq)) > 0L)
        {   /* packet position is "ahead" of current packet expected */
          if(cur_dataseq >= 10)   /* if not one of first packets then */
            ++missing_errcnt;     /* inc "missing packets" error count */
          if ((int)(datahdr.DataSeq - cur_dataseq) <= gcfg_pktwait_lim)
          {      /* packet position is not too far "ahead" of current pkt */
            if(gcfg_debug > 0)
            {      /* debug output enabled; log message */
              logit("et","Detected %ld missing packet", idx);
              if ( idx != 1 ) logit( "e", "s" );
              logit("e"," (errorcount=%lu)\n", missing_errcnt);
            }
            if(cur_dataseq >= 10)      /* if not one of first packets then */
              retried_pckcnt += idx;   /* add to # of packet retries count */
            for (c = 0; c < idx; ++c)
            {      /* for each position passed-by */
                        /* put a "waiting" block into circular buffer */
                        /*  (resend of block will be requested elsewhere) */
              if ( (cb_rc = k2cb_blkwait_in(cur_stmnum, cur_dataseq)) !=
                                                               K2R_NO_ERROR)
              {
                break;     /* if error then exit loop */
              }
              ++g_wait_count;     /* increment waiting list entry count */
                   /* increment current stream (and data sequence) numbers */
              if (++cur_stmnum >= (unsigned char)g_numstms)
              {         /* increment puts it past last stream number */
                cur_stmnum = (unsigned char)0;  /* wrap-around to 0 */
                ++cur_dataseq;              /* increment data sequence number */
              }
            }
          }
          else   /* packet position is too far "ahead" of current packet; */
          {
            ++resync_errcnt;           /* increment "resync" error count */
            /*  show message and resync to new packet position */
            logit("et","Too many missing packets (%ld) detected "
                  "(errorcount=%lu)\nResync-ing (count=%lu) to:  "
                  "stm#=%hu, dseq=%lu, secs=%lu\n", idx,
                  missing_errcnt, resync_errcnt,
                  (unsigned short)(datahdr.StreamNum),
                  datahdr.DataSeq, datahdr.Seconds);
            k2cb_check_waits((uint32_t)-1);      /* Clear all wait pkts */
          }
              /* put "current" packet position in sync with received */
          cur_stmnum = datahdr.StreamNum;    /* take new stream number */
          cur_dataseq = datahdr.DataSeq;     /* take new data sequence # */
              /* enter received data block into circular buffer */
          if((cb_rc=k2cb_block_in(datahdr.StreamNum, datahdr.DataSeq,
                                             datahdr.Seconds, datahdr.Msecs,
                                                 databuff)) != K2R_NO_ERROR)
          {      /* error code returned */
            break;           /* exit loop (and program) */
          }
          ++pktin_okcount;        /* increment packet-received-OK count */
          /* increment current stream (and data sequence) numbers */
          if (++cur_stmnum >= (unsigned char)g_numstms)
          {      /* increment puts it past last stream number */
            cur_stmnum = (unsigned char)0;
            ++cur_dataseq;           /* increment data sequence number */
          }
          last_sec = datahdr.Seconds; /* Save the packet time for later */
        }
        else
        {   /* packet position is "behind" current packet expected */
          if((rc=k2cb_fill_waitblk(datahdr.StreamNum, datahdr.DataSeq,
                                    datahdr.Seconds, datahdr.Msecs,databuff,
                                         &rerequest_idnum)) == K2R_NO_ERROR)
          {      /* "waiting" block filled OK */
            if(g_wait_count > 0)  /* if count greater than zero then */
              --g_wait_count;     /* decrement waiting list entry count */
            if(g_req_pend > 0)    /* if count greater than zero then */
              --g_req_pend;       /* decrement pending request count */
            ++pktin_okcount;      /* increment packet-received-OK count */
            if(gcfg_debug > 0)
            {
              logit("et","\"Waiting\" data block received:  stm#=%hu, "
                    "dseq=%lu, secs=%lu, ms=%hu\n",
                    (unsigned short)(datahdr.StreamNum),
                    datahdr.DataSeq, datahdr.Seconds, datahdr.Msecs);
            }
          }
          else
          {      /* "waiting" block not filled */
            if(rc == K2ERR_CB_NOTFOUND)
            {    /* no matching "waiting" block was found */
              /* Is this packet time later than the last known packet time?
               * If so, the K2 has reset its sequence number. We will
               * enter this packet in the CB, and reset our counters for
               * the next packet. We also need to clear out the CB of old
               * wait packets, since the K2 doesn't recognize their sequence
               * numbers any more.
               */
              if (datahdr.Seconds > last_sec)
              {
                logit("et",
                      "K2 sequence number reset; updating k2ew sequence numbers\n"
                      "\tand clearing old wait entries from circular buffer\n");
                ++resync_errcnt;           /* increment "resync" error count */
                sprintf(msg_txt, "K2 <%s> has restarted", g_stnid);
                k2mi_status_hb(g_error_ltype, K2STAT_RESTART, msg_txt);
                k2cb_check_waits((uint32_t)-1);    /* Clear wait pkts */
                   /* put "current" packet position in sync with received */
                cur_stmnum = datahdr.StreamNum;    /* take new stream number */
                cur_dataseq = datahdr.DataSeq;     /* take new data sequence # */
                   /* enter received data block into circular buffer */
                if((cb_rc=k2cb_block_in(datahdr.StreamNum, datahdr.DataSeq,
                                             datahdr.Seconds, datahdr.Msecs,
                                                 databuff)) != K2R_NO_ERROR)
                {      /* error code returned */
                  break;           /* exit loop (and program) */
                }
                ++pktin_okcount;        /* increment packet-received-OK count */
                /* increment current stream (and data sequence) numbers */
                if (++cur_stmnum >= (unsigned char)g_numstms)
                {      /* increment puts it past last stream number */
                  cur_stmnum = (unsigned char)0;
                  ++cur_dataseq;           /* increment data sequence number */
                }
                last_sec = datahdr.Seconds; /* Save packet time for later */
              }
              else
              {
                ++unexpect_errcnt;       /* inc "unexpected pkt" error count */
                if (gcfg_debug > 0)
                   logit("et","Unexpected data block (count=%lu) received "
                         "and discarded:  stm#=%hu, dseq=%lu, secs=%lu, "
                         "ms=%hu\n", unexpect_errcnt,
                         (unsigned short)(datahdr.StreamNum),
                         datahdr.DataSeq, datahdr.Seconds, datahdr.Msecs);
              }
            }
            else
            {    /* other (more serious) error code returned */
              cb_rc = rc;       /* save code for processing below */
              break;            /* exit loop (and program) */
            }
          }
        }
      }

      /* update "tick" counters on any waiting blocks in circ buffer */
      if((idnum=k2cb_tick_waitents(wait_resenditvl,&stmnum,&dataseq,
                                 &numticks,&resendcnt)) == K2CB_DBFIDX_NONE)
      {  /* waiting list empty; make sure resend request counter agrees */
        g_req_pend = 0;
      }
      if(rerequest_idnum != K2CB_DBFIDX_NONE)
      {  /* re-request of waiting packet was specified */
                             /* get information for re-requested packet */
        if(k2cb_get_entry(rerequest_idnum,&rr_stmnum,&rr_dataseq,NULL,
                                             &rr_resendcnt) == K2R_POSITIVE)
        {     /* waiting list not empty & info on re-req packet fetched OK */
          idnum = K2CB_DBFIDX_NONE;    /* clear to avoid request below */
          if(++rr_resendcnt > 1)       /* inc (local) resend req count */
            ++retried_pckcnt;          /* if > 1 then inc retried pck cnt */
          if (gcfg_debug > 0)
          {        /* debug is enabled; show message */
            logit("et","Re-requesting resend (#%d) of packet:  stm#=%d, "
                       "dseq=%lu (pending=%d)\n",rr_resendcnt,
                                    (int)rr_stmnum,rr_dataseq,g_req_pend);
          }
          if((rc=k2pm_req_stmblk(rr_stmnum,rr_dataseq,1)) != K2R_NO_ERROR)
          {      /* resend-request function returned error */
            ++request_errcnt;        /* inc "request resend" error count */
            logit("et","Error (count=%lu) requesting resend of packet "
                  "(stm#=%d, dseq=%lu)\n",request_errcnt,
                  (int)rr_stmnum,rr_dataseq);

          }
                        /* increment resend and clear tick count for block */
          if (k2cb_incwt_resendcnt(rerequest_idnum) != K2R_POSITIVE)
          {
            logit("et", "IDnum (%d) not found by 'k2cb_incwt_resendcnt()'\n",
                                                           rerequest_idnum);
          }
        }
        else
        {
          logit("et", "IDnum (%d) not found by 'k2cb_get_entry()'\n",
                                                           rerequest_idnum);
        }
        rerequest_idnum = K2CB_DBFIDX_NONE;      /* clear specifier */
      }
      if(idnum != K2CB_DBFIDX_NONE)
      {       /* waiting list not empty */
        if(resendcnt == 0 || numticks >= wait_resenditvl)
        {     /* no resend-requests have yet happened or time for next one */
          if(++resendcnt <= gcfg_max_blkresends)
          {   /* not too many resend requests so far (for this block) */
                   /* if this is not the first resend request for block */
                   /*  then bypass the 'MaxReqPending' limit and push   */
                   /*  this resend request through                      */
            if(resendcnt <= 1)
            {      /* this is first resend request for block */
              if(g_req_pend >= gcfg_max_reqpending)
              {    /* at or beyond maximum allowed new resend requests */
                if(no_req == 0)
                {       /* new resent requests are currently enabled */
                  no_req = 1;     /* stop new resend requests for now */
                  if(gcfg_debug > 1)
                  {
                    logit("et","New resend requests paused (%d waiting)\n",
                                                                g_req_pend);
                  }
                }
              }
              else if(no_req != 0 && g_req_pend <= resume_numpend)
              {    /* new resend requests may now be resumed */
                no_req = 0;
                if(gcfg_debug > 1)
                {
                  logit("et","New resend requests resumed (%d waiting)\n",
                                                                g_req_pend);
                }
              }
            }
            if(no_req == 0 || resendcnt > 1)
            {      /* new resend reqs are enabled or not first for block */
              if(resendcnt <= 1)  /* if first resend for block then */
                ++g_req_pend;     /* inc total # of resend req count */

	      /* if there is a resend request, then we need to make sure the serial port has control,
		not sure we need to do this EACH resend request ,
		so, only do this if we haven't succeeded in forcing it in the last FORCE_TIMEOUT_CHECK secs or 
			if we last_attempted to force it within 10 seconds of the last time we tried */

#define FORCE_TIMEOUT_CHECK 20  /* this is in seconds */

 	      time(&now);
  	      if ( gcfg_force_blkmde && ((now - last_force_block) > FORCE_TIMEOUT_CHECK) ) 
              {
                 if( (now - last_attempted_force_block >= 10) && k2mi_force_blkmde() == K2R_ERROR)
                 {     
                   logit ("et", "Unable to force block mode with K2 as requested for resend requests\n");
 	           time(&last_attempted_force_block);
		   continue;
                 } else {
                   logit("et","Forced block mode with K2 for resend requests, will not try again for %d secs\n", FORCE_TIMEOUT_CHECK);
 	           time(&last_force_block);
   	  	 }
              }
              if (gcfg_debug > 0)
              {
                logit("et","Requesting resend (#%d) of packet:  stm#=%d, "
                    "dseq=%lu (pending=%d)\n",resendcnt,(int)stmnum,dataseq,
                                                                g_req_pend);
              }
              if((rc=k2pm_req_stmblk(stmnum, dataseq, 1)) != K2R_NO_ERROR)
              {      /* resend-request function returned error */
                ++request_errcnt;        /* inc "request resend" error count */
                logit("et","Error (count=%lu) requesting resend of packet "
                      "(stm#=%d, dseq=%lu)\n",request_errcnt,
                      (int)stmnum, dataseq);

              }
                        /* increment resend and clear tick count for block */
              if (k2cb_incwt_resendcnt(idnum) != K2R_POSITIVE)
              {
                logit("et", "IDnum (%d) not found by k2cb_incwt_resendcnt\n",
                                                                     idnum);
              }
              if(resendcnt > 1)        /* if resend req count > 1 then   */
                ++retried_pckcnt;      /* increment retried packet count */
            }
          }
          else
          {   /* too many resends have occurred; change status to "skip" */
            ++g_skip_errcnt;      /* inc "packet skipped" error count */
            ++retried_pckcnt;     /* increment retried packet count */
            logit("et","Excessive resend count (skipcount=%lu), "
                  "skipping:  stm#=%d, dseq=%lu\n",g_skip_errcnt,
                  (int)stmnum, dataseq);
            /* set wait entry to "skip" */
            if (k2cb_skip_waitent(stmnum,dataseq) != K2R_POSITIVE)
            {
              logit("et", "k2cb_skip_waitent unable to find entry: "
                  "stm#=%d, dseq=%lu\n",(int)stmnum,dataseq);
            }
            else
            {
              if(g_wait_count > 0)     /* if count greater than zero then */
                --g_wait_count;        /* decrement waiting list entry count */
            }
          }
        }
      }
    }
    else
    {    /* error retrieving SDS block or number of data entries mismatches */
      if(rc != K2R_ERROR && rc != K2R_TIMEOUT)
      {       /* returned code is not one the "fatal" errors */
        if(rc == K2R_PAYLOAD_ERR || rc == K2ERR_BAD_STMNUM ||
                                                    rc == K2ERR_BAD_DATACNT)
        {     /* SDS packet was received and its header interpreted OK   */
              /*  but there was an error in its payload data, its stream */
              /*  number was out of range, its data count was wrong, its */
              /*  data size was wrong or its CRC was bad                 */
          ++packet_errcnt;       /* increment "packet data" error count */
          logit("et","Error %d (count=%lu) on received packet (stm#=%hu, "
                     "dseq=%lu)\n", rc, packet_errcnt,
                       (unsigned short)(datahdr.StreamNum),datahdr.DataSeq);
          if(rerequest_idnum == K2CB_DBFIDX_NONE &&
              datahdr.StreamNum > (unsigned char)0 && datahdr.DataSeq > 0 &&
                                         (datahdr.StreamNum != cur_stmnum ||
                                            datahdr.DataSeq != cur_dataseq))
          {        /* no re-request of wait block pending, stream  */
                   /*  and data sequence number of received packet */
                   /*  are valid and packet is not "current"       */
            rerequest_idnum =          /* look for matching waiting entry */
                        k2cb_get_waitent(datahdr.StreamNum,datahdr.DataSeq);
          }
        }
        else
        {     /* received data not recognized as SDS packet */
          ++receive_errcnt;       /* increment "received data" error count */
          logit("et","Error (count=%lu) processing received data\n",
                receive_errcnt);
        }
      }
      else if (rc == K2R_TIMEOUT)
      {
        if (gcfg_dont_quit == 0)
        {
          g_terminate_flg = 1;  /* Tell the output thread to terminate */
          sprintf (msg_txt, "Timed out communicating with K2, aborting %s: %s",g_stnid, argv[1]);
          k2ew_enter_exitmsg(K2TERM_K2_COMMERR,   msg_txt); /* log & enter exit message */
          break;                   /* exit loop (and program) */
        }
        else  /* gcfg_dont_quit == 1 */
        {
          if (timeout_logged == 0)
          {   /* this is a "new" timeout event */
	    comm_retries++;
            logit("et", "Timed out communicating with K2; continuing\n");
            timeout_logged = 1;
            if(gcfg_restart_commflag != 0)
            {      /* flag is set; close and then reopen comm to K2 */
              logit("et","Closing and reopening communications with K2"
                         " after timeout\n");
              k2c_close_io();         /* close IO port */
                                      /* reopen IO port */
                   /* Initialize IO to the K2: sockets or serial comms */
              if((rc=k2c_init_io(&gen_io,gcfg_commtimeout_itvl)) !=
                                                               K2R_NO_ERROR)
              {    /* comms initialization failed */
                sprintf (msg_txt, "Unable to reopen IO port for %s: %s",g_stnid, argv[1]);
                k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
                break;                 /* exit loop (and program) */
              }
  	      if ( gcfg_force_blkmde ) 
              {
                 if(k2mi_force_blkmde() < K2R_NO_ERROR)
                 {     
                   logit ("et", "Unable to force block mode with K2 as requested");
		   break;
                 }
                 logit("et","Forced block mode with K2\n");
 	         time(&last_force_block);
              }
              /* confirm or restart block mode in the K2: */
              if(k2mi_init_blkmde(&seqnum) < K2R_NO_ERROR)
              {    /* error code returned; show error msg and exit prog */
                sprintf (msg_txt, "Unable to re-establish communications link with K2 %s: %s",
			g_stnid, argv[1]);
                k2ew_enter_exitmsg(K2TERM_K2_STARTUP, msg_txt);
                break;                 /* exit loop (and program) */
              }
              logit("et", "Communications with K2 reopened OK\n");
            }
          }
        }
      }
      else
      {
        g_terminate_flg = 1;  /* Tell the output thread to terminate */
        sprintf (msg_txt, "Error communicating with K2 %s, aborting: %s",
			g_stnid, argv[1]);
	k2ew_enter_exitmsg(K2TERM_K2_COMMERR,  msg_txt);  /* log & enter exit message */
        break;                   /* exit loop (and program) */
      }
    }

    if (cb_rc != K2R_NO_ERROR)   /* if circular buffer write fn error then */
    {
      g_terminate_flg = 1;  /* Tell the output thread to terminate */
      break;                      /* exit loop (and program) */
    }

    /* check from terminate request from Earthworm */
    if ( tport_getflag(&g_tport_region) == TERMINATE  ||
         tport_getflag(&g_tport_region) == g_pidval     )
    {         /* terminate request received */
      g_terminate_flg = 1;
      sprintf (msg_txt, "Terminate request received from Earthworm for %s, Exiting: %s",
			g_stnid, argv[1]);
      k2ew_enter_exitmsg(K2TERM_EW_TERM,  msg_txt);     /* log & enter exit message */
      break;                      /* exit loop (and program) */
    }

  }
  while (g_terminate_flg == 0);     /* loop until terminate flag is set */
  /************************* End of Main Loop *************************/

  if (k2ew_signal_val != -1)
  {      /* loop termination was caused by "signal" event */
    char sigstr[32];
    k2ew_signal_string( k2ew_signal_val, sigstr, 32 );
    sprintf (msg_txt, "Signal %s detected. Exiting program: %s", 
                       sigstr, argv[1]);
    k2ew_enter_exitmsg(K2TERM_SIG_TRAP, msg_txt);
  }
  else
  {
    if (cb_rc != K2R_NO_ERROR)
    {    /* circular buffer write function returned error code */
      /* Dump the circular buffer indexes to file for debugging */
      k2cb_dump_buf();

      k2ew_enter_exitmsg(K2TERM_K2_CIRBUFF,        /* log & enter exit message */
                             "Error writing to circular data buffer:  %s for %s\n",
                                                    k2ew_get_errmsg(cb_rc), argv[1]);
    }
  }

  logit("et","Ending K2 data processing\n");
  sleep_ew(1500);  /* give the output thread some time to write restart file */
  if (gcfg_debug > 1)
    logit("e", "shutdown indicators rc %d, rsfile %s\n", rc,
          gcfg_restart_filename);

  /* If there's no restartfile, or if we have an error other than due to
   * a handled signal, turn off streaming on the K2 */
  if (gcfg_restart_filename[0] == '\0' ||
      ( rc == K2R_ERROR && k2ew_signal_val == -1))
  {
    /* stop streaming and acquiring */
    logit("et", "Shutting down stream output...\n");
    count = K2_NUM_RETRIES;        /* initialize retry count */
    while(1)
    {      /* loop while function fails */
      if ( (rc = k2pm_stop_streaming(&seqnum, 0)) == K2R_POSITIVE)
      {    /* function succeeded; show OK message */
        logit("et","Stopped K2 serial data stream output\n");
        break;            /* exit loop */
      }
      if (--count <= 0)
      {         /* too many attempts; indicate failure */
        logit("et","Error stopping K2 serial data stream output\n");
        break;            /* exit loop */
      }
    }
  }
  else
    if (gcfg_debug)
      logit("et", "Leaving K2 in streaming mode\n");

  logit("et","End of Program Summary:\n");
  gmtime_ew(&prog_start_time,&tmblk);       /* conv & log start time */
  logit("e","Program start time:  %4d%02d%02d_UTC_%02d:%02d:%02d\n",
                (tmblk.tm_year+TM_YEAR_CORR),(tmblk.tm_mon+1),tmblk.tm_mday,
                                   tmblk.tm_hour,tmblk.tm_min,tmblk.tm_sec);
  logit("e","Count totals:\n-------------\n");
  logit("e","# of packets received OK    :  %lu\n",pktin_okcount);
  logit("e","# of packets lost           :  %lu\n",
                                   (cur_dataseq-rstrt_dataseq) * g_numstms +
                                 (cur_stmnum-rstrt_stmnum) - pktin_okcount);
  logit("e","# of packets sent to EW     :  %lu\n",g_pktout_okcount);
  logit("e","# of output sequence errors :  %lu\n",g_seq_errcnt);
  if(g_seq_errtime > (time_t)0)
  {      /* last output sequence error time has been filled in */
    gmtime_ew(&g_seq_errtime,&tmblk);                 /* convert time */
    logit("e","Time of last output seq err :  "       /* log GM time */
              "%4d%02d%02d_UTC_%02d:%02d:%02d\n",
                (tmblk.tm_year+TM_YEAR_CORR),(tmblk.tm_mon+1),tmblk.tm_mday,
                                   tmblk.tm_hour,tmblk.tm_min,tmblk.tm_sec);
  }
  logit("e","Number of packet retries    :  %lu\n",retried_pckcnt);
  logit("e","Packet error rating         :  %1.3lf%%\n",
       ((pktin_okcount!=0)?(double)retried_pckcnt/pktin_okcount*100.0:0.0));
  logit("e","Missing packet(s) events    :  %lu\n",missing_errcnt);
  logit("e","Number of packets skipped   :  %lu\n",g_skip_errcnt);
  logit("e","Request resend failures     :  %lu\n",request_errcnt);
  logit("e","Number of stream resyncs    :  %lu\n",resync_errcnt);
  logit("e","Number of unexpected packets:  %lu\n",unexpect_errcnt);
  logit("e","Bad received data count     :  %lu\n",receive_errcnt);
  logit("e","Bad packet data count       :  %lu\n",packet_errcnt);

  if (cb_rc)
    k2ew_exit(1);        /* do a core dump for circ. buffer errors */
  else
    k2ew_exit(0);        /* exit program (with cleanup & 'statmgr' msg) */
  return 0;              /* "return" statement to keep compiler happy */
}


/**************************************************************************
 * k2ew_signal_hdlr:  function to handle "signal" events                  *
 *         signum - ID value of signal that caused event                  *
 **************************************************************************/

void k2ew_signal_hdlr(int signum)
{
  signal(signum,k2ew_signal_hdlr);     /* call fn to reset signal handling */
  k2ew_signal_val = signum;            /* save signal ID value */
  g_terminate_flg = 1;                 /* set flag to terminate program */
}

/**************************************************************************
 * k2ew_signal_string:  function to provide a meaningful character string *
 *                      to "signal" events which are being handled        *
 **************************************************************************/

void k2ew_signal_string( int signum, char *str, int len )
{
  if     (signum==SIGINT  ) strncpy( str, "SIGINT (ctrl-C)",                len-1 );
  else if(signum==SIGTERM ) strncpy( str, "SIGTERM (termination request)",  len-1 );
  else if(signum==SIGABRT ) strncpy( str, "SIGABRT (abnormal termination)", len-1 );
  else if(signum==SIGFPE  ) strncpy( str, "SIGFPE (arithmetic error)",      len-1 );
#ifdef SIGBREAK
  else if(signum==SIGBREAK) strncpy( str, "SIGBREAK (console closed)",      len-1 );
#endif
  else                      sprintf( str, "event (%d)", signum );
  str[len-1] = 0;
}

/**************************************************************************
 * k2ew_log_cfgparams:  logs the configuration parameters                 *
 **************************************************************************/

void k2ew_log_cfgparams()
{
  int i;

  logit("et","Configuration file command values:\n");
  switch (gen_io.mode)
  {
    case IO_COM_NT:
      logit("e","Using <COM%d> at %d baud\n",gen_io.com_io.commsel,
            gen_io.com_io.speed );
      break;
    case IO_TCP:
      logit("e", "Using TCP address <%s> and port %d\n",
            gen_io.tcp_io.k2_address, gen_io.tcp_io.k2_port );
      break;
    case IO_TTY_UN:
      logit("e", "Using TTY port <%s> at %d baud\n", gen_io.tty_io.ttyname,
            gen_io.tty_io.speed);
    case IO_NONE:
      break;
  }
  logit("e","ModuleId=\"%s\" (%d), RingName=\"%s\" (%ld), HeartbeatInt=%d\n",
                     gcfg_module_name,(int)gcfg_module_idnum,gcfg_ring_name,
                                         gcfg_ring_key,gcfg_heartbeat_itvl);
  logit("e","LogFile=%d, Debug=%d\n",gcfg_logfile_flgval,gcfg_debug);
  logit("e","CommTimeout=%d, WaitTime=%d\n",gcfg_commtimeout_itvl,
                                                          gcfg_pktwait_lim);
  logit("e","DontQuit=%d, RestartComm=%d\n",gcfg_dont_quit,
                                                     gcfg_restart_commflag);
  logit("e","RestartFile=\"%s\", MaxRestartAge=%d\n",gcfg_restart_filename,
                                                       gcfg_restart_maxage);
  logit("e","MaxBlkResends=%d, WaitResendVal=%d\n",
                                   gcfg_max_blkresends,gcfg_wait_resendval);
  logit("e","MaxReqPending=%d, ResumeReqVal=%d\n",
                                    gcfg_max_reqpending,gcfg_resume_reqval);
  logit("e","Network=\"%s\", ",gcfg_network_buff );
  logit("e","BasePinno=%d\n", gcfg_base_pinno);
  logit("e","StatusInterval=%d, ExtStatus=%d\n",gcfg_status_itvl/60,
                                                           gcfg_ext_status);
  logit("e","HighTempAlarm=%d, LowTempAlarm=%d, LowBattAlarm=%d\n",
                    gcfg_temphi_alarm,gcfg_templo_alarm,gcfg_battery_alarm);
  logit("e","OnBattery=%d, MinDiskKB=%.0lf,%.0lf\n",gcfg_on_batt,
                           gcfg_disk_alarm[0]/1024,gcfg_disk_alarm[1]/1024);
  logit("e","StationID=\"%s\"\n",gcfg_stnid_remap);
  logit("e","ChannelNames=");
  i = 0;
  while(1)
  {      /* for each channel (stream) name */
    if(gcfg_chnames_arr[i] != NULL)
      logit("e","\"%s\"",gcfg_chnames_arr[i]);
    else
      logit("e","\"\"");
    if(++i >= K2_MAX_STREAMS)     /* if no more channels then */
      break;                      /* exit loop */
    logit("e",",");               /* put in separator */
  }
  logit("e","\n");
  logit("e","LocationNames=");
  i = 0;
  while(1)
  {      /* for each channel (stream) name */
    if(gcfg_locnames_arr[i] != NULL)
      logit("e","\"%s\"",gcfg_locnames_arr[i]);
    else
      logit("e","\"\"");
    if(++i >= K2_MAX_STREAMS)     /* if no more channels then */
      break;                      /* exit loop */
    logit("e",",");               /* put in separator */
  }
  logit("e","\n");
  logit("e","InvPolFlags=");
  i = 0;
  while(1)
  {      /* for each channel (stream) */
    logit("e","%d",gcfg_invpol_arr[i]);
    if(++i >= K2_MAX_STREAMS)     /* if no more channels then */
      break;                      /* exit loop */
    logit("e",",");               /* put in separator */
  }
  logit("e","\n");
}


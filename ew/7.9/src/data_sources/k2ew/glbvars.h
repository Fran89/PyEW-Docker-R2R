/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: glbvars.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.17  2005/07/27 19:28:49  friberg
 *     2.40 changes for ForceBlockMode and comm stats
 *
 *     Revision 1.16  2005/03/26 00:17:46  kohler
 *     Version 2.38.  Added capability to get network code values from the K2
 *     headers.  The "Network" parameter in the config file is now optional.
 *     WMK 3/25/05
 *
 *     Revision 1.15  2004/06/03 16:54:01  lombard
 *     Updated for location code (SCNL) and TYPE_TRACEBUF2 messages.
 *
 *     Revision 1.14  2003/05/29 13:33:40  friberg
 *     Added InjectInfo directive global variable.
 *
 *     Revision 1.13  2002/01/17 21:58:08  kohler
 *     Changed type of g_mt_working and g_ot_working to volatile.
 *
 *     Revision 1.12  2001/10/16 22:03:56  friberg
 *     Upgraded to version 2.25
 *
 *     Revision 1.11  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.9  2001/05/23 00:18:59  kohler
 *     New config parameter: HeaderFile
 *
 *     Revision 1.8  2001/04/23 20:22:33  friberg
 *     Added station name remapping using the StationId config parameter.
 *     g_stnid_orig and g_stnid_remap char arrays added.
 *
 *     Revision 1.7  2000/11/07 19:40:15  kohler
 *     Added new global variable: gcfg_gpslock_alarm
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
 *     Revision 1.3  2000/05/16 23:40:55  lombard
 *     bug fixes, removed OutputThread Keepalive, added OnBattery alarm
 *     made alarms report only once per occurence
 *
 *     Revision 1.2  2000/05/09 23:58:49  lombard
 *     Added restart mechanism
 *
 *     Revision 1.1  2000/05/04 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */
/*  glbvars.h:  Access to global variables defined in 'k2ewmain.c' *
 *                                                                 *
 *   3/12/99 -- [ET]                                               *
 *                                                                 */

#ifndef GLBVARS_H                 /* process file only once per compile */
#define GLBVARS_H 1

#include <transport.h>       /* Earthworm shared-memory transport routines */
#include "k2pktdef.h"        /* K2 packet defs & types (for MAX_K2_CHANNELS) */
#include "k2comif.h"         /* serial and TCP communications */

                                       /* max streams allowed */
#define K2_MAX_STREAMS ((unsigned char)MAX_K2_CHANNELS)

#define K2_NETBUFF_SIZE 10   /* size of 'gcfg_network_buff[]' */
#undef MAXNAMELEN
#define MAXNAMELEN 80        /* maximum file name length */
/* constant to convert K2 date/time in seconds from Jan 1, 1980 to an */
/*  Earthworm/Unix/PC data/time in seconds from Jan 1, 1970: */
#define K2_TIME_CONV ((uint32_t)315532800)

extern char g_progname_str[];          /* global program name string */
extern int g_terminate_flg;            /* = 1 for program termination */
extern int g_numstms;                  /* number of data streams coming in */
extern int g_smprate;                  /* sample rate (& # of ents per pkt) */
extern char g_k2_stnid[STN_ID_LENGTH+1];    /* station-ID string read from K2 */
extern char g_stnid[STN_ID_LENGTH+1];       /* station-ID string in use */

extern uint32_t g_pktout_okcount;      /* # of packets sent to Earthworm OK */
extern uint32_t g_seq_errcnt;          /* number of output sequence errors */
extern uint32_t g_skip_errcnt;         /* number of packets skipped */
extern uint32_t g_trk_dataseq;         /* output thread tracked data seqence */
extern unsigned char g_trk_stmnum;     /* output thread tracked stream */
extern time_t g_seq_errtime;           /* time of last output seq err */
extern int g_validrestart_flag;        /* =1 if restart file is valid */
extern int g_extstatus_avail;          /* 0 if extended status fails */

extern SHM_INFO g_tport_region;        /* transport region struct */
extern MSG_LOGO g_heartbeat_logo;      /* Transport logo for heartbeats */
extern MSG_LOGO g_error_logo;          /* Transport logo for error messages */
extern unsigned char g_heart_ltype;
extern unsigned char g_trace_ltype;    /* logo type value for trace messages */
extern unsigned char g_error_ltype;    /* logo type value for error messages */
extern unsigned char g_instid_val;     /* installation ID value from/for EW */
extern volatile int g_mt_working;      /* =1 says main thread is working */
extern volatile int g_ot_working;      /* =1 says output thread is working */
extern unsigned g_output_threadid;     /* ID value for output thread */
extern unsigned g_hrtbt_threadid;      /* ID value for heartbeat thread */
extern pid_t g_pidval;                 /* Our PID for heartbeat messages */
extern int g_wait_count;               /* Count of waiting slots in buffer */
extern int g_req_pend;                 /* # of resend requests pending */

/* array of channel ID strings, 1 for each logical stream expected */
extern char g_stmids_arr[K2_MAX_STREAMS][CHANNEL_ID_LENGTH+1];
/* array of network code strings, 1 for each logical stream expected */
extern char g_netcode_arr[K2_MAX_STREAMS][K2_NETBUFF_SIZE+2];

/* global configuration variables set in 'getconfig.c': */
extern s_gen_io gen_io;                /* Generalized communication struct */
extern char gcfg_module_name[MAXNAMELEN+2]; /* module name for k2ew */
extern unsigned char gcfg_module_idnum;     /* module id for k2ew */
extern char gcfg_ring_name[MAXNAMELEN+2];   /* name of ring buffer */
extern int32_t gcfg_ring_key;          /* key to ring buffer k2ew dumps data */
extern int gcfg_heartbeat_itvl;        /* heartbeat interval (secs) */
extern int gcfg_logfile_flgval;        /* output to logfile enable flag */
extern char gcfg_network_buff[K2_NETBUFF_SIZE+2];  /* network name buff for EW */
extern int gcfg_base_pinno;            /* pin number for stream 0 */
extern int gcfg_status_itvl;           /* status interval (minutes) */
extern int gcfg_commtimeout_itvl;      /* communication timeout (millisecs) */
extern int gcfg_pktwait_lim;           /* maximum time to wait for packets */
extern int gcfg_max_blkresends;        /* max # resend requests per block */
extern int gcfg_max_reqpending;        /* max # of pending resend requests */
extern int gcfg_resume_reqval;         /* # blks needed to resume requests */
extern int gcfg_wait_resendval;        /* # data seq between resend reqs */
extern int gcfg_restart_commflag;      /* 1 to restart comm after timeout */
extern int gcfg_ext_status;            /* flag to get extended status msgs */
extern int gcfg_on_batt;               /* flag for sending `on battery' msg */
extern int gcfg_battery_alarm;         /* battery low-voltage alarm, 0.1 V */
extern int gcfg_extern_alarm;          /* external low-voltage alarm, 0.1 V */
extern double gcfg_disk_alarm[2];      /* disk space low alarm, bytes */
extern int gcfg_hwstat_alarm;          /* hardware status alarm flag */
extern int gcfg_templo_alarm;          /* low temperature alarm, 0.1 deg C */
extern int gcfg_temphi_alarm;          /* high temperature alarm, 0.1 deg C */
extern double gcfg_gpslock_alarm;      /* report if no GPS synch for this many hours */
extern char gcfg_restart_filename[MAXNAMELEN+2]; /* restart file name */
extern char gcfg_header_filename[MAXNAMELEN+2];  /* K2 header file name */
extern int gcfg_restart_maxage;        /* Maximum age of restart file in sec */
extern char gcfg_stnid_remap[STN_ID_LENGTH+1];   /* new STN_ID after remap */
                   /* array of "ChannelNames" remapping name strings: */
extern char *gcfg_chnames_arr[K2_MAX_STREAMS];
                   /* array of "LocationNames", one per channel */
extern char *gcfg_locnames_arr[K2_MAX_STREAMS];
                   /* array of invert-polarity flags; 1 per channel: */
extern int gcfg_lc_flag;               /* =1 to use "--" if no loc code for K2 chan;
					  =2 to die if no loc code for K2 chan */
extern int gcfg_invpol_arr[K2_MAX_STREAMS];
extern int gcfg_dont_quit;             /* =0 if we should quit on timeout */
extern int gcfg_inject_info;	       /* =1 if K2INFO packets should be sent to the ring */
extern int gcfg_force_blkmde;		/* if set to 1, then force block mode, for dual feed modem stations */
extern int gcfg_debug;                 /* k2ew debug level */

#endif


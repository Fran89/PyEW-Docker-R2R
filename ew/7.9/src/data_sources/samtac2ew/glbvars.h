/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *   
 *    $Id: glbvars.h 3552 2009-01-22 16:47:20Z tim $
 *
 * 	  Revision history:
 *     $Log$
 *     Revision 1.16  2009/01/22 16:47:20  tim
 *     *** empty log message ***
 *
 *     Revision 1.15  2009/01/22 15:59:55  tim
 *     get rid of Network from config file
 *
 *     Revision 1.14  2009/01/21 16:17:28  tim
 *     cleaned up and adjusted data collection for minimal latency
 *
 *     Revision 1.13  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *     Revision 1.12  2009/01/14 22:06:11  tim
 *     Works for other sample rates now
 *
 *     Revision 1.11  2009/01/13 17:12:53  tim
 *     Clean up source
 *
 *     Revision 1.10  2009/01/13 15:41:27  tim
 *     Removed more k2 references
 *
 *     Revision 1.9  2009/01/12 20:52:32  tim
 *     Removing K2 references
 *
 *     Revision 1.8  2009/01/08 15:05:31  tim
 *     Integrated the buffer and tested.  works
 *
 *     Revision 1.7  2008/10/29 15:44:02  tim
 *     Added Logo, typetrace, and mypid
 *
 *     Revision 1.6  2008/10/21 14:30:37  tim
 *     fat fingered K@_MAX_STREAMS
 *
 *     Revision 1.5  2008/10/21 14:29:21  tim
 *     define K@_MAX_STREAMS 10
 *
 *     Revision 1.4  2008/10/17 20:19:14  tim
 *     more cleanup
 *
 *     Revision 1.3  2008/10/17 20:17:45  tim
 *     Cleaning up unused variables in glbvars.h
 *
 *     Revision 1.2  2008/10/17 19:59:55  tim
 *     remove refrences to STN_ID_LENGTH as it is used for reading station IDs from K2
 *
 *     Revision 1.1  2008/10/17 19:14:18  tim
 *     Adding to CVS, taken from k2ew
 *
 *    
 */


#ifndef GLBVARS_H                 /* process file only once per compile */
#define GLBVARS_H

#include <transport.h>       /* Earthworm shared-memory transport routines */
#include "samtac_comif.h"         /* serial and TCP communications */

                                       /* max streams allowed */

#undef MAXNAMELEN
#define MAXNAMELEN 80        /* maximum file name length */
/* constant to convert K2 date/time in seconds from Jan 1, 1980 to an */
/*  Earthworm/Unix/PC data/time in seconds from Jan 1, 1970: */
//#define K2_TIME_CONV ((unsigned long)315532800)

#define PACKET_MAX_SIZE  13519
#define PACKET_HEADER_SIZE 17

extern unsigned char g_samtac_buff[PACKET_MAX_SIZE]; /* received data buffer */
extern unsigned char samtac_initial_buffer[PACKET_MAX_SIZE * 3]; /* received data buffer */

extern char g_progname_str[];          /* global program name string */
extern int g_terminate_flg;            /* = 1 for program termination */

extern SHM_INFO g_tport_region;        /* transport region struct */
extern MSG_LOGO g_heartbeat_logo;      /* Transport logo for heartbeats */
extern MSG_LOGO g_error_logo;          /* Transport logo for error messages */
extern MSG_LOGO DataLogo;               /* EW logo tag  for data */
extern MSG_LOGO SOHLogo;                /* EW logo tag  for TYPE_GCFSOH_PACKET */

extern unsigned char g_heart_ltype;
extern unsigned char TypeTrace;         /* Trace EW type for logo */
extern unsigned char TypeTrace2;        /* Trace2  EW type for logo */
extern unsigned char TypeErr;           /* Error EW type for logo */
extern unsigned char TypeSAMTACSOH;        /* EW TYPE_GCFSOH_PACKET type for logo */

/* global configuration variables set in 'getconfig.c': */
extern s_gen_io gen_io;                /* Generalized communication struct */
extern char gcfg_module_name[MAXNAMELEN+2]; /* module name for samtac2ew */
extern unsigned char gcfg_module_idnum;     /* module id for samtac2ew */
extern char gcfg_ring_name[MAXNAMELEN+2];   /* name of ring buffer */
extern long gcfg_ring_key;             /* key to ring buffer samtac2ew dumps data */
extern int gcfg_heartbeat_itvl;        /* heartbeat interval (secs) */
extern int gcfg_logfile_flgval;        /* output to logfile enable flag */
extern int gcfg_commtimeout_itvl;      /* communication timeout (millisecs) */
extern int gcfg_debug;                 /* samtac2ew debug level */
extern pid_t MyPid;

extern unsigned int gcfg_deviceID;		//DeviceID(serial) as read from config
extern char device_id[2];

extern int last_found_packetstart;

extern double SOH_itvl;		//5 minutes is default(300 seconds)


#endif


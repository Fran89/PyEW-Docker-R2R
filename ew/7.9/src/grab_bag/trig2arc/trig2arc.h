
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: trig2arc.h,v 1.1 2010/01/01 00:00:00 jmsaurel Exp $
 *
 *    Revision history:
 *     $Log: trig2arc.h,v $
 *     Revision 1.1 2010/01/01 00:00:00 jmsaurel
 *     Initial revision
 *
 *
 */

#ifndef TRIG2HYP_H
#define TRIG2HYP_H

/*
 * trig2arc.h : Include file for trig2arc.c
 */

#include <trace_buf.h>
#include <chron3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <kom.h>
#include "statrig.h"

#define MAX_STR          255
#define MAXLINE          1000
#define MAX_NAME_LEN     50
#define MAXCOLS          100
#define MAXTXT           150
#define MAX_STATIONS     1024
#define LINE_LEN     	 500

static int     Debug = 0;          /* 0=no debug msgs, non-zero=debug   */

/* Globals
 *********/
static SHM_INFO  InRegion;    /* shared memory region to use for input  */
static SHM_INFO  OutRegion;   /* shared memory region to use for output */
static pid_t	 MyPid;       /* Our process id is sent with heartbeat  */

/* Things to read or derive from configuration file
 **************************************************/
static int     LogSwitch;          /* 0 if no logfile should be written */
static long    HeartbeatInt;       /* seconds between heartbeats        */
static MSG_LOGO *GetLogo = NULL;   /* logo(s) to get from shared memory */
static short   nLogo   = 0;        /* # logos we're configured to get   */
static int     UseOriginalLogo=0;  /* 0=use trig2arc's own logo on output msgs   */
                                   /* non-zero=use original logos on output      */
                                   /*   NOTE: this requires that output go to    */
                                   /*   different transport ring than input      */
float  UseLatitude=0; 	   /* Event latitude in the hyp2000_arc message  */
float  UseLongitude=0;      /* Event longitude in the hyp2000_arc message  */
float  UseDepth=0;	   /* Event depth in the hyp2000_arc message  */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input   */
static long          OutRingKey;    /* key of transport ring for output  */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeTrig;
static unsigned char TypeHyp2000Arc;

/* Error messages used by trig2arc
 *********************************/
#define  ERR_MISSGAP       0   /* sequence gap in transport ring         */
#define  ERR_MISSLAP       1   /* missed messages in transport ring      */
#define  ERR_TOOBIG        2   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       3   /* msg retreived; tracking limit exceeded */
static char  Text[MAXTXT];        /* string for log/error messages          */

#endif

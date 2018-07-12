/* THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU CHECKED IT OUT!
 *
 *  $Id: coda_dur.h 498 2009-12-17 19:07:23Z dietz $
 * 
 *  Revision history:
 *   $Log$
 *   Revision 1.3  2009/12/17 19:07:23  dietz
 *   Modified to look at only newly-arrived CAAVs when scanning for coda
 *   termination value.
 *
 *   Revision 1.2  2009/12/08 18:46:21  dietz
 *   Removed MinCodaLen parameter (was never used).
 *   Added logic to stop coda processing for gappy or dead traces.
 *
 *   Revision 1.1  2009/11/09 19:16:15  dietz
 *   Initial version, may still contain bugs and debugging statements
 *
 */

/******************************************************************
 *                         File coda_dur.h                        *
 ******************************************************************/
#ifndef _CODA_DUR_H
#define _CODA_DUR_H

#include <trace_buf.h>
#include <rw_coda_aav.h>
#include <rdpickcoda.h>

#define MAX_PICK             5  /* number of picks to buffer per SCNL    */
#define MAX_CAAV           180  /* number of CAAVs to buffer per SCNL    */

/* Information about picks being processed
 *****************************************/
typedef struct _PKINFO {
/* Info from the TYPE_PICK_SCNL message */
   unsigned char msgtype;  /* one-byte number message type         */
   unsigned char modid;    /* one-byte number module id            */
   unsigned char instid;   /* one-byte number installation id      */
   int           seq;      /* sequence number                      */
   double        tpick;    /* time of pick - seconds since 1970    */
/* Info used by coda_dur in processing this pick */
   time_t        tstop;    /* system-time to give up looking for this coda */
   int           pkstat;   /* current state of this pick in list           */
   int           nlow;     /* number of consecutive "imcomplete" CAAVs     */
   unsigned long icaav;    /* index of CAAV window containing pick time    */
   unsigned long lcaav;    /* index of last-inspected CAAV window          */
   unsigned long ecaav;    /* index of coda-ending CAAV window             */ 
   double        cterm;    /* coda termination value to use for this pick  */
} PKINFO;

/* Station list parameters
 *************************/
typedef struct {
/* fields read from pick_ew config file */
   char   sta[TRACE2_STA_LEN];    /* Station name                            */
   char   chan[TRACE2_CHAN_LEN];  /* Component code                          */
   char   net[TRACE2_NET_LEN];    /* Network code                            */
   char   loc[TRACE2_LOC_LEN];    /* Location code                           */
   double CodaTerm;         /* cdur: "normal" Coda termination threshold     */
                            /*       (in counts)                             */
   double AltCoda;          /* cdur: if pre-pick-AAV >= AltCoda*CodaTerm,    */
                            /*       use alternate coda termination value    */
                            /*       AltCoda is usually ~0.8 (always <1.0)   */
   double PreEvent;         /* cdur: define alternate coda termination value */
                            /*       as pre-pick-AAV*PreEvent, where         */
                            /*       PreEvent is usually ~1.2 (always >1.0)  */
/* Circular buffer of coda avg absolute values */
   CAAV          *caav;     /* circular buffer of caav values for this SCNL  */
   unsigned long  lcaav;    /* cumulative # of caavs seen for this SCNL      */
/* Circular buffer of picks */
   PKINFO        *pk;       /* circular buffer of picks for this SCNL        */ 
   unsigned long  lpk;      /* cumulative # of picks seen for this SCNL      */
} STATION;

#define STAFILE_LEN 64
typedef struct {
   char   name[STAFILE_LEN]; /* Name of station file */
   int    nsta;              /* number of channels configure in this file */
} STAFILE;

typedef struct {
   STAFILE  *StaFile;       /* Name of file(s) with SCNL info */
   int       nStaFile;      /* Number of StaFile commands given */
   long      CaKey;         /* Key to ring where caav's live */
   long      PkKey;         /* Key to ring where picks/codas live */
   int       HeartbeatInt;  /* Heartbeat interval in seconds */
   int       Debug;         /* If 1, print debug messages */
   unsigned char MyModId;   /* Module id of this program */
   SHM_INFO  CaRegion;      /* Info structure for region w/CAAV msgs */
   SHM_INFO  PkRegion;      /* Info structure for input pick region */
   int       nGetCaLogo;    /* Number of logos in GetCaLogo   */
   MSG_LOGO *GetCaLogo;     /* Logos of requested caav's */
   int       nGetPkLogo;    /* Number of logos in GetPkLogo   */
   MSG_LOGO *GetPkLogo;     /* Logos of requested picks */
   unsigned long mCaav;     /* maximum number of CAAVs to store per SCNL */
   unsigned long mPick;     /* maximum number of Picks to store per SCNL */
} GPARM;

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char InstIdWild;      /* Wildcard for inst id */
   unsigned char ModIdWild;       /* Wildcard for module id */
   unsigned char TypeHeartBeat;
   unsigned char TypeError;
   unsigned char TypeCodaAAV;
   unsigned char TypePickSCNL;    
   unsigned char TypeCodaSCNL;  
} EWH;

#endif

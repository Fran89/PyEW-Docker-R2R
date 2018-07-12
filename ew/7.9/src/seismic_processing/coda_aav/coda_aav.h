/* THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU CHECKED IT OUT!
 *
 *  $Id: coda_aav.h 490 2009-11-09 19:16:15Z dietz $
 * 
 *  Revision history:
 *   $Log$
 *   Revision 1.1  2009/11/09 19:15:28  dietz
 *   Initial version, may still contain bugs and debugging statements
 *
 */

/******************************************************************
 *                         File coda_aav.h                        *
 ******************************************************************/
#ifndef _CODA_AAV_H
#define _CODA_AAV_H

#include <trace_buf.h>

/* Station list parameters
 *************************/
typedef struct {
/* fields read from coda_aav config file */
   char   sta[TRACE2_STA_LEN];    /* Station name                            */
   char   chan[TRACE2_CHAN_LEN];  /* Component code                          */
   char   net[TRACE2_NET_LEN];    /* Network code                            */
   char   loc[TRACE2_LOC_LEN];    /* Location code                           */
   double RawDataFilt;      /* caav: Filter parameter for raw data           */
/* Variables filled in from trace header (don't change much) */
   int    ready;            /* =0 until this channel is found & initialized  */
   double samprate;         /* sample rate for this channel sample/sec       */
   double sampintrvl;       /* sample interval for this channel sec/sample   */
   int    nbin;             /* expected number of samples in coda AAV bin    */
/* Variables to keep track of coda AAV calculations */ 
   double    told;          /* time of old_sample             */
   int       old_sample;    /* Old value of integer data      */
   double    rdat;          /* filtered data value            */
   double    tstart;        /* start time of this running sum */
   double    tend;          /* end time of this running sum   */
   double    rsrdat;        /* Running sum of rdat for coda AAV calculation   */
   int       ndat;          /* actual number of data points in current rsrdat */
} STATION;

#define STAFILE_LEN 64
typedef struct {
   char   name[STAFILE_LEN]; /* Name of station file */
   int    nsta;              /* number of channels configure in this file */
} STAFILE;

typedef struct {
   STAFILE  *StaFile;       /* Name of file(s) with SCNL info */
   int       nStaFile;      /* Number of StaFile commands given */
   long      InKey;         /* Key to ring where waveforms live */
   long      OutKey;        /* Key to ring where coda_aavs will live */
   int       HeartbeatInt;  /* Heartbeat interval in seconds */
   int       Debug;         /* If 1, print debug messages */
   unsigned char MyModId;   /* Module id of this program */
   SHM_INFO  InRegion;      /* Info structure for input region */
   SHM_INFO  OutRegion;     /* Info structure for output region */
   int       nGetLogo;      /* Number of logos in GetLogo   */
   MSG_LOGO *GetLogo;       /* Logos of requested waveforms */
} GPARM;

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char InstIdWild;      /* Wildcard for inst id */
   unsigned char ModIdWild;       /* Wildcard for module id */
   unsigned char TypeHeartBeat;
   unsigned char TypeError;
   unsigned char TypeCodaAAV;
   unsigned char TypeTracebuf;    /* Waveform buffer for data input (no loc code) */
   unsigned char TypeTracebuf2;   /* Waveform buffer for data input (w/loc code) */
} EWH;

#endif

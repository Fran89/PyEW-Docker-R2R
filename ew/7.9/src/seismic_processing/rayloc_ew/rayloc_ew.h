/***************************************************************************
 *  This code is a part of rayloc_ew / USGS EarthWorm module               *
 *                                                                         *
 *  It is written by ISTI (Instrumental Software Technologies, Inc.)       *
 *          as a part of a contract with CERI USGS.                        *
 * For support contact info@isti.com                                       *
 *   Ilya Dricker (i.dricker@isti.com)                                     *
 *                                                   Aug 2004              *
 ***************************************************************************/


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rayloc_ew.h 1669 2004-08-05 04:15:11Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2004/08/05 04:15:11  friberg
 *     First commit of rayloc_ew in EW-CENTRAL CVS
 *
 *     Revision 1.5  2004/08/04 19:27:54  ilya
 *     Towards version 1.0
 *
 *     Revision 1.4  2004/06/24 16:47:05  ilya
 *     Version compiles
 *
 *     Revision 1.3  2004/06/24 16:32:06  ilya
 *     global_msg.h
 *
 *     Revision 1.2  2004/06/24 16:15:07  ilya
 *     Integration phase started
 *
 *     Revision 1.1.1.1  2004/06/22 21:12:06  ilya
 *     initial import into CVS
 *
 */

/******************************************************************
 *                         File rayloc_ew.h                         *
 ******************************************************************/
#include "rayloc1.h"

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char GetThisInstId;   /* Get messages from this inst id */
   unsigned char GetThisModId;    /* Get messages from this module */
   unsigned char TypeHeartBeat;
   unsigned char TypeError;
   unsigned char TypePick2k;
   unsigned char TypeCoda2k;
   unsigned char TypeWaveform;    /* Waveform buffer for data input */
} EWH;

typedef struct {
   char StaFile[1024];             /* Name of file with SCN info */
   char workDirName[1024];        /* Dirname of working directory */
   long InKey;                    /* Key to ring where waveforms live */
   long OutKey;                   /* Key to ring where picks will live */
   int  HeartbeatInt;             /* Heartbeat interval in seconds */
/*   int  RestartLength;   */         /* Number of samples to process for restart */
/*   int  MaxGap;    */               /* Maximum gap to interpolate */
   int  Debug;                    /* If 1, print debug messages */
   unsigned char MyModId;         /* Module id of this program */
   SHM_INFO InRegion;              /* Info structure for input region */
   SHM_INFO OutRegion;            /* Info structure for output region */
} GPARM;

/* Function prototypes
   *******************/
	
/* Private functions */
int rayloc_ew_GetConfig( char *, GPARM *, RAYLOC_PROC_FLAGS *);
void rayloc_ew_LogConfig( GPARM * );
int  rayloc_ew_GetEwh( EWH *);

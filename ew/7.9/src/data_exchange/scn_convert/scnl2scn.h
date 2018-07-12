/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: scnl2scn.h 1692 2004-10-19 21:59:41Z lombard $
 *
 */

/******************************************************************
 *                        File scn2scnl.h                         *
 ******************************************************************/
#ifndef SCNL2SCN_H
#define SCNL2SCN_H


#include <earthworm.h>

typedef struct
{
   char MyModName[MAX_MOD_STR];  /* Module name */
   char InRing[MAX_RING_STR];    /* Name of ring containing input messages */
   char OutRing[MAX_RING_STR];   /* Name of ring containing output messages */
   long InKey;                   /* Key to input ring */
   long OutKey;                  /* Key to output ring */
   int  HeartbeatInt;            /* Heartbeat interval in seconds */
} GPARM;


#endif

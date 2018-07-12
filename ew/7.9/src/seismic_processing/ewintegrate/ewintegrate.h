
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewintegrate.h,v 1.6 2004/05/11 18:53:40 dietz Exp $
 *
 *    Revision history:
 *
 *
 
 ewintegrate: Sample xfrm module's header file
     
 */

/*
 * ewintegrate.h: Definitions for the Your Earthworm Module.
 */

/*******                                                        *********/
/*      Redefinition Exclusion                                          */
/*******                                                        *********/
#ifndef __EWINTEGRATE_H__
#define __EWINTEGRATE_H__

#include <xfrm.h> 
#include "butterworth_c.h"

/*******                                                        *********/
/*      Constant Definitions                                            */
/*******                                                        *********/

/*******                                                        *********/
/*      Structure Definitions                                           */
/*******                                                        *********/

  /*    Debias Parameter Structure (read from *.d files)                */
typedef struct _INTPARAM
{
#include <xfrm_param.h>              /* Common fields -- MUST BE FIRST  */
  /* Fields specific to debias */
  int    avgWindowSecs;          /* Size of averaging window in seconds */
  int    doIntegration;                      /* = "perform integration" */
  int    hpOrder;                 /* Order of optional high pass filter */
  double hpFreqCutoff;      /* Cutoff freq of optional high pass filter */
} INTPARAM;

  /*    Information Retrieved from Earthworm*.h                         */
typedef struct _INTEWH
{
#include <xfrm_ewh.h>                /* Common fields -- MUST BE FIRST  */
  /* Fields specific to debias */
} INTEWH;

  /*    Decimate World structure                                        */
typedef struct _INTWORLD
{
  /* FIRST 3 LINES MUST BE THESE 3                                      */
  INTEWH      *xfrmEWH;         /* Structure for Earthworm parameters.  */
  INTPARAM    *xfrmParam;       /* Network parameters.                  */
#include <xfrm_world.h>         /* Other Common fields                  */
  /* Fields specific to debias */
  mutex_t      completionMutex; /* To wait for after last packet        */
} INTWORLD;

typedef struct _HP_STAGE
{
  double d1, d2;
  double f;
  double a1, a2, b1, b2;
} HP_STAGE;

  /*    SCNL structure specific to this filter                        */
typedef struct _INTSCNL
{
  /* FIRST LINE MUST BE THIS                                      */
#include <xfrm_scnl.h>         /* Other Common fields                  */
  /* Fields specific to ewintegrate */
  TRACE2_HEADER *outBuf;        /* outbuf                            */
  int nHPLevels;                /* # of filter levels                */
  HP_STAGE *hpLevel;            /* array of filter level defns       */
  double hpFreq;                /* filter frequency parameter        */
  int hpOrder;                  /* filter order parameter            */
  double period;                /* sample period (secs)              */
  double prev_acc, prev_vel;    /* previous accel & velocity         */
  double hp_gain;               /* needed for filter calc            */
  complex *poles;               /* needed for filter calc            */
  TRACE2_HEADER *outBufHdr;     /* headers for cached outbufs        */
  int obhAllocated;             /* outbuf headers allocated          */
  int nCachedOutbufs;           /* # of cached outbufs               */
  void* avgWindow;              /* rolling average window            */
  int awPosn;                   /* index of avgWindow to fill next   */
  int awBytesAllocated;         /* bytes allocated to avgWindow      */
  int awSamples;                /* samples needed for avgWindow      */
  int awFull;                   /* = "avgWindow is full"             */
  size_t awDataSize;            /* size of a single sample           */
  double sum;                   /* sum of samples in avgWindow       */
  int awEmpty;                  /* = "avgWindow is empty"            */
} INTSCNL;


#endif  /*  __EWINTEGRATE_H__                                                */

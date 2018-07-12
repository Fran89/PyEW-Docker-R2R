
/*
 debias: removes a mean value over a specified time window
			it DOES NOT perform any other actions on the data.
 */

/*
 * debias.h: Definitions for the Debias Earthworm Module.
 */

#define VERSION_ID "1.0.1 2014.02.20"

/*******                                                        *********/
/*      Redefinition Exclusion                                          */
/*******                                                        *********/
#ifndef __DEBIAS_H__
#define __DEBIAS_H__

#include <xfrm.h> 

/*******                                                        *********/
/*      Constant Definitions                                            */
/*******                                                        *********/


/*    FilterDecimate Error Codes, returned from DoOneStage              */
#define FD_NOROOM        -1     /* No room in output buffer             */


/*******                                                        *********/
/*      Structure Definitions                                           */
/*******                                                        *********/

  /*    Debias Parameter Structure (read from *.d files)                */
typedef struct _DBPARAM
{
#include <xfrm_param.h>              /* Common fields -- MUST BE FIRST  */
  /* Fields specific to debias */
  int   minTraceBufLen;              /* Length of smallest TRACEBUF     */
  int   avgWindowSecs;           /* Size of averaging window in seconds */
} DBPARAM;

  /*    Information Retrieved from Earthworm*.h                         */
typedef struct _DBEWH
{
#include <xfrm_ewh.h>                /* Common fields -- MUST BE FIRST  */
  /* Fields specific to debias */
} DBEWH;

  /*    Decimate World structure                                        */
typedef struct _DBWORLD
{
  /* FIRST 3 LINES MUST BE THESE 3                                      */
  DBEWH       *xfrmEWH;         /* Structure for Earthworm parameters.  */
  DBPARAM     *xfrmParam;       /* Network parameters.                  */
#include <xfrm_world.h>         /* Other Common fields                  */
  /* Fields specific to debias */
} DBWORLD;

  /*    Decimate World structure                                        */
typedef struct _DBSCNL
{
  /* FIRST LINE MUST BE THIS                                      */
#include <xfrm_scnl.h>         /* Other Common fields                  */
  /* Fields specific to debias */
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
} DBSCNL;


#endif  /*  __DEBIAS_H__                                                */

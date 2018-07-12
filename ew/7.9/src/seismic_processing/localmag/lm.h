/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm.h 5193 2012-11-13 16:43:26Z quintiliani $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.9  2005/08/15 15:30:54  friberg
 *     version 2.0.3, added in notUsed flag to PCOMP1 to indicate that the
 *     channels from this component set were not used. This can only
 *     happen currently because of the require2Horizontals configuration
 *     parameter.
 *
 *     Revision 1.8  2002/10/29 18:48:11  lucky
 *     Added origin version number tracking.. this is how we associate mags to
 *     origins.
 *
 *     Revision 1.7  2002/02/04 17:02:11  lombard
 *     Added checks for short traces (such as might come from wave_server
 *     for a new event).
 *
 *     Revision 1.6  2002/01/24 19:34:09  lombard
 *     Added 5 percent cosine taper in time domain to both ends
 *     of trace data. This is to eliminate `wrap-around' spikes from
 *     the pre-event noise-check window.
 *
 *     Revision 1.5  2001/03/01 05:25:44  lombard
 *     changed FFT package to fft99; fixed bugs in handling of SCNPars;
 *     changed output to Magnitude message using rw_mag.c
 *
 *     Revision 1.4  2001/01/15 03:55:55  lombard
 *     bug fixes, change of main loop, addition of stacker thread;
 *     moved fft_prep, transfer and sing to libsrc/util.
 *
 *     Revision 1.3  2000/12/31 17:27:25  lombard
 *     More bug fixes and cleanup.
 *
 *     Revision 1.2  2000/12/25 22:14:39  lombard
 *     bug fixes and development
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/* Include file for localmag program */

#ifndef LM_H
#define LM_H

#include <trace_buf.h>
#include <transfer.h>

/* Various macros */
/*                  Length of the component name including null terminator */
#define LM_COMP_LEN 4
/*                  Length of the location code name including null terminator */
#define LM_LOC_LEN 3
/*                  Value to indicate no magnitude has been calculated */
#define NO_MAG -99.0
/*                  Maximum expected trace length in seconds;        *
 *                  so we can declare arrays instead of using malloc */
#define MAX_TRACE_SEC 600
#define MAX_SAMPRATE 200
#ifndef PATH_MAX
#define PATH_MAX 512
#endif

/* fraction of the trace length that will be used for time-domain taper */
#define TD_TAPER 0.10

/*
 * logA0 table; one element of.
 */
#define  NOINITVAL -100
typedef struct _LOGA0
{
  int dist;
  double val;  /* horizontal component attenuation */
  double val2; /* vertical component attenuation */
} LOGA0;

/*
 * Description of a data gap.
 * Note: if a gap would be declared at end of data, the data must be 
 * truncated instead of adding another GAP structure. A gap may be
 * declared at the start of the data, however.
 */
typedef struct _GAP *PGAP;
typedef struct _GAP 
{
  double starttime;  /* time of first sample in the gap                      */
  double gapLen;     /* time from first gap sample to first sample after gap */
  long firstSamp;    /* index of first gap sample in data buffer             */
  long lastSamp;     /* index of last gap sample in data buffer              */
  PGAP next;         /* The next gap structure in the list                   */
} GAP;

/* Structure for keeping track of buffer of trace data */
typedef struct _DATABUF 
{
  double *rawData;   /* The raw trace data; native byte order                */
  double *procData;  /* the processed data                                   */
  double delta;      /* The nominal time between sample points               */
  double starttime;  /* time of first sample in raw data buffer              */
  double endtime;    /* time of last sample in raw data buffer               */
  long nRaw;         /* number of samples in raw data buffer, including gaps */
  long lenRaw;       /* length to the rawData array                          */
  long nProc;        /* number of samples in processed data buffer           */
  long lenProc;      /* length of the procData array                         */
  long padLen;       /* Number of padded samples due to convolution          */
  GAP *gapList;      /* linked list of gaps in raw data                      */
  int nGaps;         /* number of gaps found in raw data                     */
} DATABUF;

typedef struct _SCNLPAR {
  double magCorr;
  double fTaper[4];
  double clipLimit;
  char sta[TRACE_STA_LEN];
  char comp[TRACE_CHAN_LEN];
  char net[TRACE_NET_LEN];
  char loc[TRACE_LOC_LEN];
} SCNLPAR;

/* Macros for the three component directions */
#define LM_E 0
#define LM_N 1
#define LM_Z 2
/* The direction part of the component structure */
typedef struct _COMP3
{
  double peakWinStart; /* starttime of peak-search window                  */
  double peakWinEnd;  /* end time of peak-search window                    */
  double p2pAmp;      /* Peak-to-peak amplitude of Wood-Anderson trace     */
  double p2pMin;      /* Amplitude on negative side of peak-to-peak        */
  double p2pMax;      /* Amplitude on positive side of peak-to-peak        */
  double p2pMinTime;  /* Time of min side of peak-to-peak amplitude        */
  double p2pMaxTime;  /* Time of max side of peak-to-peak amplitude        */
  double z2pAmp;      /* Zero-to-peak amplitude                            */
  double z2pTime;     /* Zero-to-peak time                                 */
  double mag;         /* Local magnitude for this component/direction      */
  double mag_corr;    /* local magnitude correction, to be added           */
  int priority;       /* priority-level for this trace 1 is high, 3 is low */
  int BadBitmap;      /* bitmap to indicate why this component is bad      */
  SCNLPAR *pSCNLPar;    /* parameters for this SCNL                        */
  char name[4];       /* The full 3-character null termed component name   */
  char loc[3];        /* The full 2-character  null termed location code   */
} COMP3;

/*
 * The non-directional part of the component structure;
 *  zero or more per station.
 */
typedef struct _COMP1 *PCOMP1;
typedef struct _COMP1
{
  COMP3 c3[3];        /* The three direction structures                   */
  double mag;         /* Local magnitude for this 2-letter component      */
  PCOMP1 next;         /* The next component in linked list; NULL if none */
  int notUsed;		/* set to 1 if not used in the magnitude calculation (for whatever reason) */
  char n2[LM_COMP_LEN];  /* First 2 chars of component name, the C in SCNL */
  char loc[LM_LOC_LEN]; /* location code that needs part of any n2 match */
} COMP1;

/* The station structure, one or more per event */
typedef struct _STA
{
  double lat;         /* Station latitude; south is negative */
  double lon;         /* Station longitude; west is negative */
  double dist;        /* Epicentral distance in kilometers   */
  double mag;         /* Magnitude for this station */
  double p_est;       /* Estimated P arrival                 */
  double s_est;       /* Estimated S arrival                 */
  double timeTaper;   /* time in seconds for time-domain taper at each end */
  COMP1 *comp;        /* List of components for this station */
  char sta[TRACE_STA_LEN];     /* Station name, the S in SCNL */
  char net[TRACE_NET_LEN];     /* Network name, the N in SCNL */
} STA;

/*
 * The event structure, one per event. Note this structure holds only the
 * information needed by the localmag module!
 */
typedef struct _EVT
{
  char eventId[EVENTID_SIZE+1];  /* The event ID string from archive message */
  char author[AUTHOR_FIELD_SIZE+1]; /* The author ID, from msg logos         */
  double origin_time; /* The origin time, seconds since midnight, 1 Jan 1970 */
  double lat;         /* Hypocenter latitude; south is negative              */
  double lon;         /* Hypocenter longitude; west is negative              */
  double depth;       /* Hypocenter depth in kilometers                      */
  double mag;         /* Local magnitude for this event                      */
  double magMed;      /* Median magnitude for this event                     */
  double sdev;        /* Standard deviation of magnitudes                    */
  int nMags;          /* Number of station magnitudes in event               */
  STA *Sta;           /* Array of stations for this event                    */
  int numSta;         /* Number of filled STA structures                     */
  int origin_version; /* Origin version for this event                       */
  int qdds_version;   /* QDDS version for this event                         */
  /* Further information */
  struct Hpck *ArcPck;       /* Array of phases in ARC message                      */
  int numArcPck;      /* Number of ArcPck structures                         */
  int maxArcPck;      /* Max number of phases allocated in ArcPck            */
} EVENT;
  

/* Bad Bits: used to indicate why a component trace should not be used */
/*                      Trace is clipped and clipping is not allowed */
#define LM_BAD_CLIP  1<<0
/*                      Trace has a gap within the peak-search window */
#define LM_BAD_GAP   1<<1
/*                      Trace is too short to process */
#define LM_BAD_SHORT 1<<2
/* 			Trace had a bad Z2P threshold */
#define LM_BAD_Z2P   1<<3
/* 			Trace had all zero values */
#define LM_BAD_ZEROS 1<<4

#endif


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: decimate.h 6325 2015-05-01 00:44:09Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2004/05/11 18:53:40  dietz
 *     fixed typo
 *
 *     Revision 1.5  2004/05/11 18:14:17  dietz
 *     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
 *
 *     Revision 1.4  2002/10/25 17:59:16  dietz
 *     Added support for multiple GetWavesFrom commands
 *
 *     Revision 1.3  2000/07/24 20:39:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.2  2000/02/15 18:10:46  bogaert
 *     Changed QUEUE_SIZE from 100 to 300 for AEIC
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * decimate.h: Definitions for the Decimate Earthworm Module.
 */

/*******                                                        *********/
/*      Redefinition Exclusion                                          */
/*******                                                        *********/
#ifndef __DECIMATE_H__
#define __DECIMATE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <transport.h>  
#include <trace_buf.h>
#include <mem_circ_queue.h>

/* Flag to adjust the time lag from the FIR filter to zero.             */
/* Used in FiltOneSCN and some other places.                            */
#define FIXTIMELAG

/*******                                                        *********/
/*      Constant Definitions                                            */
/*******                                                        *********/


/*    StatusReport Error Codes - Must coincide with decimate.desc       */
#define  ERR_MISSMSG       0   /* message missed in transport ring      */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer    */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_DECIM         3   /* call to the decimation routine failed */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

/*    FilterDecimate Error Codes, returned from DoOneStage              */
#define FD_NOROOM        -1     /* No room in output buffer             */

/*    Buffer Lengths                                                    */
#define MAXFILENAMELEN  80      /* Maximum length of a file name.       */
#define MAXLINELEN      240     /* Maximum length of a line read from   */
                                /*   or written to a file.              */
#define MAXMESSAGELEN   160     /* Maximum length of a status or error  */
                                /*   message.                           */
#define MAX_LOGO        10      /* Maximum number of logos to listen to */
#define BUFFSIZE MAX_TRACEBUF_SIZ * 2 /* Size of a channel trace buffer */
#define MAXSTAGE        10      /* Maximum number of decimation stages  */
#define	QUEUE_SIZE	300	/* How many msgs can we queue           */
#define THREAD_STACK    8192    /* How big is our thread stack          */

  /*    Filter design coefficients                                      */
#define PASS_RIPPLE 0.005       /* Filter design criteria               */
#define STOP_RIPPLE 0.0031      /* Perhaps these should be in config file */
#define PASS_ALLOW  0.8         /* Fraction of Nyquist freq for pass-band */
                                /*   This must be less than 1.0         */

/*******                                                        *********/
/*      Structure Definitions                                           */
/*******                                                        *********/

  /*    Decimate Parameter Structure (read from *.d files)              */
typedef struct _DCMPARAM
{
  char  myModName[MAX_MOD_STR];         /* Name of this instance of the */
                                        /*   Decimate module -          */
                                        /*   REQUIRED.                  */
  char  readInstName[MAX_LOGO][MAX_INST_STR];  /* Name of installation  */
                                        /*   that is producing trace    */
                                        /*   data messages.             */
  char  readModName[MAX_LOGO][MAX_MOD_STR];   /* Name of module at the  */
                                        /*   above installation that is */
                                        /*   producing the trace data   */
                                        /*   messages.                  */
  char  readTypeName[MAX_LOGO][MAX_TYPE_STR]; /* Type of trace data msg */
                                        /*   above module is producing. */
  char  ringIn[MAX_RING_STR];           /* Name of ring from which      */
                                        /*   trace data will be read -  */
                                        /* REQUIRED.                    */
  char  ringOut[MAX_RING_STR];          /* Name of ring to which        */
                                        /*   triggers will be written - */
                                        /* REQUIRED.                    */
  int   nlogo;                          /* number of instid/modid pairs */
  int   heartbeatInt;                   /* Heartbeat Interval(seconds). */
  int   logSwitch;                      /* Write logs?                  */
                                        /*   ( 0 = No, 1 = Yes )        */
  int   debug;                          /* Write out debug messages?    */
                                        /*   ( 0 = No, 1 = Yes )        */
  int   quiet;                          /* SHUT UP: don't warn me about gaps/overlaps to stderr */
                                        /*   ( 0 = No (default), 1 = Yes )        */
  int   minTraceBufLen;                 /* Length of smallest TRACEBUF  */
  int   cleanStart;                     /* Clean Start flag             */
                                        /*   ( 0 = No, 1 = Yes )        */
  int   testMode;                       /* Test mode flag               */
                                        /*   ( 0 = No, 1 = Yes )        */
  double maxGap;                        /* Gap between tracedata points */
} DCMPARAM;

  /*    Information Retrieved from Earthworm*.h                         */
typedef struct _DCMEWH
{
  unsigned char myInstId;       /* Installation running this module.    */
  unsigned char myModId;        /* ID of this module.                   */
  unsigned char readInstId[MAX_LOGO];  /* Retrieve trace messages from  */
                                /*   specified installation(s).         */
  unsigned char readModId[MAX_LOGO];   /* Retrieve trace messages from  */
                                /*    specified module(s).              */
  unsigned char readMsgType[MAX_LOGO];  /* Retrieve trace messages of   */
                                /*    specified type.                   */
  unsigned char typeError;      /* Error message type.                  */
  unsigned char typeHeartbeat;  /* Heartbeat message type.              */
  unsigned char typeTrace;      /* Original trace message type (SCN).   */
  unsigned char typeTrace2;     /* New trace message type (SCNL).       */
  long  ringInKey;                      /* Key to input shared memory   */
                                        /*   region.                    */
  long  ringOutKey;                     /* Key to output shared memory  */
                                        /*   region.                    */
} DCMEWH;

typedef struct _BUFF
{
  double buffer[BUFFSIZE];
  double starttime;              /* time of first datapoint in buffer   */
  double samplerate;             /* sample rate for this buffer         */
  int    read;                   /* Index to start reading from         */
  int    write;                  /* Index to start writing to           */
} BUFFER; 

typedef struct _FILTER
{
  int decRate;                  /* Decimation rate of this stage        */
  int Length;                   /* Length of the symmetric filter       */
  double *coef;                 /* Array of coefficients for this filter*/
} FILTER;

typedef struct _STAGE *PSTAGE;
typedef struct _STAGE
{
  FILTER  *pFilt;               /* Pointer to the filter for this stage */  
  BUFFER  inBuff;               /* The input buffer for this stage      */
                                /* for output, use the buffer of the    */
                                /* following stage.                     */
  PSTAGE next;                  /* Pointer to the next stage.           */
} STAGE;

typedef struct _STATION
{
  PSTAGE     pStage;             /* Pointer to filter stage list        */
  double     inEndtime;          /* end time of last sample in          */
  char       inSta[TRACE2_STA_LEN];    /* input Station code (name).    */
  char       inChan[TRACE2_CHAN_LEN];  /* input Channel code.           */
  char       inNet[TRACE2_NET_LEN];    /* input Network code.           */
  char       inLoc[TRACE2_LOC_LEN];    /* input Location code.          */
  char       outSta[TRACE2_STA_LEN];   /* output Station code (name).   */
  char       outChan[TRACE2_CHAN_LEN]; /* output Channel code.          */
  char       outNet[TRACE2_NET_LEN];   /* output Network code.          */
  char       outLoc[TRACE2_LOC_LEN];   /* output Location code.         */
} STATION;

  /*    Decimate World structure                                        */
typedef struct _WORLD
{
  DCMEWH        dcmEWH;         /* Structure for Earthworm parameters.  */
  DCMPARAM      dcmParam;       /* Network parameters.                  */
  SHM_INFO      regionOut;      /* Output shared memory region info     */
  MSG_LOGO      hrtLogo;        /* Logo of outgoing heartbeat message.  */
  MSG_LOGO      errLogo;        /* Logo of outgoing error message.      */
  MSG_LOGO      trcLogo;        /* Logo of outgoing tracebuf message.   */
  QUEUE 	MsgQueue;	/* The message queue                    */
  pid_t         MyPid;          /* My pid for restart from startstop    */
  int           DecimatorStatus;/* 0=> Decimator thread ok. <0 => dead  */
  STATION*      stations;       /* Array of stations in the network     */
  int           nSta;           /* How many stations do we have         */  
  FILTER        filter[MAXSTAGE]; /* Array of filter/decimation stages  */
  int           nStage;         /* How many stages do we have.          */
} WORLD;

/* Decimate function prototypes                                         */
void    InitializeParameters(WORLD *);
int     Configure (WORLD *, char **, const char *);
int     ReadConfig(char *, WORLD *, char *);
int     ReadEWH(WORLD *);
int     SetDecStages( WORLD *, char *);
int     remeznp(int, double *, double *, double *, double *, int *, double **);
int     Zeroes( double *, double **, double **, int);
void    hqr( double *, int, int, double *, double *);
int     SetStaFilters( WORLD *);
void  	StatusReport (WORLD *, unsigned char, short, char *);
int 	matchSCNL  (TracePacket *, unsigned char, WORLD *);
thr_ret DecimateThread (void *);
int   	FiltDecim (int **, int *, int, int *, int, float *, int, int);
int     DoOneStage (PSTAGE);
int     FilterDecimate (WORLD *, TracePacket *, int, unsigned char, TracePacket *);
void    ResetStation(STATION *);


#endif  /*  __DECIMATE_H__                                              */

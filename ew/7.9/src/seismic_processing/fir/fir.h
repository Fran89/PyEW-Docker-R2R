
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: fir.h 4013 2010-08-18 20:59:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/07/28 22:43:05  lombard
 *     Modified to handle SCNLs and TYPE_TRACEBUF2 (only!) messages.
 *
 *     Revision 1.2  2000/07/24 20:39:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * fir.h: Definitions for the Fir Earthworm Module.
 */

/*******                                                        *********/
/*      Redefinition Exclusion                                          */
/*******                                                        *********/
#ifndef __FIR_H__
#define __FIR_H__

#include <transport.h>  
#include <trace_buf.h>
#include <mem_circ_queue.h>

/* Flag to adjust the time lag from the FIR filter to zero.             */
/* Used in FiltOneSCN and some other places.                            */
#define FIXTIMELAG

/*******                                                        *********/
/*      Constant Definitions                                            */
/*******                                                        *********/

/*    StatusReport Error Codes - Must coincide with fir.desc            */
#define  ERR_MISSMSG       0   /* message missed in transport ring      */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer    */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_FIR           3   /* call to the FirFilter routine failed  */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation   */

/*    FirFilter Error Codes, returned from FiltOneSCN                   */
#define FIR_NOROOM       -1     /* No room in output buffer             */

/*    Buffer Lengths                                                    */
#define MAXFILENAMELEN  80      /* Maximum length of a file name.       */
#define MAXLINELEN      240     /* Maximum length of a line read from   */
                                /*   or written to a file.              */
#define MAXMESSAGELEN   160     /* Maximum length of a status or error  */
                                /*   message.                           */
#define BUFFSIZE MAX_TRACEBUF_SIZ * 2 /* Size of a channel trace buffer */
#define THREAD_STACK    8192    /* How big is our thread stack          */

/* Filter design constants                                              */
#define MIN_TRANS_BAND  0.0001  /* Smallest allowed transition band     */
#define MAX_BANDS       10      /* Maximum number of allowed bands      */
#define MIN_DEV         0.0005  /* Smallest allowed ripple limit        */

/*******                                                        *********/
/*      Structure Definitions                                           */
/*******                                                        *********/

  /*    Fir Parameter Structure (read from *.d files)              */
typedef struct _FIRPARAM
{
  char  myModName[MAX_MOD_STR];      /* Name of this instance of the */
                                        /*   Fir module -          */
                                        /*   REQUIRED.                  */
  char  readInstName[MAX_INST_STR];   /* Name of installation that    */
                                        /*   is producing trace data    */
                                        /*   messages.                  */
  char  readModName[MAX_MOD_STR];    /* Name of module at the above  */
                                        /*   installation that is       */
                                        /*   producing the trace data   */
                                        /*   messages.                  */
  char  ringIn[MAX_RING_STR];         /* Name of ring from which      */
                                        /*   trace data will be read -  */
                                        /* REQUIRED.                    */
  char  ringOut[MAX_RING_STR];        /* Name of ring to which        */
                                        /*   triggers will be written - */
                                        /* REQUIRED.                    */
  int   heartbeatInt;                   /* Heartbeat Interval(seconds). */
  int   logSwitch;                      /* Write logs?                  */
                                        /*   ( 0 = No, 1 = Yes )        */
  int   debug;                          /* Write out debug messages?    */
                                        /*   ( 0 = No, 1 = Yes )        */
  int   testMode;                       /* Test mode flag               */
                                        /*   ( 0 = No, 1 = Yes )        */
  double maxGap;                        /* Gap between tracedata points */

	/* newly added in 2010-08-18 by paulf for NEIC */
  int   QueueSize;			/* the number of messages to queue, defaults to 100 */
  int   SleepMilliSeconds;		/* the number of milliseconds to sleep between polls, defaults to 500 */

} FIRPARAM;

  /*    Information Retrieved from Earthworm*.h                         */
typedef struct _FIREWH
{
  unsigned char myInstId;       /* Installation running this module.    */
  unsigned char myModId;        /* ID of this module.                   */
  unsigned char readInstId;     /* Retrieve trace messages from         */
                                /*   specified installation.            */
  unsigned char readModId;      /* Retrieve trace messages from         */
                                /*   specified module.                  */
  unsigned char typeError;      /* Error message type.                  */
  unsigned char typeHeartbeat;  /* Heartbeat message type.              */
  unsigned char typeWaveform;   /* Waveform message type.               */
  long  ringInKey;                      /* Key to input shared memory   */
                                        /*   region.                    */
  long  ringOutKey;                     /* Key to output shared memory  */
                                        /*   region.                    */
} FIREWH;

typedef struct _BUFF
{
  double buffer[BUFFSIZE];
  double starttime;              /* time of first datapoint in buffer   */
  double samplerate;
  int    read;                   /* Index to start reading from         */
  int    write;                  /* Index to start writing to           */
} BUFFER; 

typedef struct _FILTER
{
  int Length;                   /* Length of the symmetric filter       */
  double *coef;                 /* Array of coefficients for this filter*/
} FILTER;

typedef struct _BAND *PBAND;
typedef struct _BAND
{
  double f_low;                 /* Low frequency limit of band         */
  double f_high;                /* High frequency limit of band        */
  int level;                    /* Desired level of the band: 1 or 0   */
  double dev;                   /* Allowed ripple from level           */
  PBAND next;                   /* Pointer to next band in list        */
} BAND;

typedef struct _STATION
{
  BUFFER     inBuff;                  /* Filter input buffer            */
  BUFFER     outBuff;                 /* Filter output buffer           */
  double     inEndtime;               /* end time of last sample in     */
  char       inSta[TRACE2_STA_LEN];    /* input Station code (name).     */
  char       inChan[TRACE2_CHAN_LEN];  /* input Channel code.            */
  char       inNet[TRACE2_NET_LEN];    /* input Network code.            */
  char       inLoc[TRACE2_LOC_LEN];    /* input Location code.		 */
  char       outSta[TRACE2_STA_LEN];   /* output Station code (name).    */
  char       outChan[TRACE2_CHAN_LEN]; /* output Channel code.           */
  char       outNet[TRACE2_NET_LEN];   /* output Network code.           */
  char       outLoc[TRACE2_LOC_LEN];   /* output Location code.          */
} STATION;

  /*    Fir World structure                                        */
typedef struct _WORLD
{
  FIREWH        firEWH;         /* Structure for Earthworm parameters.  */
  FIRPARAM      firParam;       /* Network parameters.                  */
  SHM_INFO      regionOut;      /* Output shared memory region info     */
  MSG_LOGO      hrtLogo;        /* Logo of outgoing heartbeat message.  */
  MSG_LOGO      errLogo;        /* Logo of outgoing error message.      */
  MSG_LOGO      trcLogo;        /* Logo of outgoing tracebuf message.   */
  QUEUE 	MsgQueue;	/* The message queue                    */
  pid_t         MyPid;          /* My pid for restart from startstop    */
  int           FirStatus;      /* 0=> Fir thread ok. <0 => dead        */
  STATION*      stations;       /* Array of stations in the network     */
  int           nSta;           /* How many stations do we have         */  
  FILTER        filter;         /* Filter coefficient structure         */
  PBAND         pBand;          /* List of filter bands during config   */
} WORLD;

/* Fir function prototypes                                         */
void    InitializeParameters(WORLD *);
void    InitSta(WORLD *);
int  	Configure(WORLD *, char **);
int     ReadConfig(char *, WORLD *);
int     ReadEWH(WORLD *);
int     BandCom(WORLD *);
int     SetFilter(WORLD *);
int     remeznp(int, double *, double *, double *, double *, int *, double **,
                int);
int     Zeroes(double *, double **, double **, int);
void    hqr(double *, int, int, double *, double *);
int     SetStations(WORLD *);
void  	StatusReport(WORLD *, unsigned char, short, char *);
int 	matchSCNL(TRACE2_HEADER*, WORLD *);
thr_ret FirThread(void *);
int     FirFilter(WORLD *, char *, int, char *);
int     FiltOneSCNL(WORLD *, STATION *);
void    ResetStation(STATION *);


#endif  /*  __FIR_H__                                                  */

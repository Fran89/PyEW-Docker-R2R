/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm.h 6487 2016-04-18 18:51:32Z scott $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2009/08/25 00:10:40  paulf
 *     added extraDelay parameter go gmew
 *
 *     Revision 1.5  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.4  2001/07/18 19:41:36  lombard
 *     Changed XMLDir, TempDir and MappingFile in GMPARAMS struct from string
 *     arrays to string pointers. Changed gm_config.c and gm_util.c to support thes
 *     changes. This solved a problem where the GMPARAMS structure was getting
 *     corrupted when a pointer to it was passed into getGMFromTrace().
 *     It's not clear why this was necessary; purify didn't complain.
 *
 *     Revision 1.3  2001/06/11 01:27:27  lombard
 *     cleanup
 *
 *     Revision 1.2  2001/06/10 21:27:36  lombard
 *     Changed single transport ring to input and output rings.
 *     Added ability to handle multiple getEventsFrom commands.
 *     Fixed handling of waveservers in config file.
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm.h
 */

#ifndef LM_H
#define LM_H

#include <earthworm.h>
#include <mem_circ_queue.h>
#include <trace_buf.h>
#include <rw_strongmotionII.h>
#include <trace_buf.h>
#include <transfer.h>

/* Various macros */
/*                  Length of the component name including null terminator */
#define GM_COMP_LEN 4
/*                  Value to indicate no magnitude has been calculated */
#define MAX_TRACE_SEC 600
#define MAX_SAMPRATE 200

#define EVENTID_SIZE 50
#define GM_SM_LEN 600
#ifndef PATH_MAX
#define PATH_MAX 512
#endif

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
  long lenRaw;       /* length of the rawData array                          */
  long nProc;        /* number of samples in processed data buffer           */
  long lenProc;      /* length of one partition the procData array           */
  long numProc;      /* number of partitions of the procData array           */
  long padLen;       /* Number of padded samples due to convolution          */
  GAP *gapList;      /* linked list of gaps in raw data                      */
  int nGaps;         /* number of gaps found in raw data                     */
} DATABUF;

/* Macros for the three component directions */
#define GM_E 0
#define GM_N 1
#define GM_Z 2

typedef struct _SCNLPAR {
  double fTaper[4];
  double clipLimit;
  double taperTime;  
  char sta[TRACE_STA_LEN];
  char comp[TRACE_CHAN_LEN];
  char net[TRACE_NET_LEN];
  char loc[TRACE_LOC_LEN];
} SCNLPAR;

/* The direction part of the component structure */
typedef struct _COMP3
{
  double peakWinStart; /* starttime of peak-search window                  */
  double peakWinEnd;  /* end time of peak-search window                    */
  int BadBitmap;      /* bitmap to indicate why this component is bad      */
  SCNLPAR *pSCNLPar;    /* parameters for this SCN                           */
  char name[4];       /* The full 3-character component name               */
  char loc[3];       /* The full 2-character location code                 */
  double RSAPeakTime[SM_MAX_RSA];  /* Time of the peak RSA value, mising   *
                                    * from the SM_INFO structure           */
} COMP3;

/*
 * The non-directional part of the component structure;
 *  zero or more per station.
 */
typedef struct _COMP1 *PCOMP1;
typedef struct _COMP1
{
  COMP3 c3[3];        /* The three direction structures                   */
  PCOMP1 next;         /* The next component in linked list; NULL if none */
  char n2[GM_COMP_LEN];  /* First 2 chars of component name, the C in SCN */
} COMP1;

/* The station structure, one or more per event */
typedef struct _STA
{
  double lat;         /* Station latitude; south is negative */
  double lon;         /* Station longitude; west is negative */
  double dist;        /* Epicentral distance in kilometers   */
  double p_est;       /* Estimated P arrival                 */
  double s_est;       /* Estimated S arrival                 */
  COMP1 *comp;        /* List of components for this station */
  char sta[TRACE_STA_LEN];     /* Station name, the S in SCN */
  char net[TRACE_NET_LEN];     /* Network name, the N in SCN */
} STA;

/*
 * The event structure, one per event. Note this structure holds only the
 * information needed by the localmag module!
 */
typedef struct _EVT
{
  char eventId[EVENTID_SIZE];    /* The event ID string                      */
  char authorId[AUTHOR_FIELD_SIZE]; /* The author ID string                  */
  double origin_time; /* The origin time, seconds since midnight, 1 Jan 1970 */
  double lat;         /* Hypocenter latitude; south is negative              */
  double lon;         /* Hypocenter longitude; west is negative              */
  double depth;       /* Hypocenter depth in kilometers                      */
  STA *Sta;           /* Array of stations for this event                    */
  int numSta;         /* Number of filled STA structures                     */
} EVENT;
  

/* a link describing a singe wave_server */
typedef struct _SERVER *PSERVER;
typedef struct _SERVER {
  char IPAddr[16];
  char port[6];
  PSERVER next;
} SERVER;

/* Description of all wave_servers */
typedef struct _WS_ACCESS {
  char *serverFile;      /* a file listing wave_servers     */
  SERVER *pList;         /* the linked list of wave_servers */
} WS_ACCESS;

/* A link for SCNL selection */
typedef struct _SCNLSEL *PSCNLSEL;
typedef struct _SCNLSEL {
  char sta[TRACE_STA_LEN];
  char comp[TRACE_CHAN_LEN];
  char net[TRACE_NET_LEN];
  char loc[TRACE_LOC_LEN];
  PSCNLSEL next;
} SCNLSEL;

/* Earthworm transport stuff for ground-motion */
typedef struct _GMEW {
  pid_t myPid;
  long RingInKey;              /* key of transport ring for input    */
  long RingOutKey;             /* key of transport ring for output   */
  SHM_INFO InRegion;           /* shared memory region to use for i/o */
  SHM_INFO OutRegion;          /* shared memory region to use for i/o */
  QUEUE msgQ;
  mutex_t Qmutex;
  int nGetLogo;
  MSG_LOGO *GetLogo;        /* array of logos to get */
  MSG_LOGO hrtLogo;         /* heartbeat message logo */
  MSG_LOGO errLogo;         /* error message logo */
  MSG_LOGO gmLogo;          /* gmew message logo */
  int terminate;
  MSG_LOGO amLogo;          /* activate module message logo */
  MSG_LOGO ha2kLogo;        /* HYPOARC2000 message logo */
  MSG_LOGO threshLogo;      /* Thresh ALARM message logo */
} GMEW;

enum Version {vAll = -1, vPrelim, vRapid, vFinal};

typedef struct _GMPARAMS {
  double maxDist;         /* Maximum epicentral distance for a station       */
  double peakSearchStart; /* Fraction of P - S to search for peak before S   */
  double peakSearchStartMin; /* Minimum number of seconds to search before S */
  double peakSearchEnd;   /* Fraction of P - S to search for peak after S    */
  double peakSearchEndMin; /* Minimum numberr of seconds to search after S   */
  double traceStart;      /* Seconds before P_est to start trace             */
  double traceEnd;        /* Seconds after S_est to end trace                */
  double snrThresh;       /* Signal-to-noize ratio threshold to pick amps    */
  SCNLSEL *pAdd;           /* SCN selections lists                            */
  SCNLSEL *pDel;           /* SCN deletion lists                              */
  SCNLPAR *pSCNLPar;        /* Array of SCN parameter structures               */
  WS_ACCESS *pWSV;        /* wave_server access information                  */
  GMEW *pEW;              /* Earthworm transport structure                   */
  long maxTrace;          /* Maximum number of trace data points             */
  int debug;              /* debug level                                     */
  int HeartBeatInterval;  /* Earthworm heartbeat interval in seconds         */
  int maxSCNLPar;          /* How many SCN parameter entries allocated        */
  int maxSta;             /* How many stations to use                        */
  int numSCNLPar;          /* Number of SCNL parameter entries used            */
  int respSource;         /* Where to get response information               */
  int saveTrace;          /* whether and how to save synthetic traces        */
  int staLoc;             /* Where to find station locations                 */
  int traceSource;        /* source for trace data                           */
  int wsTimeout;          /* wave_server timeout in milliseconds             */
  int waitTime;          /* time to wait before hitting wave_server after arc message received */
  enum Version LookAtVersion; /* Which version to look at, default is vAll */
  char *eventID;          /* event ID, for accessing event from EW databese  */
  char *respDir;          /* directory for response files, if needed         */
  char *respNameFormat;   /* format of response filename                     */
  char *sacOutDir;        /* Where to write SAC output files                 */
  char *saveNameFormat;   /* format ground-motion saved file names           */
  char *saveDirFormat;    /* format string for SAC output directory          */
  char *staLocFile;       /* station location filename, if needed            */
  char *TempDir;
  char *XMLDir;
  char *MappingFile;
  int alarmDuration;	 /* when activated by alarm, use this duration for snapshot */
  int allowDuplicates;	 /* set to 1 to allow duplicate SCNL's, otherwise just first location code gets reported */
  int sendActivate;	 /* set to 1 to send ACTIVATE messages when XML completes */
  int threshDuration;	 /* when activated by threshold, use this duration for snapshot */
  long preTriggerSeconds;  /* Used to compute start time, P and S           */
} GMPARAMS;

#define GM_UNDEF -1

/* Trace Source */
/*                Wave_serverV         */
#define GM_TS_WS   11
#ifdef EWDB
/*                Earthworm Database   */
#define GM_TS_EWDB 12
#endif


/* Station Location Source */
/*                Hyp2000 station file */
#define GM_SL_HYP  21
#ifdef EWDB
/*                Earthworm Database   */
#define GM_SL_EWDB 22
#endif

/* Instrument Response Source */
#ifdef EWDB
/*                Earthworm Database   */
#define GM_RS_EWDB 31
#endif
/*                SAC file on other directory */
#define GM_RS_FILE 33

/* Save ground-motion Traces */
/*                Don't Save                  */
#define GM_ST_NO   41
/*                Save in SAC files           */
#define GM_ST_SAC  42

/* Debug options */
/*                trace/search and estimated arrival times */
#define GM_DBG_TIME  1<<0
/*                SCN selection */
#define GM_DBG_SEL   1<<1
/*                ws_client debug */
#define GM_DBG_WSC   1<<4
/* Keep the following three aligned with debug settings in transfer.h */
/*                response poles, zeros, gain */
#define GM_DBG_PZG   1<<5
/*                trial response function */
#define GM_DBG_TRS   1<<6
/*                actual response function */
#define GM_DBG_ARS   1<<7
/*                input and output traces */
#define GM_DBG_TRC   1<<8

/* Bad Bits: used to indicate why a component trace should not be used */
/*                        Trace is clipped and clipping is not allowed */
#define GM_BAD_CLIP  1<<0
/*                       Trace has a gap within the peak-search window */
#define GM_BAD_GAP   1<<1
/*             Trace has insufficient event signal for pre-event noise */
#define GM_LOW_SNR   1<<2

/* Function prototypes */
int send_gm( SM_INFO *, GMEW * );
int Configure( GMPARAMS *, int, char **, EVENT *, char *);


#endif

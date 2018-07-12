/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_config.h 6487 2016-04-18 18:51:32Z scott $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2009/08/25 00:10:40  paulf
 *     added extraDelay parameter go gmew
 *
 *     Revision 1.2  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/* gm_config.h: header file for gm_config.c */

#ifndef GM_CONFIG_H
#define GM_CONFIG_H

#include <earthworm.h>
#include <mem_circ_queue.h>
#include "gm.h"

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
  char sta[7];
  char comp[9];
  char net[9];
  char loc[3];
  PSCNLSEL next;
} SCNLSEL;

typedef struct _SCNLPAR {
  double fTaper[4];
  double clipLimit;
  double taperTime;  
  char sta[TRACE_STA_LEN];
  char comp[TRACE_CHAN_LEN];
  char net[TRACE_NET_LEN];
  char loc[TRACE_LOC_LEN];
} SCNLPAR;

/* Earthworm transport stuff for ground-motion */
typedef struct _GMEW {
  pid_t myPid;
  long RingKey;                /* key of transport ring for i/o      */
  unsigned char GetInstId;     /* id of inst to get from             */
  unsigned char MyInstId;      /* local installation id              */
  unsigned char MyModId;       /* Module Id for this program         */
  unsigned char GetMod;
  unsigned char TypeHeartBeat; 
  unsigned char TypeError;
  unsigned char TypeHyp2kArk;
  unsigned char TypeSM;
  unsigned char TypeThreshAlert;
  SHM_INFO Region;             /* shared memory region to use for i/o */
  QUEUE msgQ;
  mutex_t Qmutex;
  MSG_LOGO hrtLogo;         /* heartbeat message logo */
  MSG_LOGO errLogo;         /* error message logo */
  MSG_LOGO gmLogo;          /* gmew message logo */
  int terminate;
} GMEW;


typedef struct _GMPARAMS {
  double maxDist;         /* Maximum epicentral distance for a station       */
  double peakSearchStart; /* Fraction of P - S to search for peak before S   */
  double peakSearchStartMin; /* Minimum number of seconds to search before S */
  double peakSearchEnd;   /* Fraction of P - S to search for peak after S    */
  double peakSearchEndMin; /* Minimum numberr of seconds to search after S   */
  double traceStart;      /* Seconds before P_est to start trace             */
  double traceEnd;        /* Seconds after S_est to end trace                */
  double snrThresh;       /* Signal-to-noize ratio threshold to pick amps    */
  SCNLSEL *pAdd;          /* SCN selections lists                            */
  SCNLSEL *pDel;          /* SCN deletion lists                              */
  SCNLPAR *pSCNLPar;      /* Array of SCNL parameter structures              */
  WS_ACCESS *pWSV;        /* wave_server access information                  */
  GMEW *pEW;              /* Earthworm transport structure                   */
  long maxTrace;          /* Maximum number of trace data points             */
  int debug;              /* debug level                                     */
  int HeartBeatInterval;  /* Earthworm heartbeat interval in seconds         */
  int maxSCNLPar;         /* How many SCNL parameter entries allocated       */
  int maxSta;             /* How many stations to use                        */
  int numSCNLPar;         /* Number of SCNL parameter entries used           */
  int respSource;         /* Where to get response information               */
  int saveTrace;          /* whether and how to save synthetic traces        */
  int staLoc;             /* Where to find station locations                 */
  int traceSource;        /* source for trace data                           */
  int wsTimeout;          /* wave_server timeout in milliseconds             */
  int waitTime;	/* number of seconds to delay processing an event when in RT mode */
  char *eventID;          /* event ID, for accessing event from EW databese  */
  char *respDir;          /* directory for response files, if needed         */
  char *respNameFormat;   /* format of response filename                     */
  char *sacOutDir;        /* Where to write SAC output files                 */
  char *saveNameFormat;   /* format ground-motion saved file names           */
  char *saveDirFormat;    /* format string for SAC output directory          */
  char *staLocFile;       /* station location filename, if needed            */
  char MyModName[MAX_MOD_STR];  /* Earthworm module name for local_mag       */
  char GetModName[MAX_MOD_STR]; /* Module to get messages from               */
  char GetInstName[MAX_INST_STR]; /* Inst to get messages from               */
  char RingName[MAX_RING_STR];  /* Earthworm transport ring name             */
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
/*                SCNL selection */
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

/* Function prototypes for gm_config.h */
int Configure( GMPARAMS *, int, char **, EVENT *);

#endif

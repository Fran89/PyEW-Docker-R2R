/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_config.h 6333 2015-05-06 04:53:22Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.14  2010/03/15 16:22:50  paulf
 *     merged from Matteos branch for adding version to directory naming
 *
 *     Revision 1.13.2.1  2010/03/15 15:15:55  quintiliani
 *     Changed output file and directory name
 *     Detailed description in ticket 22 from
 *     http://bigboy.isti.com/trac/earthworm/ticket/22
 *
 *     Revision 1.13  2007/03/30 14:14:05  paulf
 *     added saveXMLdir option
 *
 *     Revision 1.12  2007/03/29 20:09:50  paulf
 *     added eventXML option from INGV. This option allows writing the Shakemap style event information out as XML in the SAC out dir
 *
 *     Revision 1.11  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.10  2005/08/08 18:38:14  friberg
 *
 *     Fixed bug from last version that had station corrections added in twice.
 *     Added in new directive require2Horizontals to require 2 components for a station Ml
 *     	example:   require2Horizontals 1
 *     	Note needs the 1 after the directive to be used
 *     Added in new directive useMedian
 *     	example:   useMedian
 *     	Note for this one, no flag is needed after the directive
 *     Also updated the Doc files.
 *
 *     Revision 1.9  2005/07/27 16:35:21  friberg
 *     minStationsMl changes
 *
 *
 *     Revision 1.8  2002/03/17 18:16:38  lombard
 *     Added LogFile command, added second logit_init call to conform to
 *       latest standard.
 *     Added SgSpeed, searchTimes, searchStartPhase commands in place
 *       of searchWindow command to support new search times calculation.
 *     Added extraDelay command to control localmag scheduling for realtime
 *       events.
 *
 *     Revision 1.7  2001/06/10 21:21:46  lombard
 *     Changed single transport ring to two rings, added allowance
 *     for multiple getEventsFrom commands.
 *     These changes necessitated several changes in the way config
 *     and earthworm*.d files were handled.
 *
 *     Revision 1.6  2001/05/31 17:41:13  lucky
 *     Added support for outputFormat = File. This option works only in
 *     standalone mode. It writes TYPE_MAGNITUDE message to a specified file.
 *     We need this for review.
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

/* lm_config.h: header for configuration of localmag */

#ifndef LM_CONFIG_H
#define LM_CONFIG_H

#include <earthworm.h>
#include <mem_circ_queue.h>
#include <trace_buf.h>
#include <transport.h>

/* a link describing a single wave_server */
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

/* Description of the Wood-Anderson instrument */
typedef struct _WA_PARAMS {
  double period;    /* period, in seconds */
  double damp;      /* damping coefficient, relative to critical damping */
  double gain;      /* overall gain, no units */
} WA_PARAMS;

/* A link for SCNL selection */
typedef struct _SCNLSEL *PSCNLSEL;
typedef struct _SCNLSEL {
  char sta[TRACE_STA_LEN];
  char comp[TRACE_CHAN_LEN];
  char net[TRACE_NET_LEN];
  char loc[TRACE_LOC_LEN];
  PSCNLSEL next;
} SCNLSEL;

typedef struct _DBACCESS {
  char *user;
  char *pwd;
  char *service;
} DBACCESS;


#define MINIMUM_STATIONS_TO_REPORT 1

/* Earthworm transport stuff for localmag */
typedef struct _LMEW {
  long RingInKey;              /* key of transport ring for input    */
  long RingOutKey;             /* key of transport ring for output   */
  SHM_INFO InRegion;           /* shared memory region to use for i/o */
  SHM_INFO OutRegion;          /* shared memory region to use for i/o */
  int nGetLogo;
  MSG_LOGO *GetLogo;        /* array of logos to get */
  MSG_LOGO hrtLogo;         /* heartbeat message logo */
  MSG_LOGO errLogo;         /* error message logo */
  MSG_LOGO magLogo;         /* magnitude message logo */
  MSG_LOGO noMagLogo;       /* no-magnitude message logo */
} LMEW;
  

enum Version {vAll = -1, vPrelim, vRapid, vFinal};

typedef struct _LMPARAMS {
  double maxDist;         /* Maximum epicentral distance for a station       */
  double waitTime;       /* Seconds from origin to wait for wave propagation */
  int    waitNow;	/* 1 = wait from now, 0 = wait from origin time      */
  double SgSpeed;         /* Speed (km/sec of Sg phase                       */
  double peakSearchStart; /* Fraction of P - S to search for peak before S   */
  double peakSearchStartMin; /* Minimum number of seconds to search before S */
  double peakSearchEnd;   /* Fraction of P - S to search for peak after S    */
  double peakSearchEndMin; /* Minimum numberr of seconds to search after S   */
  double slideLength;     /* Length of sliding window in seconds             */
  double traceStart;      /* Seconds before P_est to start trace             */
  double traceEnd;        /* Seconds after S_est to end trace                */
  double z2pThresh;       /* Signal-to-noize ratio threshold to pick amps    */
  DBACCESS *pDBaccess;    /* access to the database                          */
  SCNLSEL *pAdd;           /* SCN selections lists                            */
  SCNLSEL *pDel;           /* SCN deletion lists                              */
  SCNLPAR *pSCNLPar;        /* Array of SCNL parameter structures               */
  WA_PARAMS *pWA;         /* optional Wood-Anderson coefficients             */
  WS_ACCESS *pWSV;        /* wave_server access information                  */
  LMEW *pEW;
  long maxTrace;          /* Maximum number of trace data points             */
  int HeartBeatInterval;  /* Earthworm heartbeat interval in seconds         */
  int debug;              /* debug level                                     */
  int fEWTP;              /* Flag to use earthworm transport (1) or not (0)  */
  int fDist;              /* Flag for distance in logA0 relation             */
  int fGetAmpFromSource;  /* TRUE if amplitudes are to be read from source   */
  int fMeanCompMags;      /* TRUE if taking mean of component magnitudes     */
  int fWAsource;          /* TRUE if source contains Wood-Anderson traces    */
  int eventSource;        /* source of event location and time               */
  int minStationsMl;      /* minimum number of stations for Ml Avg reporting */
  int require2Horizontals;      /* Flag (1) to require 2 horiz channels for Ml from a sta or (0)-default to not */
  int allowVerticals;      /* Flag (1) to Z channels for Ml from a sta or (0)-default to not */
  int useMedian;          /* Flag (1) to use median instead of mean (default)*/
  int maxSCNLPar;          /* How many SCN parameter entries allocated        */
  int maxSta;             /* How many stations to use                        */
  int nLtab;              /* Number of entries in la0tab                     */
  int numSCNLPar;          /* Number of SCN parameter entries used            */
  int outputFormat;       /* type of output for localmag                     */
  int respSource;         /* source of instrument response data              */
  int saveTrace;          /* whether and how to save Wood-Anderson traces    */
  int searchStartPhase;   /* phase (P or S) to use for search start calcs    */
  int staLoc;             /* station location source                         */
  int traceSource;        /* source for trace data                           */
  int wsTimeout;          /* wave_server timeout in milliseconds             */
  int eventXML;           /* Flag (1=output event xml) default is (0) not to     */
  int saveSCNL;           /* Flag (1=output L of SCNL in message) (0) to just do SCN (old style)     */
  char *saveXMLdir;	  /* if set, use this dir instead of sacOutDir  for option above...automagically turns on eventXML flag */
  enum Version LookAtVersion;	 /* Which version to look at, default is vAll */
  char *eventID;          /* event ID, for accessing event from EW databese  */
  char *loga0_file;       /* Nmae of the logA0 table file                    */
  char *outputNameFormat; /* format of output filenames                      */
  char *respDir;          /* directory for response files, if needed         */
  char *respNameFormat;   /* format of response filename                     */
  char *sacInDir;         /* directory for reading and writing SAC files     */
  char *sacOutDir;        /* Where to write SAC output files                 */
  char *saveDirFormat;    /* format string for SAC output directory          */
  char *saveNameFormat;   /* format of WA traces filenames                   */
  char *sourceNameFormat; /* format string of SAC filenames                  */
  char *staLocFile;       /* station location filename, if needed            */
  char *outputFile;       /* output filename, if needed                      */
  char *MlmsgOutDir;      /* directory for writing Mlmsg                     */
#ifdef UW
  char *UWpickfile;       /* name of UW-format pickfile                      */
#endif
  char ChannelNumberMap[5];	/* map channel numbers to LM_ orientation codes */
  int  SkipStationsNotInArc; /* flag for excluding stations that are not in ARC message */
  double MinWeightPercent;   /* MinWeightPercent based on the maximum weight of
				ther ARC message.  Used only with SkipStationsNotInArc */
  double MLQpar1;            /* First parameter for computing the quality of magnitude */
} LMPARAMS;


/***** Macros for configuration flags ****/

#define LM_UNDEF -1


/* Event Source */
/*                Hyp2000 archive file */
#define LM_ES_ARCH 1
#ifdef EWDB
/*                Earthworm Database   */
#define LM_ES_EWDB 2
#endif
/*                SAC file header      */
#define LM_ES_SAC  3
#ifdef UW
/*                UW-format pickfile   */
#define LM_ES_UW   4
#endif


/* Trace Source */
/*                Wave_serverV         */
#define LM_TS_WS   11
#ifdef EWDB
/*                Earthworm Database   */
#define LM_TS_EWDB 12
#endif
/*                SAC file             */
#define LM_TS_SAC  13
#ifdef UW
/*                UW-format datafile   */
#define LM_TS_UW   14
#endif


/* Station Location Source */
/*                Hyp2000 station file */
#define LM_SL_HYP  21
#ifdef EWDB
/*                Earthworm Database   */
#define LM_SL_EWDB 22
#endif
/*                SAC file header      */
#define LM_SL_SAC  23
#ifdef UW
/*                UW wash.sta file */
#define LM_SL_UW   24
#endif

/* Instrument Response Source */
#ifdef EWDB
/*                Earthworm Database   */
#define LM_RS_EWDB 31
#endif
/*                SAC file in SAC directory */
#define LM_RS_SAC  32
/*                SAC file on other directory */
#define LM_RS_FILE 33
#ifdef UW
/*                UW calib2.sta file */
#define LM_RS_UW   34
#endif

/* Save Wood-Anderson Traces */
/*                Don't Save                  */
#define LM_ST_NO   41
/*                Save in SAC files           */
#define LM_ST_SAC  42
#ifdef UW
/*                Save in UW-format datafile  */
#define LM_ST_UW   43
#endif


/* Localmag Output Format */
/*                None: send the report to stdout */
#define LM_OM_SO   51
/*                Earthworm TYPE_LOCALMAG Message written to ring */
#define LM_OM_LM   52

/*                TYPE_LOCALMAG Message written to file */
#define LM_OM_FL   55

#ifdef EWDB
/*                Earthworm Database */
#define LM_OM_EWDB 53
#endif
#ifdef UW
/*                UW-format pickfile */
#define LM_OM_UW   54
#endif


/* Distance type used on LogA0 relation */
/*                Epicenter - station distance */
#define LM_LD_EPI  61
/*                Hypocenter - station distance */
#define LM_LD_HYPO 62


/* Search Start Phase */
/*                first-arrival P phase from layered model */
#define LM_SSP_P 71
/*                first-arrival S phase from layered model */
#define LM_SSP_S 72


/* Debug options */
/*                trace/search and estimated arrival times */
#define LM_DBG_TIME  1<<0
/*                SCNL selection */
#define LM_DBG_SEL   1<<1
/*                log A0 values */
#define LM_DBG_LA0   1<<2
/*                SAC file selection */
#define LM_DBG_SAC   1<<3
/*                ws_client debug */
#define LM_DBG_WSC   1<<4
/* Keep the following three aligned with debug settings in transfer.h */
/*                response poles, zeros, gain */
#define LM_DBG_PZG   1<<5
/*                trial response function */
#define LM_DBG_TRS   1<<6
/*                actual response function */
#define LM_DBG_ARS   1<<7
/*                input and output traces */
#define LM_DBG_TRC   1<<8

/* Function prototypes for lm_config.c: */
int Configure( LMPARAMS *, int, char **, EVENT *);

#endif   /* End of LM_CONFIG_H */

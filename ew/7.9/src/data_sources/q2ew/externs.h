#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#include <time.h>
#include <sys/shm.h>	
#include "transport.h" 	/* needed for MSG_LOGO typedef */
/****************************************************************/
/* 
	Put all external variables here.
        NOTE: All extern vars start with a capital letter
*/

EXTERN int Verbose;		/* spew messages to stderr */
EXTERN char * Progname;		/* my name */
EXTERN int ShutMeDown;		/* shut down flag */
EXTERN char * Config;		/* config file name */


/****************************************************************/
/* EarthWorm global configs : all of these are REQUIRED from the config/desc
        file: All are set in the GetConfig() routine found in getconfig.c
*/
EXTERN unsigned char QModuleId;         /* module id for q2ew */
EXTERN long RingKey;                    /* key to ring buffer q2ew dumps data */
EXTERN int HeartbeatInt;                /* my heartbeat interval (secs) */
EXTERN int LogFile;                     /* generate a logfile? */
EXTERN int LOG2LogFile;                 /* put LOG channels in logfile? */
EXTERN int TimeoutNoSend;               /* no data from DAQ timeout */
EXTERN int ComservSeqBuf;               /* comserv Sequence Buffer control */

/* some globals for EW not settable in the .d file. */
EXTERN SHM_INFO  Region;  
EXTERN MSG_LOGO DataLogo;               /* EW logo tag  for data */
EXTERN MSG_LOGO OtherLogo;              /* EW logo tag  for err,log,heart */
EXTERN unsigned char TypeTrace;         /* Trace EW type for logo */
EXTERN unsigned char TypeTrace2;         /* Trace EW type for logo (supporting SCNL) */
EXTERN unsigned char TypeHB;            /* HB=HeartBeat EW type for logo */
EXTERN unsigned char TypeErr;           /* Error EW type for logo */

EXTERN time_t	TSLastBeat;		/* time stamp since last heartbeat */
EXTERN unsigned TidHB;			/* ID for the hearbeat thread */
EXTERN int UseTraceBuf2;                /* IGD 2006/11/20 1 if use TBuf2, 0 if TBuf old style */
/****************************************************************/
/* COMSERV globals */
EXTERN time_t	TSLastCSData;		/* time stamp since last ComservData 
					recv'ed FROM ANY STATION! */

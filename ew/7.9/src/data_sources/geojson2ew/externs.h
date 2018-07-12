#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#include <time.h>
#include <sys/shm.h>	
#include "json_conn.h"
#include "transport.h" 	/* needed for MSG_LOGO typedef */
/****************************************************************/
/* 
	Put all external variables here.
        NOTE: All extern vars start with a capital letter
*/

EXTERN pid_t MyPid;   /* needed for restart message for heartbeat */
EXTERN unsigned int Verbose;		/* spew messages to stderr */
#define VERBOSE_GENERAL	0x0001
#define VERBOSE_SNCL	0x0002
#define VERBOSE_EWSENT	0x0004
#define VERBOSE_DUP	0x0008
EXTERN char * VerboseSncl;
EXTERN char * Progname;			/* my name */
EXTERN volatile int ShutMeDown;		/* shut down flag */
EXTERN char * Config;			/* config file name */


/****************************************************************/
/* EarthWorm global configs : all of these are REQUIRED from the config/desc
        file: All are set in the GetConfig() routine found in getconfig.c
*/
EXTERN unsigned char QModuleId;         /* module id for geojson2ew */
EXTERN long RingKey;                    /* key to ring buffer geojson2ew dumps data */
EXTERN int HeartbeatInt;                /* my heartbeat interval (secs) */
EXTERN int LogFile;                     /* generate a logfile? */
EXTERN char * Map_sncl;			/* MAP_SNCL */
EXTERN char * Map_time;			/* MAP_TIME */
EXTERN char * Map_samplerate;		/* MAP_SAMPLERATE */
EXTERN JSON_CONN_PARAMS Conn_params;	/* JSON connection parameters */

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

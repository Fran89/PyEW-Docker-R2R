#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#define   DEBUG

#include <time.h>
#include <sys/shm.h>	
#include "transport.h" 	/* needed for MSG_LOGO typedef */
#define   FILE_NAM_LEN 500
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
EXTERN unsigned char QModuleId;         /* module id for nq2ring */
EXTERN long RingKey;                    /* key to ring buffer nq2ring dumps data */
EXTERN long InRingKey;                    /* key to ring buffer nq2ring dumps data */
EXTERN long OutRingKey;                    /* key to ring buffer nq2ring dumps data */
EXTERN int HeartBeatInt;                /* my heartbeat interval (secs) */
EXTERN int LogFile;                     /* generate a logfile? */
EXTERN int LOG2LogFile;                 /* put LOG channels in logfile? */

/* some globals for EW not settable in the .d file. */
EXTERN SHM_INFO  Region;  
EXTERN SHM_INFO  InRegion;  
EXTERN SHM_INFO  OutRegion;  
EXTERN MSG_LOGO DataLogo;               /* EW logo tag  for data */
EXTERN MSG_LOGO OtherLogo;              /* EW logo tag  for err,log,heart */
EXTERN unsigned char TypeTrace;         /* Trace EW type for logo */
EXTERN unsigned char TypeTrace2;         /* Trace EW type for logo (supporting SCNL) */
EXTERN unsigned char TypeHB;            /* HB=HeartBeat EW type for logo */
EXTERN unsigned char TypeErr;           /* Error EW type for logo */

EXTERN time_t	TSLastBeat;		/* time stamp since last heartbeat */
EXTERN time_t	timeNow;		/* time stamp since last heartbeat */
EXTERN unsigned TidHB;			/* ID for the hearbeat thread */
/****************************************************************/

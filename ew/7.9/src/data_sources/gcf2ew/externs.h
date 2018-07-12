#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#include <time.h>
#include <sys/shm.h>	
#include <termios.h>	
#include "transport.h" 	/* needed for MSG_LOGO typedef */
/****************************************************************/
/* 
	Put all external variables here.
        NOTE: All extern vars start with a capital letter
*/

EXTERN int gcf_socket_fd;       /* the file descriptor for the socket */
EXTERN int Verbose;		/* spew messages to stderr */
EXTERN char * Progname;		/* my name */
EXTERN int ShutMeDown;		/* shut down flag */
EXTERN char * Config;		/* config file name */


/****************************************************************/
/* EarthWorm global configs : all of these are REQUIRED from the config/desc
        file: All are set in the GetConfig() routine found in getconfig.c
*/
EXTERN unsigned char GModuleId;         /* module id for gcf2ew */
EXTERN long RingKey;                    /* key to ring buffer gcf2ew dumps data */
EXTERN int HeartbeatInt;                /* my heartbeat interval (secs) */
EXTERN int LogFile;                     /* generate a logfile? */
EXTERN int SaveLOGS;                 	/* put LOG channels in logfile? */
EXTERN int TimeoutNoSend;               /* no data from DAQ timeout */
EXTERN char * GCFHost;			/* the host name or IP a gcfserver*/
EXTERN char * Host;			/* the host name or IP of the MSS100 */
EXTERN int Port;			/* the port number of the MSS100 */
EXTERN char * SerialPort;		/* the serial terminal port */
EXTERN speed_t BaudRate;		/* the baud rate of the SerialPort */

/* some globals for EW not settable in the .d file. */
EXTERN SHM_INFO  Region;  
EXTERN MSG_LOGO DataLogo;               /* EW logo tag  for data */
EXTERN MSG_LOGO OtherLogo;              /* EW logo tag  for err,log,heart */
EXTERN MSG_LOGO SOHLogo;                /* EW logo tag  for TYPE_GCFSOH_PACKET */
EXTERN unsigned char TypeTrace;         /* Trace EW type for logo */
EXTERN unsigned char TypeTrace2;        /* Trace2  EW type for logo */
EXTERN unsigned char TypeHB;            /* HB=HeartBeat EW type for logo */
EXTERN unsigned char TypeErr;           /* Error EW type for logo */
EXTERN unsigned char TypeGCFSOH;        /* EW TYPE_GCFSOH_PACKET type for logo */
EXTERN int UseTraceBuf2;		/* 1 if use TBuf2, 0 if TBuf old style */
EXTERN int InjectSOH;			/* 1 if TYPE_GCFSOH_PACKETS should be sent , 0 not sent */

EXTERN time_t	TSLastBeat;		/* time stamp since last heartbeat */
EXTERN unsigned TidHB;			/* ID for the hearbeat thread */
/****************************************************************/
/* GCF globals */
EXTERN time_t	TSLastGCFData;		/* time stamp since last GCF packet */

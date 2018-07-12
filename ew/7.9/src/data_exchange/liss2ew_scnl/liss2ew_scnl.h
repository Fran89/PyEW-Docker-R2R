/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: liss2ew_scnl.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:40:12  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:51:30  mark
 *     Initial checkin
 *
 *     Revision 1.3  2003/06/16 22:08:11  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.2  2000/03/13 23:50:24  lombard
 *     Added decompression struct to WORLD structure.
 *
 *     Revision 1.1  2000/03/05 21:45:57  lombard
 *     Initial revision
 *
 *
 *
 */

/* LISS2ew definitions */

#ifndef __LISS2EW__
#define __LISS2EW__

#include <math.h>

extern "C"
{
#include <transport.h>
#include <trace_buf.h>
#include <dcc_std.h>
#include <dcc_misc.h>
#include <dcc_time.h>
#include <seed_data.h>
#include <seed_data_proto.h>
#include <steim/steim.h>
#include <steim/steimlib.h>
#include <steim/miniseed.h>
#include <TCPSocket.h>
}

#define DEF_LEN_SEED    512     /* Default packet size from LISS        */
#define DEF_LISS_PORT  4000     /* Default TCP port for LISS            */

#define MAXMESSAGELEN   160     /* Maximum length of a status or error  */
                                /*   message.                           */
#define MAXRINGNAMELEN  28      /* Maximum length of a ring name.       */
                                /* Should be defined by kom.h           */
#define MAXMODNAMELEN   30      /* Maximum length of a module name      */
                                /* Should be defined by kom.h           */
#define MAXADDRLEN      80      /* Length of LISS hostname or address   */
#define MAXFILENAMELEN  100     /* Length of a file name                */
#define LC_LEN          4       /* Bytes for the location code          */
#define LC_WILD        '*'      /* Wildcard character for location code */
#define RETRIES_LOG     4       /* Report after this many connect tries */
#define RETRIES_EXIT    20      /* Give up after this many retries      */
#define RETRIES_RECONN  100     /* After this many timed out reads,     */
                                /*   close and reopen the socket        */
#define MAX_SCN_PER_CONN	16	/* Maximum SCNLs per IP connection      */

#ifdef _INTEL
#define FLIP 1
#else
#define FLIP 0
#endif

/* LISS2EW status indicator */
typedef enum _STATUS
{
  closed,           /* Socket is closed */
  connected,        /* Connected to LISS */
  conn_fail1,       /* Connection failed, no report yet */
  conn_fail2,       /* First failure report */
  receiving         /* Data is being received from LISS */
} STATUS;


/*******                                                        *********/
/*      Structure Definitions                                           */
/*******                                                        *********/

/* Structure to buffer trace data for a given SCN 
**************************************************/
typedef struct {       
  char     sta[TRACE2_STA_LEN];   
  char     chan[TRACE2_CHAN_LEN];
  char     net[TRACE2_NET_LEN];
  char     lc[TRACE2_LOC_LEN];
  char     quality[2];  /* Data-quality field                          */
  char     pad[3];      /* padding to make 28 bytes from top           */
  int      pinno;       /* pin number assigned to this scn             */
  double   starttime;   /* Time of earliest sample in this SCNs buffer */  
  double   endtime;     /* Time of last sample in this SCNs buffer     */
  double   samplerate;  
  int      writeP;      /* Buffer index for writing next sample        */
  long    *buf;         /* The actual buffer                           */
} SCN_BUFFER;

  /*    LISS2ew Parameter Structure (read from *.d files)      */
typedef struct
{
  char  debug;                          /* Write out debug messages?    */
                                        /*   ( 0 = No, 1 = Yes )        */
  char  myModName[MAXMODNAMELEN];      /* Name of this instance of the */
                                        /*   liss2ew module - REQUIRED  */
  char  ringOut[MAXRINGNAMELEN];        /* Name of ring to which        */
                                        /*   traces will be written -   */
                                        /* REQUIRED.                    */
  int   heartbeatInt;                   /* Heartbeat Interval(seconds). */
  int   logSwitch;                      /* 1 to write logs to disk      */
} PARAM;

  /*    Information Retrieved from Earthworm*.d                         */
typedef struct
{
  long  ringOutKey;             /* Key to output shared memory region   */
  unsigned char myInstId;       /* Installation running this module.    */
  unsigned char myModId;        /* ID of this module.                   */
  unsigned char typeError;      /* Error message type.                  */
  unsigned char typeHeartbeat;  /* Heartbeat message type.              */
  unsigned char typeWaveform;   /* Waveform message type.               */
} EWH;


/* LISS2EW Global structure                                             */
typedef struct
{
  EWH         Ewh;
  PARAM       Param;
  SHM_INFO    regionOut;
  MSG_LOGO    hrtLogo;
  MSG_LOGO    waveLogo;
  MSG_LOGO    errLogo;
  pid_t       MyPid;
  int         lenSEED;                /* How many bytes of miniSEED     */
  int         sockTimeout;            /* socket timeout in MILLISECONDS */
  int         traceLen;             /* Number of samples to put in TRACE_BUF */
} WORLD;

typedef struct
{
	int         IPport;
	char        IPaddr[MAXADDRLEN];   /* IP address or hostname */
	SCN_BUFFER  pscnB[MAX_SCN_PER_CONN]; /* pointer to array of SCN buffers  */
	int         Nscn;                 /* Number of SCNs to handle         */

	/* SCNL of packet received; this can be different than what's in pscnB, since
	 * we can use wildcard masks there.
	 */
	char		PacketSta[TRACE2_STA_LEN];   
	char		PacketChan[TRACE2_CHAN_LEN];
	char		PacketNet[TRACE2_NET_LEN];
	char		PacketLc[TRACE2_LOC_LEN];
	char		pad[1];		/* align on long boundry */

	char       *inBuf;
	long       *outBuf;               /* Buffer for output of SEED decoder */
	int         outBufLen;            /* Length of outBuf (natch!)         */
	dcptype     dcp;                  /* Steim decompression structure     */
	CTCPSocket *pSocket;
	unsigned int thread_id;
	int			thread_index;
} LISSConnection;

/* All the LISS2ew function prototypes other than SEED libraries */
int  Configure( char **);
int  ReadConfig( char * );
int  ReadEWH();
int  SetAddress(struct sockaddr_in *, WORLD);
void StatusReport(unsigned char, short, char *); 
int  ProcessData(LISSConnection *);
int  DisposePacket(LISSConnection *, SCN_BUFFER *, int, double);
int  SendTrace(LISSConnection *, SCN_BUFFER *);

#endif   /* __LISS2EW__  */

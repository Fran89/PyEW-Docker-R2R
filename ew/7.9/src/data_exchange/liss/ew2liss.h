/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ew2liss.h 205 2000-07-24 19:07:11Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2000/07/24 19:06:48  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/03/05 21:45:41  lombard
 *     Initial revision
 *
 *
 *
 */

/* ew2LISS definitions */

#ifndef __EW2LISS__
#define __EW2LISS__

#include <socket_ew.h>
#include <transport.h>
#include <trace_buf.h>
#include <mem_circ_queue.h>
#include <steim/steim.h>
#include <steim/steimlib.h>
#include <steim/miniseed.h>


#define MAXMESSAGELEN   160     /* Maximum length of a status or error  */
                                /*   message.                           */
#define MAXADDRLEN      80      /* Length of LISS hostname or address   */
#define MAXFILENAMELEN  100     /* Length of a file name                */
                                /*   close and reopen the socket        */
#define LC_LEN          4       /* Bytes for the location code          */
#define IN_QUEUE_SIZE   100     /* Number of TRACEBUF messages to hold  */
#define OUT_QUEUE_SIZE  50      /* Number of SEED messages to hold      */     
#define THREAD_STACK    8192    /* How big is our thread stack          */
#define GAP_THRESH      1.5     /* Size of gap, in sample intervals     */
#define LISS_SEED_LEN   512     /* Packet size from LISS                */
#define FIRSTFRAME      0       /* First data frame comes immediately   */
                                /*   after the DOB and DOE blockettes.  */
#define MAXFRAMES       7       /* 7 data frames is all that fits in 512*/
#define STEIM_DIFF      1       /* Differencing method for Steim        */
#define STEIM_LEVEL     2       /* Steim compression level for LISS     */
#define SEQ_COMMAND  "next_seq" /* sequence command in sequence file    */
#define MAXSEQ       1000000    /* Fill up six decimal digits           */

/*    StatusReport Error Codes - Must coincide with ew2LISS.desc        */
#define  ERR_MISSMSG       0   /* message missed in transport ring      */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer    */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded*/
#define  ERR_QUEUE         3   /* trouble with input queue operation    */
#define  ERR_OUTQUEUE      4   /* trouble with output queue operation   */

/*   Values for ServerStatus */
#define SS_DISCO           0   /* Server socket disconnected            */
#define SS_CONN            1   /* Server socket connected and sending   */

#ifdef _INTEL
#define FLIP 1
#else
#define FLIP 0
#endif

/*******                                                        *********/
/*      Structure Definitions                                           */
/*******                                                        *********/

/* Structure to buffer trace data for a given SCN 
**************************************************/
typedef struct {       
  char     sta[TRACE_STA_LEN];   
  char     chan[TRACE_CHAN_LEN];
  char     net[TRACE_NET_LEN];
  char     locID[5];       /* We need 2 bytes plus null, extra is padding */  
  char     quality[2];  
  short    sr;             /* memorized SEED samplerate factor           */
  short    sm;             /* memorized SEED samplerate multiplier       */
  double   pb_endtime;     /* Time of last sample in this peek buffer    */
  double   bestSR;         /* Best sample rate, from packet times        */
  SEED_data_record *sdp; /* pointer to structure for output of miniSEED  */
  gdptype  gdp;          /* pointer to Steim compressor data structure   */  
} SCN_BUFFER;

  /*    ew2LISS Parameter Structure (read from *.d files)      */
typedef struct
{
  char  debug;                          /* Write out debug messages?    */
                                        /*   ( 0 = No, 1 = Yes )        */
  char  myModName[MAX_MOD_STR];       /* Name of this instance of the */
                                        /*   liss2ew module - REQUIRED  */
  char  readInstName[MAX_INST_STR];    /* Name of installation that    */
                                        /*   is producing trace data    */
                                        /*   messages.                  */
  char  readModName[MAX_MOD_STR];     /* Name of module at the above  */
                                        /*   installation that is       */
                                        /*   producing the trace data   */
                                        /*   messages.                  */
  char  ringIn[MAX_RING_STR];         /* Name of ring to read from    */
                                        /* REQUIRED.                    */
  int   heartbeatInt;                   /* Heartbeat Interval(seconds). */
  int   logSwitch;                      /* 1 to write logs to disk      */
} PARAM;

  /*    Information Retrieved from Earthworm*.d                         */
typedef struct
{
  long  ringInKey;              /* Key to output shared memory region   */
  unsigned char myInstId;       /* Installation running this module.    */
  unsigned char myModId;        /* ID of this module.                   */
  unsigned char readInstId;     /* Retrieve trace messages from         */
                                /*   specified installation.            */
  unsigned char readModId;      /* Retrieve trace messages from         */
                                /*   specified module.                  */
  unsigned char typeError;      /* Error message type.                  */
  unsigned char typeHeartbeat;  /* Heartbeat message type.              */
  unsigned char typeWaveform;   /* Waveform message type.               */
} EWH;


/* EW2LISS Global structure                                             */
typedef struct
{
  EWH         Ewh;
  PARAM       Param;
  SHM_INFO    regionIn;
  MSG_LOGO    hrtLogo;
  MSG_LOGO    waveLogo;
  MSG_LOGO    errLogo;
  pid_t       MyPid;
  int         LISSport;
  char        LISSaddr[MAXADDRLEN]; /* IP address or hostname           */
  char        seqFile[MAXFILENAMELEN]; /* sequence number memory file   */
  long        seqNo;                /* LISS packet sequence number      */
  int         sockTimeout;          /* socket timeout in MILLISECONDS   */
  SCN_BUFFER *pscnB;                /* pointer to array of SCN buffers  */
  int         Nscn;                 /* Number of SCNs to handle         */
  QUEUE       tbQ;                  /* The TRACE_BUF queue for input    */
  mutex_t     tbQMutex;             /* For the input queue              */
  QUEUE       seedQ;                /* The miniSEED queue for output    */
  mutex_t     seedQMutex;           /* For the output queue             */
  int         ProcessStatus;        /* status of Process thread         */
  int         ServerStatus;         /* status of the Server thread      */
  int         terminate;            /* to tell all our threads to quit  */
  
  
} WORLD;


/* All the ew2LISS function prototypes other than SEED libraries */
void Configure( WORLD *, char **);
int  ReadConfig( WORLD *, char * );
int  ReadEWH( WORLD * );
void StatusReport( WORLD *, unsigned char, short, char * ); 
thr_ret ProcessThread(void * );
int FillPeekBuffer(SCN_BUFFER *, TracePacket * );
int  FillSeed( SCN_BUFFER *, int, WORLD * );
void setSeedStartTime( SCN_BUFFER *);
void install_SEED_header( SCN_BUFFER *, WORLD *);
void copy_chars( char *, char *, int);
void setSampleNumbers( SCN_BUFFER *);
double GetBestRate( TRACE_HEADER *);
int SendSeed(SCN_BUFFER *, WORLD *);
void CleanSeed( SCN_BUFFER *);
thr_ret ServerThread(void *);
int ServiceSocket(SOCKET, WORLD *);
int  SetAddress( struct sockaddr_in *, WORLD );

#endif   /* __LISS2EW__  */

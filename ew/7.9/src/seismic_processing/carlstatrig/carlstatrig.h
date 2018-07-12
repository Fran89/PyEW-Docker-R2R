
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: carlstatrig.h 6351 2015-05-13 00:30:04Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2006/10/20 15:44:37  paulf
 *     udpated with changes from Utah, STAtime now configurable
 *
 *     Revision 1.5  2005/04/12 22:43:07  dietz
 *     Added optional command "GetWavesFrom <instid> <module_id>"
 *
 *     Revision 1.4  2004/06/02 22:36:22  dietz
 *     added pad field to STATION struct for byte alignment
 *
 *     Revision 1.3  2004/05/05 23:54:04  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.2  2000/07/24 20:39:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/*
 * carlstatrig.h: Definitions for the CarlStaTrig Earthworm Module.
 */

/*******                                                        *********/
/*      Redefinition Exclusion                                          */
/*******                                                        *********/
#ifndef __CARLSTATRIG_H__
#define __CARLSTATRIG_H__

#include <earthworm.h>
#include <transport.h>  /* Shared memory                                */
#include <trace_buf.h>  /* TracePacket                                  */

/*******                                                        *********/
/*      Constant Definitions                                            */
/*******                                                        *********/

#define CST_VERSION     "v2.0"  /* version string for carlstatrig msg	*/

#define DOUBLE_EQUAL    0.0001  /* If the difference in value of        */
                                /*   variables of type double is less   */
                                /*   than this value, the variables are */
                                /*   considered equal.                  */

  /*    Error Codes - Must coincide with carl_trig.desc                 */
#define ERR_USAGE       1       /* Module started with improper params. */
#define ERR_INIT        2       /* Module initialization failed.        */
#define ERR_CONFIG_OPEN 3       /* Error opening configuration file.    */
#define ERR_CONFIG_READ 4       /* Error reading configuration file.    */
#define ERR_EWH_READ    5       /* Error reading earthworm.h info.      */
#define ERR_STA_OPEN    6       /* Error opening station file.          */
#define ERR_STA_READ    7       /* Error reading station file.          */
#define ERR_SUB_OPEN    8       /* Error opening subnet file.           */
#define ERR_SUB_READ    9       /* Error reading subnet file.           */
#define ERR_MALLOC      10      /* Memory allocation failed.            */

#define ERR_MISSMSG     11      /* Message missed in transport ring.    */
#define ERR_NOTRACK     12      /* Message retreived, but tracking      */
                                /*   limit exceeded.                    */
#define ERR_TOOBIG      13      /* Retreived message too large for      */
                                /*   buffer.                            */
#define ERR_MISSLAPMSG  14      /* Some messages were overwritten.      */
#define ERR_MISSGAPMSG  15      /* There was a gap in message seq #'s.  */
#define ERR_UNKNOWN     16      /* Unknown function return code.        */
#define ERR_PROC_MSG    17      /* Error processing a trace message.    */
#define ERR_PROD_MSG    18      /* Error producing a trigger message.   */


  /*    Buffer Lengths                                                  */
#define MAXFILENAMELEN  80      /* Maximum length of a file name.       */
#define MAXLINELEN      240     /* Maximum length of a line read from   */
                                /*   or written to a file.              */
#define MAXMESSAGELEN   160     /* Maximum length of a status or error  */
                                /*   message.                           */
#define BUFFSIZE MAX_TRACEBUF_SIZ * 4  /* Size of the station buffer    */

/*******                                                        *********/
/*      Macro Definitions                                               */
/*******                                                        *********/
#define CT_FAILED( a ) ( 0 != a )       /* Generic function failure     */
                                        /*   test.                      */


/*******                                                        *********/
/*      Enumeration Definitions                                         */
/*******                                                        *********/

  /*    Data type of trace data                                         */
typedef enum _DATATYPE
{
  UNKNOWN = 0,  /* Unknown data type.                                   */
  CT_SHORT = 1, /* Trace data is of type short.                         */
  CT_LONG = 2,  /* Trace data is of type long.                          */
  CT_FLOAT = 3, /* Trace data is of type float.                         */
  CT_DOUBLE = 4 /* Trace data is of type double.                        */
} DATATYPE;

  /*    Trigger status of the network, subnet, or station               */
typedef enum _TRIGGER
{
  TRIG_ON = 1,  /* Netowrk/Subnet is definitely triggered.              */
  TRIG_OFF = 2, /* Network/Subnet is definitely NOT triggered.          */
  TRIG_UNK = 3  /* Unable to determine status (not enough reporting     */
                /*   stations).                                         */
} TRIGGER;


/*******                                                        *********/
/*      Structure Definitions                                           */
/*******                                                        *********/

  /*    CarlTrig Trigger Parameter Structure (read from *.d files)      */
typedef struct _CSTPARAM
{
  char  debug;                          /* Write out debug messages?    */
                                        /*   ( 0 = No, 1 = Yes )        */
  char  myModName[MAX_MOD_STR];         /* Name of this instance of the */
                                        /*   CarlStaTrig module -       */
                                        /*   REQUIRED.                  */
  char  ringIn[MAX_RING_STR];           /* Name of ring from which      */
                                        /*   trace data will be read -  */
                                        /* REQUIRED.                    */
  char  ringOut[MAX_RING_STR];          /* Name of ring to which        */
                                        /*   triggers will be written - */
                                        /* REQUIRED.                    */
  char  staFile[MAXFILENAMELEN];        /* Name of file containing      */
                                        /*   station information -      */
                                        /* REQUIRED.                    */
  int   heartbeatInt;                   /* Heartbeat Interval(seconds). */
  long  ringInKey;                      /* Key to input shared memory   */
                                        /*   region.                    */
  long  ringOutKey;                     /* Key to output shared memory  */
                                        /*   region.                    */
  short nGetLogo;                       /* Number of logos in GetLogo   */
  MSG_LOGO *GetLogo;                    /* Logos of requested waveforms */
} CSTPARAM;

  /*    Information Retrieved from earthworm.d, earthworm_global.d      */
typedef struct _CSTEWH
{
  unsigned char myInstId;       /* Installation running this module.    */
  unsigned char myModId;        /* ID of this module.                   */
  unsigned char instWild;       /* Wildcard for Installation id         */
  unsigned char modWild;        /* Wildcard for Module id               */
  unsigned char typeCarlStaTrig;/* CarlStaTrig message type.            */
  unsigned char typeError;      /* Error message type.                  */
  unsigned char typeHeartbeat;  /* Heartbeat message type.              */
  unsigned char typeWaveform;   /* Waveform message type.               */
} CSTEWH;

typedef struct _STATION
{
  char          staCode[TRACE2_STA_LEN];    /* Station code (name).     */
  char          compCode[TRACE2_CHAN_LEN];  /* Component code.          */
  char          netCode[TRACE2_NET_LEN];    /* Network code.            */
  char          locCode[TRACE2_LOC_LEN];    /* Location code.           */
  char          pad;                        /* padding/byte alignment   */
  char          traceBuf[BUFFSIZE]; /* Buffered trace data.             */
  double        holdLTA;        /* Long-term average at calcTime.       */
  double        holdLTAR;       /* Long-term average at calcTime        */
                                /*   (rectified).                       */
  double        holdSTA;        /* Short-term average at calcTime.      */
  double        holdSTAR;       /* Short-term average at calcTime       */
                                /*   (rectified).                       */
  double        sampleRate;     /* Sample Rate (hz)                     */
  int           channelNum;     /* Unique channel number (pin number).  */
  int           dataSize;       /* Byte size of one data value.         */
  long          startupSamps;   /* number of samples before we can      */
                                /*   start using LTA values             */
  long          buffSamps;      /* Number of samples in the buffer      */
  double        buffRefTime;    /* Expected time of sample at           */
                                /*   traceBuf[bufSamps]                 */
  long          calcSamps;      /* Number of samples already calced     */
  long          calcSecs;       /* Latest time for which an STA/LTA     */
                                /*   calculation has been performed.    */
                                /*   This is the time of sample at      */
                                /*   traceBuf[calcSamps] in seconds     */
  long          numSSR;         /* Number of Samples Since Restart at   */
                                /*   calcTime.                          */
  double        trigOnTime;     /* Time when a station trigger went on  */
  double        trigOffTime;    /* Time when a station trigger went off */
  double        trigEta;        /* eta when the station trigger went on */  
  long          trigCount;      /* Serial number for station triggers   */
  DATATYPE      dataType;       /* Trace data type (short, float, etc.) */
  TRIGGER       trigger;        /* Trigger status at time bufTimeEnd.   */
} STATION;

  /*    CarlStaTrig World structure                                     */
typedef struct _WORLD
{
  CSTEWH*       cstEWH;         /* Pointer to the Earthworm parameters. */
  CSTPARAM      cstParam;       /* Network parameters.                  */
  SHM_INFO      regionOut;      /* Output shared memory region info     */
  MSG_LOGO      outLogo;        /* Logo of outgoing message.            */
  pid_t         MyPid;          /* My pid for restart from startstop    */
  double        Ratio;          /* CarlTrig parameter enumer / edenom.  */
  double        Quiet;          /* equiet CarlTrig parameter.           */
  STATION*      stations;       /* Array of Stations in the network.    */
  int           nSta;           /* Number of stations in the array      */
  int           LTAtime;        /* Number of seconds used in LTA        */
  long          STAtime;        /* Number of seconds used in STA        */

/*   
 *   Added member STAtime to WORLD structure.  Allows short period  
 *   average time window to be specified by user via the configuration
 *   file.  Note STAtime is of type long.  Previously hard coded to  
 *   1 second. Note that LTAtime is now in units of STAtime, not seconds. (if STAtime is specified)
 *   Michelle Kline, University of Utah Seismograph Stations, 
 *   07/14/06                                                 
 */

  int           startUp;        /* Number of seconds of trace data      */
                                /*   before we can start using LTAs     */
  long          decimation;     /* Trace data decimation factor.        */
  long          maxGap;         /* Maximum gap between trace data       */
                                /*   points that can be interpolated    */
                                /*   (otherwise restart the station).   */
} WORLD;

/*      All the CarlStaTrig function prototypes                         */
int AppendTraceData( TracePacket *data, STATION *station, WORLD *cstWorld );
int CompareSCNs( const void *s1, const void *s2 );
STATION * FindStation( char *staCode, char *compCode, char *netCode, 
                       char *locCode, WORLD *cstWorld );
int FlushBuffer( STATION* station, int debug );
int InitializeParameters( CSTEWH *cstEwh, WORLD *cstWorld );
int InitializeStation( STATION *station );
int InterpolateGap( long gapSize, char *traceData, STATION *station );
int ProduceStationTrigger( STATION *station, WORLD *cstWorld );
int ProcessTraceMsg( WORLD *cstWorld, char *msg );
int ReadConfig( char *filename, WORLD *cstWorld );
int ReadEWH( CSTPARAM *cstParam, CSTEWH *cstEwh );
int ReadStations( WORLD* cstWorld );
void ResetStation( STATION *station );
void StatusReport( WORLD *cstWorld, unsigned char type, short code, 
                   char *message );
int UpdateStation( STATION *station, WORLD *cstWorld );


#endif /*       __CARLSTATRIG_H__                                       */


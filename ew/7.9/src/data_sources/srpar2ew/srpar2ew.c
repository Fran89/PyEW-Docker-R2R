/* FILE: srpar2ew.c           Copyright (c), Symmetric Research, 2004-2010
 *
 * SrPar2Ew is a program for sending data from a SR PARxCH 24 bit
 * system to an Earthworm WAVE_RING buffer as TYPE_TRACEBUF messages.  It
 * works with all three PARxCH systems, one, four, and eight channels.
 * It can also support 16 channels if run with 2 PAR8CH that are on the same
 * PC and share one PARGPS.  It is similar in concept and organization to
 * the ADSEND module except it works for the SR PARxCH DAQs.
 *
 *
 * What you need to run:
 *    The Earthworm system installed
 *    A Symmetric Research PARxCH A/D board (PARGPS recommended too)
 *    The srpar2ew executable (.exe), config (.d) and error (.desc) files
 *    You must change the values in srpar2ew.d so they match your A/D
 *    You will probably also want to change some values in other .d files
 *       like wave_serverV.d or export.d to save or process the acquired data
 *
 *
 * What you need to compile:
 *   The Earthworm include and library files
 *   The srpar2ew source
 *   The PARxCH software source (you must define GPS_AVAILABLE at compile time)
 *   The PARGPS software source (required even if not using the PARGPS hardware)
 *   Note: The PARxCH and PARGPS source is available free
 *         from the symres.com website
 *
 *
 * Please refer to the readme.txt and srpar2ew_ovr.html files for
 * information about how to run this program.  Also see the sample
 * srpar2ew.d configuration file and srpar2ew_cmd.html file
 * for a discussion of the configuration parameters and their options.
 *
 *
 * How this program is organized:
 *
 * The main routine calls several subroutines to initialize Earthworm,
 * the PARxCH and PARGPS, and some software buffers.  Then it starts the
 * PARxCH and PARGPS and goes into an endless loop reading data and
 * sending out heartbeat and tracebuf messages.  The loop can be
 * terminated by setting the Earthworm terminate flag.
 *
 * SrPar2Ew tries to acquire roughly 1 second of data at a time.
 * Once it has done so, the heartbeat and tracebuf messages are sent.
 * This means heartbeats will never be sent any faster than 1 per second
 * and sampling rates beyond 1000 samples/second are forbidden since
 * they would exceed the size allocated for tracebuf messages.  While a
 * sampling rate of 1000 SPS has been achieved for a short time on our
 * test machines, there are no guarantees that your Earthworm
 * configuration can support rates this fast over the long term.
 *
 * The Earthworm initialization involves setting up the log file,
 * reading the .d configuration file, looking up Earthworm info from the
 * earthworm.h file, and attaching to a shared memory region or "ring"
 * which allows various Earthworm modules, each of which is a separate
 * program, to communicate with each other.
 *
 * The PARxCH initialization involves opening the PARxCH and PARGPS
 * drivers, initializing the PARxCH and PARGPS hardware, and setting
 * default values for a variety of acquisition parameters.  Routines
 * with sr_atod and sr_ts in their name are the only ones which call PARxCH
 * and PARGPS hardware specific functions supplied by the PARxCH and PARGPS
 * libraries that come with the hardware and are available for free
 * download from the Symmetric Research website, symres.com.  Use of the
 * PARGPS for timestamping the data is optional but highly recommended.
 * If it is not used, the time assigned to the acquired data will be
 * determined by adding an elapsed time, computed assuming the nominal
 * sampling rate is totally accurate, to the PC time when acquisition
 * started.
 *
 * Three large arrays are allocated during the initialization phase:
 * AtodBuffer1,2 and TraceBuffer.  AtodBufferFill points to the buffer
 * which will be filled with acquired data read from the PARxCH, until
 * it is copied into TraceBuffer which holds the data in Earthworm
 * tracebuf message format until it is written out to the WAVE_RING.
 * Because two data buffers will be used, AtodBufferWrite points to the
 * other buffer, which was filled earlier, and is the one used to copy
 * data into TraceBuffer.
 *
 * The routine sr_atod_getdata takes care of reading in the acquired
 * data and, if the PARGPS is being used, the PPS and serial NMEA timing
 * information.  A loop is used for reading in the requested amount of
 * acquired data.  After each pass through the loop, if more data is
 * still needed to fill the request, the program sleeps awhile to let
 * other programs run while it gives the remaining data a chance to
 * arrive.  The sleep time has been set so 5 or 6 passes through the
 * loop should be needed to satisfy the request.  If the data request is
 * satisfied on the very first pass, it means data has accumulated in
 * the FIFO on the PARxCH board faster than it is being read, presumably
 * because SrPar2Ew is not getting enough CPU cycles to keep up.
 * While this can be tolerated for a little while, the FIFO will
 * eventually overflow if this situation persists.
 *
 * If the PARGPS is being used to time stamp the data, sr_atod_getdata
 * also reads in three different types of GPS information which it
 * stores in a temporary timestamp area.  These three types of GPS
 * information are 1) mark data, 2) PPS data, and 3) serial NMEA data.
 * Once all three types are available for a given second, the info is
 * save as a timestamp.
 *
 * We maintain two sets of timestamps.  One set, TsReal, contains the
 * last four raw timestamps.  The information in these timestamps is
 * exactly what has been extracted from the data.  The other set,
 * TimeStamp, contains the last three final timestamps.  These timestamps
 * are used to compute the time for each tracebuf.  They may have been
 * modified from the raw values in order to correct for an outlier.
 *
 * Mark data refers to the index or sample number of the mark point, the
 * first sample acquired after the start of a second as indicated by the
 * arrival of the high precision PPS pulse per second signal.  PPS data
 * refers to the values the high speed PC counter has at the time of PPS
 * and again at the time of Dready (data ready) for the Mark point.
 * Serial NMEA data refers to the coarse GPS timing and location
 * information containing the time and date for the PPS second.  These
 * three types of information are stored at slightly different times and
 * in different structures within the PARxCH and PARGPS drivers.  So
 * each PPS signal is given an index, known as the PPS event number or
 * the mark number, which is used to tie the three types of GPS
 * information together.
 *
 * Because each buffer of acquired data is roughly 1 second long, we
 * should see one PPS signal per buffer.  However, the buffers are not
 * exactly 1 second long, but are slightly longer.  This means we should
 * periodically get a buffer that includes two PPS signals.  This "extra"
 * PPS is correct and expected.  It is logged at Debug level 2.
 *
 *
 * Note: Global variables are capitalized, locals have first letter lower case.
 *       Tab stops are set at 3.
 */

/* changes:

  Revision 1.4  2007/12/05  W.Tucker:
     added support for Garmin GPS 18 LVC receiver

  Revision 1.3  2006/09/30  W.Tucker:
     added support for 2nd PAR8CH to share GPS with 1st PAR8CH for 16 channels

  Revision 1.2  2006/02/25  W.Tucker:
     delayed tracebuf output by 1 sec to ensure endtimes are calculated
         with valid timestamps
     updated to support the new TYPE_TRACEBUF2 messages and SCNL location codes
     improved messages for various Debug output levels

  Revision 1.1  2004/12/30  W.Tucker:
     improved non-GPS timing to use fractional seconds
     added GPS lock status logging

  Revision 1.0  2004/11/10  W.Tucker:
     first version
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>   /* inst, mod + ring globals, logit utilities, etc */
#include <kom.h>         /* configuration parsing  k_xxx                   */
#include <time_ew.h>     /* OS independent time functions                  */
#include <transport.h>   /* ring transport utilities  tport_xxx            */
#include <trace_buf.h>   /* tracebuf waveform structure                    */

#include "parxch.h"      /* PARxCH DAQ defines and library routines        */
#include "pargps.h"      /* PARGPS timing defines and library routines     */


/* This code is designed to support both v7.0 which knows about the new
 * TYPE_TRACEBUF2 and v6.3 which doesn't.  If the new TRACE2 defines exist
 * all is OK, otherwise, we just dummy them out to match the old versions.
 **************************************************************************/
#if defined( TRACE2_VERSION0 )
#define TRACE2_OK        1
#else
#define TRACE2_OK        0
#define TRACE2_CHAN_LEN  4
#define TRACE2_LOC_LEN   3
#define TRACE2_VERSION0  '2'   /* version[0] for TYPE_TRACEBUF2       */
#define TRACE2_VERSION1  '0'   /* version[1] for TYPE_TRACEBUF2       */
#define LOC_NULL_STRING  "--"  /* NULL string for location code field */
#endif

#define TRACE_POS_LOC    (TRACE2_CHAN_LEN)
#define TRACE_POS_VER    (TRACE_POS_LOC+TRACE2_LOC_LEN)


/* Prototypes of functions in this file
 **************************************/
int   main( int, char ** );
void  sr_ew_init( char *, char * );
void  sr_atod_init( void );
void  sr_atod_init_model( void );
void  sr_atod_open( void );
void  sr_atod_correct_sps( double * );
void  sr_summary_init( void );
void  sr_ts_init( void );
void  sr_filter_init( void );
void  sr_allocate_arrays( void );
void  sr_atod_start( int, int );
void  sr_atod_synchronize( void );
int   sr_atod_getdata( void );
void  sr_send_traces( void );
unsigned int sr_ts_process_analog( int * );
unsigned int sr_ts_process_pps( void );
unsigned int sr_ts_process_serial( void );
int   sr_analog_startup_checks( int, long * );
int   sr_analog_process_secondary( int, long * );
int   sr_analog_check_spacing( int , long * );
int   sr_ts_update( int );
void  sr_filter( int, double );
void  sr_ts_set_tracebuf_time( void );
int   sr_compute_daq_times( double, int, int, int );
int   sr_ts_match_ppsevent( int, int, int * );
int   sr_ts_select( long, int, int *, int * );
void  sr_atod_stop( void );
void  sr_atod_close( void );
void  sr_atod_restart_recovery( void );
void  sr_check_terminate( void );
void  sr_terminate( void );
int   sr_verify_string( int, char *, int * );
void  sr_read_config( char * );
void  sr_lookup_ew( void );
void  sr_heartbeat( void );
void  sr_atod_toggle_led( void );
void  sr_error( int, char * );
int   sr_strncpy( char *, char *, int );
void  sr_log_analog_data( void );
void  sr_log_gap_details( int, double, double );
void  sr_log_trace_head( void );


/* Defines and typedefs
 **********************/
#define REV_STRING  "SrPar2Ew Rev 09/25/06\n"
#define MAX_STR      256      /* Maximum dimension for some strings      */
#define MAX_CHAN      11      /* Maximum number of channels allowed      */
#define MAX_BOARD      2      /* Maximum number of DAQ boards allowed    */
#define MAX_TS         5      /* Maximum number of good timestamps       */
#define MAX_TSR        4      /* Maximum number of raw timestamps        */
#define MAX_GPS        2      /* Max num GPS records read at once        */
#define QUALITY_OK   0x0      /* See trace_buf.h for other codes         */
#define BEHIND_LIMIT  20      /* Num times acquisition falls behind
                                 before printing a warning               */
#define CBP_RANGE  0.00005L   /* cnt betw PPS must be w/in 50 ppm        */
#define CBD_RANGE  0.00005L   /* cnt betw Drdy must be w/in 50 ppm       */

#define FILTER_PPS     1      /* filter best count between PPS           */
#define FILTER_DREADY  2      /* filter best count between Dready        */

#define TIME_INIT_NO   0      /* On restart, do not init times in start  */
#define TIME_INIT_YES  1      /* On first start, do init times in start  */

#define EW_ERROR_BASE_PARXCH  100 /* Convert PARXCH_ERROR_XXX to EW ierr */
#define EW_ERROR_BASE_PARGPS  200 /* Convert PARGPS_ERROR_XXX to EW ierr */

#define SLOPMS         0.01L  /* Windows system time only good to 10ms   */

#define MAXGAPTRAIL    2      /* Dbg: Num buffers to log after a gap     */
#define MAX_PREV_SER   6      /* Dbg: Num prev serial to show after gap  */

#define LAST_PT_INVALID 0x80000000L /* Invalid point number              */

#define TOGGLE_MASK    0x01;  /* Pick GPS toggle signal from digital IO  */

#define COMPUTE_START  1      /* compute a start time for 2ndary DAQ     */
#define COMPUTE_END    2      /* compute an end  time for 2ndary DAQ     */

typedef struct _SCNL          /* Channel information structure           */
{
   char sta[ TRACE_STA_LEN];
   char comp[TRACE_CHAN_LEN]; /* Older value has larger dimension        */
   char net[ TRACE_NET_LEN];
   char loc[ TRACE2_LOC_LEN];
   char ver[2];
   int  pin;
}
SCNL;

/* Global variables
 ******************/

/* Things to read or derive from configuration file
 **************************************************/
static char   MyModName[MAX_MOD_STR];   /* speak as this module name/id      */
static char   RingName[MAX_RING_STR];   /* name of transport ring for i/o    */
static int    LogSwitch;                /* 0 if no logfile should be written */
static long   HeartBeatInterval;        /* seconds between heartbeats        */

static char   AtodDriverName[MAX_BOARD][MAX_STR];  /* PARxCH driver name     */
static char   AtodXchModelName[MAX_BOARD][MAX_STR];/* PARxCH model name      */
static char   AtodPortModeName[MAX_BOARD][MAX_STR];/* PARxCH port mode name  */
static double AtodRequestedSps;         /* requested sampling rate           */
static int    UsingGps;                 /* Is PARGPS being used              */
static char   GpsDriverName[MAX_STR];   /* PARGPS driver name                */
static char   GpsModelName[MAX_STR];    /* GPS model name (eg PARGPS)        */
static int    GpsSerialPort;            /* PC serial COM port for GPS        */
static SCNL   ChanList[MAX_BOARD*MAX_CHAN];/* Channel info:site,comp,net,pinno  */
static int    OutputMsgType;            /* TYPE _TRACEBUF or _TRACEBUF2      */



/* Things to look up in the earthworm.d tables
 *********************************************/
static long          RingKey;           /* key of transport ring for i/o     */
static unsigned char InstId;            /* local installation id             */
static unsigned char MyModId;           /* module id for this program        */
static unsigned char TypeHeartBeat;     /* msg type id for heartbeats        */
static unsigned char TypeError;         /* msg type id for errors            */
static unsigned char TypeTraceBuf;      /* msg type id for old tracebuf      */
static unsigned char TypeTraceBuf2;     /* msg type id for new tracebuf2     */



/* Things computed or set in the code
 ************************************/
static SHM_INFO       Region;          /* shared memory region to use for i/o    */
static MSG_LOGO       Logo;            /* msg header with module,type,instid     */
static pid_t          MyPid;           /* for restarts by startstop              */
static time_t         TimeNow;         /* current time in sec since 1970         */
static time_t         TimeLastBeat;    /* time last heartbeat was sent           */

static long           TraceBufSize;    /* size in bytes of tracebuf message      */
static char          *TraceBuffer;     /* tracebuf message buffer                */
static long          *TraceDat;        /* where in TraceBuffer the data starts   */
static TRACE_HEADER  *TraceHead;       /* where in TraceBuffer old header starts */

static long          AtodNumBoards;     /* number of DAQ boards being used (max2) */
static long          AtodNumChannels[MAX_BOARD];/* DAQ channels being acquired    */
static long          AtodNumChannelsTotal;/* total DAQ channels being acquired   */
static long          AtodNumSamples;    /* number of data samples per tracebuf    */
static long          AtodNumSps;        /* number of data samples per sec rounded */
static double        AtodSps;           /* actual sampling rate                   */
static int           AtodLedValue;      /* state of PARxCH yellow LED             */
static long          AtodBufSize;       /* size in bytes of acquired data buffer  */
static long         *AtodBuffer1;       /* pointer to first acquired data buffer  */
static long         *AtodBuffer2;       /* pointer to next acquired data buffer   */
static long         *AtodBufferFill;    /* pointer to data buffer to fill         */
static long         *AtodBufferWrite;   /* pointer to data buffer to write out    */
static int           AtodXchModel[MAX_BOARD];/* which PARxCH: 1CH, 4CH, or 8CH    */
static int           AtodPortMode[MAX_BOARD];/* PC port mode: EPP, BPP, etc       */
static double        AtodStartTime[MAX_BOARD];/* start time of next tracebuf      */
static double        AtodEndTime[MAX_BOARD];/* end time of next tracebuf          */
static double        AtodEndTimeLast[MAX_BOARD];/* end time of last tracebuf      */
static int           AtodCntrChan[MAX_BOARD];/* index of PAR8CH GPS OBC chan      */
static int           AtodMarkChan[MAX_BOARD];/* index of PARxCH GPS mark chan     */
static int           AtodDataRequest;   /* what data to read, eg analog, gps, etc */
static long          AtodCurrentPtFill; /* index of current fill pt since start   */
static long          AtodCurrentPtWrite;/* index of current write pt since start  */
static unsigned long AtodBufferNum;     /* index numer of current buffer          */
static int           GpsModel;          /* which Gps model (eg PARGPS)            */
static long          ObcMarkCount[MAX_BOARD];/* GPS count for optional PAR8CH     */
static long          LastPtMark[MAX_BOARD];/* Prev point containing mark value    */
static long          LastPtObc[MAX_BOARD];/* Prev point containing OBC value      */
static long          LastMark[MAX_BOARD]; /* Prev PAR8CH mark value               */
static long          LastObc[MAX_BOARD];  /* Prev PAR8CH OnBoardCounter value     */
static int           GapTrailer[MAX_BOARD];/* debug count of buffers after gap    */
static unsigned int  LastDataPpsEvent;  /* Prev PpsEvent num seen in analog data  */
static unsigned int  LastPpsPpsEvent;   /* Prev PpsEvent num seen in PPS data     */
static unsigned int  LastSerialPpsEvent;/* Prev PpsEvent num seen in serial data  */

static DEVHANDLE ParGpsHandle;          /* handle to PARGPS device driver         */
static DEVHANDLE ParXchHandle[MAX_BOARD];/* handle to PARxCH device drivers       */

static TIMESTAMP TimeStamp[MAX_BOARD][MAX_TS];/* Good or corrected time stamps    */
static TIMESTAMP TsReal[MAX_TSR];       /* Time stamps from raw data, no mods     */
static TIMESTAMP TsFix;                 /* Time stamp corrected for outlier       */

static PPSTDATA   GpsPpsBuffer[MAX_GPS];    /* GPS PPS data                        */
static SERIALDATA GpsSerialBuffer[MAX_GPS]; /* GPS serial NMEA data                */
static SERIALDATA GpsPrevSerial[MAX_PREV_SER]; /* Dbg: Prev GPS serial to save     */

static long       AtodNumPps;               /* Number GPS PPS records read         */
static long       AtodNumSerial;            /* Number GPS serial NMEA records read */
static double     ObcCountsPerSample;       /* PAR8CH OBC counts per data point    */
static double     BestObcCountsPerSample;   /* PAR8CH OBC counts per data point    */
static int        FirstTimeFilter;          /* 1 if BestCountBetweenXxx need init  */
static int        FirstTimeSerial;          /* 1 if PrevSecSince1970 needs init    */
static int        FirstTimePctime;          /* 1 if starting to use time from PC   */
static int        FirstTimeData;            /* 1 if data needs init after start    */
static int        SleepMs;                  /* Ms to wait for more data to arrive  */
static int        PcCounter1Type;           /* PC counter depends on OS            */
static int        PcCounter2Type;           /* PC counter depends on OS            */
static int        UseTimeMethod;            /* how to compute trace start time     */
static int        LastTimeMethod;           /* how last trace start time computed  */
static int        TsValid;                  /* TS_VALID_ALL unless time from PC    */

static double     BestCountsBetweenPps;     /* 64 bit PC counts between 2 PPS      */
static double     BestCountsBetweenDready;  /* 64 bit PC counts between 2 Dready   */
static double     BestCountsPerSecond;      /* 64 bit PC counts between 2 seconds  */
static double     CbpLimit;                 /* ppm range around PC counter freq    */
static double     CbdLimit;                 /* ppm range around PC counts/sample   */
static double     CbpCoeff;                 /* count betw. PPS filter coefficient  */
static double     CbpScale;                 /* count betw. PPS filter scaling      */
static double     CbdCoeff;                 /* count betw. Dready filter coeff.    */
static double     CbdScale;                 /* count betw. Dready filter scaling   */
static double     RestartBegin;             /* time when restart begins            */

static long       GpsLockReportInterval;    /* sec in log betw. gps lock reports   */
static long       GpsLockBadLimit;          /* num bad locks/interval for error    */
static long       GpsLockCount;             /* num sec so far in gps lock summary  */
static long       GpsLockGood;              /* num good locks in current count     */
static long       GpsLockBad;               /* num bad  locks in current count     */
static long       GpsLockStatus;            /* 1 if last gps lock report was good  */

static long       SummaryReportInterval;    /* sec in log between summary reports  */
static long       SummaryCount;             /* num sec so far in general summary   */
static long       SummaryMin[MAX_BOARD][MAX_CHAN];/* min data value per channel    */
static long       SummaryMax[MAX_BOARD][MAX_CHAN];/* max data value per channel    */


static int Debug = 0;                       /* 0,1,2,3 = few, some, many... logits */


double DbgLastStartDiff, DbgLastStart;
double DbgLastEndDiff, DbgLastEnd;


/* Shared string for temp use
 ****************************/
static char Msg[MAX_STR];                   /* string for log messages       */
static char ErrMsg[MAX_STR];                /* string for error messages     */






/******************************************************************************
 *  Function: srpar2ew main                                                   *
 *  Purpose:  SrPar2Ew is an earthworm module for acquiring waveform data     *
 *            from one of the Symmetric Research 24 bit PARxCH A/D (DAQ)      *
 *            systems.  After several initialization routines are called,     *
 *            this program goes into a loop acquiring data and sending it out *
 *            to an earthworm shared memory region (ring).  A heartbeat       *
 *            message is also sent on each pass through the loop.  The loop   *
 *            continues until an earthworm terminate request is received, a   *
 *            Ctrl+C is pressed, or a fatal error occurs.                     *
 ******************************************************************************/
int main( int argc, char **argv )
{
  /* Show program banner
   *********************/
//   printf( REV_STRING );


  /* Check command line arguments
   ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: srpar2ew <configfile>\n" );
        exit( 0 );
   }


  /* Earthworm initialization
   **************************/
   sr_ew_init( argv[0], argv[1] );



  /* PARxCH DAQ initialization
   ***************************/
   sr_atod_init();


  /* Allocate acquired data (input) + tracebuf (output) arrays
   ***********************************************************/
   sr_allocate_arrays();


  /* Begin acquiring after 2 second wait
   *************************************/
   sr_atod_start( 2, TIME_INIT_YES );


  /* Main acquisition loop
   ***********************/
   sr_heartbeat();
   sr_atod_getdata();                /* Get a starting buffer of data */
   sr_heartbeat();

   while ( 1 )
   {
       sr_check_terminate();         /* Exit if termination requested */

       if ( sr_atod_getdata() > 0 )  /* Read new acquired data        */
       {
           sr_heartbeat();           /* Send heartbeat message        */
           sr_send_traces();         /* Send tracebuf messages        */
       }

   } /* end while */

   exit( 0 );
}

/******************************************************************************
 *  Function: sr_ew_init                                                      *
 *  Purpose:  Perform the standard initialization tasks for a well behaved    *
 *            earthworm module.  These include starting a log file, reading   *
 *            the configuration file, looking up any global earthworm data,   *
 *            getting the process id, and attaching to an earthworm shared    *
 *            memory region (ring).                                           *
 ******************************************************************************/
void sr_ew_init( char *argv0, char *argv1 )
{
   int i;

   if ( Debug >= 6 )  logit( "", "Starting sr_ew_init\n" );


  /* Init some values to ensure safe exit on error
   ***********************************************/
    ParGpsHandle  = BAD_DEVHANDLE;
    for ( i = 0 ; i < MAX_BOARD ; i++ )
       ParXchHandle[i]  = BAD_DEVHANDLE;
    Region.addr   = NULL;


  /* Initialize and open log-file
   ******************************/
   logit_init( argv1, 0, 256, 1 );
   if ( Debug >= 0 )  logit( "", REV_STRING );


  /* Read the configuration file
   *****************************/
   sr_read_config( argv1 );
   if ( Debug >= 0 )  logit( "", "%s: Read command file <%s>\n", argv0, argv1 );


  /* Look up important info from earthworm.d tables
   ************************************************/
   sr_lookup_ew();

   Logo.instid = InstId;
   Logo.mod    = MyModId;


  /* Reinitialize logit to desired logging level
   *********************************************/
   logit_init( argv1, 0, 256, LogSwitch );


  /* Get process ID for heartbeat messages
   ***************************************/
   MyPid = getpid();
   if ( MyPid == -1 )
   {
     logit( "e", "srpar2ew: Cannot get pid. Exiting.\n" );
     exit( -1 );
   }


  /* Attach to shared memory ring
   ******************************/
   tport_attach( &Region, RingKey );
   if ( Debug >= 0 )
      logit( "", "Attached to public memory region %s: %d\n",
             RingName, RingKey );


  /* Force a heartbeat to be issued on the first call
   **************************************************/
   TimeLastBeat = time( &TimeNow ) - HeartBeatInterval - 1;

   if ( Debug >= 6 )  logit( "", "Leaving  sr_ew_init\n" );
   return;
}


/******************************************************************************
 *  Function: sr_atod_init                                                    *
 *  Purpose:  Initialize the PARxCH DAQ and optional PARGPS timing modules    *
 *            and the parameters related to them.  First open the device      *
 *            drivers and initialize the hardware.  Next set various DAQ      *
 *            parameters.  Some are common to all PARxCH, while others        *
 *            depend on which XchModel is being used.  Finally, initialize    *
 *            the GPS timestamp structures which are used in determining the  *
 *            start time for each tracebuf.                                   *
 ******************************************************************************/
void sr_atod_init( void )
{
    int  i;
    long pcCounter1FreqHi, pcCounter1FreqLo, pcCounter2FreqHi, pcCounter2FreqLo;

    if ( Debug >= 6 )  logit( "", "Starting sr_atod_init\n" );


   /* Correct requested Sps
    ***********************/
    sr_atod_correct_sps( &AtodRequestedSps );


   /* Open and initialize the PARxCH and PARGPS
    *******************************************/
    sr_atod_open();


   /* Set some default DAQ values
    *****************************/
    AtodLedValue       = 0;
    AtodCurrentPtFill  = 0L;
    AtodCurrentPtWrite = 0L;
    AtodBufferNum      = 0L;
    AtodNumPps         = 0L;
    AtodNumSerial      = 0L;

    // WCT: Right now this limits us to about 1000 samples per second
    AtodNumSamples     = (long)AtodSps + 1;      /* want 1 sec of data per tracebuf */
    AtodNumSps         = (long)(AtodSps + 0.5);  /* rounded number of samples per sec */

    AtodDataRequest    = PARXCH_REQUEST_ANALOG | PARXCH_REQUEST_DIGITAL;

    for ( i = 0 ; i < AtodNumBoards ; i++ )
    {
       AtodMarkChan[i] = INDEXCHANNEL_NONE;
       AtodCntrChan[i] = INDEXCHANNEL_NONE;
    }

    UseTimeMethod      = TIME_METHOD_SPS;
    LastTimeMethod     = TIME_METHOD_NONE;
    ObcCountsPerSample = OBC_FREQ / AtodSps;
    SleepMs            = 1000 / 5;  /* If we're keeping up with the data it */
                                    /* will take 5 tries to get 1 sec worth */

    BestObcCountsPerSample = ObcCountsPerSample;



   /* Set DAQ values that depend on XchModel and GpsModel
    *****************************************************/
    sr_atod_init_model();



   /* Announce acquisition channels to log file
    *******************************************/
    if ( Debug >= 0 )
    {
       logit( "", "Acquiring from the following %d channels\n", AtodNumChannelsTotal );
       logit( "", "               Sta Comp Net Loc Pin\n" );

       for ( i = 0 ; i < AtodNumChannelsTotal ; i++ )
          logit( "", "Channel %2d = %6s %3s %3s %2s %4d\n",
                 i, ChanList[i].sta, ChanList[i].comp,
                 ChanList[i].net, ChanList[i].loc, ChanList[i].pin );
    }



   /* Initialize general logging parameters
    ***************************************/
    sr_summary_init();


    PcCounter1Type   = DEFAULT_PC_COUNTER_TYPE;
    pcCounter1FreqLo = 1;
    PcCounter2Type   = DEFAULT_PC_COUNTER2_TYPE;
    pcCounter2FreqLo = 1;

    if ( UsingGps )
    {
        ParGpsGetFullCounterFrequency( ParGpsHandle,
                                       &pcCounter1FreqHi, &pcCounter1FreqLo,
                                       &pcCounter2FreqHi, &pcCounter2FreqLo
                                     );
        if (pcCounter1FreqLo < 0.0001)  /* prevent future divide by 0 */
            pcCounter1FreqLo = 1;
        if (pcCounter2FreqLo < 0.0001)  /* prevent future divide by 0 */
            pcCounter2FreqLo = 1;
    }



   /* PC counter runs up to 100 ppm faster or slower than the official
    * rate, so consider anything beyond the ppm range to be an outlier.
    * We want to track the number of 64 bit PC counts between one PPS
    * and the next and the counts between one Dready mark and the next.
    * This allows us to know if any other program has excessively delayed
    * the servicing of PC interrupts in that time which would create
    * outliers in our data that we'd like to exclude.
    ******************************************************************/
    BestCountsBetweenPps    = pcCounter1FreqLo;
    CbpLimit                = BestCountsBetweenPps * 0.002L;     /* 2000 ppm */

    BestCountsBetweenDready = ((double)pcCounter1FreqLo * AtodNumSps) / AtodSps;
    CbdLimit                = BestCountsBetweenDready * 0.002L;  /* 2000 ppm */

    BestCountsPerSecond     = pcCounter2FreqLo; /* For world time counter */

    if ( Debug >= 3 )
        logit( "", "Starting BestCountsBetween Pps %lf, Dready %lf, Sec %lf (Nsps %ld, Sps %lf)\n",
               BestCountsBetweenPps, BestCountsBetweenDready, BestCountsPerSecond,
               AtodNumSps, AtodSps );


   /* Initialize GPS logging parameters
    ***********************************/
    GpsLockCount = GpsLockGood = GpsLockBad = 0;
    GpsLockStatus = -1;


   /* Initialize GPS timestamp structures
    *************************************/
    sr_ts_init();


   /* Initialize averaging filters
    ******************************/
    sr_filter_init( );


    if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_init\n" );

    return;
}

/******************************************************************************
 *  Function: sr_summary_init                                                 *
 *  Purpose:  Initialize some summary parameters used for logging.            *
 ******************************************************************************/
void sr_summary_init( void )
{
    int i, j;

    if ( Debug >= 6 )  logit( "", "Starting sr_summary_init\n" );

    SummaryCount = 0;

    for ( i = 0 ; i < AtodNumBoards ; i++ )
       for ( j = 0 ; j < AtodNumChannels[i] ; j++ )
       {
          SummaryMin[i][j] = 0x00FFFFFFL;
          SummaryMax[i][j] = 0x00000000L;
       }

    if ( Debug >= 6 )  logit( "", "Leaving  sr_summary_init\n" );

    return;
}

/******************************************************************************
 *  Function: sr_atod_init_model                                              *
 *  Purpose:  Initialize some PARxCH DAQ parameters that depend on XchModel   *
 *            and GpsModel.  For example, number of channels.                 *
 ******************************************************************************/
void sr_atod_init_model( void )
{
    int i;

    if ( Debug >= 6 )  logit( "", "Starting sr_atod_init_model\n" );


    AtodNumChannelsTotal = 0;

    for ( i = 0 ; i < AtodNumBoards ; i++ )
    {

       if ( AtodXchModel[i] == PARXCH_MODEL_PAR1CH )       /* PAR1CH */
       {
           AtodNumChannels[i] = PAR1CH_ANALOG_CHANNELS +
                                PAR1CH_DIGITAL_CHANNELS;
           if ( UsingGps )
           {
               AtodNumChannels[i] += PAR1CH_GPS_CHANNELS;
               AtodMarkChan[i]     = AtodNumChannels[i]-1;
               AtodCntrChan[i]     = INDEXCHANNEL_NONE;
               AtodDataRequest    |= PARXCH_REQUEST_GPS;
               UseTimeMethod       = TIME_METHOD_GPS;
               if (GpsModel == GPSMODEL_PCTIME)
                       UseTimeMethod = TIME_METHOD_PCT;
           }
       }

       else if ( AtodXchModel[i] == PARXCH_MODEL_PAR4CH )   /* PAR4CH */
       {
           AtodNumChannels[i] = PAR4CH_ANALOG_CHANNELS +
                                PAR4CH_DIGITAL_CHANNELS;
           if ( UsingGps )
           {
               AtodNumChannels[i] += PAR4CH_GPS_CHANNELS;
               AtodMarkChan[i]     = AtodNumChannels[i]-1;
               AtodCntrChan[i]     = INDEXCHANNEL_NONE;
               AtodDataRequest    |= PARXCH_REQUEST_GPS;
               UseTimeMethod       = TIME_METHOD_GPS;
               if (GpsModel == GPSMODEL_PCTIME)
                       UseTimeMethod = TIME_METHOD_PCT;
           }
       }
       else if ( AtodXchModel[i] == PARXCH_MODEL_PAR8CH )   /* PAR8CH */
       {
           AtodNumChannels[i] = PAR8CH_ANALOG_CHANNELS +
                                PAR8CH_DIGITAL_CHANNELS;
           if ( UsingGps )
           {
               AtodNumChannels[i] += PAR8CH_GPS_CHANNELS;
               AtodMarkChan[i]     = AtodNumChannels[i]-1;
               AtodNumChannels[i] += PAR8CH_COUNTER_CHANNELS;
               AtodCntrChan[i]     = AtodNumChannels[i]-1;
               AtodDataRequest    |= PARXCH_REQUEST_GPS;
               AtodDataRequest    |= PARXCH_REQUEST_COUNTER;
               UseTimeMethod    = TIME_METHOD_OBC;
               if (GpsModel == GPSMODEL_PCTIME)
                       UseTimeMethod = TIME_METHOD_PCT;
           }
       }
       else
       {
           logit( "e", "srpar2ew: Unknown PARxCH requested.  Exiting.\n" );
           sr_terminate();
       }


       AtodNumChannelsTotal += AtodNumChannels[i];

    } /* end for i < AtodNumBoards */



    if (UseTimeMethod == TIME_METHOD_PCT)
            TsValid = TS_VALID_MOST;    /* Time from pc is sufficient     */
    else
            TsValid = TS_VALID_ALL;     /* Serial input required for time */


    if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_init_model\n" );

    return;
}

/******************************************************************************
 *  Function: sr_atod_open                                                    *
 *  Purpose:  Open the PARxCH DAQ and optional PARGPS timing module device    *
 *            drivers.  Once both drivers are opened, they must be 'attached' *
 *            so they can communication with each other.  The hardware is     *
 *            initialized when the drivers are opened.   This routine exits   *
 *            if any of these steps fail.                                     *
 ******************************************************************************/
void sr_atod_open( void )
{
   int i, parxchError, pargpsError;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_open\n" );


  /* Start with invalid driver handles
   ***********************************/
   ParGpsHandle  = BAD_DEVHANDLE;
   for ( i = 0 ; i < AtodNumBoards ; i++ )
         ParXchHandle[i]  = BAD_DEVHANDLE;


  /* Open the PARxCH drivers and initialize the PARxCH DAQ boards
   **************************************************************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
       if ( Debug >= 5 )
       {
           logit( "", "Calling ParXchOpen with:\n" );
           logit( "", "     Driver   %s\n",  AtodDriverName[i] );
           logit( "", "     XchModel %d\n",  AtodXchModel[i] );
           logit( "", "     PortMode %d\n",  AtodPortMode[i] );
           logit( "", "     Sps      %lf\n", AtodRequestedSps );
       }
       ParXchHandle[i] = ParXchOpen( AtodDriverName[i], AtodXchModel[i],
                                     AtodPortMode[i], AtodRequestedSps,
                                     &AtodSps, &parxchError );

       if ( Debug >= 5 )  logit( "", "Result is Sps %lf, error %d = %s\n",
                               AtodSps, parxchError, PARXCH_ERROR_MSG[parxchError] );

       if ( ParXchHandle[i] == BAD_DEVHANDLE )
       {
           sprintf( Msg, "Failed to open PARxCH driver %s (Err=%s)",
                    AtodDriverName[i], PARXCH_ERROR_MSG[parxchError] );
           sr_error( EW_ERROR_BASE_PARXCH+parxchError, Msg );
           sr_terminate( );
       }
       if ( Debug >= 0 )
          logit( "", "DAQ %d: Opened %s driver %s port mode %s with SPS = %lf\n",
                 i, AtodXchModelName[i], AtodDriverName[i], AtodPortModeName[i], AtodSps );

   } /* end for i < AtodNumBoards */



   /* If using the PARGPS, then open and initialize it too
    ******************************************************/
    if ( UsingGps )
    {
        if ( Debug >= 5 )  logit( "", "Calling PARGPS open with Driver %s, serial COM%d\n",
                                 GpsDriverName, GpsSerialPort );

        ParGpsHandle = ParGpsFullOpen( GpsDriverName,
                                       GpsModel,
                                       GpsSerialPort,
                                       NULL,
                                       &pargpsError );
        if ( ParGpsHandle == BAD_DEVHANDLE )
        {
            sprintf( Msg, "Failed to open PARGPS driver %s (Err=%s)",
                     GpsDriverName, PARGPS_ERROR_MSG[pargpsError] );
            sr_error( EW_ERROR_BASE_PARGPS+pargpsError, Msg );
            if ( pargpsError == PARGPS_ERROR_SERIAL_PORT_NOT_OPEN )
                logit( "e", "                Is serial port COM%d already in use?",
                       GpsSerialPort );
            sr_terminate( );
        }
        if ( Debug >= 0 )
           logit( "", "Opened PARGPS driver %s with serial port = COM%d, GpsModel %s\n",
                  GpsDriverName, GpsSerialPort, GpsModelName );

        if ( Debug >= 5 )
           logit( "", "Successful PARGPS open (Xch = 0x%08X, Gps = 0x%08X)\n",
                  ParXchHandle[0], ParGpsHandle );



       /* Attach 1st PARxCH and PARGPS drivers together
        ***********************************************/
        if ( !ParXchAttachGps( ParXchHandle[0], ParGpsHandle, &parxchError ) )
        {
            sprintf( Msg, "%s attach to PARGPS failed (Err=%s)",
                     AtodXchModelName[0], PARXCH_ERROR_MSG[parxchError] );
            sr_error( EW_ERROR_BASE_PARXCH+parxchError, Msg );
            sr_terminate( );
        }

       /* Require the PARxCH driver to have an interrupt line
        *****************************************************/
        if ( ParXchInterruptGetNumber( ParXchHandle[0] ) == 0 )
        {
            sprintf( Msg, "No IRQ assigned.  Please reinstall PARxCH driver with an IRQ value." );
            sr_error( EW_ERROR_BASE_PARXCH+PARXCH_ERROR_NO_INTERRUPT, Msg );
            sr_terminate( );
        }

    } /* end if UsingGps */



    if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_open\n" );
    return;
}

/******************************************************************************
 *  Function: sr_atod_correct_sps                                             *
 *  Purpose:  This corrects the requested sampling rate (sps) to the closest  *
 *            rate actually achievable on the PARxCH hardware.                *
 ******************************************************************************/
void sr_atod_correct_sps( double *sps )
{
    int    gl, tb1, tb2, dec1, dec2, extdec1, extdec2, dummy;
    double sps0, sps1, sps2, diff1, diff2;

    if ( Debug >= 5 )
        logit( "", "Starting sr_atod_correct_sps with requested sps = %lf\n", *sps );


   /* Compute decimation from requested sps
    ***************************************/
    sps0 = *sps;
    gl   = 0;                                              /* log(gain of 1) */
    ParXchSpsGainToTde( sps0, gl, &tb1, &dec1, &extdec1 );


   /* Set up next decimation too
    ****************************/
    tb2     = tb1;                                         /* turbo mode */
    dec2    = dec1 + 1;
    extdec2 = extdec1;


   /* Use both decimations to compute actual sps
    ********************************************/
    ParXchTdeToSpsGain( tb1, dec1, extdec1, &sps1, &dummy );
    ParXchTdeToSpsGain( tb2, dec2, extdec2, &sps2, &dummy );


   /* Compare both actual sps values with requested sps
    ***************************************************/
    diff1 = sps1 - sps0;
    diff2 = sps0 - sps2;


   /* Pick the closer one
    *********************/
    if (diff1 < diff2)
        *sps = sps1;
    else
        *sps = sps2;


    if ( Debug >= 5 )
        logit( "", "Leaving  sr_atod_correct_sps with corrected sps = %lf\n", *sps );

    return;
}

/******************************************************************************
 *  Function: sr_ts_init                                                      *
 *  Purpose:  Initialize the good and raw timestamp structures.               *
 ******************************************************************************/
void sr_ts_init( void )
{
    int i, j;

    if ( Debug >= 6 )  logit( "", "Starting sr_ts_init\n" );


   /* Clear the good timestamp structures
    *************************************/
    for ( i = 0 ; i < MAX_BOARD ; i++ )
       for ( j = 0 ; j < MAX_TS ; j++ )
       {
           ParGpsTsClear( &TimeStamp[i][j] );
       }

   /* Clear the raw timestamps
    **************************/
    for ( i = 0 ; i < MAX_TSR ; i++ )
    {
        ParGpsTsClear( &TsReal[i] );
    }

    ParGpsTsClear( &TsFix );


    if ( Debug >= 6 )  logit( "", "Leaving  sr_ts_init\n" );
    return;
}

/******************************************************************************
 *  Function: sr_filter_init                                                  *
 *  Purpose:  This initializes the scale and coefficient values used in       *
 *            the two term filter formulas described in sr_filter and used    *
 *            to update the BestCountsBetweenPps and BestCountsBetweenDready  *
 *            values.                                                         *
 *                                                                            *
 *            For this application, choose the coefficient C to be .95 and    *
 *            assume an N of 150 is a long enough tail.  Then C**N => .001    *
 *            which means that the 150th previous sample is contributing only *
 *            a very small amount to the average.  This also means that the   *
 *            amount that the current sample contributes is about 1/20th      *
 *            of the contribution from all previous samples so one point      *
 *            wont't change the average too quickly.  This number comes from  *
 *            computing C/1-C which is 19 for a C of .95.                     *
 ******************************************************************************/
void sr_filter_init( void )
{
    if ( Debug >= 6 )  logit( "", "Starting sr_filter_init\n" );

   /* Set filter coefficients
    *************************/
    CbpCoeff = 0.95;
    CbdCoeff = 0.95;

   /* Set scale to 1-C to normalize
    *******************************/
    CbpScale = 1.0L - CbpCoeff;
    CbdScale = 1.0L - CbdCoeff;

    if ( Debug >= 5 )
    {
        logit( "", "Pps    filter scale %lf, coeff %lf\n", CbpScale, CbpCoeff );
        logit( "", "Dready filter scale %lf, coeff %lf\n", CbdScale, CbdCoeff );
    }

    if ( Debug >= 6 )  logit( "", "Leaving  sr_filter_init\n" );
    return;
}

/******************************************************************************
 *  Function: sr_allocate_arrays                                              *
 *  Purpose:  Allocate space for the input acquired data and output tracebuf  *
 *            arrays.  Also initialize some common values in the output       *
 *            header that are shared for all channels.                        *
 ******************************************************************************/
void sr_allocate_arrays( void )
{
    unsigned int bufferSize;

    if ( Debug >= 6 )  logit( "", "Starting sr_allocate_arrays\n" );


   /* Allocate space for the input acquired data.  We will alternately
    * fill two buffers.  By allowing a second buffer of data to be read
    * before the first is output, we ensure there is enough information
    * to accurately determine the end time of the first buffer before.
    ********************************************************************/
    bufferSize  = AtodNumChannelsTotal * AtodNumSamples; /* size in samples */
    AtodBufSize = bufferSize * sizeof(long);             /* size in bytes   */

    AtodBuffer1 = (long *) calloc( bufferSize, sizeof(long) );
    if ( AtodBuffer1 == NULL )
    {
        logit( "e", "srpar2ew: Cannot allocate DAQ buffer\n" );
        sr_terminate( );
    }

    AtodBuffer2 = (long *) calloc( bufferSize, sizeof(long) );
    if ( AtodBuffer2 == NULL )
    {
        logit( "e", "srpar2ew: Cannot allocate DAQ buffer\n" );
        sr_terminate( );
    }

    AtodBufferFill  = AtodBuffer2; /* Initialize with buffer 2 since */
    AtodBufferWrite = AtodBuffer1; /* switch occurs before filling   */

    if ( Debug >= 5 )
        logit( "", "Starting with Fill (2) = 0x%lX and Write (1) = 0x%lX\n",
               AtodBufferFill, AtodBufferWrite );


   /* Allocate space for the output trace buffer including both header + data
    *************************************************************************/
    TraceBufSize = sizeof(TRACE_HEADER) + (AtodNumSamples * sizeof(long));
    if ( Debug >= 0 )  logit( "", "Trace buffer size: %d bytes\n", TraceBufSize );

    if ( TraceBufSize > MAX_TRACEBUF_SIZ )
    {
        logit( "e", "srpar2ew: Tracebuf size %ld exceeds %ld max; reduce sampling rate\n",
               TraceBufSize, MAX_TRACEBUF_SIZ );
        sr_terminate( );
    }

    TraceBuffer = (char *) malloc( TraceBufSize );
    if ( TraceBuffer == NULL )
    {
        logit( "e", "srpar2ew: Cannot allocate the trace buffer\n" );
        sr_terminate( );
    }

   /* Set up pointers to the tracebuf header and data parts
    *******************************************************/
    TraceHead  = (TRACE_HEADER  *) &TraceBuffer[0];
    TraceDat   = (long *) &TraceBuffer[sizeof(TRACE_HEADER)];


   /* Set output values common to all channels
    ******************************************/
    TraceHead->nsamp      = AtodNumSamples;     /* number of samples in message   */
    TraceHead->samprate   = AtodSps;            /* sample rate; nominal           */
    TraceHead->quality[0] = QUALITY_OK;         /* one bit per condition          */
    TraceHead->quality[1] = QUALITY_OK;         /* one bit per condition          */
    TraceHead->pad[0]     = 'S';
    TraceHead->pad[1]     = 'R';
    sr_strncpy( TraceHead->datatype, "i4", 3 ); /* data format code (intel long)  */

    if (OutputMsgType == TypeTraceBuf)          /* old default loc + version info */
    {
       sr_strncpy( &TraceHead->chan[TRACE_POS_LOC], LOC_NULL_STRING, 3 );
       TraceHead->chan[TRACE_POS_VER]   = '1';
       TraceHead->chan[TRACE_POS_VER+1] = '0';
    }
    if (OutputMsgType == TypeTraceBuf2)         /* new tracebuf2 version info     */
    {
       TraceHead->chan[TRACE_POS_VER]   = TRACE2_VERSION0;
       TraceHead->chan[TRACE_POS_VER+1] = TRACE2_VERSION1;
    }

    TraceHead->starttime  = 0.0L;
    TraceHead->endtime    = 0.0L;


    if ( Debug >= 6 )  logit( "", "Leaving  sr_allocate_arrays\n" );
    return;
}



/******************************************************************************
 *  Function: sr_atod_start                                                   *
 *  Purpose:  Start the PARxCH DAQ and optional PARGPS timing module          *
 *            acquiring data.                                                 *
 ******************************************************************************/
void sr_atod_start( int waitSec, int initTimes )
{
   int    i;
   double startTime, endTime, restartDelay;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_start\n" );


  /* Initialize OnBoardCount (OBC) and gap variables
   *************************************************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
      ObcMarkCount[i]    = 0;
      LastObc[i]         = INDEXCHANNEL_NONE;
      LastMark[i]        = INDEXCHANNEL_NONE;
      LastPtObc[i]       = LAST_PT_INVALID;
      LastPtMark[i]      = LAST_PT_INVALID;
      GapTrailer[i]      = 0;
   }


  /* Initialize PpsEvent variables
   *******************************/
   LastDataPpsEvent   = 0;
   LastPpsPpsEvent    = 0;
   LastSerialPpsEvent = 0;



  /* Initialize AtodBuffer variables
   *********************************/
   AtodBufferFill  = AtodBuffer2; /* Initialize with buffer 2 since */
   AtodBufferWrite = AtodBuffer1; /* switch occurs before filling   */

   if ( Debug >= 5 )
      logit( "", "Starting with Fill (2) = 0x%lX and Write (1) = 0x%lX\n",
             AtodBufferFill, AtodBufferWrite );



  /* Initialize other Atod variables
   *********************************/
   AtodLedValue       = 0;
   AtodBufferNum      = 0L;
   AtodNumPps         = 0L;
   AtodNumSerial      = 0L;



  /* Initialize time stamps
   ************************/
   sr_ts_init();


  /* On first startup, we wait a bit so the other EW modules have a chance to
   * finish their setup before we start.  This helps ensure that the parallel
   * port outs done by our two start calls will really occur in quick succession
   * to each other and not be delayed by various interrupts or other activities
   * clogging the SuperIo or bridge chip.
   *****************************************************************************/
   if ( waitSec > 0 )
   {
      logit( "et", "srpar2ew: Starting acquisition after a %d second wait\n", waitSec );
      sleep_ew( waitSec * 1000 );
   }
   else
      logit( "et", "srpar2ew: Starting acquisition\n" );


  /* Start the optional PARGPS
   ***************************/
   if ( UsingGps )
      ParGpsStart( ParGpsHandle );



  /* Start all PARxCH DAQs
   ***********************/
   if ( UsingGps )
      sr_atod_synchronize();

   for ( i = 0 ; i < AtodNumBoards ; i++ )
      ParXchStart( ParXchHandle[i] );


  /* Get approximate start time from the PC
   * or (on restart) use previous values
   ****************************************/
   hrtime_ew( &startTime );             /* start of first tracebuf */
   endTime = startTime - (1 / AtodSps); /* end of "prev" tracebuf  */

   if  (initTimes == TIME_INIT_YES )
   {
      for ( i = 0 ; i < AtodNumBoards ; i++ )
      {
         AtodStartTime[i]   = startTime;
         AtodEndTimeLast[i] = endTime;
      }
   }
   else
   {
      if ( startTime > RestartBegin )
         restartDelay = startTime - RestartBegin;
      else
         restartDelay = 0.0L;

      if ( Debug >= 5 )
         logit( "", "RestartBegin %lf, true start %lf, restartDelay %lf\n",
                RestartBegin, startTime, restartDelay );

      for ( i = 0 ; i < AtodNumBoards ; i++ )
      {
         AtodStartTime[i]   += restartDelay;
         AtodEndTimeLast[i] += restartDelay;
      }
   }

   AtodCurrentPtFill     = 0L;
   AtodCurrentPtWrite    = 0L;

   if ( Debug >= 5 )
      logit( "", "DAQ last end time %lf, this start time %lf \n",
             AtodEndTimeLast[0], AtodStartTime[0] );


  /* We'll need to re-initialize some values later
   ***********************************************/
   FirstTimeFilter = 1;
   FirstTimeSerial = 1;
   FirstTimePctime = 1;
   FirstTimeData   = 1;

   if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_start\n" );
   return;
}

/******************************************************************************
 *  Function: sr_atod_synchronize                                             *
 *  Purpose:  Synchronize starting the PARxCH DAQs to ensure they all start   *
 *            their GPS mark count from the same PPS signal.  We do this by   *
 *            polling the GPS toggle signal until we see a change.  This      *
 *            means a PPS signal has just arrived so we have the maximum      *
 *            amount of time in which to get all the DAQs started before the  *
 *            next PPS arrives.                                               *
 ******************************************************************************/
void sr_atod_synchronize( void )
{
   int    digioValue, lastToggleValue, thisToggleValue, toggleWait, sampleWait;
   double toggleTime, toggleLimit;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_synchronize\n" );


  /* Initialize toggle value and time limit
   ****************************************/
   ParXchUserIoRd( ParXchHandle[0], &digioValue );
   lastToggleValue = digioValue & TOGGLE_MASK;

   toggleWait = 10; /* seconds */
   hrtime_ew( &toggleTime );
   toggleLimit = toggleTime + toggleWait;



   if ( Debug >= 5 )
      logit( "", "Begin  synchronize loop at %lf, lastToggle %d (digioValue %d)\n",
             toggleTime, lastToggleValue, digioValue );

   while( 1 )
   {
     /* Poll DAQ digital IO for Gps toggle signal
      *******************************************/
      ParXchUserIoRd( ParXchHandle[0], &digioValue );
      thisToggleValue = digioValue & TOGGLE_MASK;

      if ( thisToggleValue != lastToggleValue ) /* Change found, go start DAQs */
         break;

      else                                      /* No change, update + repeat */
         lastToggleValue = thisToggleValue;


     /* Prevent infinite loop, when GPS toggle signal is not present
      **************************************************************/
      hrtime_ew( &toggleTime );
      if ( toggleTime > toggleLimit )
      {
         logit( "t", "Failed to find Gps toggle signal within %d seconds.\nDAQ start may not be synchronized.\n",
                toggleWait );
         break;

      } /* end if toggleTime > toggleLimit */

      sleep_ew( 50 );   /* Wait 50 milliseconds before trying again */

   } /* end while ( 1 ) */

   hrtime_ew( &toggleTime );
   if ( Debug >= 5 )
      logit( "", "Finish synchronize loop at %lf, lastToggle %d, thisToggle %d (digioValue %d)\n",
             toggleTime, lastToggleValue, thisToggleValue, digioValue );


  /* Add an additional wait so we don't start the DAQs between a PPS
   * and the next sample as that would confuse the code by producing an
   * OBC (started by the PPS) without a matching Gps Mark bit (since
   * interrupts were not enabled until after that PPS was already done)
   ********************************************************************/
   sampleWait = 200;            // WCT - 200 ms wait is good down to 10sps
   sleep_ew( sampleWait );



   if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_synchronize\n" );
   return;
}

/******************************************************************************
 *  Function: sr_atod_getdata                                                 *
 *  Purpose:  Read AtodNumSamples points for each of AtodNumChannels channels *
 *            into the AtodBuffer.                                            *
 ******************************************************************************/
int sr_atod_getdata( void )
{
    static int nbehind = 0;

    int           i, istr, npass, nextpt[MAX_BOARD], firstTry, isSummary;
    int           parxchError, pargpsError, analogError;
    unsigned int  nremain[MAX_BOARD], nread[MAX_BOARD], nremainTotal, serialDelay;
    unsigned int  dataPpsEvent, ppsPpsEvent, serialPpsEvent, minPpsEvent, maxPpsEvent;
    long          nDataEvents, nPpsEvents, nSerialEvents;
    unsigned long diffDP;

    if ( Debug >= 6 )  logit( "", "Starting sr_atod_getdata\n" );

   /* Initialize some variables
    ***************************/
    analogError  = 0;
    nextpt[0]    = 0;
    nremain[0]   = (unsigned int)(AtodNumSamples * AtodNumChannels[0]);
    nremainTotal = nremain[0];

    for ( i = 1 ; i < AtodNumBoards ; i++ )
    {
       nextpt[i]     = nremain[i-1];
       nremain[i]    = (unsigned int)(AtodNumSamples * AtodNumChannels[i]);
       nremainTotal += nremain[i];
       if ( Debug >= 5 )  logit( "", "Board %d starts at %d and has %u points\n", i, nextpt[i], nremain[i] );
    }


    if (SummaryReportInterval > 0 && SummaryCount >= SummaryReportInterval-1)
       isSummary = 1;
    else
       isSummary = 0;
    if ( isSummary || Debug >= 5 )
        logit( "t", "srpar2ew: summary info follows ...\n" );


   /* Select which of two alternating buffers is to be used
    *******************************************************/
    if ( AtodBufferFill == AtodBuffer1 )
    {
        AtodBufferFill  = AtodBuffer2;
        AtodBufferWrite = AtodBuffer1;
    }
    else /* ( AtodBufferFill == AtodBuffer2 ) */
    {
        AtodBufferFill  = AtodBuffer1;
        AtodBufferWrite = AtodBuffer2;
    }


    if ( Debug >= 5 )  logit( "", "Now Filling = 0x%lX\n", AtodBufferFill );


   /* Read Analog data (plus digital, GPS, and counter if requested)
    ****************************************************************/
    npass = 0;

    if ( isSummary || Debug >= 5 )
        logit( "", "Reading Analog Info:\n" );

    while ( nremainTotal > 0 )
    {

       for ( i = 0 ; i < AtodNumBoards ; i++ )
       {

          /* Read DAQ data
           ***************/
           if ( nremain[i] > 0 )
           {
               nread[i] = ParXchFullReadData( ParXchHandle[i], &AtodBufferFill[nextpt[i]],
                                              nremain[i], AtodDataRequest, &parxchError );
               nextpt[i]    += nread[i];
               nremain[i]   -= nread[i];
               nremainTotal -= nread[i];
           }
           else
               nread[i] = 0;


          /* Check for overflow and other errors
           *************************************/
           if ( parxchError == PARXCH_ERROR_OVERFLOW )
           {
               sprintf( Msg, "ReadData error %s", PARXCH_ERROR_MSG[PARXCH_ERROR_OVERFLOW] );
               sr_error( EW_ERROR_BASE_PARXCH+PARXCH_ERROR_OVERFLOW, Msg );
               sr_atod_restart_recovery( );
               return( 0 );
           }
           if ( parxchError != PARXCH_ERROR_NONE )
           {
               sprintf( Msg, "ReadData error %s", PARXCH_ERROR_MSG[parxchError] );
               sr_error( EW_ERROR_BASE_PARXCH+parxchError, Msg );
               return( 0 );
           }


           if ( isSummary || Debug >= 5 )
           {
               logit( "", "  DAQ %d Read %4u pts, nextpt = %4d, nremain = %4u, nremainTotal = %4u\n",
                            i, nread[i], nextpt[i], nremain[i], nremainTotal );
           }

       } /* end for i < AtodNumBoards */

       npass++;


       /* If more points wanted, give them time to arrive
        *************************************************/
        if ( nremainTotal > 0 )
            sleep_ew( SleepMs );   /* Wait SleepMs milliseconds (eg 200) */


    } /* end while nremainTotal > 0 */



   /* If we read all the requested data in right away,
    * it means we're starting to fall behind.  If this
    * condition lasts too long, give a warning.
    **************************************************/
    if ( npass == 1 )
        nbehind++;
    else
        nbehind = 0;

    if ( nbehind > 0 )
        logit( "et", "srpar2ew: Earthworm running slow but still OK ...\n     (data built up in FIFO %d times)\n", nbehind );

    if ( nbehind == BEHIND_LIMIT )
    {
        sprintf( Msg, "Warning - Earthworm is not keeping up with acquisition, overflow could result" );
        sr_error( EW_ERROR_BASE_PARXCH+PARXCH_ERROR_OVERFLOW, Msg );
    }




    if ( UsingGps )
    {

        if ( Debug >= 5 )
           sr_log_analog_data();


       /* Look in analog data for GPS marks and OBC counts
        **************************************************/
        dataPpsEvent = sr_ts_process_analog( &analogError );

        if ( analogError )
           return( 0 );

        minPpsEvent      = LastDataPpsEvent+1;
        maxPpsEvent      = dataPpsEvent;
        nDataEvents      = dataPpsEvent - LastDataPpsEvent;
        LastDataPpsEvent = dataPpsEvent;

        if ( isSummary || Debug >= 4 )
            logit( "", "  Analog data points %ld to %ld contain PPS Event %d\n",
                   AtodCurrentPtFill, (AtodCurrentPtFill+AtodNumSamples-1), dataPpsEvent );


        if ( Debug >= 1 )
        {
            if ( nDataEvents == 0 )
                logit("et", "srpar2ew: No GPS marks found!\n");

            if ( dataPpsEvent < minPpsEvent )
                logit( "t", "srpar2ew: Missed PPS Event in Data   (found %ld, expected %ld)\n",
                      dataPpsEvent, minPpsEvent );

            else if ( dataPpsEvent > minPpsEvent )
                logit( "t", "srpar2ew: Extra  PPS Event in Data   (found %ld, expected %ld - this is %s)\n",
                       dataPpsEvent, minPpsEvent, (dataPpsEvent == minPpsEvent+1)?"Normal":"Unexpected" );
        }




       /* Read and process GPS PPS data
        *******************************/
        if ( isSummary || Debug >= 5 )
            logit( "", "Reading GPS PPS Info:\n" );
        firstTry   = 1;
        nPpsEvents = nDataEvents;     /* Only read PPS data for events we */
        while( nPpsEvents > 0 )       /* already have analog data for.    */
        {
            AtodNumPps = ParGpsReadPpsData( ParGpsHandle, GpsPpsBuffer,
                                            1, &pargpsError );
            if ( pargpsError != PARGPS_ERROR_NONE )
            {
                sprintf( Msg, "ReadPpsData error %s", PARGPS_ERROR_MSG[pargpsError] );
                sr_error( EW_ERROR_BASE_PARGPS+pargpsError, Msg );
            }
            if ( isSummary || Debug >= 4 )
            {
               logit( "", "  PPS Event # %d with\n", GpsPpsBuffer[0].PpsEventNum );
               diffDP = (unsigned long)(GpsPpsBuffer[0].CountAtDready.QuadPart -
                                        GpsPpsBuffer[0].CountAtPps.QuadPart);
               logit( "", "  PC Counter1 at PPS 0x%08lX %08lX  at Dready 0x%08lX %08lX  (D-P=%lu)\n",
                     GpsPpsBuffer[0].CountAtPps.u.HighPart,
                     GpsPpsBuffer[0].CountAtPps.u.LowPart,
                     GpsPpsBuffer[0].CountAtDready.u.HighPart,
                     GpsPpsBuffer[0].CountAtDready.u.LowPart,
                     diffDP );
               diffDP = (unsigned long)(GpsPpsBuffer[0].PctimeAtDready.QuadPart -
                                        GpsPpsBuffer[0].PctimeAtPps.QuadPart);
               logit( "", "  PC Counter2 at PPS 0x%08lX %08lX  at Dready 0x%08lX %08lX  (D-P=%lu)\n",
                     GpsPpsBuffer[0].PctimeAtPps.u.HighPart,
                     GpsPpsBuffer[0].PctimeAtPps.u.LowPart,
                     GpsPpsBuffer[0].PctimeAtDready.u.HighPart,
                     GpsPpsBuffer[0].PctimeAtDready.u.LowPart,
                     diffDP );
            }

           /* If no PPS data read, wait and try again once
            **********************************************/
            if ( AtodNumPps <= 0 )
            {
                if ( firstTry )
                {
                    firstTry = 0;
                    sleep_ew( 10 );
                    continue;
                }
                else
                {
                    logit( "et", "srpar2ew: No PPS data was read\n" );
                    break;
                }
            }


           /* Get PpsEvent number for PPS data and error check
            **************************************************/
            ppsPpsEvent = GpsPpsBuffer[0].PpsEventNum;

            if ( ppsPpsEvent < minPpsEvent )        /* PpsEvent too old, discard */
            {
                logit("et", "srpar2ew: Discarding old PPS PpsEvent %d, current min is %d\n",
                      ppsPpsEvent, minPpsEvent );
                continue;
            }
            else if ( ppsPpsEvent > maxPpsEvent )   /* PpsEvent too new */
            {
                logit( "et", "srpar2ew: PPS PpsEvent %d read too soon, current max is %d\n",
                      ppsPpsEvent, maxPpsEvent );
                  nPpsEvents--;
                  // WCT: Consider making synthetic tempstamp for missed PpsEvent here
            }

            if ( ppsPpsEvent != LastPpsPpsEvent+1 )
                logit( "et", "srpar2ew: missed PPS Event in PPS,    found %ld, expected %ld\n",
                       ppsPpsEvent, LastDataPpsEvent+1);


           /* PpsEvent is good, extract info and save in timestamp
            ******************************************************/
            sr_ts_process_pps( );

            nPpsEvents--;
            LastPpsPpsEvent = ppsPpsEvent;


           /* Exit loop if PPS data caught up to analog data
            ************************************************/
            if ( ppsPpsEvent >= dataPpsEvent )
                break;

        } /* end while 1 to read PPS data */




       /* Read and process GPS serial data
        **********************************/
        if ( isSummary || Debug >= 5 )
            logit( "", "Reading GPS Serial Info:\n" );
        firstTry      = 1;
        serialDelay   = 2;
        nSerialEvents = nDataEvents;           /* Only read serial data if we have */
        while( GpsModel != GPSMODEL_PCTIME &&  /* serial data expected and         */
               dataPpsEvent > serialDelay &&   /* matching analog data already     */
               nSerialEvents > 0 )
        {
            AtodNumSerial = ParGpsReadSerialData( ParGpsHandle, GpsSerialBuffer,
                                                  1, &pargpsError );
            if ( pargpsError != PARGPS_ERROR_NONE )
            {
                sprintf( Msg, "ReadSerialData error %s", PARGPS_ERROR_MSG[pargpsError] );
                sr_error( EW_ERROR_BASE_PARGPS+pargpsError, Msg );
            }
            if ( isSummary || Debug >= 4 )
            {
               logit( "", "  Serial Event # %d with %d NMEA strings\n",
                     GpsSerialBuffer[0].PpsEventNum, GpsSerialBuffer[0].NmeaCount );
               for ( i = 0 ; i < GpsSerialBuffer[0].NmeaCount ; i++ )
               {
                  istr = i * MAX_NMEA_SIZE;
                  logit( "","  %d: %s", i, &GpsSerialBuffer[0].NmeaMsg[istr] );
               }
            }

           /* If no serial data read, wait and try again once
            *************************************************/
            if ( AtodNumSerial <= 0 )
            {
                if ( firstTry )
                {
                    firstTry = 0;
                    sleep_ew( 10 );
                    continue;
                }
                else
                {
                    logit( "et", "srpar2ew: No serial data was read\n" );
                    break;
                }
            }


           /* Get PpsEvent number for serial data and error check
            *****************************************************/
            serialPpsEvent = GpsSerialBuffer[0].PpsEventNum;

            if ( serialPpsEvent+serialDelay < minPpsEvent )      /* PpsEvent too old, discard */
            {
                logit( "et", "srpar2ew: Discarding old serial PpsEvent %d (+delay=%d), current min is %d\n",
                      serialPpsEvent, serialPpsEvent+serialDelay, minPpsEvent );
                continue;
            }
            else if ( serialPpsEvent+serialDelay > maxPpsEvent ) /* PpsEvent too new */
            {
                logit( "et", "srpar2ew: Serial PpsEvent %d (+delay=%d) read too soon, current max is %d\n",
                      serialPpsEvent, serialPpsEvent+serialDelay, maxPpsEvent );
                nSerialEvents--;
                // WCT: Consider making synthetic tempstamp for missed PpsEvent here
            }

            if (serialPpsEvent != LastSerialPpsEvent+1)
                logit( "et", "srpar2ew: missed PPS Event in Serial, found %ld, expected %ld (%ld->%ld)\n",
                      serialPpsEvent, LastSerialPpsEvent+1,
                      serialPpsEvent, serialPpsEvent+serialDelay);



           /* PpsEvent is good, extract info and save in timestamp
            ******************************************************/
            sr_ts_process_serial( );

            nSerialEvents--;
            LastSerialPpsEvent = serialPpsEvent;


           /* Exit loop if serial data caught up to analog data
            * (note, we expect serial data to be delayed by 2 sec)
            *****************************************************/
            if ( serialPpsEvent+serialDelay >= dataPpsEvent )
            {
                if ( Debug >= 5 )  logit( "", "Serial data read caught up\n" );
                break;
            }

        } /* end while 1 to read serial data */


    } /* end if UsingGps */


   /* Update current point and buffer count
    ***************************************/
    if ( Debug >= 5 )  logit( "", "Completed buffer %lu\n", AtodBufferNum );
    AtodBufferNum++;
    AtodCurrentPtFill += AtodNumSamples;


    if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_getdata\n" );
    return( nextpt[0] );
}



/******************************************************************************
 *  Function: sr_ts_process_analog                                            *
 *  Purpose:  To extract time stamp information from the analog data buffer.  *
 *            This includes the sample index for the mark point and, for the  *
 *            PAR8CH, the value of the on board counter (OBC).                *
 *                                                                            *
 *            Return 0 or last PPS event processed.                           *
 ******************************************************************************/
unsigned int sr_ts_process_analog( int *analogError )
{
    int          i, istart, nValid;
    unsigned int currEvent;
    long        *dataValues;

    if ( Debug >= 6 )  logit( "", "Starting sr_ts_process_analog\n" );


    currEvent = 0;
    istart    = 0;

    for ( i = 0 ; i < AtodNumBoards ; i++ )
    {

      /* Nothing to check if neither channel exists
       ********************************************/
       if ( AtodMarkChan[i] == INDEXCHANNEL_NONE && AtodCntrChan[i] == INDEXCHANNEL_NONE )
       {
          if ( Debug >= 5 )
             logit( "", "No mark or counter channel to check\n" );
          continue;
       }

       if ( AtodMarkChan[i] == INDEXCHANNEL_NONE && AtodCntrChan[i] != INDEXCHANNEL_NONE )
       {
           logit( "e", "srpar2ew: Error GPS channel MUST exist if OBC does\n" );
           continue;
       }



      /* Process the analog data for timestamp information
       ***************************************************/
       if ( Debug >= 5 )  logit( "", "DAQ #%d, now Processing = 0x%lX\n", i, AtodBufferFill );

       dataValues = &AtodBufferFill[istart];


       if ( i == 0 ) /* Processing for first DAQ board is special and done by ParGps library */
       {

          ParGpsTsProcessAnalog( dataValues, AtodNumSamples, AtodNumChannels[i],
                                 AtodCurrentPtFill, AtodMarkChan[i], AtodCntrChan[i],
                                 (long)BestObcCountsPerSample, TsValid,
                                 &currEvent, &nValid );

          if ( Debug >= 6 )  logit( "", "AtodCurrentPtFill %ld, currEvent %u, nValid %d\n",
                                   AtodCurrentPtFill, currEvent, nValid );


         /* If any new timestamps are valid, update
          * the good ones we'll use to compute time
          *****************************************/
          if ( nValid > 0 )
              sr_ts_update( i );


         /* The first time through, we check to make sure there is no OBC
          * value without a matching Gps Mark in the first data point.
          ***************************************************************/
          if ( FirstTimeData )
          {
             sr_analog_startup_checks( i, dataValues );
             FirstTimeData = 0;

          } /* end if FirstTimeData */

       } /* end if i == 0 (primary DAQ board) */


       else /* Processing for all remaining DAQ boards */
       {
          if ( Debug >= 5 )
          {
              logit( "", "Now ProcessingN at AtodBufferFill 0x%lX + istart %d = dataValues 0x%lX\n",
                     AtodBufferFill, istart, dataValues );
              logit( "", "AtodCurrentPtFill %ld, AtodMarkChanN %ld, AtodCntrChanN %ld\n",
                     AtodCurrentPtFill, AtodMarkChan[i], AtodCntrChan[i] );
          }

          sr_analog_process_secondary( i, dataValues );
       }


      /* Common processing for all DAQ
       *******************************/
       if ( !sr_analog_check_spacing( i, dataValues ) ) /* */
       {
          if ( analogError )  *analogError = 1;
          if ( Debug >= 6 )  logit( "", "Leaving  sr_ts_process_analog with check spacing error\n" );
          return( 0 );
       }

       istart += AtodNumChannels[i] * AtodNumSamples;

    } /* end if for i < AtodNumBoards */



    if ( analogError )  *analogError = 0;

    if ( Debug >= 6 )  logit( "", "Leaving  sr_ts_process_analog\n" );

    return( currEvent );
}

/******************************************************************************
 *  Function: sr_ts_process_pps                                               *
 *  Purpose:  To extract time stamp information from the PPS data buffer.     *
 *            This includes the 64 bit PC counter value at PPS (CountAtPps)   *
 *            and at the next data ready (CountAtDready).                     *
 *                                                                            *
 *            Return 0 or last PPS event processed.                           *
 ******************************************************************************/
unsigned int sr_ts_process_pps( void )
{
    int          nValid;
    unsigned int currEvent;

    if ( Debug >= 6 )  logit( "", "Starting sr_ts_process_pps\n" );


   /* Process the pps data for timestamp information
    ************************************************/
    currEvent = 0;
    ParGpsTsProcessPps( GpsPpsBuffer, 1, BestCountsPerSecond,
                        GpsModel, TsValid, &currEvent, &nValid );

    if ( Debug >= 6 )  logit( "", "Pps currEvent %u, nValid %d\n",
                             currEvent, nValid );


   /* If any new timestamps are valid, update
    * the good ones we'll use to compute time
    *****************************************/
    if ( nValid > 0 )
        sr_ts_update( 0 );


    if ( Debug >= 6 )  logit( "", "Leaving  sr_ts_process_pps\n" );
    return( currEvent );
}

/******************************************************************************
 *  Function: sr_ts_process_serial                                            *
 *  Purpose:  To extract time stamp information from the serial data buffer.  *
 *            This includes the time in secons since 1970 (SecSince1970), the *
 *            number of satellites in view (NumSat), and the source of the    *
 *            year-month-day (YmdSource) and hour-minute-second (HmsSource)   *
 *            component of the time.                                          *
 *                                                                            *
 *            Because of the way it is acquired, the GPS serial data tends to *
 *            be unavailable until about 2 seconds after the corresponding    *
 *            PPS event has occurred.  Since we don't like waiting that long, *
 *            we start with serial data from the 2nd prior PPS event and add  *
 *            two seconds to get the correct time for the current PPS event.  *
 *                                                                            *
 *            Return 0 or last PPS event processed.                           *
 ******************************************************************************/
unsigned int sr_ts_process_serial( void )
{
    int                 i, nValid, serialDelay;
    unsigned int        currEvent;
    static unsigned int PrevPpsEvent;
    static double       PrevSecSince1970;

    if ( Debug >= 6 )  logit( "", "Starting sr_ts_process_serial\n" );


   /* Adjust for 2 second delay
    ***************************/
    serialDelay = 2;


   /* First time init
    *****************/
    if ( FirstTimeSerial )
    {
        PrevPpsEvent     = INVALID_PPSEVENT;
        PrevSecSince1970 = AtodStartTime[0] + serialDelay;
        FirstTimeSerial  = 0;
        if ( Debug >= 5 ) logit( "", "First time init for serial info AtodStartTime %lf\n",
                                AtodStartTime[0] );
    }


   /* Save previous GPS serial NMEA info for future debugging purposes
    ******************************************************************/
    for ( i = 0 ; i < MAX_PREV_SER-1 ; i++ )
            GpsPrevSerial[i] = GpsPrevSerial[i+1];

    GpsPrevSerial[MAX_PREV_SER-1] = GpsSerialBuffer[0];



   /* Process the pps data for timestamp information
    ************************************************/
    currEvent   = 0;
    ParGpsTsProcessSerial( GpsSerialBuffer, 1, serialDelay, TsValid,
                           &PrevPpsEvent, &PrevSecSince1970,
                           &currEvent, &nValid );

    if ( Debug >= 6 )
       logit( "", "Serial PrevEv %u, PrevSec %lf, currEvent %u, nValid %d\n",
              PrevPpsEvent, PrevSecSince1970, currEvent, nValid );


   /* If any new timestamps are valid, update
    * the good ones we'll use to compute time
    *****************************************/
    if ( nValid > 0 )
        sr_ts_update( 0 );


    if ( Debug >= 6 )  logit( "", "Leaving  sr_ts_process_serial\n" );
    return( currEvent );
}

/******************************************************************************
 *  Function: sr_analog_startup_checks                                        *
 *  Purpose:  To perform a check at startup so we can make a correction if any*
 *            special cases are detected.  Right now, the only check is for   *
 *            the case where the first data point has an OBC value, but there *
 *            is no matching Gps Mark.  If we think we see one, we check the  *
 *            next few samples to be sure the GpsMark was not just delayed.   *
 *            But if there really is no GpsMark, the OBC needs to be ignored  *
 *            by the secondary DAQs when they assign GpsMark counts to their  *
 *            OBCs.  Returns 1 if the first OBC will be ignored, 0 otherwise. *
 *            NOTE: This check only works for PAR8CH boards with GPS since    *
 *            the 1 and 4 channel versions don't have an OBC value.           *
 ******************************************************************************/
int sr_analog_startup_checks( int iboard, long *dataValues )
{
    int   ii, ignore;
    long  j;
    long *dataGpsPtr, *dataObcPtr;

    if ( Debug >= 6 )  logit( "", "Starting sr_analog_startup_checks\n" );


   /* Quick return if we don't have both Mark and OBC values
    ********************************************************/
    if ( AtodMarkChan[iboard] == INDEXCHANNEL_NONE  ||
         AtodCntrChan[iboard] == INDEXCHANNEL_NONE )
            return( 0 );


   /* Check for an OBC in the first point
    *************************************/
    ignore     = 0;
    dataGpsPtr = dataValues + AtodMarkChan[iboard];
    dataObcPtr = dataValues + AtodCntrChan[iboard];

    if ( *dataObcPtr != 0L  &&  *dataGpsPtr == 0L ) /* OBC with no GpsMark */
    {
       if ( Debug >= 2 )
          logit( "", "First time data, OBC 0x%X with no mark at point 0\n",
                 *dataObcPtr );

       ignore = 1;                                  /* ignore this OBC     */
       for ( j = 0 ; j < 20 ; j++ )                 /* check 20 samples    */
       {
          dataGpsPtr++;
          dataObcPtr++;
          if ( *dataGpsPtr != 0L  &&  *dataObcPtr == 0L )
          {
             if ( Debug >= 2 )
                logit( "", "First time data, deleay mark 0x%X found at point %d\n",
                       *dataGpsPtr, j+1 );

             ignore = 0;                            /* OBC ok after all    */
             break;
          }

       } /* end for j < 20 */

    } /* end if *dataObcPtr != 0 */


    if ( Debug >= 5 )
       logit( "", "First time data, OBC ignore = %d\n", ignore );


   /* Update the ObcMark count for the secondary DAQs
    * so they will effectively ignore the first OBC
    *************************************************/
    for ( ii = 1 ; ii < AtodNumBoards ; ii++ )
          ObcMarkCount[ii] -= ignore;


    if ( Debug >= 6 )  logit( "", "Leaving  sr_analog_startup_checks\n" );

    return( ignore );
}

/******************************************************************************
 *  Function: sr_analog_process_secondary                                     *
 *  Purpose:  To process the secondary DAQ boards and to fill in timestamp    *
 *            structures for them.  We can not use ParGpsTsProcessAnalog to   *
 *            do this job since the secondary DAQs do not have associated     *
 *            PPS, serial NMEA, and Gps Mark data like the primary DAQ board  *
 *            does.  When we find an OBC value, we generate a synthetic Mark  *
 *            that we refer to as the ObcMark.  This is supposed to match the *
 *            standard GpsMark on the primary DAQ, but can be different if    *
 *            certain unusual or error conditions occur.  We try to track and *
 *            correct for this condition by restarting.                       *
 *            NOTE: This only works for PAR8CH boards with GPS since the 1    *
 *            and 4 channel versions don't have an OBC value.                 *
 ******************************************************************************/
int sr_analog_process_secondary( int iboard, long *dataValues )
{
    long  j, pt, obc;
    long *dataGpsPtr, *dataObcPtr;

    if ( Debug >= 6 )  logit( "", "Starting sr_analog_process_secondary\n" );


   /* Quick return if we don't have both Mark and OBC values
    ********************************************************/
    if ( AtodMarkChan[iboard] == INDEXCHANNEL_NONE  ||
         AtodCntrChan[iboard] == INDEXCHANNEL_NONE )
            return( 1 );


    dataGpsPtr = dataValues + AtodMarkChan[iboard];
    dataObcPtr = dataValues + AtodCntrChan[iboard];

    for ( j = 0 ; j < AtodNumSamples ; j++ )
    {
        pt = AtodCurrentPtFill + j;

        if ( *dataObcPtr != 0L ) /* Found OBC */
        {
            obc = *dataObcPtr;
            ObcMarkCount[iboard]++;
            if ( Debug >= 5 )
               logit( "", "Found Obc value 0x%lX at point %ld, MarkCount %ld\n",
                      obc, pt, ObcMarkCount[iboard] );

            if ( *dataGpsPtr != 0L )
               logit( "", "WARNING: Overwriting existing Gps Marks for DAQ #%d\n", iboard );

            *dataGpsPtr = ObcMarkCount[iboard]; /* Fill in synthetic mark value */


            /* Prepare timestamp structure */

            ParGpsTsClear( &TsFix );
            TsFix.Valid    = (TS_VALID_ANALOG | TS_VALID_OBC);
            TsFix.PpsEvent = ObcMarkCount[iboard];
            TsFix.Sample   = pt;
            TsFix.CountObc = obc;

            if ( Debug >= 5)
               logit( "",  "DAQ #%d TsFix => Valid 0x%X, PpsEvent 0x%X (%u), Sample %ld, CountObc 0x%X\n",
                   iboard, TsFix.Valid, TsFix.PpsEvent, TsFix.PpsEvent,
                   TsFix.Sample, TsFix.CountObc );

            sr_ts_update( iboard );
        }

        dataGpsPtr += AtodNumChannels[iboard];
        dataObcPtr += AtodNumChannels[iboard];

    } /* end for i < AtodNumSamples */



    if ( Debug >= 6 )  logit( "", "Leaving  sr_analog_process_secondary\n" );

    return( 1 );
}

/******************************************************************************
 *  Function: sr_analog_check_spacing                                         *
 *  Purpose:  To perform various sanity checks on the analog data involving   *
 *            the expected 1 second spacing that should occur between         *
 *            consecutive OBC and Gps Mark values.  When bad conditions are   *
 *            identified, we take corrective action.                          *
 *                                                                            *
 *            Corrective action for the most serious conditions involves      *
 *            shutting down the module.  For moderately serious conditions,   *
 *            we restart the acquisition.  Less serious conditions just get   *
 *            warning messages.                                               *
 *                                                                            *
 *            We expect to see 1 OBC and 1 Mark per buffer, with perhaps a    *
 *            second of each if they are right at the beginning and end of    *
 *            the buffer.  More than that indicates that something very       *
 *            serious is wrong.                                               *
 *                                                                            *
 *            We also expect to see one Mark for every OBC.  The Mark may be  *
 *            delayed by a sample or two.  But if it is missing entirely,     *
 *            this is moderately serious and indicates the PPS interrupt has  *
 *            arrived (generating the OBC), but the Dready interrupt is never *
 *            seen (so no Mark is generated).  This should not occur, but we  *
 *            have seen it happen on one PC.  It appears to be a result of    *
 *            noise on parallel port signal lines that are not properly       *
 *            conditioned by the PC.  It can be improved by adding some       *
 *            capacitors to the PAR8CH.  Please call Symmetric Research if    *
 *            you need more details.                                          *
 *                                                                            *
 *            NOTE: This check only works for PAR8CH boards with GPS since    *
 *            the 1 and 4 channel versions don't have an OBC value.           *
 ******************************************************************************/
int sr_analog_check_spacing( int iboard, long *dataValues )
{
    int   spaceErrorObc, spaceErrorMark, badFound, foundObc, foundMark, nDiff;
    long  j, pt, nSamp, nSampDiff, currPtObc, wantPtObc, currPtMark, wantPtMark;
    long *dataGpsPtr, *dataObcPtr;

    if ( Debug >= 6 )  logit( "", "Starting sr_analog_check_spacing\n" );


   /* Quick return if we don't have both Mark and OBC values
    ********************************************************/
    if ( AtodMarkChan[iboard] == INDEXCHANNEL_NONE  ||
         AtodCntrChan[iboard] == INDEXCHANNEL_NONE )
            return( 1 );


   /* Initialize variables
    **********************/

    spaceErrorObc    = 0;
    spaceErrorMark   = 0;
    badFound  = 0;

    foundObc  = 0;                      /* Number OBC found, expect 1 or 2       */
    foundMark = 0;

    nSamp     = (long)(AtodSps);        /* Samples expected between OBC or Marks */
    nSampDiff = 1 + nSamp / 10;         /* Allow 10% difference from expected    */


    dataGpsPtr = dataValues + AtodMarkChan[iboard];
    dataObcPtr = dataValues + AtodCntrChan[iboard];

    for ( j = 0 ; j < AtodNumSamples ; j++ )
    {

        pt = AtodCurrentPtFill + j;

       /* Check that OBC values occur 1 second +/- 10% apart
        ****************************************************/
        if ( *dataObcPtr != 0L ) /* Found OBC */
        {
           if ( Debug >= 5  &&  LastPtObc[iboard] == LAST_PT_INVALID )
              logit( "", "Initializing LastPtObc[%d] to %ld (%ld - %ld)\n",
                     iboard, (pt-nSamp), pt, nSamp );

           if (LastPtObc[iboard] == LAST_PT_INVALID) /* Initialize on first pass */
                   LastPtObc[iboard] = pt - nSamp;

           foundObc++;
           currPtObc = pt;
           wantPtObc = LastPtObc[iboard] + nSamp;
           if ( currPtObc < wantPtObc - nSampDiff  ||
                currPtObc > wantPtObc + nSampDiff  )
           {
              spaceErrorObc = 1;
              logit( "t", "Wrong spacing: OBC found at pt %d, but wanted at pt %d\n", currPtObc, wantPtObc );
           }
           else
              if ( Debug >= 5 )
                 logit( "", "OBC  found at pt %ld, and wanted at pt %ld (%ld + %ld)\n",
                        currPtObc, wantPtObc, LastPtObc[iboard], nSamp );

           LastPtObc[iboard] = currPtObc; /* Update for next pass */

        } /* end if *dataObcPtr */


       /* Check that Gps Mark values occur 1 second +/- 10% apart
        *********************************************************/
        if ( *dataGpsPtr != 0L ) /* Found Mark */
        {
           if ( Debug >= 5  &&  LastPtMark[iboard] == LAST_PT_INVALID )
              logit( "", "Initializing LastPtMark[%d] to %ld (%ld - %ld)\n",
                     iboard, (pt-nSamp), pt, nSamp );

           if (LastPtMark[iboard] == LAST_PT_INVALID) /* Initialize on first pass */
                   LastPtMark[iboard] = pt - nSamp;

           foundMark++;
           currPtMark = pt;
           wantPtMark = LastPtMark[iboard] + nSamp;
           if ( currPtMark < wantPtMark - nSampDiff  ||
                currPtMark > wantPtMark + nSampDiff  )
           {
              spaceErrorMark = 1;
              logit( "t", "Wrong spacing: Mark found at pt %d, but wanted at pt %d\n", currPtMark, wantPtMark );
           }
           else
              if ( Debug >= 5 )
                 logit( "", "MARK found at pt %ld, and wanted at pt %ld (%ld + %ld)\n",
                        currPtMark, wantPtMark, LastPtMark[iboard], nSamp );

           LastPtMark[iboard] = currPtMark; /* Update for next pass */

        } /* end if *dataGpsPtr */


        dataGpsPtr += AtodNumChannels[iboard];
        dataObcPtr += AtodNumChannels[iboard];

    } /* end for j < AtodNumSamples */



   /* Check for unusual or error conditions and take corrective action
    * See header comments for more detailed discussion of conditions
    ******************************************************************/

    /* Moderate - Wrong OBC or Mark spacing */

    if ( spaceErrorObc  ||  spaceErrorMark )
    {
       if ( spaceErrorObc )
          logit( "t", "WARNING: OBC  spacing not at 1 second intervals\n" );
       if ( spaceErrorMark )
          logit( "t", "WARNING: MARK spacing not at 1 second intervals\n" );

       sr_atod_restart_recovery();
       if ( Debug >= 6 )  logit( "", "Leaving  sr_analog_check_spacing with space error\n" );
       return( 0 );
    }


    /* Serious - Too many OBC or Marks */

    if ( foundObc > 2  ||  foundMark > 2 )
    {
       logit( "et", "WARNING: Found too many OBC %d or Marks %d.  Should be 1 or 2 only!\n",
              foundObc, foundMark );
       if ( Debug >= 6 )  logit( "", "Leaving  sr_analog_check_spacing with fatal error\n" );
       sr_terminate();
       return( 0 );
    }


    /* Mild - Not enough OBC or Marks */

    if ( foundObc == 0  ||  foundMark == 0 )
    {
       logit( "t", "WARNING: Found too few  OBC %d or Marks %d.  Should be 1 or 2!\n",
              foundObc, foundMark );
    }


    /* Mild - Mismatched OBC and Marks (moderate case caught by space error above) */

    if ( foundObc > foundMark )
    {
       nDiff    = foundObc - foundMark;
       logit( "", "WARNING: Found %d OBC, but only %d Marks.  Mark may be delayed.\n",
              foundObc, foundMark );
    }


    if ( foundMark > foundObc )
    {
       nDiff    = foundObc - foundMark;
       logit( "", "WARNING: Found %d OBC, but more %d Marks.  Mark may be catching up.\n",
              foundObc, foundMark );
    }



    if ( Debug >= 6 )  logit( "", "Leaving  sr_analog_check_spacing\n" );

    return( 1 );
}

/******************************************************************************
 *  Function: sr_ts_update                                                    *
 *  Purpose:  When a new raw timestamp is available from the ParGps library,  *
 *            we process it.  This involves making space for it, reading it   *
 *            in from the library, checking to see if it is an outlier and    *
 *            correcting it if it is, using it to update the final timestamps *
 *            that will be used to compute tracebuf time.  If the new raw     *
 *            timestamp is not an outlier, it is also used to update the long *
 *            term averages BestCountsBetweenPps and BestCountsBetweenDready. *
 *                                                                            *
 *            The argument to this routine indicates which DAQ board is       *
 *            having its timestamps updated.  The first DAQ is special since  *
 *            it must always exist and it is the only one which can have      *
 *            true GPS information available.  So some parts of this          *
 *            method are only done for the first DAQ (iboard = 0).            *
 ******************************************************************************/
int sr_ts_update( int iboard )
{
    int    i, isGood;
    double cbpCounts, cbdCounts;

    if ( Debug >= 6 )  logit( "", "Starting sr_ts_update\n" );


    if ( iboard == 0 )
    {

      /* Read new valid timestamp from ParGps library
       **********************************************/
       if ( !ParGpsTsReadValid( &TsReal[0], TsValid ) )
               logit( "", "TsReal TimeStamp invalid\n" );

       if ( Debug >= 6 )
       {

           logit( "", "TsReal Valid   Pps   Samp       ObcCount  CountAtP             CountAtD            PctimeAtP             PctimeAtD        YmdSrc  HmsSrc   SecSince1970        PCTPsec1970         PCTDsec1970\n");

           for ( i = 0 ; i < 1 ; i++ ) /* was MAX_TSR */
           {
               logit( "", "TSR:   0    0x%X   %5u  %5ld  0x%08lX  0x%08lX %08lX  0x%08lX %08lX  0x%08lX %08lX   0x%08lX %08lX   %2d    %2d     %lf\n",
                      TsReal[i].Valid, TsReal[i].PpsEvent, TsReal[i].Sample, TsReal[i].CountObc,
                      TsReal[i].CountAtPps.u.HighPart,     TsReal[i].CountAtPps.u.LowPart,
                      TsReal[i].CountAtDready.u.HighPart,  TsReal[i].CountAtDready.u.LowPart,
                      TsReal[i].PctimeAtPps.u.HighPart,    TsReal[i].PctimeAtPps.u.LowPart,
                      TsReal[i].PctimeAtDready.u.HighPart, TsReal[i].PctimeAtDready.u.LowPart,
                      TsReal[i].YmdSource, TsReal[i].HmsSource, TsReal[i].SecSince1970 );
           }
       }


      /* Check if new raw timestamp is an outlier */

       isGood    = 1; // WCT - Hardwired good for now
       cbpCounts = BestCountsBetweenPps;
       cbdCounts = BestCountsBetweenDready;
       TsFix     = TsReal[0];


      /* If it is an outlier (ie not good), then fix it up */

       if ( !isGood )
       {
           // WCT - Code to correct TsFix goes here
       }

    } /* end if iboard == 0 */


   /* Shuffle final timestamps to make room for one more
    * and add the new raw (possibly fixed) timestamp
    *****************************************************/
    for ( i = 0 ; i < MAX_TS-1 ; i++ )
    {
        TimeStamp[iboard][i] = TimeStamp[iboard][i+1];
        if ( Debug >= 5 )
            logit( "", "TS%d Sample %ld, Valid = 0x%X, PpsEvent = %ld, Obc = 0x%lX, SecSince1970 = %lf\n",
                   i, TimeStamp[iboard][i].Sample, TimeStamp[iboard][i].Valid,
                      TimeStamp[iboard][i].PpsEvent, TimeStamp[iboard][i].CountObc,
                      TimeStamp[iboard][i].SecSince1970 );
    }

     TimeStamp[iboard][MAX_TS-1] = TsFix;
        if ( Debug >= 5 )
            logit( "", "TS%d Sample %ld, Valid = 0x%X, PpsEvent = %ld, Obc = 0x%lX, SecSince1970 = %lf\n",
                   MAX_TS-1,
                   TimeStamp[iboard][MAX_TS-1].Sample, TimeStamp[iboard][MAX_TS-1].Valid,
                   TimeStamp[iboard][MAX_TS-1].PpsEvent, TimeStamp[iboard][MAX_TS-1].CountObc,
                   TimeStamp[iboard][MAX_TS-1].SecSince1970 );

        if ( Debug >= 2 )
        {
            if ( TimeStamp[iboard][MAX_TS-1].SecSince1970 > 0.0L &&
                (TimeStamp[iboard][MAX_TS-1].SecSince1970 - TimeStamp[iboard][MAX_TS-2].SecSince1970) < 0.5L )
            {
                logit( "", "WARNING: New timestamp is less than 1 second from previous\n" );
                logit( "", "   Prev %5u  %5ld  0x%08lX %08lX  0x%08lX %08lX     %lf\n",
                       TimeStamp[iboard][MAX_TS-2].PpsEvent,
                       TimeStamp[iboard][MAX_TS-2].Sample,
                       TimeStamp[iboard][MAX_TS-2].CountAtPps.u.HighPart,
                       TimeStamp[iboard][MAX_TS-2].CountAtPps.u.LowPart,
                       TimeStamp[iboard][MAX_TS-2].CountAtDready.u.HighPart,
                       TimeStamp[iboard][MAX_TS-2].CountAtDready.u.LowPart,
                       TimeStamp[iboard][MAX_TS-2].SecSince1970
                       );
                logit( "", "    New %5u  %5ld  0x%08lX %08lX  0x%08lX %08lX     %lf\n",
                       TimeStamp[iboard][MAX_TS-1].PpsEvent,
                       TimeStamp[iboard][MAX_TS-1].Sample,
                       TimeStamp[iboard][MAX_TS-1].CountAtPps.u.HighPart,
                       TimeStamp[iboard][MAX_TS-1].CountAtPps.u.LowPart,
                       TimeStamp[iboard][MAX_TS-1].CountAtDready.u.HighPart,
                       TimeStamp[iboard][MAX_TS-1].CountAtDready.u.LowPart,
                       TimeStamp[iboard][MAX_TS-1].SecSince1970
                       );
            }
        }



    if ( iboard == 0 )
    {
      /* If the new one is good (not outlier), update the long term averages,
       * BestCountsBetweenPps and BestCountsBetweenDready.  These allows us to
       * throw out outliers later.
       *
       * Here, cbpCounts is the measured difference in counts from one PPS to
       * the next.  This time is exactly 1 second as defined by the "big clock
       * in the sky" (eg GPS time).  If the PC counter were perfect, this count
       * would equal the counter frequency.
       *************************************************************************/
       if ( isGood )
       {
           /* Update long term averages */

           if ( FirstTimeFilter )
           {
               BestCountsBetweenPps    = cbpCounts;
               BestCountsBetweenDready = cbdCounts;
               FirstTimeFilter         = 0;
               if ( Debug >= 5 )  logit( "", "First time init for filter info\n" );
           }

           if ( Debug >= 6 )  logit( "", "Before BCBP = %lf, BCBD = %lf\n",
                                    BestCountsBetweenPps, BestCountsBetweenDready );

           sr_filter( FILTER_PPS,    cbpCounts );
           sr_filter( FILTER_DREADY, cbdCounts );

           if ( Debug >= 6 )  logit( "", "After  BCBP = %lf, BCBD = %lf\n",
                                    BestCountsBetweenPps, BestCountsBetweenDready );

       } /* end if isGood */

    } /* end if iboard == 0 */


    if ( Debug >= 6 )  logit( "", "Leaving  sr_ts_update\n" );
    return( 1 );
}

/******************************************************************************
 *  Function: sr_filter                                                       *
 *  Purpose:  This updates the long term "counts between" averages.  It can   *
 *            be called for BestCountsBetweenPps or BestCountsBetweenDready.  *
 *            The new average value is computed based on one new value for    *
 *            measured counts in combination with the previous long term      *
 *            average value.  The formula used is a simple two term filter    *
 *            of the form:                                                    *
 *                                                                            *
 *            f[i] = S * x[i] +  C * f[i-1]                                   *
 *                                                                            *
 *            where x[i] is the new data point, S is a scaling factor, C is   *
 *            the filter coefficient that controls how much averaging is      *
 *            applied, and f[i] is the resulting filter value.                *
 *                                                                            *
 *            The formula above can also be written in expanded form as:      *
 *                                                                            *
 *                                1            2                              *
 *            f[i] = S * (x[i] + C * x[i-1] + C * x[i-2] + ...                *
 *                                                                            *
 *            The scale factor S should be set to 1-C for normalized data.    *
 *                                                                            *
 *            When the coefficient C is chosen to be 0, no averaging is       *
 *            applied (f[i] = x[i]) and the output is equal to the current    *
 *            new data value.                                                 *
 *                                                                            *
 *            We want to select C so, in the expanded form, the coefficient   *
 *            appearing before the Nth previous data point (x[i-N]) is very   *
 *            small so that it and all earlier data points make almost no     *
 *            contribution to the filtered result.                            *
 *                                                                            *
 *            See sr_filter_init for the setting chosen for C.                *
 ******************************************************************************/
void sr_filter( int target, double newCount )
{
    if ( Debug >= 7 )  logit( "", "Starting sr_filter\n" );


    if ( target == FILTER_PPS )
    {
        BestCountsBetweenPps = (CbpScale * newCount) +
                               (CbpCoeff * BestCountsBetweenPps);
        CbpLimit             = BestCountsBetweenPps * CBP_RANGE;

        if ( Debug >= 6 )
           logit( "", "New measured CBP %lf, BestCountsBetweenPps    %lf, limit %lf\n",
                  newCount, BestCountsBetweenPps, CbpLimit );
    }

    else if ( target == FILTER_DREADY )
    {
        BestCountsBetweenDready = (CbdScale * newCount) +
                                  (CbdCoeff * BestCountsBetweenDready);
        CbdLimit                = BestCountsBetweenDready * CBD_RANGE;

        if ( Debug >= 6 )
           logit( "", "New measured CBD %lf, BestCountsBetweenDready %lf, limit %lf\n",
                  newCount, BestCountsBetweenDready, CbdLimit );
    }

    else
    {
        logit( "", "Filter function called with unknown type %d\n", target );
    }

    if ( Debug >= 7 )  logit( "", "Leaving  sr_filter\n" );
    return;
}

/******************************************************************************
 *  Function: sr_send_traces                                                  *
 *  Purpose:  Fill a earthworm tracebuf messages with data just acquired by   *
 *            the PARxCH and send it out to the ring.  The data is demuxed    *
 *            here so there will be one message per channel.                  *
 *                                                                            *
 *            Note: GpsLockCount increments for each buffer of data and       *
 *            GpsLockReportInterval is measured in seconds, so comparing      *
 *            them is only valid because each buffer is about 1 second long.  *
 ******************************************************************************/
void sr_send_traces( void )
{
    static int    lastObcMethod  = TIME_METHOD_NONE;
    static int    lastGpsMethod  = TIME_METHOD_NONE;
    static int    lastPctMethod  = TIME_METHOD_NONE;
    static int    lastSpsMethod  = TIME_METHOD_NONE;
    static int    firstPass = 1;
    static double lastObcEndTime, lastObcStartTime;
    static double lastGpsEndTime, lastGpsStartTime;
    static double lastPctEndTime, lastPctStartTime;
    static double lastSpsEndTime, lastSpsStartTime;
    int    i, j, k, ichan, rc, saveTimeMethod, saveLastMethod;
    long  *tracePtr, *atodPtr, istartpt;
    double saveStartTime, saveEndTime, saveTHstart, saveTHend;
    char  *gpsLockCurrStr;

    if ( Debug >= 6 )  logit( "", "Starting sr_send_traces\n" );


   /* Debugging code that compares four time computation methods
    ************************************************************/
    if ( Debug >= 7 )
    {
        if (firstPass)
        {
            lastObcStartTime = AtodStartTime[0];
            lastGpsStartTime = AtodStartTime[0];
            lastPctStartTime = AtodStartTime[0];
            lastSpsStartTime = AtodStartTime[0];
            lastObcEndTime   = AtodEndTimeLast[0];
            lastGpsEndTime   = AtodEndTimeLast[0];
            lastPctEndTime   = AtodEndTimeLast[0];
            lastSpsEndTime   = AtodEndTimeLast[0];
            firstPass        = 0;
        }

        /* Save true values */
        saveTimeMethod = UseTimeMethod;
        saveLastMethod = LastTimeMethod;
        saveStartTime  = AtodStartTime[0];
        saveEndTime    = AtodEndTimeLast[0];
        saveTHstart    = TraceHead->starttime;
        saveTHend      = TraceHead->endtime;


        /* Use OBC method */
        UseTimeMethod        = TIME_METHOD_OBC;
        LastTimeMethod       = lastObcMethod;
        AtodStartTime[0]     = lastObcStartTime;
        AtodEndTimeLast[0]   = lastObcEndTime;
        TraceHead->starttime = saveTHstart;
        TraceHead->endtime   = saveTHend;
        sr_ts_set_tracebuf_time();
        lastObcMethod        = LastTimeMethod;
        lastObcStartTime     = AtodStartTime[0];
        lastObcEndTime       = AtodEndTimeLast[0];


        /* Use GPS method */
        UseTimeMethod        = TIME_METHOD_GPS;
        LastTimeMethod       = lastGpsMethod;
        AtodStartTime[0]     = lastGpsStartTime;
        AtodEndTimeLast[0]   = lastGpsEndTime;
        TraceHead->starttime = saveTHstart;
        TraceHead->endtime   = saveTHend;
        sr_ts_set_tracebuf_time();
        lastGpsMethod        = LastTimeMethod;
        lastGpsStartTime     = AtodStartTime[0];
        lastGpsEndTime       = AtodEndTimeLast[0];


        /* Use PCTIME method */
        UseTimeMethod        = TIME_METHOD_PCT;
        LastTimeMethod       = lastPctMethod;
        AtodStartTime[0]     = lastPctStartTime;
        AtodEndTimeLast[0]   = lastPctEndTime;
        TraceHead->starttime = saveTHstart;
        TraceHead->endtime   = saveTHend;
        sr_ts_set_tracebuf_time();
        lastPctMethod        = LastTimeMethod;
        lastPctStartTime     = AtodStartTime[0];
        lastPctEndTime       = AtodEndTimeLast[0];


        /* Use SPS method */
        UseTimeMethod        = TIME_METHOD_SPS;
        LastTimeMethod       = lastSpsMethod;
        AtodStartTime[0]     = lastSpsStartTime;
        AtodEndTimeLast[0]   = lastSpsEndTime;
        TraceHead->starttime = saveTHstart;
        TraceHead->endtime   = saveTHend;
        sr_ts_set_tracebuf_time();
        lastSpsMethod        = LastTimeMethod;
        lastSpsStartTime     = AtodStartTime[0];
        lastSpsEndTime       = AtodEndTimeLast[0];


        /* Restore true values */
        AtodStartTime[0]     = saveStartTime;
        AtodEndTimeLast[0]   = saveEndTime;
        TraceHead->starttime = saveTHstart;
        TraceHead->endtime   = saveTHend;
        UseTimeMethod        = saveTimeMethod;
        LastTimeMethod       = saveLastMethod;

    } /* end if Debug >= 7 */



   /* Set trace start and end times (sec since 1970) same for all channels
    **********************************************************************/
    sr_ts_set_tracebuf_time();


    if ( UsingGps )
    {
       /* Update GPS lock summary info
        ******************************/
        if ( TimeStamp[0][MAX_TS-1].NumSat >= 3 )
        {
            GpsLockGood++;
            gpsLockCurrStr = "Good";
        }
        else
        {
            GpsLockBad++;
            gpsLockCurrStr = "Bad";
        }
        GpsLockCount++;


       /* We've reached the end of a GPS reporting interval
        ***************************************************/
        if ( GpsLockCount >= GpsLockReportInterval && GpsLockReportInterval > 0 )
        {

           /* Log the GPS lock info
            ***********************/
            if ( Debug >= 1 )
                logit( "t", "srpar2ew: GPS Lock Stats = Good %5ld, Bad %5ld, Currently %s\n",
                       GpsLockGood, GpsLockBad, gpsLockCurrStr );


           /* Send error message if status changed
            **************************************/
            if ( GpsLockBad < GpsLockBadLimit ) /* GPS lock good now */
            {
                if ( GpsLockStatus != 1 )       /* but bad before    */
                {
                    sprintf( Msg, "GPS satellite count now OK (Good %ld / %ld seconds)",
                             GpsLockGood, GpsLockReportInterval );
                    sr_error( EW_ERROR_BASE_PARGPS+PARGPS_ERROR_NOT_AVAILABLE, Msg );
                }
                GpsLockStatus = 1;
            }

            else /* ( GpsLockBad >= GpsLockBadLimit ) */ /* GPS lock bad now */
            {
                if ( GpsLockStatus != 0)                 /* but good before  */
                {
                    sprintf( Msg, "GPS satellite count too low (Bad %ld / %ld seconds)",
                             GpsLockBad, GpsLockReportInterval );
                    sr_error( EW_ERROR_BASE_PARGPS+PARGPS_ERROR_NOT_AVAILABLE, Msg );
                }
                GpsLockStatus = 0;
            }


           /* Reset lock counts for next interval
            *************************************/
            GpsLockCount = GpsLockGood = GpsLockBad = 0;


        } /* end if GpsLockCount > GpsLockReportInterval */


    } /* end if UsingGps */



   /* Set tracebuf type as new or old format
    ****************************************/
    if (OutputMsgType == TypeTraceBuf2 && TRACE2_OK)
        Logo.type = TypeTraceBuf2;   /* We'll send the new tracebuf2s */
    else
        Logo.type = TypeTraceBuf;    /* We'll send the old tracebufs  */



   /* Loop making one tracebuf message per channel
    **********************************************/
    if ( Debug >= 5 )
        logit( "", "Preparing to send traces\n" );

    DbgLastStartDiff = AtodStartTime[0] - DbgLastStart;
    DbgLastEndDiff   = AtodEndTime[0]   - DbgLastEnd;

    if (Debug >= 5)
            logit( "", "Buf %3lu, Start %lf, End %lf, rate = %10.6lf, (Diff S = %lf, E = %lf)\n",
                   AtodBufferNum-1,
                   TraceHead->starttime,
                   TraceHead->endtime,
                   TraceHead->samprate,
                   DbgLastStartDiff,
                   DbgLastEndDiff
                   );

    DbgLastStart = AtodStartTime[0];
    DbgLastEnd   = AtodEndTime[0];

    if ( Debug >= 5 )  logit( "", "Now Writing = 0x%lX\n", AtodBufferWrite );


    istartpt = 0;
    ichan    = 0;

   /* Process all channels for all DAQ boards
    *****************************************/
    for ( i = 0 ; i < AtodNumBoards ; i++ )
    {
       TraceHead->starttime = AtodStartTime[i];
       TraceHead->endtime   = AtodEndTime[i];

      /* Process all channels for selected board
       *****************************************/
       for ( j = 0 ; j < AtodNumChannels[i] ; j++ )
       {

          /* Fill the trace buffer header
           ******************************/
           sr_strncpy( TraceHead->sta,  ChanList[ichan].sta,  TRACE_STA_LEN );  /* Site      */
           sr_strncpy( TraceHead->net,  ChanList[ichan].net,  TRACE_NET_LEN );  /* Network   */
           if (OutputMsgType == TypeTraceBuf2)
           {
              sr_strncpy(  TraceHead->chan,                ChanList[ichan].comp, TRACE2_CHAN_LEN );
              sr_strncpy( &TraceHead->chan[TRACE_POS_LOC], ChanList[ichan].loc,  TRACE2_LOC_LEN );
           }
           else /* (OutputMsgType == TypeTraceBuf) */
              sr_strncpy( TraceHead->chan, ChanList[ichan].comp, TRACE_CHAN_LEN );

           TraceHead->pinno = ChanList[ichan].pin;                           /* Pin num   */



          /* Transfer samples from DAQ buffer to tracebuf buffer
           *****************************************************/
           tracePtr = &TraceDat[0];
           atodPtr  = &AtodBufferWrite[istartpt+j];

           for ( k = 0 ; k < AtodNumSamples ; k++ )
           {
               *tracePtr = *atodPtr;

               if (SummaryMin[i][j] > *tracePtr)
                  SummaryMin[i][j] = *tracePtr;
               else if (SummaryMax[i][j] < *tracePtr)
                  SummaryMax[i][j] = *tracePtr;

               tracePtr++;
               atodPtr += AtodNumChannels[i];  /* Demux as we go */
           }

           if ( Debug >= 5 )
           {
               logit( "", "  S=%s C=%s N=%s L=%s pin=%d, ver=%c%c\n",
                     TraceHead->sta,
                     TraceHead->chan,
                     TraceHead->net,
                    &TraceHead->chan[TRACE_POS_LOC],
                     TraceHead->pinno,
                     TraceHead->chan[TRACE_POS_VER],
                     TraceHead->chan[TRACE_POS_VER+1]
                    );
           }


          /* Send tracebuf message to the transport ring
           *********************************************/
           rc = tport_putmsg( &Region, &Logo, TraceBufSize, TraceBuffer );

           if ( rc == PUT_TOOBIG )
               logit( "e", "srpar2ew: Trace message for channel %d too big\n", i );

           if ( rc == PUT_NOTRACK )
               logit( "e", "srpar2ew: Tracking error while sending channel %d\n", i );


           ichan++;

         } /* end for j < AtodNumChannels[i] */

       istartpt += (AtodNumChannels[i] * AtodNumSamples);

    } /* end for i < AtodNumBoards */


   /* Log summary information periodically
    **************************************/
    SummaryCount++;
    if ( SummaryCount >= SummaryReportInterval && SummaryReportInterval > 0 )
    {
       if ( Debug >= 1 )
       {
          logit( "", "Summary Info:\n    STA CMP NET LOC  min val               max val\n" );
          ichan = 0;
          for ( i = 0 ; i < AtodNumBoards ; i++ )
          {
             for ( j = 0 ; j < AtodNumChannels[i] ; j++ )
             {
                logit( "", "  %5s %3s %3s %2s 0x%08lX (%8ld) 0x%08lX (%8ld)\n",
                    ChanList[ichan].sta,
                    ChanList[ichan].comp,
                    ChanList[ichan].net,
                    ChanList[ichan].loc,
                    SummaryMin[i][j], SummaryMin[i][j],
                    SummaryMax[i][j], SummaryMax[i][j]
                    );

                ichan++;

             } /* end for j < AtodNumChannels[i] */

          } /* end for i < AtodNumBoards */

       } /* end if Debug >= 1 */


      /* Reset summary info for next interval
       **************************************/
       sr_summary_init();

    } /* end if SummaryCount */



   /* Update current point count to start of next acquired data buffer
    ******************************************************************/
    AtodCurrentPtWrite += AtodNumSamples;


    if ( Debug >= 6 )
        logit( "", "Leaving  sr_send_traces with new current pt Fill = %d, Write = %d\n",
               AtodCurrentPtFill, AtodCurrentPtWrite );
    return;
}

/******************************************************************************
 *  Function: sr_ts_set_tracebuf_time                                         *
 *  Purpose:  Set the trace start and end times in seconds since 1970.  This  *
 *            value is the same for all the channels in the current buffer.   *
 *            The computation is slightly different, depending on what data   *
 *            is available.  The best is using the GPS counter info from the  *
 *            the PAR8CH on board counter.  Next best is using GPS info from  *
 *            the PC counter info kept in the PPS data area of the PARGPS.    *
 *            If no GPS is available, then just compute what the expected     *
 *            time would be assuming the sampling is perfectly stable and     *
 *            accurate.                                                       *
 ******************************************************************************/
void sr_ts_set_tracebuf_time( void )
{
    int        setStartTime, setEndTime, timesOk[MAX_BOARD];
    int        tsCurrLow, tsCurrHigh, tsEndLow, tsEndHigh;
    int        i, thisMethod, ok1, ok2, iTS;
    long       atodEndPt;
    double     newStartTime, newEndTime, computedEndTime, computedSampRate;
    double     sampleTime[MAX_BOARD], expectedStart[MAX_BOARD];
    double     correctedTime[MAX_BOARD];

    if ( Debug >= 6 )
       logit( "", "Starting sr_ts_set_tracebuf_time, UseTimeMethod = %d\n",
              UseTimeMethod );

    iTS          = MAX_TS-2;  /* This should be the timestamp between the start + end pts */
    setStartTime = 0;
    setEndTime   = 0;
    atodEndPt       = AtodCurrentPtWrite + (AtodNumSamples-1);

    if ( Debug >= 6 )
    {
        logit( "", "  Ts# Valid  Pps   Samp     CountAtP             CountAtD            PctimeAtP             PctimeAtD        YmdSrc  HmsSrc   SecSince1970\n");

        for ( i = 0 ; i < MAX_TS ; i++ )
        {
            logit( "", "  %d   0x%X  %5u  %5ld   0x%08lX %08lX  0x%08lX %08lX  0x%08lX %08lX   0x%08lX %08lX   %2d    %2d     %lf\n",
                   i, TimeStamp[0][i].Valid, TimeStamp[0][i].PpsEvent, TimeStamp[0][i].Sample,
                   TimeStamp[0][i].CountAtPps.u.HighPart,     TimeStamp[0][i].CountAtPps.u.LowPart,
                   TimeStamp[0][i].CountAtDready.u.HighPart,  TimeStamp[0][i].CountAtDready.u.LowPart,
                   TimeStamp[0][i].PctimeAtPps.u.HighPart,    TimeStamp[0][i].PctimeAtPps.u.LowPart,
                   TimeStamp[0][i].PctimeAtDready.u.HighPart, TimeStamp[0][i].PctimeAtDready.u.LowPart,
                   TimeStamp[0][i].YmdSource, TimeStamp[0][i].HmsSource, TimeStamp[0][i].SecSince1970 );
        }
    }

   /* Determine which time stamps to use for the start and end points
    *****************************************************************/
    ok1 = sr_ts_select( AtodCurrentPtWrite, TsValid, &tsCurrLow, &tsCurrHigh );
    ok2 = sr_ts_select( atodEndPt,          TsValid, &tsEndLow,  &tsEndHigh );
    if (ok1 && ok2)
            timesOk[0] = 1;
    else
            timesOk[0] = 0;

    if ( Debug >= 6 )
    {
        logit( "", "CurrPtW %ld, TS low %d, TS hi %d\n", AtodCurrentPtWrite, tsCurrLow, tsCurrHigh );
        logit( "", " EndPt  %ld, TS low %d, TS hi %d\n", atodEndPt,          tsEndLow,  tsEndHigh );
    }



   /* Use PAR8CH 800 ns On Board Counter
    ************************************/
    if ( timesOk[0] && UseTimeMethod == TIME_METHOD_OBC )
    {
        if ( ParGpsTsComputeTimeObc( &TimeStamp[0][tsCurrLow], &TimeStamp[0][tsCurrHigh],
                                     BestObcCountsPerSample,
                                     TsValid, AtodCurrentPtWrite, &newStartTime ) )
        {
            AtodStartTime[0] = newStartTime;
            setStartTime     = 1;
            thisMethod       = TIME_METHOD_OBC;
            if ( Debug >= 3 )
                logit( "t", "srpar2ew: DAQ #0 buf start time (obc) = %lf, Point %ld, GPS %d satellites\n",
                       AtodStartTime[0], AtodCurrentPtWrite, TimeStamp[0][iTS].NumSat );
            sr_compute_daq_times( newStartTime, tsCurrHigh, UseTimeMethod, COMPUTE_START );
        }
        if ( ParGpsTsComputeTimeObc( &TimeStamp[0][tsEndLow], &TimeStamp[0][tsEndHigh],
                                     BestObcCountsPerSample,
                                     TsValid, atodEndPt, &newEndTime ) )
        {
            AtodEndTime[0] = newEndTime;
            setEndTime     = 1;
            if ( Debug >= 4 )
                logit( "t", "srpar2ew: DAQ #0 buf end time   (obc) = %lf, Point %ld\n",
                       AtodEndTime[0], atodEndPt );
            sr_compute_daq_times( newEndTime, tsEndLow, UseTimeMethod, COMPUTE_END );
        }

        for ( i = 0 ; i < AtodNumBoards ; i++ )
        {
           if ( Debug >= 3  &&  setStartTime )
              logit( "t", "srpar2ew: DAQ #%d buf start time (obc) = %lf, Point %ld\n",
                     i, AtodStartTime[i], AtodCurrentPtWrite );

           if ( Debug >= 4  &&  setEndTime )
              logit( "t", "srpar2ew: DAQ #%d buf end time   (obc) = %lf, Point %ld\n",
                     i, AtodEndTime[i], atodEndPt );
        }

   } /* end if UseTimeMethod == TIME_METHOD_OBC */


   /* Use PC counter from PARGPS PPS area
    *************************************/
    else if ( timesOk[0] && UseTimeMethod == TIME_METHOD_GPS )
    {
        if ( ParGpsTsComputeTimePps( &TimeStamp[0][tsCurrLow], &TimeStamp[0][tsCurrHigh],
                                     BestCountsBetweenPps,
                                     TsValid, AtodCurrentPtWrite, &newStartTime ) )
        {
            AtodStartTime[0] = newStartTime;
            setStartTime     = 1;
            thisMethod       = TIME_METHOD_GPS;
            if ( Debug >= 3 )
                logit( "t", "srpar2ew: Buf start time (pps) = %lf, Point %ld, GPS %d satellites\n",
                       AtodStartTime[0], AtodCurrentPtWrite, TimeStamp[0][iTS].NumSat );
            sr_compute_daq_times( newStartTime, tsCurrHigh, UseTimeMethod, COMPUTE_START );
        }
        if ( ParGpsTsComputeTimePps( &TimeStamp[0][tsEndLow], &TimeStamp[0][tsEndHigh],
                                     BestCountsBetweenPps,
                                     TsValid, atodEndPt, &newEndTime ) )
        {
            AtodEndTime[0] = newEndTime;
            setEndTime     = 1;
            if ( Debug >= 4 )
                logit( "t", "srpar2ew: Buf end time   (pps) = %lf, Point %ld\n",
                       AtodEndTime[0], atodEndPt );
            sr_compute_daq_times( newEndTime, tsEndLow, UseTimeMethod, COMPUTE_END );
        }
    } /* end else if UseTimeMethod == TIME_METHOD_GPS */



   /* Use PC system time counter from PARGPS PPS area
    *************************************************/
    else if ( timesOk[0] && UseTimeMethod == TIME_METHOD_PCT )
    {
        if ( Debug >= 4 )
        {
            logit( "", "TS%d %ld, AtodCurrentPtWrite %ld, TS%d %ld\n",
                  tsCurrLow, TimeStamp[0][tsCurrLow].Sample,
                  AtodCurrentPtWrite,
                  tsCurrHigh, TimeStamp[0][tsCurrHigh].Sample );

            logit( "", "TS%d %ld, atodEndPt %ld, TS%d %ld\n",
                  tsEndLow, TimeStamp[0][tsEndLow].Sample,
                  atodEndPt,
                  tsEndHigh, TimeStamp[0][tsEndHigh].Sample );
        }

        if (AtodCurrentPtWrite < TimeStamp[0][tsCurrLow].Sample)
                logit( "t", "WARNING: current pt %ld is out of range below TS%d %ld\n",
                      AtodCurrentPtWrite, tsCurrLow, TimeStamp[0][tsCurrLow].Sample );
        if (AtodCurrentPtWrite > TimeStamp[0][tsCurrHigh].Sample)
                logit( "t", "WARNING: current pt %ld is out of range above TS%d %ld\n",
                      AtodCurrentPtWrite, tsCurrHigh, TimeStamp[0][tsCurrHigh].Sample );


        if (atodEndPt < TimeStamp[0][tsEndLow].Sample)
                logit( "t", "WARNING: thisend pt %ld is out of range below TS%d %ld\n",
                      atodEndPt, tsEndLow, TimeStamp[0][tsEndLow].Sample );
        if (atodEndPt > TimeStamp[0][tsEndHigh].Sample)
                logit( "t", "WARNING: thisend pt %ld is out of range above TS%d %ld\n",
                      atodEndPt, tsEndHigh, TimeStamp[0][tsEndHigh].Sample );


        if ( ParGpsTsComputeTimePctime( &TimeStamp[0][tsCurrLow], &TimeStamp[0][tsCurrHigh],
                                        BestCountsPerSecond,
                                        TsValid, AtodCurrentPtWrite, &newStartTime ) )
        {
            AtodStartTime[0] = newStartTime;
            setStartTime     = 1;
            thisMethod       = TIME_METHOD_PCT;
            if ( Debug >= 3 )
                logit( "t", "srpar2ew: Buf start time (pct) = %lf, Point %ld, GPS %d satellites\n",
                       AtodStartTime[0], AtodCurrentPtWrite, TimeStamp[0][iTS].NumSat );
            sr_compute_daq_times( newStartTime, tsCurrHigh, UseTimeMethod, COMPUTE_START );
        }
        else
        {
            if ( Debug >= 6 )
                logit( "", "Failed to compute PCT time\n" );
        }
        if ( ParGpsTsComputeTimePctime( &TimeStamp[0][tsEndLow], &TimeStamp[0][tsEndHigh],
                                        BestCountsPerSecond,
                                        TsValid, atodEndPt, &newEndTime ) )
        {
            AtodEndTime[0] = newEndTime;
            setEndTime     = 1;
            if ( Debug >= 4 )
                logit( "t", "srpar2ew: Buf end time   (pct) = %lf, Point %ld\n",
                       AtodEndTime[0], atodEndPt );
            sr_compute_daq_times( newEndTime, tsEndLow, UseTimeMethod, COMPUTE_END );
        }

        if ( Debug >= 5 )
        {
            logit( "", "Pct last->this (1sps) %lf, this len %lf, Last end %lf\n",
                  (AtodStartTime[0]-AtodEndTimeLast[0]), (AtodEndTime[0]-AtodStartTime[0]), AtodEndTimeLast[0] );
        }

    } /* end else if UseTimeMethod == TIME_METHOD_PCT */



   /* Use sampling rate and previous time
    *************************************/
    else if ( timesOk[0] && UseTimeMethod == TIME_METHOD_SPS )
    {
        if ( ParGpsTsComputeTimeSps( &TimeStamp[0][tsCurrLow], &TimeStamp[0][tsCurrHigh],
                                     AtodSps, TsValid,
                                     AtodCurrentPtWrite, &newStartTime ) )
        {
            AtodStartTime[0] = newStartTime;
            setStartTime     = 1;
            thisMethod       = TIME_METHOD_SPS;
            if ( Debug >= 3 )
                logit( "t", "srpar2ew: Buf start time (sps) = %lf, Point %ld, GPS %d satellites\n",
                       AtodStartTime[0], AtodCurrentPtWrite, TimeStamp[0][iTS].NumSat );
            sr_compute_daq_times( newStartTime, tsCurrHigh, UseTimeMethod, COMPUTE_START );
        }
        if ( ParGpsTsComputeTimeSps( &TimeStamp[0][tsEndLow], &TimeStamp[0][tsEndHigh],
                                     AtodSps, TsValid,
                                     atodEndPt, &newEndTime ) )
        {
            AtodEndTime[0] = newEndTime;
            setEndTime     = 1;
            if ( Debug >= 4 )
                logit( "t", "srpar2ew: Buf end time   (sps) = %lf, Point %ld\n",
                       AtodEndTime[0], atodEndPt );
            sr_compute_daq_times( newEndTime, tsEndLow, UseTimeMethod, COMPUTE_END );
        }
    } /* end else UseTimeMethod == TIME_METHOD_SPS */



   /* Compute based on global data sampling rate and previous time
    **************************************************************/
    if (!setStartTime) /* All methods failed or UseTimeMethod == TIME_METHOD_NONE */
    {
        AtodStartTime[0] = AtodEndTimeLast[0] + 1 / AtodSps;
        thisMethod       = TIME_METHOD_NONE;
        sr_compute_daq_times( AtodStartTime[0], tsCurrHigh, thisMethod, COMPUTE_START );

        for ( i = 0 ; i < AtodNumBoards ; i++ )
        {
           if ( Debug >= 3 )
              logit( "t", "srpar2ew: DAQ #%d buf start time (default) = %lf, Point %ld\n",
                     i, AtodStartTime[i], AtodCurrentPtWrite );
        }

    }
    if (!setEndTime) /* All methods failed or UseTimeMethod == TIME_METHOD_NONE */
    {
        AtodEndTime[0] = AtodStartTime[0] + (AtodNumSamples-1) / AtodSps;
        sr_compute_daq_times( AtodEndTime[0], tsEndLow, thisMethod, COMPUTE_END );
        for ( i = 0 ; i < AtodNumBoards ; i++ )
        {
           if ( Debug >= 4 )
              logit( "t", "srpar2ew: DAQ #%d buf end time   (default) = %lf, Point %ld\n",
                     i, AtodEndTime[i], atodEndPt );
        }
    }


    if ( Debug >= 5 )
    {
        logit( "", "New Last end %lf, this start %lf, this end %lf\n",
              AtodEndTimeLast[0], AtodStartTime[0], AtodEndTime[0] );
        logit( "", "New last->this (1sps) %lf, this len %lf\n",
              (AtodStartTime[0]-AtodEndTimeLast[0]), (AtodEndTime[0]-AtodStartTime[0]) );
    }

    if ( Debug >= 5 )
         logit( "", "Method used for computing time.  Last = %d, current = %d, Request = %d\n",
                LastTimeMethod, thisMethod, UseTimeMethod );



   /* Check for gaps or overlaps on all DAQs
    ****************************************/
    for ( i = 0 ; i < AtodNumBoards ; i++ )
    {
       timesOk[i]        = 1;
       sampleTime[i]     = (AtodEndTime[i] - AtodStartTime[i]) / (double)(AtodNumSamples-1);
       expectedStart[i]  = AtodEndTimeLast[i] + sampleTime[i]*1.5; /* Allow .5 sample slop */


       if ( GapTrailer[i] > 0 ) /* Print additional debugging info after a gap has occurred */
       {
           if (Debug >= 3)
           {
              logit( "", "Buffer %d after gap\n", MAXGAPTRAIL-GapTrailer[i] );
              sr_log_gap_details( thisMethod, expectedStart[i], AtodEndTime[i] );
           }
           GapTrailer[i]--;
       }

       if ( AtodStartTime[i] < AtodEndTimeLast[i] ) /* Check for overlap */
       {
           logit( "et", "srpar2ew: Gap check: Overlap detected DAQ #%d\n     (start %lf < last end %lf)\n",
                  i, AtodStartTime[i], AtodEndTimeLast[i] );
           sr_log_gap_details( thisMethod, expectedStart[i], AtodEndTime[i] );
           timesOk[i]       = 0;
           correctedTime[i] = AtodEndTimeLast[i];
           GapTrailer[i]    = MAXGAPTRAIL;
       }

       if ( AtodStartTime[i] > expectedStart[i] ) /* Check for gap */
       {
           logit( "et", "srpar2ew: Gap check: Gap detected DAQ #%d\n     (found %lf start > expected start %lf = \n      last end %lf + sample %lf)\n",
                  i, AtodStartTime[i], expectedStart[i], AtodEndTimeLast[i], sampleTime[i] );
           sr_log_gap_details( thisMethod, expectedStart[i], AtodEndTime[i] );
           timesOk[i]       = 0;
           correctedTime[i] = expectedStart[i];
           GapTrailer[i]    = MAXGAPTRAIL;
       }



       /* If times are not consistent, fix them as no gaps or overlaps are allowed
        **************************************************************************/
       if ( !timesOk[i] )
       {
           if (thisMethod == LastTimeMethod  &&  LastTimeMethod != TIME_METHOD_NONE)
           {
               if (Debug >= 3 )
                   logit( "", "DAQ #%d time corrected from %lf to %lf\n",
                         i, AtodStartTime[i], correctedTime[i] );
               AtodStartTime[i] = correctedTime[i];
           }
           else
           {
               if (Debug >= 4 ) logit( "", "DAQ #%d no time correction if we are just starting out ....\n", i );
           }

       }

    } /* end for i < AtodNumBoards */



   /* Check computed end time and sample rate
    *****************************************/
    if (Debug >= 5)
    {
       /* We currently use interpolated end time for TraceHead->endtime */
        computedEndTime = AtodStartTime[0] + (AtodNumSamples - 1)*sampleTime[0];
        logit( "", "End time: Interpolated = %lf, Computed = %lf\n",
                   AtodEndTime[0], computedEndTime );

        /* We currently do not change the sample rate */
        computedSampRate = (AtodNumSamples-1) / (AtodEndTime[0] - AtodStartTime[0]);
        logit( "", "Computed sample rate = %lf (std = %lf), keeping standard\n", computedSampRate, AtodSps );
        //WCT - Should we change sample rate or not ?
        //      Wave_Viewer says NO, Decimate says YES?
        //      TraceHead->samprate = computedSampRate;
    }



   /* Update the quality flag
    *************************/
    if ( TimeStamp[0][iTS].NumSat < 3                          ||  /* not enough sats */
         (TimeStamp[0][iTS].YmdSource != TIME_SOURCE_GPS &&        /* YMD not GPS/OBC */
          TimeStamp[0][iTS].YmdSource != TIME_SOURCE_OBC)      ||
         (TimeStamp[0][iTS].HmsSource != TIME_SOURCE_GPS &&        /* HMS not GPS/OBC */
          TimeStamp[0][iTS].HmsSource != TIME_SOURCE_OBC)
                                                          )
    {
        TraceHead->quality[0] |= TIME_TAG_QUESTIONABLE;
        if ( Debug >= 5 )  logit( "", "Setting time tag questionable (Nsat = %d)\n", TimeStamp[0][iTS].NumSat );
    }
    else
        TraceHead->quality[0] = QUALITY_OK;


   /* Prepare for next pass
    ***********************/
    LastTimeMethod = thisMethod;

    for ( i = 0 ; i < AtodNumBoards ; i++ )
       AtodEndTimeLast[i] = AtodEndTime[i];


    if ( Debug >= 6 )  logit( "", "Leaving  sr_ts_set_tracebuf_time\n" );
    return;
}

/******************************************************************************
 *  Function: sr_compute_daq_times                                            *
 *  Purpose:  Compute start time of buffer for secondary DAQ systems by       *
 *            correcting the time determined for first DAQ system.  The       *
 *            computation is described below.  The following figure may       *
 *            also help in understanding the quantities used.                 *
 *                                                                            *
 *            TB                        TP     TD                             *
 *             .          .          .          .                             *
 *                                       <- TO ->                             *
 *             <-------------- TN -------------->                             *
 *                                                                            *
 *            TB = time at sample #B (buffer start)                           *
 *            TP = time at PPS event #P                                       *
 *            TD = time at sample #D (dready = 1st sample after PPS)          *
 *            TN = elapsed time between sample #B and sample #D               *
 *            TO = elapsed time between PPS and sample #D                     *
 *                                                                            *
 *            TP = TB + TN - TO  and is the same time for both DAQ systems    *
 *                                                                            *
 *            TBi + TNi - TOi = TP = TB0 + TN0 - TO0                          *
 *                                                                            *
 *            TBi = TB0 + (TOi - TO0) - (TNi - TN0)                           *
 *                                                                            *
 *            TB = bufTime                                                    *
 *            TO = TimeStamp.CountObc / ObcPerSec                             *
 *            TN = TimeStamp.Sample / SampleRate                              *
 *                                                                            *
 *                                                                            *
 ******************************************************************************/
int sr_compute_daq_times( double bufTime0, int tsStd, int timeMethod, int StartEnd )
{
    int    i, imatch, isamp;
    double obcDiff, sampleDiff, *lastTime, *setTime;

    if ( Debug >= 6 )  logit( "", "Starting sr_compute_daq_times\n" );


   /* Initialize variables for  default computation
    ***********************************************/
    if ( StartEnd == COMPUTE_START )
    {
       lastTime = AtodEndTimeLast;
       setTime  = AtodStartTime;
       isamp    = 1;
    }
    else if ( StartEnd == COMPUTE_END )
    {
       lastTime = AtodStartTime;
       setTime  = AtodEndTime;
       isamp    = AtodNumSamples - 1;
    }
    else
       return( 0 );

   /* Default to same time as DAQ 0, if not using OBC
    *************************************************/
    if ( timeMethod != TIME_METHOD_OBC )
    {
       for ( i = 1 ; i < AtodNumBoards ; i++ )
          setTime[i] = lastTime[i] + isamp / AtodSps;

       return( 1 );
    }



   /* Compute time of DAQ system N by correcting time from DAQ system 0
    *******************************************************************/
    for ( i = 1 ; i < AtodNumBoards ; i++ )
    {

      /* Find index of timestamp containing the PpsEvent of interest
       *************************************************************/
       if ( !sr_ts_match_ppsevent( TimeStamp[0][tsStd].PpsEvent, i, &imatch ) )
       {
          setTime[i] = lastTime[i] + isamp / AtodSps;
          logit( "", "Could not find matching PPS event for DAQ #%d, using default time\n", i );
          continue;
       }

       obcDiff    = (double)(TimeStamp[i][imatch].CountObc - TimeStamp[0][tsStd].CountObc);
       sampleDiff = (double)(TimeStamp[i][imatch].Sample   - TimeStamp[0][tsStd].Sample);

       setTime[i] = bufTime0 + (obcDiff / OBC_FREQ) - (sampleDiff / AtodSps);

       if ( Debug >= 5 )
       {
           logit( "", "Called sr_compute_daq_times for DAQ #%d %lf, %d\n",
                  i, bufTime0, tsStd );
           logit( "", "PpsEvent is %d and matches index %d\n",
                  TimeStamp[0][tsStd].PpsEvent, imatch );
           logit( "", "Counts  are #%d 0x%X and #0 0x%X, Diff %lf\n",
                  i, TimeStamp[i][imatch].CountObc, TimeStamp[0][tsStd].CountObc, obcDiff );
           logit( "", "Samples are #%d %d and #0 %d, Diff %lf\n",
                  i, TimeStamp[i][imatch].Sample, TimeStamp[0][tsStd].Sample, sampleDiff );
           logit( "", "obcdiff/freq %lf - samplediff/sps %lf = %lf\n",
                  (obcDiff / OBC_FREQ), (sampleDiff / AtodSps), ((obcDiff / OBC_FREQ) - (sampleDiff / AtodSps)) );
           logit( "", "Computed setTime of %lf from bufTime0 of %lf and Sps %lf\n",
                  setTime[i], bufTime0, AtodSps );

       }

    } /* end for i < AtodNumBoards */

    if ( Debug >= 6 )  logit( "", "Leaving  sr_compute_daq_times\n" );
    return( 1 );
}

/******************************************************************************
 *  Function: sr_ts_match_ppsevent                                            *
 *  Purpose:  Find index of timestamp for specified PpsEvent number.          *
 *            Returns 1 on success, 0 on error and the index in parameter     *
 *            Match.                                                          *
 ******************************************************************************/
int sr_ts_match_ppsevent( int ppsEvent, int iboard, int *match )
{
    int i;

    if ( Debug >= 5 )  logit( "", "Starting sr_ts_match_ppsevent for Pps %d\n", ppsEvent );

    if (!match)
        return( 0 );

    *match = 0;


   /* Loop through specified DAQ system timestamps looking for a match
    ******************************************************************/
    for ( i = 0 ; i < MAX_TS ; i++ )
    {
        if ( TimeStamp[iboard][i].PpsEvent == (unsigned int)ppsEvent )
        {
            *match = i;
            if ( Debug >= 5 )  logit( "", "Leaving  sr_ts_match_ppsevent match %d\n", i );
            return( 1 );
        }
    }

    if ( Debug >= 5 )  logit( "", "Leaving  sr_ts_match_ppsevent without match\n" );
    return( 0 );
}

/******************************************************************************
 *  Function: sr_ts_select                                                    *
 *  Purpose:  To select which entries in the global list of available         *
 *            timestamp structures bound the target point.  These two can     *
 *            then be used to interpolate the value of time at the target.    *
 *            This function assumes that the timestamp list is filled in with *
 *            valid entries starting from the end, and that the sample number *
 *            for valid timestamps always increases from beginning to end.    *
 ******************************************************************************/
int sr_ts_select( long target, int valid, int *lowerTs, int *upperTs )
{
    int i, ret, tslow, tshigh, firstValid;

    if ( Debug >= 6 )  logit( "", "Starting sr_ts_select\n" );

   /* Locate first valid timestamp
    ******************************/
    firstValid = -1;
    for ( i = 0 ; i < MAX_TS ; i++ )
    {
        if ( (TimeStamp[0][i].Valid & valid) == valid )
        {
            firstValid = i;
            if ( Debug >= 6 )
                 logit( "", "First valid timestamp at %d\n", firstValid );
            break;
        }
    }



    /* Error check validity
     **********************/
    if ( firstValid == -1 )
    {
        if ( Debug >= 6 )  logit( "", "No valid timestamps yet\n");
        tslow  = -1;
        tshigh = -1;
        ret    = 0;
    }


   /* Error check target too small
    ******************************/
    else if ( target < TimeStamp[0][firstValid].Sample )
    {
        if ( Debug >= 6 )  logit( "", "Target %ld out of range below TS%d %ld\n",
                 target, firstValid, TimeStamp[0][firstValid].Sample );
        tslow  = -1;
        tshigh = -1;
        ret    = 0;
    }


   /* Error check target too large
    ******************************/
    else if ( target > TimeStamp[0][MAX_TS-1].Sample )
    {
        if ( Debug >= 6 )
           logit( "", "Target %ld out of range above TS%d %ld\n",
                  target, (MAX_TS-1), TimeStamp[0][MAX_TS-1].Sample );
        tslow  = -1;
        tshigh = -1;
        ret    = 0;
    }


   /* Also check last point in last interval
    ****************************************/
    else if ( target == TimeStamp[0][MAX_TS-1].Sample )
    {
        if ( Debug >= 6 )
           logit( "", "Target %ld squeaked in as last point of TS%d of %ld\n",
                  target, MAX_TS-1, TimeStamp[0][MAX_TS-1].Sample);
        tslow  = MAX_TS-2;
        tshigh = MAX_TS-1;
        ret    = 1;
    }


   /* Now check each interval in the list
    *************************************/
    else
    {
        for ( i = 0 ; i < MAX_TS-1 ; i++ )
        {
            if ( Debug >= 7 )
               logit( "", "Checking for %ld between positions %d and %d\n", target, i, i+1 );
            if ( (target >= TimeStamp[0][i].Sample) && (target < TimeStamp[0][i+1].Sample) )
            {
               if ( Debug >= 6 )
                  logit( "", "Target %ld between TS%d of %ld and TS%d of %ld\n",
                          target, i, TimeStamp[0][i].Sample, (i+1), TimeStamp[0][i+1].Sample);
               tslow  = i;
               tshigh = i+1;
               ret    = 1;
               break;
            }
        } /* end for i */

    } /* end else */



   /* Fill return variables
    ***********************/
    if (lowerTs)  *lowerTs = tslow;
    if (upperTs)  *upperTs = tshigh;

   if ( Debug >= 6 )  logit( "", "tslow  %d, *lowerTs %d\n", tslow, *lowerTs );
   if ( Debug >= 6 )  logit( "", "tshigh %d, *upperTs %d\n", tshigh, *upperTs );

   if ( Debug >= 6 )  logit( "", "Leaving  sr_ts_select\n" );
   return( ret );
}


/******************************************************************************
 *  Function: sr_atod_stop                                                    *
 *  Purpose:  Stop the PARxCH DAQ and PARGPS timing module from acquiring.    *
 ******************************************************************************/
void sr_atod_stop( void )
{
   int i;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_stop\n" );


  /* Stop ParXch DAQs
   ******************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
      if ( ParXchHandle[i] != BAD_DEVHANDLE )
         ParXchStop( ParXchHandle[i] );



  /* If using ParGps, stop it
   **************************/
   if ( UsingGps && ParGpsHandle != BAD_DEVHANDLE  )
      ParGpsStop( ParGpsHandle );


   if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_stop\n" );
   return;
}


/******************************************************************************
 *  Function: sr_atod_close                                                   *
 *  Purpose:  Perform a clean and orderly shutdown of the PARxCH DAQ and      *
 *            optional PARGPS timing unit.                                    *
 ******************************************************************************/
void sr_atod_close( void )
{
   int i;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_close\n" );

   /* Stop ParXch DAQ and PARGPS
    ****************************/
    sr_atod_stop();



   /* If using ParGps, disconnect it from ParXch driver, and close it
    *****************************************************************/
    if ( UsingGps                         &&
         ParXchHandle[0] != BAD_DEVHANDLE &&
         ParGpsHandle    != BAD_DEVHANDLE )
    {
        ParXchReleaseGps( ParXchHandle[0] );
        ParGpsClose( ParGpsHandle );
        ParGpsHandle = BAD_DEVHANDLE;
    }



  /* Close ParXch DAQs
   *******************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
      if ( ParXchHandle[i] != BAD_DEVHANDLE )
      {
         ParXchClose( ParXchHandle[i] );
         ParXchHandle[i] = BAD_DEVHANDLE;
      }


   if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_close\n" );
   return;
}

/******************************************************************************
 *  Function: sr_atod_restart_recovery                                        *
 *  Purpose:  After an overflow, future data would be distorted.  So, we      *
 *            throw away any existing data in the FIFO then stop and restart  *
 *            the DAQ.  This ensures that future data is good.                *
 *                                                                            *
 *            There are also a number of other conditions involving GPS that  *
 *            could lead to corrupted time data and would benefit from a      *
 *            restart.  For example if a PPS interrupt arrives, but the       *
 *            following DREADY interrupt does not.  Or if PPS is lost         *
 *            entirely for a long time and then comes back after the system   *
 *            has been running from the A/D or Trimble clock for so long that *
 *            it has drifted off true GPS time.                               *
 *                                                                            *
 *            Send some earthworm heartbeats since this restart will take     *
 *            about two seconds and make us miss at least one pass through    *
 *            the main loop.                                                  *
 *                                                                            *
 *            Rather than simply doing stop/start, we go through a complete   *
 *            close/open cycle first to ensure the PARxCH are in HALT_UDIO    *
 *            mode which allows access to the user digital io before start    *
              is called.  This is required as the digital io is needed for    *
 *            synchronizing the start.                                        *
 ******************************************************************************/
void sr_atod_restart_recovery( void )
{
   if ( Debug >= 3 )  logit( "", "Starting sr_atod_restart_recovery\n" );

   hrtime_ew( &RestartBegin );
   sr_atod_close();                  sr_heartbeat(); /* Close first calls sr_atod_stop */
   sr_atod_open();                   sr_heartbeat();
   sr_atod_start( 0, TIME_INIT_NO ); sr_heartbeat();
   sr_atod_getdata();                sr_heartbeat(); /* Get a starting buffer of data */

   if ( Debug >= 3 )  logit( "", "Leaving  sr_atod_restart_recovery\n" );
   return;
}

/******************************************************************************
 *  Function: sr_check_terminate()                                            *
 *  Purpose:  Check to see if this earthworm ring is requesting a termination *
 *            and exit if it is.                                              *
 ******************************************************************************/
void sr_check_terminate( void )
{
    if ( Debug >= 7 )  logit( "", "Starting sr_check_terminate\n" );


   /* Check for EW terminate request
    ********************************/
    if ( tport_getflag( &Region ) == TERMINATE ||
         tport_getflag( &Region ) == MyPid     )
    {
        sr_terminate();
    }

    if ( Debug >= 7 )  logit( "", "Leaving  sr_check_terminate\n" );
    return;
}


/******************************************************************************
 *  Function: sr_terminate                                                    *
 *  Purpose:  Do the earthworm and DAQ exit tasks.  These include detaching   *
 *            from the earthworm ring and stopping and closing the DAQ.       *
 ******************************************************************************/
void sr_terminate( void )
{
    if ( Debug >= 6 )  logit( "", "Starting sr_terminate\n" );

   /* Clean exit for earthworm
    **************************/
    if ( Region.addr != NULL )
        tport_detach( &Region );

   /* Clean exit for DAQ
    ********************/
    sr_atod_close();

   /* Notify user
    *************/
    logit( "t", "srpar2ew: Termination requested; exiting!\n" );
    fflush( stdout );

   /* Bye bye
    *********/
    exit( 0 );
}


/******************************************************************************
 *  Function: sr_verify_string                                                *
 *  Purpose:  Verify that provided string matches the values allowed for the  *
 *            specified type and return the corresponding integer key.        *
 ******************************************************************************/

#define VERIFY_ONOFF            1
#define VERIFY_ATODXCHMODEL     2
#define VERIFY_ATODPORTMODE     3
#define VERIFY_GPSMODEL         4
#define MAX_VERIFY              5

int sr_verify_string( int type, char *str, int *key )
{
   int result, keyvalue;

   if ( Debug >= 6 )  logit( "", "Starting sr_verify_string\n" );


  /* Quick exit if no string given
   *******************************/
   if (!str)
   {
       if ( Debug >= 6 )  logit( "", "Leaving  sr_verify_string because string not given\n" );
       return( 0 );
   }


   keyvalue = -1;
   result   = 1;

  /* Verify provided string and return key value
   *********************************************/
   switch ( type )
   {
      /* Check for ON or OFF
       *********************/
       case VERIFY_ONOFF:
           if ( Debug >= 6 )  logit( "", "Verifying On/Off\n" );
           if (      strcmp( "ON", str ) == 0 )
               keyvalue = 1;
           else if ( strcmp( "OFF", str ) == 0 )
               keyvalue = 0;
           else
               result = 0;
           break;


      /* Check for PARxCH model
       ************************/
       case VERIFY_ATODXCHMODEL:
           if ( Debug >= 6 )  logit( "", "Verifying PARxCH model\n" );
           if (      strcmp( str, "PAR1CH" ) == 0 )
               keyvalue = PARXCH_MODEL_PAR1CH;
           else if ( strcmp( str, "PAR4CH" ) == 0 )
               keyvalue = PARXCH_MODEL_PAR4CH;
           else if ( strcmp( str, "PAR8CH" ) == 0 )
               keyvalue = PARXCH_MODEL_PAR8CH;
           else
               result = 0;
           break;


      /* Check for PARxCH parallel port mode
       *************************************/
       case VERIFY_ATODPORTMODE:
           if ( Debug >= 6 )  logit( "", "Verifying parallel port mode\n" );
           if (      strcmp( str, "EPP" )     == 0 )
               keyvalue = PARXCH_PORT_MODE_EPP;
           else if ( strcmp( str, "BPP" )     == 0 )
               keyvalue = PARXCH_PORT_MODE_BPP;
           else if ( strcmp( str, "ECP/EPP" ) == 0 )
               keyvalue = PARXCH_PORT_MODE_ECP_EPP;
           else if ( strcmp( str, "ECP/BPP" ) == 0 )
               keyvalue = PARXCH_PORT_MODE_ECP_BPP;
           else
               result = 0;
           break;


      /* Check for PARGPS model
       ************************/
       case VERIFY_GPSMODEL:
           if ( Debug >= 6 )  logit( "", "Verifying GPS model\n" );
           if ( strcmp( str, "GARMIN" ) == 0 )
               keyvalue = GPSMODEL_GARMIN;
           else if ( strcmp( str, "TRIMBLE" ) == 0  ||
                     strcmp( str, "PARGPS"  ) == 0 )
               keyvalue = GPSMODEL_TRIMBLE;
           else if ( strcmp( str, "ONCORE" ) == 0 )
               keyvalue = GPSMODEL_ONCORE;
           else if ( strcmp( str, "PCTIME" ) == 0 )
               keyvalue = GPSMODEL_PCTIME;
           else
               result = 0;
           break;


      /* Default is to succeed with key value 0
       ****************************************/
       default:
           if ( Debug >= 6 )  logit( "", "Verify default\n" );
           keyvalue = 0;
           result   = 1;
   }

   if (result && key)
       *key = keyvalue;

   if ( Debug >= 6 )  logit( "", "Leaving  sr_verify_string\n" );

   return( result );
}

/******************************************************************************
 *  Function: sr_read_config                                                  *
 *  Purpose:  Process command file(s) using kom.c functions;                  *
 *            exits if any errors are encountered.                            *
 ******************************************************************************/

#define CMD_MODULEID         0
#define CMD_RINGNAME         1
#define CMD_LOGFILE          2
#define CMD_HEARTBEAT        3
#define CMD_ATODDRIVER       4
#define CMD_ATODXCHMODEL     5
#define CMD_ATODPORTMODE     6
#define CMD_ATODSPS          7
#define CMD_USINGGPS         8
#define CMD_GPSDRIVER        9
#define CMD_GPSSERIALPORT   10
#define MAX_CMD             11

#define OPT_SUMMARYINTERVAL  0
#define OPT_LOCKINTERVAL     1
#define OPT_LOCKLIMIT        2
#define OPT_OUTPUTMSGTYPE    3
#define OPT_CHANNEL          4
#define OPT_CHANNELSCNL      5
#define OPT_GPSMODEL         6
#define OPT_ATODDRIVER       7
#define OPT_ATODXCHMODEL     8
#define OPT_ATODPORTMODE     9
#define MAX_OPT             10

void sr_read_config( char *configfile )
{
   char  init[MAX_CMD];    /* init flags, one byte for each required command */
   char *cmdstr[MAX_CMD];  /* pointer to required command string to match    */
   char *optstr[MAX_OPT];  /* pointer to optional command string to match    */
   int   nmiss;            /* number of required commands that were missed   */
   char *com;
   char *str;
   int   nfiles;
   int   success;
   int   i;
   int   chan;
   int   rc;
   int   is_scnl;
   int   firstwarn;

   if ( Debug >= 6 )  logit( "", "Starting sr_read_config\n" );


  /* Set optional command defaults
   *******************************/
   SummaryReportInterval = 0;
   GpsLockReportInterval = 0;
   GpsLockBadLimit       = 1;
   sr_strncpy( GpsModelName,      "TRIMBLE", MAX_STR );

   for ( i = 1 ; i < MAX_BOARD ; i++ )
   {
      sr_strncpy( AtodDriverName[i],   "NONE",    MAX_STR );
      sr_strncpy( AtodXchModelName[i], "PAR8CH",  MAX_STR );
      sr_strncpy( AtodPortModeName[i], "ECP/BPP", MAX_STR );
   }

   if ( TRACE2_OK )
      OutputMsgType = -2; /* Set to TypeTraceBuf2 when defined */
   else
      OutputMsgType = -1; /* Set to TypeTraceBuf  when defined */

   for ( i = 0 ; i < MAX_CHAN*MAX_BOARD ; i++ )
   {
       sprintf( ChanList[i].sta, "CH%02d", i );
       sprintf( ChanList[i].comp, "xxx" );
       sprintf( ChanList[i].net, "SR" );
       sprintf( ChanList[i].loc, LOC_NULL_STRING );
       ChanList[i].ver[0] = TRACE2_VERSION0;
       ChanList[i].ver[1] = TRACE2_VERSION1;
       ChanList[i].pin = i;
   }

   AtodNumBoards = 0;
   firstwarn     = 1;


   /* These are the command strings to match
    ****************************************/
    cmdstr[CMD_MODULEID]        = "MyModuleId";
    cmdstr[CMD_RINGNAME]        = "RingName";
    cmdstr[CMD_LOGFILE]         = "LogFile";
    cmdstr[CMD_HEARTBEAT]       = "HeartBeatInterval";
    cmdstr[CMD_ATODDRIVER]      = "AtodDriverName";
    cmdstr[CMD_ATODXCHMODEL]    = "AtodModelName";
    cmdstr[CMD_ATODPORTMODE]    = "PortMode";
    cmdstr[CMD_ATODSPS]         = "SamplingRate";
    cmdstr[CMD_USINGGPS]        = "GpsEnable";
    cmdstr[CMD_GPSDRIVER]       = "GpsDriverName";
    cmdstr[CMD_GPSSERIALPORT]   = "GpsSerialPort";

    optstr[OPT_SUMMARYINTERVAL] = "SummaryInterval";
    optstr[OPT_LOCKINTERVAL]    = "GpsReportInterval";
    optstr[OPT_LOCKLIMIT]       = "GpsBadLimit";
    optstr[OPT_GPSMODEL]        = "GpsModelName";
    optstr[OPT_OUTPUTMSGTYPE]   = "OutputMsgType";
    optstr[OPT_CHANNEL]         = "EwChannel";
    optstr[OPT_CHANNELSCNL]     = "EwChannelScnl";
    optstr[OPT_ATODDRIVER]      = "AtodDriverNameN";
    optstr[OPT_ATODXCHMODEL]    = "AtodModelNameN";
    optstr[OPT_ATODPORTMODE]    = "PortModeN";



   /* Zero out one init flag for each required command
    **************************************************/
    for ( i = 0 ; i < MAX_CMD ; i++ )
        init[i] = 0;


   /* Open the main configuration file
    **********************************/
    nfiles = k_open( configfile );
    if ( nfiles == 0 )
    {
        logit( "e",
                "srpar2ew: Error opening command file <%s>; exiting!\n",
                 configfile );
        exit( -1 );
    }


   /* Process all command files
    ***************************/
    while ( nfiles > 0 )   /* While there are command files open */
    {
        while ( k_rd() )        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

           /* Ignore blank lines & comments
            *******************************/
            if ( !com )           continue;  /* Ignore blank lines            */
            if ( com[0] == '#' )  continue;  /* Ignore comment lines          */
            if ( com[0] == ';' )  continue;  /* Ignore .ini style comment too */


           /* Open a nested configuration file
            **********************************/
            if ( com[0] == '@' )
            {
               success = nfiles+1;
               nfiles  = k_open( &com[1] );
               if ( nfiles != success )
               {
                  logit( "e",
                          "srpar2ew: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

           /* Process anything else as a command
            ************************************/
  /*=*/     if ( k_its( "=" ) )
            {
                ; /* skip = so .ini style files can be used */
            }
/*Dbg*/     if ( k_its( "Debug" ) )
            {
                Debug = k_int();
            }
  /*0*/     else if ( k_its( cmdstr[CMD_MODULEID] ) )
            {
                str = k_str();
                if (str) sr_strncpy( MyModName, str, MAX_MOD_STR );
                init[CMD_MODULEID] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",cmdstr[CMD_MODULEID], MyModName );
            }
  /*1*/     else if ( k_its( cmdstr[CMD_RINGNAME] ) )
            {
                str = k_str();
                if (str) sr_strncpy( RingName, str, MAX_RING_STR );
                init[CMD_RINGNAME] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",cmdstr[CMD_RINGNAME], RingName );
            }
  /*2*/     else if ( k_its( cmdstr[CMD_LOGFILE] ) )
            {
                LogSwitch = k_int();
                init[CMD_LOGFILE] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %d\n",cmdstr[CMD_LOGFILE], LogSwitch );
            }
  /*3*/     else if ( k_its( cmdstr[CMD_HEARTBEAT] ) )
            {
                HeartBeatInterval = k_long();
                init[CMD_HEARTBEAT] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %ld\n",cmdstr[CMD_HEARTBEAT], HeartBeatInterval );
            }
  /*4*/     else if ( k_its( cmdstr[CMD_ATODDRIVER] ) )
            {
                str = k_str();
                if (str) sr_strncpy( AtodDriverName[0], str, MAX_STR );
                init[CMD_ATODDRIVER] = 1;
                AtodNumBoards++;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",cmdstr[CMD_ATODDRIVER], AtodDriverName[0] );
            }
  /*5*/     else if ( k_its( cmdstr[CMD_ATODXCHMODEL] ) )
            {
                str = k_str();
                if ( sr_verify_string( VERIFY_ATODXCHMODEL, str, &AtodXchModel[0] ) )
                {
                    init[CMD_ATODXCHMODEL] = 1;
                    sr_strncpy( AtodXchModelName[0], str, MAX_STR );
                }
                else
                    init[CMD_ATODXCHMODEL] = 0;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",
                                         cmdstr[CMD_ATODXCHMODEL],
                                         AtodXchModelName[0] );
            }
  /*6*/     else if ( k_its( cmdstr[CMD_ATODPORTMODE] ) )
            {
                str = k_str();
                if ( sr_verify_string( VERIFY_ATODPORTMODE, str, &AtodPortMode[0] ) )
                {
                    init[CMD_ATODPORTMODE] = 1;
                    sr_strncpy( AtodPortModeName[0], str, MAX_STR );
                }
                else
                    init[CMD_ATODPORTMODE] = 0;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",cmdstr[CMD_ATODPORTMODE], AtodPortModeName[0] );
            }
  /*7*/     else if ( k_its( cmdstr[CMD_ATODSPS] ) )
            {
                AtodRequestedSps = k_val();
                init[CMD_ATODSPS] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %lf\n",cmdstr[CMD_ATODSPS], AtodRequestedSps );
            }
  /*8*/     else if ( k_its( cmdstr[CMD_USINGGPS] ) )
            {
                str = k_str();
                if ( sr_verify_string( VERIFY_ONOFF, str, &UsingGps ) )
                    init[CMD_USINGGPS] = 1;
                else
                    init[CMD_USINGGPS] = 0;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %d\n",cmdstr[CMD_USINGGPS], UsingGps );
            }
  /*9*/     else if ( k_its( cmdstr[CMD_GPSDRIVER] ) )
            {
                str = k_str();
                if (str) sr_strncpy( GpsDriverName, str, MAX_STR );
                init[CMD_GPSDRIVER] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",cmdstr[CMD_GPSDRIVER], GpsDriverName );
            }
 /*10*/     else if ( k_its( cmdstr[CMD_GPSSERIALPORT] ) )
            {
                GpsSerialPort = k_int();
                init[CMD_GPSSERIALPORT] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %d\n",cmdstr[CMD_GPSSERIALPORT], GpsSerialPort );
            }


          /* Get optional commands
           ***********************/
            else if ( k_its( optstr[OPT_SUMMARYINTERVAL] ) )
            {
                SummaryReportInterval = k_long();

                if ( Debug >= 2 )  logit( "", "Found <%s> value %ld\n",optstr[OPT_SUMMARYINTERVAL], SummaryReportInterval );
            }
            else if ( k_its( optstr[OPT_LOCKINTERVAL] ) )
            {
                GpsLockReportInterval = k_long();

                if ( Debug >= 2 )  logit( "", "Found <%s> value %ld\n",optstr[OPT_LOCKINTERVAL], GpsLockReportInterval );
            }
            else if ( k_its( optstr[OPT_LOCKLIMIT] ) )
            {
                GpsLockBadLimit = k_long();

                if ( Debug >= 2 )  logit( "", "Found <%s> value %ld\n",optstr[OPT_LOCKLIMIT], GpsLockBadLimit );
            }
            else if ( k_its( optstr[OPT_GPSMODEL] ) )
            {
                str = k_str();
                if ( sr_verify_string( VERIFY_GPSMODEL, str, &GpsModel ) )
                    sr_strncpy( GpsModelName, str, MAX_STR );
                else
                    logit( "e", "srpar2ew: <%s> value %s not allowed, using default of %s instead.\n",
                            optstr[OPT_GPSMODEL], str, GpsModelName );

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",optstr[OPT_GPSMODEL], GpsModelName );
            }
            else if ( k_its( optstr[OPT_ATODDRIVER] ) )
            {
                AtodNumBoards++;
                str = k_str();
                if (str) sr_strncpy( AtodDriverName[AtodNumBoards-1], str, MAX_STR );

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",optstr[OPT_ATODDRIVER], AtodDriverName[AtodNumBoards-1] );
            }
            else if ( k_its( optstr[OPT_ATODXCHMODEL] ) )
            {
                str = k_str();
                if ( sr_verify_string( VERIFY_ATODXCHMODEL, str, &AtodXchModel[AtodNumBoards-1] ) )
                    sr_strncpy( AtodXchModelName[AtodNumBoards-1], str, MAX_STR );
                else
                    logit( "e", "srpar2ew: <%s> value %s not allowed, using default of %s instead.\n",
                            optstr[OPT_ATODXCHMODEL], str, AtodXchModelName[AtodNumBoards-1] );

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",
                                         optstr[OPT_ATODXCHMODEL],
                                         AtodXchModelName[AtodNumBoards-1] );
            }
            else if ( k_its( optstr[OPT_ATODPORTMODE] ) )
            {
                str = k_str();
                if ( sr_verify_string( VERIFY_ATODPORTMODE, str, &AtodPortMode[AtodNumBoards-1] ) )
                    sr_strncpy( AtodPortModeName[AtodNumBoards-1], str, MAX_STR );
                else
                    logit( "e", "srpar2ew: <%s> value %s not allowed, using default of %s instead.\n",
                            optstr[OPT_ATODPORTMODE], str, AtodPortModeName[AtodNumBoards-1] );

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",optstr[OPT_ATODPORTMODE], AtodPortModeName[AtodNumBoards-1] );
            }
            else if ( k_its( optstr[OPT_OUTPUTMSGTYPE] ) )
            {
                str = k_str();
                if ( strcmp( "TYPE_TRACEBUF", str ) == 0 )
                    OutputMsgType = -1;      /* Set to TypeTraceBuf when defined */
                else if ( strcmp( "TYPE_TRACEBUF2", str ) == 0 )
                {
                    if ( TRACE2_OK )
                        OutputMsgType = -2;  /* Set to TypeTraceBuf2 when defined */
                    else
                        logit( "e", "srpar2ew: <%s> TYPE_TRACEBUF2 not allowed in this version, using TYPE_TRACEBUF instead <%s>.\n",
                                       com, configfile );
                }

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s (%ld)\n",optstr[OPT_OUTPUTMSGTYPE], str, OutputMsgType );
            }


           /* Get the channel list
            **********************/
            else if ( (is_scnl = k_its( optstr[OPT_CHANNELSCNL] ))  ||
                                 k_its( optstr[OPT_CHANNEL]     )   )
            {

            if ( firstwarn && is_scnl && (OutputMsgType == -1) )
            {
               firstwarn = 0;
               logit( "et", "srpar2ew: Warning.  OutputMsgType is old style\n      TYPE_TRACEBUF but channel list is new %s.\n", optstr[OPT_CHANNELSCNL] );
            }

            chan = k_int();                         /* Get channel number */
            if ( chan >= MAX_CHAN*MAX_BOARD || chan < 0 )
            {
               logit( "e", "srpar2ew: Error. Bad channel number (%d) in config file.\n", chan );
               exit( 0 );
            }

            sr_strncpy( ChanList[chan].sta,  k_str(), TRACE_STA_LEN );    /* station   */
            sr_strncpy( ChanList[chan].comp, k_str(), TRACE_CHAN_LEN );   /* component */
            sr_strncpy( ChanList[chan].net,  k_str(), TRACE_NET_LEN );    /* network   */
            if ( is_scnl )
               sr_strncpy( ChanList[chan].loc, k_str(), TRACE2_LOC_LEN ); /* location  */
            ChanList[chan].pin = k_int();                                 /* pin num   */

            rc = k_err();

            if ( rc == -1 )                          /* No more tokens in line     */
               ChanList[chan].pin = chan;            /* Default pin to chan number */

            if ( rc == -2 )                          /* Unreadable pin number      */
            {
               logit( "e", "srpar2ew: Bad pin number for DAQ channel %d\n", chan );
               exit( 0 );
            }
                if ( Debug >= 2 )
                    logit( "", "Found %s %2d Sta=%s Chan=%s Net=%s Loc=%s pin=%d\n",
                       (is_scnl ? optstr[OPT_CHANNELSCNL] : optstr[OPT_CHANNEL]),
                       chan,
                       ChanList[chan].sta,
                       ChanList[chan].comp,
                       ChanList[chan].net,
                       ChanList[chan].loc,
                       ChanList[chan].pin );

         } /* end of if EwChannel or EwChannelScnl */



           /* Unknown command
            *****************/
            else
            {
                logit( "e", "srpar2ew: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

           /* See if there were any errors processing the command
            *****************************************************/
            if ( k_err() )
            {
               logit( "e",
                       "srpar2ew: Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
               exit( -1 );
            }

        } /* end while k_rd */

        nfiles = k_close();

   } /* end while nfiles > 0 */


  /* After all files are closed, check init flags for missed commands
   ******************************************************************/
   nmiss = 0;
   for ( i = 0 ; i < MAX_CMD ; i++ )
   {
       if ( !init[i] )
           nmiss++;
   }
   if ( nmiss )
   {
       logit( "e", "srpar2ew: ERROR, no " );
       if ( !init[CMD_MODULEID] )      logit( "e", "<%s> ", cmdstr[CMD_MODULEID] );
       if ( !init[CMD_RINGNAME] )      logit( "e", "<%s> ", cmdstr[CMD_RINGNAME] );
       if ( !init[CMD_LOGFILE] )       logit( "e", "<%s> ", cmdstr[CMD_LOGFILE] );
       if ( !init[CMD_HEARTBEAT] )     logit( "e", "<%s> ", cmdstr[CMD_HEARTBEAT] );
       if ( !init[CMD_ATODDRIVER] )    logit( "e", "<%s> ", cmdstr[CMD_ATODDRIVER] );
       if ( !init[CMD_ATODXCHMODEL] )  logit( "e", "<%s> ", cmdstr[CMD_ATODXCHMODEL] );
       if ( !init[CMD_ATODPORTMODE] )  logit( "e", "<%s> ", cmdstr[CMD_ATODPORTMODE] );
       if ( !init[CMD_ATODSPS] )       logit( "e", "<%s> ", cmdstr[CMD_ATODSPS] );
       if ( !init[CMD_USINGGPS] )      logit( "e", "<%s> ", cmdstr[CMD_USINGGPS] );
       if ( !init[CMD_GPSDRIVER] )     logit( "e", "<%s> ", cmdstr[CMD_GPSDRIVER] );
       if ( !init[CMD_GPSSERIALPORT] ) logit( "e", "<%s> ", cmdstr[CMD_GPSSERIALPORT] );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   if ( Debug >= 6 )  logit( "", "Leaving  sr_read_config\n" );
   return;
}

/******************************************************************************
 *  Function: sr_lookup_ew                                                    *
 *  Purpose:  Look up important info from earthworm.h tables including        *
 *               RingKey                                                      *
 *               InstId                                                       *
 *               ModId                                                        *
 *               several message types                                        *
 ******************************************************************************/
void sr_lookup_ew( void )
{
   if ( Debug >= 6 )  logit( "", "Starting sr_lookup_ew\n" );

  /* Look up key to shared memory region
   *************************************/
   if ( ( RingKey = GetKey( RingName ) ) == -1 )
   {
       logit( "e",
              "srpar2ew: Invalid ring name <%s>; exiting!\n", RingName );
       exit( -1 );
   }


  /* Look up installation of interest
   **********************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      logit( "e",
             "srpar2ew: Error getting local installation id; exiting!\n" );
      exit( -1 );
   }


  /* Look up module of interest
   ****************************/
   if ( GetModId( MyModName, &MyModId ) != 0 )
   {
      logit( "e",
             "srpar2ew: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }


  /* Look up message types of interest
   ***********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
   {
      logit( "e",
             "srpar2ew: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      logit( "e",
             "srpar2ew: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_TRACEBUF", &TypeTraceBuf ) != 0 )
   {
      logit( "e",
             "srpar2ew: Invalid message type <TYPE_TRACEBUF>; exiting!\n" );
      exit( -1 );
   }

   if ( TRACE2_OK )
   {
      if ( GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 ) != 0 )
      {
         logit( "e",
                "srpar2ew: Invalid message type <TYPE_TRACEBUF2>; exiting!\n" );
         exit( -1 );
      }
   }
   else
   {
      TypeTraceBuf2 = TypeError;
   }


  /* Reset OutputMsgType now that TypeTraceBuf is defined
   ******************************************************/
   if ( OutputMsgType == -1 )
           OutputMsgType = TypeTraceBuf;
   else if ( OutputMsgType == -2 )
           OutputMsgType = TypeTraceBuf2;
   if ( Debug >= 2 )  logit( "", "OutputMsgType reset to %d\n", OutputMsgType );

   if ( Debug >= 6 )  logit( "", "Leaving  sr_lookup_ew\n" );
   return;
}

/******************************************************************************
 *  Function: sr_heartbeat                                                    *
 *  Purpose:  Send a heartbeat message if enough time has elapsed since the   *
 *            last one.  Also toggle the PARxCH yellow LED.  Depending on the *
 *            selected sampling rate, the yellow LED may blink faster or      *
 *            slower than the rate at which heartbeat messages are sent to    *
 *            the earthworm shared memory ring.                               *
 ******************************************************************************/
void sr_heartbeat( void )
{
   long msgLen; /* Length of the heartbeat message */

   if ( Debug >= 6 )  logit( "", "Starting sr_heartbeat\n" );


   /* Send heartbeat message if enough time has elapsed
    ***************************************************/
    if ( time( &TimeNow ) - TimeLastBeat  >=  HeartBeatInterval )
    {
        sprintf( Msg, "%ld %ld\n", (long)TimeNow, (long)MyPid ); /* Fill message string */
        msgLen    = strlen( Msg );                     /* Set message size    */
        Logo.type = TypeHeartBeat;                     /* Set message type    */


        if ( Debug >= 5 ) logit( "", "Heartbeat string (time,pid) = %s", Msg );

        if ( tport_putmsg( &Region, &Logo, msgLen, Msg ) != PUT_OK )
            logit( "et", "srpar2ew:  Error sending heartbeat.\n" );

        TimeLastBeat = TimeNow;                 /* Update last heartbeat time */
    }


  /* Toggle yellow LED
   *******************/
   sr_atod_toggle_led( );


   if ( Debug >= 6 )  logit( "", "Leaving  sr_heartbeat\n" );
   return;
}

/******************************************************************************
 *  Function: sr_atod_toggle_led                                              *
 *  Purpose:  Change the ON/OFF state of the yellow LED on the PARxCH front   *
 *            panel to provide a visible heartbeat and indicate another       *
 *            buffer of data has been read.                                   *
 ******************************************************************************/
void  sr_atod_toggle_led( void )
{
   int i;

   if ( Debug >= 7 )  logit( "", "Starting sr_atod_toggle_led\n" );

   AtodLedValue ^= 1;                            /* twiddle led state */

   for ( i = 0 ; i < AtodNumBoards ; i++ )
      ParXchUserLed( ParXchHandle[i], AtodLedValue );

   if ( Debug >= 7 )  logit( "", "Leaving  sr_atod_toggle_led\n" );
   return;
}

/******************************************************************************
 *  Function: sr_error                                                        *
 *  Purpose:  Build and send an earthworm error message to the shared memory  *
 *            region (ring) and logs it.                                      *
 ******************************************************************************/
void sr_error( int ierr, char *note )
{
   long msgLen;  /* Length of the error message */

   if ( Debug >= 6 )  logit( "", "Starting sr_error\n" );

  /* Build the message
   *******************/
   time( &TimeNow );
   sprintf( ErrMsg, "%ld %d %s\n", (long)TimeNow, ierr, note); /* Fill errmsg str */
   msgLen    = strlen( ErrMsg );                           /* Set errmsg size */
   Logo.type = TypeError;                                  /* Set errmsg type */


  /* Log it and send it to shared memory
   *************************************/
   logit( "et", "srpar2ew: %s\n", note );

   if ( tport_putmsg( &Region, &Logo, msgLen, ErrMsg ) != PUT_OK )
           logit( "et", "srpar2ew:  Error sending error:%d.\n", ierr );

   if ( Debug >= 6 )  logit( "", "Leaving  sr_error\n" );
   return;
}

/******************************************************************************
 *  Function: sr_strncpy                                                      *
 *  Purpose:  Copy a string from src to dest and ensure the destination       *
 *            is null terminated.  Unlike the standard C runtime function of  *
 *            similar name, the length parameter should be the number of      *
 *            characters INCLUDING the terminating null.  For example, if     *
 *            len = 4, and src = "abcdef", then dest = "abc", ie 3 characters *
 *            plus 1 null = 4.                                                *
 ******************************************************************************/
int sr_strncpy( char *dest, char *src, int len )
{
   int   i, ok;
   char *dstart;

   if ( Debug >= 8 )  logit( "", "Starting sr_strncpy\n" );


   ok = 0;
   if ( dest && src && (len > 0) )  /* Error check inputs */
   {

     /* Copy characters from src to dest, stopping after len-1
      * characters or reaching a null at the end of the source string
      ****************************************************************/
      dstart = dest;
      i      = 1;
      while ( (i < len)  &&  (*dest++ = *src++) )
         i++;

     /* If copying stopped because of hitting the character limit, fill
      * the last character with null (it will already be null if the copy
      * stopped because of reaching the end of the source string).
      *******************************************************************/
      if ( i == len )
         *dest = '\0';

      ok = 1;

      if ( Debug >= 8)  logit( "", "src |%s|, dest |%s|\n", src, dstart );

   }

   if ( Debug >= 8) logit( "", "Leaving  sr_strncpy\n" );
   return( ok );
}

/******************************************************************************
 *  Function: sr_log_analog_data                                              *
 *  Purpose:  Display the analog data for all DAQ boards.                     *
 *            It assumes that GPS is being used.  If it is not, the last 1 or *
 *            2 samples which represent the Gps Mark and PAR8CH OBC will      *
 *            contain garbage values.                                         *
 ******************************************************************************/
void sr_log_analog_data( void )
{
   int  i, j, pt;
   char datamsg[256], *markstr, *obcstr;

   if ( Debug >= 6) logit( "", "Starting  sr_log_analog_data\n" );

   pt = 0;

   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
       logit( "", "Data for DAQ #%d:\n", i );
       for ( j = 0 ; j < AtodNumSamples ; j++ )
       {
         /* Prepare string to highlight sample with Gps Mark
          **************************************************/
          if ( AtodMarkChan[i] == INDEXCHANNEL_NONE ||
               AtodBufferFill[pt+AtodMarkChan[i]] == 0 )
             markstr = "  ";
          else
             markstr = "<<";


         /* Prepare string to highlight sample with OBC
          *********************************************/
          if ( AtodCntrChan[i] == INDEXCHANNEL_NONE ||
               AtodBufferFill[pt+AtodCntrChan[i]] == 0 )
             obcstr = "  ";
          else
             obcstr = "oo";


         /* Prepare data string apprpriate for each type of PARxCH
          ********************************************************/
          if ( AtodXchModel[i] == PARXCH_MODEL_PAR8CH )
             sprintf( datamsg,"%3d (%8ld): %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %s%s\n",
                   j, AtodCurrentPtFill+j,
                   AtodBufferFill[pt+0],
                   AtodBufferFill[pt+1],
                   AtodBufferFill[pt+2],
                   AtodBufferFill[pt+3],
                   AtodBufferFill[pt+4],
                   AtodBufferFill[pt+5],
                   AtodBufferFill[pt+6],
                   AtodBufferFill[pt+7],
                   AtodBufferFill[pt+8],
                   AtodBufferFill[pt+9],
                   AtodBufferFill[pt+10],
                   markstr, obcstr
                  );

          else if ( AtodXchModel[i] == PARXCH_MODEL_PAR4CH )
             sprintf( datamsg,"%3d (%8ld): %08lX %08lX %08lX %08lX %08lX %s%s\n",
                   j, AtodCurrentPtFill+j,
                   AtodBufferFill[pt+0],
                   AtodBufferFill[pt+1],
                   AtodBufferFill[pt+2],
                   AtodBufferFill[pt+3],
                   AtodBufferFill[pt+4],
                   markstr, obcstr
                  );

          else if ( AtodXchModel[i] == PARXCH_MODEL_PAR1CH )
             sprintf( datamsg,"%3d (%8ld): %08lX %08lX %s%s\n",
                   j, AtodCurrentPtFill+j,
                   AtodBufferFill[pt+0],
                   AtodBufferFill[pt+1],
                   markstr, obcstr
                  );

           logit( "", "%s", datamsg );


           pt += AtodNumChannels[i];

       } /* for j < AtodNumSamples */

    } /* for i < AtodNumBoards */


   if ( Debug >= 6 ) logit( "", "Leaving  sr_log_analog_data\n" );
   return;
}

/******************************************************************************
 *  Function: sr_log_gap_details                                              *
 *  Purpose:  Gaps in the data can cause problems for downstream programs.    *
 *            If one occurs, we want to log lots of details about it so we    *
 *            track it down and eliminate it in the future.                   *
 ******************************************************************************/
void sr_log_gap_details( int thisMethod, double expectedStart, double atodEndTime  )
{
   int     i, j, k, ok1, ok2;
   int     tsCurrLow, tsCurrHigh, tsEndLow, tsEndHigh;
   long    atodEndPt;
   double  diff[MAX_TS], des, dse, dss;

   if ( Debug >= 6) logit( "", "Starting  sr_log_gap_details\n" );

   if ( Debug >= 3 )
   {

      logit( "", "Gap Details:\n" );


      for ( i = 0 ; i < MAX_PREV_SER ; i++ )
      {
         logit( "", "  Serial %d: Valid 0x%X, NmeaCount %d, PpsEvent %8d\n",
                i,
                GpsPrevSerial[i].ValidFields,
                GpsPrevSerial[i].NmeaCount,
                GpsPrevSerial[i].PpsEventNum
               );
          for ( j = 0 ; j < GpsPrevSerial[i].NmeaCount ; j++ )
          {
             k = MAX_NMEA_SIZE * j;
             logit( "", "     NMEA %d: %s", j, &GpsPrevSerial[i].NmeaMsg[k] );
           } /* End for j */

      } /* End for i */

      atodEndPt  = AtodCurrentPtWrite + (AtodNumSamples-1);
      logit( "", "  Current StartPt = %ld, EndPt = %ld\n",
              AtodCurrentPtWrite, atodEndPt );

      ok1 = sr_ts_select( AtodCurrentPtWrite, TsValid, &tsCurrLow, &tsCurrHigh );
      ok2 = sr_ts_select( atodEndPt,          TsValid, &tsEndLow,  &tsEndHigh );
      logit( "", "  TS Start ok = %d, Low = %d, High = %d\n", ok1, tsCurrLow, tsCurrHigh );
      logit( "", "  TS End   ok = %d, Low = %d, High = %d\n", ok1, tsEndLow, tsEndHigh );


      logit( "", "  Ts# Valid  Pps   Samp     CountAtP             CountAtD            PctimeAtP             PctimeAtD        YmdSrc  HmsSrc   SecSince1970\n");

      for ( i = 0 ; i < MAX_TS ; i++ )
      {
         logit( "", "  %d   0x%X  %5u  %5ld   0x%08lX %08lX  0x%08lX %08lX  0x%08lX %08lX   0x%08lX %08lX   %2d    %2d     %lf\n",
                 i, TimeStamp[0][i].Valid, TimeStamp[0][i].PpsEvent, TimeStamp[0][i].Sample,
                 TimeStamp[0][i].CountAtPps.u.HighPart,     TimeStamp[0][i].CountAtPps.u.LowPart,
                 TimeStamp[0][i].CountAtDready.u.HighPart,  TimeStamp[0][i].CountAtDready.u.LowPart,
                 TimeStamp[0][i].PctimeAtPps.u.HighPart,    TimeStamp[0][i].PctimeAtPps.u.LowPart,
                 TimeStamp[0][i].PctimeAtDready.u.HighPart, TimeStamp[0][i].PctimeAtDready.u.LowPart,
                 TimeStamp[0][i].YmdSource, TimeStamp[0][i].HmsSource, TimeStamp[0][i].SecSince1970 );
           ParGpsLargeIntDiffFull( TimeStamp[0][i].CountAtDready, TimeStamp[0][i].CountAtPps,
                                   NULL, &diff[i] );
      }

      logit( "", "  (D-P = %lf %lf %lf %lf %lf)\n", diff[0], diff[1], diff[2], diff[3], diff[4] );

      for ( i = 0 ; i < MAX_TS-1 ; i++ )
      {
         ParGpsLargeIntDiffFull( TimeStamp[0][i+1].CountAtPps, TimeStamp[0][i].CountAtPps,
                                 NULL, &diff[i] );
      }
      logit( "", "  (P-P = %lf %lf %lf %lf)\n", diff[0], diff[1], diff[2], diff[3] );

      for ( i = 0 ; i < MAX_TS-1 ; i++ )
      {
         ParGpsLargeIntDiffFull( TimeStamp[0][i+1].CountAtDready, TimeStamp[0][i].CountAtDready,
                                 NULL, &diff[i] );
      }
      logit( "", "  (D-D = %lf %lf %lf %lf)\n", diff[0], diff[1], diff[2], diff[3] );

      logit( "", "  This time method %d, last %d\n", thisMethod, LastTimeMethod );
      logit( "", "  Last end %lf, this start %lf, this end %lf\n",
             AtodEndTimeLast[0], AtodStartTime[0], AtodEndTime[0] );

      des =  AtodStartTime[0] - AtodEndTimeLast[0];
      dse =  AtodEndTime[0]   - AtodStartTime[0];
      logit( "", "  Last end to this start %lf, this start to this end %lf\n", des, dse );
      dss =  AtodStartTime[0] - expectedStart;
      logit( "", "  expected start %lf, to this start %lf\n", expectedStart, dss );

   }

   if ( Debug >= 6 ) logit( "", "Leaving  sr_log_gap_details\n" );
   return;
}

/******************************************************************************
 *  Function: sr_log_trace_head                                               *
 *  Purpose:  Output the trace header for debugging purposes.                 *
 *            Currently, not called by any routines.                          *
 ******************************************************************************/
void sr_log_trace_head( void )
{
   if ( Debug >= 6 ) logit( "", "Starting  sr_log_trace_head\n" );

   logit( "", "TraceHeader:\n" );
   logit( "", "  Pinno   %d\n",   TraceHead->pinno );
   logit( "", "  Nsamp   %d\n",   TraceHead->nsamp );
   logit( "", "  Start   %lf\n",  TraceHead->starttime );
   logit( "", "  End     %lf\n",  TraceHead->endtime );
   logit( "", "  Rate    %lf\n",  TraceHead->samprate );
   logit( "", "  Sta     %s\n",   TraceHead->sta );
   logit( "", "  Net     %s\n",   TraceHead->net );
   logit( "", "  Chan    %s\n",   TraceHead->chan );
   logit( "", "  Loc     %s\n",  &TraceHead->chan[TRACE_POS_LOC] );
   logit( "", "  Version %c%c\n", TraceHead->chan[TRACE_POS_VER], TraceHead->chan[TRACE_POS_VER+1] );
   logit( "", "  Type    %s\n",   TraceHead->datatype );
   logit( "", "  Quality %X%X\n", TraceHead->quality[0], TraceHead->quality[1] );
   logit( "", "  Pad     %X%X\n", TraceHead->pad[0], TraceHead->pad[1] );

   if ( Debug >= 6 ) logit( "", "Leaving  sr_log_trace_head\n" );
   return;
}


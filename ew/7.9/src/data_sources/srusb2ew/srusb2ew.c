/* FILE: srusb2ew.c           Copyright (c), Symmetric Research, 2010
 *
 * srusb2ew is a program for sending data from a SR USBxCH 24 bit system 
 * to an Earthworm WAVE_RING buffer as TYPE_TRACEBUF messages.  Currently
 * it works with a single four channel USB4CH.  Future plans include having
 * it work with eight channel USB8CH systems and multiple USBxCH's.  It is
 * similar in concept and organization to the ADSEND module except it works
 * for the SR USBxCH DAQs.
 *
 * The most recent version of srusb2ew can be downloaded for free from
 * the symres.com website as it is included as part of the USBxCH software
 * in the Extras\Earthworm subdirectory.
 *
 *
 * What you need to run:
 *    The Earthworm system installed
 *    A Symmetric Research USBxCH A/D board (GPS antenna recommended too)
 *    The srusb2ew executable (.exe), config (.d) and error (.desc) files
 *    You must change the values in srusb2ew.d so they match your A/D
 *    You will probably also want to change some values in other .d files
 *       like wave_serverV.d or export.d to save or process the acquired data
 *
 *
 * What you need to compile:
 *   The Earthworm include and library files
 *   The srusb2ew source
 *   The USBxCH software source (available free from the symres.com website)
 *
 *
 * Please refer to the readme.txt and srusb2ew_ovr.html files for
 * information about how to run this program.  Also see the sample
 * srusb2ew.d configuration file and srusb2ew_cmd.html file
 * for a discussion of the configuration parameters and their options.
 *
 *
 * How this program is organized:
 *
 * The main routine calls several subroutines to initialize Earthworm,
 * the USBxCH, and some software buffers.  Then it starts the USBxCH
 * and goes into an endless loop reading data and sending out heartbeat
 * and tracebuf messages.  The loop can be terminated by setting the
 * Earthworm terminate flag.
 *
 * srusb2ew tries to acquire roughly 1 second of data at a time.
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
 * The USBxCH initialization involves opening the USBxCH driver,
 * initializing the USBxCH hardware, and setting default values for a
 * variety of acquisition parameters.  Routines with sr_atod in their
 * name are the only ones which call USBxCH hardware specific functions
 * supplied by the USBxCH library that comes with the hardware and is
 * available for free download from the Symmetric Research website,
 * symres.com.  Use of a GPS antenna for timestamping the data is
 * optional but highly recommended.  Otherwise, time from the PC will be
 * used to timestamp the data.  If neither GPS or PCTIME is used, the time
 * assigned to the acquired data will be determined by adding an elapsed
 * time, computed assuming the nominal sampling rate is totally
 * accurate, to the PC time when acquisition started.
 *
 * Two large arrays are allocated during the initialization phase:
 * AtodPacketBuffer and TraceBuffer.  AtodPacketBuffer will be filled
 * with acquired data read from the USBxCH, until it is copied into
 * TraceBuffer which holds the data in Earthworm tracebuf message format
 * until it is written out to the WAVE_RING.
 *
 * The routine sr_atod_getdata takes care of reading in the acquired
 * data as USB packets that include both the analog data and timing info
 * and converting the packets into columns of data where each row
 * represents one sample.  A loop is used for reading in the requested
 * amount of acquired data.  After each pass through the loop, if more
 * data is still needed to fill the request, the program sleeps awhile
 * to let other programs run while it gives the remaining data a chance
 * to arrive.  The sleep time has been set so 5 or 6 passes through the
 * loop should be needed to satisfy the request.  If the data request is
 * satisfied on the very first pass, it means data has accumulated in
 * the FIFO on the USBxCH board faster than it is being read, presumably
 * because srusb2ew is not getting enough CPU cycles to keep up.  While
 * this can be tolerated for a little while, the FIFO will eventually
 * overflow if this situation persists.
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

  Revision 1.0  2010/03/10  W.Tucker:
     first version, modified from PARxCH srparxchewsend.c
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>       /* for _O file open constants                     */
#include <sys/stat.h>    /* for _S file share constants                    */
#include <time.h>

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )
#include <windows.h>
#include <io.h>          /* for low level file disk read/write functions   */
#define OsOpen  _open
#define OsWrite _write
#define OsClose _close
#define OFLAGS (_O_WRONLY|_O_CREAT|_O_TRUNC|_O_BINARY) /* open  flags for pak files */
#define SFLAGS (_S_IWRITE)                             /* share flags for pak files */

#elif defined( SROS_LINUX )
#include <unistd.h>      /* for access, close function and STDIN_FILENO    */
#define OsOpen  open
#define OsWrite write
#define OsClose close
#define OFLAGS (O_WRONLY|O_CREAT|O_TRUNC)                        /* open  flags for pak files */
#define SFLAGS (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) /* share flags for pak files */
#endif  /* SROS_xxxxx */


#include <earthworm.h>   /* inst, mod + ring globals, logit utilities, etc */
#include <kom.h>         /* configuration parsing  k_xxx                   */
#include <time_ew.h>     /* OS independent time functions                  */
#include <transport.h>   /* ring transport utilities  tport_xxx            */
#include <trace_buf.h>   /* tracebuf waveform structure                    */

#include "SrUsbXch.h"    /* USBxCH DAQ defines and library routines        */
#include "SrHelper.h"    /* for os independent mkdir, keyboard support ... */


/* This code is designed to support both v7.x which knows about the new
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

#define MAX_TRACEDAT_SIZ (MAX_TRACEBUF_SIZ-sizeof(TRACE_HEADER))
#define MAX_TRACEDAT_PTS (MAX_TRACEDAT_SIZ/sizeof(long))


/* Prototypes of functions in this file
 **************************************/
int   main( int, char ** );
void  sr_ew_init( char *, char * );
void  sr_atod_init( void );
void  sr_atod_init_model( void );
void  sr_atod_open( void );
void  sr_atod_correct_sps( double * );
void  sr_summary_init( void );
void  sr_allocate_arrays( void );
void  sr_atod_start( int, int );
void  sr_atod_synchronize( void );
void  sr_atod_send_pctimestamp( void );
int   sr_atod_getdata( void );
void  sr_send_traces( void );
void  sr_set_tracebuf_time( int, int );
void  sr_gpslock_update( void );
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
void  sr_log_ts_data( void );
void  sr_log_sample_data( void );
void  sr_log_status_data( void );
void  sr_log_gap_details( int, double, double );
void  sr_log_trace_head( void );
void  sr_pakfile_write_when_full( void );
void  sr_pakfile_write( int );
void  sr_pakfile_mkdir( void );


/* Defines and typedefs
 **********************/
#define REV_STRING  "srusb2ew Rev 2010/04/15\n"
#define MAX_STR      256      /* Maximum dimension for some strings      */
#define MAX_CHAN      11      /* Maximum number of channels allowed      */
#define MAX_BOARD      2      /* Maximum number of DAQ boards allowed    */
#define QUALITY_OK   0x0      /* See trace_buf.h for other codes         */
#define BEHIND_LIMIT  20      /* Num times acquisition falls behind
                                 before printing a warning               */
#define TIME_INIT_NO   0      /* On restart, do not init times in start  */
#define TIME_INIT_YES  1      /* On first start, do init times in start  */

#define EW_ERROR_BASE_USBXCH  100 /* Convert USBxCH_ERROR_XXX to EW ierr */

#define MAXGAPTRAIL    2      /* Dbg: Num buffers to log after a gap     */

#define TOGGLE_MASK    0x01;  /* Pick GPS toggle signal from digital IO  */

#define COMPUTE_START  1      /* compute a start time for 2ndary DAQ     */
#define COMPUTE_END    2      /* compute an end  time for 2ndary DAQ     */

//WCT - Consider making this 3x-5x sampling rate
#define MAX_COLROWS    5000   /* Max num points in data column structure */

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

static char   AtodDriverName[MAX_BOARD][MAX_STR];  /* USBxCH driver name     */
static char   AtodXchModelName[MAX_BOARD][MAX_STR];/* USBxCH model name      */
static char   AtodGpsModelName[MAX_STR];/* GPS model name (eg GARMIN)        */
static double AtodRequestedSps;         /* requested sampling rate           */
static int    UsingGps;                 /* is GPS being used                 */
static int    WritingPak;               /* are SR PAK files written too      */
static int    PacketsPerFile;           /* number of packets per .PAK file   */
static SCNL   ChanList[MAX_BOARD*MAX_CHAN];/* Chan info:site,comp,net,pinno  */
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
static SHM_INFO      Region;            /* shared memory region to use for i/o    */
static MSG_LOGO      Logo;              /* msg header with module,type,instid     */
static pid_t         MyPid;             /* for restarts by startstop              */
static time_t        TimeNow;           /* current time in sec since 1970         */
static time_t        TimeLastBeat;      /* time last heartbeat was sent           */

static char         *TraceBuffer;       /* tracebuf message buffer                */
static long         *TraceDat;          /* where in TraceBuffer the data starts   */
static TRACE_HEADER *TraceHead;         /* where in TraceBuffer old header starts */

static char          YMDHMS[MAX_STR];   /* date style name for pak file directory */
static unsigned long PakFilesWritten;   /* number of .PAK files written so far    */

static SRUSBXCH_PACKET *AtodPacketBuffer; /* pointer to acquired data buffer      */
static int              AtodPacketBufferCount;     /* num data positions filled   */
static int              AtodPacketBufferMaxCount;  /* total data positions        */

static SRUSBXCH_PACKET *FilePacketBuffer;         /* buffer of packets to file   */
static int              FilePacketBufferCount;    /* num positions filled        */
static int              FilePacketBufferMaxCount; /* total num positions         */

static long          AtodNumBoards;     /* number of DAQ boards being used (max2) */
static long          AtodNumChannels[MAX_BOARD];/* channels being acquired/DAQ    */
static long          AtodNumChannelsTotal;   /* total DAQ channels being acquired */
static long          AtodNumSamplesSec;      /* number of data samples per second */
static long          AtodNumSamplesTb;       /* number data samples per tracebuf  */
static long          AtodChanDig[MAX_BOARD]; /* "channel" index of digital data   */
static long          AtodChanPps[MAX_BOARD]; /* "channel" index of PPS toggle     */
static long          AtodChanObc[MAX_BOARD]; /* "channel" index of OBC data       */
static long          AtodChanPwr[MAX_BOARD]; /* "channel" index of power data     */
static long          AtodChanTmp[MAX_BOARD]; /* "channel" index of temperature    */
static double        AtodSps;                /* actual sampling rate              */
static int           AtodLedValue;           /* state of USBxCH yellow LED        */

static int           AtodXchModel[MAX_BOARD]; /* which USBxCH: 4CH, or 8CH        */
static int           AtodGpsModel;            /* which Gps model (eg GARMIN)      */
static double        AtodStartTime[MAX_BOARD];/* start time of next tracebuf      */
static double        AtodEndTime[MAX_BOARD];  /* end time of next tracebuf        */
static double        AtodEndTimeLast[MAX_BOARD];/* end time of last tracebuf      */
static long          AtodCurrentPt;           /* index of current pt since start  */
static unsigned long AtodBufferNum;           /* index numer of current buffer    */
static int           GapTrailer[MAX_BOARD];   /* debug count of buffers after gap */

static DEVHANDLE UsbXchHandle[MAX_BOARD];   /* handle to USBxCH device drivers    */

static int        SleepMs;                  /* Ms to wait for more data to arrive  */
static int        UseTimeMethod;            /* how to compute trace start time     */
static int        LastTimeMethod;           /* how last trace start time computed  */

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



static SRUSBXCH_SAMPLEDATA AtodSamples[MAX_COLROWS];
static int AtodSamplesReady[MAX_BOARD];
static SRUSBXCH_STATUSDATA AtodStatus[MAX_COLROWS];
static int AtodStatusReady[MAX_BOARD];
static TS AtodTimeStamps[2];
static int AtodTimeStampsReady[MAX_BOARD];


/* Shared string for temp use
 ****************************/
static char Msg[MAX_STR];                   /* string for log messages       */
static char ErrMsg[MAX_STR];                /* string for error messages     */






/******************************************************************************
 *  Function: srusb2ew main                                                   *
 *  Purpose:  srusb2ew is an earthworm module for acquiring waveform          *
 *            data from one of the Symmetric Research 24 bit USBxCH A/D (DAQ) *
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
        fprintf( stderr, "Usage: srusb2ew <configfile>\n" );
        exit( 0 );
   }


  /* Earthworm initialization
   **************************/
   sr_ew_init( argv[0], argv[1] );


  /* USBxCH DAQ initialization
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

   while ( 1 )
   {
      sr_check_terminate();         /* Exit if termination requested */

      if ( sr_atod_getdata() > 0 )  /* Read new acquired data        */
      {
         sr_heartbeat();            /* Send heartbeat message        */
         sr_send_traces();          /* Send tracebuf messages        */
      }
      else
      {
         sr_heartbeat();            /* Send heartbeat message        */
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
    for ( i = 0 ; i < MAX_BOARD ; i++ )
       UsbXchHandle[i]  = BAD_DEVHANDLE;
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
     logit( "e", "srusb2ew: Cannot get pid. Exiting.\n" );
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
 *  Purpose:  Initialize the USBxCH DAQ and optional PARGPS timing modules    *
 *            and the parameters related to them.  First open the device      *
 *            drivers and initialize the hardware.  Next set various DAQ      *
 *            parameters.  Some are common to all USBxCH, while others        *
 *            depend on which XchModel is being used.  Finally, initialize    *
 *            the GPS timestamp structures which are used in determining the  *
 *            start time for each tracebuf.                                   *
 ******************************************************************************/
void sr_atod_init( void )
{
   int i;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_init\n" );


  /* Correct requested Sps
   ***********************/
   sr_atod_correct_sps( &AtodRequestedSps );


  /* Open and initialize the USBxCH
   ********************************/
   sr_atod_open();


  /* Set some default DAQ values
   *****************************/
   AtodLedValue         = 0;
   AtodCurrentPt        = 0L;
   AtodBufferNum        = 0L;
   AtodNumSamplesSec    = (long)AtodSps; /* num samples per 1 sec of data */
   

   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
      AtodSamplesReady[i]    = 0;
      AtodStatusReady[i]     = 0;
      AtodTimeStampsReady[i] = 0;
   }

   UseTimeMethod      = SRDAT_TIME_METHOD_NONE;
   LastTimeMethod     = SRDAT_TIME_METHOD_NONE;
   SleepMs            = 1000 / 5;  /* If we're keeping up with the data it */
                                   /* will take 5 tries to get 1 sec worth */




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



   /* Initialize general and GPS logging parameters
    ***********************************************/
   sr_summary_init();

   GpsLockCount = GpsLockGood = GpsLockBad = 0;
   GpsLockStatus = -1;


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
 *  Purpose:  Initialize some USBxCH DAQ parameters that depend on XchModel   *
 *            and GpsModel.  For example, number of channels.                 *
 ******************************************************************************/
void sr_atod_init_model( void )
{
    int i;

    if ( Debug >= 6 )  logit( "", "Starting sr_atod_init_model\n" );


    AtodNumChannelsTotal = 0;

    for ( i = 0 ; i < AtodNumBoards ; i++ )
    {

       AtodChanDig[i] = SRDAT_INDEXCHANNEL_NONE;
       AtodChanPps[i] = SRDAT_INDEXCHANNEL_NONE;
       AtodChanObc[i] = SRDAT_INDEXCHANNEL_NONE;
       AtodChanPwr[i] = SRDAT_INDEXCHANNEL_NONE;
       AtodChanTmp[i] = SRDAT_INDEXCHANNEL_NONE;
       
       if ( AtodXchModel[i] == SRDAT_ATODMODEL_USB4CH )   /* USB4CH */
       {
           AtodNumChannels[i]  = USB4CH_ANALOG_CHANNELS;
           AtodChanDig[i]      = AtodNumChannels[i];
           
           AtodNumChannels[i] += USB4CH_DIGITAL_CHANNELS;
           AtodChanPps[i]      = AtodNumChannels[i];
           
           AtodNumChannels[i] += 1;
           AtodChanPwr[i]      = AtodNumChannels[i];
               
           AtodNumChannels[i] += 1;
           AtodChanTmp[i]      = AtodNumChannels[i];

           AtodNumChannels[i] += USB4CH_GPS_CHANNELS;
           AtodChanObc[i]      = AtodNumChannels[i];
               

logit( "", "DAQ %d: Chan Dig=%d, Pps=%d, Obc=%d, Pwr=%d, Tmp=%d, Total=%d\n",
i,AtodChanDig[i],AtodChanPps[i],AtodChanObc[i],AtodChanPwr[i],AtodChanTmp[i],AtodNumChannels[i] );
               
           if ( AtodGpsModel == SRDAT_GPSMODEL_PCTIME ||
                AtodGpsModel == SRDAT_GPSMODEL_TCXO )
              UseTimeMethod = SRDAT_TIME_METHOD_PCT;
           else
              UseTimeMethod = SRDAT_TIME_METHOD_OBC;
       }
       else if ( AtodXchModel[i] == SRDAT_ATODMODEL_USB8CH )   /* USB8CH */
       {
           AtodNumChannels[i]  = USB8CH_ANALOG_CHANNELS;
           AtodChanDig[i]      = AtodNumChannels[i];
       
           AtodNumChannels[i] += USB8CH_DIGITAL_CHANNELS;
           AtodChanPps[i]      = AtodNumChannels[i];
           
           AtodNumChannels[i] += 1;
           AtodChanPwr[i]      = AtodNumChannels[i];
               
           AtodNumChannels[i] += 1;
           AtodChanTmp[i]      = AtodNumChannels[i];

           AtodNumChannels[i] += USB8CH_GPS_CHANNELS;
           AtodChanObc[i]      = AtodNumChannels[i];
               
           if ( AtodGpsModel == SRDAT_GPSMODEL_PCTIME ||
                AtodGpsModel == SRDAT_GPSMODEL_TCXO )
              UseTimeMethod = SRDAT_TIME_METHOD_PCT;
           else
              UseTimeMethod = SRDAT_TIME_METHOD_OBC;
       }
       else
       {
           logit( "e", "srusb2ew: Unknown USBxCH requested.  Exiting.\n" );
           sr_terminate();
       }


       AtodNumChannelsTotal += AtodNumChannels[i];

    } /* end for i < AtodNumBoards */



    if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_init_model\n" );

    return;
}

/******************************************************************************
 *  Function: sr_atod_open                                                    *
 *  Purpose:  Open the USBxCH DAQ and optional PARGPS timing module device    *
 *            drivers.  Once both drivers are opened, they must be 'attached' *
 *            so they can communication with each other.  The hardware is     *
 *            initialized when the drivers are opened.   This routine exits   *
 *            if any of these steps fail.                                     *
 ******************************************************************************/
void sr_atod_open( void )
{
   int i, usbxchError;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_open\n" );


  /* Start with invalid driver handles
   ***********************************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
         UsbXchHandle[i]  = BAD_DEVHANDLE;


  /* Open the USBxCH drivers and initialize the USBxCH DAQ boards
   **************************************************************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
       if ( Debug >= 5 )
       {
           logit( "", "Calling SrUsbXchOpen with:\n" );
           logit( "", "     Driver   %s\n",  AtodDriverName[i] );
           logit( "", "     XchModel %d\n",  AtodXchModel[i] );
           logit( "", "     Sps      %lf\n", AtodRequestedSps );
       }
       UsbXchHandle[i] = SrUsbXchOpen( AtodDriverName[i],
                                       AtodXchModel[i],
                                       AtodGpsModel,
                                       USERCFG_DEFAULT_BYTE, //WCT allow AtodUserCfgByte[i]
                                       AtodRequestedSps,
                                       &AtodSps,
                                       &usbxchError );

       if ( Debug >= 5 )  logit( "", "Result is Sps %lf, error %d = %s\n",
                               AtodSps, usbxchError, USBXCH_ERROR_MSG[usbxchError] );

       if ( UsbXchHandle[i] == BAD_DEVHANDLE )
       {
           sprintf( Msg, "Failed to open USBxCH driver %s (Err=%s)",
                    AtodDriverName[i], USBXCH_ERROR_MSG[usbxchError] );
           sr_error( EW_ERROR_BASE_USBXCH+usbxchError, Msg );
           sr_terminate( );
       }
       if ( Debug >= 0 )
          logit( "", "DAQ %d: Opened %s driver %s with GPS %s and SPS = %lf\n",
                 i, AtodXchModelName[i], AtodDriverName[i], AtodGpsModelName, AtodSps );

   } /* end for i < AtodNumBoards */



    if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_open\n" );
    return;
}

/******************************************************************************
 *  Function: sr_atod_correct_sps                                             *
 *  Purpose:  This corrects the requested sampling rate (sps) to the closest  *
 *            rate actually achievable on the USBxCH hardware.                *
 ******************************************************************************/
void sr_atod_correct_sps( double *sps )
{
    int    ok, usbxchError, packetRate;
    double sps0, sps1;

    if ( Debug >= 5 )
        logit( "", "Starting sr_atod_correct_sps with requested sps = %lf\n", *sps );


   /* Find closest allowable sampling rate
    **************************************/
    sps0 = *sps;
    ok   = SrUsbXchSpsRateValidate( sps0, &sps1, &usbxchError );

    if ( ok  &&  usbxchError == USBXCH_ERROR_NONE )
    {
       if ( Debug >= 1 )  logit( "", "Requested Sps is %lf, Actual is Sps %lf\n", sps0, sps1 );
       *sps = sps1;
    }
    else
    {
           sprintf( Msg, "Could not determine a valid sampling rated (req %lf, act %lf) (Err %d=%s)",
                    sps0, sps1, usbxchError, USBXCH_ERROR_MSG[usbxchError] );
           sr_error( EW_ERROR_BASE_USBXCH+usbxchError, Msg );
           sr_terminate( );
    }

    
   /* Only rates 651 or less are allowed currently
    **********************************************/
    if ( sps1 > 652 )
    {
       sprintf( Msg, "Sampling rates must be less than 652Hz (not req %lf or act %lf)", sps0, sps1 );
       usbxchError = USBXCH_ERROR_OPEN_SAMPLE_RATE;
       sr_error( EW_ERROR_BASE_USBXCH+usbxchError, Msg );
       sr_terminate( );
    }
    

   /* Ensure that PacketsPerFile value is at least 1 second of data
    ***************************************************************/
    packetRate = (int)(sps1*0.5) + 5;
    if ( PacketsPerFile < packetRate )
    {
       if ( Debug >= 1 )
          logit( "", "Adjusting PacketsPerFile from %d to %d (sps = %lf)\n",
                 PacketsPerFile, packetRate, sps1 );
       PacketsPerFile = packetRate;
    }

    if ( Debug >= 5 )
        logit( "", "Leaving  sr_atod_correct_sps with corrected sps = %lf\n", *sps );

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
    unsigned int nsamples, npackets, ntracebuf;

    if ( Debug >= 6 )  logit( "", "Starting sr_allocate_arrays\n" );


   /* Allocate space for the input acquired packet data
    ***************************************************/

    nsamples = AtodNumSamplesSec+1;                   /* samp/sec + spare     */
    if ( 2*(nsamples/2) != nsamples )                 /* round up to even num */
       nsamples++;

    if ( nsamples <= MAX_TRACEDAT_PTS )
       ntracebuf = 1;                                 /* all pts fit in 1 tb  */
    else
       ntracebuf = (nsamples / MAX_TRACEDAT_PTS) + 1; /* #tb's needed         */

    AtodNumSamplesTb = nsamples / ntracebuf;          /* samples per tb       */

    if ( Debug >= 1 ) 
       logit( "", "Samples per sec %d, per buf %d, #samples %d, #buff %d\n", 
              AtodNumSamplesSec, AtodNumSamplesTb, nsamples, ntracebuf );


    npackets = (AtodNumChannelsTotal * nsamples) + 8; /* total samples/second for all channels */
                                                      /* 2 samp/packet + spare status packets */
                                                      /* => room for about 2 seconds worth of data */
    AtodPacketBufferCount    = 0;
    AtodPacketBufferMaxCount = npackets;
    
    AtodPacketBuffer = (SRUSBXCH_PACKET *) calloc( npackets, sizeof(SRUSBXCH_PACKET) );
    if ( AtodPacketBuffer == NULL )
    {
        logit( "e", "srusb2ew: Cannot allocate DAQ packet buffer\n" );
        sr_terminate( );
    }


    FilePacketBufferCount    = 0;
    FilePacketBufferMaxCount = PacketsPerFile;
    
    FilePacketBuffer = (SRUSBXCH_PACKET *) calloc( FilePacketBufferMaxCount, sizeof(SRUSBXCH_PACKET) );
    if ( FilePacketBuffer == NULL )
    {
        logit( "e", "srusb2ew: Cannot allocate buffer for writing packets to file\n" );
        sr_terminate( );
    }
    
    

   /* Allocate space for the output trace buffer including both header + data
    *************************************************************************/
    TraceBuffer = (char *) malloc( MAX_TRACEBUF_SIZ ); /* alloc max size */
    if ( TraceBuffer == NULL )
    {
        logit( "e", "srusb2ew: Cannot allocate the trace buffer\n" );
        sr_terminate( );
    }

   /* Set up pointers to the tracebuf header and data parts
    *******************************************************/
    TraceHead  = (TRACE_HEADER  *) &TraceBuffer[0];
    TraceDat   = (long *) &TraceBuffer[sizeof(TRACE_HEADER)];


   /* Set output values common to all channels
    ******************************************/
    TraceHead->nsamp      = AtodNumSamplesTb;   /* number of samples in message   */
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
 *  Purpose:  Start the USBxCH DAQ acquiring data.                            *
 ******************************************************************************/
void sr_atod_start( int waitSec, int initTimes )
{
   int    i, ok, usbxchError;
   double startTime, endTime, restartDelay;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_start\n" );


  /* Initialize OnBoardCount (OBC) and gap variables
   *************************************************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
//      LastObc[i]         = SRDAT_INDEXCHANNEL_NONE;
      GapTrailer[i]      = 0;
   }


  /* Initialize other Atod variables
   *********************************/
   AtodLedValue       = 0;
   AtodBufferNum      = 0L;



  /* Initialize run parameters
   **************************/
   PakFilesWritten       = 0;
   AtodPacketBufferCount = 0;


  /* On first startup, we wait a bit so the other EW modules have a chance to
   * finish their setup before we start.  This helps ensure that the parallel
   * port outs done by our two start calls will really occur in quick succession
   * to each other and not be delayed by various interrupts or other activities
   * clogging the SuperIo or bridge chip.
   *****************************************************************************/
   if ( waitSec > 0 )
   {
      logit( "et", "srusb2ew: Starting acquisition after a %d second wait\n", waitSec );
      sleep_ew( waitSec * 1000 );
   }
   else
      logit( "et", "srusb2ew: Starting acquisition\n" );


  /* Start all USBxCH DAQs
   ***********************/
   if ( UsingGps )
      sr_atod_synchronize();

   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
      ok = SrUsbXchStart( UsbXchHandle[i], &usbxchError );
      if ( !ok  ||  usbxchError != USBXCH_ERROR_NONE )
      {
           sprintf( Msg, "Could not start USBxCH board %d (Err %d=%s)",
                    i, usbxchError, USBXCH_ERROR_MSG[usbxchError] );
           sr_error( EW_ERROR_BASE_USBXCH+usbxchError, Msg );
           sr_terminate( );
      }
   }


  /* Get approximate start time from the PC
   * or (on restart) use previous values
   ****************************************/
   hrtime_ew( &startTime );             /* start of first tracebuf */
   endTime = startTime - (1 / AtodSps); /* end of "prev" tracebuf  */


  /* Send first time stamp if using PC time
   ****************************************/
   sr_atod_send_pctimestamp();

   
  /* Make YMDHMS directory for holding pak files
   *********************************************/
   if ( WritingPak )
   {
      sr_pakfile_mkdir( );
   }


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

   AtodCurrentPt = 0L;
   
   if ( Debug >= 5 )
      logit( "", "At atod_start, DAQ 0 last end time %lf, this start time %lf \n",
             AtodEndTimeLast[0], AtodStartTime[0] );


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


  /* Quick exit if only working one DAQ board
   ******************************************/
   if ( AtodNumBoards == 1 )
   {
      if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_synchronize - no boards to synch\n" );
      return;
   }


  /* Initialize toggle value and time limit
   ****************************************/
   SrUsbXchUserIoRd( UsbXchHandle[0], &digioValue );
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
      SrUsbXchUserIoRd( UsbXchHandle[0], &digioValue );
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
         logit( "t", "srusb2ew: Failed to find Gps toggle signal within %d seconds.\nDAQ start may not be synchronized.\n",
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
 *  Function: sr_atod_send_pctimestamp                                        *
 *  Purpose:  If PCTIME style timing was requested, send the time from the    *      
 *            PC down to the USBxCH as a synthetic GPS time stamp.            *
 ******************************************************************************/
void sr_atod_send_pctimestamp( void )
{
   int    i, ok, usbxchError;
   double nowTime;
   
   static double lastTime = 0.0L;


  /* Quick exit if we have real GPS time
   *************************************/
   if ( AtodGpsModel != SRDAT_GPSMODEL_PCTIME &&
        AtodGpsModel != SRDAT_GPSMODEL_TCXO )
      return;


  /* Quick exit if it is too soon after sending the last PC timestamp
   ******************************************************************/
   hrtime_ew( &nowTime );
   if ( nowTime < (lastTime + 1) ) /* Wait at least 1 second */
      return;


  /* Send PC time stamp
   ********************/
   lastTime = nowTime;
   if ( Debug >= 5 )  logit( "t", "srusb2ew: Sending PC timestamp\n" );

   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
      ok = SrUsbXchSendTimeStamp( UsbXchHandle[i], &usbxchError );

      if ( !ok  ||  usbxchError != USBXCH_ERROR_NONE )
      {
         sprintf( Msg, "Could not send PC time stamp to USBxCH board %d (Err %d=%s)",
                  i, usbxchError, USBXCH_ERROR_MSG[usbxchError] );
         sr_error( EW_ERROR_BASE_USBXCH+usbxchError, Msg );
         sr_terminate( );
      }
   }

}  

/******************************************************************************
 *  Function: sr_atod_getdata                                                 *
 *  Purpose:  Read AtodNumSamples points for each of AtodNumChannels channels *
 *            into the AtodBuffer.                                            *
 ******************************************************************************/
int sr_atod_getdata( void )
{
    static int nbehind = 0;

    int           ok, i, npass, isSummary, usbxchError, nRequested;
    int           nSamples, nTotalSamples, nStatus, nTotalStatus, nTs;
    unsigned long nPacketsUL, nReadPacketsUL, nMaxPacketsUL;

    if ( Debug >= 6 )  logit( "", "Starting sr_atod_getdata with NumBoards %d\n", AtodNumBoards );


   /* Initialize some variables
    ***************************/
    nSamples      = 0;
    nStatus       = 0;
    nTs           = 0;
    nTotalSamples = 0;
    nTotalStatus  = 0;
    nRequested    = AtodNumSamplesSec * AtodNumBoards;

    nPacketsUL     = 0;
    nReadPacketsUL = 0;
    nMaxPacketsUL  = AtodPacketBufferMaxCount;
    
    for ( i = 1 ; i < AtodNumBoards ; i++ )
    {
      AtodSamplesReady[i]    = 0;
      AtodStatusReady[i]     = 0;
      AtodTimeStampsReady[i] = 0;
    }


    if (SummaryReportInterval > 0 && SummaryCount >= SummaryReportInterval-1)
       isSummary = 1;
    else
       isSummary = 0;
    if ( isSummary || Debug >= 5 )
        logit( "t", "srusb2ew: summary info follows ...\n" );




   /* Read data from DAQ
    ********************/
    npass = 0;

    if ( isSummary || Debug >= 5 )
       logit( "", "Reading Packet Data:\n" );


    while ( nSamples == 0 ) // was nTotalSamples < nRequested ) 
    {

       for ( i = 0 ; i < AtodNumBoards ; i++ ) //WCT does nSamples==0 really work for multiple boards ?
       {

         /* Read DAQ data as packets
          **************************/
          nReadPacketsUL = SrUsbXchGetDataAsPackets( UsbXchHandle[i],
                                                     AtodPacketBuffer,
                                                     nMaxPacketsUL,
                                                     &nPacketsUL, 
                                                     &usbxchError
                                                    );
          AtodPacketBufferCount = (int)nPacketsUL;

          if ( Debug >= 6 )
          {
             if ( nReadPacketsUL != nPacketsUL )
                logit( "", "GetDataAsPackets packet counts disagree, %lu vs %lu\n",
                       nReadPacketsUL, nPacketsUL );
             if ( usbxchError == USBXCH_ERROR_NONE )
                logit( "", "GetDataAsPackets read %d packets\n",
                       AtodPacketBufferCount );
             else
                logit( "", "GetDataAsPackets read %d packets, usbxchError 0x%X = %s\n",
                       AtodPacketBufferCount, usbxchError, USBXCH_ERROR_MSG[usbxchError]  );
          }


         /* Check for overflow and other errors
          *************************************/
          if ( usbxchError == USBXCH_ERROR_OVERFLOW )
          {
             sprintf( Msg, "GetDataAsPackets overflow error %s", USBXCH_ERROR_MSG[USBXCH_ERROR_OVERFLOW] );
             sr_error( EW_ERROR_BASE_USBXCH+USBXCH_ERROR_OVERFLOW, Msg );
             sr_atod_restart_recovery( );
             return( 0 );
          }

          if ( usbxchError != USBXCH_ERROR_NONE )
          {
             sprintf( Msg, "GetDataAsPackets error %s", USBXCH_ERROR_MSG[usbxchError] );
             sr_error( EW_ERROR_BASE_USBXCH+usbxchError, Msg );
             return( 0 );
         }



         /* If output of SR PAK files was requested, save a copy of
          * the newly read in packets until there are enough to output.
          ************************************************************/
          if ( WritingPak )
             sr_pakfile_write_when_full();



         /* Convert packets to more convenient
          * columns and account for packets converted.
          ********************************************/
          ok = SrUsbXchFullPacketsToColumns( 0, // Normal not final processing
                                        AtodPacketBuffer, AtodPacketBufferCount,
                                        AtodSamples, MAX_COLROWS, &nSamples,
                                        AtodStatus, MAX_COLROWS, &nStatus,
                                        AtodTimeStamps, 2, &nTs,
                                        &usbxchError );

          if ( !ok  || usbxchError != USBXCH_ERROR_NONE )
          {
             logit( "o", "srusb2ew: SrUsbXchPacketsToColumns failed with err #%d = %s\n",
                 usbxchError, USBXCH_ERROR_MSG[usbxchError]  );
             sr_terminate();
          }

          AtodSamplesReady[i]    += nSamples;
          AtodStatusReady[i]     += nStatus;
          AtodTimeStampsReady[i] += nTs;
          nTotalSamples          += nSamples;
          nTotalStatus           += nStatus;

          if ( (nSamples > 0)  &&  (isSummary || Debug >= 6) )
          {
             logit( "", "After PacketsToColumns DAQ %d, nSamples %d, AtodSamplesReady %d, nTotalSamples %d\n",
                    i, nSamples, AtodSamplesReady[i], nTotalSamples );
             logit( "", "After PacketsToColumns DAQ %d, nStatus %d, AtodStatusReady %d, nTotalStatus %d\n",
                    i, nStatus, AtodStatusReady[i], nTotalStatus );

             sr_log_ts_data();
             sr_log_sample_data();
             sr_log_status_data();
          }



       } /* end for i < AtodNumBoards */

       npass++;


       /* If more points wanted, give them time to arrive
        * and send a synthetic time stamp if needed.
        *************************************************/
       if ( nSamples == 0 )
       {
          if ( Debug >= 7 )  
             logit( "", "  wait while nSamples == 0 (was nTotalSamples %d < nRequested %d)\n",
                    nTotalSamples, nRequested );

          sleep_ew( SleepMs );   /* Wait SleepMs milliseconds (eg 200) */
       
          sr_atod_send_pctimestamp( );
       }


    } /* end while nSamples == 0 (was nTotalSamples < nRequested) */



   /* If we read all the requested data in right away,
    * it means we're starting to fall behind.  If this
    * condition lasts too long, give a warning.
    **************************************************/
    if ( npass == 1 )
        nbehind++;
    else
        nbehind = 0;

    if ( nbehind > 0 )
        logit( "et", "srusb2ew: Earthworm running slow but still OK ...\n     (data built up in FIFO %d times)\n", nbehind );

    if ( nbehind == BEHIND_LIMIT )
    {
        sprintf( Msg, "Warning - Earthworm is not keeping up with acquisition, overflow could result" );
        sr_error( EW_ERROR_BASE_USBXCH+USBXCH_ERROR_OVERFLOW, Msg );
    }




   /* Update current point and buffer count
    ***************************************/
    if ( Debug >= 5 )  logit( "", "Completed buffer %lu\n", AtodBufferNum );
    AtodBufferNum++;


    if ( Debug >= 6 )  logit( "", "Leaving  sr_atod_getdata with %d points\n", nTotalSamples );

    return( nTotalSamples );
}

/******************************************************************************
 *  Function: sr_send_traces                                                  *
 *  Purpose:  Fill a earthworm tracebuf messages with data just acquired by   *
 *            the USBxCH and send it out to the ring.  The data is demuxed    *
 *            here so there will be one message per channel.                  *
 ******************************************************************************/
void sr_send_traces( void )
{
    int    i, j, k, ichan, rc, ptstart, ptend; 
    long  *tracePtr, readySamples, totalReadySamples, buffersize;
    double tempc, tempf;

    if ( Debug >= 6 )  logit( "", "Starting sr_send_traces\n" );


   /* Set tracebuf type as new or old format
    ****************************************/
    if (OutputMsgType == TypeTraceBuf2 && TRACE2_OK)
        Logo.type = TypeTraceBuf2;   /* We'll send the new tracebuf2s */
    else
        Logo.type = TypeTraceBuf;    /* We'll send the old tracebufs  */


   /* Loop around filling tracebufs until all ready samples have been sent
    **********************************************************************/
    ptstart           = 0;
    totalReadySamples = AtodSamplesReady[0];
//WCT - could multiple boards have diff number of samples ready
    
    while ( totalReadySamples > 0 )
    {
       readySamples = totalReadySamples;
       if ( readySamples > MAX_TRACEDAT_PTS )   /* ensure we do not overflow buffer */
          readySamples = MAX_TRACEDAT_PTS;

       if ( readySamples > 2*AtodNumSamplesTb ) /* try to send 1-2 sec of data      */
          readySamples = AtodNumSamplesTb;

       

       ichan = 0;
       ptend = ptstart + readySamples - 1;

       if ( Debug >= 6 )
	  logit( "", "SendTraces start %d + ready %ld - 1 = end %d, total ready %ld\n",
                 ptstart, readySamples, ptend, totalReadySamples );
              

      /* Set trace start and end times (sec since 1970) same for all channels
       **********************************************************************/
       sr_set_tracebuf_time( ptstart, ptend );


      /* Compute some debugging info
       *****************************/
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
       

      /* Make one tracebuf message per channel.  Looping
       * over all DAQ boards and then all channels per board.
       *****************************************************/
       for ( i = 0 ; i < AtodNumBoards ; i++ )
       {
          TraceHead->starttime = AtodStartTime[i];
          TraceHead->endtime   = AtodEndTime[i];
          TraceHead->nsamp     = readySamples;     /* number of samples in message   */

         /* Process all channels for selected board
          *****************************************/
          for ( j = 0 ; j < AtodNumChannels[i] ; j++ )
          {

             /* Fill the trace buffer header
              ******************************/
              sr_strncpy( TraceHead->sta,  ChanList[ichan].sta,  TRACE_STA_LEN );  /* Site      */
              sr_strncpy( TraceHead->net,  ChanList[ichan].net,  TRACE_NET_LEN );  /* Network   */
              if ( OutputMsgType == TypeTraceBuf2 )
              {
                 sr_strncpy(  TraceHead->chan,                ChanList[ichan].comp, TRACE2_CHAN_LEN );
                 sr_strncpy( &TraceHead->chan[TRACE_POS_LOC], ChanList[ichan].loc,  TRACE2_LOC_LEN );
              }
              else /* ( OutputMsgType == TypeTraceBuf ) */
                 sr_strncpy( TraceHead->chan, ChanList[ichan].comp, TRACE_CHAN_LEN );

              TraceHead->pinno = ChanList[ichan].pin;                           /* Pin num   */


             /* Transfer samples from DAQ buffer to tracebuf buffer
              *****************************************************/
              tracePtr = &TraceDat[0];

              for ( k = 0 ; k < readySamples ; k++ )
              {

                 if ( j == AtodChanDig[i] )
                    *tracePtr = (long)AtodSamples[ptstart+k].DigitalIn;//WCT allow per board AtodSamples...
                 else if ( j == AtodChanPps[i] )
                    *tracePtr = (long)AtodSamples[ptstart+k].PpsToggle;
                 else if ( j == AtodChanPwr[i] )
                    *tracePtr = (long)AtodStatus[0].PowerInfo;
                 else if ( j == AtodChanTmp[i] ) {
                    SrDatUsbTemperatureCompute( AtodStatus[0].Temperature, &tempc, &tempf );
                    *tracePtr = (long)(tempc * 10.0L);
		    /* *tracePtr = (long)(tempf * 10.0L); */
                    }
                 else 
                    *tracePtr = AtodSamples[ptstart+k].Channel[j];

                  if (SummaryMin[i][j] > *tracePtr)
                     SummaryMin[i][j] = *tracePtr;
                  else if (SummaryMax[i][j] < *tracePtr)
                     SummaryMax[i][j] = *tracePtr;

                  tracePtr++;
              }

              if ( Debug >= 6 )
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
              buffersize = sizeof(TRACE_HEADER) + (readySamples*sizeof(long));
              rc = tport_putmsg( &Region, &Logo, buffersize, TraceBuffer );

              if ( rc == PUT_TOOBIG )
                  logit( "e", "srusb2ew: Trace message for channel %d too big\n", i );

              if ( rc == PUT_NOTRACK )
                  logit( "e", "srusb2ew: Tracking error while sending channel %d\n", i );


              ichan++;

            } /* end for j < AtodNumChannels[i] */

       } /* end for i < AtodNumBoards */

       ichan              = 0;
       ptstart            = ptend + 1;
       totalReadySamples -= readySamples;
       
    } /* end while totalReadySamples > 0 */




   /* Log GPS and summary information periodically
    **********************************************/
    sr_gpslock_update();
    
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
    AtodCurrentPt    += AtodSamplesReady[0];
//WCT - could different boards have diff number of samples ready

    for ( i = 0 ; i < AtodNumBoards ; i++ )
    {
       AtodSamplesReady[i]    = 0;
       AtodStatusReady[i]     = 0;
       AtodTimeStampsReady[i] = 0;
    }

    if ( Debug >= 6 )
        logit( "", "Leaving  sr_send_traces with new current pt %d\n", AtodCurrentPt );
    return;
}

/******************************************************************************
 *  Function: sr_set_tracebuf_time                                            *
 *  Purpose:  Set the trace start and end times in seconds since 1970.  This  *
 *            value is the same for all the channels in the current buffer.   *
 *            The computation is slightly different, depending on what data   *
 *            is available.  The best is using the timestamp data included    *
 *            with the data.  If no timestamp info is available, then just    *
 *            compute what the expected time would be assuming the sampling   *
 *            is perfectly stable and accurate.                               *
 ******************************************************************************/
void sr_set_tracebuf_time( int ptstart, int ptend )
{
    int    i, thisMethod, timesOk[MAX_BOARD];
    long   atodStartIndex, atodEndIndex, atodStartPt, atodEndPt, atodNumSamples;
    double computedEndTime, computedSampRate, correctedTime[MAX_BOARD];
    double sampleTime[MAX_BOARD], expectedStart[MAX_BOARD], allowedStart[MAX_BOARD];
    double oneSample, diff, tolerance;
    NI     *NmeaInfo;


    if ( Debug >= 6 )
       logit( "", "Starting sr_set_tracebuf_time, ptstart %d, ptend %d, AtodCurrentPt = %d\n",
              ptstart, ptend, AtodCurrentPt );

    atodStartIndex  = ptstart;
    atodEndIndex    = ptend;
    atodStartPt     = AtodCurrentPt + atodStartIndex;
    atodEndPt       = AtodCurrentPt + atodEndIndex;
    atodNumSamples  = atodEndIndex - atodStartIndex + 1;
    oneSample       = 1.0L / AtodSps;

    NmeaInfo        = &AtodStatus[0].NmeaInfo;



    /* Compute based on time saved in data
    **************************************/
    if ( UsingGps )
    {
       thisMethod       = SRDAT_TIME_METHOD_OBC;
       AtodStartTime[0] = AtodSamples[atodStartIndex].SampleTime;
       AtodEndTime[0]   = AtodSamples[atodEndIndex].SampleTime;
        
       for ( i = 1 ; i < AtodNumBoards ; i++ )
       {
           AtodStartTime[i] = AtodStartTime[0];
           AtodEndTime[i]   = AtodEndTime[0];
       }
    
        if ( Debug >= 3 )
           logit( "t", "srusb2ew: DAQ #0 buf start time (obc) = %lf, Point %ld, GPS %d satellites\n",
                       AtodStartTime[0], atodStartPt, NmeaInfo->Nsat );
    
        if ( Debug >= 4 )
           logit( "t", "srusb2ew: DAQ #0 buf end time   (obc) = %lf, Point %ld\n",
                       AtodEndTime[0], atodEndPt );
    }


    /* Compute based on global data sampling rate and previous time
    **************************************************************/
    else
    {
        thisMethod       = SRDAT_TIME_METHOD_NONE;
        AtodStartTime[0] = AtodEndTimeLast[0] + oneSample;
        AtodEndTime[0]   = AtodStartTime[0] + ((atodEndIndex-atodStartIndex) * oneSample);

        for ( i = 1 ; i < AtodNumBoards ; i++ )
        {
            AtodStartTime[i] = AtodStartTime[0];
            AtodEndTime[i]   = AtodEndTime[0];
        }
    
        
        for ( i = 0 ; i < AtodNumBoards ; i++ )
        {
           if ( Debug >= 3 )
              logit( "t", "srusb2ew: DAQ #%d buf start time (default) = %lf, Point %ld\n",
                     i, AtodStartTime[i], atodStartPt );
           if ( Debug >= 4 )
              logit( "t", "srusb2ew: DAQ #%d buf end time   (default) = %lf, Point %ld\n",
                     i, AtodEndTime[i], atodEndPt );
        }
    }


    if ( Debug >= 5 )
    {
        logit( "", "Last end %lf, This start %lf, This end %lf\n",
              AtodEndTimeLast[0], AtodStartTime[0], AtodEndTime[0] );
        logit( "", "LastEnd->ThisStart (1sps %lf) %lf, ThisStart->ThisEnd %lf\n",
              (AtodStartTime[0]-AtodEndTimeLast[0]), oneSample, (AtodEndTime[0]-AtodStartTime[0]) );
    }

    if ( Debug >= 5 )
         logit( "", "Method used for computing time.  Last = %d, Current = %d, Requested = %d\n",
                LastTimeMethod, thisMethod, UseTimeMethod );



   /* Check for gaps or overlaps on all DAQs
    ****************************************/
    for ( i = 0 ; i < AtodNumBoards ; i++ )
    {
       timesOk[i]       = 1;
       sampleTime[i]    = (AtodEndTime[i] - AtodStartTime[i]) / (double)(atodNumSamples-1);
       expectedStart[i] = AtodEndTimeLast[i] + oneSample;
       allowedStart[i]  = AtodEndTimeLast[i] + oneSample*1.5; /* Allow .5 sample slop */

       if ( GapTrailer[i] > 0 ) /* Print additional debugging info after a gap has occurred */
       {
           if ( Debug >= 7 )
           {
              logit( "", "Buffer %d after gap\n", MAXGAPTRAIL-GapTrailer[i] );
              sr_log_gap_details( thisMethod, expectedStart[i], AtodEndTime[i] );
           }
           GapTrailer[i]--;
       }

       if ( AtodStartTime[i] < AtodEndTimeLast[i] ) /* Check for overlap */
       {
           logit( "et", "srusb2ew: Gap check: Overlap detected DAQ #%d\n     (this start %lf < last end %lf)\n",
                  i, AtodStartTime[i], AtodEndTimeLast[i] );
           sr_log_gap_details( thisMethod, expectedStart[i], AtodEndTime[i] );
           timesOk[i]       = 0;
           correctedTime[i] = AtodEndTimeLast[i];
           GapTrailer[i]    = MAXGAPTRAIL;
       }

       if ( AtodStartTime[i] > allowedStart[i] ) /* Check for gap */
       {
           logit( "et", "srusb2ew: Gap check: Gap detected DAQ #%d\n     (found %lf start > expected start %lf = \n      last end %lf + 1sample %lf (computed 1sample %lf))\n",
                  i, AtodStartTime[i], expectedStart[i], AtodEndTimeLast[i], oneSample, sampleTime[i] );
           sr_log_gap_details( thisMethod, expectedStart[i], AtodEndTime[i] );
           timesOk[i]       = 0;
           correctedTime[i] = expectedStart[i];
           GapTrailer[i]    = MAXGAPTRAIL;
       }



       /* If times are not consistent, fix them as no gaps or overlaps are allowed
        **************************************************************************/
       if ( !timesOk[i] )
       {
           if (thisMethod == LastTimeMethod  &&  LastTimeMethod != SRDAT_TIME_METHOD_NONE)
           {
               if (Debug >= 3 )
                   logit( "", "DAQ #%d start time corrected from %lf to %lf\n",
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
    computedEndTime = AtodStartTime[0] + (atodEndIndex-atodStartIndex)*oneSample;
    diff = computedEndTime - AtodEndTime[0];
    computedSampRate = (atodEndIndex-atodStartIndex) / (AtodEndTime[0] - AtodStartTime[0]);
    
    if (Debug >= 5)
    {
       /* We currently use interpolated end time for TraceHead->endtime if quality is ok */
        logit( "", "End time: Interpolated = %lf, Computed = %lf, Difference = %lf\n",
                   AtodEndTime[0], computedEndTime, diff );

        /* We currently do not change the sample rate */
        logit( "", "Computed sample rate = %lf (std = %lf), keeping standard\n", computedSampRate, AtodSps );
        //WCT - Should we change sample rate or not ?
        //      Wave_Viewer says NO, Decimate says YES?
        //      TraceHead->samprate = computedSampRate;
    }



   /* Update the quality flag
    *************************/
    if ( diff < 0 )
            diff = -diff;
    if ( NmeaInfo->Nsat < 3                       ||  /* not enough sats */
         (NmeaInfo->YmdSource != SRDAT_SOURCE_GPS &&  /* YMD not GPS/OBC */
          NmeaInfo->YmdSource != SRDAT_SOURCE_OBC)||
         (NmeaInfo->HmsSource != SRDAT_SOURCE_GPS &&  /* HMS not GPS/OBC */
          NmeaInfo->HmsSource != SRDAT_SOURCE_OBC)
                                                          )
    {
       TraceHead->quality[0] = TIME_TAG_QUESTIONABLE;
        if ( Debug >= 5 )  logit( "", "Setting time tag questionable (Nsat = %d)\n", NmeaInfo->Nsat );
    }
    else
    {
       TraceHead->quality[0] = QUALITY_OK;
    }


   /* Change time if too questionable
    *********************************/
    tolerance = oneSample * 0.5;
    if ( diff > tolerance )
    {
       if ( TraceHead->quality[0] == QUALITY_OK )
       {
             logit( "", "Leaving end time alone at %lf instead of %lf because time ok (Nsat %d)\n",
                    AtodEndTime[0], computedEndTime, NmeaInfo->Nsat );
       }
       else /* if ( TraceHead->quality[0] == TIME_TAG_QUESTIONABLE ) */
       {
          if ( AtodGpsModel == SRDAT_GPSMODEL_TCXO )
          {
              logit( "", "ERROR: End time of %lf should equal computed %lf for TCXO\n",
                     AtodEndTime[0], computedEndTime );
          }
          else /* AtodGpsModel == SRDAT_GPSMODEL_PCTIME, GARMIN, etc ) */
          {
              logit( "", "Corrected end time from %lf to %lf because time questionable (Nsat %d)\n",
                     AtodEndTime[0], computedEndTime, NmeaInfo->Nsat );

              for ( i = 0 ; i < AtodNumBoards ; i++ )
                 AtodEndTime[i] = computedEndTime;
          }

       } /* end if TraceHead->quality */

    } /* end if diff > tolerance */
    

   /* Prepare for next pass
    ***********************/
    LastTimeMethod = thisMethod;

    for ( i = 0 ; i < AtodNumBoards ; i++ )
       AtodEndTimeLast[i] = AtodEndTime[i];


    if ( Debug >= 6 )  logit( "", "Leaving  sr_set_tracebuf_time\n" );
    return;
}

/******************************************************************************
 *  Function: sr_gpslock_update                                               *
 *  Purpose:  Update GPS lock summary info. This indicates GPS timing quality.*
 *                                                                            *
 *            Note: GpsLockCount increments for each buffer of data and       *
 *            GpsLockReportInterval is measured in seconds, so comparing      *
 *            them is only valid because each buffer is about 1 second long.  *
 ******************************************************************************/
void sr_gpslock_update( void )
{
   char  *gpsLockCurrStr;

   if ( Debug >= 6 )  logit( "", "Starting sr_gpslock_update\n" );
    
   if ( UsingGps )
   {
      /* Update GPS lock summary info
       ******************************/
       if ( AtodStatus[0].NmeaInfo.Nsat >= 3 )
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
              logit( "t", "srusb2ew: GPS Lock Stats = Good %5ld, Bad %5ld, Currently %s\n",
                     GpsLockGood, GpsLockBad, gpsLockCurrStr );


          /* Send error message if status changed
           **************************************/
           if ( GpsLockBad < GpsLockBadLimit ) /* GPS lock good now */
           {
               if ( GpsLockStatus != 1 )       /* but bad before    */
               {
                   sprintf( Msg, "GPS satellite count now OK (Good %ld / %ld seconds)",
                            GpsLockGood, GpsLockReportInterval );
                   sr_error( EW_ERROR_BASE_USBXCH+USBXCH_ERROR_GPS_STATUS_CHANGE, Msg );
               }
               GpsLockStatus = 1;
           }

           else /* ( GpsLockBad >= GpsLockBadLimit ) */ /* GPS lock bad now */
           {
               if ( GpsLockStatus != 0)                 /* but good before  */
               {
                   sprintf( Msg, "GPS satellite count too low (Bad %ld / %ld seconds)",
                            GpsLockBad, GpsLockReportInterval );
                   sr_error( EW_ERROR_BASE_USBXCH+USBXCH_ERROR_GPS_STATUS_CHANGE, Msg );
               }
               GpsLockStatus = 0;
           }


          /* Reset lock counts for next interval
           *************************************/
           GpsLockCount = GpsLockGood = GpsLockBad = 0;


       } /* end if GpsLockCount > GpsLockReportInterval */

   } /* end if UsingGps */

}


/******************************************************************************
 *  Function: sr_atod_stop                                                    *
 *  Purpose:  Stop the PARxCH DAQ and PARGPS timing module from acquiring.    *
 ******************************************************************************/
void sr_atod_stop( void )
{
   int i, usbxchError;

   if ( Debug >= 6 )  logit( "", "Starting sr_atod_stop\n" );


  /* Stop UsbXch DAQs
   ******************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
      if ( UsbXchHandle[i] != BAD_DEVHANDLE )
         SrUsbXchStop( UsbXchHandle[i], &usbxchError );

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

   /* Stop UsbXch DAQ and PARGPS
    ****************************/
    sr_atod_stop();



  /* Close UsbXch DAQs
   *******************/
   for ( i = 0 ; i < AtodNumBoards ; i++ )
      if ( UsbXchHandle[i] != BAD_DEVHANDLE )
      {
         SrUsbXchClose( UsbXchHandle[i] );
         UsbXchHandle[i] = BAD_DEVHANDLE;
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
   int flag;
   
   if ( Debug >= 7 )  logit( "", "Starting sr_check_terminate\n" );


  /* Check for EW terminate request
   ********************************/
   flag = tport_getflag( &Region );

   if ( flag == TERMINATE || flag == MyPid )
       sr_terminate();

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

   /* Output all remaining packets
    ******************************/
    if ( WritingPak )
       sr_pakfile_write( FilePacketBufferCount );

   /* Notify user
    *************/
    logit( "t", "srusb2ew: Termination requested; exiting!\n" );
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
#define VERIFY_GPSMODEL         3
#define MAX_VERIFY              4

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
           if ( Debug >= 6 )  logit( "", "Verifying USBxCH model\n" );
           if ( strcmp( str, "USB4CH" ) == 0 )
              keyvalue = SRDAT_ATODMODEL_USB4CH;
           else
               result = 0;
           break;


      /* Check for GPS model
       *********************/
       case VERIFY_GPSMODEL:
           if ( Debug >= 6 )  logit( "", "Verifying GPS model\n" );
           if ( strcmp( str, "GARMIN" ) == 0 )
               keyvalue = SRDAT_GPSMODEL_GARMIN;
           else if ( strcmp( str, "PCTIME" ) == 0 )
               keyvalue = SRDAT_GPSMODEL_PCTIME;
           else if ( strcmp( str, "TCXO" ) == 0 )
               keyvalue = SRDAT_GPSMODEL_TCXO;
           else if ( strcmp( str, "TRIMBLE" ) == 0 )
               keyvalue = SRDAT_GPSMODEL_TRIMBLE;
           else if ( strcmp( str, "ONCORE" ) == 0 )
               keyvalue = SRDAT_GPSMODEL_ONCORE;
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

   if ( Debug >= 6 )  logit( "", "Leaving  sr_verify_string with keyvalue %d\n", keyvalue );

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
#define CMD_GPSMODEL         6
#define CMD_ATODSPS          7
#define CMD_UNUSED4          8
#define CMD_UNUSED3          9
#define CMD_UNUSED2         10
#define MAX_CMD             11

#define OPT_SUMMARYINTERVAL  0
#define OPT_LOCKINTERVAL     1
#define OPT_LOCKLIMIT        2
#define OPT_OUTPUTMSGTYPE    3
#define OPT_CHANNEL          4
#define OPT_CHANNELSCNL      5
#define OPT_OUTPUTPAK        6
#define OPT_ATODDRIVER       7
#define OPT_ATODXCHMODEL     8
#define OPT_PACKETSPERFILE   9
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
   int   unknown;

   if ( Debug >= 6 )  logit( "", "Starting sr_read_config\n" );


  /* Set optional command defaults
   *******************************/
   SummaryReportInterval = 0;
   GpsLockReportInterval = 0;
   GpsLockBadLimit       = 1;
   UsingGps              = 1;
   WritingPak            = 1;
   PacketsPerFile        = -1; /* Set to sampling rate when defined */ 
   sr_strncpy( AtodGpsModelName, "GARMIN", MAX_STR );

   for ( i = 1 ; i < MAX_BOARD ; i++ )
   {
      sr_strncpy( AtodDriverName[i],   "NONE",    MAX_STR );
      sr_strncpy( AtodXchModelName[i], "USB4CH",  MAX_STR );
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
   unknown       = 0;


   /* These are the command strings to match
    ****************************************/
    cmdstr[CMD_MODULEID]        = "MyModuleId";
    cmdstr[CMD_RINGNAME]        = "RingName";
    cmdstr[CMD_LOGFILE]         = "LogFile";
    cmdstr[CMD_HEARTBEAT]       = "HeartBeatInterval";
    cmdstr[CMD_ATODDRIVER]      = "AtodDriverName";
    cmdstr[CMD_ATODXCHMODEL]    = "AtodModelName";
    cmdstr[CMD_GPSMODEL]        = "GpsModelName";
    cmdstr[CMD_ATODSPS]         = "SamplingRate";
    cmdstr[CMD_UNUSED4]         = "Unused4";
    cmdstr[CMD_UNUSED3]         = "Unused3";
    cmdstr[CMD_UNUSED2]         = "Unused2";

    optstr[OPT_SUMMARYINTERVAL] = "SummaryInterval";
    optstr[OPT_LOCKINTERVAL]    = "GpsReportInterval";
    optstr[OPT_LOCKLIMIT]       = "GpsBadLimit";
    optstr[OPT_OUTPUTMSGTYPE]   = "OutputMsgType";
    optstr[OPT_CHANNEL]         = "EwChannel";
    optstr[OPT_CHANNELSCNL]     = "EwChannelScnl";
    optstr[OPT_OUTPUTPAK]       = "OutputPakFiles";
    optstr[OPT_ATODDRIVER]      = "AtodDriverNameN";
    optstr[OPT_ATODXCHMODEL]    = "AtodModelNameN";
    optstr[OPT_PACKETSPERFILE]  = "PacketsPerFile";



   /* Zero out one init flag for each required command
    **************************************************/
    for ( i = 0 ; i < MAX_CMD ; i++ )
        init[i] = 0;


    init[CMD_UNUSED2] = 1;
    init[CMD_UNUSED3] = 1;
    init[CMD_UNUSED4] = 1;
    

   /* Open the main configuration file
    **********************************/
    nfiles = k_open( configfile );
    if ( nfiles == 0 )
    {
        logit( "e",
                "srusb2ew: Error opening command file <%s>; exiting!\n",
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
                          "srusb2ew: Error opening command file <%s>; exiting!\n",
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
  /*6*/     else if ( k_its( cmdstr[CMD_GPSMODEL] ) )
            {
                str = k_str();
                if ( sr_verify_string( VERIFY_GPSMODEL, str, &AtodGpsModel ) )
                {
                    sr_strncpy( AtodGpsModelName, str, MAX_STR );
                    if ( AtodGpsModel == SRDAT_GPSMODEL_TCXO)
                            UsingGps = 0;
                    else
                            UsingGps = 1;
                }
                else
                    logit( "e", "srusb2ew: <%s> value %s not allowed, using default of %s instead.\n",
                           cmdstr[CMD_GPSMODEL], str, AtodGpsModelName );

                init[CMD_GPSMODEL] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",cmdstr[CMD_GPSMODEL], AtodGpsModelName );
            }
  /*7*/     else if ( k_its( cmdstr[CMD_ATODSPS] ) )
            {
                AtodRequestedSps = k_val();
                init[CMD_ATODSPS] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %lf\n",cmdstr[CMD_ATODSPS], AtodRequestedSps );
            }
  /*8*/     else if ( k_its( cmdstr[CMD_UNUSED4] ) )
            {
                str = k_str();
                init[CMD_UNUSED4] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",cmdstr[CMD_UNUSED4], str );
            }
  /*9*/     else if ( k_its( cmdstr[CMD_UNUSED3] ) )
            {
                str = k_str();
                init[CMD_UNUSED3] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",cmdstr[CMD_UNUSED3], str );
            }
 /*10*/     else if ( k_its( cmdstr[CMD_UNUSED2] ) )
            {
                init[CMD_UNUSED2] = 1;

                if ( Debug >= 2 )  logit( "", "Found <%s> value %d\n",cmdstr[CMD_UNUSED2], k_int() );
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
            else if ( k_its( optstr[OPT_OUTPUTPAK] ) )
            {
                WritingPak = k_long();
                if ( Debug >= 2 )  logit( "", "Found <%s> value %ld\n",optstr[OPT_OUTPUTPAK], WritingPak );
            }
            else if ( k_its( optstr[OPT_PACKETSPERFILE] ) )
            {
                PacketsPerFile = k_long();
                if ( Debug >= 2 )  logit( "", "Found <%s> value %ld\n",optstr[OPT_PACKETSPERFILE], PacketsPerFile );
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
                    logit( "e", "srusb2ew: <%s> value %s not allowed, using default of %s instead.\n",
                            optstr[OPT_ATODXCHMODEL], str, AtodXchModelName[AtodNumBoards-1] );

                if ( Debug >= 2 )  logit( "", "Found <%s> value %s\n",
                                         optstr[OPT_ATODXCHMODEL],
                                         AtodXchModelName[AtodNumBoards-1] );
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
                        logit( "e", "srusb2ew: <%s> TYPE_TRACEBUF2 not allowed in this version, using TYPE_TRACEBUF instead <%s>.\n",
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
               logit( "et", "srusb2ew: Warning.  OutputMsgType is old style\n      TYPE_TRACEBUF but channel list is new %s.\n", optstr[OPT_CHANNELSCNL] );
            }

            chan = k_int();                         /* Get channel number */
            if ( chan >= MAX_CHAN*MAX_BOARD || chan < 0 )
            {
               logit( "e", "srusb2ew: Error. Bad channel number (%d) in config file.\n", chan );
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
               logit( "e", "srusb2ew: Bad pin number for DAQ channel %d\n", chan );
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
                logit( "e", "srusb2ew: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                unknown = 1;
                continue;
            }

           /* See if there were any errors processing the command
            *****************************************************/
            if ( k_err() )
            {
               logit( "e",
                       "srusb2ew: Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
               exit( -1 );
            }

        } /* end while k_rd */

        nfiles = k_close();

   } /* end while nfiles > 0 */


  /* After all files are closed, check init flags for missed or unknown commands
   *****************************************************************************/
   nmiss = 0;

   for ( i = 0 ; i < MAX_CMD ; i++ )
   {
       if ( !init[i] )
           nmiss++;
   }

   if ( nmiss )
   {
       logit( "e", "srusb2ew: ERROR, no " );
       if ( !init[CMD_MODULEID] )      logit( "e", "<%s> ", cmdstr[CMD_MODULEID] );
       if ( !init[CMD_RINGNAME] )      logit( "e", "<%s> ", cmdstr[CMD_RINGNAME] );
       if ( !init[CMD_LOGFILE] )       logit( "e", "<%s> ", cmdstr[CMD_LOGFILE] );
       if ( !init[CMD_HEARTBEAT] )     logit( "e", "<%s> ", cmdstr[CMD_HEARTBEAT] );
       if ( !init[CMD_ATODDRIVER] )    logit( "e", "<%s> ", cmdstr[CMD_ATODDRIVER] );
       if ( !init[CMD_ATODXCHMODEL] )  logit( "e", "<%s> ", cmdstr[CMD_ATODXCHMODEL] );
       if ( !init[CMD_GPSMODEL] )      logit( "e", "<%s> ", cmdstr[CMD_GPSMODEL] );
       if ( !init[CMD_ATODSPS] )       logit( "e", "<%s> ", cmdstr[CMD_ATODSPS] );
       logit( "e", "srusb2ew: command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   
   if ( unknown )
   {
       logit( "e", "srusb2ew: Unknown command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   if ( Debug >= 2 )  logit( "", "Number of A/D boards (AtodNumBoards) is %d\n", AtodNumBoards );
   
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
              "srusb2ew: Invalid ring name <%s>; exiting!\n", RingName );
       exit( -1 );
   }


  /* Look up installation of interest
   **********************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      logit( "e",
             "srusb2ew: Error getting local installation id; exiting!\n" );
      exit( -1 );
   }


  /* Look up module of interest
   ****************************/
   if ( GetModId( MyModName, &MyModId ) != 0 )
   {
      logit( "e",
             "srusb2ew: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }


  /* Look up message types of interest
   ***********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
   {
      logit( "e",
             "srusb2ew: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      logit( "e",
             "srusb2ew: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_TRACEBUF", &TypeTraceBuf ) != 0 )
   {
      logit( "e",
             "srusb2ew: Invalid message type <TYPE_TRACEBUF>; exiting!\n" );
      exit( -1 );
   }

   if ( TRACE2_OK )
   {
      if ( GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 ) != 0 )
      {
         logit( "e",
                "srusb2ew: Invalid message type <TYPE_TRACEBUF2>; exiting!\n" );
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
            logit( "et", "srusb2ew:  Error sending heartbeat.\n" );

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
      SrUsbXchUserLed( UsbXchHandle[i], AtodLedValue, LED_OFF );

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
   logit( "et", "srusb2ew: %s\n", note );

   if ( tport_putmsg( &Region, &Logo, msgLen, ErrMsg ) != PUT_OK )
           logit( "et", "srusb2ew:  Error sending error:%d.\n", ierr );

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
 *  Function: sr_log_ts_data                                                  *
 *  Purpose:  Display the bounding timestamp data.                            *
 ******************************************************************************/
void sr_log_ts_data( void )
{
   int    i, j, readyTimeStamps;
   char   datamsg[256];
   double tempC, tempF;

   if ( Debug >= 6) logit( "", "Starting  sr_log_ts_data\n" );

  /* Prepare title string
   **********************/
                  /*  N  NN  NNNNNNNNNN  ffffffffff.ffffff  0xHHHHHHHH  NN  N   N   N  fff.fC fffF */
   sprintf( datamsg, "TS Pps   Sample         TotalTime         Obc    Sat Ymd Hms Pwr Temperature\n" );
   logit( "", "%s", datamsg );

   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
      logit( "", "TS data for DAQ #%d:\n", i );

      readyTimeStamps = AtodTimeStampsReady[i];

      for ( j = 0 ; j < readyTimeStamps ; j++ )
      {
        /* Prepare ts string
         *******************/
         SrDatUsbTemperatureCompute( AtodTimeStamps[j].Temperature, &tempC, &tempF );
         sprintf( datamsg, "%1d  %02d  %10d  %17.6lf  0x%08X  %2d  %1d   %1d   %1d  %5.1fC %3.0fF\n",
                  j,
                  AtodTimeStamps[j].PpsCount,
                  AtodTimeStamps[j].Sample,
                  AtodTimeStamps[j].TotalTime,
                  AtodTimeStamps[j].ObcCount,
                  AtodTimeStamps[j].NumSat,
                  AtodTimeStamps[j].YmdSource,
                  AtodTimeStamps[j].HmsSource,
                  AtodTimeStamps[j].PowerInfo,
                  tempC,
                  tempF
                );
         logit( "", "%s", datamsg );

      } /* for j < 2 */

   } /* for i < AtodNumBoards */


   if ( Debug >= 6 ) logit( "", "Leaving  sr_log_ts_data\n" );
   return;
}

/******************************************************************************
 *  Function: sr_log_sample_data                                              *
 *  Purpose:  Display the analog data for all DAQ boards.                     *
 ******************************************************************************/
void sr_log_sample_data( void )
{
   int  i, j, readySamples;
   char datamsg[256], *obcstr;

   if ( Debug >= 6) logit( "", "Starting  sr_log_sample_data\n" );

   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
      logit( "", "Data for DAQ #%d:\n", i );
      if ( AtodXchModel[i] == SRDAT_ATODMODEL_USB8CH )
         logit( "", " Pt   TotalPt       Chan 0     Chan 1     Chan 2     Chan 3     Chan 4     Chan 5     Chan 6     Chan 7    Dig      OBC    SampleTime\n" );
      else if ( AtodXchModel[i] == SRDAT_ATODMODEL_USB4CH )
         logit( "", " Pt   TotalPt       Chan 0     Chan 1     Chan 2     Chan 3    Dig      OBC    SampleTime\n" );

      readySamples = AtodSamplesReady[i];

      for ( j = 0 ; j < readySamples ; j++ )
      {
        /* Prepare string to highlight sample with OBC
         *********************************************/
         if (AtodSamples[j].OnBoardCount == 0x0 )
            obcstr = "  ";
         else
            obcstr = "<<";


        /* Prepare data string appropriate for each type of USBxCH
         *********************************************************/
         if ( AtodXchModel[i] == SRDAT_ATODMODEL_USB8CH )
            sprintf( datamsg,"%3d (%8ld): %10ld %10ld %10ld %10ld %10ld %10ld %10ld %10ld   %04X %08lX %s %lf\n",
                     j, AtodCurrentPt+j,
                     AtodSamples[j].Channel[0],
                     AtodSamples[j].Channel[1],
                     AtodSamples[j].Channel[2],
                     AtodSamples[j].Channel[3],
                     AtodSamples[j].Channel[4],
                     AtodSamples[j].Channel[5],
                     AtodSamples[j].Channel[6],
                     AtodSamples[j].Channel[7],
                     AtodSamples[j].DigitalIn,
                     AtodSamples[j].OnBoardCount,
                     obcstr,
                     AtodSamples[j].SampleTime
                    );

         else if ( AtodXchModel[i] == SRDAT_ATODMODEL_USB4CH )
            sprintf( datamsg,"%3d (%8ld): %10ld %10ld %10ld %10ld   %04X %08lX %s %lf\n",
                     j, AtodCurrentPt+j,
                     AtodSamples[j].Channel[0],
                     AtodSamples[j].Channel[1],
                     AtodSamples[j].Channel[2],
                     AtodSamples[j].Channel[3],
                     AtodSamples[j].DigitalIn,
                     AtodSamples[j].OnBoardCount,
                     obcstr,
                     AtodSamples[j].SampleTime
                    );

         logit( "", "%s", datamsg );

      } /* for j < readySamples */

   } /* for i < AtodNumBoards */


   if ( Debug >= 6 ) logit( "", "Leaving  sr_log_sample_data\n" );
   return;
}

/******************************************************************************
 *  Function: sr_log_status_data                                              *
 *  Purpose:  Display the bounding status (GPS) data.                         *
 ******************************************************************************/
void sr_log_status_data( void )
{
   int  i, j, k, startmsg, dramEmpty, dramPartFull, dramOverflow, readyStatus;
   char datamsg[256];

   if ( Debug >= 6) logit( "", "Starting  sr_log_status_data\n" );

   for ( i = 0 ; i < AtodNumBoards ; i++ )
   {
      logit( "", "Status data for DAQ #%d:\n", i );

      readyStatus = AtodStatusReady[i];

      for ( j = 0 ; j < readyStatus ; j++ )
      {
        /* Prepare ts string
         *******************/
         SrUsbXchDramFlagsSplit( AtodStatus[j].DramFlags,
                                 &dramEmpty, &dramPartFull, &dramOverflow );
         sprintf( datamsg, "%1d Pt = %-10d  Pwr = %1d  DramEFV = %1d/%1d/%1d  Temp = 0x%04lX  Nsat = %02d\n",
                  j,
                  AtodStatus[j].CurrentPt,
                  AtodStatus[j].PowerInfo,
                  dramEmpty,
                  dramPartFull,
                  dramOverflow,
                  AtodStatus[j].Temperature,
                  AtodStatus[j].NmeaInfo.Nsat
                );
         logit( "", "%s", datamsg );

         sprintf( datamsg, "%1d Date = %04d/%02d/%02d  Time = %02d:%02d:%02d.%06ld  Sec1970 = %17.6lf\n",
                  j,
                  AtodStatus[j].NmeaInfo.Year,
                  AtodStatus[j].NmeaInfo.Month,
                  AtodStatus[j].NmeaInfo.Day,
                  AtodStatus[j].NmeaInfo.Hour,
                  AtodStatus[j].NmeaInfo.Minute,
                  AtodStatus[j].NmeaInfo.Second,
                  AtodStatus[j].NmeaInfo.MicroSecond,
                  AtodStatus[j].NmeaInfo.SecSince1970
                );
         logit( "", "%s", datamsg );
         
         sprintf( datamsg, "%1d Src Y=%1X H=%1X S=%1X L=%1X P=%1X    Valid Y=%1d H=%1d S=%1d L=%1d P=%1d C=%1d\n",
                  j,
                  AtodStatus[j].NmeaInfo.YmdSource,
                  AtodStatus[j].NmeaInfo.HmsSource,
                  AtodStatus[j].NmeaInfo.SatSource,
                  AtodStatus[j].NmeaInfo.LocSource,
                  AtodStatus[j].NmeaInfo.PosSource,
                  AtodStatus[j].NmeaInfo.YmdIsValid,
                  AtodStatus[j].NmeaInfo.HmsIsValid,
                  AtodStatus[j].NmeaInfo.SatIsValid,
                  AtodStatus[j].NmeaInfo.LocIsValid,
                  AtodStatus[j].NmeaInfo.PosIsValid,
                  AtodStatus[j].NmeaInfo.SecIsValid
                );
         logit( "", "%s", datamsg );

         sprintf( datamsg, "%1d Location LatLongAlt %10.4lf %10.4lf %10.4lf\n",
                  j,
                  AtodStatus[j].NmeaInfo.Latitude,
                  AtodStatus[j].NmeaInfo.Longitude,
                  AtodStatus[j].NmeaInfo.Altitude
                );
         logit( "", "%s", datamsg );

         sprintf( datamsg, "%1d NmeaCount %02d\n", j, AtodStatus[j].NmeaCount );
         logit( "", "%s", datamsg );
         
         startmsg = 0;
         for ( k = 0 ; k < AtodStatus[j].NmeaCount ; k++ )
         {
            logit( "", "  %s", &AtodStatus[j].NmeaMsg[startmsg] );
            startmsg += SRDAT_NMEA_MAX_SIZE;
         } /* for k < NmeaCount */
        
      } /* for j < 2 */

   } /* for i < AtodNumBoards */


   if ( Debug >= 6 ) logit( "", "Leaving  sr_log_status_data\n" );
   return;
}

/******************************************************************************
 *  Function: sr_log_gap_details                                              *
 *  Purpose:  Gaps in the data can cause problems for downstream programs.    *
 *            If one occurs, we want to log lots of details about it so we    *
 *            track it down and eliminate it in the future.                   *
 ******************************************************************************/
void sr_log_gap_details( int thisMethod, double expectedStart, double atodEndTime )
{
   long    atodEndPt;
   double  des, dse, dss;

   if ( Debug >= 6) logit( "", "Starting  sr_log_gap_details\n" );

   if ( Debug >= 3 )
   {

      logit( "", "Gap Details:\n" );

      atodEndPt  = AtodCurrentPt + AtodSamplesReady[0] - 1;
      logit( "", "  Current StartPt = %ld, EndPt = %ld\n",
              AtodCurrentPt, atodEndPt );


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

/******************************************************************************
 *  Function: sr_pakfile_write_when_full                                      *
 *  Purpose:  To collect up and save any recently acquired packets until      *
*             the requested number of PacketsPerFile is available.  Once      *
*             enought packets are available, then write them out.             *
 ******************************************************************************/
void sr_pakfile_write_when_full( void )
{
   int k;

  /* Quick exit if writing of SR PAK files was not requested
   *********************************************************/
   if ( !WritingPak )
      return;

  /* Save a copy of the newly read packets until there are enough to output
   ************************************************************************/
   for ( k = 0 ; k < AtodPacketBufferCount ; k++ )
   {
      FilePacketBuffer[FilePacketBufferCount] = AtodPacketBuffer[k];
      FilePacketBufferCount++;

      if ( FilePacketBufferCount >= PacketsPerFile )
      {
         sr_pakfile_write( PacketsPerFile );
         FilePacketBufferCount = 0;
      }

      if ( FilePacketBufferCount >= FilePacketBufferMaxCount )
      {
         logit( "", "Fatal Error: FilePacketBufferCount %d >= MaxCount %d, Pack/File %d\n",
                FilePacketBufferCount, FilePacketBufferMaxCount, PacketsPerFile );
         sr_terminate();
      }

   } /* end for k < AtodPacketBufferCount */

}

/******************************************************************************
 *  Function: sr_pakfile_write                                                *
 *  Purpose:  Write to the open binary file for storing acquired data as raw  *
 *            USB packets.                                                    *
 ******************************************************************************/
void sr_pakfile_write( int npackets )
{
   unsigned long  totalBytes;
   char           dataFileName[MAX_STR];
   int            fh;

   if ( Debug >= 7 )  logit( "", "Entering sr_pakfile_write\n" );

   
  /* Compute the output file size in bytes
   ***************************************/
   totalBytes = npackets * sizeof(SRUSBXCH_PACKET);


  /* Form the YMDHMS/nnnnnnnn.pak sequential output file name
   **********************************************************/
   sprintf( dataFileName, "%s/%08lu.pak", YMDHMS, PakFilesWritten );
   if ( Debug >= 7 )  logit( "", "Writing file: %s ... \n", dataFileName );



  /* Open the output file
   **********************/
   fh = OsOpen( dataFileName, OFLAGS, SFLAGS );
   if ( fh == -1 )
   {
      logit( "e", "srusb2ew: Could not open file: %s", dataFileName );
      sr_terminate();
   }


  /* Write out the FilePacketBuffer array
   **************************************/
   if ( OsWrite( fh, FilePacketBuffer, totalBytes ) != totalBytes )
   {
      logit( "e", "srusb2ew: sr_pakfile_write failed to write out all requested data." );
      sr_terminate();
   }


  /* Close the output file
   ***********************/
   OsClose( fh );


  /* Increment the number of files written
   ***************************************/
   PakFilesWritten++;

   
   if ( Debug >= 7 )  logit( "", "Leaving  sr_write_pak\n" );
}

/******************************************************************************
 *  Function: sr_pakfile_mkdir                                                *
 *  Purpose:  Make date/time based directory for storing output .PAK files.   *
 ******************************************************************************/
void sr_pakfile_mkdir( void )
{
   time_t utcTimeInSecondsSince_1970;
   struct tm *locTime;


  /* Get the current time
   **********************/
                  time( &utcTimeInSecondsSince_1970 );
   locTime = localtime( &utcTimeInSecondsSince_1970 );


  /* Prepare the directory name: YYYY-MM-DD-at-HH-MM-SS
   * YMDHMS = ( year, month, day, hours, minuts, seconds)
   *****************************************************/
   sprintf( YMDHMS, "%04d-%02d-%02d-at-%02d-%02d-%02d",
            locTime->tm_year+1900,
            locTime->tm_mon+1,
            locTime->tm_mday,
            locTime->tm_hour,
            locTime->tm_min,
            locTime->tm_sec
          );

  /* Make the directory
   ********************/
   if ( SrMkDir( YMDHMS ) == -1 /* == failure */ )
   {
      logit( "e", "srusb2ew: sr_pakfile_mkdir unable to create YMDHMS output data directory." );
      sr_terminate();
   }

}

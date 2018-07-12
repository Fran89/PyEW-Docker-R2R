/***********************************************************************
  adsendxs.c

  Earthworm digitizer using National Instruments X-series DAQ modules
  and Trimble TSIP GPS units.  Uses a 1-PPS pulse from the GPS to trigger
  collection of N samples, using NI Retrigerable Analog Input mode.
  This program replaces the adsend program, which works with an older-
  an older-model NI PCI-6040E board configured for +-2.5V analog signals.

  Requirements:

   1. NI X-series DAQ device.  PCI-Express and USB models are available.
      Program tested with a model NI PCIe-6343 DAQ board.  Inputs are
      connected through SCB-68 patch panels.

   2. GPS receiver/clock capable of producing a 1-PPS output on the
      second.  Configuration and time access are provided by RS-232
      serial port, using TSIP protocol.  Program tested with a Trimble
      ThunderBolt E GPS Disciplined Clock.

  Features and Notes:

   1. NI X-series modules are available with 16 or 32 input channels.
      To achieve a higher channel count, more than one NI module may
      be connected to the same computer.

   2. NI modules were tested in NSRE mode, in which input voltages are
      referenced to AISENSE (pin 62).  We jumper AISENSE to AIGND (pin 29).

   3. The T-Bolt E contains a double-ovenized, disciplined clock that
      will survive GPS signal losses of days or weeks with no
      appreciable loss of timing accuracy.

   4. Sample rate is configurable.  Tested sample rates are 100, 200,
      500, and 1000 sps.  Higher sample rates are possible but limited
      by the maximum tracebuf message length of 4096 bytes.

   5. Tracebuf messages are all one-second in length, and they start
      on the second.

   6. Program will optionally set PC system clock to GPS time.

   7. Program creates TYPE_SNW messages for loss-of-satellite synch
      and GPS antenna open.  These can be converted to files using
      the Earthworm ew2file program.  The snwclient program can pick
      up these files and send them to a SeisnetWatch server.

   8. Tested with NI-DAQmx 9.2.3

   Version Numbers:
   1.1  Uses defined function DAQmxErrChk()
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/timeb.h>
#include <NIDAQmx.h>

#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <time_ew.h>
#include "adsendxs.h"

/* Macro definition to handle error returns from NI function calls.
   If the function fails, the program goes to the Error lablel and then exits.
   **************************************************************************/
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

/* Function declarations
   *********************/
int   GetGpsTime( time_t *tgps, int *antennaOpen );
int   OpenPort( int ComPort, int BaudRate, int ReadFileTimeout );
void  ClosePort( void );
int   DisableAutoPackets( void );
int   SetUtcMode( void );
int   GetPpsOutputStatus( void );
int   GetConfig( char *configfilename );
void  LogConfig( void );
void  SendHeartbeat( void );
void  ReportErrorToStatmgr( int errNum, char *errmsg );
void  FloatToShort( int Nchan, uInt32 nSamples, short *idata );
void  ReportTimeSinceLastLockToSNW( time_t tgps, int gpsTimeStatus, FILE *gpsLockFile );
void  ReportAntennaStatusToSNW( int antennaOpen );
void  SetPcClock( time_t tgps );
int32 CVICALLBACK EveryNCallback( TaskHandle taskHandle, int32 everyNsamplesEventType,
                                  uInt32 nSamples, void *callbackData );
int32 CVICALLBACK DoneCallback( TaskHandle taskHandle, int32 status, void *callbackData );

/* Declared in getconfig.c
   ***********************/
extern unsigned char ModuleId;     // Data source id placed in trace header
extern long    OutKey;             // Key to ring where traces will live
extern int     Nchan;              // Number of channels to digitize
extern int     SampRate;           // Samples per second per channel
extern SCNL    *ChanList;          // Array to fill with SCNL values
extern char    TriggerChan[];      // Physical channel where trigger is found
extern int     ComPort;            // Com port for reading GPS time
extern int     BaudRate;           // Baud rate of com port
extern int     GetGpsDelay;        // Wait this many msec before requesting GPS time
extern int     UpdatePcClock;      // If 1, update PC clock from GPS
extern int     PcClockUpdInt;      // Seconds between PC clock updates from GPS
extern int     Debug;              // If > 0, log GPS flags
extern int     ReadFileTimeout;    // ReadFile times out in this many milliseconds.

/* Variables global to this file
   *****************************/
static float64  *fldata;            // NIDAQmx routines fill this buffer
static char     *traceBuf;          // Where the trace message is assembled
static TRACE2_HEADER *traceHead;    // Where the trace header is stored
static short    *traceDat;          // Where the data points are stored in the trace msg
static int      traceBufSize;       // In bytes, including tracebuf header
static MSG_LOGO tbufLogo;           // Logo of tracebuf message to put out
static FILE     *gpsLockFile;       // Contains time of last GPS lock
static int      antennaOpenPrev = UNKNOWN;
static int      EveryNCallbackRequestedExit = FALSE;
static int      DoneCallbackRequestedExit = FALSE;

/* Share these with other files
   ****************************/
SHM_INFO        Region;              // Structure for shared-memory region
pid_t           MyPid;               // Process id, sent to statmgr with heartbeat
MSG_LOGO        snwLogo;             // Logo of SeisnetWatch message to be sent


/* The main program starts here
   ****************************/
int main( int argc, char *argv[] )
{
   int32         error = 0;
   char          errBuff[2048]={'\0'};
   TaskHandle    taskHandle=0;       // For NI callback task
   unsigned char InstId;             // Installation id placed in trace header
   int           chan;               // Channel number

/* Get command line arguments
   **************************/
   if ( argc < 2 )
   {
      printf( "Usage: adsendxs <config file>\n" );
      return -1;
   }

/* Initialize name of log file and open it
   ***************************************/
   logit_init( argv[1], 0, 1024, 1 );

/* Read and log configuration parameters
   *************************************/
   if ( GetConfig( argv[1] ) < 0 )
   {
      logit( "e", "GetConfig() error.  Exiting.\n" );
      return -1;
   }
   LogConfig();

/* Get our process id for restart purposes
   ***************************************/
   MyPid = _getpid();
   if( MyPid == -1 )
   {
      logit( "e", "Error. Cannot get PID. Exiting.\n" );
      return -1;
   }

/* Get logos of outgoing waveform and SNW messages
   ***********************************************/
   if ( GetLocalInst( &InstId ) < 0 )
   {
      logit( "e", "Error getting local installation id. Exiting.\n" );
      return -1;
   }
   logit( "t", "Local installation id: %u\n", InstId );

   tbufLogo.instid = InstId;
   tbufLogo.mod    = ModuleId;
   if ( GetType( "TYPE_TRACEBUF2", &tbufLogo.type ) == -1 )
   {
      logit( "e", "Error getting type code TYPE_TRACEBUF2. Exiting.\n" );
      return -1;
   }

   snwLogo.instid = InstId;
   snwLogo.mod    = ModuleId;
   if ( GetType( "TYPE_SNW", &snwLogo.type ) == -1 )
   {
      logit( "e", "Error getting type code TYPE_SNW. Exiting.\n" );
      return -1;
   }

/* Allocate some array space.
   fldata = Buffer for 64-bit float data from NIDAQmx
   traceBuf = Earthworm message buffer
   **************************************************/
   if ( (fldata = malloc(Nchan * SampRate * sizeof(float64))) == NULL )
   {
      logit( "e", "Error allocating fldata array.  Exiting.\n" );
      return 0;
   }
   traceBufSize = sizeof(TRACE2_HEADER) + (SampRate * sizeof(short));
   logit( "t", "Trace buffer size: %d bytes\n", traceBufSize );
   traceBuf = (char *) malloc( traceBufSize );
   if ( traceBuf == NULL )
   {
      logit( "e", "Error: Cannot allocate trace buffer. Exiting.\n" );
      return -1;
   }
   traceHead = (TRACE2_HEADER *) &traceBuf[0];
   traceDat  = (short *) &traceBuf[sizeof(TRACE2_HEADER)];

/* Open file containing time of last GPS lock to satellites.
   File will be opened for updating (reading and writing).
   In mode r+, the file must exist.
   In mode w+, a new empty file is created.
   ********************************************************/
   if ( fopen_s( &gpsLockFile, "gps_lock_file", "r+" ) != 0 )
   {
      logit( "et", "Error opening gps_lock_file.  Creating a new lock file.\n" );
      if ( fopen_s( &gpsLockFile, "gps_lock_file", "w+" ) != 0 )
      {
         logit( "et", "Error creating gps_lock_file.  Exiting.\n" );
         return -1;
      }
   }

/* Attach to Earthworm transport ring
   **********************************/
   tport_attach( &Region, OutKey );
   logit( "et", "Attached to Earthworm transport ring: %d\n", OutKey );

/* Send a heartbeat to statmgr before attempting to communicate
   with the GPS.  After receiving the first heartbeat from
   adsendxs, statmgr will be able to restart adsendxs.
   ************************************************************/
   SendHeartbeat();

/* Initialize the PC com port that will talk to the Trimble GPS
   ************************************************************/
   logit( "et", "Opening PC com port COM%d for GPS communications.\n", ComPort );
   if ( OpenPort(ComPort, BaudRate, ReadFileTimeout) < 0 )
   {
      logit( "et", "OpenPort error on GPS com port.  Exiting.\n" );
      return -1;
   }

/* Disable automatic output packets from the GPS.  By default,
   the primary and supplemental timing packets are sent after
   every one-second pulse.
   **********************************************************/
   logit( "et", "Disabling automatic packet output from GPS.\n" );
   if ( DisableAutoPackets() < 0 )
   {
      ClosePort();
      logit( "et", "DisableAutoPackets failed.\n" );
      logit( "et", "Check serial cable and communication rate.\n" );
      logit( "et", "Check overall GPS function with Trimble Studio.\n" );
      logit( "et", "Exiting.\n" );
      return -1;
    }

/* Get 1-PPS status byte.
   *********************/
   logit( "et", "Getting 1-PPS status byte.\n" );
   if ( GetPpsOutputStatus() < 0 )
   {
      logit( "et", "GetPpsOutputStatus error.\n" );
      ClosePort();
      return -1;
   }

/* Put the GPS into UTC mode.  This will cause leap seconds
   to be applied to GPS time, and the one-PPS pulse will be
   referenced to UTC time.  This feature is stored in NV
   memory, so it will survive a GPS reboot or power cycle.
   ********************************************************/
   logit( "et", "Putting GPS into UTC mode.\n" );
   if ( SetUtcMode() < 0 )
   {
      logit( "et", "SetUtcMode error.\n" );
      ClosePort();
      return -1;
   }

/* Start setting up the X-series DAQ board.
   First, create an analog input task.
   ***************************************/
   logit( "et", "Creating the NI analog input task.\n" );
   DAQmxErrChk( DAQmxCreateTask("AnalogInputTask", &taskHandle) );

/* Use non-referenced, single-ended (NRSE) inputs.
   Set up min and max input voltages for each channel.
   **************************************************/
   logit( "et", "Setting up NI NRSE inputs for each channel to digitize.\n" );
   for ( chan = 0; chan < Nchan; chan++ )
   {
      DAQmxErrChk( DAQmxCreateAIVoltageChan( taskHandle,
           ChanList[chan].daqChan,    // Physical channel name
           "",                        // Virtual name to assign to channel
           DAQmx_Val_NRSE,            // Input terminal config for channel
           -2.5,                      // Min voltage you expect to measure
           +2.5,                      // Max voltage you expect to measure
           DAQmx_Val_Volts,           // Values are returned in volts
           NULL) );                   // Custom scale name (not used)
   }

/* Specify sample clock timing
   ***************************/
   logit( "et", "Configuring NI sample clock timing.\n" );
   DAQmxErrChk( DAQmxCfgSampClkTiming( taskHandle,
           NULL,                       // Use on-board clock
           (float64)SampRate,          // Sampling rate
           DAQmx_Val_Rising,           // Acquire on rising edge of sample clock
           DAQmx_Val_FiniteSamps,      // Specify finite data collection
           (uInt64)SampRate) );        // Samples per channel to acquire

/* Specify the physical channel of the start trigger for the AI task.
   This channel is connected to the 1-pps output from the GPS receiver.
   Make the analog input task retriggerable.  X-series NI boards
   support retriggerable AI tasks, but E and M-series boards do not.
   *******************************************************************/
   logit( "et", "Configuring the NI digital edge start trigger.\n" );
   DAQmxErrChk( DAQmxCfgDigEdgeStartTrig( taskHandle,
        TriggerChan,                    // Physical channel, eg PFI1
        DAQmx_Val_Rising) );            // Trigger on rising edge

   logit( "et", "Making the NI start trigger retriggerable.\n" );
   DAQmxErrChk( DAQmxSetStartTrigRetriggerable(taskHandle, TRUE) );

/* The EveryNCallback function is invoked after SampRate samples are acquired.
   **************************************************************************/
   logit( "et", "Registering the NI acquisition callback function.\n" );
   DAQmxErrChk(  DAQmxRegisterEveryNSamplesEvent( taskHandle,
        DAQmx_Val_Acquired_Into_Buffer,  // Data are being acquired
        (uInt32)SampRate,                // After SampRate samples are collected,
                                         //    the callback event will occur.
        0,                               // Function called in a DAQmx thread
        EveryNCallback,                  // Name of callback function
        NULL) );                         // Value passed to the EveryNCallback function

/* The DoneCallback function is invoked if the analog
   input task stops due to an error.
   **************************************************/
   logit( "et", "Registering the NI done-event callback function.\n" );
   DAQmxErrChk( DAQmxRegisterDoneEvent( taskHandle,
        0,                      // Function called in a DAQmx thread
        DoneCallback,           // Name of callback function
        NULL) );                // Value passed to the callback function

/* Start the analog input task
   ***************************/
   logit( "et", "Starting the analog input task.\n" );
   DAQmxErrChk( DAQmxStartTask(taskHandle) );
   logit( "et", "The analog input task is started.\n" );

/* Sleep until one of the callback functions fails
   or startstop/statmgr requests program termination
   *************************************************/
   while ( 1 )
   {
      if ( EveryNCallbackRequestedExit )
      {
         logit( "et", "EveryNCallback function requested program exit.\n" );
         break;
      }
      if ( DoneCallbackRequestedExit )
      {
         logit( "et", "DoneCallback function requested program exit.\n" );
         break;
      }
      if ( tport_getflag( &Region ) == TERMINATE  ||
           tport_getflag( &Region ) == MyPid )
      {
         logit( "et", "Startstop/statmgr requested program exit.\n" );
         break;
      }
      sleep_ew( 100 );
   }

/* Exit program
   ************/
Error:
   if ( DAQmxFailed(error) )
           DAQmxGetExtendedErrorInfo( errBuff,2048 );
   if ( taskHandle!=0 )
   {
      DAQmxStopTask( taskHandle );
      DAQmxClearTask( taskHandle );
   }
   if ( DAQmxFailed(error) )
      logit( "et", "DAQmx Error: %s\n", errBuff );
   logit( "et", "Exiting adsendxs.\n" );
   return 0;
}


/**************************************************************
  EveryNCallback

  This function is called after nSamples samples are read
  from the NIDAQmx device.
 **************************************************************/

int32 CVICALLBACK EveryNCallback( TaskHandle taskHandle,
                                  int32 everyNsamplesEventType,
                                  uInt32 nScansRequested,
                                  void *callbackData )
{
   int32  error = 0;
   int    i;
   char   errBuff[2048] = {'\0'};       // Message returned by NIDAQmx
   char   errMsg[80];                   // Message sent to statmgr
   int32  nScansRead;
   short  *idata = (short *)fldata;
   time_t tgps;
   int    gpsTimeStatus;                // Possible values are CRITICAL ERROR,
                                        // GPS TIME_UNAVAILABLE,
                                        // UNLOCKED_GPS_TIME_AVAILABLE,
                                        // or LOCKED_GPS_TIME_AVAILABLE
   int    antennaOpen;                  // Possible values are TRUE, FALSE, or UNKNOWN
   static int sendingTracebuf = FALSE;  // Sending tracebuf messages

/* Read DAQ samples in floating-point format.
   A read error will occur if data processing falls behind
   the collection rate, causing the program to exit.
   *******************************************************/
   DAQmxErrChk( DAQmxReadAnalogF64( taskHandle,
        -1,                           // Read all samples that were requested
        0.0,                          // Timeout in seconds.  If 0, read all
                                      // available samples and return immediately.
        DAQmx_Val_GroupByScanNumber,  // Multiplexed data
        fldata,                       // f64 array to contain data
        Nchan*nScansRequested,        // Size of fldata array, in samples
        &nScansRead,                  // Samples per channel read
        NULL) );                      // Reserved by NI

/* Did we get all data requested from the DAQ module?
   *************************************************/
   if ( (uInt32)nScansRead < nScansRequested )
   {
      logit( "et", "Error.  We requested %u scans, but\n", nScansRequested );
      logit( "et", "DAQmxReadAnalogF64 returned only %d scans.\n", nScansRead );
      DAQmxStopTask( taskHandle );
      DAQmxClearTask( taskHandle );
      EveryNCallbackRequestedExit = TRUE;
      return 0;
   }

/* Convert floating-point volts to short integers,
   to emulate output from an NI PCI-6040E board.
   **********************************************/
   FloatToShort( Nchan, nScansRead, idata );

/* Sleep until reliable timing packets are available
   *************************************************/
   sleep_ew( GetGpsDelay );

/* Get time from GPS clock.  Possible return values are:
   CRITICAL ERROR, GPS TIME_UNAVAILABLE,
   UNLOCKED_GPS_TIME_AVAILABLE, or LOCKED_GPS_TIME_AVAILABLE
   *********************************************************/
   gpsTimeStatus = GetGpsTime( &tgps, &antennaOpen );

   if ( gpsTimeStatus == CRITICAL_ERROR )
   {
      logit( "et", "Critical error detected by GetGpsTime.\n" );
      DAQmxStopTask( taskHandle );
      DAQmxClearTask( taskHandle );
      EveryNCallbackRequestedExit = TRUE;
      return 0;
   }

/* Notify someone if changes occur in the antenna-open
   status.  Send antenna status to SNW periodically.
   ***************************************************/
   if ( (antennaOpen == TRUE) && (antennaOpenPrev != TRUE) )
   {
      sprintf_s( errMsg, ERRMSGSIZE, "GPS antenna open." );
      logit( "et", "%s\n", errMsg );
      ReportErrorToStatmgr( GPS_ERROR, errMsg );
   }
   if ( (antennaOpen != TRUE) && (antennaOpenPrev == TRUE) )
   {
      sprintf_s( errMsg, ERRMSGSIZE, "GPS antenna not open." );
      logit( "et", "%s\n", errMsg );
      ReportErrorToStatmgr( GPS_ERROR, errMsg );
   }
   antennaOpenPrev = antennaOpen;
   ReportAntennaStatusToSNW( antennaOpen );

/* Log if we start or stop sending tracebuf messages
   *************************************************/
   if ( gpsTimeStatus == GPS_TIME_UNAVAILABLE )
   {
      if ( sendingTracebuf == TRUE )
         logit( "et", "Stopped sending tracebuf messages to transport ring.\n" );
      sendingTracebuf = FALSE;
      return 0;                // Wait for next callback to occur
   }
   else
   {
      if ( sendingTracebuf == FALSE)
         logit( "et", "Sending tracebuf messages to transport ring.\n" );
      sendingTracebuf = TRUE;
   }

/* We got time from the GPS clock, although the clock may not
   be locked to the satellites.  Report time since GPS was last
   locked to SNW.  Then, update gpsLockFile if GPS clock is
   currently locked to satellites.
   ************************************************************/
   ReportTimeSinceLastLockToSNW( tgps, gpsTimeStatus, gpsLockFile );

/* Set PC clock to GPS time
   ************************/
   if ( UpdatePcClock )
      if ( (tgps % PcClockUpdInt) == 0 )
      {
         SetPcClock( tgps );
         logit( "et", "PC clock synched to GPS time.\n" );
      }

/* Fill in tracebuf start and end times, assuming first sample
   of tracebuf message occurred at time of previous trigger
   pulse from GPS.  Then, fill in tracebuf header with other
   stuff that's the same for all channels.
   ***********************************************************/
   traceHead->starttime  = tgps - 1.0;
   traceHead->endtime    = traceHead->starttime + (nScansRead - 1)/(double)nScansRead;
   traceHead->nsamp      = nScansRead;          // Number of samples in tbuf msg
   traceHead->samprate   = (double)nScansRead;  // Sample rate
   traceHead->version[0] = TRACE2_VERSION0;     // Header version number
   traceHead->version[1] = TRACE2_VERSION1;     // Header version number
   traceHead->quality[0] = 0;                   // Not used
   traceHead->quality[1] = 0;                   // Not used
   strcpy_s( traceHead->datatype, DATATYPESIZE, "i2" );  // Data format code (short integer)

/* Send one tracebuf packet per channel to the EW transport ring
   *************************************************************/
   for ( i = 0; i < Nchan; i++ )
   {
      int rc;
      int index = i;
      int j;

/* Put the SCNL and pin number into the traceBuf header
   ****************************************************/
      strcpy_s( traceHead->sta, STASIZE,   ChanList[i].sta );  // Site name
      strcpy_s( traceHead->net, NETSIZE,   ChanList[i].net );  // Network name
      strcpy_s( traceHead->chan, COMPSIZE, ChanList[i].comp ); // Component code
      strcpy_s( traceHead->loc, LOCSIZE,   ChanList[i].loc );  // Location code
      traceHead->pinno = ChanList[i].pin;          // Pin number

/* Demux data samples and copy to the traceBuf message
   ***************************************************/
      for ( j = 0; j < nScansRead; j++ )
      {
         traceDat[j] = idata[index];
         index += Nchan;
      }

/* Send tracebuf message to transport ring
   ***************************************/
      rc = tport_putmsg( &Region, &tbufLogo, traceBufSize, traceBuf );
      if ( rc == PUT_TOOBIG )
         logit( "", "Tracebuf message for channel %d too big\n", i );
      if ( rc == PUT_NOTRACK )
         logit( "", "Tracking error while sending channel %d\n", i );

/* Send a heartbeat to statmgr if tport_putmsg() succeeded
   and HeartbeatInt seconds have elapsed since the previous
   heartbeart was sent.
   ********************************************************/
      if ( rc == PUT_OK ) SendHeartbeat();
   }

Error:
   if ( DAQmxFailed(error) )
   {
      DAQmxGetExtendedErrorInfo( errBuff, 2048 );
      logit( "et", "DAQmx error detected by EveryNCallback: %s\n", errBuff);
      ReportErrorToStatmgr( NI_ERROR, "EveryNCallback error.\n" );
      DAQmxStopTask( taskHandle );
      DAQmxClearTask( taskHandle );
      EveryNCallbackRequestedExit = TRUE;
   }
   return 0;
}


/**************************************************************
  DoneCallback

  This function is called if an error causes the analog task
  to stop.
 **************************************************************/

int32 CVICALLBACK DoneCallback( TaskHandle taskHandle,
                                int32      status,
                                void       *callbackData )
{
   int32 error = 0;
   char  errBuff[2048] = {'\0'};

// See if an error stopped the task.
   DAQmxErrChk( status );

Error:
   if ( DAQmxFailed( error ) )
   {
      DAQmxGetExtendedErrorInfo( errBuff, 2048 );
      logit( "et", "DAQmx error detected by DoneCallback: %s\n", errBuff);
      DAQmxClearTask( taskHandle );
      DoneCallbackRequestedExit = TRUE;
   }
   return 0;
}


/*******************************************************************
  FloatToShort

  NI X-series devices collect data in floating-point volts.
  Here we convert these values to short integers which emulate
  output from a PCI-6040E DAQ board.
  For a PCI-6040E with gain setting of 2, the input range is +-2.5
  volts, and the digital samples are set to 819.0 * volts.
  The minimum sample value is -2048 at -2.5 volts input.
  The maximum sample value is 2047 at +2.5 volts input.
  Sample counts are clipped if the absolute value of the input
  voltage is > 2.5.
 *******************************************************************/
#define ROUND(x) (((x)<0.0) ? ((x)-0.5) : ((x)+0.5))

void FloatToShort( int Nchan, uInt32 nSamples, short *idata )
{
   unsigned i;

   for ( i = 0; i < Nchan*nSamples; i++ )
   {
      if ( fldata[i] < -2.5 )
         idata[i] = -2048;
      else if ( fldata[i] >= +2.5 )
         idata[i] = +2047;
      else
         idata[i] = (short) ROUND(819.0 * fldata[i]);
   }
   return;
}

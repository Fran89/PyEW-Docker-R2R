/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: adsend.c 1423 2004-04-22 17:20:04Z kohler $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2004/04/22 17:20:04  kohler
 *     Inserted version number in TRACE2_HEADER message headers.  WMK 4/22/04
 *
 *     Revision 1.9  2004/04/21 18:53:34  kohler
 *     Fix bug in location code update.  WMK 4/21/04
 *
 *     Revision 1.8  2004/04/20 16:26:32  kohler
 *     Modified adsend to produce TYPE_TRACEBUF2 messages, which contain location
 *     codes in the headers.  The configuration file, adsend.d, must contain
 *     location codes in the channel specifications.  Also, cleaned up some error
 *     reporting.  WMK 4/20/04
 *
 *     Revision 1.7  2003/02/04 00:14:56  alex
 *     fixed comments
 *
 *     Revision 1.6  2002/07/17 23:25:13  alex
 *     tweaks to run at 500 sps. See "factor". Alex
 *
 *     Revision 1.5  2002/06/11 14:19:43  patton
 *     Made logit changes.
 *
 *     Revision 1.4  2002/03/06 01:29:29  kohler
 *     Added function prototype for irige_init().
 *
 *     Revision 1.3  2002/01/15 22:44:28  alex
 *     mod to use _ftime() rather than time() when getting time from system
 *     clock. This provides time to the millisecond rather than whole seconds only.
 *     Alex
 *
 *     Revision 1.2  2001/05/08 23:11:34  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

      /*********************************************************************
       *                              adsend.c                             *
       *                                                                   *
       *                 Digitizing program for Earthworm.                 *
       *********************************************************************/

/*                                     Story

   This is the Windows NT version of adsend, written by W Kohler in
   August, 1997.  The original adsend program, which ran under DOS, was
   written by Alex Bittenbinder in 1994.

   This program was tested with a National Instruments DAQ board (model
   PCI-MIO-16E-4) and two MUX boards (model AMUX-64T).  The DAQ board can
   be used without MUXs as a 16 channel system.  Up to four MUX boards can
   be used, for a maximum of 256 channels, 64 channels per MUX.  At least
   one channel must contain IRIGE time code, which is decoded and used to
   time-stamp the data.  Up to four "guide channels" are used to detect
   "channel-rotation", a condition in which the signals appear on the wrong
   channels. The program detects channel rotation automatically, and resets
   the DAQ system to fix the problem.

                    Differences From The Old Adsend Program

   The old program rebooted the entire PC to reset the DAQ system.  This
   program just does a software reset.  The old program reported errors
   through the picker program, which ran on a different system.  This program
   reports its own errors.  The old program attempted to update the year
   automatically  This program reads the year from its configuration file,
   and the year must be manually reset on New Year's Eve.

                               Single Threading

   The NIDAQ functions supplied with National Instruments DAQ boards may not
   be used in multi-threaded applications.  adsend is compiled single
   threaded, and the earthworm library functions are recompiled as single
   threaded (see file makefile.nt).  The program sleeps between messages to
   give some CPU time to other processes.

                    Time Stamping Using The IRIGE Time Code

   When the program starts up, it will not send data until the IRIGE time
   signal is in synch.  After this time, the program will keep on sending
   data even if time synch is lost.  This way, we keep getting data even if
   the times aren't too good.  A bit is set in the trace header to indicate
   "time code out of synch".

                      Channel Rotation Correction Feature

   Up to four channels, one from each MUX are monitored for triangle waves.
   These channels are known as "guide channels".  If a guide channel does
   not contain a triangle wave, we assume that a channel rotation has occurred
   on a MUX, and we reset the entire DAQ system.  Actually, we don't reset
   unless all guide channels have been in synch for a specified amount of time
   (eg 30 seconds).

                                Input Signals

   IRIGE code:      0-10V from a USGS master clock (used for testing)
                    0-0.5V or 0-1.0V in production system
   Analog signals:  -2.5V to +2.5V
   Guide channels:  Triangle wave; -1V to +1V; 2hz
   SCAN trigger:    Square wave; 0-5V; 100hz; connect to PFI0

                              Grounding Issues

   We are using non-referenced, single-ended mode (NRSE), in which the input
   signal's ground reference is connected to AISENSE, which is tied to the
   negative input of the instrumentation amplifier.  We have found that for
   good noise suppression, connect AISENSE to AIGND.


  Alex 4/28/98: permit message length in multiples of 100 samples

  Whitmore: 10/20/98:  1) Let system clock be updated with IRIGE time.
                       2) Let the year for time be taken from PC clock instead of "Year"

  Lombard: 11/19/98: V4.0 changes: no Y2K date problems
     1) changed argument of logit_init to the config file name.
     2) process ID in heartbeat message: already done
     3) flush input transport ring: there isn't one
     4) add `restartMe' to .desc file: already done
     5) multi-threaded logit: not applicable

  Alex: 7/5/2:
  changes to run at 500 hz for Factor building in LA. This consisted of:
        * Adding the .d parameter "Nchan". Previously, all channels from all
          specified Mux's would be digitized. This was a scary load for the
          Factor project.
        * Fixing a time-code reader bug in irige.c
   Search tag: "factor"

   WMK: 4/20/04:
   Changed waveform message type to TYPE_TRACEBUF2, to accomodate
   location codes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <conio.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <process.h>    /* for _getpid() */

#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "guidechk.h"
#include "adsend.h"
#include "irige.h"

/* Functions prototypes
   ********************/
void CombGuideChk( int *, int, int * );
int  GetArgs( int, char **, char ** );
int  GetConfig( char * );
int  GetDAQ( short *, unsigned long * );
int  GuideChk( short *, int, int, int, double *, double *, double, double );
void Heartbeat( void );
void InitCon( void );
void InitDAQ( int );
void irige_init( struct TIME_BUFF * );
int  Irige( struct TIME_BUFF *, long, int, int, short *, int, int );
void LogAndReportError( int, char * );
void LogConfig( void );
void LogSta( int, SCNL * );
void PrintGmtime( double, int );
void LogitGmtime( double, int );
int  ReadSta( char *, unsigned char, int, SCNL * );
void ReportError( int, char * );
void SetCurPos( int, int );
void StopDAQ( void );
int  NoSend( int );
int  NoTimeSynch( int );

/* Global configuration parameters
   *******************************/
extern unsigned char ModuleId;   // Data source id placed in the trace header
extern int    OnboardChans;      // The number of channels on the DAQ board
extern int    NumMuxBoards;      // Number of mux cards
extern double ChanRate;          // Rate in samples per second per channel
extern int    ChanMsgSize;       // Message size in samples per channel
extern int    NumTimeCodeChan;   // How many time-code channels there are
extern int    *TimeCodeChan;     // The chans where we find the IRIGE signals
extern int    ExtTrig;           // External trigger flag
extern double LowValue;          // The low hysteresis voltage for external triggering
extern double HighValue;         // The high hysteresis voltage for external triggering
extern long   OutKey;            // Key to ring where traces will live
extern double MinGuideSignal;    // Guides are declared dead if the mean value of
                                 //   guide 1st differences < MinGuideSignal
extern double MaxGuideNoise;     // Guides are declared dead if standard error of
                                 //   guide 1st differences > MaxGuideNoise
extern int    Year;              // The current year
extern int    NumGuide;          // Number of guide channels
extern int    *GuideChan;        // The chans where we find the reference voltage
extern int    SendBadTime;       // Send data even if no IRIGE signal is present
extern int    UpdateSysClock;    // Update PC clock with good IRIGE time
extern int    YearFromPC;        // Take year from PC instead of "Year"
extern SCNL   *ChanList;         // Array to fill with SCNL values
extern int    Nchan;             // Number of channels in ChanList
extern int    IrigeIsLocalTime;  // Set to 1 if IRIGE represents local time; 0 if GMT time.
extern int    ErrNoLockTime;     // If no guide lock for ErrNoLockTime sec, report an error
extern int    EnableBell;        // If non-zero, ring bell if no time synch or no guides

/* Global variables
   ****************/
SHM_INFO      OutRegion;         // Info structure for output region
pid_t         MyPid;             // process id, sent with heartbeat



int main( int argc, char *argv[] )
{
   int       i, j,             // Loop indexes
             sendIt,           // 1 if data is being sent, else 0.
            *GuideStat,        // Status of each guide channel, from GuideChk()
             CombGuideStat,    // Combined guide status
             traceBufSize,     // Size of the trace buffer, in bytes
             IrigeStatusPrev = TIME_UNKNOWN,  // Initially, there is no status
             tcIndex = 0,      // Index of time-code channel to decode
             tcChan,           // The time-code channel to decode
             extTrig,          // 1 for external triggering
             TimeCnt = 0,      // Counter, increments every new packet
             guideStartUp;     // 1 at startup or after restart

   short    *halfBuf,          // Pointer to half buffer of A/D data
            *traceDat;         // Where the data points are stored in the trace msg

   char     *traceBuf;         // Where the trace message is assembled

   unsigned  halfBufSize,      // Size of the half buffer in number of samples
             scan = 0;         // Scan number of first scan in a message

   unsigned char InstId;       // Installation id placed in the trace header

   MSG_LOGO  logo;             // Logo of message to put out

   TRACE2_HEADER   *traceHead; // Where the trace header is stored
   struct TIME_BUFF Tbuf;      // Irige()'s read-write structure
   SYSTEMTIME    st;           // UTC time taken from PC clock
   struct tm    *gmt;          // IRIGE time (hh:mm:ss, etc.)
   time_t        ltime;        // IRIGE time (# seconds)
   time_t        tReset;       // Time the DAQ system last restarted


/* Get command line arguments
   **************************/
   if ( argc < 2 )
   {
      printf( "Usage: adsend <config file>\n" );
      return -1;
   }

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read configuration parameters
   *****************************/
   if ( GetConfig( argv[1] ) < 0 )
   {
      logit( "", "Error reading configuration file. Exiting.\n" );
      return -1;
   }

/* Log the configuration file
   **************************/
   LogConfig();

/* Get our Process ID for restart purposes
   ***************************************/
   MyPid = _getpid();
   if( MyPid == -1 )
   {
      logit("", "Error. Cannot get PID. Exiting.\n" );
      return -1;
   }

/* Set up the logo of outgoing waveform messages
   *********************************************/
   if ( GetLocalInst( &InstId ) < 0 )
   {
      logit( "", "Error getting local installation id. Exiting.\n" );
      return -1;
   }
   logit( "", "Local InstId:         %8u\n", InstId );

   logo.instid = InstId;
   logo.mod    = ModuleId;
   if ( GetType( "TYPE_TRACEBUF2", &logo.type ) == -1 )
   {
      logit( "", "Error getting type code TYPE_TRACEBUF2. Exiting.\n" );
      return -1;
   }

/* Specify internal or external triggering according
   to the config file parameter.
   *************************************************/
   extTrig = ExtTrig;

/* Make sure that LowValue and HighValue are ok
   ********************************************/
   if ( extTrig )
   {
      if ( HighValue - LowValue < 0.1)
      {
         logit( "et", "Error: HighValue - LowValue < 0.1 volts. Exiting.\n" );
         return -1;
      }
      if ( LowValue < -10.0 )
      {
         logit( "et", "Error: LowValue < -10.0 volts. Exiting.\n" );
         return -1;
      }
      if ( HighValue > 10.0 )
      {
         logit( "et", "Error: HighValue > 10.0 volts. Exiting.\n" );
         return -1;
      }
   }

/* Allocate some array space
   *************************/
   halfBufSize = (unsigned)(ChanMsgSize * Nchan);
   logit( "t", "Half buffer size: %u samples\n", halfBufSize );

   halfBuf = (short *) calloc( halfBufSize, sizeof(short) );
   if ( halfBuf == NULL )
   {
      logit( "", "Error: Cannot allocate the A/D buffer. Exiting.\n" );
      return -1;
   }
   if ( NumGuide > 0 )
   {
      GuideStat = (int *) calloc( NumGuide, sizeof(int) );
      if ( GuideStat == NULL )
      {
         logit( "", "Error: Cannot allocate guide status array. Exiting.\n" );
         return -1;
      }
   }
   traceBufSize = sizeof(TRACE2_HEADER) + (ChanMsgSize * sizeof(short));
   logit( "t", "Trace buffer size: %d bytes\n", traceBufSize );
   traceBuf = (char *) malloc( traceBufSize );
   if ( traceBuf == NULL )
   {
      logit( "", "Error: Cannot allocate the trace buffer. Exiting.\n" );
      return -1;
   }
   traceHead = (TRACE2_HEADER *) &traceBuf[0];
   traceDat  = (short *) &traceBuf[sizeof(TRACE2_HEADER)];

/* Attach to existing transport ring and send first heartbeat
   **********************************************************/
   tport_attach( &OutRegion, OutKey );
   logit( "t", "Attached to transport ring: %d\n", OutKey );
   Heartbeat();

/* Initialize the console display
   ******************************/
   InitCon();

/* Come back to this point if the DAQ system resets.
   A reset will occur if the guide channel is corrupted.
   ****************************************************/
RESET:

/* Set flag saying we just reset the system.
   Get the time of startup or reset.
   ****************************************/
   time( &tReset );
   guideStartUp = 1;

/* Specify which time-code channel to decode
   *****************************************/
   tcChan = TimeCodeChan[tcIndex];

   SetCurPos( 4, 15 );
   if ( SendBadTime )
   {
      printf( "Time stamp from PC system clock." );
      logit( "", "Time stamp from PC system clock." );
      sendIt = TRUE;
   }
   else
   {
      printf( "Time stamp from IRIGE on pin %-3d  ", tcChan );
      logit( "", "Time stamp from IRIGE on pin %-3d  ", tcChan );
      irige_init( &Tbuf );
      sendIt = FALSE;
   }

/* Display the type of triggering we are using
   *******************************************/
   SetCurPos( 4, 17 );
   if ( extTrig )
      printf( "Scans are externally triggered." );
   else
      printf( "Scans are internally triggered." );

/* Initialize the National Instruments DAQ system
   **********************************************/
   InitDAQ( extTrig );                      // In nidaq_ew.c


   /************************* The main program loop   *********************/

   while ( tport_getflag( &OutRegion ) != TERMINATE  &&
           tport_getflag( &OutRegion ) != MyPid         )
   {
           int           IrigeStatus;            // Initially, there is no status
           int           rc;                     // Function return code
           short         *tracePtr;              // Pointer into traceBuf for demuxing
           short         *halfPtr;               // Pointer into halfBuf for demuxing
           unsigned long ptsTfr;                 // Number of points transferred to AdDat
           static time_t restartTime[] = {0,0,0};  // Times of last three restarts

        /* Beat the heart
           **************/
           Heartbeat();

           /* If the system hasn't broadcast any data for TimeoutNoSend seconds,
           reset the DAQ and attempt decode time from another IRIGE channel.
           *****************************************************************/
           if ( NoSend( sendIt ) )
           {
                   time_t     ltime;
                   static char errMsg[80];
                   const  int  errNum = 3;

                   time( &ltime );
                   restartTime[2] = restartTime[1];
                   restartTime[1] = restartTime[0];
                   restartTime[0] = ltime;
                   for ( i = 0; i < 3; i++ )
                           if ( restartTime[i] > 0 )
                           {
                                   SetCurPos( 44, 13+i );
                                   PrintGmtime( (double)restartTime[i], 0 );
                           }

                           if ( ++tcIndex >= NumTimeCodeChan )    // Decode the next IRIGE chan
                                   tcIndex = 0;

                           sprintf( errMsg, "Not sending.  Attempting IRIGE synch on pin %-3d\n",
                                   TimeCodeChan[tcIndex] );
                           SetCurPos( 4, 22 );                   // Position cursor for error msg
                           printf( "%s", errMsg );
                           ReportError( errNum, errMsg );
                           logit( "t", "%s", errMsg );
                           StopDAQ();
                           goto RESET;
           }

           /* Get a half buffer of data from the DAQ system.
           GetDAQ returns -1 if the trigger pulse was lost.
           If this occurs, reset the DAQ and use internal triggering.
           *********************************************************/
           if ( GetDAQ( halfBuf, &ptsTfr ) == -1 )
           {
                   time_t     ltime;
                   static char errMsg[] = "No trigger pulse. Switching to internal trigger.\n";
                   const  int  errNum = 5;

                   time( &ltime );
                   restartTime[2] = restartTime[1];
                   restartTime[1] = restartTime[0];
                   restartTime[0] = ltime;
                   for ( i = 0; i < 3; i++ )
                           if ( restartTime[i] > 0 )
                           {
                                   SetCurPos( 44, 13+i );
                                   PrintGmtime( (double)restartTime[i], 0 );
                           }

                           extTrig = 0;                          // Switch to internal triggering
                           SetCurPos( 4, 22 );                   // Position cursor for error msg
                           printf( "%s", errMsg );
                           ReportError( errNum, errMsg );
                           logit( "t", "%s", errMsg );
                           StopDAQ();
                           goto RESET;
           }

           /* Update the scan count
           *********************/
           scan += (unsigned)ChanMsgSize;

           SetCurPos( 10, 10 );
           printf( "%d", scan );

           /* If we are sending no matter what, use the system
           clock to set the message start time.  This mode
           is for debugging purposes.  Otherwise, IRIGE time
           code synch must be attained before data is sent.
           *************************************************/
           if ( SendBadTime )
           {
                   struct _timeb timebuffer;
                   _ftime( &timebuffer );
                   Tbuf.t = (double)timebuffer.time + 11676096000. +
                           ((double)timebuffer.millitm)/1000.;
           }

        /* Decode the IRIGE clock signal
           *******************************/
           else
           {
                   SetCurPos( 4, 22 );                   // Position cursor for error msg
                   if (YearFromPC)                       // Grab year number from PC clock
                   {
                           GetSystemTime (&st);
                           Year = st.wYear;
                   }
                   IrigeStatus = Irige( &Tbuf, (long)scan, Nchan, ChanMsgSize, halfBuf,
                           tcChan, Year );

                /* If the system hasn't had time-code synch TimeoutNoSynch seconds,
                   reset the DAQ and attempt decode time from another IRIGE channel.
                   *****************************************************************/
                   if ( NoTimeSynch( IrigeStatus ) )
                   {
                           time_t     ltime;
                           static char errMsg[80];
                           const  int  errNum = 4;

                           time( &ltime );
                           restartTime[2] = restartTime[1];
                           restartTime[1] = restartTime[0];
                           restartTime[0] = ltime;
                           for ( i = 0; i < 3; i++ )
                                   if ( restartTime[i] > 0 )
                                   {
                                           SetCurPos( 44, 13+i );
                                           PrintGmtime( (double)restartTime[i], 0 );
                                   }

                                   if ( ++tcIndex >= NumTimeCodeChan )    // Decode the next IRIGE chan
                                           tcIndex = 0;

                                   sprintf( errMsg, "No time synch.  Attempting IRIGE synch on pin %-3d\n",
                                           TimeCodeChan[tcIndex] );
                                   printf( "%s", errMsg );
                                   ReportError( errNum, errMsg );
                                   logit( "t", "%s", errMsg );
                                   StopDAQ();
                                   goto RESET;
                   }

                /* Apply time correction from local time to GMT time.
                   _timezone is the time difference in seconds between
                   GMT and local time.  For _tzset to work properly,
                   the environmental variable TZ must be set.  This is
                   done by the command file ew_nt.cmd.  On some systems
                   TZ is set in the Windows NT control panels under
                   "System", "Environment".
                   ****************************************************/
                   if ( IrigeIsLocalTime )
                   {
                           _tzset();
                           Tbuf.t += (double) _timezone;
                   }

                /* Print the time code status
                   **************************/
                   SetCurPos( 22, 12 );
                   if      ( IrigeStatus == TIME_NOSYNC )
                           puts( "Lost sync" );
                   else if ( IrigeStatus == TIME_FLAT )
                           puts( "Flat     " );
                   else if ( IrigeStatus == TIME_NOISE )
                           puts( "Noisy    " );
                   else if ( IrigeStatus == TIME_WAIT )
                           puts( "Searching" );
                   else if ( IrigeStatus == TIME_OK )
                           puts( "Locked-on" );

                /* Log any changes in the time code status
                   ***************************************/
                   if ( IrigeStatus != IrigeStatusPrev )
                   {
                           logit( "t", "Time-code status: " );
                           if ( IrigeStatus      == TIME_NOSYNC )
                                   logit( "", "Lost sync" );
                           else if ( IrigeStatus == TIME_FLAT )
                                   logit( "", "Flat" );
                           else if ( IrigeStatus == TIME_NOISE )
                                   logit( "", "Noisy" );
                           else if ( IrigeStatus == TIME_WAIT )
                                   logit( "", "Searching" );
                           else if ( IrigeStatus == TIME_OK )
                                   logit( "", "Locked-on" );
                           logit( "", "\n" );

                           IrigeStatusPrev = IrigeStatus;
                   }

                   /* If time locks on, start sending
                   *******************************/
                   if ( IrigeStatus == TIME_OK )              // TIME_OK means locked-on
                           sendIt = TRUE;
           }                                  // End of block for IRIGE time code

        /* Display the buffer time stamp
           *****************************/
           if ( (IrigeStatus != TIME_WAIT) || SendBadTime )
           {
                   SetCurPos( 4, 8 );
                   PrintGmtime( Tbuf.t - 11676096000., 2 );
                   if (!SendBadTime && UpdateSysClock && TimeCnt == TIMEUPDATE)
                   {  // Update the PC clock every TIMEUPDATE cycles
                           GetSystemTime (&st);
                           ltime = (time_t) (Tbuf.t - 11676096000. + ((double) ChanMsgSize /
                                   ChanRate));
                           gmt = gmtime (&ltime);
                           st.wMonth = gmt->tm_mon+1;
                           st.wDay = gmt->tm_mday;
                           st.wHour = gmt->tm_hour;
                           st.wMinute = gmt->tm_min;
                           st.wSecond = gmt->tm_sec;
                           st.wMilliseconds = (WORD) (1000.*(Tbuf.t-(int)Tbuf.t) + 0.5);
                           SetSystemTime (&st);
                           TimeCnt = 0;
                   }
                   TimeCnt++;
           }
           else
                   for ( i = 0; i < 26; i++ )
                           putchar( ' ' );

                   SetCurPos( 47, 18 );
                   if ( sendIt )
                           printf( "Sending...    " );
                   else
                           printf( "Not sending..." );

                /* Check for channel rotation by following a 4Vpp, 2hz
                   triangle wave, which is recorded on one channel of each mux.
                   ***********************************************************/
                   if ( NumGuide > 0 )
                   {
                           for ( i = 0; i < NumGuide; i++ )
                           {
                                   double GuideSignal;
                                   double GuideNoise;

                                   SetCurPos( 4, 22 );                   // Position cursor for error msg

                                   GuideStat[i] = GuideChk( halfBuf, Nchan, ChanMsgSize, GuideChan[i],
                                           &GuideSignal, &GuideNoise,
                                           MinGuideSignal, MaxGuideNoise );

                                   SetCurPos( 54, 6+i );
                                   if ( GuideStat[i] == GUIDE_NOISY )
                                           printf( "noisy    " );
                                   if ( GuideStat[i] == GUIDE_FLAT )
                                           printf( "flat     " );
                                   if ( GuideStat[i] == GUIDE_OK )
                                           printf( "ok       " );

                                /* Print the guide signal and noise levels
                                   ***************************************/
                                   printf( "%5.1lf %5.1lf", GuideSignal, GuideNoise );
                           }

                        /* Get the combined guide status
                           *****************************/
                           CombGuideChk( GuideStat, NumGuide, &CombGuideStat );

                           SetCurPos( 54, 5 );
                           if ( CombGuideStat == BAD )
                                   printf( "bad        " );
                           if ( CombGuideStat == OK )
                                   printf( "ok         " );
                           if ( CombGuideStat == LOCKED_ON )
                                   printf( "locked-on  " );

                        /* If the guide channels aren't locked on ErrNoLockTime seconds
                           after program startup or restart, send a message to statmgr.
                           Either way, keep digitizing (the data may be ok but the
                           triangle generator may be dead).  However, the DAQ system
                           won't ever reset unless CombGuideStat == LOCKED_ON.
                           ************************************************************/
                           if ( guideStartUp == 1 )
                           {
                                   time_t tNow;         // The current time

                                   time( &tNow );
                                   if ( tNow - tReset > ErrNoLockTime )
                                   {
                                           if ( CombGuideStat != LOCKED_ON )
                                           {
                                                   static char errMsg[80];
                                                   const  int  errNum = 6;

                                                   strcpy( errMsg, "Warning: Guide channels not locked on.\n" );
                                                   ReportError( errNum, errMsg );
                                                   logit( "t", "%s", errMsg );
                                           }
                                           guideStartUp = 0;    // Don't test CombGuideStat until next reset
                                   }
                           }

                        /* Ring the computer bell if Irige code is not in synch
                           or the guide channels are not locked-on
                           ****************************************************/
                           if ( EnableBell )
                                   if ( IrigeStatus != TIME_OK || CombGuideStat != LOCKED_ON )
                                   {
                                           const int interval = 5;     /* Every 5 seconds */
                                           static int repeat = 0;

                                           if ( ++repeat == interval )
                                           {
                                                   putchar( (char)7 );
                                                   repeat = 0;
                                           }
                                   }

                                /* Display the times of the last three restarts.  Then,
                                   log the event and initiate restart sequence.  When
                                   the system restarts, don't broadcast data until IRIGE
                                   synch is complete.  This takes about 15-20 seconds.
                                   *****************************************************/
                                   if ( CombGuideStat == RESTART_DAQ )
                                   {
                                           time_t ltime;
                                           static char errMsg[] = "Bad guides. DAQ system restarting.\n";
                                           const  int  errNum = 2;

                                           time( &ltime );
                                           restartTime[2] = restartTime[1];
                                           restartTime[1] = restartTime[0];
                                           restartTime[0] = ltime;
                                           for ( i = 0; i < 3; i++ )
                                                   if ( restartTime[i] > 0 )
                                                   {
                                                           SetCurPos( 44, 13+i );
                                                           PrintGmtime( (double)restartTime[i], 0 );
                                                   }

                                                   ReportError( errNum, errMsg );
                                                   logit( "t", "%s", errMsg );
                                                   StopDAQ();
                                                   sendIt = FALSE;            // Don't send until IRIGE synch is complete
                                                   goto RESET;
                                   }
      }

       /* Position the screen cursor for error messages
          *********************************************/
      SetCurPos( 4, 22 );

          /* Loop through the trace messages to be sent out
             **********************************************/
      for ( i = 0; i < Nchan; i++ )
      {

          /* Do not send messages for unused channels
             ****************************************/
                  if ( strlen( ChanList[i].sta  ) == 0 &&
                       strlen( ChanList[i].net  ) == 0 &&
                       strlen( ChanList[i].comp ) == 0 &&
                       strlen( ChanList[i].loc  ) == 0 )
                          continue;

               /* Fill the trace buffer header
                  ****************************/
                  traceHead->nsamp      = ChanMsgSize;         // Number of samples in message
                  traceHead->samprate   = ChanRate;            // Sample rate; nominal
                  traceHead->version[0] = TRACE2_VERSION0;     // Header version number
                  traceHead->version[1] = TRACE2_VERSION1;     // Header version number
                  traceHead->quality[0] = '\0';                // One bit per condition
                  traceHead->quality[1] = '\0';                // One bit per condition

                  strcpy( traceHead->datatype, "i2" );         // Data format code
                  strcpy( traceHead->sta,  ChanList[i].sta );  // Site name
                  strcpy( traceHead->net,  ChanList[i].net );  // Network name
                  strcpy( traceHead->chan, ChanList[i].comp ); // Component/channel code
                  strcpy( traceHead->loc,  ChanList[i].loc );  // Location code
                  traceHead->pinno = ChanList[i].pin;          // Pin number

               /* Set the trace start and end times.
                  Times are in seconds since midnight 1/1/1970
                  ********************************************/
                  traceHead->starttime = Tbuf.t - 11676096000.;
                  traceHead->endtime   = traceHead->starttime + (ChanMsgSize - 1)/ChanRate;

               /* Set error bits in buffer header
                  *******************************/
                  if ( IrigeStatus < 0 )
                          traceHead->quality[0] |= TIME_TAG_QUESTIONABLE;

                /* Transfer ChanMsgSize samples from halfBuf to traceBuf
                  *****************************************************/
                  tracePtr = &traceDat[0];
                  halfPtr  = &halfBuf[i];
                  for ( j = 0; j < ChanMsgSize; j++ )
                  {
                          *tracePtr = *halfPtr;
                          tracePtr++;
                          halfPtr += Nchan;
                  }

                  /* When time code synch is first attained, start
                  sending messages to the transport ring.  After this
                  time, keep sending messages even if synch is lost.
                  ***************************************************/
                  if ( sendIt )
                  {
                          rc = tport_putmsg( &OutRegion, &logo, traceBufSize, traceBuf );

                          if ( rc == PUT_TOOBIG )
                                  printf( "Trace message for channel %d too big\n", i );

                          if ( rc == PUT_NOTRACK )
                                  printf( "Tracking error while sending channel %d\n", i );
                  }
      }
      SetCurPos( 0, 0 );
   }

/* Clean up and exit program
   *************************/
   StopDAQ();
   free( halfBuf );
   free( ChanList );
   if ( NumGuide > 0 )
   {
      free( GuideStat );
      free( GuideChan );
   }
   logit( "t", "Adsend terminating.\n" );
   return 0;
}

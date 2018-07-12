/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nidaq_ew.c 886 2002-03-06 01:30:08Z kohler $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2002/03/06 01:30:08  kohler
 *     Added include <earthworm.h> for function prototypes.
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

     /*********************************************************************
      *                             nidaq_ew.c                            *
      *                                                                   *
      *  Calls to National Instruments Data Aquisition (NIDAQ) functions  *
      *  are isolated in this file.                                       *
      *********************************************************************/

#include <stdlib.h>
#include <windows.h>
#include <nidaq.h>
#include <nidaqcns.h>
#include <earthworm.h>

/* Global configuration parameters
   *******************************/
extern double ChanRate;              // Rate in samples per second per channel
extern double SampRate;              // Sample rate per scan (in samp/s)
extern int    ChanMsgSize;           // Message size in samples per channel
extern int    OnboardChans;          // The number of onboard channels
extern int    NumMuxBoards;          // Number of mux cards
extern int    Gain;                  // Gain of amp in front of A/D converter.
extern double LowValue;              // The low hysteresis voltage for external triggering
extern double HighValue;             // The high hysteresis voltage for external triggering
extern int    TimeoutNoTrig;         // Take action if no trigger for this many secs

/* Constants
   *********/
const short DAQ_device = 1;          // The DAQ board must be the first device

/* Global to this file
   *******************/
static short   *Buffer;              // The DAQ buffer
static unsigned BufSize;             // The size of the DAQ buffer

/* Function prototypes
   *******************/
void DAQ_ErrorHandler( char *, short );       // In errors.c


      /**************************************************************
       *                          InitDAQ()                         *
       *                                                            *
       *  Initialize the National Instruments data acquisition      *
       *  system.                                                   *
       **************************************************************/

void InitDAQ( int extTrig )
{
   short     status,          // Return value from the DAQ functions
             sampTimebase,    // Onboard source signal used for sample rate
             scanTimebase;    // Onboard source signal used for scan rate

   unsigned short
             sampInterval,    // Number of timebase units between consecutive samples
             scanInterval;    // Number of timebase units between consecutive scans

   int       totalChans;      // The number of channels to sample

/* Allocate space for the acquired data
   ************************************/
   totalChans = (NumMuxBoards == 0) ? OnboardChans : (4 * OnboardChans * NumMuxBoards);

   BufSize = (unsigned)(2 * ChanMsgSize * totalChans);

   Buffer = (short *) calloc( BufSize, sizeof(short) );
   if ( Buffer == NULL )
   {
      logit( "", "Cannot allocate A/D buffer\n" );
      return;
   }

/* Initialize the hardware and software for the DAQ board.
   Device number code 206 is the PCI-MIO-16E-4 DAQ board.
   ******************************************************/
   {
      short deviceNumberCode;

      status = Init_DA_Brds( DAQ_device, &deviceNumberCode );
      DAQ_ErrorHandler( "Init_DA_Brds", status );
   }

/* This sets a timeout limit (#Sec * 18ticks/Sec) so that if there is
   something wrong, the program won't hang on the DAQ_DB_Transfer call.
   *******************************************************************/
   {
      const long lTimeout = 180;      // Equivalent to 10 seconds

      status = Timeout_Config( DAQ_device, lTimeout );
      DAQ_ErrorHandler( "Timeout_Config", status );
   }

/* AI_Configure() informs NIDAQ of the input mode (single-ended or
   differential) and input polarity selected for the DAQ_device.
   If inputMode = 2 (non-referenced, single-ended mode, NRSE),
   the input signal's ground reference is connected to AISENSE,
   which is tied to the negative input of the instrumentation
   amplifier.  We jumper the AISENSE pin to AIGND.
   inputRange and driveAIS are ignored for the AT-MIO-16F-5 and
   PCI-MIO-16E-4 DAQ cards.
   ***************************************************************/
   {
      const short chan      = -1;      // -1 means "set all channels identically"
      const short inputMode =  2;      // 2 means NRSE configuration
      const short inputRange = 0;      // Ignored. Set in SCAN_Setup().
      const short polarity  =  0;      // 0 means bipolar operation (+ or - voltage)
      const short driveAIS  =  0;      // Ignored

      status = AI_Configure( DAQ_device, chan, inputMode, inputRange,
                              polarity, driveAIS );
      DAQ_ErrorHandler( "AI_Configure", status );
   }

/* AI_Mux_Config() informs NI-DAQ of the presence
   of any AMUX-64T devices attached to the system.
   The system supports 0, 1, 2, or 4 MUX boards.
   **********************************************/
   status = AI_Mux_Config( DAQ_device, (short)NumMuxBoards );
   DAQ_ErrorHandler( "AI_Mux_Config", status );

/* Set up "High-Hysteresis Analog Triggering Mode" as follows:
   1. Compute lowValue and highValue (0-255)
   2. Set up analog triggering using Configure_HW_Analog_Trigger().
   3. Link scan timing to analog trigger using SelectSignal().
   4. Specify external scan timing using DAQ_Config().
   ***************************************************************/
   if ( extTrig )
   {
      int         lowValue;           // Detrigger threshhold (0=-10V, 255=10V)
      int         highValue;          // Trigger threshhold (0=-10V, 255=10V)
      const short StartTrig = 0,      // 0 = internal start trigger
                  ExtScan   = 2;      // 2 = external scan timing

      lowValue  = (int)((LowValue/10.  + 1.0) * 128.);     // Compute lowValue
      if ( lowValue  <   0 ) lowValue  = 0;
      if ( lowValue  > 254 ) lowValue  = 254;
      logit( "t", "lowValue:  %d\n", lowValue );

      highValue = (int)((HighValue/10. + 1.0) * 128.);     // Compute highValue
      if ( highValue <   1 ) highValue = 1;
      if ( highValue > 255 ) highValue = 255;
      logit( "t", "highValue: %d\n", highValue );

      status = Configure_HW_Analog_Trigger( DAQ_device, ND_ON, lowValue,
                  highValue, ND_HIGH_HYSTERESIS, ND_PFI_0 );
      DAQ_ErrorHandler( "Configure_HW_Analog_Trigger", status );

      status = Select_Signal( DAQ_device, ND_IN_SCAN_START, ND_PFI_0,
                  ND_LOW_TO_HIGH );
      DAQ_ErrorHandler( "SelectSignal", status );

      status = DAQ_Config( DAQ_device, StartTrig, ExtScan );
      DAQ_ErrorHandler( "DAQ_Config", status );
   }

/* Turn ON software double-buffered mode.
   *************************************/
   {
      const short DBmodeON = 1;

      status = DAQ_DB_Config( DAQ_device, DBmodeON );
      DAQ_ErrorHandler( "DAQ_DB_Config", status );
   }

/* Convert sample rate (samples/sec) to appropriate
   timebase and sample interval values.
   ************************************************/
   {
      const short  iUnits = 0;          // 0 = points per second

      status = DAQ_Rate( SampRate, iUnits, &sampTimebase, &sampInterval );
      DAQ_ErrorHandler( "DAQ_Rate", status );
   }

/* Convert scan rate (samples/sec) to appropriate
   timebase and sample interval values.
   **********************************************/
   if ( extTrig )
   {
      scanTimebase = (short)0;
      scanInterval = (unsigned short)0;
   }
   else
   {
      const short iUnits = 0;           // 0 = points per second

      status = DAQ_Rate( ChanRate, iUnits, &scanTimebase, &scanInterval );
      DAQ_ErrorHandler( "DAQ_Rate", status );
   }

/* SCAN_Setup() initializes circuitry for a scanned data acquisition
   operation.  Initialization includes storing a table of the channel
   sequence and gain setting for each channel to be digitized.
   ******************************************************************/
   {
      int i;
      short *chans,                 // The analog input channels to be sampled
            *gains;                 // The gains for input channels

      chans = malloc( OnboardChans * sizeof(short) );
      if ( chans == NULL )
      {
         logit( "et", "Error allocating the chans array. Exiting.\n" );
         exit( -1 );
      }
      gains = malloc( OnboardChans * sizeof(short) );
      if ( gains == NULL )
      {
         logit( "et", "Error allocating the gains array. Exiting.\n" );
         free( chans );
         exit( -1 );
      }

      for ( i = 0; i < OnboardChans; i++ )
      {
         chans[i] = (short)i;       // See NIDAQ user's manual for details
         gains[i] = (short)Gain;
      }

      status = SCAN_Setup( DAQ_device, (short)OnboardChans, chans, gains );
      DAQ_ErrorHandler( "SCAN_Setup", status );

      free( chans );
      free( gains );
   }

/* SCAN_Start() initiates a multiple channel scanned DAQ data
   acquisition operation, and stores its input in an array.
   **********************************************************/
   {
      status = SCAN_Start( DAQ_device,
                           Buffer, BufSize,
                           sampTimebase, sampInterval,
                           scanTimebase, scanInterval );
      DAQ_ErrorHandler( "SCAN_Start", status );
   }
   return;
}


      /**************************************************************
       *                          GetDAQ()                          *
       *                                                            *
       *  Get a buffer full of data from the DAQ system.            *
       *                                                            *
       *  Returns -1 if timed out waiting for data.                 *
       *           0 if all is ok.                                  *
       **************************************************************/

int GetDAQ( short         *halfBuf,   // Buffer to contain the data obtained
            unsigned long *ptsTfr )   // Number of points transferred to AdDat
{
   short   status;                    // Return value from the DAQ functions
   short   daqStopped;                // 1 if DAQ operations are stopped
   int     SleepLen;                  // Check data ready every SleepLen msec
   int     nLoop;                     // Time out if no data after this many loops
   int     i;                         // Loop counter

/* DAQ_DB_HalfReady() checks whether the next half buffer is
   ready to transfer.  SleepLen is 0.1 * the message length in msec.
   ****************************************************************/
   SleepLen = (int)(100. * ChanMsgSize / ChanRate + 0.5);

   nLoop = 1000 * TimeoutNoTrig / SleepLen;

   for ( i = 0; i < nLoop; i++ )
   {
      short halfReady;               // 1 if data is available, 0 otherwise

      status = DAQ_DB_HalfReady( DAQ_device, &halfReady, &daqStopped );
      DAQ_ErrorHandler( "DAQ_DB_HalfReady", status );

/* DAQ_DB_Transfer() transfers half of the data from the buffer being
   used for double-buffered data acquisition to another buffer, which
   is passed to the function.  This function waits until the data to
   be transferred is available before returning (blocking).  You can
   execute DAQ_DB_Transfer repeatedly to return sequential half
   buffers of the data.
   ******************************************************************/
      if ( halfReady )
      {
         status = DAQ_DB_Transfer( DAQ_device, halfBuf, ptsTfr, &daqStopped );
         DAQ_ErrorHandler( "DAQ_DB_Transfer", status );
         return 0;
      }
      sleep_ew( SleepLen );          // Sleep for 0.1 of the message length
   }

/* Timed out waiting for the buffer to fill
   ****************************************/
   return -1;
}


void StopDAQ( void )
{
   const short DBmodeOFF = 0;

/* DAQ_Clear() cancels the current data acquisition operation
   **********************************************************/
   DAQ_Clear( DAQ_device );

/* Set DB mode back to initial state
   *********************************/
   DAQ_DB_Config( DAQ_device, DBmodeOFF );

/* Release the double buffer
   *************************/
   free( Buffer );
   return;
}

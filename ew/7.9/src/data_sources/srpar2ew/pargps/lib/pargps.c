/* FILE: pargps.c             Copyright (c), Symmetric Research, 2003-2011

These are the PARGPS user library functions.  The main library functions
are listed in pargps.h and can be called by all users to control the
PARGPS board.  See pargps.h for a description of the typical calling
sequence.  Study the PARxCH simp.c acquisition program and the PARGPS
Visual Basic demo program for examples of differnt ways in which these
functions can be used.

The library functions are mostly wrappers around the lower level read,
write, and ioctl device driver functions that do the actual work of
controlling the PARGPS.  Users should limit themselves to calling
functions listed in the parxch.h header file unless they are EXTREMELY
EXPERT at calling device driver code directly.

To use this library, just compile it to an object file and statically
link it with your application.  It can also be compiled to a DLL or
shared object library.  See the makefiles for examples of how to
compile.

This single source code files services all the supported operating
systems.  When compiling, define one of the SROS_xxxxx constants
shown in pargps.h on the compiler command line to indicate which OS
you are using.

For NT, Win2K/XP and Linux, the driver code is provided as a kernel
mode device driver and must be installed using the indriver utility
in the driver directory.  For Win2K/XP and NT, the driver is named
SrParGps.sys and for Linux it is named SrParGps.ko.  See the driver
directory for more details.

Change history:

  2011/08/09  WCT  Minor changes for POSIX compliance
  2008/10/15  WCT  Updated ParGpsRequestNmeaInfo function

*/


// OS dependent includes ...

#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )
#include <windows.h>     // windef.h -> winnt.h -> defines GENERIC_READ etc
#include <winioctl.h>    // device driver I/O control macros
#include <process.h>     // for thread info
#include <errno.h>       // for errno values


#elif defined( SROS_LINUX )
#include <errno.h>       // errno global
#include <fcntl.h>       // open  function, fcntl, F_ and O_ defines
#include <signal.h>      // for signal processing
#include <string.h>      // for strncopy, strncmp, strcmp
#include <termios.h>     // for terminal io structure and defines
#include <unistd.h>      // close, access, fsync fns and STDIN_FILENO define
#include <sys/ioctl.h>   // ioctl function and macros and TIOCM defines
#include <stdlib.h>      // exit function

typedef int BOOLEAN;
#define FALSE 0
#define TRUE  1

#endif  // SROS_xxxxx


// Common includes

#include <stdio.h>
#include <time.h>

#include "pargps.h"
#include "pargpskd.h"




// This define controls whether the PARGPS functions are static or visible
// to other modules.  The latter case is typically only required for
// debugging or with programs like PARGPS diag that require special access.

#if !defined( SRLOCAL )
#define SRLOCAL static
#endif



// Global variables

unsigned long ParGpsLastDriverError = 0;
char ParGpsLastErrorMessage[1024];

#define MAXCIRC 100
TIMESTAMP CircBuff[MAXCIRC];
int CbNumValid;

int CurrentGpsModel = GPSMODEL_TRIMBLE;


// These signal length defines indicate the number of seconds between
// the initial rising edge and the final falling edge of the signal in
// question.  They are used when calculating the GPS time of the first
// data sample after the PPS signal arrives.  They account for the fact
// that, due to the way the PARxCH circuitry and firmware is setup, the
// PC is interrupted on the falling (not rising) edge of the PPS and
// Dready signals which are of different length.  SignalLengthPps is now
// a parameter that is set in ParGpsInit according to the selected
// GPSMODEL_ define.

double SignalLengthPps;           // PARGPS default is 0.000010L or 10 usec
#define SIGNAL_LENGTH_DREADY      0.000052L  // 52 microseconds (usec)
#define SIGNAL_LENGTH_CORRECTION (SIGNAL_LENGTH_DREADY - SignalLengthPps)




// Use for comparing doubles

#define TOLERANCE 0.00001


#define MAXNAME 30


// Note: The NMEA message IDs are listed in the order in which they
// come out on the Trimble ACE III receiver used by the PARGPS.
// Be sure these match up with the NMEA_MSGID defines in pargps.h.

VARTYPE( char ) *NmeaMsgId[] = { "ZDA","GGA","GLL","VTG","GSA","GSV","RMC" };



// Actual allocation of PARGPS error string array, see pargps.h ...

VARTYPE( char ) *PARGPS_ERROR_MSG[] = {

        PARGPS_ERROR_MSG_NONE,
        PARGPS_ERROR_MSG_NOT_AVAILABLE,
        PARGPS_ERROR_MSG_INTERRUPT_NUMBER,
        PARGPS_ERROR_MSG_INTERRUPT_MODE,
        PARGPS_ERROR_MSG_SERIAL_PORT_NUMBER,
        PARGPS_ERROR_MSG_SERIAL_PORT_NOT_OPEN,
        PARGPS_ERROR_MSG_DRIVER_NOT_OPEN,
        PARGPS_ERROR_MSG_DRIVER_REQUEST_FAILED,
        PARGPS_ERROR_MSG_KERNEL_AREA,
        PARGPS_ERROR_MSG_KEYPRESS,
        PARGPS_ERROR_MSG_BAD_PARAMETER,
        PARGPS_ERROR_MSG_BAD_GPSMODEL,
        PARGPS_ERROR_MSG_BAD_NMEA,
        PARGPS_ERROR_MSG_MAX

        };


#if defined( SROS_LINUX )
DEVHANDLE Signal_hSerial;     // for signal handler
DEVHANDLE save_serial_handle; // for closing serial
#endif


struct NMEA_process; // forward declaration for NMEAPROCESS

//WCT - The first 6 are declared in pargps.h with INCLUDE_HELPER_DEFINES

// Prototypes for private PARGPS helper and OS dependent functions
// Most users should only use the standard PARGPS functions which are
// declared in pargps.h.

FUNCTYPE( int ) ParGpsIoctl( DEVHANDLE ParGpsHandle, unsigned long IoCtlCode,
                          void *pValueIn, unsigned long InSize,
                          void *pValueOut, unsigned long OutSize,
                          unsigned long *ReturnSize );
FUNCTYPE( int ) ParGpsInit( DEVHANDLE ParGpsHandle, int SerialPort,
                            int InterruptMode, int GpsModel );
FUNCTYPE( int ) ParGpsSerialSetup( DEVHANDLE ParGpsHandle, int PortNumber );
FUNCTYPE( int ) ParGpsSetInterruptMode( DEVHANDLE ParGpsHandle, int Mode );
FUNCTYPE( int ) ParGpsReset( DEVHANDLE ParGpsHandle );
FUNCTYPE( int ) ParGpsGetInterruptCount( DEVHANDLE ParGpsHandle, int *Count );
FUNCTYPE( int ) ParGpsMatchNmeaTable( char *MsgId, struct NMEA_process **t );
FUNCTYPE( int ) ParGpsExtractParsedDate( NMEAPARSE *ParseData,
                                         int DateLength, int DateField,
                                         int *pYr, int *pMo, int *pDy );
FUNCTYPE( int ) ParGpsExtractParsedTime( NMEAPARSE *ParseData,
                                         int TimeField,
                                         int *pHh, int *pMm, int *pSs,
                                         long *pUsec );
FUNCTYPE( int ) ParGpsExtractParsedLoc( NMEAPARSE *ParseData,
                                        int LatLonField,
                                        double *pLat, double *pLon );
FUNCTYPE( int ) ParGpsExtractParsedAlt( NMEAPARSE *ParseData,
                                        int AltitudeField,
                                        int AltitudeRank,
                                        double *pAlt );
FUNCTYPE( int ) ParGpsExtractParsedPos( NMEAPARSE *ParseData,
                                        int LatLonField, int AltitudeField,
                                        int AltitudeRank,
                                        double *pLat, double *pLon,
                                        double *pAlt );
FUNCTYPE( int ) ParGpsExtractParsedSat( NMEAPARSE *ParseData,
                                        int NsatField, int *pSat );
FUNCTYPE( int ) ParGpsRoundTimeToPps( int *Year, int *Month, int *Day,
                                      int *Hour, int *Minute, int *Second,
                                      long *MicroSecond );
FUNCTYPE( int ) ParGpsAdjustNmeaInfo( int Yr, int Mo, int Dy, int *YmdSrc,
                                      int Hh, int Mm, int Ss, long Us, int *HmsSrc,
                                      unsigned int EventNum,
                                      int SerialDelay,
                                      unsigned int *PrevEventNum,
                                      double *PrevSecSince1970);
SRLOCAL int ParGpsInitCircBuff( void );
SRLOCAL int ParGpsSkipCircBuff( void );
SRLOCAL int ParGpsPeekCircBuffState( void );
SRLOCAL int ParGpsReadCircBuff( TIMESTAMP *Buffer, int TsValid );
SRLOCAL int ParGpsAnalogWriteCircBuff( unsigned int PpsEvent, long Sample,
                                       int Obc, int TsValid );
SRLOCAL int ParGpsPpsWriteCircBuff( unsigned int PpsEvent,
                                    SR64BIT CountAtPps, SR64BIT CountAtDready,
                                    SR64BIT PctimeAtPps, SR64BIT PctimeAtDready,
                                    double CountsPerSecond,
                                    int GpsModel, int TsValid );
SRLOCAL int ParGpsSerialWriteCircBuff( unsigned int PpsEvent, double SecSince1970,
                                       int YMDSource, int HMSSource, int Nsat,
                                       int TsValid, int Erase );
SRLOCAL int ParGpsIncCbPtr( int *PtrIndex );
SRLOCAL int ParGpsUpdateCbPtrForSkip( int *PtrIndex );
SRLOCAL int ParGpsCheckEventNum( unsigned int PpsEvent, int TsValid, int *wrptrAPS, char *StrAPS );
SRLOCAL int ParGpsSrcToSource( int YmdSrc, int HmsSrc, int YmdValid, int HmsValid,
                               int *YmdSource, int *HmsSource );
SRLOCAL int ParGpsEquation2pt( double x1, double y1,
                               double x2, double y2,
                               double xk, double *yk );
SRLOCAL int ParGpsEquationSlope( double m,
                                 double x1, double y1,
                                 double xk, double *yk );


SRLOCAL DEVHANDLE ParGpsOsDriverOpen( char *DriverName );
SRLOCAL int ParGpsOsDriverClose( DEVHANDLE *ParGpsHandle );
SRLOCAL int ParGpsOsDriverIoctl( DEVHANDLE ParGpsHandle, unsigned long IoCtlCode,
                           void *pValueIn, unsigned long InSize,
                           void *pValueOut, unsigned long OutSize,
                           unsigned long *pBytesReturned );
SRLOCAL long ParGpsOsGetLastError( void );
SRLOCAL int ParGpsOsGetDefaultModel( char *DriverName );
SRLOCAL DEVHANDLE ParGpsOsSerialOpen( char *SerialPortName );
SRLOCAL int ParGpsOsSerialInit( DEVHANDLE hSerial );
SRLOCAL int ParGpsOsSerialRead( DEVHANDLE SerialHandle, void *pValues,
                                unsigned long  BytesToRead,
                                unsigned long *pBytesRead );
SRLOCAL void ParGpsOsSerialClose( DEVHANDLE hSerial );
SRLOCAL void ParGpsOsSerialSignalHandler( int SigNum );

SRLOCAL int ParGpsOsSetPcTime( double Time,
                               int Year, int Month, int Day,
                               int Hour, int Minute, int Second,
                               long Microsecond, int FromYMD );





//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetRev
// PURPOSE: Determine which version of the PARGPS library is being used.
//
// Short rev history:
//
//   PARGPS_REV  100  ( 07/25/2002 )  < first beta release
//   PARGPS_REV  101  ( 08/25/2002 )  < next  beta release
//   PARGPS_REV  102  ( 09/05/2002 )  < added ChangeDate to Inc/Dec time funcs
//   PARGPS_REV  103  ( 04/15/2003 )  < reorg + added CurrentTime function
//   PARGPS_REV  104  ( 06/15/2003 )  < added NMEA ZDA string YMD support
//   PARGPS_REV  105  ( 08/05/2003 )  < changed NMEA reading + wait timeout in driver
//   PARGPS_REV  106  ( 09/10/2003 )  < Added Gps prefix to Os functions
//   PARGPS_REV  107  ( 09/30/2003 )  < Have GetRev return value
//   PARGPS_REV  108  ( 11/15/2003 )  < Added function to get PC time
//   PARGPS_REV  109  ( 02/01/2004 )  < Added error checking for bad serial open
//   PARGPS_REV  110  ( 03/20/2004 )  < Added PARPGS LabView support
//   PARGPS_REV  111  ( 05/30/2004 )  < Added SetPcTime + chg SecTime split/combine
//   PARGPS_REV  112  ( 10/31/2004 )  < Added timestamp functions
//   PARGPS_REV  113  ( 11/15/2004 )  < Included timestamp init function
//   PARGPS_REV  114  ( 07/01/2005 )  < Added UserIrp for Linux 2.6 kernels
//   PARGPS_REV  115  ( 08/10/2005 )  < More support for other GPS receivers
//   PARGPS_REV  116  ( 03/01/2006 )  < Allow PCtime timestamping, SRDAT_REV 106
//   PARGPS_REV  117  ( 09/15/2006 )  < Added FlushSerial to support error recovery
//   PARGPS_REV  118  ( 12/05/2007 )  < Added Garmin GPS 18 LVC support
//   PARGPS_REV  119  ( 10/15/2008 )  < Improved RequestNmeaInfo processing
//
//------------------------------------------------------------------------------

#define PARGPS_REV      119

FUNCTYPE( int ) ParGpsGetRev( int *Rev ) {

        *Rev = PARGPS_REV;
        return( *Rev );
}







/************************ PARGPS MAIN LIBRARY FUNCTIONS ************************/

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOpen
// PURPOSE: To open the PARGPS driver for use with a PARGPS timing board.
//------------------------------------------------------------------------------

#define ERROR_RETURN( err ) {   if ( Error )              \
                                        *Error = err;     \
                                return( BAD_DEVHANDLE );  \
                                }

FUNCTYPE( DEVHANDLE ) ParGpsOpen( char *ParGpsName, int SerialPortNumber, int *Error ) {

        int GpsModel;

        // Use the default GPS model

        GpsModel = ParGpsGetDefaultModel( ParGpsName );

        return( ParGpsFullOpen( ParGpsName,
                                GpsModel,
                                SerialPortNumber,
                                NULL,
                                Error )
              );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsFullOpen
// PURPOSE: To open the PARGPS driver.  This full version of the open function
//          allows not only the typical use with a PARGPS timing board, but also
//          for timestamping based on the PC system time.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) ParGpsFullOpen( char *ParGpsName,
                                      int GpsModel,
                                      int SerialPortNumber,
                                      int *Spare,
                                      int *Error ) {

        DEVHANDLE ParGpsHandle;
        int       ret;

        ParGpsLastErrorMessage[0] = '\0';


        // Error check the inputs

        if ( (SerialPortNumber < 1) || (SerialPortNumber > 8) )
                ERROR_RETURN( PARGPS_ERROR_SERIAL_PORT_NUMBER );

        if ( (GpsModel <= GPSMODEL_NONE) || (GpsModel >= GPSMODEL_MAX) )
                ERROR_RETURN( PARGPS_ERROR_BAD_GPSMODEL );



        // Open device driver

        ParGpsHandle = ParGpsOsDriverOpen( ParGpsName );

        if ( ParGpsHandle == BAD_DEVHANDLE ) {
                ParGpsLastDriverError = ParGpsOsGetLastError();
                ERROR_RETURN( PARGPS_ERROR_DRIVER_NOT_OPEN );
                }

        ParGpsLastDriverError = 0;



        // Setup serial port baud rate, etc unless using PCTIME
        // which does not require serial port

        if ( GpsModel != GPSMODEL_PCTIME ) {
                ret = ParGpsSerialSetup( ParGpsHandle, SerialPortNumber );
                if (!ret) {
                        ParGpsOsDriverClose( &ParGpsHandle );
                        ERROR_RETURN( PARGPS_ERROR_SERIAL_PORT_NOT_OPEN );
                        }
                }
#if defined( SROS_LINUX )
        else
                save_serial_handle = BAD_DEVHANDLE;
#endif


        // Initialize driver by setting
        //    serial port, typically 1 or 2, and
        //    interrupt mode to INTERRUPT_ALTERNATING

        ret = ParGpsInit( ParGpsHandle, SerialPortNumber,
                          INTERRUPT_ALTERNATING, GpsModel );
        if (!ret) {
                sprintf( ParGpsLastErrorMessage,
                         "Could not init serial port,interrupt mode\n");
                ParGpsOsDriverClose( &ParGpsHandle );
                ERROR_RETURN( PARGPS_ERROR_SERIAL_PORT_NOT_OPEN );
                }


        // Initialize circular time stamp buffer

        ParGpsInitCircBuff();


        return( ParGpsHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOpenForTimeOnly
// PURPOSE: To open the PARGPS driver a second time in order to access the
//          current GPS time information.  When this version of open is used,
//          only the ParGpsGetCurrentTime function should be called.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) ParGpsOpenForTimeOnly( char *ParGpsName, int *Error ) {

        DEVHANDLE ParGpsHandle;


        // Open device driver

        ParGpsHandle = ParGpsOsDriverOpen( ParGpsName );

        if ( ParGpsHandle == BAD_DEVHANDLE ) {
                ParGpsLastDriverError = ParGpsOsGetLastError();
                ERROR_RETURN( PARGPS_ERROR_DRIVER_NOT_OPEN );
                }

        ParGpsLastDriverError = 0;


        return( ParGpsHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsClose
// PURPOSE: To close the PARGPS driver.  Since only one program can have the
//          PARGPS driver open for full use, it is important to close the
//          driver when you are done with it.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsClose( DEVHANDLE ParGpsHandle ) {

        // Only close open drivers

        if (ParGpsHandle == BAD_DEVHANDLE) {
                ParGpsLastDriverError = ERROR_SERVICE_DISABLED;
                return( 0 );
                }


#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )
        // Already closed earlier
#elif defined( SROS_LINUX )
        if ( save_serial_handle != BAD_DEVHANDLE )
                ParGpsOsSerialClose( save_serial_handle );
#endif


        // Close driver

        if ( !ParGpsOsDriverClose( &ParGpsHandle ) )
                return( 0 );

        ParGpsLastDriverError = 0;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsStart
// PURPOSE: Start the PARGPS driver so it responds to PPS interrupts coming over
//          the PC parallel port via the PARxCH A/D board and reads NMEA strings
//          from the serial port.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsStart( DEVHANDLE ParGpsHandle ) {

        int ret, dummy;
        ret = ParGpsIoctl( ParGpsHandle,
                        IOCTL_PARGPS_START,
                        &dummy,
                        sizeof(int),
                        NULL,
                        0,
                        NULL
                      );

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsStop
// PURPOSE: Stop the PARGPS driver from responding to PPS interrupts and from
//          reading NMEA strings.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsStop( DEVHANDLE ParGpsHandle ) {

        int ret, dummy;
        ret = ParGpsIoctl( ParGpsHandle,
                        IOCTL_PARGPS_STOP,
                        &dummy,
                        sizeof(int),
                        NULL,
                        0,
                        NULL
                      );

        return( ret );
}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetKernelArea
// PURPOSE: Return a pointer to a common kernel area that the PARxCH driver can
//          use to communicate with the PARGPS driver.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetKernelArea( DEVHANDLE ParGpsHandle, void **KernelArea ) {

        int ret;

        ret = ParGpsIoctl( ParGpsHandle,
                        IOCTL_PARGPS_GET_KAREA,
                        NULL,
                        0,
                        KernelArea,
                        sizeof(void*),
                        NULL
                      );

        return( ret );
}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetCounterFrequency
// PURPOSE: This function is obsolete and will soon be removed, use
//          ParGpsGetFullCounterFrequency instead.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetCounterFrequency( DEVHANDLE ParGpsHandle,
                                           long *CounterFreq ) {

        return( ParGpsGetFullCounterFrequency( ParGpsHandle,
                                             NULL, CounterFreq,
                                             NULL, NULL ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetFullCounterFrequency
// PURPOSE: Get the frequency of two counters used in the PARGPS driver so the
//          respective count values can be converted into seconds.  These
//          counters are used to determine the elapsed time between the arrival
//          of a PPS signal marking the beginning of a second and the time at
//          which the next acquired data sample is ready.
//          The first hi/lo pair is for the counter working with high speed
//          Pentium counts, while the second pair is for the counter working
//          with PC system time (typically in us or 100ns per count).
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetFullCounterFrequency( DEVHANDLE ParGpsHandle,
                                               long *CounterFreq1Hi,
                                               long *CounterFreq1Lo,
                                               long *CounterFreq2Hi,
                                               long *CounterFreq2Lo ) {
        int     ret;
        SR64BIT CounterFreqs[2];


        // Set defaults

        if (CounterFreq1Hi)  *CounterFreq1Hi = 0L;
        if (CounterFreq1Lo)  *CounterFreq1Lo = 0L;
        if (CounterFreq2Hi)  *CounterFreq2Hi = 0L;
        if (CounterFreq2Lo)  *CounterFreq2Lo = 0L;


        // Ask driver for frequency info

        ret = ParGpsIoctl( ParGpsHandle,
                           IOCTL_PARGPS_GET_COUNTERFREQ,
                           NULL,
                           0,
                           CounterFreqs,
                           sizeof(CounterFreqs),
                           NULL
                         );


        // Fill in requested results

        if (ret) {
                if (CounterFreq1Hi)  *CounterFreq1Hi = CounterFreqs[0].u.HighPart;
                if (CounterFreq1Lo)  *CounterFreq1Lo = CounterFreqs[0].u.LowPart;
                if (CounterFreq2Hi)  *CounterFreq2Hi = CounterFreqs[1].u.HighPart;
                if (CounterFreq2Lo)  *CounterFreq2Lo = CounterFreqs[1].u.LowPart;
                }

        return( ret );
}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsReadPpsData
// PURPOSE: Read PPS/Dready counter information from the PARGPS driver.  The
//          driver collects this info as it comes in and saves it up in a
//          software buffer until the user asks for it.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) ParGpsReadPpsData( DEVHANDLE     ParGpsHandle,
                                            void         *Values,
                                            unsigned int  Nvalues,
                                            int          *Error
                                            ) {

        int ret, Nbytes, NvaluesReturned;
        unsigned long NbytesReturned;

        // Quick exit if not asking for any data

        if (Nvalues == 0) {
                if (Error)
                        *Error = PARGPS_ERROR_NONE;
                return( 0 );
                }


        // Don't ask for more data than MAX_PARGPS_STORAGE since that is
        // the total amount that can be stored in the PARGPS device driver
        // software FIFO without losing data.

        if (Nvalues < MAX_PARGPS_STORAGE)
                Nbytes = Nvalues * sizeof(PPSTDATA);
        else
                Nbytes = MAX_PARGPS_STORAGE * sizeof(PPSTDATA);


        ret = ParGpsIoctl(
                       ParGpsHandle,
                       IOCTL_PARGPS_READ_PPS_DATA,
                       NULL,
                       0,
                       Values,
                       Nbytes,
                       &NbytesReturned
                      );

        NvaluesReturned = NbytesReturned / sizeof(PPSTDATA);

        if (Error) {
                if (!ret)
                        *Error = PARGPS_ERROR_DRIVER_REQUEST_FAILED;
                else
                        *Error = PARGPS_ERROR_NONE;
                }

        return( NvaluesReturned );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsReadSerialData
// PURPOSE: Read serial NMEA string information from the PARGPS driver.  The
//          driver collects this info as it comes in and saves it up in a
//          software buffer until the user asks for it.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) ParGpsReadSerialData( DEVHANDLE     ParGpsHandle,
                                               void         *Values,
                                               unsigned int  Nvalues,
                                               int          *Error
                                           ) {

        int ret, Nbytes, NvaluesReturned;
        unsigned long NbytesReturned;


        // Quick exit if not asking for any data

        if (Nvalues == 0) {
                if (Error)
                        *Error = PARGPS_ERROR_NONE;
                return( 0 );
                }


        // Don't ask for more data than MAX_PARGPS_STORAGE since that is
        // the total amount that can be stored in the PARGPS device driver
        // software FIFO without losing data.

        if (Nvalues < MAX_PARGPS_STORAGE)
                Nbytes = Nvalues * sizeof(SERIALDATA);
        else
                Nbytes = MAX_PARGPS_STORAGE * sizeof(SERIALDATA);


        ret = ParGpsIoctl(
                       ParGpsHandle,
                       IOCTL_PARGPS_READ_SERIAL_DATA,
                       NULL,
                       0,
                       Values,
                       Nbytes,
                       &NbytesReturned
                      );

        NvaluesReturned = NbytesReturned / sizeof(SERIALDATA);

        if (Error) {
                if (!ret)
                        *Error = PARGPS_ERROR_DRIVER_REQUEST_FAILED;
                else
                        *Error = PARGPS_ERROR_NONE;
                }

        return( NvaluesReturned );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetCurrentTime
// PURPOSE: Ask the PARGPS driver for the current time.  It determines this from
//          the PPS counter and serial NMEA info stored from the most recent PPS
//          time and the current counter values.  This is the only function
//          that should be called if the driver has been opened with the
//          ParGpsOpenForTimeOnly function.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetCurrentTime( DEVHANDLE ParGpsHandle,
                                   double *TimeAsSecsSince1970,
                                   int *Year,
                                   int *Month,
                                   int *Day,
                                   int *Hour,
                                   int *Minute,
                                   int *Second,
                                   long *MicroSecond,
                                   int *NumSatellites,
                                   double *Latitude,
                                   double *Longitude,
                                   double *Altitude,
                                   int *SourceYMD,
                                   int *SourceHMS,
                                   int *SourceNsat,
                                   int *SourcePos,
                                   int *Error ) {

static unsigned int PrevEventNum     = INVALID_PPSEVENT;
static double       PrevSecSince1970 = 0.0;

        int           Yr, Mo, Dy, Hh, Mm, Ss, Nsat, Ok, UsePcTime;
        int           YmdSrc, HmsSrc, LocSrc, AltSrc, SatSrc, ValidNmea;
        int           YmdIsValid, HmsIsValid, LocIsValid, AltIsValid, SatIsValid;
        long          CounterFreqHi, CounterFreqLo, Us;
        double        DiffCount, Freq, Lat, Lon, Alt, DiffTm, Tm;
        unsigned int  EventNum, SerialDelay;
        GPSTIME2DATA  CurrentTimeData;
        SR64BIT       CounterFreq;
        time_t        ttNow;



        // Initialize variables.

        if (Error)               *Error               = PARGPS_ERROR_NONE;
        if (TimeAsSecsSince1970) *TimeAsSecsSince1970 = 0.0;
        if (Year)                *Year                = 0;
        if (Month)               *Month               = 0;
        if (Day)                 *Day                 = 0;
        if (Hour)                *Hour                = 0;
        if (Minute)              *Minute              = 0;
        if (Second)              *Second              = 0;
        if (MicroSecond)         *MicroSecond         = 0L;
        if (NumSatellites)       *NumSatellites       = 0;
        if (Latitude)            *Latitude            = 0;
        if (Longitude)           *Longitude           = 0;
        if (Altitude)            *Altitude            = 0;
        if (SourceYMD)           *SourceYMD           = SRCSTR_MIN;
        if (SourceHMS)           *SourceHMS           = SRCSTR_MIN;
        if (SourceNsat)          *SourceNsat          = SRCSTR_MIN;
        if (SourcePos)           *SourcePos           = SRCSTR_MIN;



        // Call kernel function.

        Ok = ParGpsIoctl( ParGpsHandle,
                          IOCTL_PARGPS_GET_CURRENT_TIME,
                          NULL,
                          0,
                          &CurrentTimeData,
                          sizeof(GPSTIME2DATA),
                          NULL
                        );

        if (!Ok) {
                if (Error)  *Error = PARGPS_ERROR_DRIVER_REQUEST_FAILED;
                return( Ok );
                }




        // Verify returned kernel time info.

        if ( !(CurrentTimeData.ValidFields & VALID_PPS)    ||
             !(CurrentTimeData.ValidFields & VALID_DREADY) ||
              (CurrentTimeData.EventNumP == 0)             ||
              (CurrentTimeData.EventNumS == 0)             ) {
                if (Error)  *Error = PARGPS_ERROR_NONE;
                return( 0 );
                }


        // The PPS and NMEA info will be from Events separated by
        // 1 second or 2 seconds depending on when the CurrentTimeData
        // was requested.  If the request comes after a PPS but before
        // the next DREADY then the VALID_NMEA field is set and the
        // PPS to NMEA serial separation is 2.

        SerialDelay = CurrentTimeData.EventNumP - CurrentTimeData.EventNumS;

        ValidNmea = CurrentTimeData.ValidFields & VALID_NMEA;
        if ( (ValidNmea && (SerialDelay != 1)) ||
            (!ValidNmea && (SerialDelay != 2)) ) {
                        if (Error)  *Error = PARGPS_ERROR_BAD_PARAMETER;
                        return( 0 );
                        }


        // Get counter frequency for interpreting PPS and DREADY counts

        ParGpsGetFullCounterFrequency( ParGpsHandle,
                                       &CounterFreqHi, &CounterFreqLo,
                                       NULL, NULL );
        if (CounterFreqHi == 0L && CounterFreqLo == 0L) {
                if (Error)  *Error = PARGPS_ERROR_NOT_AVAILABLE;
                return( 0 );
                }

        CounterFreq.u.HighPart = CounterFreqHi;
        CounterFreq.u.LowPart  = CounterFreqLo;
        Freq = (double)(CounterFreq.QuadPart);


        // Look through all NMEA messages for date, time,
        // position and number of satellites

        Ok = ParGpsRequestNmeaInfo2( CurrentTimeData.NmeaMsg,
                                     CurrentTimeData.NmeaCount,
                                     &Yr, &Mo, &Dy, &YmdSrc, &YmdIsValid,
                                     &Hh, &Mm, &Ss, &Us, &HmsSrc, &HmsIsValid,
                                     &Lat, &Lon, &LocSrc, &LocIsValid,
                                     &Alt, &AltSrc, &AltIsValid,
                                     &Nsat, &SatSrc, &SatIsValid );
        if (!Ok) {
                if (Error)  *Error = PARGPS_ERROR_BAD_NMEA;
                return( 0 );
                }


        // Force use of old time info if NMEA strings not currently valid

        if ( (YmdIsValid != 1) || (HmsIsValid != 1) ) {
                YmdSrc = SRCSTR_UNK;
                HmsSrc = SRCSTR_UNK;
                }



        // Didn't find date or have prev value, so use pc time as prev

        UsePcTime = 0;
        if ( (YmdSrc == SRCSTR_UNK) && (PrevSecSince1970 == 0.0) ) {
                UsePcTime = 1;
                time( &ttNow );
                PrevSecSince1970 = (double)ttNow;
                }


        // Adjust info using previous values if needed

        EventNum = CurrentTimeData.EventNumP + SerialDelay;

        Ok = ParGpsAdjustNmeaInfo( Yr, Mo, Dy, &YmdSrc,
                                   Hh, Mm, Ss, Us, &HmsSrc,
                                   EventNum, SerialDelay,
                                   &PrevEventNum,
                                   &PrevSecSince1970 );

        // If old time used, indicate if it came from pc

        if ( (YmdSrc == SRCSTR_OLD) && (UsePcTime == 1) )
                YmdSrc = SRCSTR_PC;


        // No NMEA time ever, so just send back Nsat info

        if (!Ok) {
                if (Error)         *Error         = PARGPS_ERROR_NONE;
                if (NumSatellites) *NumSatellites = Nsat;
                return( 0 );
                }



        // Adjust the Now time for the difference since the PPS.
        // PrevSecSince1970 actually holds the current PPS time as it
        // was updated by the adjust function above in preparation for
        // the next pass.  Also include the PPS signal length correction.

        ParGpsLargeIntDiffFull( CurrentTimeData.CountAtNow,
                                CurrentTimeData.CountAtPps,
                                NULL, &DiffCount );

        DiffTm    = DiffCount/Freq;
        Tm        = PrevSecSince1970 + DiffTm + SIGNAL_LENGTH_CORRECTION;

        ParGpsSecTimeSplit( Tm, &Yr, &Mo, &Dy, &Hh, &Mm, &Ss, &Us );


        // Fill user parameters.

        if (TimeAsSecsSince1970) *TimeAsSecsSince1970 = Tm;
        if (Year)                *Year                = Yr;
        if (Month)               *Month               = Mo;
        if (Day)                 *Day                 = Dy;
        if (Hour)                *Hour                = Hh;
        if (Minute)              *Minute              = Mm;
        if (Second)              *Second              = Ss;
        if (MicroSecond)         *MicroSecond         = Us;
        if (NumSatellites)       *NumSatellites       = Nsat;
        if (Latitude)            *Latitude            = Lat;
        if (Longitude)           *Longitude           = Lon;
        if (Altitude)            *Altitude            = Alt;
        if (SourceYMD)           *SourceYMD           = YmdSrc;
        if (SourceHMS)           *SourceHMS           = HmsSrc;
        if (SourceNsat)          *SourceNsat          = SatSrc;
        if (SourcePos)           *SourcePos           = LocSrc;


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsFlushSerial
// PURPOSE: Send a request to the ParGps driver to flush the contents of the
//          serial buffer.  This function should only be called after the
//          serial port has been opened and an error has been detected.  Then
//          it can be used re-synchronized the PPS signal with the NMEA strings.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsFlushSerial( DEVHANDLE ParGpsHandle ) {

        int ret, dummy;
        ret = ParGpsIoctl( ParGpsHandle,
                        IOCTL_PARGPS_FLUSH_SERIAL,
                        &dummy,
                        sizeof(int),
                        NULL,
                        0,
                        NULL
                      );

        return( ret );
}




/********************* PARGPS DEFAULT GPS MODEL FUNCTION *********************/

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetDefaultModel
// PURPOSE: Get the default PARGPS model information.
//          The PARGPS timing board comes in two versions.  One uses the now
//          discontinued Trimble ACE III receiver and the other uses the Garmin
//          GPS 18 LVC antenna/receiver.  While the software is designed to work
//          with either of the PARGPS models, the application programs often
//          need to know which model is being used.  Typically, the specific
//          model is given as a command line argument.  But for convenience, a
//          default choice can be saved (in either the registry or an environment
//          variable depending on OS) by using the setmodel utility.  This
//          function retrieves this default model information.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetDefaultModel( char *DriverName ) {

        int GpsModel;

        GpsModel = ParGpsOsGetDefaultModel( DriverName );

        if (GpsModel == GPSMODEL_UNKNOWN)
                GpsModel = GPSMODEL_GARMIN;

        return( GpsModel );
}






/**************** ALTERNATE AND STRUCTURE HELPER FUNCTIONS ****************/

/*
 * Some of the previous functions pass structures or have large numbers
 * of arguments which makes them difficult to call from other languages.
 * So these alternate and helper functions have been provided.  If you
 * have a choice, it is better to use an original function than an
 * alternate since the alternate just calls the original and then
 * repackages the results.
 *
 */

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetCurrentTimeAlt
// PURPOSE: Provide an alternate interface to the ParGpsGetCurrentTime function
//          that is easier to call from LabView.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetCurrentTimeAlt( DEVHANDLE ParGpsHandle,
                                         int *IntReturns,
                                         long *LongReturns,
                                         double *DoubleReturns,
                                         int *Error ) {

        int Year, Month, Day, Hour, Minute, Second, NumSatellites, RetStd;
        int SourceYMD, SourceHMS, SourceNsat, SourcePos, ErrorStd;
        double Latitude, Longitude, Altitude, TimeAsSecsSince1970;
        long MicroSecond;


        // Call standard function

        RetStd = ParGpsGetCurrentTime( ParGpsHandle,
                                       &TimeAsSecsSince1970,
                                       &Year, &Month, &Day,
                                       &Hour, &Minute, &Second,
                                       &MicroSecond,
                                       &NumSatellites,
                                       &Latitude, &Longitude, &Altitude,
                                       &SourceYMD, &SourceHMS,
                                       &SourceNsat, &SourcePos,
                                       &ErrorStd );


        // Repackage values in alternate configuration

        if (IntReturns) {
                IntReturns[0]  = Year;
                IntReturns[1]  = Month;
                IntReturns[2]  = Day;
                IntReturns[3]  = Hour;
                IntReturns[4]  = Minute;
                IntReturns[5]  = Second;
                IntReturns[6]  = NumSatellites;
                IntReturns[7]  = SourceYMD;
                IntReturns[8]  = SourceHMS;
                IntReturns[9]  = SourceNsat;
                IntReturns[10] = SourcePos;
                }

        if (LongReturns) {
                LongReturns[0] = MicroSecond;
                }

        if (DoubleReturns) {
                DoubleReturns[0] = TimeAsSecsSince1970;
                DoubleReturns[1] = Latitude;
                DoubleReturns[2] = Longitude;
                DoubleReturns[3] = Altitude;
                }

        if (Error)
                *Error = ErrorStd;

        return( RetStd );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSplitCombinePpsData
// PURPOSE: This function is obsolete and will soon be removed, use
//          ParGpsFullSplitCombinePpsData instead.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSplitCombinePpsData( int           SplitCombine,
                                           unsigned int  Nvalues,
                                           void         *PpsData,
                                           int          *ValidFields,
                                           int          *PpsEventNum,
                                           int          *CountAtPpsHigh,
                                           unsigned int *CountAtPpsLow,
                                           int          *CountAtDreadyHigh,
                                           unsigned int *CountAtDreadyLow,
                                           int          *Error
                                            ) {

        return( ParGpsFullSplitCombinePpsData(
                 SplitCombine, Nvalues, PpsData, ValidFields, PpsEventNum,
                 CountAtPpsHigh, CountAtPpsLow, CountAtDreadyHigh, CountAtDreadyLow,
                 NULL, NULL, NULL, NULL, Error )
              );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsFullSplitCombinePpsData
// PURPOSE: Translate between the PPSTDATA structure and its individual elements.
//          This can make some things easier for LabView.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsFullSplitCombinePpsData(
                                           int           SplitCombine,
                                           unsigned int  Nvalues,
                                           void         *PpsData,
                                           int          *ValidFields,
                                           int          *PpsEventNum,
                                           int          *CountAtPpsHigh,
                                           unsigned int *CountAtPpsLow,
                                           int          *CountAtDreadyHigh,
                                           unsigned int *CountAtDreadyLow,
                                           int          *PctimeAtPpsHigh,
                                           unsigned int *PctimeAtPpsLow,
                                           int          *PctimeAtDreadyHigh,
                                           unsigned int *PctimeAtDreadyLow,
                                           int          *Error
                                            ) {

        unsigned int i, HavePctime;
        PPSTDATA    *PpsDataStructure;

        // Quick exit if not working with any data

        if (Nvalues == 0) {
                if (Error)
                        *Error = PARGPS_ERROR_NONE;
                return( 1 );
                }


        // Error exit if no structure

        if (!PpsData) {
                if (Error)
                        *Error = PARGPS_ERROR_BAD_PARAMETER;
                return( 0 );
                }

        PpsDataStructure = (PPSTDATA *)PpsData;


        // Split the structure into its pieces

        if (SplitCombine == 0) {

                for ( i = 0 ; i < Nvalues ; i++ ) {

                        if (ValidFields)
                                ValidFields[i] = PpsDataStructure[i].ValidFields;

                        if (PpsEventNum)
                                PpsEventNum[i] = PpsDataStructure[i].PpsEventNum;

                        if (CountAtPpsHigh)
                                CountAtPpsHigh[i] = PpsDataStructure[i].CountAtPps.u.HighPart;

                        if (CountAtPpsLow)
                                CountAtPpsLow[i] = PpsDataStructure[i].CountAtPps.u.LowPart;

                        if (CountAtDreadyHigh)
                                CountAtDreadyHigh[i] = PpsDataStructure[i].CountAtDready.u.HighPart;

                        if (CountAtDreadyLow)
                                CountAtDreadyLow[i] = PpsDataStructure[i].CountAtDready.u.LowPart;

                        if (PctimeAtPpsHigh)
                                PctimeAtPpsHigh[i] = PpsDataStructure[i].PctimeAtPps.u.HighPart;

                        if (PctimeAtPpsLow)
                                PctimeAtPpsLow[i] = PpsDataStructure[i].PctimeAtPps.u.LowPart;

                        if (PctimeAtDreadyHigh)
                                PctimeAtDreadyHigh[i] = PpsDataStructure[i].PctimeAtDready.u.HighPart;

                        if (PctimeAtDreadyLow)
                                PctimeAtDreadyLow[i] = PpsDataStructure[i].PctimeAtDready.u.LowPart;

                        } // end for i

                } // end split


        // Combine the pieces into the structure

        else { //  (SplitCombine == 1)

                // Must have all parts when combining

                if (!ValidFields        || !PpsEventNum      ||
                    !CountAtPpsHigh     || !CountAtPpsLow    ||
                    !CountAtDreadyHigh  || !CountAtDreadyLow ) {
                        if (Error)
                                *Error = PARGPS_ERROR_BAD_PARAMETER;
                        return( 0 );
                        }


                if (PctimeAtPpsHigh    && PctimeAtPpsLow   &&
                    PctimeAtDreadyHigh && PctimeAtDreadyLow)
                        HavePctime = 1;
                else
                        HavePctime = 0;

                for ( i = 0 ; i < Nvalues ; i++ ) {

                        PpsDataStructure[i].ValidFields               = ValidFields[i];
                        PpsDataStructure[i].PpsEventNum               = PpsEventNum[i];
                        PpsDataStructure[i].CountAtPps.u.HighPart     = CountAtPpsHigh[i];
                        PpsDataStructure[i].CountAtPps.u.LowPart      = CountAtPpsLow[i];
                        PpsDataStructure[i].CountAtDready.u.HighPart  = CountAtDreadyHigh[i];
                        PpsDataStructure[i].CountAtDready.u.LowPart   = CountAtDreadyLow[i];
                        if ( HavePctime ) {
                                PpsDataStructure[i].PctimeAtPps.u.HighPart    = PctimeAtPpsHigh[i];
                                PpsDataStructure[i].PctimeAtPps.u.LowPart     = PctimeAtPpsLow[i];
                                PpsDataStructure[i].PctimeAtDready.u.HighPart = PctimeAtDreadyHigh[i];
                                PpsDataStructure[i].PctimeAtDready.u.LowPart  = PctimeAtDreadyLow[i];
                                }
                        else {
                                PpsDataStructure[i].PctimeAtPps.u.HighPart    = 0L;
                                PpsDataStructure[i].PctimeAtPps.u.LowPart     = 0L;
                                PpsDataStructure[i].PctimeAtDready.u.HighPart = 0L;
                                PpsDataStructure[i].PctimeAtDready.u.LowPart  = 0L;
                                }

                        } // end for i

                } // end combine


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSplitCombineSerialData
// PURPOSE: Translate between the SERIALDATA structure and its individual
//          elements.  This can make some things easier for LabView.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSplitCombineSerialData( int           SplitCombine,
                                              unsigned int  Nvalues,
                                              void         *SerialData,
                                              int          *ValidFields,
                                              int          *PpsEventNum,
                                              int          *NmeaCount,
                                              char         *NmeaMsg,
                                              int          *Error
                                              ) {

        unsigned int i;
        int          istart;
        SERIALDATA  *SerialDataStructure;

        // Quick exit if not working with any data

        if (Nvalues == 0) {
                if (Error)
                        *Error = PARGPS_ERROR_NONE;
                return( 0 );
                }


        // Error exit if no structure

        if (!SerialData) {
                if (Error)
                        *Error = PARGPS_ERROR_BAD_PARAMETER;
                return( 0 );
                }

        SerialDataStructure = (SERIALDATA *)SerialData;
        istart = 0;



        // Split the structure into its pieces

        if (SplitCombine == 0) {

                for ( i = 0 ; i < Nvalues ; i++ ) {

                        if (ValidFields)
                                ValidFields[i] = SerialDataStructure[i].ValidFields;

                        if (PpsEventNum)
                                PpsEventNum[i] = SerialDataStructure[i].PpsEventNum;

                        if (NmeaCount)
                                NmeaCount[i] = SerialDataStructure[i].NmeaCount;

                        if (NmeaMsg)
                                memcpy( &NmeaMsg[istart], SerialDataStructure[i].NmeaMsg, MAX_NMEA_BUFF );


                        istart += MAX_NMEA_BUFF;

                        } // end for i

                } // end split


        // Combine the pieces into the structure

        else { //  (SplitCombine == 1)

                // Must have all parts when combining

                if (!ValidFields || !PpsEventNum ||
                    !NmeaCount || !NmeaMsg ) {
                        if (Error)
                                *Error = PARGPS_ERROR_BAD_PARAMETER;
                        return( 0 );
                        }


                for ( i = 0 ; i < Nvalues ; i++ ) {

                        SerialDataStructure[i].ValidFields = ValidFields[i];
                        SerialDataStructure[i].PpsEventNum = PpsEventNum[i];
                        SerialDataStructure[i].NmeaCount = NmeaCount[i];
                        memcpy( SerialDataStructure[i].NmeaMsg, &NmeaMsg[istart], MAX_NMEA_BUFF );
                        istart += MAX_NMEA_BUFF;

                        } // end for i

                } // end combine

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetStructureSize
// PURPOSE: Get the size in bytes of the PPSTDATA and SERIALDATA structures.
//          This can make some things easier for LabView.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetStructureSize( int *PpsDataSize, int *SerialDataSize ) {

        if (PpsDataSize)
                *PpsDataSize = sizeof( PPSTDATA );

        if (SerialDataSize)
                *SerialDataSize = sizeof( SERIALDATA );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsScaleGpsMark
// PURPOSE: Skip through an array of multiplexed data replacing non-zero values
//          in the indicated channel with a specified value.  This can make
//          some things easier for LabView.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsScaleGpsMark( int   MarkIndex,
                                    int   Nchannels,
                                    int   Nsamples,
                                    long  ScaleValue,
                                    long *Data,
                                    int  *Error
                                   ) {

        int i, j;

        // Quick exit if not working with any data

        if (Nchannels == 0 || Nsamples == 0) {
                if (Error)
                        *Error = PARGPS_ERROR_NONE;
                return( 1 );
                }


        // Error exit if no data array or bad mark index

        if (!Data || MarkIndex < 0 || MarkIndex > Nchannels) {
                if (Error)
                        *Error = PARGPS_ERROR_BAD_PARAMETER;
                return( 0 );
                }



        // Jump through multiplexed data array processing GPS Mark samples

        j = MarkIndex;

        for ( i = 0 ; i < Nsamples ; i++ ) {

                if ( Data[j] != 0L )
                    Data[j] = ScaleValue;

                j += Nchannels;
                }


        return( 1 );
}



/************************* NMEA HELPER FUNCTIONS *************************/

// This structure contains information about which fields in the various
// NMEA messages contain certain common types of information such as
// time and position.  The various NMEA messages are ranked in terms of
// how preferred they are for determining a particular quantity.  For
// example, for the PARGPS it is better to get time from a ZDA string
// than from a GGA string since the latter may stop incrementing when
// there are not enough satellites in view.

#define NORANK 99 // Must be greater than max num of NMEA strings

typedef struct NMEA_process {
        int     NmeaMsgIndex;
        char    *GPMsgId;
        int     ValidRank;
        int     ValidField;
        char    ValidValue;
        int     DateRank;
        int     DateField;
        int     DateLength;
        int     TimeRank;
        int     TimeField;
        int     LatLonRank;
        int     LatLonField;
        int     AltitudeRank;
        int     AltitudeField;
        int     NsatRank;
        int     NsatField;
        } NMEAPROCESS;

NMEAPROCESS NmeaProcessTable[] = {

//                        Valid             Date           Time        LatLon      Alt         Nsat
NMEA_MSGID_RMC, "GPRMC",       1, 2,  'A',       1, 9, 1,       1, 1,       2, 3,  NORANK, 0,  NORANK, 0,
NMEA_MSGID_ZDA, "GPZDA",  NORANK, 0, '\0',       2, 2, 3,       2, 1,  NORANK, 0,  NORANK, 0,  NORANK, 0,
NMEA_MSGID_GGA, "GPGGA",       1, 6,  '1',  NORANK, 0, 0,       3, 1,       1, 2,       1, 9,       1, 7,
NMEA_MSGID_GLL, "GPGLL",       1, 6,  'A',  NORANK, 0, 0,       4, 5,       3, 1,  NORANK, 0,  NORANK, 0,
NMEA_MSGID_VTG, "GPVTG",  NORANK, 0, '\0',  NORANK, 0, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,
NMEA_MSGID_GSA, "GPGSA",  NORANK, 0, '\0',  NORANK, 0, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,
NMEA_MSGID_GSV, "GPGSV",  NORANK, 0, '\0',  NORANK, 0, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,       2, 3,
NMEA_MSGID_MIN, NULL,     NORANK, 0, '\0',  NORANK, 0, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0
        };


//------------------------------------------------------------------------------
// ROUTINE: ParGpsRequestNmeaInfo2
// PURPOSE: Process a collection of NMEA message strings and extract the
//          requested items.  These include things like the date, time,
//          position, and number of satellites.  This enhanced version also
//          returns info indicating if the fields came from valid NMEA strings
//          or not.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsRequestNmeaInfo2( char *NmeaMsgAll, int NmeaCount,
                int *Year, int *Mon, int *Day, int *YmdSource, int *YmdIsValid,
                int *Hour, int *Minute, int *Second,
                long *MicroSecond, int *HmsSource, int *HmsIsValid,
                double *Latitude, double *Longitude, int *LocSource, int *LocIsValid,
                double *Altitude, int *AltSource, int *AltIsValid,
                int *Nsat, int *SatSource, int *SatIsValid ) {

        int    DateOk,      TimeOk,      LocOk,      AltOk,      SatOk;
        int    DateSrc,     TimeSrc,     LocSrc,     AltSrc,     SatSrc;
        int    DateAsk,     TimeAsk,     LocAsk,     AltAsk,     SatAsk;
        int    DateValid,   TimeValid,   LocValid,   AltValid,   SatValid;
        int    DateRank,    TimeRank,    LocRank,    AltRank,    SatRank;
        int    DateTmpSrc,  TimeTmpSrc,  LocTmpSrc,  AltTmpSrc,  SatTmpSrc;
        int    DateTmpRank, TimeTmpRank, LocTmpRank, AltTmpRank, SatTmpRank;
        int    MsgIsValid, Yr, Mo, Dy, Hh, Mm, Ss, Sat, i, imsg, Ok;
        int    Tmp1, Tmp2, Tmp3;
        long   Usec, LTmp;
        char   MsgChar;
        double Lat, Lon, Alt, DTmp1, DTmp2;
        NMEAPARSE ParseData;
        NMEAPROCESS *t;

        // Note: Use of t->NmeaMsgIndex for setting XxxSrc assumes
        //       SRCSTR_ defines match NMEA_MSGID_ defines



        // Error check

        if ( !NmeaMsgAll || NmeaCount == 0 )
                return( 0 );


        // Initialize local values

        Yr = Mo = Dy = Hh = Mm = Ss = Sat = 0;
        Usec = 0L;
        Lat = Lon = Alt = 0.0L;



        // Identify items to gather from NMEA messages

        DateSrc     = TimeSrc     = LocSrc     = AltSrc     = SatSrc     = SRCSTR_UNK;
        DateTmpSrc  = TimeTmpSrc  = LocTmpSrc  = AltTmpSrc  = SatTmpSrc  = SRCSTR_UNK;
        DateValid   = TimeValid   = LocValid   = AltValid   = SatValid   = 0;
        DateAsk     = TimeAsk     = LocAsk     = AltAsk     = SatAsk     = 0;
        DateRank    = TimeRank    = LocRank    = AltRank    = SatRank    = NORANK;
        DateTmpRank = TimeTmpRank = LocTmpRank = AltTmpRank = SatTmpRank = NORANK;

        if ( Year || Mon || Day || YmdSource )
                DateAsk = 1;
        if ( Hour || Minute || Second || MicroSecond || HmsSource )
                TimeAsk = 1;
        if ( Latitude || Longitude || LocSource )
                LocAsk = 1;
        if ( Altitude || AltSource )
                AltAsk = 1;
        if ( Nsat || SatSource )
                SatAsk = 1;


        // Process each available NMEA message in turn

        for ( i = 0 ; i < NmeaCount ; i++ ) {

                // Break it up into its component fields

                imsg = i * MAX_NMEA_SIZE;
                Ok = ParGpsParseNmeaData( &NmeaMsgAll[imsg], &ParseData );
                if ( !Ok )
                        continue; // skip this unparsable msg and go on to next



                // Select NMEA table entry (t) that matches the parsed data id

                Ok = ParGpsMatchNmeaTable( ParseData.Field[0], &t );
                if ( !Ok )
                        continue; // skip this unknown msg and go on to next

                

                // Check NMEA message for validity.  Assume it is valid by default.
                // If message has a validity rank, then read validity indicator 
                // character and compare it with the required valid value.
                // GGA string is valid if it matches alternate valid value.

                MsgIsValid = 1;

                if ( t->ValidRank < NORANK ) {

                        sscanf( ParseData.Field[t->ValidField], "%c", &MsgChar );

                        if ( MsgChar != t->ValidValue )
                                MsgIsValid = 0;
                        
                        if ( t->NmeaMsgIndex == NMEA_MSGID_GGA  &&  MsgChar == '2' )
                                MsgIsValid = 1;
                        }




                // Now gather asked for data.  Messages are ranked in
                // terms of which is best for providing each piece of
                // data.  Only fill in data if it is asked for and this
                // message has a lower and better rank than the one that
                // supplied the current data values.

                // Look for date (YMD)

                if ( DateAsk  &&  (t->DateRank < DateRank) ) {

                        DateOk = ParGpsExtractParsedDate( &ParseData,
                                                          t->DateLength,
                                                          t->DateField,
                                                          &Tmp1, &Tmp2, &Tmp3 );
                        if ( DateOk ) {
                                if ( MsgIsValid ) {
                                        Yr          = Tmp1;
                                        Mo          = Tmp2;
                                        Dy          = Tmp3;
                                        DateTmpRank = t->DateRank;
                                        DateTmpSrc  = t->NmeaMsgIndex;

                                        DateRank    = t->DateRank;
                                        DateSrc     = t->NmeaMsgIndex;
                                        DateValid   = 1;
                                        }
                                else if ( !DateValid && (t->DateRank < DateTmpRank) ) {
                                        Yr          = Tmp1;
                                        Mo          = Tmp2;
                                        Dy          = Tmp3;
                                        DateTmpRank = t->DateRank;
                                        DateTmpSrc  = t->NmeaMsgIndex;
                                        }
                                }

                        } // end DateAsk



                // Look for time (HMS)

                if ( TimeAsk  &&  (t->TimeRank < TimeRank)  ) {

                        TimeOk = ParGpsExtractParsedTime( &ParseData,
                                                          t->TimeField,
                                                          &Tmp1, &Tmp2, &Tmp3,
                                                          &LTmp );

                        if ( TimeOk ) {
                                if ( MsgIsValid ) {
                                        Hh          = Tmp1;
                                        Mm          = Tmp2;
                                        Ss          = Tmp3;
                                        Usec        = LTmp;
                                        TimeTmpRank = t->TimeRank;
                                        TimeTmpSrc  = t->NmeaMsgIndex;

                                        TimeRank    = t->TimeRank;
                                        TimeSrc     = t->NmeaMsgIndex;
                                        TimeValid   = 1;
                                        }
                                else if ( !TimeValid && (t->TimeRank < TimeTmpRank) ) {
                                        Hh          = Tmp1;
                                        Mm          = Tmp2;
                                        Ss          = Tmp3;
                                        Usec        = LTmp;
                                        TimeTmpRank = t->TimeRank;
                                        TimeTmpSrc  = t->NmeaMsgIndex;
                                        }
                                }

                        } // end TimeAsk


                // Look for location (Lat,Lon)

                if ( LocAsk  &&  (t->LatLonRank < LocRank) ) {

                        LocOk = ParGpsExtractParsedLoc( &ParseData,
                                                        t->LatLonField,
                                                        &DTmp1, &DTmp2 );
                                if ( LocOk ) {
                                        if ( MsgIsValid ) {
                                                Lat        = DTmp1;
                                                Lon        = DTmp2;
                                                LocTmpRank = t->LatLonRank;
                                                LocTmpSrc  = t->NmeaMsgIndex;

                                                LocRank    = t->LatLonRank;
                                                LocSrc     = t->NmeaMsgIndex;
                                                LocValid   = 1;
                                                }
                                        else if ( !LocValid && (t->LatLonRank < LocTmpRank) ) {
                                                Lat        = DTmp1;
                                                Lon        = DTmp2;
                                                LocTmpRank = t->LatLonRank;
                                                LocTmpSrc  = t->NmeaMsgIndex;
                                                }
                                        }

                        } // end LocAsk


                // Look for altitude

                if ( AltAsk  &&  (t->AltitudeRank < AltRank) ) {

                        AltOk = ParGpsExtractParsedAlt( &ParseData,
                                                        t->AltitudeField,
                                                        t->AltitudeRank,
                                                        &DTmp1 );
                                if ( AltOk ) {
                                        if ( MsgIsValid ) {
                                                Alt        = DTmp1;
                                                AltTmpRank = t->AltitudeRank;
                                                AltTmpSrc  = t->NmeaMsgIndex;

                                                AltRank    = t->AltitudeRank;
                                                AltSrc     = t->NmeaMsgIndex;
                                                AltValid   = 1;
                                                }
                                        else if ( !AltValid && (t->AltitudeRank < AltTmpRank) ) {
                                                Alt        = DTmp1;
                                                AltTmpRank = t->AltitudeRank;
                                                AltTmpSrc  = t->NmeaMsgIndex;
                                                }
                                        }

                        } // end AltAsk

                
                // Look for nsat

                if ( SatAsk  &&  (t->NsatRank < SatRank) ) {

                        SatOk = ParGpsExtractParsedSat( &ParseData,
                                                        t->NsatField,
                                                        &Tmp1 );
                        if ( SatOk ) {
                                if ( MsgIsValid ) {
                                        Sat        = Tmp1;
                                        SatTmpRank = t->NsatRank;
                                        SatTmpSrc  = t->NmeaMsgIndex;
                                                        
                                        SatRank    = t->NsatRank;
                                        SatSrc     = t->NmeaMsgIndex;
                                        SatValid   = 1;
                                        }
                                else if ( !SatValid && (t->NsatRank < SatTmpRank) ) {
                                        Sat        = Tmp1;
                                        SatTmpRank = t->NsatRank;
                                        SatTmpSrc  = t->NmeaMsgIndex;
                                        }
                                }

                        } // end SatAsk


                } // end for i < NmeaCount


        // Always return number of satellites even if no messages valid

        if ( !SatValid ) {
                SatRank = SatTmpRank;
                SatSrc  = SatTmpSrc;
                }



        // If we looked for the time, we ensure it is aligned on a
        // second since the various GPS receivers don't always do this.

        if ( TimeAsk ) {

                TimeOk = ParGpsRoundTimeToPps( &Yr, &Mo, &Dy,
                                               &Hh, &Mm, &Ss, &Usec );
                }



        // Fill in requested fields

        if ( Year )               *Year = Yr;
        if ( Mon )                 *Mon = Mo;
        if ( Day )                 *Day = Dy;
        if ( YmdSource )     *YmdSource = DateTmpSrc;
        if ( YmdIsValid )   *YmdIsValid = DateValid;

        if ( Hour )               *Hour = Hh;
        if ( Minute )           *Minute = Mm;
        if ( Second )           *Second = Ss;
        if ( MicroSecond ) *MicroSecond = Usec;
        if ( HmsSource )     *HmsSource = TimeTmpSrc;
        if ( HmsIsValid )   *HmsIsValid = TimeValid;

        if ( Latitude )       *Latitude = Lat;
        if ( Longitude )     *Longitude = Lon;
        if ( LocSource )     *LocSource = LocTmpSrc;
        if ( LocIsValid )   *LocIsValid = LocValid;
        
        if ( Altitude )       *Altitude = Alt;
        if ( AltSource )     *AltSource = AltTmpSrc;
        if ( AltIsValid )   *AltIsValid = AltValid;

        if ( Nsat )               *Nsat = Sat;
        if ( SatSource )     *SatSource = SatTmpSrc;
        if ( SatIsValid )   *SatIsValid = SatValid;

        // Note: When calling this function and getting a valid return,
        //       you should always check the IsValid and Source variables 
        //       to be sure you really got the quantities you requested and
        //       not just defaults.

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsRequestNmeaInfo
// PURPOSE: Process a collection of NMEA message strings and extract the
//          requested items.  These include things like the date, time,
//          position, and number of satellites.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsRequestNmeaInfo( char *NmeaMsgAll, int NmeaCount,
                int *Year, int *Mon, int *Day, int *YmdSource,
                int *Hour, int *Minute, int *Second,
                long *MicroSecond, int *HmsSource,
                double *Latitude, double *Longitude,
                double *Altitude, int *PosSource,
                int *Nsat, int *SatSource ) {

        int  Ok, AltSource, LocSource;
        int YmdValid, HmsValid, LocValid, AltValid, SatValid;

        Ok = ParGpsRequestNmeaInfo2( NmeaMsgAll, NmeaCount,
                               Year, Mon, Day, YmdSource, &YmdValid,
                               Hour, Minute, Second, MicroSecond, HmsSource, &HmsValid,
                               Latitude, Longitude, &LocSource, &LocValid,
                               Altitude, &AltSource, &AltValid,
                               Nsat, SatSource, &SatValid );

        if ( LocValid )
                if ( PosSource )  *PosSource = LocSource;
        if ( !YmdValid )
                if ( YmdSource )  *YmdSource = SRCSTR_UNK;
        if ( !HmsValid )
                if ( HmsSource )  *HmsSource = SRCSTR_UNK;
        if ( !LocValid )
                if ( PosSource )  *PosSource = SRCSTR_UNK;
        if ( !SatValid )
                if ( SatSource )  *SatSource = SRCSTR_UNK;

        return( Ok );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsParseNmeaData
// PURPOSE: Split the provided NMEA message string into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsParseNmeaData( char *NmeaMsg, NMEAPARSE *ParseData ) {

        char *field, c;
        int   done, ifield, checksum, tmpsum, chcount;


        ParGpsLastErrorMessage[0] = '\0';

        
        // Error check Nmea string

        if ( !NmeaMsg ) {
                sprintf( ParGpsLastErrorMessage, "No input NMEA string given." );
                return( 0 );
                }
        if ( *NmeaMsg++ != '$' ) {
                sprintf( ParGpsLastErrorMessage, "NMEA string must start with $." );
                return( 0 );
                }



        // Initialze ParseData structure

        ParseData->Nfields  = 0;
        ParseData->CheckSum = 0;
        memset( ParseData->Field, '\0', MAX_NMEA_FIELDS*MAX_NMEA_FIELDSIZE );


        // Initialize local variables

        done     = 0;
        chcount  = 0;
        checksum = 0;
        tmpsum   = 0;
        ifield   = 0;
        field    = ParseData->Field[ifield];


        // Parsing loop

        while ( !done ) {

                c = *NmeaMsg++;

                switch ( c ) {

                // Found unexpected start of next string

                        case '$':
                                sprintf( ParGpsLastErrorMessage,
                                         "Found extra $ in NMEA string." );
                                return( 0 );
                                break;


                // Found end of current field

                        case ',':
                        case '*':
                        case '\r':
                                *field = '\0';
                                chcount = 0;
                                ifield++;
                                if (ifield < MAX_NMEA_FIELDS)
                                        field = ParseData->Field[ifield];
                                else {
                                        sprintf( ParGpsLastErrorMessage,
                                           "Too many fields in NMEA string (%d >= %d).",
                                           ifield, MAX_NMEA_FIELDS );
                                        return( 0 );
                                        }
                                if (c == '*')
                                        checksum = tmpsum;
                                else
                                        tmpsum ^= c;
                                break;


                // Finished with string
                // Note: Null termination of field strings provided by
                // original memset zeroing remaining without overwrite

                        case '\n':
                        case '\0':
                                done = 1;
                                break;


                // Add current character to field string and checksum

                        default:
                                chcount++;
                                if (chcount > MAX_NMEA_FIELDSIZE) {
                                        sprintf( ParGpsLastErrorMessage,
                                           "Too many characters in NMEA field %d (%d >= %d).",
                                           ifield, chcount, MAX_NMEA_FIELDSIZE );
                                        return( 0 );
                                        }
                                *field++ = c;
                                tmpsum ^= c;

                        }

                } // end while



        // Set checksum and validate

        field = ParseData->Field[ifield-1];
        sscanf( field, "%X", &ParseData->CheckSum );
        if ( ParseData->CheckSum != checksum ) {
                sprintf( ParGpsLastErrorMessage,
                         "Invalid checksum in NMEA string (computed 0x%X, read 0x%X).",
                         checksum, ParseData->CheckSum );
                return( 0 );
                }


        // Set number of fields since all is good
        
        ParseData->Nfields = ifield;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsMatchNmeaTable
// PURPOSE: Given a NMEA message string, determine which entry in the
//          NMEAPROCESS table has a matching message id.  The NMEAPROCESS table
//          contains information for interpreting the data fields in the various
//          NMEA messages.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsMatchNmeaTable( char *MsgId, NMEAPROCESS **t ) {

        NMEAPROCESS *TableEntry;


        if (t)  *t = NULL;


        // Point to start of table

        TableEntry = NmeaProcessTable;


        // Look for table entry with matching message id

        while ( TableEntry->GPMsgId  &&  strcmp( MsgId, TableEntry->GPMsgId ) )
                TableEntry++;


        // Make sure we are not off the end of the table (ie message type unknown)

        if ( TableEntry->GPMsgId == NULL )
                return( 0 );


        // Success, return pointer to matching entry

        if (t)  *t = TableEntry;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsExtractParsedDate
// PURPOSE: Determine the Year/Month/Day date information from a NMEA message
//          string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsExtractParsedDate( NMEAPARSE *ParseData,
                                         int DateLength,
                                         int DateField,
                                         int *pYr,
                                         int *pMo,
                                         int *pDy ) {
        int DateOk, ddmmyy;

        DateOk = 0;

        if ( DateLength == 1
             && (sscanf( ParseData->Field[DateField], "%d", &ddmmyy ) == 1)
                   ) {
                        ParGpsUnpack3( ddmmyy, pDy, pMo, pYr );
                        *pYr += 2000;
                        DateOk = 1;
                }

        else if ( DateLength == 3
                  && (sscanf( ParseData->Field[DateField+0], "%d", pDy ) == 1)
                  && (sscanf( ParseData->Field[DateField+1], "%d", pMo ) == 1)
                  && (sscanf( ParseData->Field[DateField+2], "%d", pYr ) == 1)
                                ) {
                        DateOk = 1;
                }

        return( DateOk );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsExtractParsedTime
// PURPOSE: Determine the Hour:Minute:Second time information from a NMEA message
//          string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsExtractParsedTime( NMEAPARSE *ParseData,
                                         int TimeField,
                                         int *pHh,
                                         int *pMm,
                                         int *pSs,
                                         long *pUsec ) {
        int TimeOk, hhmmss, sf, sh;
        double fulltime;

        TimeOk = 0;

        // Extract both the full and integer hhmmss clock part of the time.

        sf = sscanf( ParseData->Field[TimeField], "%lf", &fulltime );
        sh = sscanf( ParseData->Field[TimeField], "%d", &hhmmss );


        // Unpack clock part of time into HMS.  This should be
        // sufficient since the PPS mark is exactly on the second mark.

        if (sh == 1) {

                ParGpsUnpack3( hhmmss, pHh, pMm, pSs );
                *pUsec = 0L;
                TimeOk = 1;


                // But we'll try to get the remaining fractional part too.

                if (sf == 1)
                        *pUsec = (long)((fulltime - (double)hhmmss) * USPERSEC);

                }

        return( TimeOk );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsExtractParsedLoc
// PURPOSE: Determine the Latitude-Longitude location information from a NMEA
//          message string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsExtractParsedLoc( NMEAPARSE *ParseData,
                                        int LatLonField,
                                        double *pLat,
                                        double *pLon ) {

        char LatQuad, LonQuad;
        int  LocOk;

        LocOk = 0;

        if ( (sscanf( ParseData->Field[LatLonField],   "%lf", pLat ) == 1)
          && (sscanf( ParseData->Field[LatLonField+1], "%c",  &LatQuad ) == 1)
          && (sscanf( ParseData->Field[LatLonField+2], "%lf", pLon ) == 1)
          && (sscanf( ParseData->Field[LatLonField+3], "%c",  &LonQuad ) == 1)
           ) {
                LocOk = 1;
                if (LatQuad == 'S')
                        *pLat = -(*pLat);
                else if (LatQuad != 'N')
                        LocOk = 0;

                if (LonQuad == 'W')
                        *pLon = -(*pLon);
                else if (LonQuad != 'E')
                        LocOk = 0;

                } // end if sscanf

        return( LocOk );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsExtractParsedAlt
// PURPOSE: Determine the Altitude information from a NMEA message string that
//          has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsExtractParsedAlt( NMEAPARSE *ParseData,
                                        int AltitudeField,
                                        int AltitudeRank,
                                        double *pAlt ) {

        char AltUnits;
        int  AltOk;

        AltOk = 0;

        // Check altitude, forcing units as meters

        if ( (sscanf( ParseData->Field[AltitudeField],   "%lf", pAlt )  == 1)
          && (sscanf( ParseData->Field[AltitudeField+1], "%c",  &AltUnits ) == 1)
          && (AltUnits == 'M')
           ) {
                AltOk = 1;
                }

        return( AltOk );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsExtractParsedPos
// PURPOSE: Determine the Latitude-Longitude-Altitude position information from
//          a NMEA message string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsExtractParsedPos( NMEAPARSE *ParseData,
                                        int LatLonField,
                                        int AltitudeField,
                                        int AltitudeRank,
                                        double *pLat,
                                        double *pLon,
                                        double *pAlt ) {

        char LatQuad, LonQuad, AltUnits;
        int  PosOk;

        PosOk = 0;

        if ( (sscanf( ParseData->Field[LatLonField],   "%lf", pLat ) == 1)
          && (sscanf( ParseData->Field[LatLonField+1], "%c",  &LatQuad ) == 1)
          && (sscanf( ParseData->Field[LatLonField+2], "%lf", pLon ) == 1)
          && (sscanf( ParseData->Field[LatLonField+3], "%c",  &LonQuad ) == 1)
           ) {
                PosOk = 1;
                if (LatQuad == 'S')
                        *pLat = -(*pLat);
                else if (LatQuad != 'N')
                        PosOk = 0;

                if (LonQuad == 'W')
                        *pLon = -(*pLon);
                else if (LonQuad != 'E')
                        PosOk = 0;


                // Check altitude, forcing units as meters

                if (AltitudeRank != NORANK) {

                        PosOk = 0;
                        if ( (sscanf( ParseData->Field[AltitudeField],   "%lf", pAlt )  == 1)
                          && (sscanf( ParseData->Field[AltitudeField+1], "%c",  &AltUnits ) == 1)
                          && (AltUnits == 'M')
                           ) {
                                PosOk = 1;
                                }
                        } // end if AltitudeRank

                } // end if sscanf

        return( PosOk );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsExtractParsedSat
// PURPOSE: Determine the number of satellites information from a NMEA message
//          string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsExtractParsedSat( NMEAPARSE *ParseData,
                                        int NsatField,
                                        int *pSat ) {
        int SatOk;

        SatOk = 0;
        if (sscanf( ParseData->Field[NsatField], "%d", pSat ) == 1) {
                SatOk = 1;
                }

        return( SatOk );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsRoundTimeToPps
// PURPOSE: Correct the GPS PPS time so it falls exactly on the second.
//
//          By definition, the PPS signal always arrives at the start
//          of a second.  So the microsecond part of the time should
//          be 0.  But, the various GPS receivers don't always do this.
//          So this routine forces the PPS time to be on a second by
//          rounding up or down as appropriate.
//
//          For the PARGPS Trimble receiver, the ZDA time is the time
//          when the string comes out and not the time of the PPS.  Thus
//          the times typically end in .2 and we don't want to exclude
//          the fractional part.  For the Motorola Oncore ZDA time is the
//          PPS time so it usually ends in .00.  But occasionally it ends
//          in .99 with the second not incremented.  So, we want to round
//          up to the next second.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsRoundTimeToPps( int *Year, int *Month, int *Day,
                                      int *Hour, int *Minute, int *Second,
                                      long *MicroSecond ) {

        int    Yr, Mo, Dy, Hh, Mm, Ss, Ret;
        long   Usec;
        double SecSince1970;



        // Check for most trivial case of rounding down

        if (!MicroSecond) // error check
                return( 0 );

        if (*MicroSecond <= 500000L) {  // <= .5 sec
                *MicroSecond = 0L;
                return( 1 );
                }


        // Check for simple case of rounding up with no seconds carry

        if (!Second) // error check
                return( 0 );

        if (*Second < 59) {
                *MicroSecond = 0L;
                *Second      = *Second + 1;
                return( 1 );
                }


        // Check for case of rounding up with seconds but not minutes carry

        if (!Minute) // error check
                return( 0 );

        if (*Minute < 59) {
                *MicroSecond = 0L;
                *Second      = *Second - 59; // + 1 - 60;
                *Minute      = *Minute + 1;
                return( 1 );
                }


        // Check for rounding up with seconds + minutes but not hours carry

        if (!Hour) // error check
                return( 0 );

        if (*Hour < 23) {
                *MicroSecond = 0L;
                *Second      = *Second - 59; // + 1 - 60 sec/min;
                *Minute      = *Minute - 59; // + 1 - 60 min/hr;
                *Hour        = *Hour   + 1;
                return( 1 );
                }


        // Ok, remaining cases involve a date change so make
        // the library time functions handle them


        Hh   = *Hour;                    // Set up time values
        Mm   = *Minute;
        Ss   = *Second;
        Usec = *MicroSecond;

        if ( Year && Month && Day ) {   // Use provided date
                Yr = *Year;
                Mo = *Month;
                Dy = *Day;
                }
        else {                          // Use default date
                Yr = 1970;
                Mo = 1;
                Dy = 1;
                }



        // To round up, we'll truncate Usec, convert time/date into
        // seconds since 1970, inc by a second, and convert back

        Usec = 0L;                 // Here's the truncate

        Ret = ParGpsSecTimeCombine( Yr, Mo, Dy, Hh, Mm, Ss,
                                    Usec, &SecSince1970 );
        if (!Ret)
                return( 0 );

        SecSince1970 += 1.00000L;  // Here's the inc

        Ret = ParGpsSecTimeSplit( SecSince1970,
                                  &Yr, &Mo, &Dy, &Hh, &Mm, &Ss, &Usec );
        if (!Ret)
                return( 0 );

        if ( Usec != 0L )         // Sainty check
                return( 0 );


        // Return updated values

        if ( Year )               *Year = Yr;
        if ( Month )             *Month = Mo;
        if ( Day )                 *Day = Dy;

        if ( Hour )               *Hour = Hh;
        if ( Minute )           *Minute = Mm;
        if ( Second )           *Second = Ss;
        if ( MicroSecond ) *MicroSecond = Usec;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsAdjustNmeaInfo
// PURPOSE: Adjust the provided NMEA to ensure a valid YMD and HMS if possible.
//          Information from previous seconds may be used.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsAdjustNmeaInfo( int Yr, int Mo, int Dy, int *YmdSrc,
                                      int Hh, int Mm, int Ss, long Us, int *HmsSrc,
                                      unsigned int EventNum,
                                      int SerialDelay,
                                      unsigned int *PrevEventNum,
                                      double *PrevSecSince1970 ) {

        double SecSince1970;

        // Note: SerialDelay corrects for the delay between the NMEA
        // string being used and the PPS being timed (eg Serial
        // PpsEventNum != PPS PpsEventNum).  This does not apply if
        // the time and date are taken from a previous PPS time.



        // Take YMD and HMS from NMEA and combine to a double
        // representing number of seconds since 1970

        if ( (SRCSTR_MIN < *YmdSrc)  &&  (*YmdSrc < SRCSTR_PC) &&
             (SRCSTR_MIN < *HmsSrc)  &&  (*HmsSrc < SRCSTR_PC) ) {
                ParGpsSecTimeCombine( Yr, Mo, Dy, Hh, Mm, Ss, Us, &SecSince1970);
                SecSince1970 += SerialDelay;
                }



        // Compute time from previous time plus the number of elapsed seconds

        else if ( *PrevEventNum != INVALID_PPSEVENT ) {

                *YmdSrc = *HmsSrc = SRCSTR_OLD;

                SecSince1970 = *PrevSecSince1970
                                + (EventNum - *PrevEventNum);
                }


        // Can't adjust time

        else {
                return( 0 );
                }


        // Save valid info as previous for next pass

        *PrevEventNum     = EventNum;
        *PrevSecSince1970 = SecSince1970;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetNmeaSatellites
// PURPOSE: Return the number of satellites as determined from the provided NMEA
//          message string.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetNmeaSatellites( char *NmeaMsg, int *NumSat, int *Source ) {

        int Src, Nsat, Valid, Ok;


        // Parse NMEA string into data tokens
        // Extract number of satellites from tokens

        Ok = ParGpsRequestNmeaInfo2( NmeaMsg, 1,
                                     NULL, NULL, NULL, NULL, NULL,       // date
                                     NULL, NULL, NULL, NULL, NULL, NULL, // time
                                     NULL, NULL, NULL, NULL,             // loc
                                     NULL, NULL, NULL,                   // alt
                                     &Nsat, &Src, &Valid );


        if (Ok && Src < MAX_NMEA_TYPE) {
                if (NumSat)  *NumSat = Nsat;
                if (Source)  *Source = Src;
                return( 1 );
                }
        else {
                if (NumSat)  *NumSat = 0;
                if (Source)  *Source = SRCSTR_MIN;
                return( 0 );
                }
}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetNmeaTime
// PURPOSE: Return the time in seconds since 1970 as determined from the
//          provided NMEA message string.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetNmeaTime( char *NmeaMsg, double *Time,
                                   int *SourceYMD, int *SourceHMS ) {

        int    hh, mm, ss, yr, mo, dy, SrcYmd, SrcHms, ValYmd, ValHms, Ok;
        long   us;
        double Tm;


        // Parse NMEA string into data tokens
        // Extract date and time from NMEA string tokens
        // Combine the parts into the total time

        Ok = ParGpsRequestNmeaInfo2( NmeaMsg, 1,
                                     &yr, &mo, &dy, &SrcYmd, &ValYmd,      // date
                                     &hh, &mm, &ss, &us, &SrcHms, &ValHms, // time
                                     NULL, NULL, NULL, NULL,               // loc
                                     NULL, NULL, NULL,                     // alt
                                     NULL, NULL, NULL );                   // sat

        if ( Ok  &&  (SrcYmd < MAX_NMEA_TYPE)  &&  (SrcHms < MAX_NMEA_TYPE) ) {
                Ok = ParGpsSecTimeCombine( yr, mo, dy, hh, mm, ss, us, &Tm );
                }

        if ( Ok ) {
                if (SourceYMD)  *SourceYMD = SrcYmd;
                if (SourceHMS)  *SourceHMS = SrcHms;
                if (Time)       *Time      = Tm;
                return( 1 );
                }
        else {
                if (SourceYMD)  *SourceYMD = 0;
                if (SourceHMS)  *SourceHMS = 0;
                if (Time)       *Time      = 0.0;
                return( 0 );
                }

}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetNmeaHMS
// PURPOSE: Return the Hour:Minute:Second time as determined from the provided
//          NMEA message string.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetNmeaHMS( char *NmeaMsg, double *HMS,
                                  int *Hour, int *Minute, int *Second,
                                  long *MicroSecond, int *Source ) {

        int    hh, mm, ss, SrcHms, ValHms, Ok;
        long   us;
        double hmstime;


        // Parse NMEA string into data tokens
        // Extract clock part of time from the NMEA tokens
        // Combine the parts into the total time

        Ok = ParGpsRequestNmeaInfo2( NmeaMsg, 1,
                                     NULL, NULL, NULL, NULL, NULL,         // date
                                     &hh, &mm, &ss, &us, &SrcHms, &ValHms, // time
                                     NULL, NULL, NULL, NULL,               // loc
                                     NULL, NULL, NULL,                     // alt
                                     NULL, NULL, NULL );                   // sat

        if ( Ok  &&  (SrcHms < MAX_NMEA_TYPE) ) {
                Ok = ParGpsSecTimeCombine( 0, 0, 0, hh, mm, ss, us, &hmstime );
                }

        if ( Ok ) {
                if (HMS)         *HMS         = hmstime;
                if (Hour)        *Hour        = hh;
                if (Minute)      *Minute      = mm;
                if (Second)      *Second      = ss;
                if (MicroSecond) *MicroSecond = us;
                if (Source)      *Source      = SrcHms;
                return( 1 );
                }
        else {
                if (HMS)         *HMS         = 0.0;
                if (Hour)        *Hour        = 0;
                if (Minute)      *Minute      = 0;
                if (Second)      *Second      = 0;
                if (MicroSecond) *MicroSecond = 0L;
                if (Source)      *Source      = SRCSTR_MIN;
                return( 0 );
                }

}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetNmeaYMD
// PURPOSE: Return the Year/Month/Day date as determined from the provided
//          NMEA message string.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetNmeaYMD( char *NmeaMsg, double *YMD,
                                  int *Year, int *Month, int *Day, int *Source ) {

        double YmdTime;
        int    Yr, Mo, Dy, SrcYmd, ValYmd, Ok;


        // Parse NMEA string into data tokens
        // Extract date from NMEA string tokens
        // Combine the parts into the total date

        Ok = ParGpsRequestNmeaInfo2( NmeaMsg, 1,
                                     &Yr, &Mo, &Dy, &SrcYmd, &ValYmd,    // date
                                     NULL, NULL, NULL, NULL, NULL, NULL, // time
                                     NULL, NULL, NULL, NULL,             // loc
                                     NULL, NULL, NULL,                   // alt
                                     NULL, NULL, NULL );                 // sat

        if ( Ok  &&  (SrcYmd < MAX_NMEA_TYPE) ) {
                Ok = ParGpsSecTimeCombine( Yr, Mo, Dy, 0, 0, 0, 0, &YmdTime );
                }

        if ( Ok ) {
                if (YMD)    *YMD    = YmdTime;
                if (Year)   *Year   = Yr;
                if (Month)  *Month  = Mo;
                if (Day)    *Day    = Dy;
                if (Source) *Source = SrcYmd;
                return( 1 );
                }
        else {
                if (YMD)    *YMD    = 0.0;
                if (Year)   *Year   = 0;
                if (Month)  *Month  = 0;
                if (Day)    *Day    = 0;
                if (Source) *Source = SRCSTR_MIN;
                return( 0 );
                }
}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetNmeaPosition
// PURPOSE: Return the Latitude-Longitude-Altitude position as determined from
//          the provided NMEA message string.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetNmeaPosition( char *NmeaMsg,
                                      double *Latitude, double *Longitude,
                                      double *Altitude, int *Source ) {

        int    SrcLoc, ValLoc, SrcAlt, ValAlt, Ok;
        double Lat, Lon, Alt;


        // Parse NMEA string into data tokens
        // Extract position information from tokens

        Ok = ParGpsRequestNmeaInfo2( NmeaMsg, 1,
                                     NULL, NULL, NULL, NULL, NULL,        // date
                                     NULL, NULL, NULL, NULL, NULL, NULL,  // time
                                     &Lat, &Lon,  &SrcLoc, &ValLoc,       // loc
                                     &Alt, &SrcAlt, &ValAlt,              // alt
                                     NULL, NULL, NULL );                  // sat


        if ( Ok && (SrcLoc < MAX_NMEA_TYPE) ) {
                if (Latitude)  *Latitude  = Lat;
                if (Longitude) *Longitude = Lon;
                if (Altitude)  *Altitude  = Alt;
                if (Source)    *Source    = SrcLoc;
                return( 1 );
                }
        else {
                if (Latitude)  *Latitude  = 0.0L;
                if (Longitude) *Longitude = 0.0L;
                if (Altitude)  *Altitude  = 0.0L;
                if (Source)    *Source    = SRCSTR_MIN;
                return( 0 );
                }
}





/************************* TIME HELPER FUNCTIONS *************************/

// Packed digit time is a double where two digits have been allocated to
// each of hours, minutes, and seconds (or year, month, day).  For
// example 112307 represents 23 minutes and 7 seconds past 11 o'clock.
// See pargps.h for more details.

#define SHIFTDIG4   10000
#define SHIFTDIG2     100

//------------------------------------------------------------------------------
// ROUTINE: ParGpsUnpack3
// PURPOSE: Split a packed digit time into its component digits.
//          See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsUnpack3( int Xxx, int *Top, int *Middle, int *Bottom ){

        int tt, mm, bb;

        // Unpack 3 pieces (ttmmbb) from one int

        tt = Xxx / SHIFTDIG4;
        mm = (Xxx-tt*SHIFTDIG4) / SHIFTDIG2;
        bb = (Xxx-tt*SHIFTDIG4-mm*SHIFTDIG2);


        // Save pieces

        *Top    = tt;
        *Middle = mm;
        *Bottom = bb;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsPack3
// PURPOSE: Create a packed digit time from its component digits.
//          See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsPack3( int Top, int Middle, int Bottom, int *Xxx  ){

        // Pack 3 pieces (ttmmbb) into one int

        *Xxx = (Top * SHIFTDIG4) + (Middle * SHIFTDIG2) + Bottom;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetPcTime
// PURPOSE: Return the current PC system time as YMD HMS and as seconds since
//          1970.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetPcTime( double *Time,
                                 int *Year, int *Month, int *Day,
                                 int *Hour, int *Minute, int *Second,
                                 long *Microsecond ) {

        time_t    ttNow;
        struct tm tmNow;
        double    dtime;
        long      ltime;

        // Return time as YMD HMS for UTC (not local)

        time( &ttNow );
        tmNow   = *gmtime( &ttNow );
        dtime   = (double)ttNow;
        ltime   = (long)ttNow;

        if (Time)         *Time        = dtime;
        if (Year)         *Year        = tmNow.tm_year + 1900;
        if (Month)        *Month       = tmNow.tm_mon + 1;
        if (Day)          *Day         = tmNow.tm_mday;
        if (Hour)         *Hour        = tmNow.tm_hour;
        if (Minute)       *Minute      = tmNow.tm_min;
        if (Second)       *Second      = tmNow.tm_sec;
        if (Microsecond)  *Microsecond = (long)((dtime - ltime) * USPERMS);

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSetPcTime
// PURPOSE: Set the current PC system time.
//
//          This function returns 1 for success, 0 for failure.
//          FromYMD is 1 when the time to set is taken from the Year, Month, etc
//          values and and 0 when it is taken from the Time in seconds since
//          1970.  The underlying Windows function uses YMD, while the underlying
//          Linux function uses Time.  But both work with either setting.
//
//          NOTE: You MUST have the proper permissions to successfully set
//                the PC time.  Typically this means being an Administrator
//                or Power User under Windows and being root or running
//                "set uid" root under Linux.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSetPcTime( double Time,
                                 int Year, int Month, int Day,
                                 int Hour, int Minute, int Second,
                                 long Microsecond, int FromYMD ) {



        return( ParGpsOsSetPcTime( Time, Year, Month, Day, Hour, Minute, Second,
                                   Microsecond, FromYMD )  );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsCalcTimeZoneCorr
// PURPOSE: Calculate the correction between the local time zone and UTC time.
//
//          This function does two conversions between time stored as a structure
//          and time as seconds since the epoch (1970-01-01).  They should cancel
//          out, but the first is done using GMT time and the second is done using
//          local time.  The difference gives us the amount of the local correction.
//          Subtract this value from a local time to get a GMT time.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsCalcTimeZoneCorr( long StartTime ) {

        time_t    TargetTime, NewTime;
        struct tm tm;
        int       ZoneCorrection;


        // Convert epoch seconds to time structure for gmt

        TargetTime = (time_t)StartTime;
        tm = *gmtime( &TargetTime );


        // Convert time structure for local to epoch seconds (force DST unknown)

        tm.tm_isdst = -1;
        NewTime = mktime( &tm );


        // Difference is local correction

        ZoneCorrection = NewTime - TargetTime;
        return( ZoneCorrection );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSecTimeSplit
// PURPOSE: Convert time represented as seconds since 1970 into YMD HMS.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSecTimeSplit( double Time,
                                    int *Year, int *Month, int *Day,
                                    int *Hour, int *Minute, int *Second,
                                    long *Microsecond ) {
        time_t    ttTime;
        struct tm tmTime;
        double    usec;


        // Split time into pieces (for UTC)

        ttTime = (time_t)Time;
        tmTime = *gmtime( &ttTime );
        usec   = Time - (double)ttTime + 0.0000005;

        // Save pieces

        if (Year)         *Year        = tmTime.tm_year + 1900;
        if (Month)        *Month       = tmTime.tm_mon + 1;
        if (Day)          *Day         = tmTime.tm_mday;
        if (Hour)         *Hour        = tmTime.tm_hour;
        if (Minute)       *Minute      = tmTime.tm_min;
        if (Second)       *Second      = tmTime.tm_sec;
        if (Microsecond)  *Microsecond = (long)(usec * USPERSEC);

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSecTimeSplit
// PURPOSE: Convert time represented as YMD HMS into seconds since 1970.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSecTimeCombine( int Year, int Month, int Day,
                                   int Hour, int Minute, int Second,
                                   long MicroSecond, double *Time ) {

        time_t    ttTime;
        struct tm tmTime;

        // Set default YMD values when 0's are passed in

        if (Year == 0)   Year  = 1970;
        if (Month == 0)  Month = 1;
        if (Day == 0)    Day   = 1;


        // Fill pieces

        tmTime.tm_year  = Year - 1900;
        tmTime.tm_mon   = Month - 1;
        tmTime.tm_mday  = Day;
        tmTime.tm_hour  = Hour;
        tmTime.tm_min   = Minute;
        tmTime.tm_sec   = Second;
        tmTime.tm_wday  = 0;
        tmTime.tm_yday  = 0;
        tmTime.tm_isdst = -1;


        // Combine time from GMT pieces to epoch seconds using local correction,
        // then use the zone offset to remove the local correction

        ttTime = mktime( &tmTime );
        if (ttTime == (time_t)-1)
                return( 0 );
        ttTime -= ParGpsCalcTimeZoneCorr( ttTime );
        if (Time)  *Time  = (double)ttTime + (double)MicroSecond / (double)USPERSEC;

        return( 1 );
}






/******************** PRIVATE LIBRARY HELPER FUNCTIONS ********************/

// Most users should NOT use these functions, but only the main ones
// above.  These are intended for use by the main library functions and
// programs like diag.c with special access.

//------------------------------------------------------------------------------
// ROUTINE: ParGpsIoctl
// PURPOSE: Request the PARGPS device driver perform a specialized function.
//          The IoCtlCode parameter selects which of the possible functions is
//          to be done.  These include functions like starting/stopping,
//          setting the serial port, reading the counter frequency,
//          reading the pps/serial data, etc.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsIoctl( DEVHANDLE     ParGpsHandle,
                             unsigned long IoCtlCode,
                             void         *pValueIn,
                             unsigned long InSize,
                             void         *pValueOut,
                             unsigned long OutSize,
                             unsigned long *ReturnSize
                           ) {

        int           Result;
        unsigned long BytesReturned;


        // Device driver must be opened first.

        if (ParGpsHandle == BAD_DEVHANDLE) {
                ParGpsLastDriverError = ERROR_SERVICE_DISABLED;
                return( 0 );
                }



        // Request specialized services from device driver.

        Result = ParGpsOsDriverIoctl(
                               ParGpsHandle,     // Handle to device
                               IoCtlCode,        // IO Control code
                               pValueIn,         // Input data to driver
                               InSize,           // Length of in buffer in bytes
                               pValueOut,        // Output data from driver
                               OutSize,          // Length of out buffer in bytes
                               &BytesReturned    // Bytes placed in output buffer
                              );

        if (ReturnSize)
                *ReturnSize = BytesReturned;



        // Failure output.

        if (!Result) {
                ParGpsLastDriverError = ParGpsOsGetLastError();
                return( 0 );
                }

        ParGpsLastDriverError = 0;
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsReset
// PURPOSE: Reset the PARGPS driver's count of interrupts and PPS events.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsReset( DEVHANDLE ParGpsHandle ) {

        int dummy, ret;

        ret = ParGpsIoctl( ParGpsHandle,
                           IOCTL_PARGPS_RESET,
                           &dummy,
                           sizeof(int),
                           NULL,
                           0,
                           NULL
                         );

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsGetInterruptCount
// PURPOSE: Read the PARGPS driver's count of interrupts that have occurred.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsGetInterruptCount( DEVHANDLE ParGpsHandle, int *count ) {

        int ret;

        ret = ParGpsIoctl( ParGpsHandle,
                           IOCTL_PARGPS_GET_INTR_COUNT,
                           NULL,
                           0,
                           count,
                           sizeof(int),
                           NULL
                         );

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSetInterruptMode
// PURPOSE: Set whether the PARGPS driver toggles between PPS and Dready
//          interrupts or not.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSetInterruptMode( DEVHANDLE ParGpsHandle, int Mode ) {

        int IntrMode, ret;

        // Change user define to kernel define

        if (Mode == INTERRUPT_ALTERNATING)
                IntrMode = PARGPS_INTTYPE_PPS;
        else
                IntrMode = PARGPS_INTTYPE_PLAIN;



        ret = ParGpsIoctl( ParGpsHandle,
                           IOCTL_PARGPS_SET_INTR_MODE,
                           &IntrMode,
                           sizeof(int),
                           NULL,
                           0,
                           NULL
                         );

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSerialSetup
// PURPOSE: Initialize the serial port to the proper baud rate for the PARGPS
//          NMEA strings.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSerialSetup( DEVHANDLE ParGpsHandle, int PortNumber ) {

        DEVHANDLE  SerialHandle;
        char       SerialPortName[MAXNAME];
        int        ret;


        // Translate port number to a port name

        if ( !ParGpsSerialPortName( PortNumber, SerialPortName ) )
                return( 0 );


        // Open serial port in user space

        SerialHandle = ParGpsOsSerialOpen( SerialPortName );
        if (SerialHandle == BAD_DEVHANDLE)
                return( 0 );


        // Set serial port baud rate, etc

        ret = ParGpsOsSerialInit( SerialHandle );
        if (!ret) {
                ParGpsOsSerialClose( SerialHandle );
                return( ret );
                }



#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )
        // Close serial port in user space, kernel will re-open
        ParGpsOsSerialClose( SerialHandle );
#elif defined( SROS_LINUX )
        // Don't do close so kernel serial access continues to work
        save_serial_handle = SerialHandle;
#endif

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsInit
// PURPOSE: Initialize the PARGPS driver.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsInit( DEVHANDLE ParGpsHandle,
                            int SerialPort,
                            int InterruptMode,
                            int GpsModel ) {

        int ret, ParGpsInitParms[3];


        // Error check input values

        if ( (SerialPort < 1) || (SerialPort > 9) )
                return( 0 );

        if ( (InterruptMode != INTERRUPT_PPS_ONLY)    &&
             (InterruptMode != INTERRUPT_ALTERNATING) )
                return( 0 );

        if ( (GpsModel <= GPSMODEL_NONE) ||
             (GpsModel >= GPSMODEL_MAX)  )
                return( 0 );


        // Set up GPS model related library parameters

        if ( GpsModel == GPSMODEL_GARMIN )
                SignalLengthPps = 0.020000L;  // 20 milliseconds

        else if ( GpsModel == GPSMODEL_TRIMBLE )
                SignalLengthPps = 0.000010L;  // 10 microseconds

        else if ( GpsModel == GPSMODEL_ONCORE )
                SignalLengthPps = 0.200000L;  // 200 milliseconds

        else if ( GpsModel == GPSMODEL_PCTIME )
                SignalLengthPps = SIGNAL_LENGTH_DREADY; // 0 diff from dready

        else
                SignalLengthPps = 0.000010L;  // 10 microseconds


        // Save GPS model type

        CurrentGpsModel = GpsModel;


        // Set up parameters and call kernel driver

        ParGpsInitParms[0] = SerialPort;
        ParGpsInitParms[1] = InterruptMode;
        ParGpsInitParms[2] = GpsModel;

        ret = ParGpsIoctl( ParGpsHandle,
                           IOCTL_PARGPS_INIT,
                           ParGpsInitParms,
                           sizeof(ParGpsInitParms),
                           ParGpsInitParms,
                           sizeof(ParGpsInitParms),
                           NULL
                         );

        return( ret );
}



/*************************** SERIAL PORT HELPER FUNCTIONS ********************/

// Functions for allowing easy OS-independent user space access to the
// PARGPS serial port NMEA strings.

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSerialPortName
// PURPOSE: Translate a serial port number into a serial port name.  Under
//          Windows, serial ports have names like COM1.  While under Linux,
//          they have names like /dev/ttyS0.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSerialPortName( int PortNumber, char *SerialPortName ) {

        // Error check

        if ( PortNumber < COMOFFSET  ||  !SerialPortName )
                return( 0 );

        // Translate port number to a port name using COMFMT and
        // COMOFFSET as defined in pargpskd.h.  Their values depend on
        // the OS.  For example COMFMT will be "COM%d" for Windows and
        // "/dev/ttyS%d" for Linux.  And COMOFFSET will be 0 and 1
        // respectively.

        sprintf( SerialPortName, COMFMT, (PortNumber - COMOFFSET) );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSerialOpen
// PURPOSE: Open the named serial port.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) ParGpsSerialOpen( char *SerialPortName ) {

        // Open serial port in user space and return its handle

        return( ParGpsOsSerialOpen( SerialPortName ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSerialClose
// PURPOSE: Close the specified serial port.
//------------------------------------------------------------------------------
FUNCTYPE( void ) ParGpsSerialClose( DEVHANDLE SerialHandle ) {

        // Close serial port in user space.

        ParGpsOsSerialClose( SerialHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSerialInit
// PURPOSE: Initialize the specified serial port.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSerialInit( DEVHANDLE SerialHandle ) {

        // Initialize serial port in user space.
        // Baud settings and such are appropriate for the PARGPS.

        return( ParGpsOsSerialInit( SerialHandle ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSerialRead
// PURPOSE: Read from the specified serial port.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsSerialRead( DEVHANDLE SerialHandle,
                                  void          *pValues,
                                  unsigned long  BytesToRead,
                                  unsigned long *pBytesRead ) {

        int ok;

        ok = ParGpsOsSerialRead( SerialHandle, pValues,
                                 BytesToRead, pBytesRead );

        if (!ok)
                ParGpsLastDriverError = ParGpsOsGetLastError();
        else
                ParGpsLastDriverError = 0;

        return( ok );
}




/******************* SR64BIT LARGE INTEGER HELPER FUNCTIONS ******************/

// Functions for subtracting two SR64BIT values and for adding to a
// SR64BIT value.  The computation varies depending on what type of
// data is stored in the SR64BIT variable.

//------------------------------------------------------------------------------
// ROUTINE: ParGpsLargeIntDiffFull
// PURPOSE: Subtract 2 64 bit numbers C = A - B.
//------------------------------------------------------------------------------
int ParGpsLargeIntDiffFull( SR64BIT A, SR64BIT B, SR64BIT *C, double *C2 ) {

        SR64BIT Result;
        double  Answer;


        // Now subtract standard large ints

        Result.QuadPart = (A.QuadPart - B.QuadPart);
        Answer          = (double)(Result.QuadPart);



        // Fill the users results variables

        if (C)   C->QuadPart = Result.QuadPart;
        if (C2) *C2          = Answer;

        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsLargeIntAdd
// PURPOSE: Add a long to a 64 bit numbers C = A + long.
//------------------------------------------------------------------------------
SR64BIT ParGpsLargeIntAdd( SR64BIT A, long More ) {

        SR64BIT C;

        C.QuadPart = A.QuadPart + More;

        return( C );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsLargeIntDiffFullType
// PURPOSE: Subtract 2 64 bit numbers C = A - B, but compensate for an older
//          style of representing a number in 64 bits.
//------------------------------------------------------------------------------
int ParGpsLargeIntDiffFullType( SR64BIT A, SR64BIT B, int Type,
                                SR64BIT *C, double *C2 ) {

        long    Seconds, Usec;
        SR64BIT Result;
        double  Answer;


        // If Linux timeval style large ints, convert into standard large ints

        if ( Type == PC_COUNTER_TYPE_HIGH32LOW32 ) {

                Seconds     = A.u.HighPart;
                Usec        = A.u.LowPart;
                A.QuadPart  = Seconds;
                A.QuadPart *= USPERSEC;
                A.QuadPart += Usec;

                Seconds     = B.u.HighPart;
                Usec        = B.u.LowPart;
                B.QuadPart  = Seconds;
                B.QuadPart *= USPERSEC;
                B.QuadPart += Usec;

                }

        // Now subtract standard large ints

        Result.QuadPart = (A.QuadPart - B.QuadPart);
        Answer          = (double)(Result.QuadPart);



        // Fill the users results variables

        if (C)   C->QuadPart = Result.QuadPart;
        if (C2) *C2          = Answer;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsLargeIntAddType
// PURPOSE: Add a long to a 64 bit numbers C = A + long, but compensate for an
//          older style of representing a number in 64 bits.
//------------------------------------------------------------------------------
SR64BIT ParGpsLargeIntAddType( SR64BIT A, long More, int Type ) {

        SR64BIT C;

        // Windows style large ints

        if (Type == PC_COUNTER_TYPE_INT64) {
                C.QuadPart = A.QuadPart + More;
                }

        else { // Type == PC_COUNTER_TYPE_HIGH32LOW32
                C.u.HighPart = A.u.HighPart;         // seconds part

                if ( More >= 0 ) {
                        while ( More > USPERSEC ) {
                                C.u.HighPart++;
                                More -= USPERSEC;
                                }
                        C.u.LowPart  = A.u.LowPart + More;   // usec part
                        while ( C.u.LowPart >= USPERSEC) {
                                C.u.HighPart++;
                                C.u.LowPart -= USPERSEC;
                                }
                        }
                else { // ( More < 0 )
                        while ( More < -USPERSEC ) {
                                C.u.HighPart--;
                                More += USPERSEC;
                                }
                        while ( (long)A.u.LowPart < -More ) {
                                C.u.HighPart--;
                                A.u.LowPart += USPERSEC;
                                }
                        C.u.LowPart  = A.u.LowPart + More;   // usec part
                        }
                }

        return( C );
}



/********* COLLECT ANALOG/PPS/SERIAL DATA INTO TIMESTAMPS FUNCTIONS *********/

// For each PpsEvent, three types of data may be available - analog,
// pps, and serial.  The following functions collect the
// data in what ever order it arrives into timestamp structures.
// See pargps.h for more details.

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsClear
// PURPOSE: Clear a timestamp structure in preparation for use.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsClear( TIMESTAMP *TS ) {

        SR64BIT Zero64;

        // Error check

        if (!TS)
                return( 0 );


        // Set helper variable

        Zero64.u.HighPart = 0L;
        Zero64.u.LowPart  = 0L;


        // Initialize the provided time stamp structure

        TS->Valid           = TS_VALID_NONE;
        TS->PpsEvent        = 0L;
        TS->Sample          = 0L;
        TS->CountObc        = 0L;
        TS->CountAtPps      = Zero64;
        TS->CountAtDready   = Zero64;
        TS->SecSince1970    = 0.0L;
        TS->NumSat          = 0;
        TS->HmsSource       = TIME_SOURCE_NONE;
        TS->YmdSource       = TIME_SOURCE_NONE;
        TS->PctimeAtPps     = Zero64;
        TS->PctimeAtDready  = Zero64;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsInit
// PURPOSE: Initialize the circular buffer for holding time stamps as they are
//          being built up.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsInit( void ) {

        return( ParGpsInitCircBuff() );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsProcessAnalog
// PURPOSE: Extract the gps mark index and on board count info from analog data
//          and store it in the appropriate time stamp structure.
//------------------------------------------------------------------------------

#define LAST_OBC_NP_INVALID 0x80000000L // largest neg long

FUNCTYPE( int ) ParGpsTsProcessAnalog( long *DataValues,
                                       long NumSamples,
                                       long NumChannels,
                                       long FirstSample,
                                       long MarkChannel,
                                       long ObcChannel,
                                       double ObcCountsPerSample,
                                       int TsValid,
                                       unsigned int *PpsEvent,
                                       int  *Nvalid
                                     ) {
        long         i;
        unsigned int EventNum;
        int          Obc;
        static long  LastObc   = INDEXCHANNEL_NONE;
        static long  LastObcNp = LAST_OBC_NP_INVALID;

        long *DataGpsPtr, *DataObcPtr;


        // Search through analog data checking the "mark" channel and
        // recording the mark information when that channel is not 0.
        //
        // MarkChannel  says which channel number has the mark info
        // FirstSample  says which number is the starting time point

        // The algorithm for processing with OBC is described here:
        // Look for a valid OBC (ie non 0) and then for a valid PPS
        // EventNum (Gps Mark).  In most cases, these occur on the same
        // sample point.  However, when the next sample comes very soon
        // after PPS, the OBC will catch it while the Mark will not be
        // seen until the following sample.
        //
        // While this is a complication, it does not result in an error
        // since the Mark information (the Dready count saved for that
        // EventNum) does include the time for the extra sample.  The
        // following code increments the OBC count, if needed, so it is
        // correct for the Marked sample. The OBC and its point index are
        // saved in static variables to handle the case where the OBC
        // comes on the last sample of one record and the Mark comes on
        // the first sample of the next.


        ParGpsLastErrorMessage[0] = '\0';


        // Initialize optional return parameters

        if ( PpsEvent )  *PpsEvent = 0;
        if ( Nvalid )    *Nvalid   = 0;


        // Error check incoming values

        if ( !DataValues )
                return( 0 );


        // Early exit if no points or no GPS mark information is included

        if ( NumSamples <= 0  ||  MarkChannel == INDEXCHANNEL_NONE )
                return( 0 );


        EventNum = 0;

        // Processing without on board counter

        if (ObcChannel == INDEXCHANNEL_NONE) {

                DataGpsPtr = DataValues + MarkChannel;

                for ( i = 0 ; i < NumSamples ; i++ ) {
                        if ( *DataGpsPtr !=  0L ) {
                                EventNum = (int)*DataGpsPtr;
                                ParGpsAnalogWriteCircBuff( EventNum,
                                                     FirstSample+i,
                                                     ObcChannel, TsValid );
                                }
                        DataGpsPtr += NumChannels;

                        }  // end for i < NumSamples

                } // end if ObcChannel = NONE

        // Processing with on board counter (OBC).

        else {

                DataGpsPtr = DataValues + MarkChannel;
                DataObcPtr = DataValues + ObcChannel;

                for ( i = 0 ; i < NumSamples ; i++ ) {

                        if ( *DataObcPtr != 0L ) {
                                LastObc   = *DataObcPtr;
                                LastObcNp = i;
                                }

                        if ( *DataGpsPtr != 0L ) {
                                EventNum = (int)*DataGpsPtr;

                                if (LastObcNp == LAST_OBC_NP_INVALID)
                                        Obc  = INDEXCHANNEL_NONE;
                                else {
                                        Obc = (int)(LastObc +
                                                    (i - LastObcNp) *
                                                    ObcCountsPerSample);

                                        LastObcNp = LAST_OBC_NP_INVALID;
                                        }

                                ParGpsAnalogWriteCircBuff( EventNum,
                                                     FirstSample+i,
                                                     Obc, TsValid );
                                }
                        DataGpsPtr += NumChannels;
                        DataObcPtr += NumChannels;

                        } // end for i < NumSamples


                // We have finished with all samples, but we need to check for
                // the case where the OBC has been found, but the GPS mark has
                // not.  ie OBC and GPS are split across two buffers of data.
                // Then adjust the OBC point so it is relative to the next buffer,
                // not the one we just finished processing.

                if ( LastObcNp != LAST_OBC_NP_INVALID )
                        LastObcNp -= NumSamples;


                } // end else ObcChannel


        // Fill optional return parameters

        if ( PpsEvent )  *PpsEvent = EventNum;
        if ( Nvalid )    *Nvalid   = CbNumValid; // This is a global variable

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsProcessPps
// PURPOSE: Extract the PPS and Dready count and time info from the PPS data
//          structure and store it in the appropriate time stamp structure.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsProcessPps( PPSTDATA *PpsValues, long NumPps,
                                    double CountsPerSecond,
                                    int GpsModel, int TsValid,
                                    unsigned int *PpsEvent, int *Nvalid ) {
        long         i;
        int          ValidMask;
        unsigned int EventNum;

        // Save the GPS PPS information in a timestamp structure.  The
        // values saved are the PPS event number (MarkNum), the number of
        // 64 bit PC counter counts at a PPS (CountAtPps) and the next data
        // ready (CountAtDready), and the 64 bit PC system time counter value
        // at a PPS (PctimeAtPps) and the next data ready (PctimeAtDready).


        ParGpsLastErrorMessage[0] = '\0';

        // Initialize optional return parameters

        if ( PpsEvent )  *PpsEvent = 0;
        if ( Nvalid )    *Nvalid   = 0;


        // Error check

        if ( !PpsValues  ||  NumPps <= 0 )
                return( 0 );



        // Loop through recording the PARGPS PPS data.

        ValidMask = VALID_PPS | VALID_DREADY;
        EventNum  = 0;

        for ( i = 0 ; i < NumPps ; i++ ) {

                if ( (PpsValues[i].ValidFields & ValidMask) == ValidMask) {
                        ParGpsPpsWriteCircBuff( PpsValues[i].PpsEventNum,
                                                PpsValues[i].CountAtPps,
                                                PpsValues[i].CountAtDready,
                                                PpsValues[i].PctimeAtPps,
                                                PpsValues[i].PctimeAtDready,
                                                CountsPerSecond,
                                                GpsModel, TsValid
                                               );
                        EventNum = PpsValues[i].PpsEventNum;
                        }
                else {
                        sprintf( ParGpsLastErrorMessage,
                                 "ERROR: Pps data for event %d is invalid\n",
                                 PpsValues[i].PpsEventNum );
                        }
                }

        // Fill optional return parameters

        if ( PpsEvent )  *PpsEvent = EventNum;
        if ( Nvalid )    *Nvalid   = CbNumValid; // This is a global variable

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsProcessSerial
// PURPOSE: Extract the serial NMEA info such as date, time and number of
//          satellites from the serial data structure and store it in the
//          appropriate time stamp structure.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsProcessSerial( SERIALDATA *SerialValues,
                                       long NumSerial,
                                       int SerialDelay,
                                       int TsValid,
                                       unsigned int *PrevEventNum,
                                       double *PrevSecSince1970,
                                       unsigned int *PpsEvent,
                                       int *Nvalid ) {
        unsigned int EventNum;
        int          Yr, Mo, Dy, Hh, Mm, Ss, Nsat, Ok;
        int          YmdSrc, HmsSrc, SatSrc, YmdSource, HmsSource;
        int          YmdValid, HmsValid, SatValid;
        long         i, Us;

        ParGpsLastErrorMessage[0] = '\0';

        // Initialize optional return parameters

        if ( PpsEvent )  *PpsEvent = 0;
        if ( Nvalid )    *Nvalid   = 0;


        // Error check

        if ( !SerialValues  ||  NumSerial <= 0 )
                return( 0 );

        EventNum = 0;
        Nsat     = 0;

        // Loop for each set of serial data.  First extract GPS date and
        // time info from the NMEA strings.  Then adjust it if needed
        // (ie no date or time from NMEA so base on previous info).
        // Then save in serial buffer.

        for ( i = 0 ; i < NumSerial ; i++ ) {

                // Get date, time, and num sats from NMEA strings

                Ok = ParGpsRequestNmeaInfo2( SerialValues[i].NmeaMsg,
                                             SerialValues[i].NmeaCount,
                                             &Yr, &Mo, &Dy, &YmdSrc, &YmdValid,
                                             &Hh, &Mm, &Ss, &Us, &HmsSrc, &HmsValid,
                                             NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL,
                                             &Nsat, &SatSrc, &SatValid );

                if (!Ok)          // Couldn't get info from current set of
                        continue; // serial data, so go on to next



                // Adjust info using previous values if needed

                EventNum = SerialValues[i].PpsEventNum + SerialDelay;

                Ok = ParGpsAdjustNmeaInfo( Yr, Mo, Dy, &YmdSrc,
                                           Hh, Mm, Ss, Us, &HmsSrc,
                                           EventNum, SerialDelay,
                                           PrevEventNum,
                                           PrevSecSince1970 );

                if (Ok) {

                        // Convert GPS specific Src strings (see SRCSTR_
                        // defines in pargps.h) to more generic dat2asc
                        // Source strings (see TIME_SOURCE_ defines).
                        // Save current values to circular buffer using
                        // the "prev" variables since they were updated
                        // in the adjust function above with the current
                        // values in preparation for the next pass.

                        ParGpsSrcToSource( YmdSrc, HmsSrc, YmdValid, HmsValid,
                                           &YmdSource, &HmsSource );
                        ParGpsSerialWriteCircBuff( *PrevEventNum, *PrevSecSince1970,
                                                   YmdSource, HmsSource,
                                                   Nsat, TsValid, 0 );
                        }


                // No valid NMEA time ever, so erase this entire buffer entry

                else {
                        ParGpsSerialWriteCircBuff( EventNum, (*PrevSecSince1970+1),
                                             TIME_SOURCE_NONE, TIME_SOURCE_NONE,
                                             Nsat, TsValid, 1 );
                        }


                } // end for i < NumSerial


        // Fill optional return parameters

        if ( PpsEvent )  *PpsEvent = EventNum;
        if ( Nvalid )    *Nvalid   = CbNumValid; // This is a global variable

        return( 1 );
}


/* GET VALID FROM LIBRARY AND READ/WRITE TO FILE TIMESTAMP FUNCTIONS:
 *
 * Get valid timestamps from the library's circular buffer.  Read
 * and write them from/to an open ascii file.
 *
 */

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsReadValid
// PURPOSE: Retrieve a valid time stamp structure from the circular buffer.
//          A timestamp typically becomes valid when it has all three types of
//          info: analog, pps, and serial.  Although when using PCTIME, the
//          serial info is not required.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsReadValid( TIMESTAMP *TS, int TsValid ) {

        // Error check input
        if ( !TS )
                return( 0 );


        if ( !ParGpsReadCircBuff( TS, TsValid ) )
                return( 0 );               // none ready (valid)

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsReadNext
// PURPOSE: Retrieve the next time stamp structure from the circular buffer even
//          if it is missing one or more pieces of data.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsReadNext( TIMESTAMP *TS ) {

        int TsValid;

        // Error check input
        if ( !TS )
                return( 0 );


        // See if there is another entry and if so, then read it

        TsValid = ParGpsPeekCircBuffState( );

        if ( TsValid && ParGpsReadCircBuff( TS, TsValid ) )
                return( TsValid );
        else
                return( 0 );  // none left
}



// This is used by both printf and scanf, so don't give any precisions
// in the format for reading/writing time stamp from/to an open ascii file

#define TSTITLE "Valid  PpsEvent      Sample    CountObc         CountAtPps      CountAtDready   SecSince1970      Nsat Ysrc Hsrc      PctimeAtPps     PctimeAtDready\n"
#define TSFMT   "0x%1X  %10d  %10ld  %10ld  %08lX %08lX  %08lX %08lX  %18lf  %2d  %2d  %2d    %08lX %08lX  %08lX %08lX\n"

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsFileRead
// PURPOSE: Read timestamp info from the specified file written with the
//          ParGpsTsFileWrite function.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsFileRead(  FILE *tsfile, TIMESTAMP *TS, int skiptitle ){

        int c, nval;

        if ( !tsfile || !TS )
                return( 0 );

        if ( skiptitle ) {
                c = fgetc( tsfile );
                while ( c != EOF && c != '\n' )
                        c = fgetc( tsfile );
                }


        nval = fscanf( tsfile, TSFMT,
                 &TS->Valid,
                 &TS->PpsEvent,
                 &TS->Sample,
                 &TS->CountObc,
                 &TS->CountAtPps.u.HighPart,
                 &TS->CountAtPps.u.LowPart,
                 &TS->CountAtDready.u.HighPart,
                 &TS->CountAtDready.u.LowPart,
                 &TS->SecSince1970,
                 &TS->NumSat,
                 &TS->YmdSource,
                 &TS->HmsSource,
                 &TS->PctimeAtPps.u.HighPart,
                 &TS->PctimeAtPps.u.LowPart,
                 &TS->PctimeAtDready.u.HighPart,
                 &TS->PctimeAtDready.u.LowPart
                 );

        if (nval == EOF)
                nval = 0;

        return( nval );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsFileWrite
// PURPOSE: Write timestamp info into the specified file so it can be read later
//          with the ParGpsTsFileRead function.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsFileWrite( FILE *tsfile, TIMESTAMP *TS, int writetitle ) {

        if ( !tsfile || !TS )
                return( 0 );

        if ( writetitle )
                fprintf( tsfile, TSTITLE );

        fprintf( tsfile, TSFMT,
                 TS->Valid,
                 TS->PpsEvent,
                 TS->Sample,
                 TS->CountObc,
                 TS->CountAtPps.u.HighPart,
                 TS->CountAtPps.u.LowPart,
                 TS->CountAtDready.u.HighPart,
                 TS->CountAtDready.u.LowPart,
                 TS->SecSince1970,
                 TS->NumSat,
                 TS->YmdSource,
                 TS->HmsSource,
                 TS->PctimeAtPps.u.HighPart,
                 TS->PctimeAtPps.u.LowPart,
                 TS->PctimeAtDready.u.HighPart,
                 TS->PctimeAtDready.u.LowPart
               );

        return( 1 );
}




// Check two timestamps to see if their CountAt values are normal or if
// one is an outlier.  An outlier is defined as one which is more than LimitXxx
// counts away from the StandardXxx.
//
// The number of 64 bit PC counts between successive PPS signals
// and between successive dready mark points should remain fairly
// constant.  We use this fact to check for and discard outlier
// points.  Outlier points typically result when some other
// program does lots of higher priority interrupts and so delays
// the servicing of the PARxCH/PARGPS interrupt routine.
//
// The number of sample points between successive dready marks is
// expected to be NSps.  But periodically it will be one point
// more or less because the sampling rate with respect to true
// time is not exactly an integer value (ie it is 100.001 not 100).

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsIsOutlier
// PURPOSE: Check two timestamps to see if their CountAt values are normal or if
//          one is an outlier.  An outlier is defined as one which is more than
//          LimitXxx counts away from the StandardXxx.  This function is not
//          used and has NOT been tested.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsIsOutlier( TIMESTAMP *TS0, TIMESTAMP *TS1,
                                   long Nsps, int PcCounterType,
                                   double StandardCountsPps,    double LimitPps,
                                   double StandardCountsDready, double LimitDready,
                                   double *ActualCountsPps,     double *DiffPps,
                                   double *ActualCountsDready,  double *DiffDready ) {

        int    IsOutlier, Nsec;
        double Pcounts, Dcounts, Pdiff, Ddiff, Correct;
        long   CurrNsamples, StdNsamples;


        // Error check to prevent divide by zero

        if (TS0->PpsEvent == TS1->PpsEvent)
                return( -1 );              // This will show as true, meaning outlier



        // Compute PPS counts and difference from standard

        Nsec = TS1->PpsEvent - TS0->PpsEvent;
        ParGpsLargeIntDiffFull( TS1->CountAtPps, TS0->CountAtPps,
                                NULL, &Pcounts );
        Pdiff = (Pcounts / Nsec) - StandardCountsPps;



        // Compute Dready counts and difference from standard,
        // correcting for number of samples between the two Dreadys

        ParGpsLargeIntDiffFull( TS1->CountAtDready, TS0->CountAtDready,
                                NULL, &Dcounts );

        CurrNsamples = TS1->Sample - TS0->Sample;
        StdNsamples  = Nsps * Nsec;
        if ( CurrNsamples == StdNsamples )
                Correct = 0.0L;
        else
                Correct = (double)(StdNsamples - CurrNsamples) *
                          StandardCountsDready / (double)StdNsamples;
        Dcounts += Correct;

        Ddiff = Dcounts - StandardCountsDready;


        // Check to see if differences exceed +/- limits

        if ( Pdiff > 0 && Pdiff > LimitPps )
                IsOutlier = 1;
        else if ( Pdiff < 0 && Pdiff < -LimitPps )
                IsOutlier = 1;
        else if ( Ddiff > 0.0L && Ddiff > LimitDready )
                IsOutlier = 1;
        else if ( Ddiff < 0.0L && Ddiff < -LimitDready )
                IsOutlier = 1;
        else
                IsOutlier = 0;


        // Fill optional return variables

        if (ActualCountsPps)
                *ActualCountsPps = Pcounts;
        if (DiffPps)
                *DiffPps = Pdiff;
        if (ActualCountsDready)
                *ActualCountsDready = Dcounts;
        if (DiffDready)
                *DiffDready = Ddiff;


    return( IsOutlier );
}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsOutlierAdjust
// PURPOSE: Correct the CountAt values of timestamp TSx so it matches the value
//          that would be expected if it were in line with TS0 and TS1.  This
//          function is not used and has NOT been tested.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsOutlierAdjust( TIMESTAMP *TS0, TIMESTAMP *TS1, TIMESTAMP *TSx,
                                       long Nsps, long PcCounterType, long PcCounter2Type,
                                       double StandardCountsPps, double StandardCountsDready,
                                       double StandardCountsPerSecond ) {

        int    ValidFlag, Nsec;
        long   NormalCounts, NormalCounts2, CurrNsamples, StdNsamples;
        double Correct, Ncounts, Ncounts2;


        // Set validity flag

        ValidFlag = 0x0;
        if ( TS1->Valid == TS_VALID_ALL )
                ValidFlag |= 0x1;
        if ( TS0->Valid == TS_VALID_ALL )
                ValidFlag |= 0x2;

        if (ValidFlag == 0x0) {
                sprintf( ParGpsLastErrorMessage,
                         "No timestamps valid, can't compute outlier replacement\n" );
                return( 0 );
                }


        Nsec = TSx->PpsEvent - TS1->PpsEvent;


        // Compute new Pps counts

        if (ValidFlag & 0x3) { // both time stamps valid

                ParGpsLargeIntDiffFull( TS1->CountAtPps, TS0->CountAtPps,
                                        NULL, &Ncounts );

                ParGpsLargeIntDiffFull( TS1->PctimeAtPps, TS0->PctimeAtPps,
                                        NULL, &Ncounts2 );
                NormalCounts  = (long)Ncounts;
                NormalCounts2 = (long)Ncounts2;
                }

        else { // don't have both time stamps valid
                NormalCounts  = (long)StandardCountsPps;
                NormalCounts2 = (long)StandardCountsPerSecond;
                }


        NormalCounts    *= Nsec;
        NormalCounts2   *= Nsec;
        TSx->CountAtPps  = ParGpsLargeIntAdd( TS1->CountAtPps, NormalCounts );
        TSx->PctimeAtPps = ParGpsLargeIntAdd( TS1->PctimeAtPps, NormalCounts2 );



        // Compute new Dready counts

        CurrNsamples = TSx->Sample - TS1->Sample;
        StdNsamples  = Nsps * Nsec;

        if ( CurrNsamples == StdNsamples )
                Correct = 0.0L;
        else
                Correct = (double)(StdNsamples - CurrNsamples) *
                          StandardCountsDready / (double)StdNsamples;

        if (ValidFlag & 0x3) { // both time stamps valid

                ParGpsLargeIntDiffFull( TS1->CountAtDready,
                                        TS0->CountAtDready,
                                        NULL, &Ncounts );
                ParGpsLargeIntDiffFull( TS1->PctimeAtDready,
                                        TS0->PctimeAtDready,
                                        NULL, &Ncounts2 );
                NormalCounts  = (long)Ncounts;
                NormalCounts2 = (long)Ncounts2;
                }

        else { // don't have both time stamps valid
                NormalCounts  = (long)StandardCountsDready;
                NormalCounts2 = (long)StandardCountsPerSecond;
                }


        NormalCounts  = (NormalCounts  * Nsec) - (long)Correct;
        NormalCounts2 = (NormalCounts2 * Nsec) - (long)Correct;
        TSx->CountAtDready  = ParGpsLargeIntAdd( TS1->CountAtDready,
                                                     NormalCounts );
        TSx->PctimeAtDready = ParGpsLargeIntAdd( TS1->PctimeAtDready,
                                                    NormalCounts2 );

    return( 1 );
}



/***************** COMPUTE TIME USING TIMESTAMP FUNCTIONS *****************/

/*
 * Given 2 timestamps and some additional data, interpolate to determine
 * the time in seconds since 1970 for the specified data sample.
 *
 * The SPS method just multiplies the number of samples time the sampling
 * rate.  The PPS and OBC methods are somewhat more complex.
 *
 * In those cases, we use one straight line equation to convert from DAQ sample
 * number to 64 bit PC counts:
 *
 *            c4 - c2
 *      cx = --------- * (nx - n2) + c2            (Eq 2)
 *            n4 - n2
 *
 * We use a slightly different version of that equation to convert from
 * DAQ sample number to OBC counts if they are available:
 *
 *      cx = m * (nx - n2) + c2                    (Eq 5)
 *
 * The second step in either case is to use the straight line equation to
 * convert from number of counts (either type as appropriate) to true time:
 *
 *            t3 - t1
 *      tx = --------- * (cx - c1) + t1            (Eq 1)
 *            c3 - c1
 *
 */

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsComputeTimeSps
// PURPOSE: Compute the time at Sample using the provided timestamps and
//          sampling rate.  See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsComputeTimeSps( TIMESTAMP *TS0, TIMESTAMP *TS1,
                                        double Sps, int TsValid,
                                        long Sample, double *Time ) {

        // Error check for
        //   1. Time stamps exist and are valid
        //   2. No place to put answer
        //   3. Bad sampling rate
        //   4. Sample index ok

        if ( !TS0 || (TS0->Valid&TsValid) != TsValid ||
             TS0->YmdSource == TIME_SOURCE_NONE      ||
             TS0->HmsSource == TIME_SOURCE_NONE      ||
             !Time                                   ||
             Sps < 0.000001                          ||
             Sample < 0                          ) {
                sprintf( ParGpsLastErrorMessage,
                         "ParGpsTsComputeTimeSps Invalid parameter\n" );
                return( 0 );
                }

        ParGpsLastErrorMessage[0] = '\0';


        *Time = TS0->SecSince1970 + (double)(Sample - TS0->Sample) / Sps;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsComputeTimePps
// PURPOSE: Compute the time at Sample using the provided timestamps and
//          their PPS/Dready count information.  See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsComputeTimePps( TIMESTAMP *TS0, TIMESTAMP *TS1,
                                        double CountsBetweenPps,
                                        int TsValid, long Sample,
                                        double *Time ) {

        double  c1, c2, c3, c4, cx, cxc, t1, t3, tx;


        // Compute Time at Sample using PC counter from PARGPS PPS area

        // Error check our incoming data

        if ( !TS0 || (TS0->Valid&TsValid) != TsValid    ||
             !TS1 || (TS1->Valid&TsValid) != TsValid    ||
             TS0->YmdSource == TIME_SOURCE_NONE         ||
             TS0->HmsSource == TIME_SOURCE_NONE         ||
             TS1->YmdSource == TIME_SOURCE_NONE         ||
             TS1->HmsSource == TIME_SOURCE_NONE         ||
             CountsBetweenPps < 0.00001                 ||
             Sample < 0                                 ||
             !Time                                      ) {
                sprintf( ParGpsLastErrorMessage,
                         "ParGpsTsComputeTimePps Invalid parameter\n" );
                return( 0 );
                }

        ParGpsLastErrorMessage[0] = '\0';



        // Step 0: Perform a translation.
        //
        // This is done to simplify further computations.  Subtract
        // TS0->CountAtPps (ie c1) from all count values and
        // TS0->PpsEvent (ie t1) from all t values.  When translating t
        // values, we could subtract SecSince1970 values.  At the end,
        // we will reverse the -t1 translation by adding SecSince1970
        // back to the final tx result.

        c1 = 0;
        ParGpsLargeIntDiffFull( TS0->CountAtDready, TS0->CountAtPps, NULL, &c2 );
        ParGpsLargeIntDiffFull( TS1->CountAtPps,    TS0->CountAtPps, NULL, &c3 );
        ParGpsLargeIntDiffFull( TS1->CountAtDready, TS0->CountAtPps, NULL, &c4 );


        t1 = 0;
        t3 = TS1->SecSince1970 - TS0->SecSince1970;



        // Step 1: Solve Eq 2 for cx, a 64 bit PC
        // count for the DAQ sample of interest

        if ( !ParGpsEquation2pt( TS0->Sample, c2,     // (n2,c2)
                                 TS1->Sample, c4,     // (n4,c4)
                                      Sample, &cx ) ) // (nx,cx)
                return( 0 );


        // Correct for difference in length of PPS and DREADY signals

        cxc = cx - (SIGNAL_LENGTH_CORRECTION * CountsBetweenPps);



        // Step 2: Solve Eq 1 for tx, the true
        // time for the DAQ sample of interest

        if ( !ParGpsEquation2pt( c1, t1,   c3, t3,   cxc, &tx ) )
                return( 0 );



        // Finish calculation

        *Time = tx + TS0->SecSince1970;  // Add back in time at t1


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsComputeTimePctime
// PURPOSE: Compute the time at Sample using the provided timestamps and
//          their PPS/Dready PC system time information.  See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsComputeTimePctime( TIMESTAMP *TS0, TIMESTAMP *TS1,
                                           double CountsPerSecond,
                                           int TsValid, long Sample,
                                           double *Time ) {

        double n2, n4, nx, c2, c4, cx, t2, t4, tx;
        double TS0_SecSince1970, TS1_SecSince1970;

        // Compute Time at Sample using PC system time counter


        // Error check our incoming data

        if ( !TS0 || (TS0->Valid&TsValid) != TsValid    ||
             !TS1 || (TS1->Valid&TsValid) != TsValid    ||
             TS0->YmdSource == TIME_SOURCE_NONE         ||
             TS0->HmsSource == TIME_SOURCE_NONE         ||
             TS1->YmdSource == TIME_SOURCE_NONE         ||
             TS1->HmsSource == TIME_SOURCE_NONE         ||
             Sample < 0                                 ||
             CountsPerSecond  < 0.0L                    ||
             !Time                                      ) {
                sprintf( ParGpsLastErrorMessage,
                         "ParGpsTsComputeTimePctime Invalid parameter\n" );
                return( 0 );
                }

        ParGpsLastErrorMessage[0] = '\0';



        // Step 0: Perform a translation.
        //
        // This is done to simplify further computations.  Subtract
        // TS0->CountAtDready (ie c2) from all count values and
        // TS0_SecSince1970 (ie t2) from all t values.  At the
        // end, we will reverse the -t2 translation by adding
        // SecSince1970 back to the final tx result.

        n2 = 0;
        n4 = TS1->Sample - TS0->Sample;
        nx = Sample      - TS0->Sample;


        c2 = 0;
        ParGpsLargeIntDiffFull( TS1->CountAtDready, TS0->CountAtDready,
                                NULL, &c4 );


        TS0_SecSince1970 = (double)(TS0->PctimeAtDready.QuadPart / CountsPerSecond );
        TS1_SecSince1970 = (double)(TS1->PctimeAtDready.QuadPart / CountsPerSecond );
        t2 = 0;
        t4 = TS1_SecSince1970 - TS0_SecSince1970;


        // Step 1: Solve Eq 2 for cx, a 64 bit PC
        // count for the DAQ sample of interest

        if ( !ParGpsEquation2pt( n2, c2,   n4, c4,   nx, &cx ) )
                {
                return( 0 );
                }



        // Step 2: Solve Eq 1 for tx, the true
        // time for the DAQ sample of interest

        if ( !ParGpsEquation2pt( c2, t2,   c4, t4,   cx, &tx ) ) {
                return( 0 );
                }



        // Finish calculation

        *Time = tx + TS0_SecSince1970;  // Add back in time at t1

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsTsComputeTimeObc
// PURPOSE: Compute the time at Sample using the provided timestamps and their
//          PAR8CH On Board Counter (OBC) information.  See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParGpsTsComputeTimeObc( TIMESTAMP *TS0, TIMESTAMP *TS1,
                                        double ObcCountsPerSample,
                                        int TsValid, long Sample,
                                        double *Time ) {

        double  c1, c2, c3, c4, cx, m, t1, t3, tx;

        // Compute time at specified sample point using the
        // PAR8CH 800 ns On Board Counter information from two
        // time stamp structures.

        // Make sure our raw data is ok

        if ( !TS0 || (TS0->Valid&TsValid) != TsValid    ||
             !TS1 || (TS1->Valid&TsValid) != TsValid    ||
             TS0->YmdSource == TIME_SOURCE_NONE         ||
             TS0->HmsSource == TIME_SOURCE_NONE         ||
             TS1->YmdSource == TIME_SOURCE_NONE         ||
             TS1->HmsSource == TIME_SOURCE_NONE         ||
             ObcCountsPerSample < 0.00001               ||
             Sample < 0                                 ||
             !Time                               ) {
                sprintf( ParGpsLastErrorMessage,
                         "ParGpsTsComputeTimeObc Invalid parameter\n" );
                return( 0 );
                }

        ParGpsLastErrorMessage[0] = '\0';


        // Step 0: Initialize some variables
        //
        // We translate the counts variable so C2 is the C origin and
        // the time variable so T1 is the T origin.  For T we work with
        // SecSince1970.  We will reverse the translation to T at the end
        // by adding back the SecSince1970 at t1 to the final result tx.

        m  = ObcCountsPerSample;
        c2 = 0.0L;
        t1 = 0.0L;
        t3 = TS1->SecSince1970 - TS0->SecSince1970;




        // Step 1a: Compute the OBC count at D2 from Eq 5

        if ( !ParGpsEquationSlope( m,
                                   TS0->Sample, c2,     // (n2,c2)
                                   TS1->Sample, &c4 ) ) // (n4,c4)
                return( 0 );


        // Step 1b: Compute the OBC count at Sx from Eq 5

        if ( !ParGpsEquationSlope( m,
                                   TS0->Sample, c2,     // (n2,c2)
                                   Sample,      &cx ) ) // (nx,cx)
                return( 0 );


        // Step 1c: Compute the OBC counts at P1 and P2
        // from the known OBC counts relative to D1 and D1

        c1 = c2 - TS0->CountObc;                        // c2 - o21
        c3 = c4 - TS1->CountObc;                        // c4 - o43


        // Step 2: Solve Eq 1 for tx, the true
        // time for the DAQ sample of interest

        if ( !ParGpsEquation2pt( c1, t1,   c3, t3,   cx, &tx ) )
                return( 0 );



        // Finish calculation

        *Time = tx + TS0->SecSince1970;  // Add back in time at t1


        return( 1 );
}




/***************** CIRCULAR TIMESTAMP BUFFER HELPER FUNCTIONS ***************/

// These functions are helpers and are NOT available for users to call directly

/*
 * Set up a circular buffer that has 1 read pointer and 3 write
 * pointers.  There is one write pointer for each type of data: analog,
 * GPS PPS, and GPS serial.  The read pointer is used to read "valid"
 * buffers; that is, ones which have had all of their three types of
 * data filled in.
 *
 */

//------------------------------------------------------------------------------
// ROUTINE: ParGpsInitCircBuff
// PURPOSE: Initialize a circular buffer for holding timestamps while they are
//          built up as information arrives.
//------------------------------------------------------------------------------

static int rdptr  = 0;
static int rdskip = 0;

SRLOCAL int ParGpsInitCircBuff( void ) {

        int i;

        // Initialize time stamp buffer

        for ( i = 0 ; i < MAXCIRC ; i++ ) {
                ParGpsTsClear( &CircBuff[i] );
                }

        CbNumValid = 0;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSkipCircBuff
// PURPOSE: In special cases, we may need to skip ahead one entry
//          to account for uneven startup conditions (eg where PPS data
//          for a given analog mark has been read in earlier file that
//          we don't have access to).
//------------------------------------------------------------------------------
SRLOCAL int ParGpsSkipCircBuff( void ) {


        ParGpsTsClear( &CircBuff[rdptr] );
        ParGpsIncCbPtr( &rdptr );
        rdskip = 1;

        return( rdptr );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsPeekCircBuffState
// PURPOSE: Check the current state of the next timestamp to be read from the
//          circular buffer.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsPeekCircBuffState( void ) {

        // Check valid state

        return( CircBuff[rdptr].Valid );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsReadCircBuff
// PURPOSE: Read a valid timestamp from the circular buffer.  Once read, clear
//          that entry for future use and move the read pointer to the next
//          entry in the circular buffer.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsReadCircBuff( TIMESTAMP *Buffer, int TsValid ) {

        // Check for empty or not ready

        if ( (CircBuff[rdptr].Valid & TsValid) != TsValid ) {
                return( 0 );
                }



        // Read data

        *Buffer = CircBuff[rdptr];



        // Clear valid flag and update read pointer

        ParGpsTsClear( &CircBuff[rdptr] );
        CbNumValid--;

        ParGpsIncCbPtr( &rdptr );


        // Indicate we have successfully gotten past startup skips

        rdskip = 0;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsCheckValid
// PURPOSE: Check if the timestamp being updated with new info has now become
//          valid.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsCheckValid( int wrptr, int TsValid ) {

        if ( (CircBuff[wrptr].Valid & TsValid) == TsValid ) {
                CbNumValid++;
                return( CbNumValid );
                }
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsAnalogWriteCircBuff
// PURPOSE: Write analog info into the next timestamp waiting in the circular
//          buffer for analog info.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsAnalogWriteCircBuff( unsigned int PpsEvent, long Sample,
                                       int Obc, int TsValid ) {

        static int wrptr = 0;


        // Ignore buffer entries already skipped

        ParGpsUpdateCbPtrForSkip( &wrptr );



        // Check for overflow

        if ( (CircBuff[wrptr].Valid & TS_VALID_ANALOG & TsValid) != TS_VALID_NONE) {
                sprintf( ParGpsLastErrorMessage,
                         "ERROR: CircBuff overflow on Analog write\n");
                return( 0 );
                }


        // Check or write PpsEvent

        if ( !ParGpsCheckEventNum( PpsEvent, TsValid, &wrptr, "Analog" ) )
                return( 0 );


        // Write analog data to buffer

        CircBuff[wrptr].Valid   |= (TS_VALID_ANALOG | TS_VALID_OBC);
        CircBuff[wrptr].Sample   = Sample;
        CircBuff[wrptr].CountObc = Obc;


        // See if CB element is now completely valid

        ParGpsCheckValid( wrptr, TsValid );


        // Update analog write pointer

        ParGpsIncCbPtr( &wrptr );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsPpsWriteCircBuff
// PURPOSE: Write pps/dready count info into the next timestamp waiting in the
//          circular buffer for pps info.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsPpsWriteCircBuff( unsigned int PpsEvent,
                                    SR64BIT CountAtPps,  SR64BIT CountAtDready,
                                    SR64BIT PctimeAtPps, SR64BIT PctimeAtDready,
                                    double CountsPerSecond,
                                    int GpsModel, int TsValid ) {

        static int wrptr = 0;


        // Ignore buffer entries already skipped

        ParGpsUpdateCbPtrForSkip( &wrptr );


        // Check for overflow

        if ( (CircBuff[wrptr].Valid & TS_VALID_PPS & TsValid) != TS_VALID_NONE) {
                sprintf( ParGpsLastErrorMessage,
                         "ERROR: CircBuff overflow on pps write\n");
                return( 0 );
                }


        // Check or write PpsEvent

        if ( !ParGpsCheckEventNum( PpsEvent, TsValid, &wrptr, "Pps" ) )
                return( 0 );



        // Write Pps data to buffer

        CircBuff[wrptr].Valid         |= TS_VALID_PPS;
        CircBuff[wrptr].CountAtPps     = CountAtPps;
        CircBuff[wrptr].CountAtDready  = CountAtDready;
        CircBuff[wrptr].PctimeAtPps    = PctimeAtPps;
        CircBuff[wrptr].PctimeAtDready = PctimeAtDready;


        // SecSince1970 typically comes from the serial NMEA info, but
        // if PC time was requested, and there is no serial info yet,
        // we'll use the PC time instead

        if ( ( GpsModel == GPSMODEL_PCTIME )                   &&
             ( CircBuff[wrptr].YmdSource == TIME_SOURCE_NONE ) &&
             ( CircBuff[wrptr].HmsSource == TIME_SOURCE_NONE ) ) {

                CircBuff[wrptr].YmdSource    = TIME_SOURCE_PC;
                CircBuff[wrptr].HmsSource    = TIME_SOURCE_PC;
                CircBuff[wrptr].SecSince1970 = (double)( PctimeAtPps.QuadPart /
                                                         CountsPerSecond );
                }


        // See if CB element is now completely valid

        ParGpsCheckValid( wrptr, TsValid );


        // Update pps write pointer

        ParGpsIncCbPtr( &wrptr );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsSerialWriteCircBuff
// PURPOSE: Write serial time info into the next timestamp waiting in the
//          circular buffer for serial info.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsSerialWriteCircBuff( unsigned int PpsEvent, double SecSince1970,
                                       int YMDSource, int HMSSource, int NumSat,
                                       int TsValid, int Erase ) {

        static int wrptr = 0;



        // Ignore buffer entries already skipped

        ParGpsUpdateCbPtrForSkip( &wrptr );


        // Check for overflow

        if ( (CircBuff[wrptr].Valid & TS_VALID_SERIAL & TsValid) != TS_VALID_NONE) {
                sprintf( ParGpsLastErrorMessage,
                         "ERROR: CircBuff overflow on Serial write\n");
                return( 0 );
                }


        // Check or write PpsEvent

        if ( !ParGpsCheckEventNum( PpsEvent, TsValid, &wrptr, "Serial" ) )
                return( 0 );



        // Write Serial data to buffer

        CircBuff[wrptr].Valid       |= TS_VALID_SERIAL;
        CircBuff[wrptr].NumSat       = NumSat;
//        CircBuff[wrptr].Erase        = Erase;


        // Update time source info only with valid data

        if ( (YMDSource != TIME_SOURCE_NONE) &&
             (HMSSource != TIME_SOURCE_NONE) ) {
                CircBuff[wrptr].YmdSource    = YMDSource;
                CircBuff[wrptr].HmsSource    = HMSSource;
                CircBuff[wrptr].SecSince1970 = SecSince1970;
                }


        // See if CB element is now completely valid

        ParGpsCheckValid( wrptr, TsValid );


        // Update Serial write pointer

        ParGpsIncCbPtr( &wrptr );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsIncCbPtr
// PURPOSE: Increment circular buffer pointer accounting for wrapping.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsIncCbPtr( int *PtrIndex ) {

        int origptr;

        origptr = *PtrIndex;

        *PtrIndex += 1;
        if (*PtrIndex >= MAXCIRC)
                *PtrIndex = 0;

        return( *PtrIndex );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsUpdateCbPtrForSkip
// PURPOSE: Update write pointer to circular buffer pointer so it skips over
//          discarded entries and catches up to the read pointer.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsUpdateCbPtrForSkip( int *PtrIndex ) {
//WCT - More review of wrap behavior recommended.

        int diff, rdcompare;

        if ( rdskip ) {

                // Determine how far apart the pointers are.  If they
                // are too far, we assume one has wrapped so we compute
                // a corrected ptr value to compare agains.

                if (*PtrIndex < rdptr) {
                        diff      = rdptr - *PtrIndex;
                        rdcompare = rdptr - MAXCIRC;
                        }
                else {
                        diff      = *PtrIndex - rdptr;
                        rdcompare = rdptr + MAXCIRC;
                        }

                // If they are close, no adjustment is needed.

                if ( diff < 20 )
                        rdcompare = rdptr;
                else
                        sprintf( ParGpsLastErrorMessage,
                                 "Rdskip wrap adjustment rd %d, new %d, wr %d\n",
                                  rdptr, rdcompare, *PtrIndex );


                // Write ptr behind read ptr so update it

                if (*PtrIndex < rdcompare)
                        *PtrIndex = rdptr;

                } // end if rdskip


        return( *PtrIndex );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsCheckEventNum
// PURPOSE: Check the current circular buffer entry to ensure consistency based
//          on the PpsEventNum.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsCheckEventNum( unsigned int PpsEvent, int TsValid,
                                 int *wrptrAPS, char *StrAPS ) {


        // Check or write PpsEvent

        if ( (CircBuff[*wrptrAPS].Valid & TsValid) != TS_VALID_NONE) {

                if (PpsEvent < CircBuff[*wrptrAPS].PpsEvent ) {

                        // This code discards data for an earlier event
                        // since we know we will never get all three
                        // required types of data (analog, pps, serial)
                        // for that earlier event.

                        sprintf( ParGpsLastErrorMessage,
                                 "Ignoring %s PpsEvent %u < existing PpsEvent %u\n",
                                 StrAPS, PpsEvent, CircBuff[*wrptrAPS].PpsEvent );
                        return( 0 );
                        }

                else if (PpsEvent > CircBuff[*wrptrAPS].PpsEvent ) {

                        // This code corrects for start up conditions
                        // where one of the three types of data (analog,
                        // pps, serial) needed to determine the time of
                        // an event was stored in an earlier file that
                        // we know nothing about.  In this case, we just
                        // throw away the current buffer entry and start
                        // with the next event.

                        while ( PpsEvent > CircBuff[*wrptrAPS].PpsEvent &&
                                CircBuff[*wrptrAPS].PpsEvent > 0 )        {
                                sprintf( ParGpsLastErrorMessage,
                                         "Discarding PpsEvent %u < %s PpsEvent %u\n",
                                         CircBuff[*wrptrAPS].PpsEvent, StrAPS, PpsEvent );
                                ParGpsIncCbPtr( wrptrAPS ); // changes wrptrAPS
                                }
                        while (rdptr < *wrptrAPS)
                                ParGpsSkipCircBuff( );
                        if (CircBuff[*wrptrAPS].PpsEvent == 0 )
                                CircBuff[*wrptrAPS].PpsEvent = PpsEvent;

                        } // end else if PpsEvent > CircBuff

                } // end if CircBuff valid


        else {
                // Assign the PpsEvent to this buffer entry since it
                // doesn't have one yet.

                CircBuff[*wrptrAPS].PpsEvent = PpsEvent;
                }

        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: ParGpsSrcToSource
// PURPOSE: Translate between specific NMEA string SRCSTR_ constants and more
//          general TIME_SOURCE_ constants.  Both types are defined in pargps.h.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsSrcToSource( int YmdSrc, int HmsSrc, int YmdValid, int HmsValid,
                               int *YmdSource, int *HmsSource ) {

        int YmdTemp, HmsTemp;

        // For Year Month Day

        if ( YmdSrc == SRCSTR_PC )
                YmdTemp = TIME_SOURCE_PC;

        else if ( YmdSrc == SRCSTR_OLD || YmdSrc == SRCSTR_CLC )
                YmdTemp = TIME_SOURCE_CALC;

        else if ( YmdSrc > SRCSTR_MIN  &&  YmdSrc < SRCSTR_MAX ) {
                if ( YmdValid )
                        YmdTemp = TIME_SOURCE_GPS;
                else
                        YmdTemp = TIME_SOURCE_EST;
                }

        else if ( YmdSrc == SRCSTR_UNK )
                YmdTemp = TIME_SOURCE_NONE;

        else
                YmdTemp = TIME_SOURCE_NONE;


        // For Hour Minute Second

        if ( HmsSrc == SRCSTR_PC )
                HmsTemp = TIME_SOURCE_PC;

        else if ( HmsSrc == SRCSTR_OLD || HmsSrc == SRCSTR_CLC )
                HmsTemp = TIME_SOURCE_CALC;

        else if ( HmsSrc > SRCSTR_MIN  &&  HmsSrc < SRCSTR_MAX ) {
                if ( HmsValid )
                        HmsTemp = TIME_SOURCE_GPS;
                else
                        HmsTemp = TIME_SOURCE_EST;
                }

        else if ( HmsSrc == SRCSTR_UNK )
                HmsTemp = TIME_SOURCE_NONE;

        else
                HmsTemp = TIME_SOURCE_NONE;


        // Fill optional return parameters

        if ( YmdSource )
                *YmdSource = YmdTemp;
        if ( HmsSource )
                *HmsSource = HmsTemp;

        return( 1 );
}




/********************** INTERPOLATION HELPER FUNCTIONS **********************/

/*
 * Given the x value for one point and some other data, compute the y
 * value for that point.  The other data may be two additional x,y points
 * or one x,y point and a slope.  Linear interpolation is used in either
 * case.
 *
 */

//------------------------------------------------------------------------------
// ROUTINE: ParGpsEquation2pt
// PURPOSE: Compute the y value for a given x value using linear interpolation
//          done with the 2 point version of the straight line formula.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsEquation2pt( double x1, double y1,
                               double x2, double y2,
                               double xk, double *yk ) {

        double diff;

        // Error check for
        //   1. No place to put answer
        //   2. Points are identical (this would lead to divide by zero)


        if ( x2 > x1 )
                diff = x2 - x1;
        else
                diff = x1 - x2;

        if ( !yk  || diff < TOLERANCE )
                return( 0 );



        // Solve the equation

        *yk = ((y2 - y1) * (xk - x1) / (x2 - x1)) + y1;


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsEquationSlope
// PURPOSE: Compute the y value for a given x value using linear interpolation
//          done with the slope and offset version of the straight line formula.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsEquationSlope( double m,
                                 double x1, double y1,
                                 double xk, double *yk ) {

        // Error check for no place to put answer

        if ( !yk )
                return( 0 );


        // Solve the equation

        *yk = m * (xk - x1) + y1;


        return( 1 );
}





/********************** OS DEPENDENT HELPER FUNCTIONS **********************/

// These functions exist to hide operating system differences from the
// main library functions above.

#define MAXENVSTR 256

#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsDriverOpen (Windows version)
// PURPOSE: Open the PARGPS device driver.
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE ParGpsOsDriverOpen( char *DriverName ) {

        DEVHANDLE SrHandle;
        HANDLE    NtHandle;
        char      DeviceNameBuffer[256];

        sprintf( DeviceNameBuffer, "\\\\.\\%s", DriverName );


        NtHandle = CreateFile(
                        DeviceNameBuffer,                   // "file" name
                        GENERIC_READ    | GENERIC_WRITE,    // access mode
                        FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode
                        NULL,                               // security
                        OPEN_EXISTING,                      // create mode
                        0,                                  // file attrib
                        NULL                                // copy from
                        );

        if (NtHandle == INVALID_HANDLE_VALUE)
                SrHandle = BAD_DEVHANDLE;
        else
                SrHandle = (DEVHANDLE)NtHandle;

        return( SrHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsDriverClose (Windows version)
// PURPOSE: Close the PARGPS device driver.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsDriverClose( DEVHANDLE *ParGpsHandle ) {

        int ret;

        ret = CloseHandle( *ParGpsHandle );
        *ParGpsHandle = BAD_DEVHANDLE;

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsDriverIoctl (Windows version)
// PURPOSE: Request the PARGPS device driver perform a specialized function.
//          The IoCtlCode parameter selects which of the possible functions is
//          to be done.  These include functions like starting/stopping,
//          setting the serial port, reading the counter frequency,
//          reading the pps/serial data, etc.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsDriverIoctl( DEVHANDLE      ParGpsHandle,
                                 unsigned long  IoCtlCode,
                                 void          *pValueIn,
                                 unsigned long  InSize,
                                 void          *pValueOut,
                                 unsigned long  OutSize,
                                 unsigned long *pBytesReturned
                               ) {

        int ret;

                ret = DeviceIoControl(
                       ParGpsHandle,     // Handle to device
                       IoCtlCode,        // IO Control code
                       pValueIn,         // Input data to driver
                       InSize,           // Length of in data in bytes
                       pValueOut,        // Output data from driver
                       OutSize,          // Length of out data in bytes
                       pBytesReturned,   // Bytes placed in output buffer
                       NULL              // NULL = wait till I/O completes
                       );

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsGetLastError (Windows version)
// PURPOSE: Return the error message number provided by the operating system
//          for the last operation.
//------------------------------------------------------------------------------
SRLOCAL long ParGpsOsGetLastError( void ) {

        return( GetLastError() );  // Provided by the OS
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsGetDefaultModel (Windows version)
// PURPOSE: Get the default PARGPS model information indicating if the older
//          Trimble ACE-III or newer Garmin GPS 18 LVC model is to be used.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsGetDefaultModel( char *DriverName ) {

        int   GpsModel;
        char  ServiceParmKeyName[MAXENVSTR], DefaultModel[MAXENVSTR];
        HKEY  ServiceParmKey;
        DWORD Code, BufferSize, RegType;


        // Open service parameters registry key

        sprintf( ServiceParmKeyName,
                 "System\\CurrentControlSet\\Services\\%s\\Parameters",
                 DriverName );

        Code = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,      // already open key
                       ServiceParmKeyName,      // subkey to open
                       0,                       // reserved
                       KEY_ALL_ACCESS,          // full access permission
                       &ServiceParmKey          // handle to key
                      );

        if (Code != ERROR_SUCCESS)
                return( GPSMODEL_UNKNOWN );



        // Get current value of Model Name

        DefaultModel[0] = '\0';
        BufferSize       = sizeof( DefaultModel );
        Code = RegQueryValueEx(
                               ServiceParmKey,
                               REGENVSTR_GPSMODELNAME,
                               NULL,
                               &RegType,
                               DefaultModel,
                               &BufferSize
                              );

        if (Code != ERROR_SUCCESS)
                return( GPSMODEL_UNKNOWN );


        // Close service parm key

        Code = RegCloseKey( ServiceParmKey );



        // Translate from result to model id

        if (!DefaultModel)
                GpsModel = GPSMODEL_UNKNOWN;

        else if ( strcmp( DefaultModel, GPSMODELNAME_GARMIN ) == 0 )
                GpsModel = GPSMODEL_GARMIN;

        else if ( strcmp( DefaultModel, GPSMODELNAME_TRIMBLE ) == 0 )
                GpsModel = GPSMODEL_TRIMBLE;

        else if ( strcmp( DefaultModel, GPSMODELNAME_ONCORE ) == 0 )
                GpsModel = GPSMODEL_ONCORE;

        else if ( strcmp( DefaultModel, GPSMODELNAME_PCTIME ) == 0 )
                GpsModel = GPSMODEL_PCTIME;

        else
                GpsModel = GPSMODEL_UNKNOWN;

        return( GpsModel );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialOpen (Windows version)
// PURPOSE: Open the named serial port.
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE ParGpsOsSerialOpen( char *SerialPortName ) {

        DEVHANDLE hSerial;


        // Open serial port device as non-blocking.

        hSerial = CreateFile( SerialPortName,               // port name
                              GENERIC_READ | GENERIC_WRITE, // access
                              0,                            // no share
                              NULL,                         // no security attrib
                              OPEN_EXISTING,                // create type
                              0,                            // non-overlapped I/O
                              NULL                          // no template file
                            );

//                            FILE_FLAG_OVERLAPPED,         // overlapped I/O


        if ( hSerial == INVALID_HANDLE_VALUE )
                hSerial = BAD_DEVHANDLE;

        return( hSerial );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialInit (Windows version)
// PURPOSE: Initialize the specified serial port with values appropriate for
//          the PARGPS.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsSerialInit( DEVHANDLE hSerial ) {

        DCB NewSettings;
        DWORD EventMask;
        COMMTIMEOUTS NewTimeouts;


        // Can't do init for a bad serial handle

        if (hSerial == BAD_DEVHANDLE)
                return( 0 );




        // Prepare new port settings.

        /* For a complete discussion of these flags see the MSDN
         * documentation for the DCB structure.
         *
         */

        NewSettings.DCBlength = sizeof(DCB);
        if (GetCommState( hSerial, &NewSettings ) == 0) {
                sprintf( ParGpsLastErrorMessage, "Failed to get state of COM port\n");
                return( 0 );
                }


        NewSettings.BaudRate        = CBR_4800;     // baud rate 4800
        NewSettings.ByteSize        = 8;            // 8 data bits
        NewSettings.Parity          = NOPARITY;     // no parity
        NewSettings.StopBits        = ONESTOPBIT;   // 1 stop bit

        NewSettings.fDsrSensitivity = FALSE;        // local, not modem
        NewSettings.fNull           = FALSE;        // no discard of nulls
        NewSettings.fOutX           = FALSE;        // no outgoing XON/XOFF
        NewSettings.fInX            = FALSE;        // no incoming XON/XOFF
        NewSettings.fOutxCtsFlow    = FALSE;        // no CTS flow control
        NewSettings.fOutxDsrFlow    = FALSE;        // no DSR flow control
        NewSettings.fDtrControl     = DTR_CONTROL_DISABLE; // no DTR flow control
        NewSettings.fRtsControl     = RTS_CONTROL_DISABLE; // no RTS flow control
        NewSettings.fTXContinueOnXoff = TRUE;       // XOFF continues Tx
        NewSettings.fErrorChar      = FALSE;        // disable error
        NewSettings.fAbortOnError   = FALSE;        // no r/w error abort

        NewSettings.EofChar         = '\n';         // Carriage Return is both
        NewSettings.EvtChar         = '\n';         // end of input and event


        NewTimeouts.ReadIntervalTimeout         = MAXDWORD; // Allow non-blocking
        NewTimeouts.ReadTotalTimeoutMultiplier  = 0;
        NewTimeouts.ReadTotalTimeoutConstant    = 0;
        NewTimeouts.WriteTotalTimeoutMultiplier = 0;        // Don't use
        NewTimeouts.WriteTotalTimeoutConstant   = 0;



//      Linux version registers cleanup function for Ctrl-C interrupt here
//      signal( SIGINT, ParGpsOsSerialSignalHandler );



        // Clear line and activate settings.

        if (SetCommTimeouts( hSerial, &NewTimeouts ) == 0) {
                sprintf( ParGpsLastErrorMessage, "Failed to set Timeouts for COM port\n");
                return( 0 );
                }

        if (SetCommState( hSerial, &NewSettings ) == 0) {
                sprintf( ParGpsLastErrorMessage, "Failed to set State for COM port\n");
                return( 0 );
                }



        // Set RTS low to provide negative power for SR PARGPS board

        EscapeCommFunction( hSerial, CLRRTS );


        // Set events

        EventMask = EV_RXFLAG | EV_RING;
        SetCommMask( hSerial, EventMask );



        // Clear all comm buffers

        PurgeComm( hSerial, PURGE_TXABORT | PURGE_TXCLEAR |
                            PURGE_RXABORT | PURGE_RXCLEAR );


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialRead (Windows version)
// PURPOSE: Read from the specified serial port.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsSerialRead(
                          DEVHANDLE      SerialHandle,
                          void          *pValues,
                          unsigned long  BytesToRead,
                          unsigned long *pBytesRead
                        ) {

        return(
                ReadFile(
                          SerialHandle,    // Handle to device
                          pValues,         // Buffer to receive data
                          BytesToRead,     // Number of bytes to read
                          pBytesRead,      // Bytes read
                          NULL             // NULL = wait till I/O completes
                        )
                );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialClose (Windows version)
// PURPOSE: Close from the specified serial port.
//------------------------------------------------------------------------------
SRLOCAL void ParGpsOsSerialClose( DEVHANDLE hSerial ) {


        // Close the serial port.

        CloseHandle( hSerial );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSetPcTime (Windows version)
// PURPOSE: Set the PC system time from the supplied time specified as seconds
//          since 1970 or as YMD HMS.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsSetPcTime( double Time,
                               int Year, int Month, int Day,
                               int Hour, int Minute, int Second,
                               long Microsecond, int FromYMD ) {

        SYSTEMTIME Systim;
        int        Yr, Mo, Dy, Hh, Mm, Ss;
        long       Usec;


        // Prepare time in SYSTEMTIME format Windows prefers

        if (FromYMD == 1) {
                Systim.wYear         = Year;
                Systim.wMonth        = Month;
                Systim.wDay          = Day;
                Systim.wHour         = Hour;
                Systim.wMinute       = Minute;
                Systim.wSecond       = Second;
                Systim.wMilliseconds = (int)(Microsecond / USPERMS);
                }
        else {
                ParGpsSecTimeSplit( Time, &Yr, &Mo, &Dy, &Hh, &Mm, &Ss, &Usec );
                Systim.wYear         = Yr;
                Systim.wMonth        = Mo;
                Systim.wDay          = Dy;
                Systim.wHour         = Hh;
                Systim.wMinute       = Mm;
                Systim.wSecond       = Ss;
                Systim.wMilliseconds = (int)(Usec / USPERMS);
                }

        Systim.wDayOfWeek    = 7; // this invalid number is ignored

        if (SetSystemTime( &Systim ) == 0) {
                ParGpsLastDriverError = ParGpsOsGetLastError();
                return( 0 );
                }
        else
                return( 1 );


}

#elif defined( SROS_LINUX )

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsDriverOpen (Linux version)
// PURPOSE: Open the PARGPS device driver.
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE ParGpsOsDriverOpen( char *DriverName ) {

        DEVHANDLE ParGpsHandle;
        char      DeviceNameBuffer[256];

        sprintf( DeviceNameBuffer, "/dev/%s", DriverName );


        ParGpsHandle = open( DeviceNameBuffer, O_RDWR );

        if (ParGpsHandle < 0)
                ParGpsHandle = BAD_DEVHANDLE;

        return( ParGpsHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsDriverClose (Linux version)
// PURPOSE: Close the PARGPS device driver.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsDriverClose( DEVHANDLE *ParGpsHandle ) {

        int ret;

        if ( close( *ParGpsHandle ) < 0 )
                ret = 0;
        else
                ret = 1;
        *ParGpsHandle = BAD_DEVHANDLE;

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsDriverIoctl (Linux version)
// PURPOSE: Request the PARGPS device driver perform a specialized function.
//          The IoCtlCode parameter selects which of the possible functions is
//          to be done.  These include functions like starting/stopping,
//          setting the serial port, reading the counter frequency,
//          reading the pps/serial data, etc.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsDriverIoctl( DEVHANDLE      ParGpsHandle,
                                 unsigned long  IoCtlCode,
                                 void          *pValueIn,
                                 unsigned long  InSize,
                                 void          *pValueOut,
                                 unsigned long  OutSize,
                                 unsigned long *pBytesReturned
                               ) {

        int code;
        IRP IoctlData;



        // The ioctl function just takes one buffer.  On the way in,
        // it contains the input data and on the way out, it contains
        // the output data.  So, make sure we are using the largest of
        // the two and that it contains the input data.  The size of the
        // data must be the size defined for that IO control code in
        // pargpskd.h.


        IoctlData.Command       = IoCtlCode;
        IoctlData.InBuffer      = pValueIn;
        IoctlData.InSize        = InSize;
        IoctlData.OutBuffer     = pValueOut;
        IoctlData.OutSize       = OutSize;
        IoctlData.ReturnedBytes = 0;
        IoctlData.DataMethod    = METHOD_BUFFERED;
        IoctlData.ErrorCode     = 0;
        IoctlData.UserIrp       = &IoctlData;

        // Check in pargpskd.h for which control codes do not
        // use the more common buffered method of passing data

        if ((IoCtlCode == IOCTL_PARGPS_READ_PPS_DATA)     ||
            (IoCtlCode == IOCTL_PARGPS_READ_SERIAL_DATA) ) {

                IoctlData.DataMethod = METHOD_OUT_DIRECT;
                }


        // Request the ioctl function

        code = ioctl( ParGpsHandle, IoCtlCode, &IoctlData );

        if (pBytesReturned)
                *pBytesReturned = IoctlData.ReturnedBytes;



        if (code < 0)
                return( 0 );
        else
                return( 1 );

}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsGetLastError (Linux version)
// PURPOSE: Return the error message number provided by the operating system
//          for the last operation.
//------------------------------------------------------------------------------
SRLOCAL long ParGpsOsGetLastError( void ) {

        return( (long)errno );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsGetDefaultModel (Linux version)
// PURPOSE: Get the default PARGPS model information indicating if the older
//          Trimble ACE-III or newer Garmin GPS 18 LVC model is to be used.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsGetDefaultModel( char *DriverName ) {

        int GpsModel;
        char *EnvResult, EnvString[MAXENVSTR];

        // Prepare environment string and get resulting variable

        sprintf( EnvString, "%s_%s", DriverName, REGENVSTR_GPSMODELNAME );
        EnvResult = getenv( EnvString );


        // Translate from environment variable to model id

        if (!EnvResult)
                GpsModel = GPSMODEL_UNKNOWN;
        else if ( strcmp( EnvResult, GPSMODELNAME_GARMIN ) == 0 )
                GpsModel = GPSMODEL_GARMIN;
        else if ( strcmp( EnvResult, GPSMODELNAME_TRIMBLE ) == 0 )
                GpsModel = GPSMODEL_TRIMBLE;
        else if ( strcmp( EnvResult, GPSMODELNAME_ONCORE ) == 0 )
                GpsModel = GPSMODEL_ONCORE;
        else if ( strcmp( EnvResult, GPSMODELNAME_PCTIME ) == 0 )
                GpsModel = GPSMODEL_PCTIME;
        else
                GpsModel = GPSMODEL_UNKNOWN;

        return( GpsModel );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialOpen (Linux version)
// PURPOSE: Open the named serial port.
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE ParGpsOsSerialOpen( char *SerialPortName ) {

        DEVHANDLE hSerial;

        // Open serial port device as non-blocking.

        hSerial = open( SerialPortName, O_RDONLY | O_NONBLOCK );

        if ( hSerial < 0 ) {
                hSerial = BAD_DEVHANDLE;
                Signal_hSerial = hSerial;
                perror( SerialPortName );
                return( hSerial);
                }
        else
                Signal_hSerial = hSerial;

        return( hSerial );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialInit (Linux version)
// PURPOSE: Initialize the specified serial port with values appropriate for
//          the PARGPS.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsSerialInit( DEVHANDLE hSerial ) {

        struct termios NewSettings;
        int            ModemStatus;


        // Can't do init for a bad serial handle

        if (hSerial == BAD_DEVHANDLE)
                return( 0 );


        // Prepare new port settings.

        // For a complete discussion of these flags see the Linux info
        // system for libc:Low Level Terminal Interface:Terminal Modes

        tcgetattr( hSerial, &NewSettings );

        NewSettings.c_iflag &= ~INPCK;   // input parity checking
        NewSettings.c_iflag &= ~IGNCR;   // input not ignore CR
        NewSettings.c_iflag &= ~ICRNL;   // input not change CR to NL
        NewSettings.c_iflag &= ~INLCR;   // input not change NL to CR
        NewSettings.c_iflag &= ~IXOFF;   // input  no XON/XOFF flow control
        NewSettings.c_iflag &= ~IXON;    // output no XON/XOFF flow control
        // IGNBRK ignore break
        // BRKINT break raises SIGINT
        // IGNPAR ignore frame or parity error
        // PARMRK mark   frame or parity error
        // ISTRIP strip to 7 bits
        // IXANY  any character restarts output
        // IMAXBEL send bell when terminal input buffer full (BSD only)

        NewSettings.c_oflag &= ~OPOST;   // output processing not applied

//      NewSettings.c_cflag &= ~CBAUD;      // (Not POSIX) baudrate mask
//      NewSettings.c_cflag |=  B4800;      // (Not POSIX) baudrate 4800
        cfsetispeed( &NewSettings, B4800 ); // set  input baudrate to 4800
        cfsetospeed( &NewSettings, B4800 ); // set output baudrate to 4800

        NewSettings.c_cflag &= ~CSIZE;   // data bit mask
        NewSettings.c_cflag |=  CS8;     // 8 data bits
        NewSettings.c_cflag &= ~CSTOPB;  // use 1 stop bits not 2
        NewSettings.c_cflag |=  CREAD;   // enable receiving characters
        NewSettings.c_cflag &= ~PARENB;  // parity disabled
        NewSettings.c_cflag &= ~HUPCL;   // no hang up on modem disconnect
        NewSettings.c_cflag |=  CLOCAL;  // local connection, no modem contol
        // PARODD odd parity

        NewSettings.c_lflag |=  ISIG;    // listen to signals
        NewSettings.c_lflag &= ~ICANON;  // disable canonical input
        NewSettings.c_lflag &= ~ECHO;    // no echo characters
        NewSettings.c_lflag &= ~ECHOE;   // no echo erase
        NewSettings.c_lflag &= ~ECHOK;   // no echo kill
        NewSettings.c_lflag &= ~ECHONL;  // no echo newline
        // NOFLSH no flush on signal
        // TOSTOP job control stop for terminal io from background

        NewSettings.c_cc[VMIN]  = 0;     // Can return with 0 chars
        NewSettings.c_cc[VTIME] = 0;     // Number of .1 secs to wait



        // Register cleanup function for Ctrl-C interrupt.

        signal( SIGINT, ParGpsOsSerialSignalHandler );



        // Activate settings.

        tcsetattr( hSerial, TCSANOW, &NewSettings );



        // Set RTS low to provide negative power for SR PARGPS board
        // The sense of the bit to the signal is somewhat confused,
        // but by experiment, it seems that clearing the bit causes
        // the signal to go to -volts which is what we need.

        ModemStatus = TIOCM_RTS;
        ioctl( hSerial, TIOCMBIC, &ModemStatus ); // just clr bit for active low



//      Winnt version sets up events or select here
//      EventMask = EV_RXFLAG | EV_RING;  SetCommMask( hSerial, EventMask );



        // Clear line.

        tcflush( hSerial, TCIFLUSH );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialRead (Linux version)
// PURPOSE: Read from the specified serial port.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsSerialRead(
                          DEVHANDLE      SerialHandle,
                          void          *pValues,
                          unsigned long  BytesToRead,
                          unsigned long *pBytesRead
                         ) {
        long readreturn;

        readreturn = read( SerialHandle, pValues, BytesToRead );

        if (readreturn < 0) {
                *pBytesRead = 0;
                return( 0 );
                }
        else {
                *pBytesRead = readreturn;
                return( 1 );
                }
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialClose (Linux version)
// PURPOSE: Close from the specified serial port.
//------------------------------------------------------------------------------
SRLOCAL void ParGpsOsSerialClose( DEVHANDLE hSerial ) {

        // Close serial port.

        close( hSerial );

        Signal_hSerial = BAD_DEVHANDLE;
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSerialSignalHandler (Linux version)
// PURPOSE: Respond to a signal.
//------------------------------------------------------------------------------
SRLOCAL void ParGpsOsSerialSignalHandler( int SigNum ) {

        if (Signal_hSerial != BAD_DEVHANDLE)
                ParGpsOsSerialClose( Signal_hSerial );

        exit( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParGpsOsSetPcTime (Linux version)
// PURPOSE: Set the PC system time from the supplied time specified as seconds
//          since 1970 or as YMD HMS.
//------------------------------------------------------------------------------
SRLOCAL int ParGpsOsSetPcTime( double Time,
                               int Year, int Month, int Day,
                               int Hour, int Minute, int Second,
                               long Microsecond, int FromYMD ) {

        struct timeval tv;
        double         SetTime, IntTime, FracTime;


        // Prepare time in timeval format Linux prefers

        if (FromYMD == 1)
                ParGpsSecTimeCombine( Year, Month, Day, Hour, Minute, Second, Microsecond,
                                      &SetTime );
        else
                SetTime = Time;

        IntTime = (long)SetTime;
        FracTime = SetTime - IntTime;

        tv.tv_sec = (long)IntTime;
        tv.tv_usec = (long)(FracTime*USPERSEC);

        if (settimeofday( &tv, NULL ) == -1) {
                ParGpsLastDriverError = ParGpsOsGetLastError();
                return( 0 );
                }
        else
                return( 1 );
}

#endif // SROS_xxx

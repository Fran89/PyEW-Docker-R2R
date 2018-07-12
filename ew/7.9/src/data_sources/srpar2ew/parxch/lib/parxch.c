/* FILE: parxch.c              Copyright (c), Symmetric Research, 2003-2007

These are the PARxCH user library functions.  See the include file
parxch.h for comments on how to call these functions.

These library functions are mostly wrappers around the lower level
read, write, and ioctl device driver functions that do the actual
work of controlling the PARxCH.  Users should limit themselves to
calling functions listed in the parxch.h header file unless they
are EXTREMELY EXPERT at calling device driver code directly.

To use this library, just compile it to an object file and statically
link it with your application.  For OS's other than DOS, it can also
be compiled to a DLL or shared object library.  See the makefiles for
examples of how to compile.

This single source code files services all the supported operating
systems.  When compiling, define one of the SROS_xxxxx constants
shown in parxch.h on the compiler command line to indicate which OS
you are using.

The location and use of the low level device driver code varies
depending on which OS you are using.  For DOS and Win9x, the driver
code is located in parxchkd.obj and should be linked in statically.
For Win2K/XP and Linux, the driver code is provided as a kernel
mode device driver and must be installed using the indriver utility
in the driver directory.  For Win2K/XP, the driver is named
parxchkd.sys and for Linux it is named parxchkd.ko.  See the driver
directory for more details.

The PARxCH library functions have been divided into several groups
such as Initialization, Execution control, FIFO, Read data, User
LED and digital I/O, etc.  These are noted below.  The ParXchOpen
function must be called to initialize the PARxCH A/D and the PC
parallel port before any of the other functions can be called.  The
only exceptions to this rule are a few helper functions.

*/


#include <stdio.h>
#include <string.h>      // strlen and strcmp
#include <sys/stat.h>    // fstat function (VC++ can use / in path)


// OS dependent includes ...

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )
#include <windows.h>     // windef.h -> winnt.h -> defines GENERIC_READ etc
#include <winioctl.h>    // device driver I/O control macros


#elif defined( SROS_WIN9X )
#include <windows.h>     // windef.h -> winnt.h -> defines GENERIC_READ etc
#include <conio.h>       // kbhit, getch functions
#include <stdlib.h>      // sleep function now from windows.h
#include <ctype.h>       // toupper function


#elif defined( SROS_MSDOS )
#include <sys\timeb.h>   // ftime function
#include <conio.h>       // kbhit, getch functions
#include <stdlib.h>      // getenv
#include <ctype.h>       // toupper function


#elif defined( SROS_LINUX )
#include <errno.h>       // errno global
#include <fcntl.h>       // open  function
#include <stdlib.h>      // for getenv, putenv
#include <sys/ioctl.h>   // ioctl function and macros


#endif  // SROS_xxxxx



// PARxCH includes ...

#include "parxch.h"    // for PARxCH defines and prototypes
#include "parxchkd.h"  // for PARxCH ioctl id constants


#ifdef GPS_AVAILABLE
#include "pargps.h"       // for GPS functionality
#endif


// Actual allocation of PARxCH error string array, see parxch.h ...

VARTYPE( char ) *PARXCH_ERROR_MSG[] = {

        PARXCH_ERROR_MSG_NONE,
        PARXCH_ERROR_MSG_PORT_ADDRESS,
        PARXCH_ERROR_MSG_PORT_MODE,
        PARXCH_ERROR_MSG_DATA_FORMAT,
        PARXCH_ERROR_MSG_CAL_MODE,
        PARXCH_ERROR_MSG_GAIN,
        PARXCH_ERROR_MSG_TURBO,
        PARXCH_ERROR_MSG_GAIN_TURBO_PRODUCT,
        PARXCH_ERROR_MSG_DECIMATION,
        PARXCH_ERROR_MSG_DRIVER_NOT_OPEN,
        PARXCH_ERROR_MSG_DRIVER_REQUEST_FAILED,
        PARXCH_ERROR_MSG_VOLTAGE_BAD_ON,
        PARXCH_ERROR_MSG_VOLTAGE_BAD_OFF,
        PARXCH_ERROR_MSG_NOT_UNLOCKED,
        PARXCH_ERROR_MSG_ADS1210_CR_READBACK,
        PARXCH_ERROR_MSG_FIFO_SANITY,
        PARXCH_ERROR_MSG_OVERFLOW,
        PARXCH_ERROR_MSG_KEYPRESS,
        PARXCH_ERROR_MSG_GPS_NOT_AVAILABLE,
        PARXCH_ERROR_MSG_GPS_NOT_ATTACHED,
        PARXCH_ERROR_MSG_GPS_VERSION_MISMATCH,
        PARXCH_ERROR_MSG_DIGITAL_NOT_AVAILABLE,
        PARXCH_ERROR_MSG_ATOD_UNKNOWN,
        PARXCH_ERROR_MSG_NO_INTERRUPT,
        PARXCH_ERROR_MSG_MAX

        };



/* GLOBAL ERROR VARIABLE:

The global variable ParXchLastDriverError keeps track of the last OS
provided error.  Most users will never need to access this.  If you
do, declare it as extern in your source code.

*/

long ParXchLastDriverError = 0L;




/* OS DEPENDENT FUNCTION PROTOTYPES:

This define controls whether the OS dependent functions are visible
to other source files.  These are for device driver EXPERTS ONLY.
Most users should let SRLOCAL be defined as static to avoid namespace
clutter and possible function name contention.  The only exceptions
would be for specialized debugging and diagnostic programs.

*/

#if !defined( SRLOCAL )
#define SRLOCAL static
#endif

SRLOCAL DEVHANDLE ParXchOsDriverOpen( char *DriverName );
SRLOCAL int ParXchOsDriverClose( DEVHANDLE ParXchHandle );
SRLOCAL int ParXchOsDriverRead( DEVHANDLE ParXchHandle,
                                void *pValues,
                                unsigned long BytesToRead,
                                unsigned long *pBytesRead );
SRLOCAL int ParXchOsDriverIoctl( DEVHANDLE ParXchHandle,
                                unsigned long IoCtlCode,
                                void *pValueIn, unsigned long InSize,
                                void *pValueOut, unsigned long  OutSize,
                                unsigned long *ReturnSize );
SRLOCAL long ParXchOsGetLastError( void );
SRLOCAL int ParXchOsGetDefaultModel( char *DriverName );
SRLOCAL void ParXchOsHardSleep( int ms );








//------------------------------------------------------------------------------
// ROUTINE: ParXchGetRev
// PURPOSE: Determine which version of the PARxCH library is being used.
//
// Short rev history:
//
//   PARXCH_REV  100  ( 07/01/2000 )  < First release, EPP only
//   PARXCH_REV  101  ( 10/01/2000 )  < Added BPP support, and NT
//   PARXCH_REV  110  ( 12/20/2000 )  < Toshiba fix
//   PARXCH_REV  201  ( 03/15/2001 )  < Major changes, added open etc ...
//   PARXCH_REV  210  ( 04/20/2001 )  < Added board unlock, port timeout reset
//   PARXCH_REV  211  ( 04/25/2001 )  < Added DRAM FIFO empty flag skew
//   PARXCH_REV  212  ( 05/05/2001 )  < BPP unlock and no voltage bad check
//   PARXCH_REV  213  ( 05/30/2001 )  < ECP support
//   PARXCH_REV  214  ( 12/15/2001 )  < Win2K ACPI support, SrWait for MSDOS
//   PARXCH_REV  215  ( 06/01/2001 )  < Rev D support
//   PARXCH_REV  216  ( 07/25/2002 )  < Add GPS support
//   PARXCH_REV  217  ( 08/25/2002 )  < Improved GPS support, separated helper fns
//   PARXCH_REV  218  ( 09/20/2002 )  < More GPS improvements
//   PARXCH_REV  219  ( 12/30/2002 )  < Protect BPP code from GPS interrupts
//   PARXCH_REV  220  ( 08/05/2003 )  < Added Linux memory barriers to driver
//   PARXCH_REV  221  ( 08/20/2003 )  < Added read and display of digital data
//   PARXCH_REV  222  ( 09/05/2003 )  < Reorganized and renamed OS dependent fns
//   PARXCH_REV  223  ( 01/01/2004 )  < Combined PAR1CH,4CH,8CH into PARxCH
//   PARXCH_REV  224  ( 07/01/2005 )  < Added UserIrp for Linux 2.6 kernels
//   PARXCH_REV  225  ( 03/01/2006 )  < Allow PCtime timestamping, SRDAT_REV 106
//   PARXCH_REV  226  ( 02/20/2007 )  < Make SpsGainToTde rounding consistent
//   PARXCH_REV  227  ( 08/15/2007 )  < Change InterruptEnable/Disable returns
//
//------------------------------------------------------------------------------

#define PARXCH_REV      227

FUNCTYPE( int ) ParXchGetRev( int *Rev ) {

        *Rev = PARXCH_REV;
        return( *Rev );
}




/************************ PARxCH INITIALIZATION FUNCTIONS *********************/

//------------------------------------------------------------------------------
// ROUTINE: ParXchOpen
// PURPOSE: To open the driver for a PARxCH A/D board.  The PARxCH family of
//          24 bit A/D boards includes 1, 4, and 8 channel models.  The XchModel
//          parameter specifies which model you are working with.
//
//          This is the function users should call to open the device driver and
//          initalize the PARxCH before starting execution.  It takes care of all
//          initialization.  No other functions need to be called to initialize
//          the PARxCH.
//
//          During initialization the ADS1210 control registers are programmed
//          with values setting all board parameters like sampling rate
//          programmable gain etc.  All ADS1210s are programmed in parallel with
//          the same values.
//
//          See parxch.h for defined constants to use with the parameters passed
//          to ParXchOpen.  To convert floating point sps (samples per second)
//          sampling rates to TurboLog and Decimation values, use the helper
//          function ParXchSpsGainToTde.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) ParXchOpen(
                                  char   *DriverName,
                                  int     XchModel,
                                  int     PortMode,
                                  double  Sps,

                                  double *ActualSps,
                                  int    *Error

                                  ) {

        DEVHANDLE ParXchHandle;
        double TrueSps;
        int TurboLog, GainLog, Decimation, ExtraDecimation;


        // Compute decimation parameters and actual Sps.

        GainLog = PARXCH_GAIN_MIN;

        ParXchSpsGainToTde(
                           Sps,
                           GainLog,
                           &TurboLog,
                           &Decimation,
                           &ExtraDecimation
                          );
        ParXchTdeToSpsGain(
                           TurboLog,
                           Decimation,
                           ExtraDecimation,
                           &TrueSps,
                           &GainLog
                          );
        if (ActualSps)
                *ActualSps = TrueSps;


        // Call advanced Open function.

        ParXchHandle = ParXchFullOpen(
                                       DriverName,
                                       XchModel,
                                       PortMode,

                                       PARXCH_DF_SIGNED,
                                       GainLog,
                                       TurboLog,
                                       Decimation,
                                       ExtraDecimation,
                                       PARXCH_UNUSED,

                                       NULL,
                                       Error
                                      );

        return( ParXchHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: Par8chOpen
// PURPOSE: To open the driver for a PAR8CH A/D board.  Most users will want to
//          use the generic ParXchOpen instead.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) Par8chOpen(
                                  char   *DriverName,
                                  int     PortMode,
                                  double  Sps,

                                  double *ActualSps,
                                  int    *Error

                                  ) {

        return( ParXchOpen( DriverName, PARXCH_MODEL_PAR8CH, PortMode,
                            Sps, ActualSps, Error ) );
}


//------------------------------------------------------------------------------
// ROUTINE: Par4chOpen
// PURPOSE: To open the driver for a PAR4CH A/D board.  Most users will want to
//          use the generic ParXchOpen instead.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) Par4chOpen(
                                  char   *DriverName,
                                  int     PortMode,
                                  double  Sps,

                                  double *ActualSps,
                                  int    *Error

                                  ) {

        return( ParXchOpen( DriverName, PARXCH_MODEL_PAR4CH, PortMode,
                            Sps, ActualSps, Error ) );
}


//------------------------------------------------------------------------------
// ROUTINE: Par1chOpen
// PURPOSE: To open the driver for a PAR1CH A/D board.  Most users will want to
//          use the generic ParXchOpen instead.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) Par1chOpen(
                                  char   *DriverName,
                                  int     PortMode,
                                  double  Sps,

                                  double *ActualSps,
                                  int    *Error

                                  ) {

        return( ParXchOpen( DriverName, PARXCH_MODEL_PAR1CH, PortMode,
                            Sps, ActualSps, Error ) );
}


//------------------------------------------------------------------------------
// ROUTINE: ParXchFullOpen
// PURPOSE: To open the driver for a PARxCH A/D board.  This version of the open
//          function provides even more control over the parameters that will be
//          used to initialize the driver.  Most users will probably prefer to
//          use the simpler ParXchOpen function shown above instead.
//------------------------------------------------------------------------------
#define ERROR_RETURN( err ) {                                   \
                                if ( Error )                    \
                                        *Error = err;           \
                                                                \
                                return( BAD_DEVHANDLE );        \
                             }

FUNCTYPE( DEVHANDLE ) ParXchFullOpen(
                                      char *DriverName,
                                      int   XchModel,
                                      int   PortMode,

                                      int   Df,
                                      int   GainLog,
                                      int   TurboLog,
                                      int   Decimation,
                                      int   ExtraDecimation,
                                      int   Unused,

                                      long *ADS1210_CrValue,
                                      int  *Error

                                     ) {

        DEVHANDLE    ParXchHandle;
        INITXCHPARMS Parm;


        // Check all the initialization parameters:
        // parallel port mode, data format, gain, turbo mode, and decimation

        if ( ( PortMode != PARXCH_PORT_MODE_EPP )     &&
             ( PortMode != PARXCH_PORT_MODE_BPP )     &&
             ( PortMode != PARXCH_PORT_MODE_ECP_BPP ) &&
             ( PortMode != PARXCH_PORT_MODE_ECP_EPP ) )

                ERROR_RETURN( PARXCH_ERROR_PORT_MODE );


        if ( (Df != PARXCH_DF_OFFSET) && (Df != PARXCH_DF_SIGNED) )

                ERROR_RETURN( PARXCH_ERROR_DATA_FORMAT );


        if ( (GainLog < PARXCH_GAIN_1) || (GainLog > PARXCH_GAIN_16) )

                ERROR_RETURN( PARXCH_ERROR_GAIN );


        if ( (TurboLog < PARXCH_TURBO_1) || (TurboLog > PARXCH_TURBO_16) )

                ERROR_RETURN( PARXCH_ERROR_TURBO );


        if ( (GainLog+TurboLog < PARXCH_GAINTURBO_MIN) ||
             (GainLog+TurboLog > PARXCH_GAINTURBO_MAX) )

                ERROR_RETURN( PARXCH_ERROR_GAIN_TURBO_PRODUCT );


        if ( (Decimation < PARXCH_DECIMATION_MIN) ||
             (Decimation > PARXCH_DECIMATION_MAX) )

                ERROR_RETURN( PARXCH_ERROR_DECIMATION );

        if ( (ExtraDecimation < PARXCH_EXTRADECIMATION_MIN) )

                ERROR_RETURN( PARXCH_ERROR_DECIMATION );






        // Open the device driver.

        ParXchHandle = ParXchOsDriverOpen( DriverName );

        if ( ParXchHandle == BAD_DEVHANDLE ) {
                ParXchLastDriverError = ParXchOsGetLastError();
                ERROR_RETURN( PARXCH_ERROR_DRIVER_NOT_OPEN );
                }


        // Set up parameters to be passed to driver.

        Parm.XchModel        = XchModel;
        Parm.ParPortMode     = PortMode;
        Parm.Df              = Df;
        Parm.GainLog         = GainLog;
        Parm.TurboLog        = TurboLog;
        Parm.Decimation      = Decimation;
        Parm.ExtraDecimation = ExtraDecimation;
        Parm.ADS1210_CrValue = 0;
        Parm.Error           = PARXCH_ERROR_NONE;


        // Initialize the A/D board.

        if ( !ParXchOsDriverIoctl(
                              ParXchHandle,
                              IOCTL_PARXCH_INIT,
                              &Parm,
                              sizeof(Parm),
                              &Parm,
                              sizeof(Parm),
                              NULL
                            ) )
              Parm.Error = PARXCH_ERROR_DRIVER_REQUEST_FAILED;


        // Save CrValue set by kernel init for optional output.

        if (ADS1210_CrValue)
                *ADS1210_CrValue = Parm.ADS1210_CrValue;



        // Trap IoCtrl failure and IoCtrl success but Kernel Init failure.
        // Driver was successfully opened, so close it.

        if ( Parm.Error != PARXCH_ERROR_NONE ) {

                ParXchLastDriverError = ParXchOsGetLastError();
                ParXchClose( ParXchHandle );
                ERROR_RETURN( Parm.Error );
                }



        // Finish.

        if ( Error )

                *Error = Parm.Error;

        return( ParXchHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchClose
// PURPOSE: To close the driver for a PARxCH A/D board.  Since only one program
//          can have the PARxCH driver open at a time, it is important to close
//          the driver when you are done with it.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchClose( DEVHANDLE ParXchHandle ) {

        // Only close open drivers.

        if (ParXchHandle == BAD_DEVHANDLE) {
                ParXchLastDriverError = ERROR_SERVICE_DISABLED;
                return( 1 );
                }


        // Close driver.

        if ( !ParXchOsDriverClose(ParXchHandle) )
                return( 0 );


        ParXchLastDriverError = 0L;
        return( 1 );
}






/********************* PARxCH EXECUTION CONTROL FUNCTIONS *********************/

//------------------------------------------------------------------------------
// ROUTINE: ParXchStart
// PURPOSE: Start the PARxCH A/D board acquiring data.
//------------------------------------------------------------------------------
FUNCTYPE( void ) ParXchStart( DEVHANDLE ParXchHandle ) {

        int dummy;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_START,
                        &dummy,
                        sizeof(int),
                        NULL,
                        0,
                        NULL
                       );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchStop
// PURPOSE: Stop the PARxCH A/D board from acquiring data.  Once acquisition
//          has been stopped, it is best to call ParXchClose and ParXchOpen
//          before starting again.
//------------------------------------------------------------------------------
FUNCTYPE( void ) ParXchStop( DEVHANDLE ParXchHandle ) {

        int dummy;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_STOP,
                        &dummy,
                        sizeof(int),
                        NULL,
                        0,
                        NULL
                      );
}






/**************************** PARxCH FIFO FUNCTIONS **************************/

//------------------------------------------------------------------------------
// ROUTINE: ParXchReady
// PURPOSE: Check the hardware FIFO on a PARxCH A/D board to see if any
//          acquired data is ready and waiting.  Most users will probably want
//          to use the ParXchReadData function instead since it returns 0 if
//          no data is waiting, but goes ahead and reads the data if is ready.
//
//          ParXchReady returns 1 if there is ANY data in the FIFO and 0 if the
//          FIFO is completely empty.  Typically, a return of 1 means that at
//          least one data point is ready to read.
//
//          Note that for requested sampling rates below about 40Hz, the native
//          averaging capabilities of the A/D chip are exceeded and the kernel
//          driver does some additional averaging.  In this situation,
//          ParXchReady returning 1 indicates that one native data point and not
//          a complete averaged data point is ready.  To handle this, just check
//          the return value from ParXchReadData as it will return 0 until it has
//          read a finished, completely averaged, data point.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchReady( DEVHANDLE ParXchHandle ) {

        int fifostatus;

        fifostatus = -1;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_READY,
                        NULL,
                        0,
                        &fifostatus,
                        sizeof(int),
                        NULL
                      );

        return( fifostatus );
}


//------------------------------------------------------------------------------
// ROUTINE: ParXchOverflow
// PURPOSE: Check the hardware FIFO on a PARxCH A/D board to see if it has
//          overflowed.  Once an overflow has occurred, the data in the FIFO is
//          valid and can be read.  But you must stop and restart acquisition
//          before any data after the overflow can be acquired.
//
//          ParXchOverflow returns 1 if the FIFO has overflowed and 0 otherwise.
//          Once overflow occurs, any additional data stored in the FIFO would be
//          misaligned.  So the overflow value is latched and no more data is
//          saved.  The valid data already in the FIFO can still be read out with
//          ParXchReadData.  When you are ready to continue acquiring new data,
//          just call ParXchClose, ParXchOpen, and ParXchStart.  This will reset
//          the FIFO and clear the overflow value so that data is saved and
//          ParXchOverflow again returns 0.
//
//          The hardware FIFO has 4 Meg of 4-bit words or 2 Mbytes.  It is shared
//          among 4 channels giving 0x80000 bytes/channel.  Each 24 bit sample
//          takes 3 bytes giving 0x2AAAA samples per channel in the hardware
//          FIFO.  So, it may take a long time before the FIFO overflows.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchOverflow( DEVHANDLE ParXchHandle ) {

        int fifostatus;

        fifostatus = -1;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_OVERFLOW,
                        NULL,
                        0,
                        &fifostatus,
                        sizeof(int),
                        NULL
                      );

        return( fifostatus );
}


/************************** PARxCH READ DATA FUNCTIONS ************************/

//------------------------------------------------------------------------------
// ROUTINE: ParXchReadData
// PURPOSE: Read acquired data from a PARxCH A/D board.  This returns the data
//          for the standard analog channels only.  See the following functions
//          if you want the digital (PAR8CH only) or GPS mark data too.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) ParXchReadData(
                                         DEVHANDLE     ParXchHandle,
                                         long         *Values,
                                         unsigned int  Nvalues,
                                         int          *Error
                                        ) {

        return( ParXchFullReadData( ParXchHandle,
                                    Values,
                                    Nvalues,
                                    PARXCH_REQUEST_ANALOG, // Just read analog data
                                    Error ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchReadDataWithDigital
// PURPOSE: Read acquired data from a PARxCH A/D board.  This returns the data
//          for both the standard analog channels and the simultaneously sampled
//          PAR8CH digital channels.  The PAR1CH and PAR4CH do not have
//          simultaneous digital data to return.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) ParXchReadDataWithDigital(
                                         DEVHANDLE     ParXchHandle,
                                         long         *Values,
                                         unsigned int  Nvalues,
                                         int          *Error
                                        ) {

        return( ParXchFullReadData( ParXchHandle,
                                    Values,
                                    Nvalues,
                                    PARXCH_REQUEST_DIGITAL,  // Read analog + dig
                                    Error ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchReadDataWithGpsMark
// PURPOSE: Read acquired data from a PARxCH A/D board.  This returns the data
//          for both the standard analog channels and the special GPS mark
//          channel.  The GPS mark channel is used to indicate which analog
//          sample was acquired immediately after the PPS signal marked the
//          start of a second.  The value of the GPS mark channel is then used
//          with the related PPS and serial NMEA data to timestamp the analog
//          data.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) ParXchReadDataWithGpsMark(
                                         DEVHANDLE     ParXchHandle,
                                         long         *Values,
                                         unsigned int  Nvalues,
                                         int          *Error
                                        ) {

        return( ParXchFullReadData( ParXchHandle,
                                    Values,
                                    Nvalues,
                                    PARXCH_REQUEST_GPS,    // Read analog + gps mark
                                    Error ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchReadDataAll
// PURPOSE: Read acquired data from a PARxCH A/D board.  This returns the data
//          for the standard analog channels, the PAR8CH digital channels,
//          and the special GPS mark channel described above.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) ParXchReadDataAll(
                                         DEVHANDLE     ParXchHandle,
                                         long         *Values,
                                         unsigned int  Nvalues,
                                         int          *Error
                                        ) {

        return( ParXchFullReadData( ParXchHandle,
                                    Values,
                                    Nvalues,
                                    PARXCH_REQUEST_ALL, // Read analog,digital + gps
                                    Error ) );
}


//------------------------------------------------------------------------------
// ROUTINE: ParXchFullReadData
// PURPOSE: Read the requested data from a PARxCH A/D board.  You can get any
//          combination of digital (PAR8CH only) and GPS mark data along with
//          the standard analog channels.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) ParXchFullReadData(
                                         DEVHANDLE     ParXchHandle,
                                         long         *Values,
                                         unsigned int  Nvalues,
                                         int           RequestedData,
                                         int          *Error

                                        ) {

        int Result, Health, Failure;
        long SaveReadError;
        unsigned long BytesRead, BytesToRead;

        ParXchLastDriverError = 0L;
        BytesToRead           = Nvalues * sizeof(long);


        // Verify that device driver is valid (open).

        if (ParXchHandle == BAD_DEVHANDLE) {
                ParXchLastDriverError = ERROR_SERVICE_DISABLED;
                if (Error)
                        *Error = PARXCH_ERROR_DRIVER_NOT_OPEN;
                return( 0 );
                }



        // Prepare to read by setting the mark behavior and
        // verifying the health of the PARxCH.

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_PREPARE_TO_READ,
                        &RequestedData,
                        sizeof(int),
                        &Health,
                        sizeof(int),
                        NULL
                      );

        if ( Health & HEALTH_VOLTAGE_BAD )
                Failure = PARXCH_ERROR_VOLTAGE_BAD_OFF;

        else if ( Health & HEALTH_OVERFLOW )
                Failure = PARXCH_ERROR_OVERFLOW;

        else
                Failure = PARXCH_ERROR_NONE;

        if ( Failure != PARXCH_ERROR_NONE ) {
                if (Error)
                        *Error = Failure;
                return( 0 );
                }




        // Read from device, saving result and possible driver error.

        Result = ParXchOsDriverRead(
                               ParXchHandle,    // Handle to device
                               Values,          // Buffer to receive data
                               BytesToRead,     // Number of bytes to read
                               &BytesRead       // Bytes read
                               );

        SaveReadError = ParXchOsGetLastError();

        Nvalues = (unsigned int) (BytesRead / sizeof( long ));


        // If the read failed, set the failure code in a prioritized manner.
        // Bad voltage, FIFO overflow, Driver FIFO sanity, Driver unknown.

        if (!Result) {

                Nvalues = 0;

                // Wait a while, allowing any power down to stabilize.

                ParXchOsHardSleep( 30 ); // ms


                // Query the health of the PARxCH.

                ParXchOsDriverIoctl(
                                ParXchHandle,
                                IOCTL_PARXCH_CHECK_HEALTH,
                                NULL,
                                0,
                                &Health,
                                sizeof(int),
                                NULL
                              );


                // Restore the read error.

                ParXchLastDriverError = SaveReadError;


                // Set prioritized failure code.

                if ( Health & HEALTH_VOLTAGE_BAD )
                        Failure = PARXCH_ERROR_VOLTAGE_BAD_OFF;

                else if ( Health & HEALTH_OVERFLOW )
                        Failure = PARXCH_ERROR_OVERFLOW;

                else if (ParXchLastDriverError == ERROR_CRC)
                        Failure = PARXCH_ERROR_FIFO_SANITY;

                else
                        Failure = PARXCH_ERROR_DRIVER_REQUEST_FAILED;
                }

        else
                Failure = PARXCH_ERROR_NONE;



        // Set error parameter if user provided it.

        if ( Error )
                *Error = Failure;



	return( Nvalues ); // # of longs just read or 0 for error
}





/************* PARxCH FRONT PANEL USER LED AND DIGITAL IO FUNCTIONS ************/


//------------------------------------------------------------------------------
// ROUTINE: ParXchUserLed
// PURPOSE: Turn the yellow LED on the PARxCH front panel on or off.
//          ParXchOpen must be called to initialize the PC parallel port before
//          this function will work.
//------------------------------------------------------------------------------
FUNCTYPE( void ) ParXchUserLed( DEVHANDLE ParXchHandle, int State ) {

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_USER_LED,
                        &State,
                        sizeof(int),
                        NULL,
                        0,
                        NULL
                      );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchUserIoRd
// PURPOSE: Read the 4 PARxCH digital input lines.
//          ParXchOpen must be called to initialize the PC parallel port before
//          this function will work.
//------------------------------------------------------------------------------
FUNCTYPE( void ) ParXchUserIoRd( DEVHANDLE ParXchHandle, int *Value ) {

        // Read the PARxCH user digital input.

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_DIO_READ,
                        NULL,
                        0,
                        Value,
                        sizeof(int),
                        NULL
                      );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchUserIoWr
// PURPOSE: Write the specified value to the 4 PARxCH digital output lines.
//          ParXchOpen must be called to initialize the PC parallel port before
//          this function will work.
//------------------------------------------------------------------------------
FUNCTYPE( void ) ParXchUserIoWr( DEVHANDLE ParXchHandle, int Value ) {

        // Write to the PARxCH user digital output.

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_DIO_WRITE,
                        &Value,
                        sizeof(int),
                        NULL,
                        0,
                        NULL
                      );
}







/************************** PARxCH INTERRUPT FUNCTIONS *************************/


//------------------------------------------------------------------------------
// ROUTINE: ParXchInterruptGetNumber
// PURPOSE: Ask the driver which IRQ line has been assigned to it.  A value of
//          0 means the driver was installed WITHOUT interrupts.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchInterruptGetNumber( DEVHANDLE ParXchHandle ) {

        int irqvalue;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_GET_INTR,
                        NULL,
                        0,
                        &irqvalue,
                        sizeof(int),
                        NULL
                      );

        return( irqvalue );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchInterruptEnable
// PURPOSE: Tell the PC to listen to interrupts sent over the parallel port by
//          the PARxCH A/D board.  A return of 1 means success, 0 means fail.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchInterruptEnable( DEVHANDLE ParXchHandle ) {

        int result;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_INTR_ENABLE,
                        NULL,
                        0,
                        &result,
                        sizeof(int),
                        NULL
                      );

        return( result );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchInterruptDisable
// PURPOSE: Tell the PC to ignore any interrupts sent over the parallel port by
//          the PARxCH A/D board.  A return of 1 means success, 0 means fail.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchInterruptDisable( DEVHANDLE ParXchHandle ) {

        int result;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_INTR_DISABLE,
                        NULL,
                        0,
                        &result,
                        sizeof(int),
                        NULL
                      );

        return( result );
}


//------------------------------------------------------------------------------
// ROUTINE: ParXchArmDreadyInterrupt
// PURPOSE: Arm the PARxCH board to trigger a parallel port interrupt when the
//          next acquired data point is ready.  This is a one shot interrupt.
//          After it has been sent, the PARxCH will return to passing on
//          interrupts from pin 4 (digital input line 3) of the digital I/O
//          header.  It returns 1 for success and 0 for failure.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchArmDreadyInterrupt( DEVHANDLE ParXchHandle ) {

        int result;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_ARM_DREADY_INTR,
                        NULL,
                        0,
                        &result,
                        sizeof(int),
                        NULL
                      );

        return( result );
}





/*********************** PARxCH MISCELLANEOUS FUNCTIONS **********************/


//------------------------------------------------------------------------------
// ROUTINE: ParXchVoltageGood
// PURPOSE: Check the PARxCH board voltage level.  If the voltage is good,
//          return 1.  Otherwise, 0.
//          NOTE: This function will NOT work unless ParXchOpen has been called.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchVoltageGood( DEVHANDLE ParXchHandle ) {

        int Val;

        ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_VOLTAGE_GOOD,
                        NULL,
                        0,
                        &Val,
                        sizeof(int),
                        NULL
                      );

        return( Val );
}



/************** PARxCH DRIVER TO DRIVER COMMUNICATION FUNCTIONS ***************/

//------------------------------------------------------------------------------
// ROUTINE: ParXchAttachGps
// PURPOSE: Attach the PARxCH driver to a common kernel area containing data
//          shared with the PARGPS driver so the two drivers can communicate with
//          each other.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchAttachGps(

                               DEVHANDLE ParXchHandle,
                               DEVHANDLE GpsHandle,
                               int *Error

                               ) {

        int ret, ErrorCode;
        void *GpsArea;


        // Set defaults

        ret       = 0;
        GpsArea   = NULL;
        ErrorCode = PARXCH_ERROR_GPS_NOT_ATTACHED;

        if (Error)
                *Error = ErrorCode;


#ifdef GPS_AVAILABLE

        // Ask the PARGPS driver for a pointer to its kernel area.

        if (GpsHandle == BAD_DEVHANDLE)
                return( 0 );

        ret = ParGpsGetKernelArea( GpsHandle, &GpsArea );
        if (ret == 0)
                return( 0 );



        // Now, pass this kernel area pointer to the ParXch driver.

        ret = ParXchOsDriverIoctl(
                              ParXchHandle,
                              IOCTL_PARXCH_ATTACH_GPS,
                              &GpsArea,
                              sizeof(void*),
                              &ErrorCode,
                              sizeof(int),
                              NULL
                             );


        // Allow void Error

        if (Error)
                *Error = ErrorCode;


        // Prepare return value

        if ( (ret == 1) && (ErrorCode == PARXCH_ERROR_NONE) )
                return( 1 );
        else
                return( 0 );

#else
        return( 0 );

#endif // GPS_AVAILABLE

}

//------------------------------------------------------------------------------
// ROUTINE: ParXchReleaseGps
// PURPOSE: Release the PARxCH driver from a common kernel area containing data
//          shared with the PARGPS driver so the two drivers can communicate with
//          each other.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchReleaseGps( DEVHANDLE ParXchHandle ) {

        int ret, dummy;

        ret = ParXchOsDriverIoctl(
                        ParXchHandle,
                        IOCTL_PARXCH_RELEASE_GPS,
                        &dummy,
                        sizeof(int),
                        NULL,
                        0,
                        NULL
                      );

        return( ret );
}




/**************** PARxCH SAMPLE RATE HELPER FUNCTIONS ******************/

/*
These are generally used to convert from user parameters like the
sampling rate to parameters required by the A/D in the init function.

Given a desired sampling rate in Hz and the GainLog, this helper
function computes the best TurboLog, Decimation ratio and
ExtraDecimation value using the following formulas:

     TurboLog   = MaxGainLog - GainLog
     TurboValue = (1<<TurboLog)
     Decimation = ((PARXCH_XTAL * TurboValue)/(PARXCH_FCONST * Sps)) - 1
     ExtraDecimation = 0 if sps > 50, ...

Most Sps values can be achieved by several different TurboLog and
Decimation combinations.  Selecting a combination with the largest
possible TurboLog  gives the best resolution.

If the desired sampling rate can not be achieved with the largest
TurboLog, that value is reduced until either the desired Sps or
the minimum TurboLog is reached.

Note that the ranges of the TurboLog and Decimation ratio are limited
by the ADS1210 architecture.  Only the following values are acceptable:

       TurboLog = 0 to 4 --> TurboValue of 1,2,4,8,16
             where TurboLog + GainLog <= 4

              19 <= Decimation <= 8000

This results in native sampling rate extremes of:

   Slowest SPS = (10MHz * 1)/(512  * 8001) =      2.441 Hz
   Fastest SPS = (10MHz * 16)/(512 * 20)   =  15625.000 Hz

   (Slowest SPS for TurboLog 4 is 39.056 Hz = 16 * 2.441 Hz)

To allow for slower sampling rates, additional averaging is done by
the device driver.  The number of samples included in this additional
averaging are controlled by the ExtraDecimation parameter.

Also note that because the TurboValue and Decimation ratio are integers,
the actual Sps rate resulting from the values computed by this function
may not result in exactly the requested Sps.  To get the exact sampling
rate corresponding to a particular Td pair of values, use the function
ParXchTdeToSpsGain or the formula:

     Sps = (PARXCH_XTAL * TurboValue)/(512.0 * (Decimation+1))

With a 10MHz crystal and the maximum TurboValue, some sampling rates
that can be achieved exactly are:

     50, 100, 125, 250, 500, 625, 1250, 2500, and 3125 Hz


See the Burr Brown ADS1210 spec sheet for more information on the
operation of the sigma delta modulator.

*/

//------------------------------------------------------------------------------
// ROUTINE: ParXchSpsGainToTde
// PURPOSE: Translate the desired samples per second (SPS) sampling rate and
//          gain values into the turbo, decimation and extra decimation values
//          understood by the PARxCH A/D.  See comments above.
//------------------------------------------------------------------------------

#define PARXCH_XTAL     ((double)10.0E+6)    // 10MHz
#define PARXCH_FCONST   ((double)512.0)
#define ROUNDCONST      ((double)0.499999)

FUNCTYPE( int ) ParXchSpsGainToTde(
                                    double Sps,
                                    int GainLog,
                                    int *TurboLog,
                                    int *Decimation,
                                    int *ExtraDecimation
                                    ) {

        double TurboXtalOverConst, TurboValue, NominalSps, TempSps;


        // Error check inputs.

        if ( (Sps > PARXCH_SPS_MAX) || (Sps < PARXCH_SPS_MIN) ) {
                *TurboLog        = -1;
                *Decimation      = -1;
                *ExtraDecimation = -1;
                return( PARXCH_ERROR_DECIMATION );
                }

        if ( (GainLog > PARXCH_GAIN_MAX) || (GainLog < PARXCH_GAIN_MIN) ) {
                *TurboLog        = -1;
                *Decimation      = -1;
                *ExtraDecimation = -1;
                return( PARXCH_ERROR_GAIN );
                }



        // Compute helper values.
        //
        //   Decimation = (int)( ((Turbo * Xtal ) / (Fconst * Sps)) - 1 );

        *TurboLog  = PARXCH_GAIN_MAX - GainLog;
        TurboValue = (double)(1 << *TurboLog);

        TurboXtalOverConst = TurboValue * PARXCH_XTAL / PARXCH_FCONST;


        // Compute decimation.

        *Decimation      = (int)( TurboXtalOverConst / Sps + ROUNDCONST ) - 1;
        *ExtraDecimation = PARXCH_EXTRADECIMATION_MIN;


        // Decimation is too small, ie Sps is too large.

        if ( *Decimation < PARXCH_DECIMATION_MIN )

                return( PARXCH_ERROR_DECIMATION );


        // Decimation is within valid range.

        if ( *Decimation <= PARXCH_DECIMATION_MAX )

                return( PARXCH_ERROR_NONE );


        // Extra decimation is required ...

        else {
                NominalSps = TurboXtalOverConst / (double)6250.0;  // 50.000000 Hz
                *ExtraDecimation = (int)( NominalSps / Sps + ROUNDCONST );

                // Don't let it go beyond maximum decimation

                TempSps = Sps * (double)(*ExtraDecimation);
                *Decimation  = (int)( TurboXtalOverConst / TempSps + ROUNDCONST - 1 );
                if (*Decimation > PARXCH_DECIMATION_MAX)
                        *ExtraDecimation += 1;              // round up

                NominalSps  = Sps * (double)(*ExtraDecimation);
                *Decimation = (int)( TurboXtalOverConst / NominalSps + ROUNDCONST - 1 );
                }

        return( PARXCH_ERROR_NONE );

}


//------------------------------------------------------------------------------
// ROUTINE: ParXchTdeToSpsGain
// PURPOSE: Translate the PARxCH A/D turbo, decimation and extra decimation
//          values into the samples per second (SPS) sampling rate and gain
//          values understood by the user.  See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchTdeToSpsGain(
                                    int TurboLog,
                                    int Decimation,
                                    int ExtraDecimation,
                                    double *Sps,
                                    int *GainLog
                                  ) {

        // Use this helper function to convert a particular set of
        // TurboLog, Decimation, and ExtraDecimation values into the
        // final sampling rate in Hz of the ADS1210.
        //
        // See the Burr Brown ADS1210 spec sheet for more information on
        // the operation of the sigma delta modulator.
        //


        // Error check TurboLog.

        if ( (TurboLog < PARXCH_TURBO_MIN) || (TurboLog > PARXCH_TURBO_MAX) ) {
                *GainLog = -1;
                *Sps     = -1.0;
                return( PARXCH_ERROR_TURBO );
                }

        // Compute GainLog.

        *GainLog = PARXCH_GAIN_MAX - TurboLog;


        // Error check Decimation and ExtraDecimation.

        if ( (Decimation < PARXCH_DECIMATION_MIN)           ||
             (Decimation > PARXCH_DECIMATION_MAX)           ||
             (ExtraDecimation < PARXCH_EXTRADECIMATION_MIN) ) {
                *Sps = -1.0;
                return( PARXCH_ERROR_DECIMATION );
                }

        // Compute Sps.

        *Sps = (PARXCH_XTAL*(double)(1<<TurboLog)) /
               (PARXCH_FCONST*(double)((Decimation+1)*ExtraDecimation));

        return( PARXCH_ERROR_NONE );
}




/********************* PARxCH DEFAULT XCH MODEL FUNCTION *********************/

//------------------------------------------------------------------------------
// ROUTINE: ParXchGetDefaultModel
// PURPOSE: Get the default PARxCH model information.
//          The PARxCH family of 24 bit A/D boards includes the 1, 4, and 8
//          channel PAR1CH, PAR4CH, PAR8CH models.  While the software is
//          designed to work with any of the PARxCH models, the application
//          programs often need to know which specific device is being used.
//          Typically, the specific model is given as a command line argument.
//          But for convenience, a default choice can be saved (in either the
//          registry or an environment variable depending on OS) by using the
//          setmodel utility.
//------------------------------------------------------------------------------
FUNCTYPE( int ) ParXchGetDefaultModel( char *DriverName ) {

        int XchModel;

        XchModel = ParXchOsGetDefaultModel( DriverName );

        if (XchModel == PARXCH_MODEL_UNKNOWN)
                XchModel = PARXCH_MODEL_PAR4CH;

        return( XchModel );
}






/************************ PARxCH OS DEPENDENT FUNCTIONS ***********************/

/* These functions help hide OS dependencies from the rest of the
 * library by providing a common interface.  They should never be
 * called by normal users.  The functions are repeated once for each
 * supported OS, but the SROS_xxxxx defines select only one set to be
 * compiled in to the code.
 *
 *
 * Supported OS's include
 *
 *     SROS_WINXP  Windows XP
 *     SROS_WIN2K  Windows 2000
 *     SROS_WIN9X  Windows 95, 98, and ME
 *     SROS_MSDOS  MS Dos
 *     SROS_LINUX  Linux 2.6.18-1.2798.fc6 kernel (Fedora Core 6)
 */


#define MAXENVSTR 256

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverOpen (Windows version)
// PURPOSE: Open the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE ParXchOsDriverOpen( char *DriverName ) {

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

        if (NtHandle == INVALID_HANDLE_VALUE) {
                SrHandle = BAD_DEVHANDLE;
                ParXchLastDriverError = ParXchOsGetLastError();
                }
        else {
                SrHandle = (DEVHANDLE)NtHandle;
                ParXchLastDriverError = ParXchOsGetLastError();
                }

        return( SrHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverClose (Windows version)
// PURPOSE: Close the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverClose( DEVHANDLE ParXchHandle ) {
        return( CloseHandle( ParXchHandle ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverRead (Windows version)
// PURPOSE: Read data from the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverRead(
                          DEVHANDLE      ParXchHandle,
                          void          *pValues,
                          unsigned long  BytesToRead,
                          unsigned long *pBytesRead
                        ) {

        return(
                ReadFile(
                          ParXchHandle,    // Handle to device
                          pValues,         // Buffer to receive data
                          BytesToRead,     // Number of bytes to read
                          pBytesRead,      // Bytes read
                          NULL             // NULL = wait till I/O completes
                        )
                );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverIoctl (Windows version)
// PURPOSE: Request the PARxCH device driver perform a specialized function.
//          The IoCtlCode parameter selects which of the possible functions is
//          to be done.  These include functions like starting/stopping
//          acquisition, toggling the yellow led, reading/writing to the
//          digital I/O, enabling interrupts, etc.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverIoctl(
                                DEVHANDLE      ParXchHandle,
                                unsigned long  IoCtlCode,
                                void          *pValueIn,
                                unsigned long  InSize,
                                void          *pValueOut,
                                unsigned long  OutSize,
                                unsigned long *ReturnSize
                               ) {
        int           Result;
        unsigned long BytesReturned;


        // Verify that device driver is valid (open).

        if (ParXchHandle == BAD_DEVHANDLE) {
                ParXchLastDriverError = ERROR_SERVICE_DISABLED;
                return( 0 );
                }


        // Request specialized services from device driver.

        Result = DeviceIoControl(
                       ParXchHandle,     // Handle to device
                       IoCtlCode,        // IO Control code
                       pValueIn,         // Input data to driver
                       InSize,           // Length of in data in bytes
                       pValueOut,        // Output data from driver
                       OutSize,          // Length of out data in bytes
                       &BytesReturned,   // Bytes placed in output buffer
                       NULL              // NULL = wait till I/O completes
                       );

        if (ReturnSize)
                *ReturnSize = BytesReturned;


        if (Result) {
                ParXchLastDriverError = 0L;
                return( 1 );
                }
        else {
                ParXchLastDriverError = ParXchOsGetLastError();
                return( 0 );
                }
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsGetLastError (Windows version)
// PURPOSE: Return the error message number provided by the operating system
//          for the last operation.
//------------------------------------------------------------------------------
SRLOCAL long ParXchOsGetLastError( void ) {

        return( GetLastError() );  // Provided by the OS
}


//------------------------------------------------------------------------------
// ROUTINE: ParXchOsGetDefaultModel (Windows version)
// PURPOSE: Get the default PARxCH model information indicating if the 1, 4, or
//          8 channel PARxCH model is to be used.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsGetDefaultModel( char *DriverName ) {

        int   XchModel;
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
                return( PARXCH_MODEL_UNKNOWN );



        // Get current value of Model Name

        DefaultModel[0] = '\0';
        BufferSize       = sizeof( DefaultModel );
        Code = RegQueryValueEx(
                               ServiceParmKey,
                               REGENVSTR_MODELNAME,
                               NULL,
                               &RegType,
                               DefaultModel,
                               &BufferSize
                              );

        if (Code != ERROR_SUCCESS)
                return( PARXCH_MODEL_UNKNOWN );


        // Close service parm key

        Code = RegCloseKey( ServiceParmKey );



        // Translate from result to model id

        if (!DefaultModel)
                XchModel = PARXCH_MODEL_UNKNOWN;

        else if ( strcmp( DefaultModel, PAR1CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR1CH;

        else if ( strcmp( DefaultModel, PAR4CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR4CH;

        else if ( strcmp( DefaultModel, PAR8CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR8CH;

        else
                XchModel = PARXCH_MODEL_UNKNOWN;

        return( XchModel );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsHardSleep (Windows version)
// PURPOSE: Sleep for the specified number of milliseconds.
//------------------------------------------------------------------------------
SRLOCAL void ParXchOsHardSleep( int ms ) {

        // Sleep for specifed number of milliseconds.

        if ( ms > 0 )
                Sleep( ms );
}


#elif defined( SROS_WIN9X ) || defined( SROS_MSDOS )

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverOpen (DOS version)
// PURPOSE: Open the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE ParXchOsDriverOpen( char *DriverName ) {
        return( Msdos_DrvOpen( DriverName ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverClose (DOS version)
// PURPOSE: Close the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverClose( DEVHANDLE ParXchHandle ) {
        return( Msdos_DrvClose( ParXchHandle ) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverRead (DOS version)
// PURPOSE: Read data from the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverRead(
                          DEVHANDLE      ParXchHandle,
                          void          *pValues,
                          unsigned long  BytesToRead,
                          unsigned long *pBytesRead
                         ) {
        return(
                Msdos_DrvRead(
                               ParXchHandle,    // Handle to device
                               pValues,         // Buffer to receive data
                               BytesToRead,     // Number of bytes to read
                               pBytesRead       // Bytes read
                             )
              );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverIoctl (DOS version)
// PURPOSE: Request the PARxCH device driver perform a specialized function.
//          The IoCtlCode parameter selects which of the possible functions is
//          to be done.  These include functions like starting/stopping
//          acquisition, toggling the yellow led, reading/writing to the
//          digital I/O, enabling interrupts, etc.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverIoctl(
                                DEVHANDLE      ParXchHandle,
                                unsigned long  IoCtlCode,
                                void          *pValueIn,
                                unsigned long  InSize,
                                void          *pValueOut,
                                unsigned long  OutSize,
                                unsigned long *ReturnSize
                               ) {
        int           Result;
        unsigned long BytesReturned;


        // Verify that device driver is valid (open).

        if (ParXchHandle == BAD_DEVHANDLE) {
                ParXchLastDriverError = ERROR_SERVICE_DISABLED;
                return( 0 );
                }


        // Request specialized services from device driver.

        Result = Msdos_DrvIoctl(
                       ParXchHandle,     // Handle to device
                       IoCtlCode,        // IO Control code
                       pValueIn,         // Input data to driver
                       InSize,           // Length of in data in bytes
                       pValueOut,        // Output data from driver
                       OutSize,          // Length of out data in bytes
                       &BytesReturned    // Bytes placed in output buffer
                       );

        if (ReturnSize)
                *ReturnSize = BytesReturned;


        if (Result) {
                ParXchLastDriverError = 0L;
                return( 1 );
                }
        else {
                ParXchLastDriverError = ParXchOsGetLastError();
                return( 0 );
                }
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsGetLastError (DOS version)
// PURPOSE: Return the error message number provided by the operating system
//          for the last operation.
//------------------------------------------------------------------------------
SRLOCAL long ParXchOsGetLastError( void ) {

        return( Msdos_DrvGetLastError() ); // Provided in parxchkd.c
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsGetDefaultModel (DOS version)
// PURPOSE: Get the default PARxCH model information indicating if the 1, 4, or
//          8 channel PARxCH model is to be used.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsGetDefaultModel( char *DriverName ) {

        int XchModel, i, n;
        char *EnvResult, EnvString[MAXENVSTR];

        // Prepare environment string and get resulting variable

        sprintf( EnvString, "%s_%s", DriverName, REGENVSTR_MODELNAME );
        EnvResult = getenv( EnvString );

        // Try again with all caps if original string failed

        if (!EnvResult) {
                n = strlen( EnvString );
                for ( i = 0 ; i < n ; i++ )
                        EnvString[i] = toupper( EnvString[i] );
                EnvResult = getenv( EnvString );
                }



        // Translate from environment variable to model id

        if (!EnvResult)
                XchModel = PARXCH_MODEL_UNKNOWN;
        else if ( strcmp( EnvResult, PAR1CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR1CH;
        else if ( strcmp( EnvResult, PAR4CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR4CH;
        else if ( strcmp( EnvResult, PAR8CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR8CH;
        else
                XchModel = PARXCH_MODEL_UNKNOWN;

        return( XchModel );
}

#if defined ( SROS_WIN9X )
//------------------------------------------------------------------------------
// ROUTINE: ParXchOsHardSleep (Windows 95/98 version)
// PURPOSE: Sleep for the specified number of milliseconds.
//------------------------------------------------------------------------------
SRLOCAL void ParXchOsHardSleep( int ms ) {

        // Sleep for specifed number of milliseconds.

        if ( ms > 0 )
                Sleep( ms );
}
#elif defined( SROS_MSDOS )
//------------------------------------------------------------------------------
// ROUTINE: ParXchOsHardSleep (DOS version)
// PURPOSE: Sleep for the specified number of milliseconds.
//------------------------------------------------------------------------------
SRLOCAL void ParXchOsHardSleep( int ms ) {

        struct _timeb timebuffer;
        unsigned short endms;
        time_t endsec;

        // Sleep for specifed number of milliseconds.  Since
        // no sleep function is available for MSDOS, just spin
        // in a loop wasting CPU cycles.

        if ( ms > 0 ) {

                _ftime( &timebuffer );
                endsec = timebuffer.time;
                endms  = timebuffer.millitm + ms;

                while (endms > 1000) {
                        endms  -= 1000;
                        endsec += 1;
                        }

                while (timebuffer.time < endsec)
                        _ftime( &timebuffer );

                while ( (timebuffer.time == endsec) &&
                        (timebuffer.millitm < endms) )
                        _ftime( &timebuffer );

                }
}
#endif // SROS_WIN9X or SROS_MSDOS for sleep function difference



#elif defined( SROS_LINUX )

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverOpen (Linux version)
// PURPOSE: Open the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE ParXchOsDriverOpen( char *DriverName ) {

        DEVHANDLE ParXchHandle;
        char      DeviceNameBuffer[256];

        sprintf( DeviceNameBuffer, "/dev/%s", DriverName );

        ParXchHandle = open( DeviceNameBuffer, O_RDWR );

        if (ParXchHandle < 0)
                ParXchHandle = BAD_DEVHANDLE;

        return( ParXchHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverClose (Linux version)
// PURPOSE: Close the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverClose( DEVHANDLE ParXchHandle ) {
        if ( close( ParXchHandle ) < 0 )
                return( 0 );
        else
                return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsDriverRead (Linux version)
// PURPOSE: Read data from the PARxCH device driver.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverRead(
                          DEVHANDLE      ParXchHandle,
                          void          *pValues,
                          unsigned long  BytesToRead,
                          unsigned long *pBytesRead
                         ) {
        long readreturn;

        readreturn = read( ParXchHandle, pValues, BytesToRead );

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
// ROUTINE: ParXchOsDriverIoctl (Linux version)
// PURPOSE: Request the PARxCH device driver perform a specialized function.
//          The IoCtlCode parameter selects which of the possible functions is
//          to be done.  These include functions like starting/stopping
//          acquisition, toggling the yellow led, reading/writing to the
//          digital I/O, enabling interrupts, etc.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsDriverIoctl(
                                DEVHANDLE      ParXchHandle,
                                unsigned long  IoCtlCode,
                                void          *pValueIn,
                                unsigned long  InSize,
                                void          *pValueOut,
                                unsigned long  OutSize,
                                unsigned long *ReturnSize
                               ) {
        int code;
	IRP IoctlData;
        int           Result;
        unsigned long BytesReturned;


        // Verify that device driver is valid (open).

        if (ParXchHandle == BAD_DEVHANDLE) {
                ParXchLastDriverError = ERROR_SERVICE_DISABLED;
                return( 0 );
                }



        // The ioctl function just takes one buffer.  On the way in,
        // it contains the input data and on the way out, it contains
        // the output data.  So, make sure we are using the largest of
        // the two and that it contains the input data.  The size of the
        // data must be the size defined for that IO control code in
        // parxchkd.h.

        IoctlData.Command       = IoCtlCode;
        IoctlData.InBuffer      = pValueIn;
        IoctlData.InSize        = InSize;
        IoctlData.OutBuffer     = pValueOut;
        IoctlData.OutSize       = OutSize;
        IoctlData.ReturnedBytes = 0;
        IoctlData.DataMethod    = METHOD_BUFFERED;
        IoctlData.ErrorCode     = 0;
        IoctlData.UserIrp       =(void *)&IoctlData;


        // Check in parxchkd.h for which control codes do not
        // use the more common buffered method of passing data

//	if ((IoCtlCode == IOCTL_??? ) {
//                IoctlData.DataMethod = METHOD_OUT_DIRECT;
//                }


        // Request specialized services from device driver.

        code = ioctl( ParXchHandle, IoCtlCode, &IoctlData );

	if (ReturnSize)
                *ReturnSize = IoctlData.ReturnedBytes;



        if (code >= 0) {
                ParXchLastDriverError = 0L;
                return( 1 );
                }
        else {
                ParXchLastDriverError = ParXchOsGetLastError();
                return( 0 );
                }

}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsGetLastError (Linux version)
// PURPOSE: Return the error message number provided by the operating system
//          for the last operation.
//------------------------------------------------------------------------------
SRLOCAL long ParXchOsGetLastError( void ) {

        return( (long)(-errno) );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsGetDefaultModel (Linux version)
// PURPOSE: Get the default PARxCH model information indicating if the 1, 4, or
//          8 channel PARxCH model is to be used.
//------------------------------------------------------------------------------
SRLOCAL int ParXchOsGetDefaultModel( char *DriverName ) {

        int XchModel;
        char *EnvResult, EnvString[MAXENVSTR];

        // Prepare environment string and get resulting variable

        sprintf( EnvString, "%s_%s", DriverName, REGENVSTR_MODELNAME );
        EnvResult = getenv( EnvString );


        // Translate from environment variable to model id

        if (!EnvResult)
                XchModel = PARXCH_MODEL_UNKNOWN;
        else if ( strcmp( EnvResult, PAR1CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR1CH;
        else if ( strcmp( EnvResult, PAR4CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR4CH;
        else if ( strcmp( EnvResult, PAR8CH_MODELNAME ) == 0 )
                XchModel = PARXCH_MODEL_PAR8CH;
        else
                XchModel = PARXCH_MODEL_UNKNOWN;

        return( XchModel );
}

//------------------------------------------------------------------------------
// ROUTINE: ParXchOsHardSleep (Linux version)
// PURPOSE: Sleep for the specified number of milliseconds.
//------------------------------------------------------------------------------
SRLOCAL void ParXchOsHardSleep( int ms ) {

        // Sleep for specifed number of milliseconds.
        // Linux nanosleep takes structure with seconds and nanoseconds.

        struct timespec twait, tleft;

        if ( ms > 0 ) {

                tleft.tv_sec  = tleft.tv_nsec = 0;
                twait.tv_sec  = (long)(ms / 1000);
                twait.tv_nsec = (ms - (twait.tv_sec*1000)) * 1000000;

                nanosleep( &twait, &tleft );

                }
}

#endif // SROS_xxxxx   End of OS dependent functions


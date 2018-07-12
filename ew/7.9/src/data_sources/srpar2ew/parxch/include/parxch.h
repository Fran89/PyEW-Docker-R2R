/* FILE: parxch.h             Copyright (c), Symmetric Research, 2003-2007

SR PARxCH C user library include file.

Study this file to see the functions that can be called from user level
programs and applications to control the PARxCH family of A/D devices.

NOTE: To compile properly, two macros must be defined.  One specifies
the target operating system and the other specifies the target A/D device.
See below for more details on the SROS_xxxxx and SRA2D_PARxxx macros.

*/





/* FUNCTION TYPE DEFINES:

Most of the SR software links in a static copy of the PARxCH library.
For static linking, function and global variable declarations are
easy and standard across operating systems.  The following defines
default to static linking if not otherwise specified.

Supporting Windows dynamic DLL libraries introduces complications.
The following ifdef defines allow for one PARxCH library source to
service either statically linked or DLL applications.

Linux shared object libraries are also supported but do not require any
compile time specificatons beyond static.

*/

#if defined( SROS_MSDOS )  // DOS requires simple declarations
#define FUNCTYPE( type )        type
#define VARTYPE( type )         type

#elif defined( _WIN_DLLEXPORT )
#define FUNCTYPE( type )        __declspec( dllexport ) type __stdcall
#define VARTYPE( type )         __declspec( dllexport ) type

#elif defined( _WIN_DLLIMPORT )
#define FUNCTYPE( type )        __declspec( dllimport ) type __stdcall
#define VARTYPE( type )         __declspec( dllimport ) type

#else           // default to simple declarations for static linking
#define FUNCTYPE( type )        type
#define VARTYPE( type )         type

#endif



/* DEVICE HANDLE AND MISCELLANEOUS OS DEPENDENT DEFINES:

The various OS's use different data types to keep track of the
current device driver.  These defines helps hide this difference
from the rest of the code.  The allowed SROS_xxxxx options are:

SROS_WINXP  Windows XP
SROS_WIN2K  Windows 2000
SROS_WIN9X  Windows 95 or 98 or ME
SROS_MSDOS  Dos
SROS_LINUX  Linux kernel 2.6.18-1.2798.fc6 = Fedora Core 6

*/

#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WIN9X )
#define DEVHANDLE void *

#elif defined( SROS_MSDOS )
#define DEVHANDLE unsigned int

#elif defined( SROS_LINUX )
#define DEVHANDLE int

#else
#pragma message( "COMPILE ERROR: SROS_xxxxx MUST BE DEFINED !! " )

#endif  // SROS_WINXP  SROS_WIN2K  SROS_WIN9X  SROS_MSDOS  SROS_LINUX

#define BAD_DEVHANDLE ((DEVHANDLE)(-1))








/* RELEASE REV:

This function allows the user to determine the library rev at
runtime.  The rev is an integer.  A number like 123 would be referred
to as rev "1.23".

*/

FUNCTYPE( int ) ParXchGetRev( int *Rev );





/* DEFAULT XCH MODEL:

The PARxCH family of 24 bit A/D boards includes the 1, 4, and 8 channel
PAR1CH, PAR4CH, PAR8CH models.  While the software is designed to work
with any of the PARxCH models, the application programs often need to
know which model is being used.  Typically, the specific model is given
as a command line argument.  But for convenience, a default choice can
be saved (in either the registry or an environment variable depending on
OS) by using the setmodel utility.  This function retrieves this default
model information.

*/

FUNCTYPE( int ) ParXchGetDefaultModel( char *DriverName );




/* SIMPLE PARxCH CONTROL/EXECUTION DEFINES AND FUNCTIONS:

The following "easy to use" defines and functions provide a basic
level of control over the PARxCH.  Many applications will not need
anything else.  The basic pattern of usage is:

1) Open and initialize the PARxCH, setting the sampling rate
2) Start conversion
3) Call ReadData to copy ready data from the PARxCH to the PC
4) Loop to continue getting more data
5) When done, stop conversion and close the PARxCH

When using the PARxCH with a PARGPS unit for accurate timing, the basic
pattern of usage is similar but with a few additional steps:

1)  Open and initialize both the PARxCH and the PARGPS
1A) Attach the PARGPS to the PARxCH so their drivers can communicate
2)  Start the PARGPS and then the PARxCH
3)  Call the PARxCH ReadDataWithGpsMark to get analog data and the PARGPS
    ReadPpsData and ReadSerialData to get timing data
4)  Loop to continue getting more data
5)  When done, stop both the PARGPS and PARxCH
5A) Release the PARGPS from the PARxCH and close both drivers

See pargps.h for more information about opening, closing, and reading
timing data from the PARGPS device.


For more detailed control of the PARxCH see the ADVANCED defines and
functions below.

Some of the abbreviations are:

       EPP    enhanced parallel port
       BPP    bidirectional parallel port
       ECP    extended capabilities port
       SPS    samples per second

Note that the function ParXchOpen returns a 1 or 0 for general
success or failure.  It also fills in an integer error code, which on
return has more information about what may have gone right or wrong.
The PARXCH_ERROR ... defines provide a mapping from these error codes
to strings.

*/



// The PARxCH family of A/D devices includes the several members.  The
// following defines are used to identify the specific A/D model.

#define PARXCH_MODEL_UNKNOWN  0x00
#define PARXCH_MODEL_PAR1CH   0x01
#define PARXCH_MODEL_PAR4CH   0x04
#define PARXCH_MODEL_PAR8CH   0x08



// PARxCH number of channels ...

// Several different types of information can be acquired from the
// PARxCH devices.  These include analog, digital, GPS and counter
// channels.  The number of channels for each type of PARxCH, defined
// below, control what data is saved to the output files.  An additional
// set of defines indicate the channel counts for plotting purposes as
// this may be different than for saving.  For example, the PAR8CH
// digital information is stored as 1 channel, but is plotted as 4
// traces.


// Channel counts for the PAR8CH

#define PAR8CH_ANALOG_CHANNELS         8
#define PAR8CH_DIGITAL_CHANNELS        1
#define PAR8CH_GPS_CHANNELS            1
#define PAR8CH_COUNTER_CHANNELS        1

#define PAR8CH_ANAPLOT_CHANNELS        8
#define PAR8CH_DIGPLOT_CHANNELS        4
#define PAR8CH_GPSPLOT_CHANNELS        1
#define PAR8CH_CNTPLOT_CHANNELS        0

#define PAR8CH_MODELNAME               "PAR8CH"



// Channel counts for the PAR4CH

#define PAR4CH_ANALOG_CHANNELS         4
#define PAR4CH_DIGITAL_CHANNELS        0
#define PAR4CH_GPS_CHANNELS            1
#define PAR4CH_COUNTER_CHANNELS        0

#define PAR4CH_ANAPLOT_CHANNELS        4
#define PAR4CH_DIGPLOT_CHANNELS        0
#define PAR4CH_GPSPLOT_CHANNELS        1
#define PAR4CH_CNTPLOT_CHANNELS        0

#define PAR4CH_MODELNAME               "PAR4CH"



// Channel counts for the PAR1CH

#define PAR1CH_ANALOG_CHANNELS         1
#define PAR1CH_DIGITAL_CHANNELS        0
#define PAR1CH_GPS_CHANNELS            1
#define PAR1CH_COUNTER_CHANNELS        0

#define PAR1CH_ANAPLOT_CHANNELS        1
#define PAR1CH_DIGPLOT_CHANNELS        0
#define PAR1CH_GPSPLOT_CHANNELS        1
#define PAR1CH_CNTPLOT_CHANNELS        0

#define PAR1CH_MODELNAME               "PAR1CH"



// Maximum channel counts for all PARxCH

#define PARXCH_MAX_ANALOG_CHANNELS     8
#define PARXCH_MAX_DIGITAL_CHANNELS    1
#define PARXCH_MAX_GPS_CHANNELS        1
#define PARXCH_MAX_COUNTER_CHANNELS    1

#define PARXCH_MAX_ANAPLOT_CHANNELS    8
#define PARXCH_MAX_DIGPLOT_CHANNELS    4
#define PARXCH_MAX_GPSPLOT_CHANNELS    1
#define PARXCH_MAX_CNTPLOT_CHANNELS    0



// Set the general channel count defines and a model define to match the
// specific PARxCH A/D requested at compile time.  The requested A/D is
// identified by the SRA2D_PARxxx macro.  The allowed values are:
//
// SRA2D_PAR8CH  for the 8 channel A/D
// SRA2D_PAR4CH  for the 4 channel A/D
// SRA2D_PAR1CH  for the 1 channel A/D
// SRA2D_PARANY  for any device in the PARxCH family (used for common driver)
//


#if defined( SRA2D_PAR8CH )

#define PARXCH_MODEL               PARXCH_MODEL_PAR8CH
#define PARXCH_MODELNAME           PAR8CH_MODELNAME

#define PARXCH_ANALOG_CHANNELS     PAR8CH_ANALOG_CHANNELS
#define PARXCH_DIGITAL_CHANNELS    PAR8CH_DIGITAL_CHANNELS
#define PARXCH_GPS_CHANNELS        PAR8CH_GPS_CHANNELS
#define PARXCH_COUNTER_CHANNELS    PAR8CH_COUNTER_CHANNELS

#define PARXCH_ANAPLOT_CHANNELS    PAR8CH_ANAPLOT_CHANNELS
#define PARXCH_DIGPLOT_CHANNELS    PAR8CH_DIGPLOT_CHANNELS
#define PARXCH_GPSPLOT_CHANNELS    PAR8CH_GPSPLOT_CHANNELS
#define PARXCH_CNTPLOT_CHANNELS    PAR8CH_CNTPLOT_CHANNELS

#elif defined( SRA2D_PAR4CH )

#define PARXCH_MODEL               PARXCH_MODEL_PAR4CH
#define PARXCH_MODELNAME           PAR4CH_MODELNAME


#define PARXCH_ANALOG_CHANNELS     PAR4CH_ANALOG_CHANNELS
#define PARXCH_DIGITAL_CHANNELS    PAR4CH_DIGITAL_CHANNELS
#define PARXCH_GPS_CHANNELS        PAR4CH_GPS_CHANNELS
#define PARXCH_COUNTER_CHANNELS    PAR4CH_COUNTER_CHANNELS

#define PARXCH_ANAPLOT_CHANNELS    PAR4CH_ANAPLOT_CHANNELS
#define PARXCH_DIGPLOT_CHANNELS    PAR4CH_DIGPLOT_CHANNELS
#define PARXCH_GPSPLOT_CHANNELS    PAR4CH_GPSPLOT_CHANNELS
#define PARXCH_CNTPLOT_CHANNELS    PAR4CH_CNTPLOT_CHANNELS


#elif defined( SRA2D_PAR1CH )

#define PARXCH_MODEL               PARXCH_MODEL_PAR1CH
#define PARXCH_MODELNAME           PAR1CH_MODELNAME


#define PARXCH_ANALOG_CHANNELS     PAR1CH_ANALOG_CHANNELS
#define PARXCH_DIGITAL_CHANNELS    PAR1CH_DIGITAL_CHANNELS
#define PARXCH_GPS_CHANNELS        PAR1CH_GPS_CHANNELS
#define PARXCH_COUNTER_CHANNELS    PAR1CH_COUNTER_CHANNELS

#define PARXCH_ANAPLOT_CHANNELS    PAR1CH_ANAPLOT_CHANNELS
#define PARXCH_DIGPLOT_CHANNELS    PAR1CH_DIGPLOT_CHANNELS
#define PARXCH_GPSPLOT_CHANNELS    PAR1CH_GPSPLOT_CHANNELS
#define PARXCH_CNTPLOT_CHANNELS    PAR1CH_CNTPLOT_CHANNELS


#elif defined( SRA2D_PARANY )  // Works for any PARxCH

#define PARXCH_MODEL               PARXCH_MODEL_UNKNOWN
#define PARXCH_MODELNAME           "PARxCH"


#else
#pragma message( "COMPILE ERROR: SRA2D_PARxxx MUST BE DEFINED !! " )

#endif // SRA2D_PARxxx







// Driver name and parallel port mode defines ...

#define PARXCH_0                                "SrParXch0"     // New style names
#define PARXCH_1                                "SrParXch1"
#define PARXCH_2                                "SrParXch2"

#define PARXCH_IRQ_7                            7
#define PARXCH_IRQ_5                            5
#define PARXCH_IRQ_17                          17

#define PARXCH_0x378                            "SrParXch378"   // Old style names
#define PARXCH_IRQ_0x378                        7

#define PARXCH_0x278                            "SrParXch278"
#define PARXCH_IRQ_0x278                        5


#define PARXCH_PORT_MODE_UNKNOWN               -1
#define PARXCH_PORT_MODE_BPP                    0
#define PARXCH_PORT_MODE_EPP                    1
#define PARXCH_PORT_MODE_ECP_BPP                2
#define PARXCH_PORT_MODE_ECP_EPP                3


// Minimum and maximum Sps values ...

#define PARXCH_SPS_MIN                          0.00001   // Hz
#define PARXCH_SPS_MAX                          15625.0   // Hz
#define PARXCH_SPS_DEFAULT                        100.0   // Hz


// Error code return values ...

#define PARXCH_ERROR_NONE                           0
#define PARXCH_ERROR_PORT_ADDRESS                   1
#define PARXCH_ERROR_PORT_MODE                      2
#define PARXCH_ERROR_DATA_FORMAT                    3
#define PARXCH_ERROR_CAL_MODE                       4
#define PARXCH_ERROR_GAIN                           5
#define PARXCH_ERROR_TURBO                          6
#define PARXCH_ERROR_GAIN_TURBO_PRODUCT             7
#define PARXCH_ERROR_DECIMATION                     8
#define PARXCH_ERROR_DRIVER_NOT_OPEN                9
#define PARXCH_ERROR_DRIVER_REQUEST_FAILED          10
#define PARXCH_ERROR_VOLTAGE_BAD_ON                 11
#define PARXCH_ERROR_VOLTAGE_BAD_OFF                12
#define PARXCH_ERROR_NOT_UNLOCKED                   13
#define PARXCH_ERROR_ADS1210_CR_READBACK            14
#define PARXCH_ERROR_FIFO_SANITY                    15
#define PARXCH_ERROR_OVERFLOW                       16
#define PARXCH_ERROR_KEYPRESS                       17
#define PARXCH_ERROR_GPS_NOT_AVAILABLE              18
#define PARXCH_ERROR_GPS_NOT_ATTACHED               19
#define PARXCH_ERROR_GPS_VERSION_MISMATCH           20
#define PARXCH_ERROR_DIGITAL_NOT_AVAILABLE          21
#define PARXCH_ERROR_ATOD_UNKNOWN                   22
#define PARXCH_ERROR_NO_INTERRUPT                   23
#define PARXCH_ERROR_MAX                            24


#define PARXCH_ERROR_MSG_NONE                       "NONE"
#define PARXCH_ERROR_MSG_PORT_ADDRESS               "PORT ADDRESS"
#define PARXCH_ERROR_MSG_PORT_MODE                  "PORT MODE"
#define PARXCH_ERROR_MSG_DATA_FORMAT                "INVALID DATA FORMAT"
#define PARXCH_ERROR_MSG_CAL_MODE                   "INVALID CALIBRATION"
#define PARXCH_ERROR_MSG_GAIN                       "INVALID GAIN"
#define PARXCH_ERROR_MSG_TURBO                      "INVALID TURBO"
#define PARXCH_ERROR_MSG_GAIN_TURBO_PRODUCT         "INVALID GAIN*TURBO"
#define PARXCH_ERROR_MSG_DECIMATION                 "INVALID DECIMATION"
#define PARXCH_ERROR_MSG_DRIVER_NOT_OPEN            "NO DEVICE DRIVER"
#define PARXCH_ERROR_MSG_DRIVER_REQUEST_FAILED      "DRIVER REQUEST"
#define PARXCH_ERROR_MSG_VOLTAGE_BAD_ON             "VOLTAGE STILL ON"
#define PARXCH_ERROR_MSG_VOLTAGE_BAD_OFF            "VOLTAGE BAD/OFF"
#define PARXCH_ERROR_MSG_NOT_UNLOCKED               "NO RESPONSE"
#define PARXCH_ERROR_MSG_ADS1210_CR_READBACK        "A/D READBACK"
#define PARXCH_ERROR_MSG_FIFO_SANITY                "FIFO SANITY"
#define PARXCH_ERROR_MSG_OVERFLOW                   "OVERFLOW"
#define PARXCH_ERROR_MSG_KEYPRESS                   "KEYPRESS"
#define PARXCH_ERROR_MSG_GPS_NOT_AVAILABLE          "PARGPS NOT AVAILABLE"
#define PARXCH_ERROR_MSG_GPS_NOT_ATTACHED           "PARGPS AREA NOT ATTACHED"
#define PARXCH_ERROR_MSG_GPS_VERSION_MISMATCH       "PARGPS VERSION MISMATCH"
#define PARXCH_ERROR_MSG_DIGITAL_NOT_AVAILABLE      "DIGITAL NOT AVAILABLE"
#define PARXCH_ERROR_MSG_ATOD_UNKNOWN               "ATOD NOT PAR1CH,4CH,8CH"
#define PARXCH_ERROR_MSG_NO_INTERRUPT               "NO INTERRUPT ASSIGNED"
#define PARXCH_ERROR_MSG_MAX                         NULL

extern VARTYPE( char ) *PARXCH_ERROR_MSG[];

        // This array maps error codes to message strings.  See parxch.c
        // for the actual allocation of PARXCH_ERROR_MSG[].
        //
        // All of the library functions return error codes.  You may wish
        // to map them to message strings for use with printf.
        //
        // = {
        //        0  PARXCH_ERROR_MSG_NONE,
        //        1  PARXCH_ERROR_MSG_PORT_ADDRESS,
        //        2  PARXCH_ERROR_MSG_PORT_MODE,
        //        3  PARXCH_ERROR_MSG_DATA_FORMAT,
        //        4  PARXCH_ERROR_MSG_CAL_MODE,
        //        5  PARXCH_ERROR_MSG_GAIN,
        //        6  PARXCH_ERROR_MSG_TURBO,
        //        7  PARXCH_ERROR_MSG_GAIN_TURBO_PRODUCT,
        //        8  PARXCH_ERROR_MSG_DECIMATION,
        //        9  PARXCH_ERROR_MSG_DRIVER_NOT_OPEN,
        //       10  PARXCH_ERROR_MSG_DRIVER_REQUEST_FAILED,
        //       11  PARXCH_ERROR_MSG_VOLTAGE_BAD_ON,
        //       12  PARXCH_ERROR_MSG_VOLTAGE_BAD_OFF,
        //       13  PARXCH_ERROR_MSG_NOT_UNLOCKED
        //       14  PARXCH_ERROR_MSG_ADS1210_CR_READBACK,
        //       15  PARXCH_ERROR_MSG_FIFO_SANITY,
        //       16  PARXCH_ERROR_MSG_OVERFLOW
        //       17  PARXCH_ERROR_MSG_KEYPRESS
        //       18  PARXCH_ERROR_MSG_GPS_NOT_AVAILABLE
        //       19  PARXCH_ERROR_MSG_GPS_NOT_ATTACHED
        //       20  PARXCH_ERROR_MSG_GPS_VERSION_MISMATCH
        //       21  PARXCH_ERROR_DIGITAL_NOT_AVAILABLE
        //       22  PARXCH_ERROR_ATOD_UNKNOWN
        //       23  PARXCH_ERROR_MSG_NO_INTERRUPT
        //       24  PARXCH_ERROR_MSG_MAX
        //
        //   };




// Simple PARxCH functions ...

FUNCTYPE( DEVHANDLE ) Par8chOpen(
                                  char *DriverName,
                                  int PortMode,
                                  double Sps,

                                  double *ActualSps,
                                  int *Error
                                  );

FUNCTYPE( DEVHANDLE ) Par4chOpen(
                                  char *DriverName,
                                  int PortMode,
                                  double Sps,

                                  double *ActualSps,
                                  int *Error
                                  );


FUNCTYPE( DEVHANDLE ) Par1chOpen(
                                  char *DriverName,
                                  int PortMode,
                                  double Sps,

                                  double *ActualSps,
                                  int *Error
                                  );




FUNCTYPE( DEVHANDLE ) ParXchOpen(
                                  char *DriverName,
                                  int XchModel,
                                  int PortMode,
                                  double Sps,

                                  double *ActualSps,
                                  int *Error
                                  );


FUNCTYPE( int ) ParXchClose( DEVHANDLE ParXchHandle );

FUNCTYPE( void ) ParXchStart( DEVHANDLE ParXchHandle );
FUNCTYPE( void ) ParXchStop( DEVHANDLE ParXchHandle );

FUNCTYPE( int ) ParXchReady( DEVHANDLE ParXchHandle );
FUNCTYPE( int ) ParXchOverflow( DEVHANDLE ParXchHandle );

FUNCTYPE( unsigned int ) ParXchReadData(

                                DEVHANDLE ParXchHandle,
                                long *Values,
                                unsigned int Nvalues,
                                int *Error

                                );

FUNCTYPE( unsigned int ) ParXchReadDataWithDigital(

                                DEVHANDLE ParXchHandle,
                                long *Values,
                                unsigned int Nvalues,
                                int *Error

                                );

FUNCTYPE( unsigned int ) ParXchReadDataWithGpsMark(

                                DEVHANDLE ParXchHandle,
                                long *Values,
                                unsigned int Nvalues,
                                int *Error

                                );

FUNCTYPE( unsigned int ) ParXchReadDataAll(

                                DEVHANDLE ParXchHandle,
                                long *Values,
                                unsigned int Nvalues,
                                int *Error

                                );



FUNCTYPE( int ) ParXchAttachGps(
                                DEVHANDLE ParXchHandle,
                                DEVHANDLE GpsHandle,
                                int *Error
                                );

FUNCTYPE( int ) ParXchReleaseGps( DEVHANDLE ParXchHandle );






/* PARxCH USER DIGITAL IO FUNCTIONS:

These functions allow programs to communicate with the DB15 digital
IO connector on the right hand side of the PARxCH front panel.

*/

FUNCTYPE( void ) ParXchUserIoRd( DEVHANDLE ParXchHandle, int *Value );
FUNCTYPE( void ) ParXchUserIoWr( DEVHANDLE ParXchHandle, int Value );

FUNCTYPE( void ) ParXchUserLed( DEVHANDLE ParXchHandle, int State );












// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY


/* ADVANCED PARxCH CONTROL/EXECUTION DEFINES AND FUNCTIONS:

These defines and functions allow a more detailed level of control
over PARxCH execution.  They are intended for users who are familiar
with the ADS1210 command register bit fields and wish to program them
directly.


Use these defines with ParXchFullOpen to initialize the PARxCH.

Several of these constants map directly into ADS1210 command register
bit setings.  Don't change their values !!

Some of the abbreviations are:

       DF               ADS1210 data format
       CAL_MODE         ADS1210 calibration mode
       GAIN             ADS1210 preamp gain
       TURBO            ADS1210 modulator turbo setting
       DECIMATION       ADS1210 modulator decimation ratio


These functions control the operation of the PARxCH.

Use the defines above and helper functions for setting ParXchFullOpen
parameters.

Note that PARxCH user digital IO will not work until the device has
been initialized with ParXchOpen.

*/


// PARxCH ADS1210 command register defines ...

#define PARXCH_DF_SIGNED                        0
#define PARXCH_DF_OFFSET                        1

#define PARXCH_CAL_NORMAL                       0
#define PARXCH_CAL_SELF                         1
#define PARXCH_CAL_SYSTEM_OFFSET                2
#define PARXCH_CAL_SYSTEM_FS                    3
#define PARXCH_CAL_PSEUDO                       4
#define PARXCH_CAL_BACKGROUND                   5

#define PARXCH_CAL_MIN                          0
#define PARXCH_CAL_MAX                          5

#define PARXCH_GAIN_1                           0
#define PARXCH_GAIN_2                           1
#define PARXCH_GAIN_4                           2
#define PARXCH_GAIN_8                           3
#define PARXCH_GAIN_16                          4

#define PARXCH_GAIN_MIN                         0
#define PARXCH_GAIN_MAX                         4

#define PARXCH_TURBO_1                          0
#define PARXCH_TURBO_2                          1
#define PARXCH_TURBO_4                          2
#define PARXCH_TURBO_8                          3
#define PARXCH_TURBO_16                         4

#define PARXCH_TURBO_MIN                        0
#define PARXCH_TURBO_MAX                        4

#define PARXCH_GAINTURBO_MIN                    0
#define PARXCH_GAINTURBO_MAX                    4

#define PARXCH_DECIMATION_MIN                   19
#define PARXCH_DECIMATION_500Hz_T1              38
#define PARXCH_DECIMATION_500Hz_T16             624
#define PARXCH_DECIMATION_100Hz_T1              194
#define PARXCH_DECIMATION_100Hz_T16             3124
#define PARXCH_DECIMATION_50Hz_T1               390
#define PARXCH_DECIMATION_50Hz_T16              6249
#define PARXCH_DECIMATION_MAX                   8000

#define PARXCH_EXTRADECIMATION_MIN              1

#define PARXCH_UNUSED                           -1



// Driver read request defines ...

#define PARXCH_REQUEST_ANALOG   0x0
#define PARXCH_REQUEST_GPS      0x1
#define PARXCH_REQUEST_DIGITAL  0x2
#define PARXCH_REQUEST_COUNTER  0x4
#define PARXCH_REQUEST_ALL     (PARXCH_REQUEST_ANALOG  | PARXCH_REQUEST_GPS |     \
                                PARXCH_REQUEST_DIGITAL | PARXCH_REQUEST_COUNTER)


// Advanced PARxCH functions ...

FUNCTYPE( DEVHANDLE ) ParXchFullOpen(

                                char *DriverName,
                                int XchModel,
                                int PortMode,

                                int Df,
                                int GainLog,
                                int TurboLog,
                                int Decimation,
                                int ExtraDecimation,
                                int Unused,

                                long *ADS1210_CrValue,
                                int *Error

                                );


FUNCTYPE( unsigned int ) ParXchFullReadData(

                                DEVHANDLE ParXchHandle,
                                long *Values,
                                unsigned int Nvalues,
                                int RequestedData,
                                int *Error

                                );



FUNCTYPE( int ) ParXchSpsGainToTde(

                                double Sps,
                                int GainLog,

                                int *TurboLog,
                                int *Decimation,
                                int *ExtraDecimation

                                );


FUNCTYPE( int ) ParXchTdeToSpsGain(

                                int TurboLog,
                                int Decimation,
                                int ExtraDecimation,

                                double *Sps,
                                int *GainLog

                                );



/* PARxCH PC INTERRUPT FUNCTIONS:

These functions check the interrupt number assigned to the driver and,
if greater than 0, allow programs to enable and disable the PC interrupt.
GetNumber returns the interrupt number while Enable and Disable return
1 for success and 0 for failure.

*/

FUNCTYPE( int ) ParXchInterruptGetNumber( DEVHANDLE ParXchHandle );

FUNCTYPE( int ) ParXchInterruptEnable( DEVHANDLE ParXchHandle );
FUNCTYPE( int ) ParXchInterruptDisable( DEVHANDLE ParXchHandle );





/* PARxCH ARM DREADY INTERRUPT FUNCTION:

This function arms the PARxCH device to trigger a parallel port
interrupt when the next acquired data point is ready.  This is a one
shot interrupt.  After it has been sent, the PARxCH will return to
passing on interrupts from the pin 3 of the digital I/O header.  It
returns 1 for success and 0 for failure.

*/

FUNCTYPE( int ) ParXchArmDreadyInterrupt( DEVHANDLE ParXchHandle );




/* PARxCH VOLTAGE CHECK FUNCTION:

This function checks the PARxCH board voltage level.  If the voltage is
good, this function returns 1.  Otherwise, 0.

*/

FUNCTYPE( int ) ParXchVoltageGood( DEVHANDLE ParXchHandle );


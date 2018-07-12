/* FILE: pargps.h             Copyright (c), Symmetric Research, 2003-2008

SR PARGPS user library include file.

Study this file to see the functions that can be called from user
level programs and applications to control the PARGPS.

Change history:

  2008-10-15  WCT  Updated ParGpsRequestNmeaInfo function
  2008-06-05  WCT  Updated NMEA comments
  2007-12-05  WCT  Added Garmin GPS 18 LVC support
  2007-01-08  WCT  Added new SrParGps0 style driver name support
  2006-09-15  WCT  Added ParGpsFlushSerial to support error recovery
  2006-01-30  WCT  Added TsReadNext and passed GpsModel to TsProcessPps
  2005-08-10  WCT  Increased MAX_NMEA_FIELDS to 22 (for GSV w/ ID+19+CHK+1extra)
                   Added ParGpsFullOpen to support other NMEA receivers + PCTIME

*/



/* FUNCTION TYPE DEFINES:

Most of the SR software links in a static copy of the PARGPS library.
For static linking, function declarations are easy and standard
across operating systems.  The following defines default to static
linking if not otherwise specified.

Supporting Windows dynamic DLL libraries introduces complications.
The following ifdef defines allow for one PARGPS library source to
service either statically linked or DLL applications.

Linux shared object libraries are also supported but do not require any
compile time specificatons beyond static.

*/

#if defined( _WIN_DLLEXPORT )
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
SROS_WINNT  Windows NT 4.0
SROS_LINUX  Linux kernel 2.6.18-1.2798.fc6 = Fedora Core 6

*/

#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )
#define DEVHANDLE void *

#elif defined( SROS_LINUX )
#define DEVHANDLE int

#else
#pragma message( "COMPILE ERROR: SROS_xxxxx MUST BE DEFINED !! " )

#endif  // SROS_WINXP  SROS_WIN2K  SROS_WINNT  SROS_LINUX

#define BAD_DEVHANDLE ((DEVHANDLE)(-1))








/* PARGPS DEFINES:

*/

// PARGPS driver name defines ...

#define PARGPS_0        "SrParGps0"
#define PARGPS_1        "SrParGps1"
#define PARGPS_2        "SrParGps2"
#define PARGPS_NAME     "SrParGps"


// Serial port numbers

#define SERIAL_PORT_1           1  // This is Com1 or ttyS0
#define SERIAL_PORT_2           2  // This is Com2 or ttyS1


// PARGPS interrupt modes

#define INTERRUPT_PPS_ONLY     0
#define INTERRUPT_ALTERNATING  1


// GPS models
//   GARMIN  assumes the Garmin GPS 18 LVC
//   TRIMBLE assumes the Trimble Ace III
//   PCTIME  assumes time taken from PC
//   ONCORE  assumes the Motorola Oncore GT+

#define GPSMODEL_UNKNOWN      -1
#define GPSMODEL_NONE          0
#define GPSMODEL_TRIMBLE       1
#define GPSMODEL_ONCORE        2
#define GPSMODEL_PCTIME        3
#define GPSMODEL_GARMIN        4
#define GPSMODEL_MAX           5

#define GPSMODELNAME_UNKNOWN   "GPS UNKNOWN"
#define GPSMODELNAME_NONE      "NO GPS"
#define GPSMODELNAME_TRIMBLE   "TRIMBLE"
#define GPSMODELNAME_ONCORE    "ONCORE"
#define GPSMODELNAME_PCTIME    "PCTIME"
#define GPSMODELNAME_GARMIN    "GARMIN"
#define GPSMODELNAME_MAX       "GPS UNKNOWN"


// This determines how much PARGPS PPS and serial data can be stored in the
// driver before it is read by a user program.  Currently it is set
// to 5 minutes of storage for data that comes in once per second.

#define MAX_PARGPS_STORAGE   (5 * 60)



// Error code return values returned by the PARGPS library functions.  The
// PARGPS_ERROR_MSG array, allocated in pargps.c, maps these error codes to
// short message strings for easy use with printf.

#define PARGPS_ERROR_NONE                           0
#define PARGPS_ERROR_NOT_AVAILABLE                  1
#define PARGPS_ERROR_INTERRUPT_NUMBER               2
#define PARGPS_ERROR_INTERRUPT_MODE                 3
#define PARGPS_ERROR_SERIAL_PORT_NUMBER             4
#define PARGPS_ERROR_SERIAL_PORT_NOT_OPEN           5
#define PARGPS_ERROR_DRIVER_NOT_OPEN                6
#define PARGPS_ERROR_DRIVER_REQUEST_FAILED          7
#define PARGPS_ERROR_KERNEL_AREA                    8
#define PARGPS_ERROR_KEYPRESS                       9
#define PARGPS_ERROR_BAD_PARAMETER                 10
#define PARGPS_ERROR_BAD_GPSMODEL                  11
#define PARGPS_ERROR_BAD_NMEA                      12
#define PARGPS_ERROR_MAX                           13

#define PARGPS_ERROR_MSG_NONE                       "NONE"
#define PARGPS_ERROR_MSG_NOT_AVAILABLE              "NOT AVAILABLE"
#define PARGPS_ERROR_MSG_INTERRUPT_NUMBER           "INTERRUPT NUM"
#define PARGPS_ERROR_MSG_INTERRUPT_MODE             "INTERRUPT MODE"
#define PARGPS_ERROR_MSG_SERIAL_PORT_NUMBER         "SERIAL NUM"
#define PARGPS_ERROR_MSG_SERIAL_PORT_NOT_OPEN       "SERIAL PORT"
#define PARGPS_ERROR_MSG_DRIVER_NOT_OPEN            "DEVICE DRIVER"
#define PARGPS_ERROR_MSG_DRIVER_REQUEST_FAILED      "DRIVER REQUEST"
#define PARGPS_ERROR_MSG_KERNEL_AREA                "KERNEL AREA"
#define PARGPS_ERROR_MSG_KEYPRESS                   "KEYPRESS"
#define PARGPS_ERROR_MSG_BAD_PARAMETER              "BAD PARAMETER"
#define PARGPS_ERROR_MSG_BAD_GPSMODEL               "BAD GPSMODEL"
#define PARGPS_ERROR_MSG_BAD_NMEA                   "BAD NMEA"
#define PARGPS_ERROR_MSG_MAX                        NULL

extern VARTYPE( char ) *PARGPS_ERROR_MSG[];


// General time defines

#define SECPERDAY   86400L // 24 hr/day * 60 min/hr * 60 sec/min = 86400 sec/day
#define SECPERHR     3600  // 60 sec/min * 60 min/hr = 3600 sec/hr
#define SECPERMIN      60  // seconds / minute
#define NSUPERSEC 10000000L // 100ns units / second
#define USPERSEC   1000000L // microseconds / second
#define USPERMS       1000L // microseconds / millisecond





/* 64 BIT SUPPORT:

Support for 64-bit values varies from one operating system to
another.  To compensate, a SR64BIT structure is defined.

*/

#if defined( SROS_WINXP ) || defined( SROS_WIN2K ) || defined( SROS_WINNT )
typedef union _SR64BIT {
    struct {
        unsigned long LowPart;
        long          HighPart;
    } u;
    __int64 QuadPart; // this is a LONGLONG
} SR64BIT;


#elif defined( SROS_WIN95 ) || defined( SROS_MSDOS )
typedef union _SR64BIT {
    struct {
        unsigned long LowPart;
        long          HighPart;
    } u;
//    LONGLONG QuadPart;
} SR64BIT;


#elif defined( SROS_LINUX )
typedef union _SR64BIT {
    struct {
        unsigned long LowPart;
        long          HighPart;
    } u;
    long long int QuadPart;
} SR64BIT;


#endif  // SROS_xxxxx




/* NMEA DEFINES:

NMEA, the National Marine Electonics Association, has defined a number
of standard messages to be used with marine instruments.  The SR PARGPS
is set up to use the RMC or ZDA and GGA messages.

The Rev D PARGPS uses a Garmin receiver programmed for RMC and GGA.  See
Chapter 4 of the Garmin GPS 18 LVC manual for a full description of the
possible NMEA messages.  Sample messages are:

  $GPRMC,204634,A,3608.9137,N,11517.9635,W,000.0,217.1,300508,013.4,E*65
  $GPGGA,204634,3608.9137,N,11517.9635,W,1,04,,,M,-25.3,M,,*71

The Rev C PARGPS uses a Trimble receiver programmed for ZDA and GGA.
See Appendix E of the Trimble ACE-III GPS receiver manual for a full
description of the possible NMEA messages.  Sample messages are:

  $GPZDA,232225.1,03,06,2003,,*55
  $GPGGA,232225.0,3609.237,N,11518.586,W,1,04,2.45,00751,M,-024,M,,*6A

*/

#define NmeaStartTag  "$"
#define NmeaTalkerId  "GP"
                                // Note: The NMEA message IDs are listed
                                // in the order they come out from Trimble
#define NMEA_MSGID_MIN  -1
#define NMEA_MSGID_ZDA   0
#define NMEA_MSGID_GGA   1
#define NMEA_MSGID_GLL   2
#define NMEA_MSGID_VTG   3
#define NMEA_MSGID_GSA   4
#define NMEA_MSGID_GSV   5
#define NMEA_MSGID_RMC   6
#define NMEA_MSGID_MAX   7

extern VARTYPE( char ) *NmeaMsgId[ NMEA_MSGID_MAX ];

#define MAX_NMEA_TYPE         NMEA_MSGID_MAX
#define MAX_NMEA_SIZE         83
#define MAX_NMEA_BUFF        (MAX_NMEA_TYPE*MAX_NMEA_SIZE)

#define MAX_NMEA_FIELDS       22  // One more than truely needed
#define MAX_NMEA_FIELDSIZE    11

typedef struct NMEA_parse {
        int  Nfields;
        int  CheckSum;
        char Field[MAX_NMEA_FIELDS][MAX_NMEA_FIELDSIZE];
        } NMEAPARSE;



// Indexes for character array holding source of HMS or YMD.
// The 7 NMEA related ones, from _ZDA to _RMC must match the
// corresponding NMEA_MSGID defines.

#define SRCSTR_MIN  -1
#define SRCSTR_ZDA   0
#define SRCSTR_GGA   1
#define SRCSTR_GLL   2
#define SRCSTR_VTG   3
#define SRCSTR_GSA   4
#define SRCSTR_GSV   5
#define SRCSTR_RMC   6
#define SRCSTR_PC    7
#define SRCSTR_OLD   8
#define SRCSTR_CLC   9
#define SRCSTR_UNK  10
#define SRCSTR_MAX  11




// Defines and structures for PARGPS PPS and serial data.  The PpsEventNum
// field is the big index that ties both sets of data together.

#define VALID_NONE       0x0
#define VALID_PPS        0x1
#define VALID_DREADY     0x2
#define VALID_NMEA       0x4
#define VALID_DATE       0x8


typedef struct _PpsPctimeData {
        int     ValidFields;
        int     PpsEventNum;
        SR64BIT CountAtPps;
        SR64BIT CountAtDready;
        SR64BIT PctimeAtPps;
        SR64BIT PctimeAtDready;
        } PPSTDATA;

typedef struct _SerialData {
        int   ValidFields;
        int   PpsEventNum;
        int   NmeaCount;
        char  NmeaMsg[MAX_NMEA_BUFF];
        } SERIALDATA;

typedef struct _GpsPctimeTimeData {
        int          ValidFields;
        unsigned int EventNumP;
        unsigned int EventNumS;
        SR64BIT      CountAtPps;
        SR64BIT      CountAtNow;
        int          NmeaCount;
        char         NmeaMsg[MAX_NMEA_BUFF];
        SR64BIT      PctimeAtPps;
        SR64BIT      PctimeAtNow;
        } GPSTIME2DATA;

typedef struct _PpsData {       // This is obsolete, use PPSTDATA instead
        int     ValidFields;
        int     PpsEventNum;
        SR64BIT CountAtPps;
        SR64BIT CountAtDready;
        } PPSDATA;

typedef struct _GpsTimeData {       // This is obsolete, use GPSTIME2DATA instead
        int          ValidFields;
        unsigned int EventNumP;
        unsigned int EventNumS;
        SR64BIT      CountAtPps;
        SR64BIT      CountAtNow;
        int          NmeaCount;
        char         NmeaMsg[MAX_NMEA_BUFF];
        } GPSTIMEDATA;



// Time stamp structure and defines

typedef struct _TIME_STAMP {

        int          Valid;         // Bit flags for which variables are valid
        unsigned int PpsEvent;      // Index of PPS event, links data, PPS & serial
        long         Sample;        // Index of DAQ data point for this PPS event
        long         CountObc;      // PAR8CH OnBoardCounter value at Sample
        SR64BIT      CountAtPps;    // 64 bit PC count value at PPS event
        SR64BIT      CountAtDready; // 64 bit PC count value at Sample data ready
        double       SecSince1970;  // Time in seconds from 1970 at PPS
        int          NumSat;        // Number of GPS satellites in view
        int          HmsSource;     // Indicates source of HMS info (eg PC or GPS)
        int          YmdSource;     // Indicates source of YMD info (eg PC or GPS)
        SR64BIT      PctimeAtPps;   // 64 bit PC system time value at PPS event
        SR64BIT      PctimeAtDready;// 64 bit PC system time value Sample data ready

        } TIMESTAMP;



#define TS_VALID_NONE    0x0   // Timestamp validity flags
#define TS_VALID_ANALOG  0x1
#define TS_VALID_PPS     0x2
#define TS_VALID_SERIAL  0x4
#define TS_VALID_OBC     0x8
#define TS_VALID_ALL     ( TS_VALID_ANALOG | TS_VALID_PPS | TS_VALID_SERIAL | TS_VALID_OBC )
#define TS_VALID_MOST    ( TS_VALID_ANALOG | TS_VALID_PPS | TS_VALID_OBC )


                            // Method for computing time at an arbitrary sample
#define TIME_METHOD_NONE 0  //   Assume perfect sampling
#define TIME_METHOD_SPS  1  //   From timestamp and sampling rate
#define TIME_METHOD_GPS  2  //   From PARGPS and PC counter (10us typical resolution)
#define TIME_METHOD_OBC  3  //   From PARGPS and PAR8CH OnBoardCounter (800ns res)
#define TIME_METHOD_PCT  4  //   From PC system time (10ms resolution on Windows!)


#define INVALID_PPSEVENT  0


// NOTE: These defines MUST match their counterparts in srdat.h

#define TIME_SOURCE_NONE          0       // none
#define TIME_SOURCE_CALC          1       // calculated
#define TIME_SOURCE_PC            2       // from the PC clock
#define TIME_SOURCE_GPS           3       // from GPS via PC counter
#define TIME_SOURCE_USER          4       // set by the user
#define TIME_SOURCE_ACQ           5       // set by acquisition program
#define TIME_SOURCE_OBC           6       // from GPS via on board counter
#define TIME_SOURCE_EST           7       // GPS estimate from invalid NMEA

#define INDEXCHANNEL_NONE        -1



// The PAR8CH On Board Counter has a period of 800 ns, which means it
// counts at rate of 1/800ns or 1,250,000 counts per second.

#define OBC_FREQ     1250000.0



// The default PC counter type is now always the standard 64 bit int
// (after 02/15/2006 changes in the driver it no longer depends on OS)
// NOTE: These values MUST match the corresponding
//       SRDAT_COUNTER_xxx defines in srdat.h

#define PC_COUNTER_TYPE_INT64        0
#define PC_COUNTER_TYPE_HIGH32LOW32  1
#define PC_COUNTER_TYPE_OTHER        2

#define DEFAULT_PC_COUNTER_TYPE    PC_COUNTER_TYPE_INT64
#define DEFAULT_PC_COUNTER2_TYPE   PC_COUNTER_TYPE_INT64





/* GENERAL PARGPS FUNCTIONS:

The following functions provide control over the PARGPS.  When used to
provide accurate timing for a PARxCH doing data acquisition, the
typical pattern of usage is:

1)  Open and initialize both the PARxCH and the PARGPS
1A) Attach the PARGPS to the PARxCH so their drivers can communicate
2)  Start the PARGPS and then the PARxCH
3)  Call the ParXchReadDataWithGpsMark to get analog data and the PARGPS
    ParGpsReadPpsData and ParGpsReadSerialData to get timing data
4)  Loop to continue getting more data
5)  When done, stop both the PARGPS and PARxCH
5A) Release the PARGPS from the PARxCH and close both drivers


Once the PARGPS and PARxCH drivers have been opened and started as
described above, the PARGPS driver can be opened by a second application
to retrieve the current GPS data.  For this case, the typical pattern
of usage is:

1) Open the PARGPS with ParGpsOpenForTimeOnly
2) Call ParGpsGetCurrentTime to get current GPS info like time, position,
   and # of satellites
3) Close the PARGPS with the standard ParGpsClose


The ParGpsGetRev function returns an integer indicating the library rev.

*/


FUNCTYPE( int ) ParGpsGetRev( int *Rev );


/* Default GPS model:

The PARGPS timing board comes in two versions.  One uses the now
discontinued Trimble ACE III receiver and the other uses the Garmin
GPS 18 LVC antenna/receiver.  While the software is designed to work
with either of the PARGPS models, the application programs often need to
know which model is being used.  Typically, the specific model is given
as a command line argument.  But for convenience, a default choice can
be saved (in either the registry or an environment variable depending on
OS) by using the setmodel utility.  This function retrieves this default
model information.

*/

FUNCTYPE( int ) ParGpsGetDefaultModel( char *DriverName );

FUNCTYPE( DEVHANDLE ) ParGpsOpen( char *ParGpsName, int SerialPortNumber, int *Error );
FUNCTYPE( DEVHANDLE ) ParGpsOpenForTimeOnly( char *ParGpsName, int *Error );
FUNCTYPE( DEVHANDLE ) ParGpsFullOpen( char *ParGpsName,
                                      int GpsModel,
                                      int SerialPortNumber,
                                      int *Spare,
                                      int *Error );

FUNCTYPE( int ) ParGpsClose( DEVHANDLE ParGpsHandle );

FUNCTYPE( int ) ParGpsStart( DEVHANDLE ParGpsHandle );
FUNCTYPE( int ) ParGpsStop( DEVHANDLE ParGpsHandle );

FUNCTYPE( int ) ParGpsGetKernelArea( DEVHANDLE ParGpsHandle, void **KernelArea );
FUNCTYPE( int ) ParGpsGetFullCounterFrequency( DEVHANDLE ParGpsHandle,
                                               long *CounterFreq1Hi,
                                               long *CounterFreq1Lo,
                                               long *CounterFreq2Hi,
                                               long *CounterFreq2Lo );

FUNCTYPE( unsigned int ) ParGpsReadPpsData( DEVHANDLE ParGpsHandle,
                                            void *Values,
                                            unsigned int Nvalues,
                                            int *Error
                                          );

FUNCTYPE( unsigned int ) ParGpsReadSerialData( DEVHANDLE ParGpsHandle,
                                               void *Values,
                                               unsigned int Nvalues,
                                               int *Error
                                             );

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
                                      int *Error );

FUNCTYPE( int ) ParGpsFlushSerial( DEVHANDLE ParGpsHandle );


/* ALTERNATE AND STRUCTURE HELPER FUNCTIONS:
 *
 * Some of the previous functions pass structures or have large numbers
 * of arguments which makes them difficult to call from other languages.
 * So these alternate and helper functions have been provided.  If you
 * have a choice, it is better to use an original function than an
 * alternate since the alternate just calls the original and then
 * repackages the results.
 */

FUNCTYPE( int ) ParGpsGetCurrentTimeAlt( DEVHANDLE ParGpsHandle,
                                         int *IntReturns,
                                         long *LongReturns,
                                         double *DoubleReturns,
                                         int *Error
                                       );

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
                                         );

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
                                         );

FUNCTYPE( int ) ParGpsSplitCombineSerialData( int           SplitCombine,
                                              unsigned int  Nvalues,
                                              void         *SerialData,
                                              int          *ValidFields,
                                              int          *PpsEventNum,
                                              int          *NmeaCount,
                                              char         *NmeaMsg,
                                              int          *Error
                                            );

FUNCTYPE( int ) ParGpsGetStructureSize( int *PpsDataSize,
                                        int *SerialDataSize
                                      );

FUNCTYPE( int ) ParGpsScaleGpsMark( int   MarkIndex,
                                    int   Nchannels,
                                    int   Nsamples,
                                    long  ScaleValue,
                                    long *Data,
                                    int  *Error
                                   );





/* NMEA HELPER FUNCTIONS:

NMEA, the National Marine Electonics Association, has defined a
number of standard messages to be used with marine instruments.
These helper functions work with the serial NMEA messages returned
by the PARGPS.

The first of these functions takes a SERIALDATA structure as returned
from the PARGPS and extract the requested data from the NMEA strings
contained in that structure.  Time and location data may be present in
more than one of the NMEA strings.  Because it has access to all the
NMEA strings received for that second, this function uses a ranking
system to get the requested data from the "best" NMEA string.  For
example, it is better to get time from the ZDA string than from the GGA
string because the latter is invalid if there are not enough satellites
in view.

Those wishing to do their own processing of the NMEA messages may
find the ParGpsParseNmeaData function helpful since it parses the
message into its component data fields.

The remaining five functions take a single NMEA string as input and
return the specified quantity.

For the PARGPS, each NMEA message starts with a $GP and a three
character message id.  That is followed by a comma separated list
of data fields which may be empty and whose meaning depends on the
message id.  Each message ends with a *, a checksum, and a carriage
return line feed pair.  For example:

$GPGGA,163859.0,3609.422,N,11518.713,W,1,03,1.75,00024,M,-024,M,,*62

*/

FUNCTYPE( int ) ParGpsRequestNmeaInfo2( char *NmeaMsgAll, int NmeaCount,
                int *Year, int *Mon, int *Day, int *YmdSource, int *YmdIsValid,
                int *Hour, int *Minute, int *Second,
                long *MicroSecond, int *HmsSource, int *HmsIsValid,
                double *Latitude, double *Longitude, int *LocSource, int *LocIsValid,
                double *Altitude, int *PosSource, int *PosIsValid,
                int *Nsat, int *SatSource, int *SatIsValid );
FUNCTYPE( int ) ParGpsParseNmeaData( char *NmeaMsg, NMEAPARSE *ParseData );

FUNCTYPE( int ) ParGpsGetNmeaSatellites( char *NmeaMsg, int *NumSat, int *Source );
FUNCTYPE( int ) ParGpsGetNmeaTime( char *NmeaMsg, double *Time,
                                   int *SourceYMD, int *SourceHMS );
FUNCTYPE( int ) ParGpsGetNmeaHMS( char *NmeaMsg, double *HMS,
                                  int *Hour, int *Minute, int *Second,
                                  long *MicroSecond, int *Source );
FUNCTYPE( int ) ParGpsGetNmeaYMD( char *NmeaMsg, double *YMD,
                                  int *Year, int *Month, int *Day, int *Source );
FUNCTYPE( int ) ParGpsGetNmeaPosition( char *NmeaMsg,
                                       double *Latitude, double *Longitude,
                                       double *Altitude, int *Source );

// ParGpsRequestNmeaInfo is being discontinued, use ParGpsRequestNmeaInfo2 instead
FUNCTYPE( int ) ParGpsRequestNmeaInfo( char *NmeaMsgAll, int NmeaCount,
                int *Year, int *Mon, int *Day, int *YmdSource,
                int *Hour, int *Minute, int *Second,
                long *MicroSecond, int *HmsSource,
                double *Latitude, double *Longitude,
                double *Altitude, int *PosSource,
                int *Nsat, int *NsatSource );



/* TIME HELPER FUNCTIONS:

Time can be represented in many different forms.  These helpers convert
between some of the forms described below.

1) SECONDS SINCE 1970
   Many of the ANSI time management functions work with time as a
   time_t, a double that represents time as the total number of seconds
   and fractional seconds since 1/1/1970.  These values incorporate both
   YMD and HMS into a single number.  It is possible to make a time_t
   number that holds just YMD or HMS, but each compensates for the local
   time offset from UTC.  So, when adding a YMD-only time_t with an
   HMS-only time_t you must subtract one local time offset.  Use the
   function ParGpsCalcTimeZoneOffset to determine this offset.

2) COLLECTION OF INTS
   Time can also be represented as a collection of individual integers:
   Year, Month, Day, Hour, Minute, Second, Microseconds.  Microseconds
   must be a long integer.  These separate components are similar to the
   corresponding fields in the standard runtime library struct tm
   structure, eg tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec.
   Use the ParGpsSecTimeSplit and ParGpsSecTimeCombine functions to convert
   between this collection of ints representation and the seconds since
   1970 representation.

3) PACKED DIGITS
   In the NMEA message strings, time is typically presented as
   a double where three quantities are packed into pairs of digits.
   The quantities involved can be either year, month, and day or
   hours, minutes, and seconds.  Each quantity is allocated two
   digits.  For example, 112307 means 23 minutes and 7 seconds past
   11 o'clock.  Since the minutes and seconds digits can never be
   more than 59, adding 1 second to 112359 gives 112400.  The
   ParGpsPack3 and ParGpsUnpack3 functions convert between this packed
   representation and a collection of individual ints.

*/


FUNCTYPE( int ) ParGpsPack3( int Top, int Middle, int Bottom, int *Xxx  );
FUNCTYPE( int ) ParGpsUnpack3( int Xxx, int *Top, int *Middle, int *Bottom );
FUNCTYPE( int ) ParGpsGetPcTime( double *Time,
                                 int *Year, int *Month, int *Day,
                                 int *Hour, int *Minute, int *Second,
                                 long *Microsecond );
FUNCTYPE( int ) ParGpsSetPcTime( double Time,
                                 int Year, int Month, int Day,
                                 int Hour, int Minute, int Second,
                                 long Microsecond, int FromYMD );
FUNCTYPE( int ) ParGpsCalcTimeZoneCorr( long StartTime );
FUNCTYPE( int ) ParGpsSecTimeSplit( double Time,
                                    int *Year, int *Month, int *Day,
                                    int *Hour, int *Minute, int *Second,
                                    long *Microsecond );
FUNCTYPE( int ) ParGpsSecTimeCombine( int Year, int Month, int Day,
                                      int Hour, int Minute, int Second,
                                      long Microsecond, double *Time );



/* SERIAL PORT HELPER FUNCTIONS:

These functions provide an OS independent way to access the serial port.

*/

FUNCTYPE( int ) ParGpsSerialPortName( int PortNumber, char *SerialPortName );
FUNCTYPE( DEVHANDLE ) ParGpsSerialOpen( char *SerialPortName );
FUNCTYPE( int ) ParGpsSerialInit( DEVHANDLE SerialHandle );
FUNCTYPE( int ) ParGpsSerialRead( DEVHANDLE SerialHandle, void *Values,
                                  unsigned long BytesToRead,
                                  unsigned long *BytesRead );
FUNCTYPE( void ) ParGpsSerialClose( DEVHANDLE SerialHandle );




/* SR64BIT LARGE INTEGER HELPER FUNCTIONS:

 */

int     ParGpsLargeIntDiffFull( SR64BIT A, SR64BIT B,
                                SR64BIT *C, double *C2 );
SR64BIT ParGpsLargeIntAdd( SR64BIT A, long More );

SR64BIT ParGpsLargeIntAddType(  SR64BIT A, long More, int Type );
int ParGpsLargeIntDiffFullType( SR64BIT A, SR64BIT B, int Type,
                                SR64BIT *C, double *C2 );



/* TIMESTAMP HELPER FUNCTIONS:

When each PPS event occurs, three different types of data (analog, pps,
and serial) are generated.  The TIMESTAMP structure is used to collect
and organize this data for a single PPS event.

The analog data relating to a timestamp includes the sample point index
of the "mark point" which is the first sample acquired after a PPS event
and, for the PAR8CH, the value of the on board counter (OBC) when the
PPS event occurs.

The PPS data relating to a timestamp includes the value of the PC
counter when the PPS event occurs and when the DREADY signal occurs for
the mark point.

The serial data relating to a timestamp includes the number of seconds
since 1970 and the source of the year-month-day (YMD) and
hour-minute-second (HMS) components of that number.  This information is
derived from the serial NMEA messages.

The ParGpsTsClear function is a stand-alone helper that initializes the
passed in timestamp.

The ParGpsTsInit function is called in ParGpsOpen, but if you will be
using the ParGpsTsXXX library functions by themselves without opening
the driver, you must call this initialization function directly.

The ParGpsTsProcessXXX functions are provided to help extract the
timestamp related info from the full analog, pps, and serial data
returned from the PARxCH and PARGPS library functions.

Because the order in which the three types of data arrives is not
guaranteed, the library maintains a large circular buffer of timestamp
structures.  As each piece of data arrives it is put into the
appropriate entry.  Once all three pieces have arrived for any given
time stamp, it is marked as valid and can be read out of the library's
circular buffer and into the user's code using the ParGpsTsReadValid
function.

The ParGpsTsFileXxx functions read or write a timestamp structure from
or to an ascii file which is already open for append.

Although the PC counter values CountAtPps and CountAtDready are
typically very accurate, they can occasionally be compromised when the
PC experiences especially long interrupt latencies.  Several helper
functions are provided to identify and correct these outliers.
ParGpsTsIsOutlier checks to see if the target timestamp count values are
close enough to the standard ones.  If not, ParGpsTsOutlierAdjust will
correct the target timestamp to have the expected count values.

While time at the PPS signal is known, we must interpolate to get the
time at an arbitrary data point.  The ParGpsTsComputeTimeXxx functions
do this computation using various methods.  The OBC method uses the on
board counts info and is most accurate, but is only available for the
PAR8CH.  The PPS method uses the PPS and DREADY CountAt values and is
the next best.  The SPS method uses the sampling rate.

*/

FUNCTYPE( int ) ParGpsTsClear( TIMESTAMP *TS );
FUNCTYPE( int ) ParGpsTsInit( void );
FUNCTYPE( int ) ParGpsTsProcessAnalog(
                           long *DataValues, long NumSamples, long NumChannels,
                           long FirstSample, long MarkChannel, long ObcChannel,
                           double ObcCountsPerSample, int TsValid,
                           unsigned int *PpsEvent, int *Nvalid );
FUNCTYPE( int ) ParGpsTsProcessPps( PPSTDATA *PpsValues, long MaxPps,
                                    double CountsPerSecond,
                                    int GpsModel, int TsValid,
                                    unsigned int *PpsEvent, int *Nvalid );
FUNCTYPE( int ) ParGpsTsProcessSerial(
                           SERIALDATA *SerialValues, long MaxSer,
                           int SerialDelay, int TsValid,
                           unsigned int *PrevPpsEvent, double *PrevSecSince1970,
                           unsigned int *PpsEvent, int *Nvalid );

FUNCTYPE( int ) ParGpsTsReadValid( TIMESTAMP *TS, int TsValid );
FUNCTYPE( int ) ParGpsTsReadNext(  TIMESTAMP *TS );

FUNCTYPE( int ) ParGpsTsFileRead(  FILE *tsfile, TIMESTAMP *TS, int skiptitle );
FUNCTYPE( int ) ParGpsTsFileWrite( FILE *tsfile, TIMESTAMP *TS, int writetitle );


FUNCTYPE( int ) ParGpsTsIsOutlier(
                       TIMESTAMP *TS0, TIMESTAMP *TS1,
                       long Nsps, int PcCounterType,
                       double StandardCountsPps,    double LimitPps,
                       double StandardCountsDready, double LimitDready,
                       double *ActualCountsPps,     double *DiffPps,
                       double *ActualCountsDready,  double *DiffDready );

FUNCTYPE( int ) ParGpsTsComputeTimeSps(
                            TIMESTAMP *TimeStamp1, TIMESTAMP *TimeStamp2,
                            double Sps, int TsValid,
                            long Sample, double *Time );
FUNCTYPE( int ) ParGpsTsComputeTimePps(
                            TIMESTAMP *TimeStamp1, TIMESTAMP *TimeStamp2,
                            double BestCountsBetweenPps,
                            int TsValid, long Sample, double *Time );
FUNCTYPE( int ) ParGpsTsComputeTimePctime(
                            TIMESTAMP *TimeStamp1, TIMESTAMP *TimeStamp2,
                            double BestCountsPerSecond,
                            int TsValid, long Sample, double *Time );
FUNCTYPE( int ) ParGpsTsComputeTimeObc(
                            TIMESTAMP *TimeStamp1, TIMESTAMP *TimeStamp2,
                            double ObcCountsPerSample, int TsValid,
                            long Sample, double *Time );





/* PRIVATE HELPER FUNCTIONS:

These functions are NOT for general use.  They are private and should
be used only by other library functions and programs like diag.c that
are given special access.

*/

#ifdef INCLUDE_HELPER_DEFINES

FUNCTYPE( int ) ParGpsIoctl( DEVHANDLE      ParGpsHandle,
                             unsigned long  IoCtlCode,
                             void          *pValueIn,
                             unsigned long  InSize,
                             void          *pValueOut,
                             unsigned long  OutSize,
                             unsigned long *ReturnSize
                           );
FUNCTYPE( int ) ParGpsInit( DEVHANDLE ParGpsHandle, int SerialPort,
                            int InterruptMode, int GpsModel );
FUNCTYPE( int ) ParGpsSerialSetup( DEVHANDLE ParGpsHandle, int PortNumber );
FUNCTYPE( int ) ParGpsSetInterruptMode( DEVHANDLE ParGpsHandle, int Mode );
FUNCTYPE( int ) ParGpsReset( DEVHANDLE ParGpsHandle );
FUNCTYPE( int ) ParGpsGetInterruptCount( DEVHANDLE ParGpsHandle, int *count );

#endif // INCLUDE_HELPER_DEFINES

// FILE: SrUsbXch.c
// COPYRIGHT: (c), Symmetric Research, 2009-2011
//
// These are the USBxCH user library functions.  They are mostly
// wrappers around the lower level read, write, and ioctl device driver
// functions that do the actual work of controlling the USBxCH.  Users
// should limit themselves to calling functions listed in the SrUsbXch.h
// header file unless they are EXTREMELY EXPERT at calling device driver
// code directly.
//
// To use this library, just compile it to an object file and statically
// link it with your application.  It can also be compiled to a DLL or
// shared object library.  See the makefiles for examples of how to
// compile.
//
// This single source code files services all the supported operating
// systems.  When compiling, define one of the SROS_xxxxx constants
// shown in SrUsbXch.h on the compiler command line to indicate which OS
// you are using.
//
// For Win2K/XP and Linux, the driver code is provided as a kernel
// mode device driver and must be installed using the indriver utility
// in the driver directory.  For Win2K/XP, the driver is named
// SrUsbXchDrv.sys and for Linux it is named SrUsbXchDrv.o.  See the driver
// directory for more details.
//
//
// Change History:
//
// 2011/08/09  WCT  Minor changes for POSIX compliance
//
//

#include <stdio.h>       // for SEEK_CUR
#include <stdlib.h>      // for malloc/free, getenv/putenv
#include <time.h>        // localtime, timer_t, struct tm, etc
#include <string.h>      // for memset
#include <sys/stat.h>    // fstat function (VC++ can use / in path)


// OS dependent includes ...

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )
#include <windows.h>     // windef.h -> winnt.h -> defines GENERIC_READ etc
#include <winioctl.h>    // device driver I/O control macros
#include <io.h>          // access function
#include <process.h>     // for thread functions _beginthreadex, etc


// String safe functions can be used as inline or as a library
// To use as library comment in next line and link to $(SDK_LIB_PATH)\strsafe.lib
//#define STRSAFE_LIB
//#include <strsafe.h>    // this should be the last include



#elif defined( SROS_LINUX )
#include <errno.h>       // errno global
#include <fcntl.h>       // open  function
#include <unistd.h>      // close, access, fsync fns and STDIN_FILENO define
#include <sys/ioctl.h>   // ioctl function and macros

#endif  // SROS_xxxxx




// USBxCH includes ...

#include "SrDefines.h"    // for function type, devhandle, and other common defines
#include "SrUsbXch.h"     // for USBXCH library defines and prototypes
#include "SrUsbXchDrv.h"  // for USBXCH driver-shared defines and prototypes
#include "SrUsbXch.hw"    // for USBXCH encoded hardware info


#define MAXSTR 255



// Actual allocation of USBxCH error string array, see SrUsbXch.h ...

VARTYPE( char ) *USBXCH_ERROR_MSG[] = {

        USBXCH_ERROR_MSG_NONE,
        USBXCH_ERROR_MSG_OPEN_DRIVER_NAME,
        USBXCH_ERROR_MSG_OPEN_SAMPLE_RATE,
        USBXCH_ERROR_MSG_OPEN_ATOD_UNKNOWN,
        USBXCH_ERROR_MSG_OPEN_GPS_UNKNOWN,
        USBXCH_ERROR_MSG_OPEN_BOARD_RESET,
        USBXCH_ERROR_MSG_OPEN_DOWNLOAD,
        USBXCH_ERROR_MSG_OPEN_START_8051,
        USBXCH_ERROR_MSG_OPEN_POWER,
        USBXCH_ERROR_MSG_OPEN_SUCCESS_PORT_RESET,
        USBXCH_ERROR_MSG_OPEN_SUCCESS_PORT_CYCLE,
        USBXCH_ERROR_MSG_OPEN_SUCCESS_BOARD_RESET,
        USBXCH_ERROR_MSG_START_POWER,
        USBXCH_ERROR_MSG_START_COMMAND,
        USBXCH_ERROR_MSG_STOP_COMMAND,
        USBXCH_ERROR_MSG_EVEN_SAMPLES,
        USBXCH_ERROR_MSG_INVALID_PARAM,
        USBXCH_ERROR_MSG_INVALID_PACKET,
        USBXCH_ERROR_MSG_BAD_NMEA,
        USBXCH_ERROR_MSG_CONFIGURE,
        USBXCH_ERROR_MSG_SPS_INVALID,
        USBXCH_ERROR_MSG_DECIMATION,
        USBXCH_ERROR_MSG_DRIVER_NOT_OPEN,
        USBXCH_ERROR_MSG_DRIVER_REQUEST_FAILED,
        USBXCH_ERROR_MSG_DRAM_READ_COMMAND,
        USBXCH_ERROR_MSG_DRAM_READ_RESULTS,
        USBXCH_ERROR_MSG_ATOD_RESET,
        USBXCH_ERROR_MSG_CALIBRATION,
        USBXCH_ERROR_MSG_OVERFLOW,
        USBXCH_ERROR_MSG_KEYPRESS,
        USBXCH_ERROR_MSG_SANITY_DATA,
        USBXCH_ERROR_MSG_SANITY_STATUS,
        USBXCH_ERROR_MSG_DIGITAL_NOT_AVAILABLE,
        USBXCH_ERROR_MSG_FAILED_SEND,
        USBXCH_ERROR_MSG_NO_SPACE_AVAILABLE,
        USBXCH_ERROR_MSG_READ_NO_DATA,
        USBXCH_ERROR_MSG_READ_NOT_ENOUGH_DATA,
        USBXCH_ERROR_MSG_READ_TOO_LARGE,
        USBXCH_ERROR_MSG_READ_RESULTS_FAILED,
        USBXCH_ERROR_MSG_WRITE_TOO_LARGE,
        USBXCH_ERROR_MSG_WRITE_FAILED,
        USBXCH_ERROR_MSG_EEPROM_EXCEEDED,
        USBXCH_ERROR_MSG_EEPROM_PROTECTED,
        USBXCH_ERROR_MSG_SERIALBUFFER_FULL,
        USBXCH_ERROR_MSG_GPS_STATUS_CHANGE,
        USBXCH_ERROR_MSG_CONVERT_PAK_BAD_INPUT,
        USBXCH_ERROR_MSG_CONVERT_PAK_BAD_OPT_INPUT,
        USBXCH_ERROR_MSG_CONVERT_PAK_ARRAY_FULL,
        USBXCH_ERROR_MSG_CONVERT_PAK_TYPE_UNKNOWN,
        USBXCH_ERROR_MSG_TS_FILL_SAMPLE_BAD_INPUT,
        USBXCH_ERROR_MSG_TS_FILL_SAMPLE_BAD_TS,
        USBXCH_ERROR_MSG_TS_FILL_SAMPLE_BAD_PKT,
        USBXCH_ERROR_MSG_TS_FILL_SAMPLE_TS_FULL,
        USBXCH_ERROR_MSG_TS_FILL_SAMPLE_STATUS_FULL,
        USBXCH_ERROR_MSG_TS_FILL_STATUS_BAD_INPUT,
        USBXCH_ERROR_MSG_TS_FILL_STATUS_BAD_TS,
        USBXCH_ERROR_MSG_TS_FILL_STATUS_TS_FULL,
        USBXCH_ERROR_MSG_TS_FILL_STATUS_STATUS_FULL,
        USBXCH_ERROR_MSG_TS_FILL_STATUS_SERIAL_FULL,
        USBXCH_ERROR_MSG_TS_FILL_STATUS_NMEA_FULL,
        USBXCH_ERROR_MSG_TS_FILL_STATUS_BAD_NMEA,
        USBXCH_ERROR_MSG_TS_VALID_OUT_OF_RANGE,
        USBXCH_ERROR_MSG_TS_VALID_NO,
        USBXCH_ERROR_MSG_PROCESS_PKT_BAD_INPUT,
        USBXCH_ERROR_MSG_PROCESS_PKT_OUT_OF_RANGE,
        USBXCH_ERROR_MSG_PROCESS_PKT_ARRAY_FULL,
        USBXCH_ERROR_MSG_OUTPUT_COL_BAD_INPUT,
        USBXCH_ERROR_MSG_OUTPUT_COL_NO_MORE_STATUS,
        USBXCH_ERROR_MSG_TSCOMBINE_NO_MORE_SAMPLE,
        USBXCH_ERROR_MSG_TSCOMBINE_NO_MORE_STATUS,
        USBXCH_ERROR_MSG_TSCOMBINE_TS_FULL,
        USBXCH_ERROR_MSG_TSCOMBINE_TS_BAD_ALIGN,
        USBXCH_ERROR_MSG_SERIALPARSE_NMEA_FULL,
        USBXCH_ERROR_MSG_BAD_GPS_MODEL,
        USBXCH_ERROR_MSG_MAX

        };


VARTYPE( char ) *UsbXchErrorMsgDetailDriverNotOpen[] = {
        "\nCould not locate device driver.  Likely causes include ...\n\n",
        "  1) USBxCH not plugged in, so driver not active\n",
        "  2) Device ID is N not 0, so driver name is SrUsbXchN\n",
        "  3) Driver file SrUsbXch.sys never installed\n",
        "  4) Power so low plug and play not activated\n\n",
        "Run DevMan showall or SetDid showall to check for active drivers\n\n",
        NULL
        };        

// GLOBAL ERROR VARIABLE:
//
// The global variable SrUsbXchLastDriverError keeps track of the last OS
// provided error.  Most users will never need to access this.  If you
// do, declare it as extern in your source code.
//
//

long SrUsbXchLastDriverError = 0L;
char SrUsbXchLastErrorString[MAXSTR];


// GLOBAL ACQUISITION PARAMETERS
//
// These parameters should be reset each time a new acquisition run
// is started.

// Some of these max values are set MUCH higher than we would normally
// expect.  This is done to allow many spurious values to accumulate
// before we get synchronized again and throw out the bad values.
// Spurious values can occur for many reasons such as
// 1) NMEA characters overwritten in USBxCH onboard memory area when
//    not serviced fast enough -> to missed NMEA messages
// 2) GPS antenna sending extra PPS signals when resuming after 0 satellites
// 3) False PPS signals created by static
// 4) etc ...

#define MAX_ACQRUN_TS                 1000      // expect 1 per second (normal 2)
#define MAX_ACQRUN_STATUS_ARRAY       1000      // expect 1 per second (normal 2)
#define MAX_ACQRUN_SAMPLE_PACKETS    10000      // expect SPS/2 per second (2x normal on startup)
#define MAX_ACQRUN_STATUS_PACKETS     5000      // expect ~3 or 4 per second

struct SrUsbXchAcquisitionRunGlobalsLayout {

        // These first two values are set once at open time and
        // not in AcqRunInit called from start
        
        double SamplePeriod;                   // Sample period = 1 / true sampling rate
        int GpsModel;                          // GPS model (eg GARMIN, PCTIME, NONE)
        
        double StartTime;                      // Start time in seconds since 1970
        int SamplesPerSecond;                  // Approx samples per second
        int ObcCorrect;                        // OBC counts per 1 sample
        
        SRUSBXCH_PACKET *SamplePacketArray;    // Raw acquired packets
        int PaSampleWritePos;                  // Indexes into packet array
        int PaSampleReadPos;                   // so it can be used as a
        int PaSamplePeekPos;                   // circular buffer.
        int PaSampleCount;                     //
                
        SRUSBXCH_PACKET *StatusPacketArray;    // Raw acquired packets
        int PaStatusWritePos;                  // Indexes into packet array
        int PaStatusReadPos;                   // so it can be used as a
        int PaStatusCount;                     // circular buffer.
                
        int nSamplePackets;              // # status type.  Can have
        int nStatusPackets;              // some in array not typed yet.

        SRUSBXCH_STATUSDATA *StatusArray;// Status data array from packet info
        int SaCount;                     // Number of entries filled in array
        int SaWritePos;                  // Next array position to fill
        int SaReadPos;                   // Next array position to use

        TS TsSample[MAX_ACQRUN_TS];      // Sample time info
        int TsSampleWritePos;
        int TsSampleReadPos;
        int TsSampleCount;

        TS TsStatus[MAX_ACQRUN_TS];      // Status time info
        int TsStatusWritePos;
        int TsStatusReadPos;
        int TsStatusCount;

        TS TsTotal[MAX_ACQRUN_TS];       // Total time info
        int TsTotalWritePos;
        int TsTotalReadPos;
        int TsTotalCount;

        TS TsLeft;                       // Need a left and right
        TS TsRight;                      // TS to interpolate times.
        int nTsValid;                    // Count of valid timestamps

        int CurrentPoint;                // Point # since acquisition start
                
        int nPoints;                     // Persistent sample data
        int nObc;
        int PpsEventLast;
        int PpsToggleLast;
        int ObcLast;
        int ObcPpsCountLast;
                        
        int nNmea;                       // Persistent status data
        int nSecondLast;
        char NmeaGroup[SRDAT_NMEA_MAX_BUFF];

        SRUSBXCH_SAMPLEDATA *SampleData; // User supplied sample data array
        int SampleMaxCount;              // Max size of sample data array
        int SampleCount;                 // Number of entries filled in array

        SRUSBXCH_STATUSDATA *StatusData; // User supplied status data array
        int StatusMaxCount;              // Max size of status data array
        int StatusCount;                 // Number of entries filled in array

        TS *TsData;                      // User supplied timestamp data array
        int TsMaxCount;                  // Max size of ts data array
        int TsCount;                     // Number of entries filled in array
 
//WCT - maybe include serial buffer and its pointers in this structure too
};
typedef struct SrUsbXchAcquisitionRunGlobalsLayout SRUSBXCH_ACQRUNGLOBAL;

SRUSBXCH_ACQRUNGLOBAL AcqRun;



// Sample rate is computed based on crystal speed (Fclkin)
// using formula 1 from page 18 of the TI ADS1255 spec sheet

typedef struct SampleRateInfo {
                                int    code;
                                double rate;
                                }  SAMPLERATEINFO;

SAMPLERATEINFO SampleRateTable[] = {
        {0x03,  (USBXCH_XTAL_SPEED/(double)(256*12000))},
        {0x13,  (USBXCH_XTAL_SPEED/(double)(256* 6000))},
        {0x23,  (USBXCH_XTAL_SPEED/(double)(256* 3000))},
        {0x33,  (USBXCH_XTAL_SPEED/(double)(256* 2000))},
        {0x43,  (USBXCH_XTAL_SPEED/(double)(256* 1200))},
        {0x53,  (USBXCH_XTAL_SPEED/(double)(256* 1000))},
        {0x63,  (USBXCH_XTAL_SPEED/(double)(256*  600))},
        {0x72,  (USBXCH_XTAL_SPEED/(double)(256*  500))},
        {0x82,  (USBXCH_XTAL_SPEED/(double)(256*  300))},
        {0x92,  (USBXCH_XTAL_SPEED/(double)(256*   60))},
        {0xA1,  (USBXCH_XTAL_SPEED/(double)(256*   30))},
        {0xB0,  (USBXCH_XTAL_SPEED/(double)(256*   15))},
        {0xC0,  (USBXCH_XTAL_SPEED/(double)(256*    8))},
        {0xD0,  (USBXCH_XTAL_SPEED/(double)(256*    4))},
        {0xE0,  (USBXCH_XTAL_SPEED/(double)(256*    2))},
        {0xF0,  (USBXCH_XTAL_SPEED/(double)(256*    1))}
        };



// The UserByte includes digital IO and LED portions

char UserByte = 0x00;

#define DIG_MASK    0x0F
#define DIG_1       0x01
#define DIG_2       0x02
#define DIG_3       0x04
#define DIG_4       0x08
#define DIG_ALL     0x0F

#define LED_MASK    0x30
#define LED_NONE    0x00
#define LED_YELLOW  0x10
#define LED_RED     0x20
#define LED_BOTH    0x30


// The XcrUserCfg byte on the CoolRunner allows various options to be
// set.  These include whether the serial NMEA signal should be inverted
// or not, whether the PPS and Hardware Trigger signals should be active
// on the rising or falling edge, and whether the red LED should be
// controlled by the PPS signal or the user.
//
// Do NOT change these bit positions as they MUST match the values
// expected by the XCR firmware (ABEL code).

#define XCRUSERCFG_NMEATXPOLARITY  0x80
#define XCRUSERCFG_NMEARXPOLARITY  0x40
#define XCRUSERCFG_PPSLED          0x20
#define XCRUSERCFG_PPSPOLARITY     0x10
#define XCRUSERCFG_PPSSOURCE       0x08
#define XCRUSERCFG_ADRDY           0x04
#define XCRUSERCFG_NOTUSED         0x02
#define XCRUSERCFG_TRIGGERPOLARITY 0x01


// The CoolRunner sends various info to the 8051 via Port A.  This info
// includes the state of various DRAM flags.  The  XCRDRAMFLAGS defines
// specify the bit positions within the Port A byte which correspond to
// the empty, full, and part full DRAM flags.
//
// These bit positions MUST agree with the XCR ABEL code

#define XCRDRAMFLAGS_EMPTY       2
#define XCRDRAMFLAGS_FULL        3
#define XCRDRAMFLAGS_PARTFULL    4



// These defines ensure the SrUsbXchDramRead + SrUsbXchDramWrite
// functions send only what the 8051 functions CMD_DRAM_READ and
// CMD_DRAM_WRITE can handle.  The number of DRAM bytes that can be sent
// is the packet size minus the two bytes required to send the command
// and number of DRAM bytes to read/write.
//
// These defines MUST agree with the 8051 code

#define USB8051_MAX_DRAM_PACKETS      255
#define USB8051_MAX_DRAM_WRITE_BYTES (SRDAT_USBPACKET_SIZE-2)


// 8051 CPU control
//
// These defines MUST agree with the Cypress SEI code

#define  USB8051_CPU_RUN    0
#define  USB8051_CPU_HOLD   1


// WCT - Reorder these and leave only those the library functions actually
//       used.  Move the others to factory.h
// Commands for output to the 8051
// NOTE: These commands MUST match the order of the ajmp CmdXxx entries
//       in the CommandTable in the ProcessCmd function in the SrUsbXch.a51
//       firmware code.  Each ajmp is 2 bytes beyond the next, so the
//       CMD defines must be even sequential numbers in the proper order.
//
// These defines MUST agree with the 8051 code

#define USB8051_CMD_DONE           0x00
#define USB8051_CMD_GET_STATE      0x02
#define USB8051_CMD_POWER_RESET    0x04
#define USB8051_CMD_CONFIGURE      0x06
#define USB8051_CMD_DIGLED         0x08
#define USB8051_CMD_TEMP_SET_REG   0x0A
#define USB8051_CMD_TEMP_READ      0x0C
#define USB8051_CMD_ATOD_RESET     0x0E
#define USB8051_CMD_ATOD_SENDCMD   0x10
#define USB8051_CMD_ATOD_READREG   0x12
#define USB8051_CMD_ATOD_START     0x14
#define USB8051_CMD_ATOD_STOP      0x16
#define USB8051_CMD_GET_DREADY     0x18
#define USB8051_CMD_DRAM_RESET     0x1A
#define USB8051_CMD_DRAM_WRITE     0x1C
#define USB8051_CMD_DRAM_READ      0x1E
#define USB8051_CMD_SERIAL_RESET   0x20
#define USB8051_CMD_SERIAL_WRITE   0x22
#define USB8051_CMD_SERIAL_READ    0x24
#define USB8051_CMD_MEM_READ       0x26
#define USB8051_CMD_MEM_WRITE      0x28
#define USB8051_CMD_SEND_TIME      0x2A
#define USB8051_CMD_GET_FRAME      0x2C
#define USB8051_CMD_INTERRUPT      0x2E
#define USB8051_CMD_EEPROM_SET_DID 0x30 // If this changes, update SetDid.c
#define USB8051_CMD_DBG            0x32
#define USB8051_CMD_UNUSED_34      0x34
#define USB8051_CMD_UNUSED_36      0x36
#define USB8051_CMD_UNUSED_38      0x38
#define USB8051_CMD_UNUSED_3A      0x3A
#define USB8051_CMD_UNUSED_3C      0x3C
#define USB8051_CMD_UNUSED_3E      0x3E

#define USB8051_CMD_MAX            0x40 // If you reach this, you must increase firmware table




#define PIPE_DIR_UNKNOWN  0
#define PIPE_DIR_IN       1
#define PIPE_DIR_OUT      2


#define MAX_VA_SAMPLES     128 // was 4096 but must be less for w2k
#define SAMPLE_SIZE        SRDAT_USBPACKET_SAMPLE_SIZE  // = 32 = 8 sanity/pps/gps/obc/etc + 24 DATA
#define VALUESARRAY_SIZE   (MAX_VA_SAMPLES*SAMPLE_SIZE)

#define MAX_PACKETS        64 // like MAX_VA_SAMPLES, is max packets per ioctl call

#define MAX_PACKETS_BULK      4000 // max packets per ioctl call for bulk pipe data
#define MAX_PACKETS_CONTROL   1000 // max packets per ioctl call for control pipe data


// Note: According to the MSDN page "Setting USB Transfer and Packet Sizes" the
//       maximum pipe transfer sizes in Win 2K, XP, and 2003 Server are as
//       follows:
//
// Control    1000 64 byte packets (only 62.5 packets or 4000 bytes for Alt 0)
// Bulk/Intr  4000 64 byte packets (256K bytes)
// Iso        256 frames
//
// Win Vista is the same for control but upto 3.5 MB for Bulk/Intr and Iso
//
// USBD_PIPE_INFORMATION field MaximumTransferSize is valid for Win2K but
// obsolete for later OS's




// Debugging

int ShowDbgLib           = 0;
int ShowDbgLibRead       = 0;
int ShowDbgLibReadBoard  = 0;
int ShowDbgLibCommentOut = 0;


char       DbgStr[MAXSTR], DbgStrT[MAXSTR];
char       TempStr[MAXSTR];
char       CurrTimeStr[25]; // need 23+1 for "%04d/%02d/%02d %02d:%02d:%02d.%03d"



// User Callable Function Prototypes are located in SrUsbXch.h

// Function Prototypes
// Helper Functions

// NOTE: This duplicates a function in SrHelper, but we don't want to mix libraries
int SrUsbXchGetPcTime( int *Year, int *Month, int *Day, 
                       int *Hour, int *Minute, int *Second,
                       long *Microsecond );

void SrUsbXchHelperAnalogDemux( unsigned char *AdData, unsigned long *DemuxedValue );

int  SrUsbXchHelperSetPipeNum( int PipeType, int PipeDirection, ULONG *PipeNum );
int  SrUsbXchHelperDownload( DEVHANDLE UsbXchHandle, char **HwData );
int  SrUsbXchHelper8051RunHold( DEVHANDLE UsbXchHandle, int RunHold );
int  SrUsbXchHelperConfigure( DEVHANDLE UsbXchHandle,
                              int UserCfg,
                              int GpsModel,
                              int *Error );
int  SrUsbXchHelperSpsRequest( double SpsRequested,
                               double *SpsActual,
                               int *SpsIndex,
                               int *SpsCode,
                               int *Error );
int  SrUsbXchHelperSpsLookup( int SpsIndex,
                              double *SpsActual,
                              int *SpsCode,
                              int *Error );
int  SrUsbXchHelperSpsInit( DEVHANDLE UsbXchHandle, int SpsIndex, int *Error );


int  SrUsbXchHelperAcqRunInit( void );
int  SrUsbXchHelperAcqRunFree( void );
int  SrUsbXchHelperPpsCountCompare( int ExistingPpsCount, int NewPpsCount );
int  SrUsbXchHelperTsFillFromSample( SRUSBXCH_PACKET *pPacket, int *Error );
int  SrUsbXchHelperTsFillFromStatus( SRUSBXCH_PACKET *pPacket, int *Error );
int  SrUsbXchHelperTsCombine( int *Error );
int  SrUsbXchHelperIsValidOBC( int PpsToggle, int *Obc, int PpsCount );
int  SrUsbXchHelperTsProcessValid( int *Error );
int  SrUsbXchHelperTsComputeTime( TS *TS0, TS *TS1, int CurrentPt, double *SampleTime );
int  SrUsbXchHelperTcxoComputeTime( double StartTime, double SamplePeriod,
                                    int CurrentPt, double *SampleTime );
int  SrUsbXchHelperProcessPackets( int Bounded, int *Error );
int  SrUsbXchHelperPktCheckSanity( SRUSBXCH_PACKET *CurrentPacket, int nTotalPackets );
int  SrUsbXchHelperExtractNmeaInfo( char *NmeaGroup, int NmeaCount, NI *NmeaInfo );



// OS Dependent Functions

// The following define controls whether the OS dependent functions are
// visible to other source files.  These are for device driver EXPERTS
// ONLY.  Most users should let SRLOCAL be defined as static to avoid
// namespace clutter and possible function name contention.  The only
// exceptions would be for specialized debugging and diagnostic programs.


#if !defined( SRLOCAL )
#define SRLOCAL static
#endif

// Prototypes of OS specific helper functions

SRLOCAL DEVHANDLE SrUsbXchOsDriverOpen( char *DriverName );
SRLOCAL int  SrUsbXchOsDriverClose( DEVHANDLE UsbXchHandle );
SRLOCAL int  SrUsbXchOsDriverIoctl( DEVHANDLE UsbXchHandle,
                                unsigned long IoCtlCode,
                                void *pValueIn, unsigned long InSize,
                                void *pValueOut, unsigned long  OutSize,
                                unsigned long *ReturnSize );
SRLOCAL int SrUsbXchOsDriverRead( DEVHANDLE      UsbXchHandle,
				  void          *pValues,
				  unsigned long  BytesToRead,
				  unsigned long *pBytesRead );

SRLOCAL long SrUsbXchOsGetLastError( void );
SRLOCAL void SrUsbXchOsSleep( int ms );









// FILE: SrDat.c
// COPYRIGHT: (c), Symmetric Research, 2009
//      
// Source file containing functions for working with the SR .dat file format.
//
// Change History:
//   2009/02/24  WCT  Upgraded to include all needed data types like GPS + USBxCH
//   2008/05/01  WCT  Upgraded to support USBxCH data
//   2006/02/25  WCT  Upgraded to support SRDAT_REV 106
//
//

//WCT - consider changing all functions to FUNCTYPE


// Defines for processing 24 bit data values

#define NEG24BIT           0x00800000L
#define SIGNEXTEND         0xFF000000L


// Defines for processing roll over counts

#define GGACOUNTROLL    0x000F
#define SERIALCOUNTROLL 0x000000FF



// Global variables
   
char ErrMsg[256];   // Used for error messages

char *NmeaMsgName[SRDAT_NMEA_MAX_TYPE+1]; // Ordered NMEA names, +1 = room for NULL
int   NmeaMsgIdSave;                      // NMEA name to match for saving data
int   NmeaMsgIdLast;                      // Last NMEA name in any second

#define MAX_SERIALBUFFER        6000
char  SerialBuffer[MAX_SERIALBUFFER];     // Circular buffer for serial NMEA data
int   SbUsePtr, SbFillPtr, SbWrap;        // Pointers into circular buffer
int   SbLastSecondCount, SbNmeaCount;     // Additional serial parsing data

// Expected sanity bit for A/D data so we can check that we're in synch
// It must be masked by the number of bits appropriate for each USBxCH

unsigned char ExpectedAnalogSanity;
unsigned char ExpectedSerialSanity;

// Base value for keeping track of total PPS count

unsigned int  LastSerialPpsEventNum;
unsigned int  LastAnalogPpsEventNum;


// Serial and equipment variables

#define MAX_SERIALSTRUCT      4           // Set number of gps nmea structs per pass

SRDAT_USBXCH_SERIAL    SerialData;        // Serial structure for dat file
SRDAT_USBXCH_EQUIPMENT EquipmentData;     // Equipment structure for dat file



#define TOLERANCE 0.00001       // Used for comparing doubles
#define SRCSTR_UNK  10





// Prototypes for helper functions not called by external programs

int  SrDatHdrUpdateRev( SRDAT_HDR *Hdr );
void SrDatError( char *Msg );
void DumpPacket( SRUSBXCH_PACKET *Packet );
void DumpBytes( char *Array, int Nbytes );

//WCT - debugging function only
int SerBufShow( void );




//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrInit
// PURPOSE: Initialize a new .dat header with known default values.
//------------------------------------------------------------------------------
int SrDatHdrInit( SRDAT_HDR *Hdr ) {

        int i;


        // Initialize a new .dat header
        
        memset( Hdr, 0, SRDAT_HDR_LAYOUTSIZE );
        
        Hdr->DatId                    = SRDAT_HDR_ID; // SRSR
        Hdr->DatRev                   = SRDAT_REV;

        strcpy( Hdr->DatProduct,        "SR" );
        strcpy( Hdr->DatApplication,    "SR" );
        
        Hdr->DatAtodModel             = SRDAT_ATODMODEL_UNKNOWN;  // Added REV 106
        Hdr->DatGpsModel              = SRDAT_GPSMODEL_UNKNOWN;   // Added REV 106
        
        strcpy( Hdr->FileName,          "*.DAT" );
        
        Hdr->FileCreateTimeHi         = SRDAT_INVALID_LONG;          // Added REV 106
        Hdr->FileCreateTime           = SRDAT_INVALID_LONG;
        Hdr->FileSeqNum               = SRDAT_INVALID_LONG; 
        Hdr->FileStartPtNum           = SRDAT_INVALID_LONG;
        Hdr->FileNameType             = SRDAT_FILENAMETYPE_UNKNOWN;  // Added REV 106
        Hdr->FileNameInc              = 0;                           // Added REV 106

        for ( i = 0 ; i < SRDAT_DIM_TIME ; i++ ) {

                Hdr->TimePoint[i].Type                = SRDAT_TIME_TYPE_NONE;
                Hdr->TimePoint[i].SampleFromRun       = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].SampleFromFile      = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].GpsMarkIndex        = SRDAT_INVALID_LONG;
                
                Hdr->TimePoint[i].Year                = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].Month               = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].Day                 = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].YMDSource           = SRDAT_SOURCE_NONE;

                Hdr->TimePoint[i].Hour                = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].Minute              = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].Second              = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].Microsecond         = SRDAT_INVALID_LONG;
                Hdr->TimePoint[i].HMSSource           = SRDAT_SOURCE_NONE;
                Hdr->TimePoint[i].Unused1             = SRDAT_INVALID_LONG;

                }
        
        Hdr->Channels                 = 0;
        Hdr->ChannelsAnalog           = 0;                       // Added REV 106
        Hdr->ChannelsDigital          = 0;                       // Added REV 106
        Hdr->ChannelsGps              = 0;                       // Added REV 106
        Hdr->ChannelsCounter          = 0;                       // Added REV 106

        Hdr->ChannelMarkIndex         = SRDAT_INVALID_LONG;
        Hdr->ChannelDigIndex          = SRDAT_INVALID_LONG;      // Added REV 106
        Hdr->ChannelObcIndex          = SRDAT_INVALID_LONG;      // Added REV 106
        Hdr->ChannelPtsPerRecord      = SRDAT_INVALID_LONG;
        Hdr->ChannelPtsPerFile        = SRDAT_INVALID_LONG;

        for ( i = 0 ; i < SRDAT_DIM_CHANNELS ; i++ ) {
                Hdr->ChannelList[i].Channel = i;
                sprintf( Hdr->ChannelList[i].Title, "CH%02d", i );
                }

        Hdr->DigitalPerChan           = 0;                      // Added REV 106
        for ( i = 0 ; i < SRDAT_DIM_DIGITAL ; i++ ) {           // Added REV 106
                Hdr->DigitalList[i].Channel = i;
                sprintf( Hdr->DigitalList[i].Title, "CH%02d", i );
                }


        Hdr->AtodWordSize             = 4;
        Hdr->AtodDataCoding           = SRDAT_DATACODING_OFFSET;
        Hdr->AtodSamplingRate         = SRDAT_INVALID_DOUBLE;
        Hdr->AtodSamplingRateMeasured = SRDAT_INVALID_DOUBLE;
        Hdr->AtodGain                 = 1;
        Hdr->AtodFilterCoeff          = 0.0;
        Hdr->AtodFilterScale          = 1.0;
        
        Hdr->CounterType              = SRDAT_COUNTERTYPE_INT64;
        Hdr->CounterFreq              = SRDAT_INVALID_LONG;
        Hdr->CounterFreqHi            = SRDAT_INVALID_LONG;        // Added REV 106

        Hdr->Counter2Type             = SRDAT_COUNTERTYPE_INT64;   // Added REV 106
        Hdr->Counter2Freq             = SRDAT_INVALID_LONG;        // Added REV 106
        Hdr->Counter2FreqHi           = SRDAT_INVALID_LONG;        // Added REV 106
        
        Hdr->LocationLatitude         = SRDAT_INVALID_DOUBLE;
        Hdr->LocationLongitude        = SRDAT_INVALID_DOUBLE;
        Hdr->LocationAltitude         = SRDAT_INVALID_DOUBLE;
        Hdr->LocationSource           = SRDAT_SOURCE_NONE;

        Hdr->TriggerType              = SRDAT_TRIGGERTYPE_UNKNOWN; // Added REV 106
        Hdr->TriggerParm1             = SRDAT_INVALID_LONG;        // Added REV 106
        Hdr->TriggerParm2             = SRDAT_INVALID_LONG;        // Added REV 106
        
        Hdr->LocationUnused1          = SRDAT_INVALID_LONG;
        Hdr->DigitalUnused1           = SRDAT_INVALID_LONG;        // Added REV 106
        Hdr->TriggerUnused1           = SRDAT_INVALID_LONG;        // Added REV 106

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrSetFileParm
// PURPOSE: Fill in the file information fields in a .dat header.
//------------------------------------------------------------------------------
int SrDatHdrSetFileParm( SRDAT_HDR *Hdr,
                         char *FileName,
                         long FileCreateTime,
                         long FileSeqNum,
                         long FileStartPtNum ) {


        // Set file information

        if (FileName != NULL) 
                strncpy( Hdr->FileName, FileName, SRDAT_DIM_STRING-1 );
        
        if (FileCreateTime != SRDAT_INVALID_LONG) {
                Hdr->FileCreateTimeHi = 0L;
                Hdr->FileCreateTime = FileCreateTime;
                }

        if (FileSeqNum >= 0)
                Hdr->FileSeqNum = FileSeqNum;

        if (FileStartPtNum >= 0)
                Hdr->FileStartPtNum = FileStartPtNum;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrSetTimeType
// PURPOSE: Fill in the .dat header fields for the specified time point with
//          the type and sample number info.
//------------------------------------------------------------------------------
int SrDatHdrSetTimeType( SRDAT_HDR *Hdr, int WhichTime, int Type, 
                         int SampleFromRun, int SampleFromFile,
                         int GpsMarkIndex  ) {

        // Check values

        if ( (WhichTime < 0) || (WhichTime > SRDAT_DIM_TIME-1) )
                return( 0 );
        

        // Set valid Type and sample from values

        Hdr->TimePoint[WhichTime].Type           = Type;
        Hdr->TimePoint[WhichTime].SampleFromRun  = SampleFromRun;
        Hdr->TimePoint[WhichTime].SampleFromFile = SampleFromFile;
        Hdr->TimePoint[WhichTime].GpsMarkIndex   = GpsMarkIndex;
        

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrSetTimeYMD
// PURPOSE: Fill in the .dat header fields for the specified time point with
//          the year, month and day info.
//------------------------------------------------------------------------------
int SrDatHdrSetTimeYMD( SRDAT_HDR *Hdr, int WhichTime,
                        int Year, int Month, int Day, int Source ) {

        // Check values

        if ( (WhichTime < 0)    || (WhichTime > SRDAT_DIM_TIME-1) ||
                  (Year < 1900) ||      (Year > 2100)             ||
                 (Month < 1)    ||     (Month > 12)               ||
                   (Day < 1)    ||       (Day > 31)               )
                return( 0 );
        

        // Set a valid date

        Hdr->TimePoint[WhichTime].Year       = Year;
        Hdr->TimePoint[WhichTime].Month      = Month;
        Hdr->TimePoint[WhichTime].Day        = Day;
        Hdr->TimePoint[WhichTime].YMDSource  = Source;
        

        return( 1 );
}

#define USECPER 1000000         // Microseconds per second

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrSetTimeHMS
// PURPOSE: Fill in the .dat header fields for the specified time point with
//          the hour, minute and second info.
//------------------------------------------------------------------------------
int SrDatHdrSetTimeHMS( SRDAT_HDR *Hdr, int WhichTime,
                        int hh, int mm, int ss, long usec, int Source ) {

        // Check values

        if ( (WhichTime < 0) || (WhichTime > SRDAT_DIM_TIME-1) ||
                    (hh < 0) ||        (hh > 23)               ||
                    (mm < 0) ||        (mm > 59)               ||
                    (ss < 0) ||        (ss > 59)               ||
                  (usec < 0) ||      (usec > USECPER)          )
                return( 0 );
        

        // Set a valid time

        Hdr->TimePoint[WhichTime].Hour        = hh;
        Hdr->TimePoint[WhichTime].Minute      = mm;
        Hdr->TimePoint[WhichTime].Second      = ss;
        Hdr->TimePoint[WhichTime].Microsecond = usec;
        Hdr->TimePoint[WhichTime].HMSSource   = Source;
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrSetLocation
// PURPOSE: Fill in the .dat header fields for the GPS location info.
//------------------------------------------------------------------------------
int SrDatHdrSetLocation( SRDAT_HDR *Hdr, long Latitude, long Longitude, 
                         long Altitude, int Source ) {

        // Check values

        if ( (Longitude < -360) || (Longitude > 360) ||
              (Latitude < -360) ||  (Latitude > 360) )
                return( 0 );
        

        // Set a valid location

        Hdr->LocationLongitude = Longitude;
        Hdr->LocationLatitude  = Latitude;
        Hdr->LocationAltitude  = Altitude;
        Hdr->LocationSource    = Source;
        

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrValidate
// PURPOSE: Check some of the .dat header fields to verify that we are really
//          working with a valid .dat file.
//------------------------------------------------------------------------------
int SrDatHdrValidate( SRDAT_HDR *Hdr ) {

        if (Hdr->DatId != SRDAT_HDR_ID) {
                SrDatError( "SRDAT found bad ID.  This is not a valid .DAT file." );
                return( 0 );
                }

        if ( (strlen(Hdr->DatProduct)     > SRDAT_DIM_STRING) ||
             (strlen(Hdr->DatApplication) > SRDAT_DIM_STRING) ||
             (strlen(Hdr->FileName)       > SRDAT_DIM_STRING) ) {
                SrDatError( "SRDAT found invalid strings in .DAT header." );
                return( 0 );
                }

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrUpdateRev
// PURPOSE: The .dat header has gone through several revisions.  This function
//          tries to compensate for files containing older revisions by filling
//          in default values for the new fields.
//------------------------------------------------------------------------------
int SrDatHdrUpdateRev( SRDAT_HDR *Hdr ) {

        int  i;
        long AtodModel, GpsModel, DigIndex, ObcIndex, DigPerChan;
        long Nanalog, Ndigital, Ngps, Nobc;

        // Error check

        if ( !Hdr ) {
                SrDatError( "SRDAT Can not update null header" );
                return( 0 );
                }


        // Validate header

        if ( !SrDatHdrValidate( Hdr ) )
             return( 0 );
        

        // Program and file header versions are the same,
        // so no updates need to be done.

        if (Hdr->DatRev == SRDAT_REV)
                return( 1 );

        // Program header version is older than the file version,
        // so we don't know how to properly interpret all the file
        // info.  Please update your software.
        
        if (Hdr->DatRev > SRDAT_REV) {
                sprintf( ErrMsg, "SRDAT Header version mismatch (%ld in file, %d in program).\nPlease update your software.\n",
                         Hdr->DatRev, SRDAT_REV );
                SrDatError( ErrMsg );
                return( 0 );
                }


        // Program header version is newer than the file version,
        // so we update the added info with appropriate defaults to
        // compensate.

        if (Hdr->DatRev <= 105) {       // Update to Rev 106

                // Choose best A/D model:
                // Choose best Digital channel:
                // Choose best OnBoardCounter channel:
                // Choose number of analog, digital, gps, and counter channels:
                
                // All PARxCH start out with DatProduct string of "SR PARxCH"
                // Check for specific model based on number of channels
                //   8CH has 8 analog (+1 dig +1 gps +1 obc) = 8 to 11
                //   4CH has 4 analog (+1 gps)               = 4 or 5
                //   1CH has 1 analog (+1 gps)               = 1 or 2
        
                AtodModel  = SRDAT_ATODMODEL_UNKNOWN;
                DigIndex   = SRDAT_INDEXCHANNEL_NONE ;
                ObcIndex   = SRDAT_INDEXCHANNEL_NONE;
                Nanalog    = 0;
                Ndigital   = 0;
                Ngps       = 0;
                Nobc       = 0;
                DigPerChan = 0;

                if ( strncmp( "SR PARxCH", Hdr->DatProduct, 9 ) == 0  ) {

                        if (Hdr->Channels == 1) {
                                AtodModel = SRDAT_ATODMODEL_PAR1CH;
                                Nanalog   = 1;
                                }

                        else if (Hdr->Channels == 2) {
                                AtodModel = SRDAT_ATODMODEL_PAR1CH;
                                Nanalog   = 1;
                                Ngps      = 1;
                                }

                        else if (Hdr->Channels == 4) {
                                AtodModel = SRDAT_ATODMODEL_PAR4CH;
                                Nanalog   = 4;
                                }

                        else if (Hdr->Channels == 5) {
                                AtodModel = SRDAT_ATODMODEL_PAR4CH;
                                Nanalog   = 4;
                                Ngps      = 1;
                                }

                        else if (Hdr->Channels == 8) {
                                AtodModel = SRDAT_ATODMODEL_PAR8CH;
                                Nanalog   = 8;
                                }

                        else if (Hdr->Channels == 9) {
                                AtodModel = SRDAT_ATODMODEL_PAR8CH;
                                Nanalog   = 8;
                                Ndigital  = 1;
                                DigIndex  = Nanalog; // 8
                                }

                        else if (Hdr->Channels == 10) {
                                AtodModel = SRDAT_ATODMODEL_PAR8CH;
                                Nanalog   = 8;
                                Ngps      = 1;
                                Nobc      = 1;
                                ObcIndex  = Nanalog + Ngps; // 9
                                }

                        else if (Hdr->Channels == 11) {
                                AtodModel = SRDAT_ATODMODEL_PAR8CH;
                                Nanalog   = 8;
                                Ndigital  = 1;
                                Ngps      = 1;
                                Nobc      = 1;
                                DigIndex  = Nanalog;                   // 8
                                ObcIndex  = Nanalog + Ndigital + Ngps; // 10
                                }

                        if (Ndigital == 1)
                                DigPerChan = 4;
                }



                // Choose best GPS model:
                // If the mark channel is set, then the file include GPS 
                // and the default GPS model is the PARGPS.

                if ( Hdr->ChannelMarkIndex != SRDAT_INDEXCHANNEL_NONE )
                        GpsModel = SRDAT_GPSMODEL_TRIMBLE;
                else
                        GpsModel = SRDAT_GPSMODEL_NONE;

                


                // Set defaults

                Hdr->DatAtodModel     = AtodModel;
                Hdr->DatGpsModel      = GpsModel;
                Hdr->ChannelsAnalog   = Nanalog;
                Hdr->ChannelsDigital  = Ndigital;
                Hdr->ChannelsGps      = Ngps;
                Hdr->ChannelsCounter  = Nobc;
                Hdr->ChannelDigIndex  = DigIndex;
                Hdr->ChannelObcIndex  = ObcIndex;
                Hdr->DigitalPerChan   = DigPerChan;
                Hdr->FileCreateTimeHi = 0L;
                Hdr->FileNameType     = SRDAT_FILENAMETYPE_UNKNOWN;
                Hdr->FileNameInc      = 0;
                Hdr->CounterFreqHi    = 0L;
                Hdr->Counter2Type     = SRDAT_COUNTERTYPE_UNKNOWN;
                Hdr->Counter2FreqHi   = SRDAT_INVALID_LONG;
                Hdr->Counter2Freq     = SRDAT_INVALID_LONG;
                Hdr->TriggerType      = SRDAT_TRIGGERTYPE_UNKNOWN;
                Hdr->TriggerParm1     = SRDAT_INVALID_LONG;
                Hdr->TriggerParm2     = SRDAT_INVALID_LONG;


                // Fill in default digital titles

                for ( i = 0 ; i < Hdr->DigitalPerChan ; i++ ) {
                        Hdr->DigitalList[i].Channel = i;
                        sprintf( Hdr->DigitalList[i].Title, "DIG%d", i);
                        }


                } // end DatRev <= 105

        
//        if (Hdr->DatRev <= 106) {       // Update to Rev 107 when defined
//                }
        

        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrDatError
// PURPOSE: Print a timestamped message to the SRDAT.ERR error file.
//------------------------------------------------------------------------------
void SrDatError( char *Msg ) {

        FILE *ErrFp;
        char *DateStr, Dbuffer[16];
        char *TimeStr, Tbuffer[16];
        time_t CurrentTime;
        struct tm *LocalCurrentTime;



        // Get the date and time.

        time( &CurrentTime );
        LocalCurrentTime = localtime( &CurrentTime );

        strftime( Dbuffer, 16, "%m/%d/%y", LocalCurrentTime );
        strftime( Tbuffer, 16, "%H:%M:%S", LocalCurrentTime );

        DateStr = Dbuffer;
        TimeStr = Tbuffer;



        // Append the error.

        ErrFp = fopen( "SRDAT.ERR", "a" );

        if ( ErrFp ) {

                fprintf( ErrFp, "[%s] [%s] : %s\n", DateStr, TimeStr, Msg );

                   fclose( ErrFp );

                }



        // Display the error and quit.

        printf( "ERROR[srdat]: %s\n", Msg );

        return;
}


//------------------------------------------------------------------------------
// NOTE: The following functions perform I/O on the .DAT file and have been
//       changed.  They now use the C runtime stream I/O functions which take
//       a FILE* file pointer to identify the file instead of the low level I/O 
//       functions which take an int file descriptor.  This change was made so
//       that MFC code can use the CStdioFile class to get a file pointer and 
//       call these functions.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrWrite
// PURPOSE: Write the provided .dat header structure out to the specified
//          and already open file.  The padding array is defined as static to
//          save on stack size.  It is used to pad the header layout structure 
//          from srdat.h to the complete .DAT header length of 4096.
//------------------------------------------------------------------------------
int SrDatHdrWrite( FILE *fp, SRDAT_HDR *Hdr ) {

        static char SrDatHdrPadding[SRDAT_HDR_PADDINGSIZE];

        // Check hdr size
        
        if (sizeof(*Hdr) > SRDAT_HDR_TOTALSIZE) {
                SrDatError( "SRDAT found hdr size is too large" );
                return( 0 );
                }

        
        // Write new .dat header

        if ( fwrite( Hdr, 1, SRDAT_HDR_LAYOUTSIZE, fp  ) != SRDAT_HDR_LAYOUTSIZE ) {
                SrDatError( "SRDAT failed to write filled part of hdr" );
                return( 0 );
                }

        if ( fwrite( SrDatHdrPadding, 1, SRDAT_HDR_PADDINGSIZE, fp ) !=
                                                     SRDAT_HDR_PADDINGSIZE ) {
                SrDatError( "SRDAT failed to write padded part of hdr" );
                return( 0 );
                }


        if ( fflush( fp ) != 0 ) {
                SrDatError( "SRDAT can't commit hdr to disk" );
                return( 0 );
                }

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrRead
// PURPOSE: Read in the .dat header from the specified and already open file
//          into the provided header structure.  See srdat.h for a description
//          of this structure.
//------------------------------------------------------------------------------
int SrDatHdrRead( FILE *fp, SRDAT_HDR *Hdr ) {

        int  ret;
        long pos;

        // Error check

        if ( !Hdr ) {
                SrDatError( "SRDAT can not read a header into a null structure" );
                return( 0 );
                }

        
        // Check amount to read
        
        if (SRDAT_HDR_LAYOUTSIZE > SRDAT_HDR_TOTALSIZE) {
                SrDatError( "SRDAT found hdr size is too large" );
                return( 0 );
                }
                
        
        // Read filled layout part of .dat header

        if ( fread( Hdr, 1, SRDAT_HDR_LAYOUTSIZE, fp ) != SRDAT_HDR_LAYOUTSIZE ) {
                SrDatError( "SRDAT failed to read filled part of hdr" );
                return( 0 );
                }


        // Skip unfilled padding part of .dat header

        if (SRDAT_HDR_PADDINGSIZE >= 0) {
                pos = fseek( fp, SRDAT_HDR_PADDINGSIZE, SEEK_CUR );
                if ( pos == -1L ) {
                        SrDatError( "SRDAT failed to read padded part of hdr" );
                        return( 0 );
                        }
                }


        // Update read header with defaults so it is compatible with latest rev

        ret = SrDatHdrUpdateRev( Hdr );

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatHdrSkip
// PURPOSE: Skip over a .dat header in the specified and already open file.
//          One might want to do this if the header was read on a previous
//          pass.
//------------------------------------------------------------------------------
int SrDatHdrSkip( FILE *fp ) {
        
        long pos;

        // Skip header by positioning after it starting from the beginning

        pos = fseek( fp, SRDAT_HDR_TOTALSIZE, SEEK_SET );
        
        if ( pos == -1L ) {
                SrDatError( "SRDAT failed to skip header" );
                return( 0 );
                }


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatRecordAllocate
// PURPOSE: Allocate contiguous memory for both the tag and data portions of a
//          complete .dat record.  Inputs include the type and size for the
//          data portion of the record.  Remember to call SrDatRecordFree to
//          release the allocated memory when you are finished using the record.
//
//          NOTE: Only use this function for records with BOTH tag and data.
//          If there is no associated data, just work with the tag structure
//          instead.
//------------------------------------------------------------------------------
int SrDatRecordAllocate( long DataType, unsigned int Nbytes,
                         SRDAT_RECORD **PtrRecord ) {

        SRDAT_RECORD *PtrRecordLocal;
        unsigned int TotalBytes;

        // Error check, record pointer invalid or no data

        if ( !PtrRecord || Nbytes == 0 )
                return( 0 );


        *PtrRecord = NULL;
        
        TotalBytes = Nbytes + SRDAT_TAGSIZE;

        PtrRecordLocal = (SRDAT_RECORD *)malloc( TotalBytes );
        if ( !PtrRecordLocal )
                return( 0 );

        PtrRecordLocal->tag.TagId   = (unsigned long)DataType;
        PtrRecordLocal->tag.Nbytes  = (unsigned long)Nbytes;
        PtrRecordLocal->tag.Unused1 = 0L;
        PtrRecordLocal->tag.Unused2 = 0L;

        *PtrRecord = PtrRecordLocal;
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatRecordFree
// PURPOSE: Free the record memory allocated by SrDatRecordAllocate and set the
//          Record variable to NULL indicating it is no longer valid.
//------------------------------------------------------------------------------
int SrDatRecordFree( SRDAT_RECORD **PtrRecord ) {

        // Error check, record already freed

        if ( !PtrRecord )
                return( 0 );


        // Free record memory and invalidate record pointer

        free( *PtrRecord );
        *PtrRecord = NULL;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatRecordPtrTag
// PURPOSE: Return a pointer to the tag portion of a complete .dat record.
//------------------------------------------------------------------------------
int SrDatRecordPtrTag( SRDAT_RECORD *Record, SRDAT_TAG **PtrTag ) {

        // Error check, invalid record or ptrtag

        if ( !Record || !PtrTag )
                return( 0 );


        // Point to start of record tag

        *PtrTag = (SRDAT_TAG *)&(Record->tag);
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatRecordPtrData
// PURPOSE: Return a pointer to the data portion of a complete .dat record.
//------------------------------------------------------------------------------
int SrDatRecordPtrData( SRDAT_RECORD *Record, char **PtrData ) {

        // Error check, invalid record or ptrdata

        if ( !Record || !PtrData )
                return( 0 );
        


        // Point to start of record data

        *PtrData = (char *)&(Record->data);

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatRecordWriteFast
// PURPOSE: Write a .dat record out to the specified and already open file.  
//          Each record consists of a tag followed by the raw data bytes.
//          This function writes the entire record in a single transaction.
//------------------------------------------------------------------------------
int SrDatRecordWriteFast( FILE *fp, SRDAT_RECORD *Record ) {

        unsigned long TotalBytes;
        

        // Error check, invalid file or record

        if ( !fp || !Record )
                return( 0 );


        // Determine total size of record

        TotalBytes = Record->tag.Nbytes + SRDAT_TAGSIZE;


        // Write complete record

        if ( fwrite( Record, 1, TotalBytes, fp ) != TotalBytes ) {
                SrDatError( "SRDAT failed to write complete record" );
                return( 0 );
                }

        if ( fflush( fp ) != 0 ) {
                SrDatError( "SRDAT can't commit complete record to disk" );
                return( 0 );
                }

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatRecordWrite
// PURPOSE: Write a .dat record out to the specified and already open file.  
//          Each record consists of a tag followed by the raw data bytes.
//------------------------------------------------------------------------------
int SrDatRecordWrite( FILE *fp, long DataType, void *Buffer, unsigned int Nbytes ) {

        SRDAT_TAG Tag;

        // Initialize tag

        Tag.TagId   = DataType;
        Tag.Nbytes  = Nbytes;
        Tag.Unused1 = 0L;
        Tag.Unused2 = 0L;

        
        // Write tag and data block

        if ( fwrite( &Tag, 1, SRDAT_TAGSIZE, fp ) != SRDAT_TAGSIZE ) {
                SrDatError( "SRDAT failed to write record tag" );
                return( 0 );
                }

        if (Nbytes > 0) {
                if ( fwrite( Buffer, 1, Nbytes, fp ) != (size_t)Nbytes ) {
                        SrDatError( "SRDAT failed to write record data" );
                        return( 0 );
                        }
                }


        if ( fflush( fp ) != 0 ) {
                SrDatError( "SRDAT can't commit tag & data to disk" );
                return( 0 );
                }

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatRecordRead
// PURPOSE: Read a .dat record tag and its associated data from the specified
//          and already open file.
//------------------------------------------------------------------------------
int SrDatRecordRead( FILE *fp, SRDAT_TAG *Tag, void *Buffer ) {

        unsigned int nbytes;
        

        // Read tag

        if ( !SrDatTagRead( fp, Tag ) )
                return( 0 );


        // Read associated data, if any
        
        if (Tag->Nbytes > 0) {
                
                // Check data size (only matters in DOS version)

                nbytes = (unsigned int)Tag->Nbytes;
                if ((unsigned long)nbytes != Tag->Nbytes)
                        return( 0 );

                // Read data

                if ( !SrDatDataRead( fp, Buffer, nbytes ) )
                        return( 0 );

                }


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTagRead
// PURPOSE: Read in a .dat record tag from the specified and already open file
//          into the provided tag structure.  See srdat.h for a description
//          of this structure.  Each data record begins with a tag describing
//          the size and type of the following data.
//------------------------------------------------------------------------------
int SrDatTagRead( FILE *fp, SRDAT_TAG *Tag ) {


        // Error check
        
        if ( !Tag ) {
                SrDatError( "SRDAT can not read a data tag into a null structure" );
                return( 0 );
                }


        // Check tag size
        
        if (sizeof(*Tag) != SRDAT_TAGSIZE) {
                SrDatError( "SRDAT found wrong size for record tag" );
                return( 0 );
                }

        
        // Read .dat tag

        if ( fread( Tag, 1, SRDAT_TAGSIZE, fp ) != SRDAT_TAGSIZE ) {
                SrDatError( "SRDAT failed to read record tag" );
                return( 0 );
                }


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatDataRead
// PURPOSE: Read in the data portion of .dat data record from the specified and
//          already open file into the provided buffer.  This version is the
//          most generic.  It just reads in the specified number of bytes.  It
//          does not know or make any assumptions about what the meaning or
//          interpretation of those bytes are.  You may prefer to use some of
//          the more specific SrDatDataReadXxx functions given below instead.
//------------------------------------------------------------------------------
int SrDatDataRead( FILE *fp, void *Buffer, unsigned int Nbytes ) {

        
        // Trivial case, nothing to read

        if (Nbytes == 0)
                return( 1 );
        

        // Error check
        
        if ( !Buffer ) {
                SrDatError( "SRDAT can not read data bytes into a null buffer" );
                return( 0 );
                }


        // Read data block

        if ( fread( Buffer, 1, Nbytes, fp ) != (size_t)Nbytes ) {
                SrDatError( "SRDAT failed to read record data" );
                return( 0 );
                }


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatDataReadUsbPacket
// PURPOSE: Read in the data portion of .dat data record from the specified and
//          already open file into the provided buffer.  This function assumes
//          the data bytes are arranged as packets of SRDAT_USBPACKET_SIZE 
//          characters and returns the number of packets (possibly including 
//          one partial packet).
//------------------------------------------------------------------------------
int SrDatDataReadUsbPacket( FILE *fp, void *Buffer, unsigned int Nbytes,
                            unsigned int *Npts ) {

        unsigned int Npackets;

        // Read raw bytes

        if ( !SrDatDataRead( fp, Buffer, Nbytes ) ) {
                if (Npts)  *Npts = 0;
                return( 0 );
                }
        

        // Do UsbPacket specific processing

        Npackets = Nbytes / SRDAT_USBPACKET_SIZE;
        if ( Npackets * SRDAT_USBPACKET_SIZE < Nbytes )
                Npackets++;

        if (Npts)  *Npts = Npackets;


        return( 1 );
}
//------------------------------------------------------------------------------
// ROUTINE: SrDatDataReadUsbAnalog
// PURPOSE: Read in the data portion of .dat data record from the specified and
//          already open file into the provided buffer.  This function assumes
//          the data bytes are arranged as SRDAT_USBXCH_ANALOG structures and 
//          returns the number of structures.
//------------------------------------------------------------------------------
int SrDatDataReadUsbAnalog( FILE *fp, void *Buffer, unsigned int Nbytes,
                            unsigned int *Npts ) {

        // Read raw bytes

        if ( !SrDatDataRead( fp, Buffer, Nbytes ) ) {
                if (Npts)  *Npts = 0;
                return( 0 );
                }
        

        // Do UsbAnalog specific processing
        
        if (Npts)  *Npts = Nbytes / sizeof( SRDAT_USBXCH_ANALOG );


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatDataReadUsbSerial
// PURPOSE: Read in the data portion of .dat data record from the specified and
//          already open file into the provided buffer.  This function assumes
//          the data bytes are arranged as SRDAT_USBXCH_SERIAL structures and 
//          returns the number of structures.
//------------------------------------------------------------------------------
int SrDatDataReadUsbSerial( FILE *fp, void *Buffer, unsigned int Nbytes,
                            unsigned int *Npts ) {

        // Read raw bytes

        if ( !SrDatDataRead( fp, Buffer, Nbytes ) ) {
                if (Npts)  *Npts = 0;
                return( 0 );
                }
        

        // Do UsbSerial specific processing
        
        if (Npts)  *Npts = Nbytes / sizeof( SRDAT_USBXCH_SERIAL );


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatDataReadUsbEquip
// PURPOSE: Read in the data portion of .dat data record from the specified and
//          already open file into the provided buffer.  This function assumes
//          the data bytes are arranged as SRDAT_USBXCH_EQUIPMENT structures and 
//          returns the number of structures.
//------------------------------------------------------------------------------
int SrDatDataReadUsbEquip( FILE *fp, void *Buffer, unsigned int Nbytes,
                            unsigned int *Npts ) {

        // Read raw bytes

        if ( !SrDatDataRead( fp, Buffer, Nbytes ) ) {
                if (Npts)  *Npts = 0;
                return( 0 );
                }
        

        // Do UsbEquip specific processing
        
        if (Npts)  *Npts = Nbytes / sizeof( SRDAT_USBXCH_EQUIPMENT );


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatDataSkip
// PURPOSE: Skip over the data bytes in a .dat data record in the specified and 
//          already open file.  One might want to do this if the data record is
//          not of a type your program understands or is interested in.
//------------------------------------------------------------------------------
int SrDatDataSkip( FILE *fp, unsigned int Nbytes ) {
        
        long pos;

        // Trivial case, nothing to skip

        if (Nbytes == 0)
                return( 1 );
        

        // Skip data block

        pos = fseek( fp, Nbytes, SEEK_CUR );
        if ( pos == -1L ) {
                SrDatError( "SRDAT failed to skip record data" );
                return( 0 );
                }

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchIsValidDriverName
// PURPOSE: This function takes a driver name and determines whether or not it
//          is valid.  Valid names are of the form SrUsbXch# where # is a
//          single digit from 0 to 9.  Returns 1 if the supplied DriverName is 
//          valid, 0 otherwise.  If the name is valid and a DriverNum argument
//          was supplied, it is filled with the matching #.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchIsValidDriverName( char *DriverName, int *DriverNum ) {

        int   i, DrvNum, IsValid;
        char *DigitStr;


        // Error check

        if ( !DriverName )
                return( 0 );


        // Initialize variables

        DrvNum   = -1;
        IsValid  = 0;
        DigitStr = "0123456789";


        // Search for a valid match

        if ( ( strlen( DriverName ) == 9 ) &&   // check length
             ( DriverName[0] == 'S' )      &&   // check first 8 required chars
             ( DriverName[1] == 'r' )      &&
             ( DriverName[2] == 'U' )      &&
             ( DriverName[3] == 's' )      &&
             ( DriverName[4] == 'b' )      &&
             ( DriverName[5] == 'X' )      &&
             ( DriverName[6] == 'c' )      &&
             ( DriverName[7] == 'h' )      ) {

                // Check last character against allowed single digits
                
                for ( i = 0 ; i < 10 ; i++ ) {
                        
                        if ( DriverName[8] == DigitStr[i] ) { // found match
                                IsValid = 1;
                                DrvNum  = i;
                                break;
                                }
                        } // end for i
                
                } // end if strlen ...

        if ( DriverNum )
                *DriverNum = DrvNum;

        return( IsValid );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatAtodModelIndex
// PURPOSE: This function takes an A/D model name and selects the matching index.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatAtodModelIndex( char *AtodName, int *AtodModel ) {

        // Error check

        if ( !AtodName || !AtodModel )
                return( 0 );

//WCT - Add code to handle default model
        
        // Fill in index based on matching name
        
        if ( strcmp( AtodName, SRDAT_ATODNAME_PAR1CH ) == 0 )
                *AtodModel = SRDAT_ATODMODEL_PAR1CH;

        else if ( strcmp( AtodName, SRDAT_ATODNAME_PAR4CH ) == 0 )
                *AtodModel = SRDAT_ATODMODEL_PAR4CH;

        else if ( strcmp( AtodName, SRDAT_ATODNAME_PAR8CH ) == 0 )
                *AtodModel = SRDAT_ATODMODEL_PAR8CH;

        else if ( strcmp( AtodName, SRDAT_ATODNAME_USB1CH ) == 0 )
                *AtodModel = SRDAT_ATODMODEL_USB1CH;

        else if ( strcmp( AtodName, SRDAT_ATODNAME_USB4CH ) == 0 )
                *AtodModel = SRDAT_ATODMODEL_USB4CH;

        else if ( strcmp( AtodName, SRDAT_ATODNAME_USB8CH ) == 0 )
                *AtodModel = SRDAT_ATODMODEL_USB8CH;
        
        else
                *AtodModel = SRDAT_ATODMODEL_UNKNOWN;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatGpsModelIndex
// PURPOSE: This function takes a GPS model name and selects the matching index.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatGpsModelIndex( char *GpsName, int *GpsModel ) {

        // Error check

        if ( !GpsName || !GpsModel )
                return( 0 );

//WCT - Add code to handle default model


        // Fill in index based on matching name
        
        if ( strcmp( GpsName, SRDAT_GPSNAME_TRIMBLE ) == 0 )
                *GpsModel = SRDAT_GPSMODEL_TRIMBLE;

        else if ( strcmp( GpsName, SRDAT_GPSNAME_ONCORE ) == 0 )
                *GpsModel = SRDAT_GPSMODEL_ONCORE;

        else if ( strcmp( GpsName, SRDAT_GPSNAME_PCTIME ) == 0 )
                *GpsModel = SRDAT_GPSMODEL_PCTIME;

        else if ( strcmp( GpsName, SRDAT_GPSNAME_GARMIN ) == 0 )
                *GpsModel = SRDAT_GPSMODEL_GARMIN;
        
        else if ( strcmp( GpsName, SRDAT_GPSNAME_TCXO ) == 0 )
                *GpsModel = SRDAT_GPSMODEL_TCXO;
        
        else
                *GpsModel = SRDAT_GPSMODEL_UNKNOWN;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatAtodModelString
// PURPOSE: Translate SRDAT_ATODMODEL integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatAtodModelString( long AtodModel ) {

        // Note: The AtodModel contants are not consecutive integers
        // starting from 0, so we can't use the usual static char
        // array technique
        
        if ( AtodModel == SRDAT_ATODMODEL_PAR1CH )
                return( SRDAT_ATODNAME_PAR1CH );

        else if ( AtodModel == SRDAT_ATODMODEL_PAR4CH )
                return( SRDAT_ATODNAME_PAR4CH );

        else if ( AtodModel == SRDAT_ATODMODEL_PAR8CH )
                return( SRDAT_ATODNAME_PAR8CH );

        else if ( AtodModel == SRDAT_ATODMODEL_USB1CH )
                return( SRDAT_ATODNAME_USB1CH );

        else if ( AtodModel == SRDAT_ATODMODEL_USB4CH )
                return( SRDAT_ATODNAME_USB4CH );

        else if ( AtodModel == SRDAT_ATODMODEL_USB8CH )
                return( SRDAT_ATODNAME_USB8CH );

        else
                return( SRDAT_ATODNAME_UNKNOWN );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatGpsModelString
// PURPOSE: Translate SRDAT_GPSMODEL integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatGpsModelString( long GpsModel ) {
        
        static char *GpsModelString[] = {
                "none",
                "Trimble",
                "Oncore",
                "PcTime",
                "Garmin",
                "TCXO",    // Temperature Controlled Xtal (crystal) Oscillator
                "Unknown"
                };

        if ( GpsModel >= 0  &&  GpsModel <= SRDAT_GPSMODEL_MAX )
                return( GpsModelString[GpsModel] );
        else
                return( GpsModelString[SRDAT_GPSMODEL_MAX] );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTimeTypeString
// PURPOSE: Translate SRDAT_TIME_TYPE integer constants into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatTimeTypeString( long Type ) {

        static char *TimeTypeString[] = {
                "none",
                "first point",
                "last point",
                "first GPS point",
                "last GPS point",
                "first event",
                "last event",
                "first user defined point",
                "last user defined point",
                "unknown"
                };

        if ( Type >= 0  &&  Type <= SRDAT_TIME_TYPE_MAX )
                return( TimeTypeString[Type] );
        else
                return( TimeTypeString[SRDAT_TIME_TYPE_MAX] );

}

//------------------------------------------------------------------------------
// ROUTINE: SrDatSourceString
// PURPOSE: Translate SRDAT_SOURCE integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatSourceString( long Source ) {
        
        static char *SourceString[] = {
                "none",
                "from calculation",
                "from PC",
                "from GPS",
                "from user",
                "from acq program",
                "from GPS + on board counter",
                "unknown"
                };

        if ( Source >= 0  &&  Source <= SRDAT_SOURCE_MAX )
                return( SourceString[Source] );
        else
                return( SourceString[SRDAT_SOURCE_MAX] );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatDataCodingString
// PURPOSE: Translate SRDAT_DATACODING_ integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatDataCodingString( long DataCoding ) {
        
        static char *DataCodingString[] = {
                "offset",
                "signed",
                "unknown"
                };

        if ( DataCoding >= 0  &&  DataCoding <= SRDAT_DATACODING_MAX )
                return( DataCodingString[DataCoding] );
        else
                return( DataCodingString[SRDAT_DATACODING_MAX] );
}


//------------------------------------------------------------------------------
// ROUTINE: SrDatCounterTypeString
// PURPOSE: Translate SRDAT_COUNTERTYPE integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatCounterTypeString( long CounterType ) {
        
        static char *CounterTypeString[] = {
                "64 bit integer",
                "2 long words, Linux timeval",
                "other",
                "unknown"
                };

        if ( CounterType >= 0  &&  CounterType <= SRDAT_COUNTERTYPE_MAX )
                return( CounterTypeString[CounterType] );
        else
                return( CounterTypeString[SRDAT_COUNTERTYPE_MAX] );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatFileNameTypeString
// PURPOSE: Translate SRDAT_FILENAMETYPE integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatFileNameTypeString( long FileNameType ) {
        
        static char *FileNameTypeString[] = {
                "None",
                "Single",
                "Sequential",
                "SequentialHex",
                "Time",
                "StdOut",
                "YmdHms",
                "Unknown"
                };

        if ( FileNameType >= 0  &&  FileNameType <= SRDAT_FILENAMETYPE_MAX )
                return( FileNameTypeString[FileNameType] );
        else
                return( FileNameTypeString[SRDAT_FILENAMETYPE_MAX] );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTriggerTypeString
// PURPOSE: Translate SRDAT_TRIGGERTYPE integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatTriggerTypeString( long TriggerType ) {
        
        static char *TriggerTypeString[] = {
                "None",
                "Threshold",
                "DigitalLevel",
                "DigitalEdge",
                "Unknown",
                };

        if ( TriggerType >= 0  &&  TriggerType <= SRDAT_TRIGGERTYPE_MAX )
                return( TriggerTypeString[TriggerType] );
        else
                return( TriggerTypeString[SRDAT_TRIGGERTYPE_MAX] );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTriggerLevelString
// PURPOSE: Translate SRDAT_TRIGGERLEVEL integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatTriggerLevelString( long TriggerLevel ) {
        
        static char *TriggerLevelString[] = {
                "Low",
                "High",
                "Unknown",
                };

        if ( TriggerLevel >= 0  &&  TriggerLevel <= SRDAT_TRIGGERLEVEL_MAX )
                return( TriggerLevelString[TriggerLevel] );
        else
                return( TriggerLevelString[SRDAT_TRIGGERLEVEL_MAX] );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTriggerEdgeString
// PURPOSE: Translate SRDAT_TRIGGEREDGE integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatTriggerEdgeString( long TriggerEdge ) {
        
        static char *TriggerEdgeString[] = {
                "Falling",
                "Rising",
                "Unknown",
                };

        if ( TriggerEdge >= 0  &&  TriggerEdge <= SRDAT_TRIGGEREDGE_MAX )
                return( TriggerEdgeString[TriggerEdge] );
        else
                return( TriggerEdgeString[SRDAT_TRIGGEREDGE_MAX] );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTimeMethodString
// PURPOSE: Translate SRDAT_TIME_METHOD integer constant into a string.
//------------------------------------------------------------------------------
FUNCTYPE( char *) SrDatTimeMethodString( long TimeMethod ) {
        
        static char *TimeMethodString[] = {
                "Default time",
                "Sample rate time",
                "GPS time",
                "GPS+ time",
                "PC/NTP time",
                "Unknown Time Method",
                };

        if ( TimeMethod >= 0  &&  TimeMethod <= SRDAT_TIME_METHOD_MAX )
                return( TimeMethodString[TimeMethod] );
        else
                return( TimeMethodString[SRDAT_TIME_METHOD_MAX] );
}


//*****************************************************************
//                SERIAL AND NMEA PARSING FUNCTIONS
//*****************************************************************

//------------------------------------------------------------------------------
// ROUTINE: SrDatNmeaInfoInit
// PURPOSE: Initialize the provided NmeaInfo structure to default values.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatNmeaInfoInit( NI *NmeaInfo ) {

        // Error check

        if ( !NmeaInfo )
                return( 0 );

        NmeaInfo->Nsat         = 0;
        NmeaInfo->Year         = 0;
        NmeaInfo->Month        = 0;
        NmeaInfo->Day          = 0;
        NmeaInfo->Hour         = 0;
        NmeaInfo->Minute       = 0;
        NmeaInfo->Second       = 0;
        NmeaInfo->MicroSecond  = 0L;
        NmeaInfo->SecSince1970 = SRDAT_INVALID_DOUBLE;
        NmeaInfo->Latitude     = SRDAT_INVALID_DOUBLE;
        NmeaInfo->Longitude    = SRDAT_INVALID_DOUBLE;
        NmeaInfo->Altitude     = SRDAT_INVALID_DOUBLE;
        NmeaInfo->YmdSource    = SRDAT_SOURCE_NONE;    // WCT define level GPS or NMEA_MSGID_MIN
        NmeaInfo->HmsSource    = SRDAT_SOURCE_NONE;
        NmeaInfo->SatSource    = SRDAT_SOURCE_NONE;
        NmeaInfo->LocSource    = SRDAT_SOURCE_NONE;
        NmeaInfo->PosSource    = SRDAT_SOURCE_NONE;
        NmeaInfo->YmdIsValid   = 0;
        NmeaInfo->HmsIsValid   = 0;
        NmeaInfo->SatIsValid   = 0;
        NmeaInfo->LocIsValid   = 0;
        NmeaInfo->PosIsValid   = 0;
        NmeaInfo->SecIsValid   = 0;
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatNmeaOrderInit
// PURPOSE: To set the arrival order of NMEA messages according to which GPS
//          is being used.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatNmeaOrderInit( int GpsModel ) {

        
//WCT - Consolidate with SrUsbXchHelperConfig as this info MUST match

        // Garmin GPS 18 LVC is the default for PARGPS Rev D and beyond
        
        NmeaMsgName[0] = "GPRMC";
        NmeaMsgName[1] = "GPGGA";
        NmeaMsgName[2] = "GPGSA";
        NmeaMsgName[3] = "GPGSV";
        NmeaMsgName[4] = "GPGLL";
        NmeaMsgName[5] = "GPVTG";
        NmeaMsgName[6] = NULL;
        NmeaMsgName[7] = NULL;
        NmeaMsgIdSave  = SRDAT_NMEA_MSGID_GGA;
        NmeaMsgIdLast  = SRDAT_NMEA_MSGID_GGA;

        // SR PARGPS Rev C with Trimble ACE III

        if ( GpsModel == SRDAT_GPSMODEL_TRIMBLE ) { // or GPSMODEL_PARGPS
        
                NmeaMsgName[0] = "GPZDA";
                NmeaMsgName[1] = "GPGGA";
                NmeaMsgName[2] = "GPGLL";
                NmeaMsgName[3] = "GPVTG";
                NmeaMsgName[4] = "GPGSA";
                NmeaMsgName[5] = "GPGSV";
                NmeaMsgName[6] = "GPRMC";
                NmeaMsgName[7] = NULL;
                NmeaMsgIdSave  = SRDAT_NMEA_MSGID_GGA;
                NmeaMsgIdLast  = SRDAT_NMEA_MSGID_GGA;
                }


        // Oncore or other receivers using alphabetical order
        // Note: This option has not been well tested.
        
        else if ( GpsModel == SRDAT_GPSMODEL_ONCORE ) {
                NmeaMsgName[0] = "GPGGA";
                NmeaMsgName[1] = "GPGLL";
                NmeaMsgName[2] = "GPGSA";
                NmeaMsgName[3] = "GPGSV";
                NmeaMsgName[4] = "GPRMC";
                NmeaMsgName[5] = "GPVTG";
                NmeaMsgName[6] = "GPZDA";
                NmeaMsgName[7] = NULL;
                NmeaMsgIdSave  = SRDAT_NMEA_MSGID_GGA;
                NmeaMsgIdLast  = SRDAT_NMEA_MSGID_ZDA;
                }


        // When using PC time, there are no messages but
        // we may generate synthetic ZDA strings
        
        else if ( GpsModel == SRDAT_GPSMODEL_PCTIME ||
                  GpsModel == SRDAT_GPSMODEL_TCXO   ) {
                NmeaMsgName[0] = "GPZDA";
                NmeaMsgName[1] = NULL;
                NmeaMsgName[2] = NULL;
                NmeaMsgName[3] = NULL;
                NmeaMsgName[4] = NULL;
                NmeaMsgName[5] = NULL;
                NmeaMsgName[6] = NULL;
                NmeaMsgName[7] = NULL;
                NmeaMsgIdSave  = SRDAT_NMEA_MSGID_ZDA;
                NmeaMsgIdLast  = SRDAT_NMEA_MSGID_ZDA;
                }


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatNmeaPcTimeToZDA
// PURPOSE: Generate a synthetic ZDA NMEA string representing the current PC
//          time.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatNmeaPcTimeToZDA( char *ZdaString, int Nmax ) {

        int  Ok, CheckSum;
	int  Year, Month, Day, Hour, Minute, Second;
	long Microsecond;


        if ( !ZdaString  ||  Nmax < 34 )
                return( 0 );


	// Get system time with OS dependent helper function

	SrUsbXchGetPcTime( &Year, &Month, &Day, 
                           &Hour, &Minute, &Second,
                           &Microsecond );

	// Fill in string

	sprintf( ZdaString, "$GPZDA,%02d%02d%02d.%06ld,%02d,%02d,%04d,,*hh\r\n",
		 Hour, Minute, Second, Microsecond, Day, Month, Year );



	// Fill in checksum

        Ok = SrDatNmeaAddCheckSum( ZdaString, &CheckSum );

        if ( !Ok )
                return( 0 );
        else
                return( 1 );
}
//------------------------------------------------------------------------------
// ROUTINE: SrDatNmeaAddCheckSum
// PURPOSE: Compute NMEA checksum for the provided NmeaString.  Then add that
//          checksum to the end of the string.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatNmeaAddCheckSum( char *NmeaString, int *CheckSum ) {

        int check, count, len, maxcount;
        char *ns, checkchar[3];

        // Error check

        if ( CheckSum )  *CheckSum = 0;
        if ( !NmeaString )
                return( 0 );

//WCT - review for off by one errors

        len      = strlen( NmeaString );
        maxcount = SRDAT_NMEA_MAX_SIZE - 4; // max size - (check chars + CR/LF)
        if ( maxcount > len )
                maxcount = len;


        // Compute checksum between starting $ and ending * (non-inclusive)

        check = 0;
        count = 0;
        ns    = NmeaString;

        if ( *ns != '$' )
                return( 0 );
        
        ns++;
        
        while ( *ns != '*' && *ns != '\n' && count < maxcount ) {
                check ^= *ns;
                count++;
                ns++;
                }

        // If we ended at the star, all is good, so insert the checksum
        // into the string

        if ( *ns == '*' ) {
                sprintf( checkchar, "%02X", check );
                ns++;
                *ns = checkchar[0];
                ns++;
                *ns = checkchar[1];
                }
                
        if ( CheckSum )  *CheckSum = check;
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTimestampInit
// PURPOSE: Clear a timestamp structure in preparation for use.
//------------------------------------------------------------------------------
int SrDatTimestampInit( TS *TimeStamp ) {

        // Error check

        if ( !TimeStamp )
                return( 0 );


        // Initialize the provided time stamp structure

        TimeStamp->Valid        = SRDAT_VALID_NONE;
        TimeStamp->PpsCount     = -1;
        TimeStamp->Sample       = 0;
        TimeStamp->TotalTime    = 0.0L;
        TimeStamp->SecSince1970 = 0.0L;
        TimeStamp->ExtraSec     = 0.0L;
        TimeStamp->ObcCount     = 0;
        TimeStamp->NumSat       = 0;
        TimeStamp->YmdSource    = SRDAT_SOURCE_NONE;
        TimeStamp->HmsSource    = SRDAT_SOURCE_NONE;
        TimeStamp->PowerInfo    = USBXCH_POWER_UNKNOWN;
        TimeStamp->Temperature  = USBXCH_TEMP_UNKNOWN;

        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTimestampWriteFile
// PURPOSE: Write timestamp info into the specified file so it can be read later
//          with the SrDatTimestampReadFile function.  Returns 1 on success, 0 
//          on invalid arguments.
//------------------------------------------------------------------------------
#define FMT_TSTITLE "  PpsCount     Sample       TotalTime          ExtraSec   ObcCount  Sat Ysrc Hsrc Pwr  Temp\n"
//                   NNNNNNNNNN NNNNNNNNNN   FFFFFFFFFFFFFFFFF  FFFFFFFFFFFF  0xXXXXXXXX  NN  NN  NN    N  0xXXXX
#define FMT_TS      "%10d %10d   %17lf  %12lf  0x%08X  %2d  %2d  %2d    %1d  0x%04X\n"
int SrDatTimestampWriteFile( FILE *Fptr, TS *TimeStamp, int WriteTitle ) {

        if ( !Fptr || !TimeStamp )
                return( 0 );

        if ( WriteTitle )
                fprintf( Fptr, FMT_TSTITLE );

        fprintf( Fptr, FMT_TS,
                 TimeStamp->PpsCount,
                 TimeStamp->Sample,
                 TimeStamp->TotalTime,
//WCT add        TimeStamp->SecSince1970,                 
                 TimeStamp->ExtraSec,
                 TimeStamp->ObcCount,
                 TimeStamp->NumSat,
                 TimeStamp->YmdSource,
                 TimeStamp->HmsSource,
                 TimeStamp->PowerInfo,
                 TimeStamp->Temperature
               );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatTimestampReadFile
// PURPOSE: Read timestamp info from the specified file written with the
//          SrDatTimestampWriteFile function.  Returns 1 on success, 0 on 
//          failure or invalid input arguments
//------------------------------------------------------------------------------
int SrDatTimestampReadFile( FILE *Fptr, TS *TimeStamp, int SkipTitle ) {

        int nval;

        if ( !Fptr || !TimeStamp )
                return( 0 );

        if ( SkipTitle )
                SrDatSkipTitleLine( Fptr );


        nval = fscanf( Fptr, FMT_TS,
                       &TimeStamp->PpsCount,
                       &TimeStamp->Sample,
                       &TimeStamp->TotalTime,
//WCT add              &TimeStamp->SecSince1970,
                       &TimeStamp->ExtraSec,
                       &TimeStamp->ObcCount,
                       &TimeStamp->NumSat,
                       &TimeStamp->YmdSource,
                       &TimeStamp->HmsSource,
                       &TimeStamp->PowerInfo,
                       &TimeStamp->Temperature
                     );

        if ( nval == 10 )
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatSkipTitleLine
// PURPOSE: Read in an discard one line from the specified file.
//------------------------------------------------------------------------------
void SrDatSkipTitleLine( FILE *Fptr ) {

        int c;

        c = fgetc( Fptr );
        while ( c != EOF && c != '\n' )
                c = fgetc( Fptr );
}


//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbTemperatureCombine
// PURPOSE: To convert the high and low bytes returned from the temp sensor chip
//          into a single int.  TA0 is the high byte of the ambient temperature
//          and TA1 is the low byte.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrDatUsbTemperatureCombine( char TA0, char TA1, int *Temp ) {

        if ( !Temp )
                return;

        // MSB  sign  2^6  2^5  2^4  2^3  2^2  2^1  2^0
        // LSB  2^-1  2^-2 2^-3 2^-4  0    0    0    0
        
        // Pack high and low bytes into a single int.  High byte goes
        // into bits 15-8, low byte into bits 7-0.  To scale from the
        // resulting integer into a temperature in degrees C, multiply
        // by 2^-8 or divide by 2^8 = 256.
        
        *Temp = ( (((int)TA0) & 0x00FF) << 8 ) | ( (int)TA1 & 0x00FF );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbTemperatureCompute
// PURPOSE: To convert the combined temp sensor integer into degrees Centigrade
//          and Fahrenheit.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrDatUsbTemperatureCompute( int RawTemp,
                                             double *TempC, double *TempF ) {
        
        double dRaw, DegC, DegF;


        // Use defaults when temp is unknown

        if ( RawTemp == USBXCH_TEMP_UNKNOWN ) {
                DegC = USBXCH_TEMP_DEG_UNKNOWN;
                DegF = USBXCH_TEMP_DEG_UNKNOWN;
                }


        // Compute degrees Centigrade and Fahrenheit
        // /256 = /2^8 = *2^-8 = *.00390625
        // 1.8 = 9/5
        // DegC is accurate to 1/2 degree, DegF rounded to 1 degree

        else {
                dRaw = (double)(RawTemp);
                DegC = dRaw / 256.0;       
                DegF = ((int)(DegC * 1.8 + 0.5)) + 32.0; 
                }


        // Fill users variables with temperature results

        if ( TempC )  *TempC = DegC;
        if ( TempF )  *TempF = DegF;
}


//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbAnalogInit
// PURPOSE: To initialize a SRDAT_USBXCH_ANALOG structure, zeroing out previous
//          data.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatUsbAnalogInit( SRDAT_USBXCH_ANALOG *AnalogData ) {

        int i;

        if ( !AnalogData )
                return( 0 );
        
        AnalogData->PpsEventNum  = 0;
        AnalogData->Sanity       = 0;
        AnalogData->PpsToggle    = 0;
        AnalogData->DigitalIn    = 0;
        AnalogData->GGACount     = 0;
        AnalogData->OnBoardCount = 0L;

        for ( i = 0 ; i < SRDAT_USBXCH_MAX_ANALOG_CHAN ; i++ )
                AnalogData->AnalogData[i] = 0L;


        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbSerialInit
// PURPOSE: To initialize a SRDAT_USBXCH_SERIAL structure, zeroing out previous
//          strings.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatUsbSerialInit( SRDAT_USBXCH_SERIAL *SerialData ) {

        if ( !SerialData )
                return( 0 );
        
        SerialData->PpsEventNum = 0;
        SerialData->NmeaCount   = 0;
        SerialData->ValidFields = SRDAT_VALID_NONE;

        memset( SerialData->NmeaMsg, 0, SRDAT_NMEA_MAX_BUFF );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbEquipInit
// PURPOSE: To initialize a SRDAT_USBXCH_EQUIPMENT structure, zeroing out
//          previous data.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatUsbEquipInit( SRDAT_USBXCH_EQUIPMENT *EquipData ) {

        if ( !EquipData )
                return( 0 );
        
        EquipData->Nbytes      = SRDAT_USBXCH_MAX_EQUIPBYTES;
        EquipData->PpsEventNum = 0;
        EquipData->VoltageGood = USBXCH_POWER_UNKNOWN;
        EquipData->Temperature = USBXCH_TEMP_UNKNOWN;
        EquipData->DramFlags   = USBXCH_DRAMFLAGS_UNKNOWN;
        EquipData->Unused1     = 0;
        EquipData->Unused2     = 0;

        return( 1 );
}


//*****************************************************************
//                SERIAL AND NMEA PARSING FUNCTIONS
//*****************************************************************


//------------------------------------------------------------------------------
// ROUTINE: SrDatSanityReset
// PURPOSE: Reset expected analog and/or serial sanity value.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrDatSanityReset( int ResetAnalog, int ResetSerial ) {

        if ( ResetAnalog )
                ExpectedAnalogSanity  = 0x00;   // reset because of call to 8051 DramReset

        if ( ResetSerial )
                ExpectedSerialSanity  = 0x00;   // reset because of call to 8051 SerialReset
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatPpsEventNumReset
// PURPOSE: Reset last analog and/or serial PpsEventNum value.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrDatPpsEventNumReset( int ResetAnalog, int ResetSerial ) {

        if ( ResetAnalog )
                LastAnalogPpsEventNum = -1;     // must start at -1 to ignore very first OBC
        if ( ResetSerial )
                LastSerialPpsEventNum = 0;      // reset PPS counts
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPacketType
// PURPOSE: Return the type of a USBxCH 64 byte packet.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPacketType( SRUSBXCH_PACKET *Packet ) {

        int ptype;

/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...
/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...
/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...

        ptype = Packet->Bytes[SRDAT_USBPACKET_TYPE_BYTE]
                & SRDAT_USBPACKET_TYPE_MASK;

        return( ptype );

}
//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPacketSanity
// PURPOSE: Return the sanity for a USBxCH 64 byte packet.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPacketSanity( SRUSBXCH_PACKET *Packet ) {

        int sanity;

/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...
/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...
/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...

        sanity = Packet->Bytes[SRDAT_USBPACKET_SANITY_BYTE]
                 & SRDAT_USBPACKET_SANITY_MASK;

        return( sanity );

}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPacketSanity2
// PURPOSE: Return the secondary sanity for a sample type USBxCH 64 byte packet.
//          All packets have a primary sanity byte, but only sample type packets
//          include a secondary sanity byte.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPacketSanity2( SRUSBXCH_PACKET *Packet ) {

        int sanity2;

/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...
/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...
/// CLEAN UP THE SR DAT USB PACKET TYPES SO THIS READS BETTER ...

        sanity2 = Packet->Bytes[SRDAT_USBPACKET_SANITY_BYTE2]
                  & SRDAT_USBPACKET_SANITY_MASK;

        return( sanity2 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchConvertPackets
// PURPOSE: This function processes the provided packet data read from 
//          the USBXCH and splits it into Analog, Serial, and Equipment
//          structures.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned long ) SrUsbXchConvertPackets(
                                        SRUSBXCH_PACKET        *PacketArray,
                                        unsigned long           NpacketsRead,
                                        int                     CheckSanity,
                                        int                    *LastMsg,
                                        SRDAT_USBXCH_ANALOG    *AnalogValues,
                                        unsigned long           NanalogRequested,
                                        unsigned long          *NanalogRead,
                                        SRDAT_USBXCH_SERIAL    *SerialValues,
                                        unsigned long           NserialRequested,
                                        unsigned long          *NserialRead,
                                        SRDAT_USBXCH_EQUIPMENT *EquipmentValues,
                                        unsigned long           NequipmentRequested,
                                        unsigned long          *NequipmentRead,
                                        unsigned long          *NpacketsProcessed,
                                        unsigned long          *NpacketsProcessedData,
                                        unsigned long          *NpacketsProcessedStatus,
                                        int                    *Error

                                        ) {

        int              PacketType, Failure, RequestBreak, LastNmeaMsg;
        int              Nserial, NserialBytes, NtotalSerialBytes, Nequip;
        unsigned long    Nanalog, Nsamples, NserRead, NeqRead;
        unsigned long    Nprocessed, NprocessedData, NprocessedStatus;
        SRUSBXCH_PACKET *CurrentPacket;


//WCT - Consider upgrading this function so it can handle null data arrays 
//      if the caller is not interested in one or more types of data.  Also
//      consider allowing packet array to be a circular buffer.  Check
//      that diag analog test runs without serial error messages after these
//      changes.


        // Ensure we are always requesting an even number of samples

        Nanalog = (NanalogRequested/2) * 2L;
        if ( NanalogRequested != Nanalog ) {
                NanalogRequested = Nanalog * 2L;
                printf("SrDatUsbPacketProcess changed NanalogRequested to an even number\n");
                }

        
   
        // Process packets

        Failure           = USBXCH_ERROR_NONE;
        Nprocessed        = 0L;
        NprocessedData    = 0L;
        NprocessedStatus  = 0L;
        Nanalog           = 0L;
        Nserial           = 0;
        Nequip            = 0;
        NserialBytes      = 0;
        NtotalSerialBytes = 0;
        Nsamples          = 0;
        RequestBreak      = 0;
        NserRead          = 0L;
        NeqRead           = 0L;

        if (LastMsg)
                LastNmeaMsg = *LastMsg;
        else
                LastNmeaMsg = -1;

        
        while ( Nprocessed < NpacketsRead ) {


                CurrentPacket = &PacketArray[Nprocessed];
                PacketType    = SrUsbXchPacketType( CurrentPacket );

                
                if ( PacketType == SRDAT_USBPACKET_TYPE_SAMPLE ) {

                        if ( Nanalog >= NanalogRequested ) {
                                RequestBreak = 1;
                                break;
                                }
                        Nsamples = SrDatUsbAnalogFill( CurrentPacket,
                                                       CheckSanity,
                                                      &AnalogValues[Nanalog],
                                                      &Failure );

                        if ( Nsamples == 0 ) {      // Error occurred in packet demux
                                sprintf(ErrMsg,"WARNING: Error occurred in packet demux\n         at packet %lu with failure %d\n",
                                          Nprocessed, Failure );
                                SrDatError( ErrMsg );
                                Nanalog      = 0;
                                RequestBreak = 1;
                                break;
                                }
                        Nanalog += Nsamples;
                        NprocessedData++;
                        }
                
                else if ( PacketType == SRDAT_USBPACKET_TYPE_STATUS ) {
                        NserialBytes = SrDatUsbSerialFillBuffer( CurrentPacket, CheckSanity, &Failure );
                        NtotalSerialBytes += NserialBytes;
                        NprocessedStatus++;
//                        if ( NserialBytes == 0 )  // WCT should this be included in failure case
//                                printf( "NserialBytes is ZERO\n" );
                        if ( Failure != USBXCH_ERROR_NONE ) {
                                sprintf(SrUsbXchLastErrorString,"WARNING: Error occurred in fill serial buffer (nsbytes %d, total %d, err %d)\n",
                                          NserialBytes, NtotalSerialBytes, Failure );
                                Nsamples     = 0;
                                RequestBreak = 1;
                                break;
                                }

                        if ( SerBufNumFree() < SRDAT_USBPACKET_STATUS_BYTES ) {
                                sprintf( SrUsbXchLastErrorString,
                                         "WARNING: Serial buffer almost full.  Stop processing now.\n" );
                                RequestBreak = 1;
                                break;
                                }
                        }
                else {
                      sprintf( SrUsbXchLastErrorString, "Packet %lu is Unknown!!!", Nprocessed );
                      SrDatError( SrUsbXchLastErrorString );
                        }
                
                Nprocessed++;

                if ( RequestBreak == 1 )      // Ensure break out of while loop
                        break;


                } // end while Nprocessed < NpacketsRead



//WCT - check NserialRequested and NequipmentRequested not exceeded
        
        // Process all serial characters into serial data structure

        if ( NtotalSerialBytes > 0 ) {

                Nserial = SrDatUsbSerialParseBuffer( &LastNmeaMsg,
                                                     &SerialValues[0],
                                                      NserialRequested,
                                                      &NserRead,
                                                     &EquipmentValues[0],
                                                      NequipmentRequested,
                                                      &NeqRead,
                                                     &Failure );

                Nequip += NeqRead;

//WCT - consider adding                
//                if ( Nserial == 0 || Failure != USBXCH_ERROR_NONE ) {
//                        sprintf( SrUsbXchLastErrorString, "WARNING: Error occurred in parse serial (nsbytes %lu, nserial %lu, err %d)\n",
//                                  NtotalSerialBytes, Nserial, Failure );
//                        }
                }
        else {
                Nserial = 0;
                Nequip = 0;
                }


        if (LastMsg)                    *LastMsg                 = LastNmeaMsg;
        if ( Error )                    *Error                   = Failure;
        if ( NpacketsProcessed )        *NpacketsProcessed       = Nprocessed;
        if ( NpacketsProcessedData )    *NpacketsProcessedData   = NprocessedData;
        if ( NpacketsProcessedStatus )  *NpacketsProcessedStatus = NprocessedStatus;
        if ( NanalogRead )              *NanalogRead             = Nanalog;
        if ( NserialRead )              *NserialRead             = Nserial;
        if ( NequipmentRead )           *NequipmentRead          = Nequip;


        return( Nanalog ); // total # of analog samples read or 0 for error
}



//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbPacketCleanUp
// PURPOSE: This function removes any packet data read from the USBXCH that 
//          has already been processed and output.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned long ) SrDatUsbPacketCleanUp(
                                        SRUSBXCH_PACKET *PacketArray,
                                        unsigned long    NpacketsRead,
                                        unsigned long    NpacketsProcessed,
                                        int             *Error        

                                        ) {

        unsigned long i, Nremain;


        // Error check

        if ( !PacketArray ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PACKET;
                return( 0 );
                }


        if ( NpacketsProcessed > NpacketsRead ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }

        

        // Remove all the processed packets and move the unprocessed
        // ones up to the front of the array for processing later

        Nremain = NpacketsRead - NpacketsProcessed;

        if ( Nremain > 0L ) {

                for ( i = 0 ; i < Nremain ; i++ ) {
                        PacketArray[i] = PacketArray[NpacketsProcessed+i];
                        }
                }


        if ( Error )  *Error = USBXCH_ERROR_NONE;

        return( Nremain ); // total # of packets remaining
}



//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPacketShow
// PURPOSE: Output the specified number of packets to the specified file.  It is
//          ok to use stdout as the output file.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned long ) SrUsbXchPacketShow(
                                        FILE *OutputFile,
                                        SRUSBXCH_PACKET *PacketArray,
                                        unsigned long    StartPacket,
                                        unsigned long    NpacketsToShow,
                                        int             *Error
                                        ) {

        int              PacketType;
        unsigned long    i, OnBoardCount1, OnBoardCount2;
        SRUSBXCH_PACKET *CurrentPacket;
        FILE            *fout;

        static int LastPpsToggle = -1; // an invalid value

        fout = OutputFile;
//WCT - error check fout or give default
//      if (!fout)
//            fout = stdout;

        
        for ( i = 0 ; i < NpacketsToShow ; i++ ) {

                CurrentPacket = &PacketArray[i];
                PacketType    = SrUsbXchPacketType( CurrentPacket );

                if ( PacketType == SRDAT_USBPACKET_TYPE_SAMPLE ) {

                        fprintf( fout, "Packet %lu has DATA (Analog/Digital/Sample)\n", StartPacket+i );
                        
                        OnBoardCount1 = SrDatOnBoardCountDemux( CurrentPacket->Sample[0].OnBoardCountBytes );
                        fprintf( fout, "  Type1 = 0x%02X, Sanity1 = 0x%02X, Toggle1 = 0x%02X, Digital1 = 0x%02X, GGA1 = 0x%02X, OnBoardCount = 0x%08lX (%8lu)",
                                  PacketType,
                                  SrUsbXchPacketSanity( CurrentPacket ),
                                  (CurrentPacket->Sample[0].PpsToggle & 0x000F),
                                  (CurrentPacket->Sample[0].DigitalIn & 0x000F),
                                  (CurrentPacket->Sample[0].GGACount  & 0x000F),
                                  OnBoardCount1, OnBoardCount1
                               );
                        
                        if ( LastPpsToggle != CurrentPacket->Sample[0].PpsToggle )
                                fprintf( fout, " **" );
                        LastPpsToggle = CurrentPacket->Sample[0].PpsToggle;

                        if ( OnBoardCount1 != 0)
                                fprintf( fout, " <<\n" );
                        else
                                fprintf( fout, "\n" );


                        OnBoardCount2 = SrDatOnBoardCountDemux( CurrentPacket->Sample[1].OnBoardCountBytes );
                        fprintf( fout, "  Type1 = 0x%02X, Sanity2 = 0x%02X, Toggle2 = 0x%02X, Digital2 = 0x%02X, GGA2 = 0x%02X, OnBoardCount = 0x%08lX (%8lu)",
                                  PacketType,
                                  SrUsbXchPacketSanity2( CurrentPacket ),
                                  (CurrentPacket->Sample[1].PpsToggle & 0x000F),
                                  (CurrentPacket->Sample[1].DigitalIn & 0x000F),
                                  (CurrentPacket->Sample[1].GGACount  & 0x000F),
                                  OnBoardCount2, OnBoardCount2
                               );
                        
                        if ( LastPpsToggle != CurrentPacket->Sample[1].PpsToggle )
                                fprintf( fout, " **" );
                        LastPpsToggle = CurrentPacket->Sample[1].PpsToggle;

                        if ( OnBoardCount2 != 0)
                                fprintf( fout, " <<\n" );
                        else
                                fprintf( fout, "\n" );
                        
                        }

                else if ( PacketType == SRDAT_USBPACKET_TYPE_STATUS ) {
                        fprintf( fout,"Packet %lu has STATUS (Serial/Equipment/GPS)\n", StartPacket+i );
                        fprintf( fout,"  Type = 0x%X, Sanity = 0x%X\n",
                                  PacketType,
                                  SrUsbXchPacketSanity( CurrentPacket )
                               );
                        }

                else {
                        fprintf( fout, "Packet %lu is an unknown packet!!!\n", StartPacket+i );
                        }



                SrUsbXchPacketOutputBytes( fout, CurrentPacket );
                

                } // end for i < NpacketsToShow


        if ( Error )  *Error = USBXCH_ERROR_NONE;
        return( 1L );
}




//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbAnalogFill
// PURPOSE: To transfer samples of USBxCH data stored in a generic USB packet
//          into the SRDAT_USBXCH_ANALOG structure.  There are two samples per
//          packet.
//------------------------------------------------------------------------------
unsigned long SrDatUsbAnalogFill( SRUSBXCH_PACKET     *Packet,
                                  int                 CheckSanity,
                                  SRDAT_USBXCH_ANALOG *AnalogValues,
                                  int                 *Failure ) {

        unsigned long Nsamp, NsamplesRead;
        int           Error;
        

        // Error check

        if ( !Packet || !AnalogValues ) {
                SrDatError( "SRDAT SrDatUsbAnalogFill invalid parameter.\n" );
                if ( Failure )  *Failure = USBXCH_ERROR_INVALID_PARAM;
                return( 0L );
                }


        // Demux first half of analog packet

        Nsamp = SrDatUsbAnalogDemux( &Packet->Sample[0],
                                     CheckSanity,
                                     &AnalogValues[0],
                                     &Error );

        if ( Nsamp == 0 ) {
                SrDatError("WARNING: Error occurred in packet demux 1\n" );
                if ( Failure )  *Failure = Error;
                return( Nsamp );
                }

        NsamplesRead = Nsamp;



        // Demux second half of analog packet

        Nsamp = SrDatUsbAnalogDemux( &Packet->Sample[1],
                                     CheckSanity,
                                     &AnalogValues[NsamplesRead],
                                     &Error );

        if ( Nsamp == 0 ) {
                SrDatError("WARNING: Error occurred in packet demux 2\n" );
                if ( Failure )  *Failure = Error;
                return( NsamplesRead );
                }

        NsamplesRead += Nsamp;


        
        if ( Failure )  *Failure = USBXCH_ERROR_NONE;

        return( NsamplesRead );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbAnalogDemux
// PURPOSE: This function fills the provided AnalogValues structure by
//          demultiplexing the provided array.  It returns 1 for success,
//          indicating 1 sample processed.  It returns 0 for failure.
//------------------------------------------------------------------------------
//FIX Rename this to SrUsbXchUsbSampleDemux
FUNCTYPE( unsigned long ) SrDatUsbAnalogDemux(
                                              SRUSBXCH_SAMPLE     *PacketSample,
                                              int                  CheckSanity,
                                              SRDAT_USBXCH_ANALOG *AnalogValues,
                                              int                 *Failure
                                             ) {

        unsigned char    PacketType, PacketSanity;
        SRUSBXCH_PACKET *Packet;


        // Error check

        if (!PacketSample || !AnalogValues) {
                if ( Failure )  *Failure = USBXCH_ERROR_INVALID_PARAM;
                SrDatError( "SRDAT SrDatUsbAnalogDemux invalid parameter.\n" );
                return( 0L );
                }


        // Get packet type and sanity
        
        // NOTE: There is a little trickiness going on here.  Everything
        // is normal when the passed Array truly starts at packet byte
        // 0, but when processing the second sample in the packet, Array
        // actually starts at packet byte SRDAT_USBPACKET_SAMPLE_SIZE.
        // So, we are relying on the fact that the names for the second
        // sample like PpsToggle2 are in the exact same relation as the
        // names for the first sample since we use only the first sample
        // names in this function.

        Packet       = (SRUSBXCH_PACKET *)PacketSample;
        PacketType   = SrUsbXchPacketType( Packet );
        PacketSanity = SrUsbXchPacketSanity( Packet );
        

        // Verify packet type

        if ( PacketType != SRDAT_USBPACKET_TYPE_SAMPLE ) {
                if ( Failure )  *Failure = USBXCH_ERROR_INVALID_PACKET;
                SrDatError( "SRDAT SrDatUsbAnalogDemux trying to analog process a NON-Analog packet\n" );
                return( 0L );
                }
        


        // Check for the sanity byte.

        if ( CheckSanity == 1 ) {
                if ( PacketSanity == ExpectedAnalogSanity ) {
        //              printf("SrDatUsbAnalogDemux - Found expected Fifo Sanity 0x%X\n",PacketSanity );
                        ExpectedAnalogSanity++;
                        ExpectedAnalogSanity &= SRDAT_USBPACKET_SANITY_MASK;
                        }

                else {
                        if ( Failure )  *Failure = USBXCH_ERROR_SANITY_DATA;
                        sprintf( ErrMsg, "SRDAT SrDatUsbAnalogDemux - Should Exit. Found Data Sanity 0x%X, expected 0x%X\n",
                                 PacketSanity, ExpectedAnalogSanity );
                        SrDatError( ErrMsg );
                        DumpBytes( (char*)PacketSample, SRDAT_USBPACKET_SAMPLE_SIZE );
                        ExpectedAnalogSanity++;
                        ExpectedAnalogSanity &= SRDAT_USBPACKET_SANITY_MASK;
                        return( 0L );
                        }
                }


        // Save sanity and other extra data

        AnalogValues->Sanity       = PacketSanity;
        AnalogValues->PpsToggle    = PacketSample->PpsToggle;
        AnalogValues->DigitalIn    = PacketSample->DigitalIn;
        AnalogValues->GGACount     = PacketSample->GGACount;
        AnalogValues->OnBoardCount = SrDatOnBoardCountDemux( PacketSample->OnBoardCountBytes );



//WCT - Review case where OBC for point 1 should truly be non-zero
//      or switch to PpsToggle method        

        // Very first point after starting A/D will have a value in OBC when it
        // really should be 0
        
        AnalogValues->PpsEventNum = 0;
        
        if ( AnalogValues->OnBoardCount != 0L ) {
                if ( LastAnalogPpsEventNum == -1 ) {
                        AnalogValues->OnBoardCount = 0L;
                        LastAnalogPpsEventNum      = 0;
                        }
                else {
                        AnalogValues->PpsEventNum  = LastAnalogPpsEventNum +
                                                     AnalogValues->GGACount;
                        if ( AnalogValues->GGACount == GGACOUNTROLL )
                                LastAnalogPpsEventNum += (int)(GGACOUNTROLL + 1);
                        }
                }


        // Demux 24 A/D bits, start by pointing to the beginning of
        // analog data in the current 32 byte half of USB 64 byte packet.

        SrUsbXchHelperAnalogDemux( PacketSample->MuxedDataBytes,
                                   (unsigned long*)AnalogValues->AnalogData );


        if (Failure)  *Failure = USBXCH_ERROR_NONE;

        return( 1L );  // We filled 1 sample
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchAnalogDemux
// PURPOSE: This function demuxes and sign extends the 24 analog bytes returned
//          by the USBxCH for 8 channels for each sample.  Data for 8 channels
//          comes up the USB cable in each packet, so all 8 channels are
//          processed even for the USB4CH.
//------------------------------------------------------------------------------
#define S       0xFF000000

struct SignExtensions {

        unsigned long B3;
        unsigned long B2;
        unsigned long B1;
        unsigned long B0;

        } LookupSign[16] = {

                { 0,0,0,0 },
                { 0,0,0,S },
                { 0,0,S,0 },
                { 0,0,S,S },
                { 0,S,0,0 },
                { 0,S,0,S },
                { 0,S,S,0 },
                { 0,S,S,S },
                { S,0,0,0 },
                { S,0,0,S },
                { S,0,S,0 },
                { S,0,S,S },
                { S,S,0,0 },
                { S,S,0,S },
                { S,S,S,0 },
                { S,S,S,S },

                };

#undef S

#define UlongBytes( B3, B2, B1, B0 )   ( (B3 << 24) | (B2 << 16) | (B1 << 8) | (B0 << 0) )

unsigned long LookUpDemux[16] = {

        UlongBytes( 0,0,0,0 ),
        UlongBytes( 0,0,0,1 ),
        UlongBytes( 0,0,1,0 ),
        UlongBytes( 0,0,1,1 ),
        UlongBytes( 0,1,0,0 ),
        UlongBytes( 0,1,0,1 ),
        UlongBytes( 0,1,1,0 ),
        UlongBytes( 0,1,1,1 ),
        UlongBytes( 1,0,0,0 ),
        UlongBytes( 1,0,0,1 ),
        UlongBytes( 1,0,1,0 ),
        UlongBytes( 1,0,1,1 ),
        UlongBytes( 1,1,0,0 ),
        UlongBytes( 1,1,0,1 ),
        UlongBytes( 1,1,1,0 ),
        UlongBytes( 1,1,1,1 )

        };

#undef UlongBytes

void SrUsbXchHelperAnalogDemux( unsigned char *AdData, unsigned long *DemuxedValue ) {

        int i;

        unsigned long Reg2, Reg1, Reg0;

        // NOTE: Inputs are assumed to exist.  They are NOT checked.


        // Demux the sign extensions in AdData[0] ...

        i = AdData[0] & 0xF;

                DemuxedValue[0] = LookupSign[ i ].B0;
                DemuxedValue[1] = LookupSign[ i ].B1;
                DemuxedValue[2] = LookupSign[ i ].B2;
                DemuxedValue[3] = LookupSign[ i ].B3;

        i = AdData[0] >> 4;

                DemuxedValue[4] = LookupSign[ i ].B0;
                DemuxedValue[5] = LookupSign[ i ].B1;
                DemuxedValue[6] = LookupSign[ i ].B2;
                DemuxedValue[7] = LookupSign[ i ].B3;


        // Demux the A/D values:

        // work on low four channels ...

                    Reg2  = LookUpDemux[ AdData[ 0] & 0xF ];  // bit 23
        Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 1] & 0xF ];  // bit 22
        Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 2] & 0xF ];  // bit 21
        Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 3] & 0xF ];  // bit 20
        Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 4] & 0xF ];  // bit 19
        Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 5] & 0xF ];  // bit 18
        Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 6] & 0xF ];  // bit 17
        Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 7] & 0xF ];  // bit 16
 
                    Reg1  = LookUpDemux[ AdData[ 8] & 0xF ];  // bit 15
        Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[ 9] & 0xF ];  // bit 14
        Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[10] & 0xF ];  // bit 13
        Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[11] & 0xF ];  // bit 12
        Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[12] & 0xF ];  // bit 11
        Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[13] & 0xF ];  // bit 10
        Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[14] & 0xF ];  // bit 09
        Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[15] & 0xF ];  // bit 08

                    Reg0  = LookUpDemux[ AdData[16] & 0xF ];  // bit 07
        Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[17] & 0xF ];  // bit 06
        Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[18] & 0xF ];  // bit 05
        Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[19] & 0xF ];  // bit 04
        Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[20] & 0xF ];  // bit 03
        Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[21] & 0xF ];  // bit 02
        Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[22] & 0xF ];  // bit 01
        Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[23] & 0xF ];  // bit 00
 
                ( (char *) &(DemuxedValue[0]) )[2] = ( (char *)(&Reg2) )[0];
                ( (char *) &(DemuxedValue[0]) )[1] = ( (char *)(&Reg1) )[0];
                ( (char *) &(DemuxedValue[0]) )[0] = ( (char *)(&Reg0) )[0];

                ( (char *) &(DemuxedValue[1]) )[2] = ( (char *)(&Reg2) )[1];
                ( (char *) &(DemuxedValue[1]) )[1] = ( (char *)(&Reg1) )[1];
                ( (char *) &(DemuxedValue[1]) )[0] = ( (char *)(&Reg0) )[1];

                ( (char *) &(DemuxedValue[2]) )[2] = ( (char *)(&Reg2) )[2];
                ( (char *) &(DemuxedValue[2]) )[1] = ( (char *)(&Reg1) )[2];
                ( (char *) &(DemuxedValue[2]) )[0] = ( (char *)(&Reg0) )[2];

                ( (char *) &(DemuxedValue[3]) )[2] = ( (char *)(&Reg2) )[3];
                ( (char *) &(DemuxedValue[3]) )[1] = ( (char *)(&Reg1) )[3];
                ( (char *) &(DemuxedValue[3]) )[0] = ( (char *)(&Reg0) )[3];
 


        // work on high four channels >>>

                     Reg2  = LookUpDemux[ AdData[ 0] >> 4 ];  // bit 23
         Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 1] >> 4 ];  // bit 22
         Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 2] >> 4 ];  // bit 21
         Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 3] >> 4 ];  // bit 20
         Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 4] >> 4 ];  // bit 19
         Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 5] >> 4 ];  // bit 18
         Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 6] >> 4 ];  // bit 17
         Reg2 <<= 1; Reg2 |= LookUpDemux[ AdData[ 7] >> 4 ];  // bit 16
 
                     Reg1  = LookUpDemux[ AdData[ 8] >> 4 ];  // bit 15
         Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[ 9] >> 4 ];  // bit 14
         Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[10] >> 4 ];  // bit 13
         Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[11] >> 4 ];  // bit 12
         Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[12] >> 4 ];  // bit 11
         Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[13] >> 4 ];  // bit 10
         Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[14] >> 4 ];  // bit 09
         Reg1 <<= 1; Reg1 |= LookUpDemux[ AdData[15] >> 4 ];  // bit 08
 
                     Reg0  = LookUpDemux[ AdData[16] >> 4 ];  // bit 07
         Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[17] >> 4 ];  // bit 06
         Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[18] >> 4 ];  // bit 05
         Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[19] >> 4 ];  // bit 04
         Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[20] >> 4 ];  // bit 03
         Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[21] >> 4 ];  // bit 02
         Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[22] >> 4 ];  // bit 01
         Reg0 <<= 1; Reg0 |= LookUpDemux[ AdData[23] >> 4 ];  // bit 00
 
                 ( (char *) &(DemuxedValue[4]) )[2] = ( (char *)(&Reg2) )[0];
                 ( (char *) &(DemuxedValue[4]) )[1] = ( (char *)(&Reg1) )[0];
                 ( (char *) &(DemuxedValue[4]) )[0] = ( (char *)(&Reg0) )[0];

                 ( (char *) &(DemuxedValue[5]) )[2] = ( (char *)(&Reg2) )[1];
                 ( (char *) &(DemuxedValue[5]) )[1] = ( (char *)(&Reg1) )[1];
                 ( (char *) &(DemuxedValue[5]) )[0] = ( (char *)(&Reg0) )[1];

                 ( (char *) &(DemuxedValue[6]) )[2] = ( (char *)(&Reg2) )[2];
                 ( (char *) &(DemuxedValue[6]) )[1] = ( (char *)(&Reg1) )[2];
                 ( (char *) &(DemuxedValue[6]) )[0] = ( (char *)(&Reg0) )[2];

                 ( (char *) &(DemuxedValue[7]) )[2] = ( (char *)(&Reg2) )[3];
                 ( (char *) &(DemuxedValue[7]) )[1] = ( (char *)(&Reg1) )[3];
                 ( (char *) &(DemuxedValue[7]) )[0] = ( (char *)(&Reg0) )[3];


}

//------------------------------------------------------------------------------
// ROUTINE: SrDatOnBoardCountDemux
// PURPOSE: This function demuxes the OnBoardCount returned from the USBxCH.
//          The data was sent out MSB first as 4 bytes with info only in the 
//          lower nibble.  This data is shifted so all 16 valid bits are in 
//          the last two bytes of an unsigned long.  Returns 0 if any 
//          argument is missing, the corrected value otherwise.
//------------------------------------------------------------------------------
unsigned long SrDatOnBoardCountDemux( unsigned char *OnBoardCountBytes ) {
        
        unsigned long OnBoardCountCorrected;

        if ( !OnBoardCountBytes )
                return( 0L );

        OnBoardCountCorrected = (  ( OnBoardCountBytes[0] << 12 ) // bits 15-12
                                 | ( OnBoardCountBytes[1] << 8 )  // bits 11-8
                                 | ( OnBoardCountBytes[2] << 4 )  // bits  7-4
                                 | ( OnBoardCountBytes[3] << 0 )  // bits  3-0
                                );

        return( OnBoardCountCorrected );
}


#ifndef _WIN64     // 'SrDatByteDemux()' with 'asm' won't compile as 64-bt

//------------------------------------------------------------------------------
// ROUTINE: SrDatByteDemux
// PURPOSE: This function demuxes data returned from the USBxCH.
//------------------------------------------------------------------------------
int SrDatByteDemux( unsigned char **Array, unsigned char *DmxValues ) {

        int             bit;
        unsigned char   MuxedBits, chan0, chan1, chan2, chan3;

//        printf("SrDatByteDemux - Enter\n" );

        // Initialize values

        chan0 = 0x00;
        chan1 = 0x00;
        chan2 = 0x00;
        chan3 = 0x00;


        // *Array points to 8 bytes of muxed USB data.
        // One bit of each muxed byte is used to generate a demuxed byte.

        // Demux A/D register bits.

        for ( bit = 7 ; bit >= 0 ; bit-- ) { // 7...0 = 8 bits

                MuxedBits = *(*Array)++;  // was *pArray++ but we want to send
                                          // changed value back to calling func

#if defined( SROS_WIN2K ) || defined( SROS_WINXP )

                        _asm {
                                mov al,BYTE PTR MuxedBits

                                rcr al,1
                                rcl chan0,1

                                rcr al,1
                                rcl chan1,1

                                rcr al,1
                                rcl chan2,1

                                rcr al,1
                                rcl chan3,1

                                }
#elif defined( SROS_LINUX )

                        asm (
                               "movb %4,%%al;        \
                                                     \
                                rcrb $1,%%al;        \
                                rclb $1,%0;          \
                                                     \
                                rcrb $1,%%al;        \
                                rclb $1,%1;          \
                                                     \
                                rcrb $1,%%al;        \
                                rclb $1,%2;          \
                                                     \
                                rcrb $1,%%al;        \
                                rclb $1,%3"          \
                                                     \
                               : "=m" (chan0),       \
                                 "=m" (chan1),       \
                                 "=m" (chan2),       \
                                 "=m" (chan3)        \
                               :  "m" (MuxedBits)    \
                               : "%ax"

                            );

// Closest C code equivalent
//
//                            chan0 <<= 1;
//                            chan0 |= (MuxedBits & 0x01);
//
//                            MuxedBits >>= 1;
//                            chan1 <<= 1;
//                            chan1 |= (MuxedBits & 0x01);
//
//                            MuxedBits >>= 1;
//                            chan2 <<= 1;
//                            chan2 |= (MuxedBits & 0x01);
//
//                            MuxedBits >>= 1;
//                            chan3 <<= 1;
//                            chan3 |= (MuxedBits & 0x01);
                        
#endif // SROS_xxxxx
                        }

      DmxValues[0] = chan0;
      DmxValues[1] = chan1;
      DmxValues[2] = chan2;
      DmxValues[3] = chan3;


//      printf("SrDatByteDemux - Exit\n" );
      return( 4 );

}

#endif


//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbSerialFillBuffer
// PURPOSE: To transfer serial NMEA data stored in a generic USB packet into
//          into the circular SerialBuffer.  From this buffer, the NMEA
//          characters are parsed into NMEA message strings and stored in a
//          SERIALDATA structure.  The number of NMEA characters per packet
//          can vary.
//------------------------------------------------------------------------------
int SrDatUsbSerialFillBuffer( SRUSBXCH_PACKET *Packet, int CheckSanity, int *Failure ) {

        int           i, TotalChar, Full;
        unsigned char PacketType, PacketSanity, CurrentByte;
        
        // Error check

        if (!Packet) {
                if ( Failure )  *Failure = USBXCH_ERROR_INVALID_PARAM;
                SrDatError( "SRDAT SrDatUsbSerialFillBuffer invalid parameter\n" );
                return( 0L );
                }


        // Initialize values

        if ( Failure )  *Failure = USBXCH_ERROR_NONE;
        Full      = 0;
        TotalChar = 0;

        PacketType   = SrUsbXchPacketType( Packet );
        PacketSanity = SrUsbXchPacketSanity( Packet );


        // Check packet data type

        if ( PacketType != SRDAT_USBPACKET_TYPE_STATUS ) {
                if ( Failure )  *Failure = USBXCH_ERROR_INVALID_PACKET;
                SrDatError( "SRDAT SrDatUsbSerialFillBuffer trying to serial process a NON-GPS packet\n");
                return( 0L );
                }



        // Check SerialSanity

        if ( CheckSanity == 1 ) {
                if ( PacketSanity == ExpectedSerialSanity ) {
        //              printf("SrDatUsbSerialFillBuffer - Found expected Serial Sanity 0x%X\n", Packet->DataBytes[0] );
                        ExpectedSerialSanity++;
                        ExpectedSerialSanity &= SRDAT_USBPACKET_SANITY_MASK;
                        }

                else {
                        if ( Failure )  *Failure = USBXCH_ERROR_SANITY_STATUS;
                        sprintf( ErrMsg, "SRDAT SrDatUsbSerialFillBuffer - Should Exit. Found Status Sanity 0x%X, expected 0x%X\n",
                                 PacketSanity, ExpectedSerialSanity );
                        SrDatError( ErrMsg );
                        ExpectedSerialSanity++;
                        ExpectedSerialSanity &= SRDAT_USBPACKET_SANITY_MASK;
                        return( 0L );
                        }
                }

        // Save original starting value for filling
        
        for ( i = 0 ; i < SRDAT_USBPACKET_STATUS_BYTES ; i++ ) {
                CurrentByte = Packet->Status.NmeaBytes[i];
                TotalChar++;
                Full = SerBufPutOne( CurrentByte );
                if (Full) {
                        printf("************ SrDatUsbSerialFillBuffer buffer full.\n");
                        if ( Failure )  *Failure = USBXCH_ERROR_SERIALBUFFER_FULL;
                        break;
                        }
                } // end for i < SRDAT_USBPACKET_STATUS_BYTES


      return( TotalChar );
}


//------------------------------------------------------------------------------
// ROUTINE: SrDatSerialPrepareStructure
// PURPOSE: 
//------------------------------------------------------------------------------
int SrDatSerialPrepareStructure( char *NmeaGroup, int NmeaCount, int LastSecondCount,
                                 int GotEqData, SRDAT_USBXCH_EQUIPMENT *EquipData,
                                 SRDAT_USBXCH_SERIAL *SerialValues,
                                 SRDAT_USBXCH_EQUIPMENT *EquipValues
                               ) {

        int i, PpsCount, PpsEventNum;


        // Compute and update PpsEventNum 

        PpsCount = LastSecondCount & SERIALCOUNTROLL;
        PpsEventNum = LastSerialPpsEventNum + PpsCount;
        if ( PpsCount == SERIALCOUNTROLL )
                LastSerialPpsEventNum += (SERIALCOUNTROLL + 1);

        
        // Fill current structures
        
        SrDatUsbSerialInit( SerialValues );
        
        SerialValues->ValidFields |= SRDAT_VALID_NMEA;
        SerialValues->PpsEventNum  = PpsEventNum;
        SerialValues->NmeaCount    = NmeaCount;
        for ( i = 0 ; i < SRDAT_NMEA_MAX_BUFF ; i++ )
                SerialValues->NmeaMsg[i] = NmeaGroup[i];

        if ( GotEqData ) {
                SrDatUsbEquipInit( EquipValues );
                *EquipValues = *EquipData;
                }

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatUsbSerialParseBuffer
// PURPOSE: 
//------------------------------------------------------------------------------
int SrDatUsbSerialParseBuffer( int                    *LastMsg,
                               SRDAT_USBXCH_SERIAL    *SerialValues,
                               unsigned long           NserialRequested,
                               unsigned long          *NserialRead,
                               SRDAT_USBXCH_EQUIPMENT *EquipValues,
                               unsigned long           NequipRequested,
                               unsigned long          *NequipRead,
                               int                    *Failure ) {
        
        int   Nserial, LastNmeaMsg, nc, NumChar, NmeaError;
        int   SecondCount, inmea, GotEqData, CurrMsg;
        SRDAT_USBXCH_EQUIPMENT EquipData;

        static char  NmeaMsg[SRDAT_NMEA_MAX_SIZE];
        static char  NmeaGroup[SRDAT_NMEA_MAX_BUFF];
        static SRDAT_USBXCH_EQUIPMENT LastEquipData;
        static int                    LastGotEqData = 0;

        
        
        // Error check

        if (!SerialValues || !EquipValues) {
                if ( Failure )  *Failure = USBXCH_ERROR_INVALID_PARAM;
                SrDatError( "SRDAT SrDatUsbSerialParseBuffer invalid parameter\n" );
                return( 0L );
                }


        // Initialize values

        if ( Failure )      *Failure     = USBXCH_ERROR_NONE;
        if ( NserialRead )  *NserialRead = 0L;
        if ( NequipRead )   *NequipRead  = 0L;
        
        if ( LastMsg )
                LastNmeaMsg = *LastMsg;
        else
                LastNmeaMsg = -1;

        Nserial   = 0;
        GotEqData = 0;

        
        // Read NMEA Data
        
        while ( 1 ) {

                NmeaError = SrDatSerialReadNmea( SRDAT_NMEA_MAX_SIZE,
                                                 NmeaMsg,
                                                &CurrMsg,
                                                &nc,
                                                &SecondCount,
                                                &GotEqData,
                                                &EquipData );
                
                if ( NmeaError == SRDAT_NMEA_ERR_NONE ) {

                        if ( SecondCount != -1 ) {              // We found first string in a second group

                                if ( (SbLastSecondCount != SecondCount )  &&
                                     (SbLastSecondCount != -1)  ) {

                                        SrDatSerialPrepareStructure( NmeaGroup, SbNmeaCount, SbLastSecondCount,
                                                                     LastGotEqData, &LastEquipData,
                                                                     &SerialValues[Nserial],
                                                                     &EquipValues[Nserial]
                                                                    );
                                        Nserial++;
                                        if ( Nserial >= MAX_SERIALSTRUCT ) { 
                                                printf("Read max serial structs of %d for this pass\n",
                                                       MAX_SERIALSTRUCT );
                                                break;
                                                }
                                        }
                                
                                SbNmeaCount       = 1;
                                SbLastSecondCount = SecondCount;
                                LastGotEqData     = GotEqData;
                                LastEquipData     = EquipData;

                                }
                        else                                    // We found a trailing string
                                SbNmeaCount++;

                        // Save NmeaMsgString into the NmeaGroup for later processing

                        inmea = (SbNmeaCount-1) * SRDAT_NMEA_MAX_SIZE;
                        if ( SbNmeaCount <= SRDAT_NMEA_MAX_TYPE )
                                strcpy( &NmeaGroup[inmea], NmeaMsg );
                        else {
                                if ( Failure )  *Failure = USBXCH_ERROR_SERIALPARSE_NMEA_FULL;
                                sprintf( TempStr,
                                         "SRDAT SrDatUsbSerialParseBuffer too many NMEA messages per second, expecting 2, got at least %d", SRDAT_NMEA_MAX_TYPE );
                                SrDatError( TempStr );
                                return( 0L );
                                }
                        }

                else { // (NmeaError != SRDAT_NMEA_ERR_NONE)

                        break;

                        } // end else NmeaError != SRDAT_NMEA_ERR_NONE

                // Exit if no more characters
                
                NumChar = SerBufNumChar();
                if ( NumChar == 0 )
                        break;


                // WCT - Consider filling serial buffer up again here as it is getting low

                } // end while 1


        if ( NserialRead )  *NserialRead = (unsigned long)Nserial;
        if ( NequipRead )   *NequipRead  = (unsigned long)Nserial;
        if ( LastMsg )      *LastMsg     = LastNmeaMsg;

        return( Nserial );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatEquipmentProcessData
// PURPOSE: Process raw equipement bytes extracted from between the ^ in the
//          NMEA string into the SRDAT_USBXCH_EQUIPMENT structure for easy
//          future access.
//------------------------------------------------------------------------------
int SrDatEquipmentProcessData( char *EquipBytes,
                               SRDAT_USBXCH_EQUIPMENT *EquipData,
                               int *Error ) {

        char c;
        int  i, Temp, Nbytes, Nread, Ndiscard, Neq;


        // Error check

        if ( !EquipBytes || !EquipData || !Error )
                return( 0 );



        // Peek number of bytes

        if ( !SerBufPeekOne( &c ) ) {
                *Error = SRDAT_NMEA_ERR_NO_EQUIPBYTE;
                return( 0 );
                }


        // Determine number of bytes to read and to discard
        // If data and software are from same rev, we will
        // read all available bytes and discard none
        
        Nbytes = (int)c & 0x00FF;
        if ( Nbytes > SRDAT_USBXCH_MAX_EQUIPBYTES )
                Nread = SRDAT_USBXCH_MAX_EQUIPBYTES;
        else
                Nread = Nbytes;

        Ndiscard = Nbytes - Nread;


        // Get all equipment bytes characters (eg those between the carats)

        if ( !SerBufGetTokenByCount( EquipBytes, Nread, 0, NULL,
                                     &Neq, SRDAT_USBXCH_MAX_EQUIPBYTES+1 )
             || (Neq != Nread) ) {

                *Error = SRDAT_NMEA_ERR_BAD_EQUIPDATA;
                return( 0 );
                }


        // Skip any excess characters the software doesn't understand
        
        for ( i = 0 ; i < Ndiscard ; i++ )
                SerBufIncUsePtr( );

        
        // Process equipment byte characters into the Equipment data structure
        // by saving the data that is known and available

        if ( Nbytes > 0 )  EquipData->Nbytes      = ((int)EquipBytes[0] & 0x00FF);
        if ( Nbytes > 1 )  EquipData->PpsEventNum = ((int)EquipBytes[1] & 0x00FF);
        if ( Nbytes > 2 )  EquipData->VoltageGood = ((int)EquipBytes[2] & 0x00FF);
        
        SrDatUsbTemperatureCombine(                       EquipBytes[3],
                                                          EquipBytes[4], &Temp );
        if ( Nbytes > 4 )  EquipData->Temperature = Temp;
        
        if ( Nbytes > 5 )  EquipData->DramFlags   = ((int)EquipBytes[5] & 0x00FF);
        if ( Nbytes > 6 )  EquipData->Unused1     = ((int)EquipBytes[6] & 0x00FF);
        if ( Nbytes > 7 )  EquipData->Unused2     = ((int)EquipBytes[7] & 0x00FF);

        if ( Nbytes != SRDAT_USBXCH_MAX_EQUIPBYTES ) {
                sprintf( SrUsbXchLastErrorString,
                         "WARNING: Unexpected number of equipment data items.  Found %d, expecting %d.\n",
                         Nbytes, SRDAT_USBXCH_MAX_EQUIPBYTES );
                return( 0 );
                }


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatSerialReadNmea
// PURPOSE: 
//------------------------------------------------------------------------------
int SrDatSerialReadNmea( int   MaxSize,
                         char *NmeaMsg,
                         int  *CurrMsg,
                         int  *Nchar,
                         int  *SecondCount,
                         int  *GotEqData,
                         SRDAT_USBXCH_EQUIPMENT *EquipData
                     ) {

        int  i, j, idiscard, checksum, readsum, nc, nlen, goteq, Error;
        int  saveuse, savefill, savewrap;
        char c, Token[SRDAT_NMEA_MAX_TOKEN], EquipBytes[SRDAT_USBXCH_MAX_EQUIPBYTES+1];

        // Initialize returns

        if ( Nchar )        *Nchar       = 0;
        if ( SecondCount )  *SecondCount = -1;
        if ( GotEqData )    *GotEqData   = 0;
        if ( EquipData )    SrDatUsbEquipInit( EquipData );


        // Initialize variables

        i        = 0;
        goteq    = 0;
        checksum = 0;
        Error    = SRDAT_NMEA_ERR_NONE;

        
        // NmeaMsg is a required parameter

        if ( !NmeaMsg )
                return( SRDAT_NMEA_ERR_INVALID_PARM ); // Exit 0, invalid parameter
        else
                *NmeaMsg = '\0';                       // initialize


        // Set up a dummy while loop so we can easily break to the
        // end of the program if we encounter an error while parsing 

        while ( 1 ) {
        
        // Discard characters until matching a $

        if ( SerBufGetToken( Token, '$', 1, NULL, &idiscard, SRDAT_NMEA_MAX_TOKEN ) ) {

                saveuse  = SbUsePtr;                    // Save current location at start of string
                savefill = SbFillPtr;
                savewrap = SbWrap;

                SerBufGetNextChar( &c );                // GetToken already peeked c, so ignore return
                NmeaMsg[i++] = c;                       // Save matched $
                }
        else {
                saveuse  = SbUsePtr;                    // Save current location even in case of error
                savefill = SbFillPtr;
                savewrap = SbWrap;

                Error = SRDAT_NMEA_ERR_NO_DOLLAR;  // We'll get this at the end of each NMEA set
                break; // Exit 1, no dollar
                }

                

        // Get MSGID, adding to checksum, by taking 5 characters
        
        if ( SerBufGetTokenByCount( Token, 5, 1, &checksum, &nc, SRDAT_NMEA_MAX_TOKEN ) &&
             nc == 5 ) {

                // If caller is interested in MSGID, get it and check it, 
                // otherwise accept any MSGID and continue
                
                if ( CurrMsg ) {
                        *CurrMsg = SerBufGetMsgIndex( Token );
                        if ( *CurrMsg == -2 ) {                 // MSGID not valid
                                Error = SRDAT_NMEA_ERR_NO_MSGID;
                                break; // Exit 2a, invalid msgid
                                }
                        }

                if ( i+nc < MaxSize ) {                         // If there is space for msgid
                        for ( j = 0 ; j < nc ; j++ )    
                                NmeaMsg[i++] = Token[j];        // Save MSGID
                        }
                else {
                        Error = SRDAT_NMEA_ERR_NO_SPACE;
                        break;  // Exit 5, no space for msgid
                        }
                }
        else {
                Error = SRDAT_NMEA_ERR_NO_MSGID;
                break; // Exit 2, no msgid
                }



        // Check for ^ bounded SR Equipment data
        // These char are not added to NmeaMsg, so we don't have to check MaxSize

        if ( SerBufPeekOne( &c ) && (c == '^') ) {
                SerBufIncUsePtr( );

                // Process following characters into the Equipment data structure

                if ( SrDatEquipmentProcessData( EquipBytes, &EquipmentData, &Error ) ) {
                        if ( SecondCount )   *SecondCount = EquipmentData.PpsEventNum;
                        if ( EquipData ) {
                                goteq      = 1;
                                *EquipData = EquipmentData;
                                }
                        }
                else           // Error set in call above
                        break; // Exit 5a,could not process eq data


                if ( SerBufPeekOne( &c ) && (c == '^') )
                        SerBufIncUsePtr( );

                else {
                        Error = SRDAT_NMEA_ERR_NO_CARAT;
                        break; // Exit 5b, did not find ending carat
                        }

                } // end peek && c == ^ (for starting ^)

        


        // Get msg data, adding to checksum, by matching to *

        if (  SerBufGetToken( Token, '*', 1, &checksum, &nc, SRDAT_NMEA_MAX_TOKEN ) ) {

                SerBufGetNextChar( &c );                        // GetToken already peeked c, so ignore return

                if ( i+nc+1 < MaxSize ) {                       // If there is space for msgdata and *
                        for ( j = 0 ; j < nc ; j++ )
                                NmeaMsg[i++] = Token[j];        // Save msg data
                        NmeaMsg[i++] = c;                       // Save matched *
                        }
                else {
                        Error = SRDAT_NMEA_ERR_NO_SPACE;
                        break; // Exit 7, no space for msgdata and *
                        }
                }
        else {
                Error = SRDAT_NMEA_ERR_NO_DATA;
                break; // Exit 6,no data before *
                }




        // Get checksum by matching until CR


        if ( SerBufGetToken( Token, '\r', 1, NULL, &nc, SRDAT_NMEA_MAX_TOKEN ) &&
             nc == 2 ) {
        
                SerBufGetNextChar( &c );                       // GetToken already peeked c, so ignore return

                if ( i+nc+1 < MaxSize ) {                      // If there is space for check sum and CR
                        for ( j = 0 ; j < nc ; j++ )
                                NmeaMsg[i++] = Token[j];       // Save check sum
                        NmeaMsg[i++] = c;                      // Save matched CR
                        }
                else {
                        Error = SRDAT_NMEA_ERR_NO_SPACE;
                        break; // Exit 10, no space for check sum %s and CR
                        }
                }
        else {
                Error = SRDAT_NMEA_ERR_NO_CR;
                break; // Exit 8, no cr
                }



        // Test the read value of the checksum against the computed
        // value which is derived by taking the XOR of all characters
        // between but not including the first $ and the last *

        readsum = SerAtoix( Token, 2 );
        if ( checksum != readsum ) {
                Error = SRDAT_NMEA_ERR_NO_CHECKSUM;
                break;  // Exit 11, no checksum
                }


        // Match and get LF

        
        if ( SerBufGetToken( Token, '\n', 1, NULL, &nc, SRDAT_NMEA_MAX_TOKEN ) &&
             nc == 0 ) {

                SerBufGetNextChar( &c );                // GetToken already peeked c, so ignore return

                if ( i+2 < MaxSize ) {                  // If there is space for LF and NULL
                        NmeaMsg[i++] = c;               // Save matched LF
                        NmeaMsg[i] = '\0';              // Save terminating NULL
                        }
                else {
                        Error = SRDAT_NMEA_ERR_NO_SPACE;
                        break; // Exit no space for LF and NULL
                        }
                }
        else {
        
                Error = SRDAT_NMEA_ERR_NO_LF;
                break; // Exit bad line feed
                }
                


        break; // exit infinite loop 

                } // end while


        // If we broke out of the loop with an error,
        // terminate the message so it will be printable


        if ( Error != SRDAT_NMEA_ERR_NONE ) {
                nlen = strlen( Token );
                if (nlen > 0 && i+nlen+1 < MaxSize ) {  // If space for token + LF
                        for ( j = 0 ; j < nlen ; j++ )
                                NmeaMsg[i++] = Token[j];
                        }
                NmeaMsg[i++] = '\n';
                NmeaMsg[i] = '\0';

                goteq = 0;                              // Throw away any eq data since we
                                                        // will be redoing entire string
                
                SbUsePtr  = saveuse;                    // Restore location at start of string
                SbFillPtr = savefill;
                SbWrap    = savewrap;
                }

        
       if ( Nchar )      *Nchar = i;
       if ( GotEqData )  *GotEqData = goteq;


//       printf("SrDatSerialReadNmea (CHK 0x%X %d) %03d: %s", checksum, idiscard, i, NmeaMsg );


        return( Error );
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufInit
// PURPOSE: 
//------------------------------------------------------------------------------
void SerBufInit( void ) {
        
        SerBufClearBytes(); // Clear bytes
        SerBufFlush();      // Clear pointers

        SbLastSecondCount = -1; // Clear parsing variables
        SbNmeaCount       = 0;
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufClearBytes
// PURPOSE: 
//------------------------------------------------------------------------------
void SerBufClearBytes( void ) {
        
        int i;

        // Clear serial buffer bytes

        for ( i = 0 ; i < MAX_SERIALBUFFER ; i++ )
                SerialBuffer[i] = '\0';
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufFlush
// PURPOSE: 
//------------------------------------------------------------------------------
void SerBufFlush( void ) {

        // Clear serial buffer pointers
        
        SbUsePtr = SbFillPtr = SbWrap = 0;
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufEmpty
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufEmpty( void ) {

        // Determine if serial buffer is empty

        if (SbUsePtr == SbFillPtr && SbWrap <= 0) { // If so, reset ptrs to
                SbUsePtr = SbFillPtr = 0;           // start of buffer
                return( 1 );
                }
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufFull
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufFull( void ) {

        // Determine if serial buffer is full

        if (SbUsePtr == SbFillPtr && SbWrap > 0)
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufNumChar
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufNumChar( void ) {

        int NumChar;

        NumChar = (SbFillPtr + SbWrap) - SbUsePtr;

        return( NumChar );
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufNumFree
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufNumFree( void ) {

        int NumFree;

        NumFree = MAX_SERIALBUFFER - SerBufNumChar() - 1;

        return( NumFree );
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufIncUsePtr
// PURPOSE: 
//------------------------------------------------------------------------------
void SerBufIncUsePtr( void ) {

        // Increment serial use (read) pointer

        SbUsePtr++;


        // Wrapping if necessary

        if (SbUsePtr >= MAX_SERIALBUFFER) {
                SbUsePtr = 0;
                SbWrap -= MAX_SERIALBUFFER;
                }
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufIncFillPtr
// PURPOSE: 
//------------------------------------------------------------------------------
void SerBufIncFillPtr( void ) {

        // Increment serial fill (write) pointer

        SbFillPtr++;
        

        // Wrapping if necessary

        if (SbFillPtr >= MAX_SERIALBUFFER) {
                SbFillPtr = 0;
                SbWrap += MAX_SERIALBUFFER;
                }
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufGetToken
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufGetToken( char *Token,
                    char  Sep,
                    int   breakdollar,
                    int  *chksum,
                    int  *numchar,
                    int   maxtoken ) {

        int  nc, exist, cs;
        char c;

        // Move character from SerialData buffer to the Token buffer
        // up to but NOT including the designated Separator character.

        nc       = 0;
        Token[0] = '\0';

        if ( chksum )
                cs = *chksum;
        else
                cs = 0;
        
        
        exist = SerBufPeekOne( &c );

        while ( exist && c != Sep ) {

                if ( breakdollar && c == '$' ) {
                        exist = 0;
                        break;
                        }

                cs ^= c;

                Token[nc] = c;
                
                if (nc < maxtoken-1)
                        nc++;
                
                SerBufIncUsePtr( );
                exist = SerBufPeekOne( &c );
                }

        Token[nc] = '\0';

        if ( numchar ) *numchar = nc;
        if ( chksum )  *chksum  = cs;

        if ( exist )
                return( 1 );
        else
                return( 0 );

}

//------------------------------------------------------------------------------
// ROUTINE: SerBufGetTokenByCount
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufGetTokenByCount( char *Token,
                           int   count,
                           int   breakdollar,
                           int  *chksum,
                           int  *numchar,
                           int   maxtoken ) {

        int  nc, i, ret, cs;
        char c;


        // Move count characters from SerialData buffer to the Token buffer

        nc       = 0;
        Token[0] = '\0';
        ret      = 1;

        if ( chksum )
                cs = *chksum;
        else
                cs = 0;
        
        for ( i = 0 ; i < count ; i++ ) {
                
                if ( !SerBufPeekOne( &c ) ) {
                        ret = 0;
                        break;
                        }

                if ( breakdollar && c == '$' ) {
                        ret = 0;
                        break;
                        }

                cs ^= c;
                
                Token[nc] = c;

                SerBufIncUsePtr( );

                if ( nc < maxtoken-1 )
                        nc++;

                } // end for i < count
        
        Token[nc] = '\0';

        if ( numchar ) *numchar = nc;
        if ( chksum )  *chksum  = cs;

        return( ret );
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufGetNextChar
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufGetNextChar( char *c ) {

        if ( !c )
                return( 0 );
        
        // Peek at next character, if it exists then change
        // the peek to a get by incrementing the use pointer
        
        if ( SerBufPeekOne( c ) ) {
                SerBufIncUsePtr( );
                return( 1 );
                }
        else {
                *c = ' ';
                return( 0 );
                }
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufPeekOne
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufPeekOne( char *c ) {

        // Check for data

        if (SerBufEmpty())
                return( 0 );


        // Get next character

        *c = SerialBuffer[SbUsePtr];
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufPutOne
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufPutOne( char c ) {

        // Save character into next free position in buffer

        SerialBuffer[SbFillPtr] = c;
        SerBufIncFillPtr( );


        // Check for fullness

        return( SerBufFull() );
}
                
//------------------------------------------------------------------------------
// ROUTINE: SerBufGetMsgIndex
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufGetMsgIndex( char *Token ) {

        int i, j, match, c;

        // Compare each possible MsgName with Token
        
        i = 0;

        
        while ( NmeaMsgName[i] ) {

                match = 1;

                // Test each character in current MsgName
                
                for ( j = 0 ; j < SRDAT_NMEA_MAX_NAME ; j++ ) {
                        
                        c = NmeaMsgName[i][j];
                        
                        if ( c != Token[j] ) {
                                match = 0;
                                break; // exit for j
                                }
                        }


                // If a match was found, return its index

                if (match)
                        return( i );

                // Else move on to next MsgName

                else
                        i++;
                
                } // end while


        // No match was found for any MsgName

        return( -2 );
}

//------------------------------------------------------------------------------
// ROUTINE: SerAtoix
// PURPOSE: 
//------------------------------------------------------------------------------
int SerAtoix( char *src, int n ) {

        char *hexdigits, *str, c;
        int   i, result, digit;
        
        hexdigits = "0123456789ABCDEF";
        digit     = -1;
        result    = 0;
        str       = src;

        while ( n-- ) {
                
                c = *str++;
                
                for ( i = 0 ; i < 16 ; i++ ) {
                        if (hexdigits[i] == c) {
                                digit = i;
                                break;
                                }
                        }
                
                result = (result * 0x10) + digit;
                }
        
        return( result );
}

//------------------------------------------------------------------------------
// ROUTINE: SerBufShow
// PURPOSE: 
//------------------------------------------------------------------------------
int SerBufShow( void ) {

        int i, j, NumChar, c;

        NumChar = SerBufNumChar();

        printf( "Serial Buffer has %d characters:\n", NumChar );

        j = SbUsePtr;
        
        for ( i = 0 ; i < NumChar ; i++ ) {
                c = SerialBuffer[j];
                if ( c > 0x20 )
                        printf( "%c", c );
                else
                        printf( " 0x%02X ", (c&0xFF) );
                j++;

                if (j >= MAX_SERIALBUFFER)
                        j = 0;
                }

        printf("\n");
        
        return( 1 );
}







//************************* NMEA HELPER FUNCTIONS *************************

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

//                                        Valid           Date        Time      LatLon         Alt        Nsat
{SRDAT_NMEA_MSGID_RMC, "GPRMC",       1, 2,  'A',       1, 9, 1,       1, 1,       2, 3,  NORANK, 0,  NORANK, 0},
{SRDAT_NMEA_MSGID_ZDA, "GPZDA",  NORANK, 0, '\0',       2, 2, 3,       2, 1,  NORANK, 0,  NORANK, 0,  NORANK, 0},
{SRDAT_NMEA_MSGID_GGA, "GPGGA",       1, 6,  '1',  NORANK, 0, 0,       3, 1,       1, 2,       1, 9,       1, 7},
{SRDAT_NMEA_MSGID_GLL, "GPGLL",       1, 6,  'A',  NORANK, 0, 0,       4, 5,       3, 1,  NORANK, 0,  NORANK, 0},
{SRDAT_NMEA_MSGID_VTG, "GPVTG",  NORANK, 0, '\0',  NORANK, 0, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0},
{SRDAT_NMEA_MSGID_GSA, "GPGSA",  NORANK, 0, '\0',  NORANK, 0, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,       2, 3},
{SRDAT_NMEA_MSGID_GSV, "GPGSV",  NORANK, 0, '\0',  NORANK, 0, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0},
{SRDAT_NMEA_MSGID_MIN, NULL,     NORANK, 0, '\0',  NORANK, 0, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0,  NORANK, 0}
        };


//------------------------------------------------------------------------------
// ROUTINE: SrDatRequestNmeaInfo2
// PURPOSE: Process a collection of NMEA message strings and extract the
//          requested items.  These include things like the date, time,
//          position, and number of satellites.  This enhanced version also
//          returns info indicating if the fields came from valid NMEA strings
//          or not.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatRequestNmeaInfo2( char *NmeaGroup, int NmeaCount,
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
        SRDAT_NMEAPARSE ParseData;
        NMEAPROCESS *t;

        // Note: Use of t->NmeaMsgIndex for setting XxxSrc assumes
        //       SRCSTR_ defines match NMEA_MSGID_ defines



        // Error check

        if ( !NmeaGroup || NmeaCount == 0 )
                return( 0 );

//WCT - Perhaps check that NmeaCount is not larger than dimension of NmeaGroup


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
        DateTmpSrc  = DateSrc; // quiet compiler about unused DateSrc;

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

                imsg = i * SRDAT_NMEA_MAX_SIZE;
                Ok = SrDatParseNmeaData( &NmeaGroup[imsg], &ParseData );
                if ( !Ok )
                        continue; // skip this unparsable msg and go on to next



                // Select NMEA table entry (t) that matches the parsed data id

                Ok = SrDatMatchNmeaTable( ParseData.Field[0], &t );
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
                        
                        if ( t->NmeaMsgIndex == SRDAT_NMEA_MSGID_GGA  &&  MsgChar == '2' )
                                MsgIsValid = 1;
                        }




                // Now gather asked for data.  Messages are ranked in
                // terms of which is best for providing each piece of
                // data.  Only fill in data if it is asked for and this
                // message has a lower and better rank than the one that
                // supplied the current data values.

                // Look for date (YMD)

                if ( DateAsk  &&  (t->DateRank < DateRank) ) {

                        DateOk = SrDatExtractParsedDate( &ParseData,
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

                        TimeOk = SrDatExtractParsedTime( &ParseData,
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

                        LocOk = SrDatExtractParsedLoc( &ParseData,
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

                        AltOk = SrDatExtractParsedAlt( &ParseData,
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

                        SatOk = SrDatExtractParsedSat( &ParseData,
                                                        t->NmeaMsgIndex,
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
        // But leave the fractional part alone if we're using PcTime
        // since that is NOT aligned on the second.

        if ( TimeAsk                                    &&
             (AcqRun.GpsModel != SRDAT_GPSMODEL_PCTIME) &&
             (AcqRun.GpsModel != SRDAT_GPSMODEL_TCXO)   ) {

                TimeOk = SrDatRoundTimeToPps( &Yr, &Mo, &Dy,
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
// ROUTINE: SrDatParseNmeaData
// PURPOSE: Split the provided NMEA message string into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatParseNmeaData( char *NmeaMsg, SRDAT_NMEAPARSE *ParseData ) {

        char *field, c;
        int   done, ifield, checksum, tmpsum, chcount;


        SrUsbXchLastErrorString[0] = '\0';


        // Error check Nmea string

        if ( !NmeaMsg ) {
                sprintf( SrUsbXchLastErrorString, "No input NMEA string given." );
                return( 0 );
                }
        if ( *NmeaMsg++ != '$' ) {
                sprintf( SrUsbXchLastErrorString, "NMEA string must start with $." );
                return( 0 );
                }



        // Initialze ParseData structure

        ParseData->Nfields  = 0;
        ParseData->CheckSum = 0;
        memset( ParseData->Field, '\0', SRDAT_NMEA_MAX_FIELDS*SRDAT_NMEA_MAX_FIELDSIZE );


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
                                sprintf( SrUsbXchLastErrorString,
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
                                if (ifield < SRDAT_NMEA_MAX_FIELDS)
                                        field = ParseData->Field[ifield];
                                else {
                                        sprintf( SrUsbXchLastErrorString,
                                           "Too many fields in NMEA string (%d >= %d).",
                                           ifield, SRDAT_NMEA_MAX_FIELDS );
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
                                if (chcount > SRDAT_NMEA_MAX_FIELDSIZE) {
                                        sprintf( SrUsbXchLastErrorString,
                                           "Too many characters in NMEA field %d (%d >= %d).",
                                           ifield, chcount, SRDAT_NMEA_MAX_FIELDSIZE );
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
                sprintf( SrUsbXchLastErrorString,
                         "Invalid checksum in NMEA string (computed 0x%X, read 0x%X).",
                         checksum, ParseData->CheckSum );
                return( 0 );
                }


        // Set number of fields since all is good
        
        ParseData->Nfields = ifield;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatMatchNmeaTable
// PURPOSE: Given a NMEA message string, determine which entry in the
//          NMEAPROCESS table has a matching message id.  The NMEAPROCESS table
//          contains information for interpreting the data fields in the various
//          NMEA messages.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatMatchNmeaTable( char *MsgId, NMEAPROCESS **t ) {

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
// ROUTINE: SrDatExtractParsedDate
// PURPOSE: Determine the Year/Month/Day date information from a NMEA message
//          string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatExtractParsedDate( SRDAT_NMEAPARSE *ParseData,
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
                        SrDatUnpack3( ddmmyy, pDy, pMo, pYr );
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
// ROUTINE: SrDatExtractParsedTime
// PURPOSE: Determine the Hour:Minute:Second time information from a NMEA message
//          string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatExtractParsedTime( SRDAT_NMEAPARSE *ParseData,
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

                SrDatUnpack3( hhmmss, pHh, pMm, pSs );
                *pUsec = 0L;
                TimeOk = 1;


                // But we'll try to get the remaining fractional part too.

                if (sf == 1)
                        *pUsec = (long)((fulltime - (double)hhmmss) * SR_USPERSEC);

                }

        return( TimeOk );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatExtractParsedLoc
// PURPOSE: Determine the Latitude-Longitude location information from a NMEA
//          message string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatExtractParsedLoc( SRDAT_NMEAPARSE *ParseData,
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
// ROUTINE: SrDatExtractParsedAlt
// PURPOSE: Determine the Altitude information from a NMEA message string that
//          has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatExtractParsedAlt( SRDAT_NMEAPARSE *ParseData,
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
// ROUTINE: SrDatExtractParsedSat
// PURPOSE: Determine the number of satellites information from a NMEA message
//          string that has been parsed into its component fields.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatExtractParsedSat( SRDAT_NMEAPARSE *ParseData,
                                        int NmeaMsgIndex,
                                        int NsatField,
                                        int *pSat ) {
        int i, Nsat, SatId, SatOk;

        SatOk = 0;

        // Special processing for GSA, adding up active satellite ids

        if ( NmeaMsgIndex == SRDAT_NMEA_MSGID_GSA ) {

                Nsat = 0;
                for ( i = 0 ; i < 12 ; i++ ) {
                        if (sscanf( ParseData->Field[NsatField+i], "%d", &SatId ) == 1) {
                                SatOk = 1;
                                Nsat++;
                                }
                        }
                *pSat = Nsat;
                }


        // Standard field lookup

        else {
                if (sscanf( ParseData->Field[NsatField], "%d", pSat ) == 1) {
                        SatOk = 1;
                        }
                }

        return( SatOk );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatRoundTimeToPps
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
FUNCTYPE( int ) SrDatRoundTimeToPps( int *Year, int *Month, int *Day,
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

        Ret = SrDatSecTimeCombine( Yr, Mo, Dy, Hh, Mm, Ss,
                                    Usec, &SecSince1970 );
        if (!Ret)
                return( 0 );

        SecSince1970 += 1.00000L;  // Here's the inc

        Ret = SrDatSecTimeSplit( SecSince1970,
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

//************************* TIME HELPER FUNCTIONS *************************

// Packed digit time is a double where two digits have been allocated to
// each of hours, minutes, and seconds (or year, month, day).  For
// example 112307 represents 23 minutes and 7 seconds past 11 o'clock.
// See SrDat.h for more details.

#define SHIFTDIG4   10000
#define SHIFTDIG2     100

//------------------------------------------------------------------------------
// ROUTINE: SrDatUnpack3
// PURPOSE: Split a packed digit time into its component digits.
//          See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatUnpack3( int Xxx, int *Top, int *Middle, int *Bottom ){

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
// ROUTINE: SrDatPack3
// PURPOSE: Create a packed digit time from its component digits.
//          See comments above.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatPack3( int Top, int Middle, int Bottom, int *Xxx  ){

        // Pack 3 pieces (ttmmbb) into one int

        *Xxx = (Top * SHIFTDIG4) + (Middle * SHIFTDIG2) + Bottom;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatSecTimeSplit
// PURPOSE: Convert time represented as seconds since 1970 into YMD HMS.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatSecTimeSplit( double Time,
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
        if (Microsecond)  *Microsecond = (long)(usec * SR_USPERSEC);

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatSecTimeSplit
// PURPOSE: Convert time represented as YMD HMS into seconds since 1970.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatSecTimeCombine( int Year, int Month, int Day,
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
        ttTime -= SrDatCalcTimeZoneCorr( ttTime );
        if (Time)  *Time  = (double)ttTime + (double)MicroSecond / (double)SR_USPERSEC;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrDatCalcTimeZoneCorr
// PURPOSE: Calculate the correction between the local time zone and UTC time.
//
//          This function does two conversions between time stored as a structure
//          and time as seconds since the epoch (1970-01-01).  They should cancel
//          out, but the first is done using GMT time and the second is done using
//          local time.  The difference gives us the amount of the local correction.
//          Subtract this value from a local time to get a GMT time.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrDatCalcTimeZoneCorr( long StartTime ) {

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


//********************** INTERPOLATION HELPER FUNCTIONS **********************

// Given the x value for one point and some other data, compute the y
// value for that point.  The other data may be two additional x,y points
// or one x,y point and a slope.  Linear interpolation is used in either
// case.

//------------------------------------------------------------------------------
// ROUTINE: SrDatEquation2pt
// PURPOSE: Compute the y value for a given x value using linear interpolation
//          done with the 2 point version of the straight line formula.
//------------------------------------------------------------------------------
int SrDatEquation2pt( double x1, double y1,
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
// ROUTINE: SrDatEquationSlope
// PURPOSE: Compute the y value for a given x value using linear interpolation
//          done with the slope and offset version of the straight line formula.
//------------------------------------------------------------------------------
int SrDatEquationSlope( double m,
                         double x1, double y1,
                         double xk, double *yk ) {

        // Error check for no place to put answer

        if ( !yk )
                return( 0 );


        // Solve the equation

        *yk = m * (xk - x1) + y1;


        return( 1 );
}




void DumpBytes( char *Array, int Nbytes ) {
        
        int i, j, nl;

        if ( !Array )
                return;
        
        j  = 1;
        nl = 0;
        ErrMsg[0] = '\0';

        for ( i = 0 ; i < Nbytes ; i++ ) {

                sprintf( &ErrMsg[nl], "%02X ", Array[i] );
                j++;
                if ( j > 16 ) {
                        SrDatError( ErrMsg );
                        ErrMsg[0] = '\0';
                        j = 1;
                        }
                nl = strlen( ErrMsg );
                }
        SrDatError( ErrMsg );
}

void DumpPacket( SRUSBXCH_PACKET *Packet ) {
        
        if ( !Packet )
                return;
        
        DumpBytes( Packet->Bytes, SRDAT_USBPACKET_SIZE );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPacketOutputBytes
// PURPOSE: Write the raw packet bytes out to the specified file in pretty format.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPacketOutputBytes( FILE *fp, SRUSBXCH_PACKET *Packet ) {

        int  i, j, BytesOnLine, CharOnLine, PacketSize, PacketType;
        char c;

        if ( !fp || !Packet )
                return( 0 );

   
        // Display raw bytes

        fprintf(  fp, "  " );
        BytesOnLine = 0;
        CharOnLine  = 0;
        PacketSize  = sizeof( SRUSBXCH_PACKET );
        PacketType  = SrUsbXchPacketType( Packet );
        
        for ( i = 0 ; i < PacketSize ; i++ ) {
                
                // Print a byte

                fprintf(  fp, "%02X ", (Packet->Bytes[i] & 0xFF) );
                BytesOnLine++;


                // Half way through a line add an extra space

                if ( BytesOnLine == 8 )
                                fprintf(  fp, " " );


                // At the end of the line add a carriage return
                // Status packets also get a gutter showing printable characters

                if ( BytesOnLine > 15 ) {

                        if ( PacketType == SRDAT_USBPACKET_TYPE_STATUS ) {

                                fprintf( fp, "   " );
                                for ( j = 0 ; j < BytesOnLine ; j++ ) {

                                        c = Packet->Bytes[CharOnLine++];

                                        if ( c < 0x20 )               // non-printable
                                                fprintf( fp, "#" );   // replace with #
                                        else                          // normal
                                                fprintf( fp, "%c", c );
                                        }
                                }

                        fprintf(  fp, "\n  " );
                        BytesOnLine = 0;
                        }
                }
        
        fprintf( fp, "\n" );

        return( 1 );
}

//********************** USER CALLABLE FUNCTIONS ****************************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchGetRev
// PURPOSE: Determine which version of the USBxCH library is being used.
//
// Short rev history:
//
//   USBXCH_REV  100  ( 2006/09/30 )  < First release
//   USBXCH_REV  101  ( 2007/05/10 )  < Updated for KMDF
//   USBXCH_REV  102  ( 2009/09/20 )  < Working with packets
//
//------------------------------------------------------------------------------

#define USBXCH_REV      102

FUNCTYPE( int ) SrUsbXchGetRev( int *Rev ) {

        *Rev = USBXCH_REV;
        return( *Rev );
}


//************************ USBxCH INITIALIZATION FUNCTIONS *********************

#define MAX_PORT_CYCLE_TRIES     10
#define MAX_BOARD_RESET_TRIES    10
        
//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOpen
// PURPOSE: To open the driver for a USBxCH A/D board.  The USBxCH
//          family of 24 bit A/D boards includes 1, 4, and 8 channel
//          models.  The XchModel parameter specifies which model you
//          are working with.
//
//          This is the function users should call to open the device
//          driver and initalize the USBxCH before starting execution.
//          It takes care of all initialization.  No other functions
//          need to be called to initialize the USBxCH.
//
//          During initialization the ADS1255 control registers are
//          programmed with values setting all board parameters like
//          sampling rate programmable gain etc.  All ADS1255s are
//          programmed in parallel with the same values.
//
//          See SrUsbXch.h for defined constants to use with the
//          parameters passed to SrUsbXchOpen.
//
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) SrUsbXchOpen(
                                  char   *DriverName,
                                  int     XchModel,
                                  int     GpsModel,
                                  int     UserCfgByte,
                                  double  Sps,

                                  double *ActualSps,
                                  int    *Error

                                  ) {

        int       Ok, ErrorCode, SpsCode, SpsIndex, Trys, CloseOnError;
        double    SpsTrue;
        DEVHANDLE UsbXchHandle;


        // Initialize the error return code.

        ErrorCode    = USBXCH_ERROR_NONE;
        CloseOnError = 0;


        // Error check all the initialization parameters:
        
        if ( XchModel != SRDAT_ATODMODEL_USB4CH ) {
                ErrorCode = USBXCH_ERROR_OPEN_ATOD_UNKNOWN;
                goto OpenError;
                }

        if ( GpsModel < SRDAT_GPSMODEL_NONE || GpsModel >= SRDAT_GPSMODEL_MAX) {
                ErrorCode = USBXCH_ERROR_OPEN_GPS_UNKNOWN;
                goto OpenError;
                }

        AcqRun.GpsModel = GpsModel;
        SrDatNmeaOrderInit( GpsModel );  // Initialize Nmea message order


//WCT - Can we check anything useful about UserCfgByte ?

        
        // Before checking the Sps parameter, convert it into an index indicating
        // which of the available sampling rates is selected and save the true
        // sampling rate for return to the caller

        Ok = SrUsbXchHelperSpsRequest( Sps, &SpsTrue, &SpsIndex, &SpsCode,
                                       &ErrorCode );

        if ( ActualSps )  *ActualSps = SpsTrue;

        if ( (!Ok)                             ||
             (SpsIndex < USBXCH_SPS_INDEX_MIN) ||
             (SpsIndex > USBXCH_SPS_INDEX_MAX) ) {
                ErrorCode = USBXCH_ERROR_OPEN_SAMPLE_RATE;
                goto OpenError;
                }

        if ( ErrorCode != USBXCH_ERROR_NONE )
                goto OpenError;

        AcqRun.SamplePeriod = 1.0L / SpsTrue;




        // Open USBxCH driver

        UsbXchHandle = SrUsbXchOsDriverOpen( DriverName );
        if ( UsbXchHandle == BAD_DEVHANDLE ) {
                SrUsbXchOsGetLastError();
                ErrorCode = USBXCH_ERROR_DRIVER_NOT_OPEN;
                goto OpenError;
                }

        CloseOnError = 1; // Now driver is open, so must close if error occurs


        // Initialize the A/D board ...
        //   Download USBxCH hardware data
        //   Start 8051 code so it can process LED and sample rate requests
        //   Verify that power is good (ie over 7.5 volts)
        //   Set user mode configuration for pps, nmea and trigger signals
        //   Set sample rate and do self calibration


        if ( ! SrUsbXchHelperDownload( UsbXchHandle, SrUsbXch_HwData ) )
                goto OpenTryPortReset;

        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) )
                goto OpenTryPortReset;

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) )
                goto OpenTryPortReset;

        if ( ! SrUsbXchHelperConfigure( UsbXchHandle, UserCfgByte,
                                        GpsModel, &ErrorCode ) )
                goto OpenTryPortReset;
  
        if ( ! SrUsbXchHelperSpsInit( UsbXchHandle, SpsIndex, &ErrorCode ) )
                goto OpenTryPortReset;

        goto OpenSuccess;



OpenTryPortReset:

        // One of the initialization functions failed, so reset the USB
        // port on the PC.
        
        if ( ! SrUsbXchPortReset( UsbXchHandle ) )
                goto OpenTryPortCycle;


        // ... and try again ...
        
        if ( ! SrUsbXchHelperDownload( UsbXchHandle, SrUsbXch_HwData ) )
                goto OpenTryPortCycle;

        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) )
                goto OpenTryPortCycle;

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) )
                goto OpenTryPortCycle;

        if ( ! SrUsbXchHelperConfigure( UsbXchHandle, UserCfgByte,
                                        GpsModel, &ErrorCode ) )
                goto OpenTryPortCycle;
  
        if ( ! SrUsbXchHelperSpsInit( UsbXchHandle, SpsIndex, &ErrorCode ) )
                goto OpenTryPortCycle;


        // Reset error code to indicate port reset was required for success

        ErrorCode = USBXCH_ERROR_OPEN_SUCCESS_PORT_RESET;
        goto OpenSuccess;



OpenTryPortCycle:

        // One of the initialization functions failed, so cycle the
        // power to the USB port on the PC causing the Device Manager to
        // update.  
        
        if ( ! SrUsbXchPortCycle( UsbXchHandle ) )
                goto OpenTryBoardReset;


        // ... and try again ...
        //
        // (but add a loop on the first function since we have to wait a
        // bit for the Device Manager update to complete).

        Trys = 0;

        while ( ! SrUsbXchHelperDownload( UsbXchHandle, SrUsbXch_HwData ) ) {

                if ( Trys < MAX_PORT_CYCLE_TRIES )      // Wait longer
                        SrUsbXchOsSleep( 500 );         // half second
                
                else                                    // Tried too long
                        goto OpenTryBoardReset;

                Trys++;
                }
        
        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) )
                goto OpenTryBoardReset;

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) )
                goto OpenTryBoardReset;

        if ( ! SrUsbXchHelperConfigure( UsbXchHandle, UserCfgByte,
                                        GpsModel, &ErrorCode ) )
                goto OpenTryBoardReset;
  
        if ( ! SrUsbXchHelperSpsInit( UsbXchHandle, SpsIndex, &ErrorCode ) )
                goto OpenTryBoardReset;


        // Reset error code to indicate port cycle was required for success
        
        ErrorCode = USBXCH_ERROR_OPEN_SUCCESS_PORT_CYCLE;
        goto OpenSuccess;



OpenTryBoardReset:

        // One of the initialization functions failed, so cycle the
        // power to the USBxCH board itself causing the driver to reload
        // and the Device Manager to update.

        UsbXchHandle = SrUsbXchPowerReset( UsbXchHandle ); // This closes driver


        // Re-open the driver, but do it in a loop since we have to wait a bit
        // for the driver reload and Device Manager update to complete.
        
        Trys         = 0;
        CloseOnError = 0;

        while ( 1 ) {

                UsbXchHandle = SrUsbXchOsDriverOpen( DriverName );
                if ( UsbXchHandle != BAD_DEVHANDLE )          // Handle is good
                        break;                                // so open is done

                if ( Trys < MAX_BOARD_RESET_TRIES )           // Wait longer
                        SrUsbXchOsSleep( 500 );               // half second

                else {                                        // Tried too long
                        ErrorCode = USBXCH_ERROR_OPEN_BOARD_RESET;
                        goto OpenError;
                        }

                Trys++;
                } // end while

        CloseOnError = 1; // Now driver is open, so must close if error occurs


        // ... and try again ...
        //
        // (but this is the last time, so set the error code indicating
        // which initialization function still didn't work).
        
        if ( ! SrUsbXchHelperDownload( UsbXchHandle, SrUsbXch_HwData ) ) {
                ErrorCode = USBXCH_ERROR_OPEN_DOWNLOAD;
                goto OpenError;
                }

        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) ) {
                ErrorCode = USBXCH_ERROR_OPEN_START_8051;
                goto OpenError;
                }

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) ) {
                ErrorCode = USBXCH_ERROR_OPEN_POWER;
                goto OpenError;
                }
        
        if ( ! SrUsbXchHelperConfigure( UsbXchHandle, UserCfgByte,
                                        GpsModel, &ErrorCode ) ) {
                goto OpenError;
                }

        if ( ! SrUsbXchHelperSpsInit( UsbXchHandle, SpsIndex, &ErrorCode ) ) {
                goto OpenError;
                }


        // Reset error code to indicate board reset was required for success

        ErrorCode = USBXCH_ERROR_OPEN_SUCCESS_BOARD_RESET;
        goto OpenSuccess;


        // Finish and return
        
OpenSuccess:

        if ( Error )  *Error = ErrorCode;

        return( UsbXchHandle );

OpenError:

        if ( CloseOnError )
                SrUsbXchClose( UsbXchHandle );

        if ( Error )  *Error = ErrorCode;

        return( BAD_DEVHANDLE );
        
}

        
//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchClose
// PURPOSE: To close the driver for a USBxCH A/D board.  Since only one program
//          can have the USBxCH driver open at a time, it is important to close
//          the driver when you are done with it.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchClose( DEVHANDLE UsbXchHandle ) {

        int Ok, Error;


        // Only close open drivers.

        if (UsbXchHandle == BAD_DEVHANDLE) {
                SrUsbXchLastDriverError = ERROR_SERVICE_DISABLED;
                return( 1 );
                }


        // Reset user mode configuration to startup default

        Ok = SrUsbXchHelperConfigure( UsbXchHandle,
                                      USERCFG_DEFAULT_BYTE,
                                      SRDAT_GPSMODEL_GARMIN,
                                      &Error );


        // Stop the 8051 code to save power

        SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_HOLD );



        // Close driver.

        if ( !SrUsbXchOsDriverClose( UsbXchHandle ) )
                return( 0 );


        if ( !Ok )
                return( 0 );


        SrUsbXchLastDriverError = 0L;
        return( 1 );
}






//********************* USBxCH EXECUTION CONTROL FUNCTIONS *********************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchStart
// PURPOSE: Start the USBxCH A/D board acquiring data.  Returns 1 on success
//          and 0 on failure.  On return, the Error argument will contain an
//          error code with more details.  Pass NULL to ignore the error code.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchStart( DEVHANDLE UsbXchHandle, int *Error ) {

        unsigned long BytesSent, Ncmd;
        int           Err;
        char          Values[2];


        // The USBxCH resets both sanity counts on A/D start
        // because of calls to 8051 DramReset and SerialReset.
        // It also resets the LastXxxPpsEventNum values.

        SrDatSanityReset(      SRDAT_RESET_ANALOG, SRDAT_RESET_SERIAL );
        SrDatPpsEventNumReset( SRDAT_RESET_ANALOG, SRDAT_RESET_SERIAL );


        // Initialize global packet and timestamp parameters

        SrUsbXchHelperAcqRunInit();

        // Reset serial processing code

        SerBufInit( ); 


//WCT - Review if anything else is reset, like UserByte or LedState


        // Verify that power is good (ie over 7.5 volts)
        
        if ( !SrUsbXchIsPowerGood( UsbXchHandle ) ) {
                if ( Error )  *Error = USBXCH_ERROR_START_POWER;
                return( 0 );
                }
        

        // Send start command

	Values[0] = USB8051_CMD_ATOD_START;
	Ncmd = 1;

	BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );
	if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
		printf("Start requested, BytesSent 0x%lX, Error %d\n", BytesSent, Err );
                if ( Error )  *Error = USBXCH_ERROR_START_COMMAND;
                return( 0 );
                }


        // Success

        if ( Error )  *Error = USBXCH_ERROR_NONE;
        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchStop
// PURPOSE: Stop the USBxCH A/D board from acquiring data.  Once acquisition
//          has been stopped, it is best to call SrUsbXchClose and SrUsbXchOpen
//          before starting again. Returns 1 on success and 0 on failure.  On
//          return, the Error argument will contain an error code with more
//          details.  Pass NULL to ignore the error code.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchStop( DEVHANDLE UsbXchHandle, int *Error ) {

        unsigned long BytesSent, Ncmd;
        int           Err;
        char          Values[2];


        // Now stop the A/D

        Values[0] = USB8051_CMD_ATOD_STOP;
        Ncmd = 1;

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );
        if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
                printf("Stop  requested, BytesSent 0x%lX, Error %d\n", BytesSent, Err );
                if ( Error )  *Error = USBXCH_ERROR_STOP_COMMAND;
                return( 0 );
                }


        // Free data areas malloc'ed in SrUsbXchHelperAcqRunInit

        SrUsbXchHelperAcqRunFree();

        
        // Success

        if ( Error )  *Error = USBXCH_ERROR_NONE;
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchGetDataAsPackets
// PURPOSE: This function reads the requested number of data packets from the
//          USBXCH over the designated data pipe.  It returns the number of
//          packets that were actually read.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned long ) SrUsbXchGetDataAsPackets(
                                               DEVHANDLE        UsbXchHandle,
                                               SRUSBXCH_PACKET *UserPacketArray,
                                               unsigned long    NpacketsRequested,
                                               unsigned long   *NpacketsRead,
                                               int             *Error
                                              ) {
        BOOL           Ok;
        char          *PacketBytes;
        unsigned long  BytesRequested, BytesRead;
        unsigned long  NpackRemain, NpackRead, NpackTotal;



        // Initialize a return parameter and do error checks

        if ( NpacketsRead )   *NpacketsRead = 0L;


        // Quick exit if no packets requested

        if ( NpacketsRequested < 1L ) {
                if (Error) *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }

        // Verify that device driver is valid.

        if ( UsbXchHandle == BAD_DEVHANDLE ) {
                SrUsbXchLastDriverError = ERROR_SERVICE_DISABLED;
                if (Error)       *Error = USBXCH_ERROR_DRIVER_NOT_OPEN;
                return( 0 );
                }

        // Verify packet array provided

        if ( !UserPacketArray ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }


        if (ShowDbgLibRead) {
                sprintf(TempStr, "SrUsbXchGetDataAsPackets - Enter (Nreq %lu)\n", NpacketsRequested );
                SrUsbXchLogMessageV( TempStr );
                }



        // Initialize packet variables

        SrUsbXchLastDriverError = 0L;
        PacketBytes             = (char *)UserPacketArray;
        NpackTotal              = 0L;
        NpackRemain             = NpacketsRequested;

        while ( NpackTotal < NpacketsRequested ) {

                // Initialize byte values

                BytesRequested = NpackRemain * SRDAT_USBPACKET_SIZE;
                BytesRead      = 0L;


                if (ShowDbgLibReadBoard) {
                        sprintf(TempStr, "SrUsbXchReadPacketsFromBoard - Enter Evt IoRead\n" );
                        SrUsbXchLogMessageV( TempStr );
                        }

                // Read data in from USBxCH

		Ok = SrUsbXchOsDriverRead(
					  UsbXchHandle,     // Handle to device
					  PacketBytes,      // Buffer to be filled with data from driver
					  BytesRequested,   // Length of buffer in bytes
					  &BytesRead        // Bytes actually read
					 );


                NpackRead    = BytesRead / SRDAT_USBPACKET_SIZE;
                PacketBytes += BytesRead;
                NpackTotal  += NpackRead;
                NpackRemain -= NpackRead;


                if (ShowDbgLibReadBoard) {
                        sprintf(TempStr, "SrUsbXchReadPacketsFromBoard - Exit Evt IoRead BytesReq %5lu, BytesRead %5lu, Npak %4lu, Ok %d\n",
                                  BytesRequested, BytesRead, NpackRead, Ok);
                        SrUsbXchLogMessageV( TempStr );
                        }


                // Check read results for general failure

                if ( !Ok ) {
                        sprintf(TempStr, "SrUsbXchReadPacketsFromBoard - Exit Evt IoRead Not Ok, read results failed\n" );
                        SrUsbXchLogMessageV( TempStr );
                        if (Error)           *Error = USBXCH_ERROR_READ_RESULTS_FAILED;
                        if ( NpacketsRead )  *NpacketsRead = NpackTotal;
                        return( 0 );
                        }

                // Check read results for partial packet read

                if ( (NpackRead * SRDAT_USBPACKET_SIZE) != BytesRead ) {
                        sprintf(TempStr, "SrUsbXchReadPacketsFromBoard - Exit Evt IoRead Invalid ! bytes read\n" );
                        SrUsbXchLogMessageV( TempStr );
                        if (Error)           *Error        = USBXCH_ERROR_READ_NO_DATA;
                        if ( NpacketsRead )  *NpacketsRead = NpackTotal;
                        return( 0 );
                        }


                // If read less than requested, do an early exit since that's all
                // the data currently available

                if ( BytesRead < BytesRequested ) {
                        if (ShowDbgLibRead) {
                                sprintf(TempStr, "SrUsbXchReadPacketsFromBoard - Exit Evt IoRead reaching empty\n" );
                                SrUsbXchLogMessageV( TempStr );
                                }
                        break;
                        }

                } // end while NpackTotal < NpacketsRequested


        if (Error)           *Error        = USBXCH_ERROR_NONE;
        if ( NpacketsRead )  *NpacketsRead = NpackTotal;


        if (ShowDbgLibRead) {
                sprintf(TempStr, "SrUsbXchGetDataAsPackets - Exit  (NpackTotal %lu)\n", NpackTotal);
                SrUsbXchLogMessageV( TempStr );
                }

        return( NpackTotal );
}

//**************************** USBxCH FIFO FUNCTIONS **************************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchEmpty
// PURPOSE: This function returns 1 when the DRAM is empty, 0 when it has
//          at least 1 sample of data, and -1 if an error occured.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchEmpty( DEVHANDLE UsbXchHandle ) {

        int Ok, Empty, PartFull, Overflow;

        Ok = SrUsbXchDramFlags(  UsbXchHandle, &Empty, &PartFull, &Overflow );

//WCT - check number of samples for empty ? 1 or 2 ?
//      since reading data is done two samples at a time

        if ( Ok )
                return( Empty );
        else
                return( -1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchReady
// PURPOSE: Check the hardware FIFO on a USBxCH A/D board to see if any
//          acquired data is ready and waiting.
//
//          SrUsbXchReady returns 1 if there is ANY data in the FIFO and 0 if the
//          FIFO is completely empty.  Typically, a return of 1 means that at
//          least one data point is ready to read.
//
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchReady( DEVHANDLE UsbXchHandle ) {

        int Ok, Empty, PartFull, Overflow;

        Ok = SrUsbXchDramFlags(  UsbXchHandle, &Empty, &PartFull, &Overflow );

//WCT - Right now we use the part full flag rather than the empty flag
//      since reading data is done two samples at a time

        if ( Ok )
                return( PartFull );
        else
                return( -1 );
}




//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOverflow
// PURPOSE: Check the hardware FIFO on a USBxCH A/D board to see if it has
//          overflowed.  Once an overflow has occurred, the data in the FIFO is
//          valid and can be read.  But you must stop and restart acquisition
//          before any data after the overflow can be acquired.
//
//          SrUsbXchOverflow returns 1 if the FIFO has overflowed and 0 otherwise.
//          Once overflow occurs, any additional data stored in the FIFO would be
//          misaligned.  So the overflow value is latched and no more data is
//          saved.  The valid data already in the FIFO can still be read out with
//          SrUsbXchGetDataAsPackets.  When you are ready to continue acquiring new data,
//          just call SrUsbXchClose, SrUsbXchOpen, and SrUsbXchStart.  This will reset
//          the FIFO and clear the overflow value so that data is saved and
//          SrUsbXchOverflow again returns 0.
//
//          The hardware FIFO has 4 Meg of 4-bit words or 2 Mbytes.  It is shared
//          among 4 channels giving 0x80000 bytes/channel.  Each 24 bit sample
//          takes 3 bytes giving 0x2AAAA samples per channel in the hardware
//          FIFO.  So, it may take a long time before the FIFO overflows.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchOverflow( DEVHANDLE UsbXchHandle ) {

        int Ok, Empty, PartFull, Overflow;

        Ok = SrUsbXchDramFlags(  UsbXchHandle, &Empty, &PartFull, &Overflow );

        if ( Ok )
                return( Overflow );
        else
                return( -1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperSetPipeNum
// PURPOSE: Using a USBXCH_PIPE_TYPE_xxx and a PIPE_DIR_xxx define, this
//          function sets the USB pipe number to use.  It fills in the
//          PipeNum parameter and returns 1 on success.
//------------------------------------------------------------------------------
int SrUsbXchHelperSetPipeNum( int PipeType, int PipeDirection, ULONG *PipeNum ) {

        // Error check

        if (!PipeNum)
                return( 0 );


        // Set input pipe number

        if (PipeDirection == PIPE_DIR_IN) {

                if (PipeType == USBXCH_PIPE_TYPE_CMD)
                        *PipeNum = PIPE_EP1_IN;

                else if (PipeType == USBXCH_PIPE_TYPE_ANALOG)
                        *PipeNum = PIPE_EP6_IN;

                else if ( PipeType == USBXCH_PIPE_TYPE_SERIAL)
                        *PipeNum = PIPE_EP8_IN;

                else
                        *PipeNum = PIPE_EP6_IN;
                }


        // Set output pipe number

        else { // PipeDirection == PIPE_DIR_OUT

                if (PipeType == USBXCH_PIPE_TYPE_CMD)
                        *PipeNum = PIPE_EP1_OUT;

                else if (PipeType == USBXCH_PIPE_TYPE_ANALOG)
                        *PipeNum = PIPE_EP2_OUT;

                else if ( PipeType == USBXCH_PIPE_TYPE_SERIAL)
                        *PipeNum = PIPE_EP4_OUT;

                else
                        *PipeNum = PIPE_EP2_OUT;
                }

        return( 1 );
}


//******************** USBxCH SAMPLE RATE HELPER FUNCTIONS ********************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSpsRateValidate
// PURPOSE: This function accepts a requested sampling rate and returns the
//          actual value of the closest allowable sampling rate.  We need to
//          do this, since the USBxCH can only sample at 16 hardwired rates
//          from about 3 to 39K sps.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchSpsRateValidate( double SpsRequested,
                                          double *SpsActual,
                                          int *Error ) {
        int Ok;

        Ok = SrUsbXchHelperSpsRequest( SpsRequested,
                                       SpsActual, NULL, NULL, Error );

        return( Ok );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSpsRateTable
// PURPOSE: This function accepts an index into the SampleRateTable
//          and returns the sampling rate associated with that entry.
//          For example, with a 10 MHz crystal, sps index 8 corresponds to a
//          sampling rate of 130 Hz.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchSpsRateTable( int SpsIndex,
                                      double *SpsActual,
                                      int *Error ) {
        int Ok, SpsCode;

        Ok = SrUsbXchHelperSpsLookup( SpsIndex, SpsActual, &SpsCode, Error );

        return( Ok );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperSpsRequest
// PURPOSE: This function accepts a requested sampling rate and returns the
//          index, code, and actual value of the closest allowable sampling
//          rate.  We need to do this, since the USBxCH can only sample
//          at certain hardwired rates.
//
//          The USBxCH can sample at 16 different rates ranging from about 3
//          to 39K sps.  These 16 distinct rates are encoded into special bit
//          patterns the ADS1255 A/D chips understand.  We call this encoding
//          the SPS Code.  For convenience, we maintain a SampleRateTable that
//          relates each rate with its code.  The SPS Index is used to index
//          into this table.
//------------------------------------------------------------------------------
int SrUsbXchHelperSpsRequest( double SpsRequested,
                              double *SpsActual,
                              int *SpsIndex,
                              int *SpsCode,
                              int *Error ) {

        int    i, Index, Index0, Index1, Code, Ok, Err;
        double Actual, Diff0, Diff1;

        // Check for maximum

        if ( SpsRequested > SampleRateTable[USBXCH_SPS_INDEX_MAX].rate ) {
                Index  = USBXCH_SPS_INDEX_MAX;
                Err    = USBXCH_ERROR_SPS_INVALID;
                Ok     = 0;
                }


        // Select next faster rate from what user requested

        else {
                Index0 = 0;
                for ( i = USBXCH_SPS_INDEX_MIN ; i < USBXCH_SPS_INDEX_MAX+1 ; i++ ) {
                        if ( SpsRequested <= SampleRateTable[i].rate ) {
                                Index0 = i;
                                break;
                                }
                        } // end for i
                Index = Index0;

                // Correct down if that rate would be closer

                if ( Index0 > 0 ) {
                        Index1 = Index0 - 1;
                        Diff0  = SampleRateTable[Index0].rate - SpsRequested;
                        Diff1  = SpsRequested - SampleRateTable[Index1].rate;
                        if (Diff0 > Diff1)
                                Index = Index1;
                        }

                Err    = USBXCH_ERROR_NONE;
                Ok     = 1;

                } // end else


        Code   = SampleRateTable[Index].code;
        Actual = SampleRateTable[Index].rate;



        // Return any results user is interested in

        if (SpsIndex)    *SpsIndex    = Index;
        if (SpsCode)     *SpsCode     = Code;
        if (SpsActual)   *SpsActual   = Actual;
        if (Error)       *Error       = Err;

        return( Ok );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperSpsLookup
// PURPOSE: This function accepts an index into the SampleRateTable
//          and returns the value and code associated with that entry.
//          For example, with a 10 MHz crystal, sps index 8 corresponds to a
//          sampling rate of 130 Hz and a code (bit encoding for the A/D) of 0x82.
//------------------------------------------------------------------------------
int SrUsbXchHelperSpsLookup( int SpsIndex,
                             double *SpsActual, int *SpsCode, int *Error ) {

        int Ok, Err, Index;


        // Set defaults

        Ok    = 1;
        Err   = USBXCH_ERROR_NONE;
        Index = SpsIndex;


        // Error check inputs

        if ( (Index < USBXCH_SPS_INDEX_MIN) || (Index > USBXCH_SPS_INDEX_MAX) ) {
                Index = 0;
                Ok    = 0;
                Err   = USBXCH_ERROR_SPS_INVALID;
                }


        // Look up code and rate values

        if (SpsActual)     *SpsActual = SampleRateTable[Index].rate;
        if (SpsCode)       *SpsCode   = SampleRateTable[Index].code;
        if (Error)         *Error     = Err;

        return( Ok );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperSpsInit
// PURPOSE: This function sets the USBxCH to the desired sample rate.  The
//          rate is specified as an index into the SampleRateTable.  For
//          For example, for a 10 MHz crystal, index USBXCH_SPS_INDEX_MIN would
//          result in setting the USBxCH to a 3.26 Hz sampling rate.
//          A self calibration is required after changing the sample rate.
//------------------------------------------------------------------------------
int SrUsbXchHelperSpsInit( DEVHANDLE UsbXchHandle, int SpsIndex, int *Error ) {

        int  Nreg, Ok, SpsCode, Err;
        char RegData[ADREG_MAX];


        // Error check input

        if (SpsIndex < USBXCH_SPS_INDEX_MIN ||
            SpsIndex > USBXCH_SPS_INDEX_MAX ) {
                printf("SrUsbXchHelperSpsInit - index < %d or > %d\n",
                       USBXCH_SPS_INDEX_MIN, USBXCH_SPS_INDEX_MAX );
                if (Error)  *Error = USBXCH_ERROR_SPS_INVALID;
                return( 0 );
                }


        // Access rate encoding value

        SpsCode = SampleRateTable[SpsIndex].code;


        // Reset A/D

        Ok = SrUsbXchAtodReset( UsbXchHandle, &Err );
        if ( !Ok ) {
                if (Error)  *Error = Err;
                return( 0 );
                }



        // Write new sampling rate to A/D register
        // Note that Rev C had RegData[1] = 0x01 for Ain0 +, Ain1 -

        RegData[0] = 0x00;          // STATUS = MSB, no auto cal, no buffer
        RegData[1] = 0x10;          // MUX    = Ain1 Positive input, Ain0 Negative input
        RegData[2] = 0x00;          // ADCON  = CLKOUT off, Sensor detect off, Gain 1
        RegData[3] = (CHAR)SpsCode; // DRATE  = SpsCode
        RegData[4] = 0x00;          // IO     = All Dx outputs and 0
        Nreg =  5;

        Ok = SrUsbXchAtodWriteReg( UsbXchHandle,
                                   ADREG_STATUS, Nreg, RegData, &Err );
        if ( !Ok ) {
                if (Error)  *Error = Err;
                return( 0 );
                }


        // Perform self calibration as required after changing sample rate

        Ok = SrUsbXchAtodCalibrate( UsbXchHandle, ADCMD_SELFCAL, &Err );

        if ( !Ok ) {
                if (Error)  *Error = USBXCH_ERROR_CALIBRATION;
                return( 0 );
                }



        if ( Error )  *Error = USBXCH_ERROR_NONE;

        return( 1 );
}



//************** USBxCH FRONT PANEL USER LED AND DIGITAL IO ****************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchFillUserCfgByte
// PURPOSE: The USBxCH has a 25 pin connector with PPS, NMEA, and trigger signals
//          digital input and output signals and two LEDs.  This function allows
//          control over some aspects of these resources.
//
//           NmeaTxPolarity     Used to program antenna, idle high (normal RS232) or low
//           NmeaRxPolarity     Used to read NMEA strings, idle high (normal RS232) or low
//           PpsLed             Red LED controlled by PPS signal or program
//           PpsPolarity        PPS signal idles low (active on rising edge) or high (falling)
//           Reserved
//           Adrdy              A/D Dready signal enabled or disabled
//           TriggerPolarity    Trigger idles low (active on rising edge) or high (falling)
//
//           The Reserved bit corresponds to PpsSource which indicated if the
//           PPS signal comes from the GPS antenna (real) or the PC (fake).  It 
//           can not be controlled directly by the user, but is set in
//           SrUsbXchHelperConfig based on the users choice of GPSMODEL.
//
//           if ( PpsSource == USERCFG_PPS_SOURCE_USB ) CfgByte |= XCRUSERCFG_PPSSOURCE;
//        
//
//           The XCRUSERCFG_ defines set bit positions that MUST correspond
//           to those actually used in the XCR ABEL code.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchFillUserCfgByte( int NmeaTxPolarity,
                                         int NmeaRxPolarity,
                                         int PpsLed,
                                         int PpsPolarity,
                                         int Adrdy,
                                         int Unused1,
                                         int TriggerPolarity,
                                         int *UserCfgByte ) {

        int CfgByte;


        // Build user mode configuration byte

        CfgByte = 0x00;

        if ( NmeaTxPolarity  == USERCFG_NMEA_TX_IDLE_LOW )   CfgByte |= XCRUSERCFG_NMEATXPOLARITY;
        if ( NmeaRxPolarity  == USERCFG_NMEA_RX_IDLE_LOW )   CfgByte |= XCRUSERCFG_NMEARXPOLARITY;
        if ( PpsLed          == USERCFG_PPS_LED_PROGRAMMED ) CfgByte |= XCRUSERCFG_PPSLED;
        if ( PpsPolarity     == USERCFG_PPS_IDLE_HIGH )      CfgByte |= XCRUSERCFG_PPSPOLARITY;
        //   PpsSource is reserved
        if ( Adrdy           == USERCFG_DB25_ADRDY_ENABLED ) CfgByte |= XCRUSERCFG_ADRDY;
        if ( TriggerPolarity == USERCFG_TRIGGER_IDLE_HIGH )  CfgByte |= XCRUSERCFG_TRIGGERPOLARITY;

        if ( UserCfgByte )  *UserCfgByte = CfgByte;


        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperConfigure
// PURPOSE: The USBxCH has a 25 pin connector with PPS, NMEA, and trigger signals
//          digital input and output signals and two LEDs.  This function allows
//          control over some aspects of these resources.
//------------------------------------------------------------------------------
int SrUsbXchHelperConfigure( DEVHANDLE UsbXchHandle, int UserCfgByte,
                             int GpsModel, int *Error ) {

        unsigned long BytesSent, Ncmd;
        int           Err;
        char          Values[6], RealGps, *FirstNmea;


        // Determine correct GPS setting

        if ( GpsModel == SRDAT_GPSMODEL_PCTIME ||
             GpsModel == SRDAT_GPSMODEL_TCXO   ) {
                // Must be given pseudo strings made from PC time
                // and the PPS source will be the USB
                RealGps = 0x00; 
                UserCfgByte |= XCRUSERCFG_PPSSOURCE;
                }
        else {
                // Real NMEA strings will be coming in
                // and the PPS source will be the DB25
                RealGps = 0x01; 
                UserCfgByte &= (~XCRUSERCFG_PPSSOURCE);
                }


//WCT - Consider consolidation with SrDatNmeaOrderInit since this info MUST match
        
        // Determine first NMEA string based on GPS model
        
        if ( GpsModel == SRDAT_GPSMODEL_PCTIME ||
             GpsModel == SRDAT_GPSMODEL_TCXO   )
                FirstNmea = "ZDA";
        else if ( GpsModel == SRDAT_GPSMODEL_GARMIN )
                FirstNmea = "RMC";
        else if ( GpsModel == SRDAT_GPSMODEL_TRIMBLE )
                FirstNmea = "GGA";
        else
                FirstNmea = "GGA";



        // Set up values to pass

        Values[0] = USB8051_CMD_CONFIGURE;
        Values[1] = UserCfgByte;
        Values[2] = RealGps;
        Values[3] = FirstNmea[0];
        Values[4] = FirstNmea[1];
        Values[5] = FirstNmea[2];
        Ncmd = 6;

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );
        if ( Err != USBXCH_ERROR_NONE ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchHelperConfig failed with BytesSent %lu\n",
                         BytesSent );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                if ( Error )  *Error = USBXCH_ERROR_CONFIGURE;
                return( 0 );
                }

        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchUserLed
// PURPOSE: The USBxCH has two LEDs, one yellow and one red, that can be
//          controlled by the user with this function.  Pass in 1 as the state
//          of the LED to turn it on and 0 to turn it off.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrUsbXchUserLed( DEVHANDLE UsbXchHandle,
                                  int YellowState,
                                  int RedState ) {

        unsigned long BytesSent, Ncmd;
        int           Err;
        char          Values[2];


        // Set up UserByte making sure to leave digital IO portion alone

        UserByte &= (~LED_MASK);        // Clear LED part

        if (YellowState)
                UserByte |= (LED_YELLOW & LED_MASK);

        if (RedState)
                UserByte |= (LED_RED & LED_MASK);


        // Prepare parameters to pass

        Values[0] = USB8051_CMD_DIGLED;
        Values[1] = UserByte;
        Ncmd = 2;


        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );
        if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchUserLed failed with BytesSent %lu\n",
                         BytesSent );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                }

        return;
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchUserIoRd
// PURPOSE: The USBxCH has 4 digital input bits that can be read by the
//          user with this function.  The current bit configuration is
//          contained in the filled Value parameter.  For example, a Value of
//          0x1 means digital input bit 0 is on and the others off.  A Value
//          of 0xC means digital input bits 3 + 4 are on, and 1 + 2 are off.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrUsbXchUserIoRd( DEVHANDLE UsbXchHandle, int *IoValue ) {

        int            Ok, DigIoValue;
        SRUSBXCH_STATE UsbState;

//WCT - is error processing needed

        // Get USBxCH state structure and extract digital input info

        Ok = SrUsbXchGetState( UsbXchHandle, &UsbState );

        if ( Ok )
                DigIoValue = UsbState.DigitalIn;
        else
                DigIoValue = 0;
        
        if ( IoValue )  *IoValue = DigIoValue;

        return;
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchUserIoWr
// PURPOSE: The USBxCH has 4 digital output bits that can be controlled
//          by the user with this function.  The desired bit configuration
//          is specified using the Value parameter.  For example, a Value of
//          0x1 turns digital output bit 0 on and the others off.  A Value
//          of 0xC turns digital output bits 3 + 4 on, and 1 + 2 off.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrUsbXchUserIoWr( DEVHANDLE UsbXchHandle, int IoValue ) {

        // Write to the USBxCH user digital output.

        unsigned long BytesSent, Ncmd;
        int           Err;
        char          Values[2];


        // Set up UserByte making sure to leave LED portion alone

        UserByte &= (~DIG_MASK);        // Clear digital part
        UserByte |= (IoValue & DIG_MASK);


        // Prepare parameters to pass

        Values[0] = USB8051_CMD_DIGLED;
        Values[1] = UserByte;
        Ncmd = 2;


        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

        if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchUserIoWr failed with BytesSent %lu\n",
                         BytesSent );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                }

        return;
}


//WCT - Consider moving these DRAM functions to diag or factory

//************************** USBxCH DRAM FIFO FUNCTIONS ***********************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchDramReset
// PURPOSE: This function resets the DRAM memory on the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrUsbXchDramReset( DEVHANDLE UsbXchHandle ) {

        unsigned long BytesSent, Ncmd;
        int           Err;
        char          Values[1];


        Values[0] = USB8051_CMD_DRAM_RESET;
        Ncmd = 1;

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

        if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchDramReset failed with BytesSent %lu\n",
                         BytesSent );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                }


        // Reset sanity because of call to 8051 DramReset

        SrDatSanityReset( SRDAT_RESET_ANALOG, SRDAT_IGNORE_SERIAL );

        return;
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchDramWrite
// PURPOSE: This function writes to the DRAM memory on the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchDramWrite( DEVHANDLE UsbXchHandle, char *Bytes,
                                  int Nbytes, int *Error ) {

        unsigned long BytesSent, Ncmd, CmdsSent;
        int           i, j, Nb, Nremain, Ndone, Npackets, Err;
        char         *Buffer;


        // Quick exit if nothing to do

        if ( Nbytes == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }


        // Error check incoming parameters

        if ( Bytes == NULL ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }



        Npackets = 1+Nbytes/(USB8051_MAX_DRAM_WRITE_BYTES);
        Buffer = (char*)malloc( Npackets*64 );
        if ( Buffer == NULL ) {
                if ( Error )  *Error = USBXCH_ERROR_NO_SPACE_AVAILABLE;
                return( 0 );
                }


        CmdsSent = 0L;
        Nremain  = Nbytes;
        Ndone    = 0;
        Ncmd     = 0;
        i        = 0;

        while ( Nremain > 0 ) {

                // Compute number of bytes to write

                Nb = Nremain;
                if ( Nb > USB8051_MAX_DRAM_WRITE_BYTES )
                        Nb = USB8051_MAX_DRAM_WRITE_BYTES;


                // Set up command values including data bytes

                Buffer[i++] = USB8051_CMD_DRAM_WRITE;
                Buffer[i++] = (CHAR)Nb;
                Ncmd += 2;

                CmdsSent += 2;

                for ( j = 0 ; j < Nb ; j++ )        // Fill data bytes
                        Buffer[i++] = Bytes[Ndone+j];

                Ncmd += Nb;


                // Update number of bytes done and remaining

                Ndone   += Nb;
                Nremain -= Nb;

                } // end while Nremain > 0



        // Send to USBxCH

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Buffer, Ncmd, &Err );

        free( Buffer );

//WCT - is error processing needed
        if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchDramWrite failed with BytesSent %lu\n",
                         BytesSent );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                }

        if ( Error ) {
                if ( Err == USBXCH_ERROR_NONE )
                        *Error = USBXCH_ERROR_NONE;
                else
                        *Error = USBXCH_ERROR_WRITE_FAILED;
                }

        Nb = (int)(BytesSent-CmdsSent);
        return( Nb );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchDramRead
// PURPOSE: This function reads from the DRAM memory on the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchDramRead( DEVHANDLE UsbXchHandle, char *Bytes,
                                int Nbytes, int *Error ) {

        unsigned long BytesSent, BytesRcvd, BytesToRead, Ncmd;
        int           Npackets, NfullBytes, NlastBytes, Err;
        int           NremainPackets, NremainBytes, NdoneBytes, Np, Nb;
        char          Values[3];

        // Quick exit if nothing to do

        if ( Nbytes == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }

        // Error check

        if ( Bytes == NULL ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }


        if ( Error )  *Error = USBXCH_ERROR_NONE;


        // Compute number of full packets and the number of
        // bytes in a last partial packet

        Npackets   = Nbytes / SRDAT_USBPACKET_SIZE;
        NfullBytes = Npackets * SRDAT_USBPACKET_SIZE;
        NlastBytes = Nbytes - NfullBytes;



        NremainPackets = Npackets;
        NremainBytes   = NlastBytes;
        NdoneBytes     = 0;


        while ( NdoneBytes < Nbytes ) {

                Np   = NremainPackets;
                Nb   = NlastBytes;
                if ( NremainPackets > USB8051_MAX_DRAM_PACKETS ) {
                        Np   = USB8051_MAX_DRAM_PACKETS;
                        Nb   = SRDAT_USBPACKET_SIZE;
                        }

                // Set up command values and send to USBxCH

                Values[0] = USB8051_CMD_DRAM_READ;
                Values[1] = (char)Np;
                Values[2] = (char)Nb;
                Ncmd = 3;

                BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );
//WCT - check BytesSent != Ncmd too?
                if ( Err != USBXCH_ERROR_NONE ) {
                        sprintf( SrUsbXchLastErrorString,
                                 "SrUsbXchDramRead failed with BytesSent %lu\n",
                                 BytesSent );
                        SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                        if ( Error )  *Error = USBXCH_ERROR_DRAM_READ_COMMAND;
                        break;
                        }



                // Read the provided results

                BytesToRead = (unsigned long)(Np*SRDAT_USBPACKET_SIZE + Nb);

                BytesRcvd = SrUsbXchReceiveResults( UsbXchHandle, &Bytes[NdoneBytes],
                                                    BytesToRead, &Err );

                if ( Err != 0x0 ) {
                        if ( Error )  *Error = USBXCH_ERROR_DRAM_READ_RESULTS;
                        break;
                        }

                if ( NremainPackets > USB8051_MAX_DRAM_PACKETS ) {
                        NremainPackets -= (USB8051_MAX_DRAM_PACKETS + 1);
                        }
                else {
                        NremainPackets -= Np;
                        NremainBytes   -= Nb;
                        }
                NdoneBytes += BytesRcvd;


                } // while NremainPackets && NremainBytes

        // Return results to user

        return( NdoneBytes );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchDramFlags
// PURPOSE: This function returns 1 if the DRAM flags are successfully read
//          and 0 otherwise.  The flags are returned in the argument list
//          parameters.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchDramFlags( DEVHANDLE UsbXchHandle, int *Empty,
                                 int *PartFull, int *Overflow ) {

        int            Ok, DramFlags;
        SRUSBXCH_STATE UsbState;


        // Get USBxCH state structure and extract DRAM flags

        Ok = SrUsbXchGetState( UsbXchHandle, &UsbState );

        if ( Ok )
                DramFlags = UsbState.DramFlags;
        else
                DramFlags = 0;


        // Split out the individual flags

        SrUsbXchDramFlagsSplit( DramFlags, Empty, PartFull, Overflow );

        return( Ok );
}



//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchDramFlagsSplit
// PURPOSE: This function takes an int containing the combined DRAM flag 
//          information returned by the USBxCH and splits out the individual
//          flags.  These flags are returned in the argument list parameters.
//          Passing NULL for arguments you are not interested in is OK.
//------------------------------------------------------------------------------
FUNCTYPE( void ) SrUsbXchDramFlagsSplit( int DramFlags, int *Empty,
                                         int *PartFull, int *Overflow ) {

        int FlagEmpty, FlagPFull, FlagOver;


        // Use defaults for unknown flags

        if ( DramFlags == USBXCH_DRAMFLAGS_UNKNOWN ) {
                FlagEmpty = USBXCH_DRAMFLAGS_UNKNOWN;
                FlagPFull = USBXCH_DRAMFLAGS_UNKNOWN;
                FlagOver  = USBXCH_DRAMFLAGS_UNKNOWN;
                }

        // Split actual flags out by bit position

        else {
                FlagEmpty = ((DramFlags >> XCRDRAMFLAGS_EMPTY)    & 0x01);
                FlagPFull = ((DramFlags >> XCRDRAMFLAGS_PARTFULL) & 0x01);
                FlagOver  = ((DramFlags >> XCRDRAMFLAGS_FULL)     & 0x01);
                }


        // Return results

        if ( Empty )     *Empty    = FlagEmpty;
        if ( PartFull )  *PartFull = FlagPFull;
        if ( Overflow )  *Overflow = FlagOver;

}



//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchGetState
// PURPOSE: This function returns 1 if the USB state variables are successfully 
//          read and 0 otherwise.  The state variables are contained in the
//          structure returned in the argument list and defined in SrUsbXch.h.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchGetState( DEVHANDLE UsbXchHandle,
                                  SRUSBXCH_STATE *UsbState ) {

        unsigned long BytesSent, BytesRcvd, Ncmd, Nres;
        int           Err;
        char          Values[1], Results[SRUSBXCH_STATE_COUNT];


        // Error check arguments

        if ( !UsbState )
                return( 0 );



        // Set up command values and send to USBxCH

        Values[0] = USB8051_CMD_GET_STATE;
        Ncmd = 1;

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );
        if ( Err != USBXCH_ERROR_NONE ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchGetState failed with BytesSent %lu\n",
                         BytesSent );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( 0 );
                }



        // Read results

        Nres = SRUSBXCH_STATE_COUNT;
        BytesRcvd = SrUsbXchReceiveResults( UsbXchHandle, Results, Nres, &Err );

        if ( BytesRcvd != Nres )
                return( 0 );


        UsbState->FirmRev   = (Results[0] & 0x00FF);
        UsbState->PowerInfo = (Results[1] & 0x00FF);
        UsbState->CountPps  = (Results[2] & 0x00FF);
        UsbState->LedValue  = (Results[3] & 0x00FF);
        UsbState->TempHi    = (Results[4] & 0x00FF);
        UsbState->TempLo    = (Results[5] & 0x00FF);
        UsbState->DigitalIn = (Results[6] & 0x00FF);
        UsbState->DramFlags = (Results[7] & 0x00FF);

        return( 1 );
}



//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSerialRead
// PURPOSE: Read Nbytes from the USBxCH serial area.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchSerialRead( DEVHANDLE UsbXchHandle, char *Bytes,
                                    int Nbytes, int *Error ) {

        unsigned long BytesSent, BytesRcvd, Ncmd, Nres, Nfill, i;
        int           Err;
        char          Values[2], Results[SRDAT_USBPACKET_SIZE];

//WCT - set up a loop if user asks for more than SRDAT_USBPACKET_SIZE at one time

        if (Nbytes > SRDAT_USBPACKET_SIZE) {
                if (Error)  *Error = USBXCH_ERROR_READ_TOO_LARGE;
                return( 0 );
                }

        if (Bytes == NULL) {
                if (Error)  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }


        Values[0] = USB8051_CMD_SERIAL_READ;
        Values[1] = (CHAR)Nbytes;
        Ncmd = 2;

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

//WCT - immediate return if send fails?
        if (BytesSent != Ncmd  ||  Err != USBXCH_ERROR_NONE) {
                if (Error)  *Error = USBXCH_ERROR_FAILED_SEND;
                }

        Nres      = Nbytes;
        BytesRcvd = SrUsbXchReceiveResults( UsbXchHandle, Results, Nres, &Err );
        if (Err != USBXCH_ERROR_NONE) {
                if (Error)  *Error = Err;
                return( (int)BytesRcvd );
                }


        Nfill = Nbytes;
        if (Nfill > BytesRcvd)
                Nfill = BytesRcvd;

        for ( i = 0 ; i < Nfill ; i++ ) {
                Bytes[i] = Results[i];
                }


        // Null out requested but unfilled entries

        for ( i = Nfill ; i < (unsigned long)Nbytes ; i++ )
                Bytes[i] = '\0';


        if (Error)  *Error = USBXCH_ERROR_NONE;

        return( (int)BytesRcvd );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSerialWrite
// PURPOSE: This function writes to the serial port on the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchSerialWrite( DEVHANDLE UsbXchHandle, char *Bytes,
                                   int Nbytes, int *Error ) {

        unsigned long BytesSent, Ncmd;
        int           i, j, Nb, Nremain, Ndone, Npackets, Err;
        char         *Buffer;


        // Quick exit if nothing to do

        if ( Nbytes == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }


        // Error check incoming parameters

        if ( Bytes == NULL ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }



        Npackets = 1+Nbytes/(SRDAT_NMEA_MAX_SIZE);
        Buffer = (char*)malloc( Npackets*64 );
        if ( Buffer == NULL ) {
                if ( Error )  *Error = USBXCH_ERROR_NO_SPACE_AVAILABLE;
                return( 0 );
                }


        Nremain = Nbytes;
        Ndone   = 0;
        Ncmd    = 0;
        i       = 0;

        while ( Nremain > 0 ) {

                // Compute number of bytes to write

                Nb = Nremain;
                if ( Nb > SRDAT_NMEA_MAX_SIZE )
                        Nb = SRDAT_NMEA_MAX_SIZE;


                // Set up command values including data bytes

                Buffer[i++] = USB8051_CMD_SERIAL_WRITE;
                Buffer[i++] = (CHAR)Nb;
                Ncmd += 2;

                for ( j = 0 ; j < Nb ; j++ )        // Fill data bytes
                        Buffer[i++] = Bytes[Ndone+j];

                Ncmd += Nb;



                // Update number of bytes done and remaining

                Ndone   += Nb;
                Nremain -= Nb;

                } // end while Nremain > 0




        // Send to USBxCH

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Buffer, Ncmd, &Err );
//WCT - is more error processing needed
//        if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd )

        free( Buffer );


        if ( Error ) {
                if ( Err == USBXCH_ERROR_NONE )
                        *Error = USBXCH_ERROR_NONE;
                else
                        *Error = USBXCH_ERROR_WRITE_FAILED;
                }

        return( (int)BytesSent );
}


//*********************** USBxCH MISCELLANEOUS FUNCTIONS *********************


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSendTimeStamp
// PURPOSE: Create a pseudo ZDA string with the current time and send it down 
//          to the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchSendTimeStamp( DEVHANDLE UsbXchHandle, int *Error ) {

        int           i, Ok, Err, Ncmd0, Ncmd, Nbytes;
        char          Values[MAXSTR+2], ZdaString[MAXSTR];
        unsigned long BytesSent;


        // Error check we're using PcTime or TCXO timing and not a GPS recevier
        
        if ( AcqRun.GpsModel != SRDAT_GPSMODEL_PCTIME &&
             AcqRun.GpsModel != SRDAT_GPSMODEL_TCXO   ) {
                if ( Error )  *Error = USBXCH_ERROR_BAD_GPS_MODEL;
                return( 0 );
                }


        // Initialize variables

        if ( Error )  *Error = USBXCH_ERROR_NONE;



        // Get PC time and generate ZDA string

        Ok = SrDatNmeaPcTimeToZDA( ZdaString, MAXSTR-1 );
        if ( !Ok ) {
                if ( Error )  *Error = USBXCH_ERROR_BAD_NMEA;
                return( 0 );
                }

        Nbytes = strlen( ZdaString );


        // Send the ZDA string down to the USBxCH

        Values[0] = USB8051_CMD_SEND_TIME;
        Values[1] = Nbytes;
        Ncmd0= 2;

        for ( i = 0 ; i < Nbytes ; i++ )        // Fill data bytes
                Values[Ncmd0+i] = ZdaString[i];

        Ncmd = Ncmd0 + Nbytes;


        // Error check that size is not too large, the CARATBYTES is to
        // compensate for the ^ bounded equipment/status bytes that will be added
        
        if ( Nbytes > (SRDAT_USBPACKET_STATUS_BYTES-SRDAT_USBXCH_MAX_CARATBYTES) ) {
                if ( Error )  *Error = USBXCH_ERROR_WRITE_TOO_LARGE;
                return( 0 );
                }

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

        if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
                if ( Error )  *Error = Err;
                return( 0 );
                }

        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchIsPowerGood
// PURPOSE: Get the power current bit status from the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchIsPowerGood( DEVHANDLE UsbXchHandle ) {

        int            Result;
        SRUSBXCH_STATE UsbState;

        // Get USBxCH state structure and extract power good info

        if ( SrUsbXchGetState( UsbXchHandle, &UsbState ) )

                Result = SrUsbXchIsPowerBitGood( UsbState.PowerInfo,
                                                 USBXCH_POWER_CURRENT );
        else
                Result = 0;
                
        return( Result );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchIsPowerBitGood
// PURPOSE: Test the specified power bit in the provided PowerInfo.  Returns
//          1 if that bit is good, 0 otherwise.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchIsPowerBitGood( int PowerInfo, int PowerBit ) {

        int PowerStatus;

        PowerStatus = (PowerInfo >> PowerBit) & 0x01;

        if ( PowerStatus == USBXCH_POWER_GOOD )
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPowerReset
// PURPOSE: Sending this command forces a hardware power down reset to occur.
//          The USBxCH must be reopened after this call is finished.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) SrUsbXchPowerReset( DEVHANDLE UsbXchHandle ) {

        DEVHANDLE     NewHandle;
        unsigned long BytesSent, Ncmd;
        int           Err;
        char          Values[2];


        // Prepare parameters to pass

        Values[0] = USB8051_CMD_POWER_RESET;
        Ncmd = 1;

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

        if ( Err != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchPowerReset failed with BytesSent %lu\n",
                         BytesSent );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                }

        SrUsbXchClose( UsbXchHandle );
        NewHandle = BAD_DEVHANDLE;

        return( NewHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchIsResetComplete
// PURPOSE: Checks to see if a board level reset is complete and the specified
//          USBxCH is ready for use again.  It does this by trying to open the 
//          driver and download the firmware so it can check the power good bit.  
//          It leaves the driver closed when done.  Returns 1 if board is ready, 
//          0 otherwise.  After a board level power reset, this function should
//          be called in a loop until it succeeds or the caller gives up.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchIsResetComplete( char *UsbxchDriverName ) {

        int       Error;
        DEVHANDLE UsbXchHandle;


        UsbXchHandle = SrUsbXchMinimalOpen( UsbxchDriverName, &Error );

        if ( UsbXchHandle == BAD_DEVHANDLE )
                return( 0 );

        if ( ! SrUsbXchHelperDownload( UsbXchHandle, SrUsbXch_HwData ) ) {
                SrUsbXchClose( UsbXchHandle );
                return( 0 );
                }

        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) ) {
                SrUsbXchClose( UsbXchHandle );
                return( 0 );
                }

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) ) {
                SrUsbXchClose( UsbXchHandle );
                return( 0 );
                }

        SrUsbXchClose( UsbXchHandle );
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchGetRevDriver
// PURPOSE: Determine which version of the USBxCH driver is being used.
//          The driver must already be open for this function to work.
//
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchGetRevDriver( DEVHANDLE UsbXchHandle, int *Rev ) {

//WCT - consider adding a driver call to return this info

        if ( Rev )  *Rev = 1;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchGetRevFirmware
// PURPOSE: Determine which version of the USBxCH firmware is being used.
//          The driver must already be open for this function to work.
//
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchGetRevFirmware( DEVHANDLE UsbXchHandle, int *Rev ) {

        int            Ok, FirmRev;
        SRUSBXCH_STATE UsbState;


        // Get USBxCH state structure and extract firmware info

        Ok = SrUsbXchGetState( UsbXchHandle, &UsbState );

        if ( Ok )
                FirmRev = UsbState.FirmRev;
        else
                FirmRev = 0;


        // Return results to user

        if (Rev)  *Rev = FirmRev;


        return( Ok );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchGetFrameNumber
// PURPOSE: Get the 1 ms USB frame number from the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchGetFrameNumber( DEVHANDLE UsbXchHandle, int *FrameNumber ) {

        unsigned long BytesSent, BytesRcvd, Ncmd, Nres;
        int           Error, FrameLow, FrameHigh, Frame;
        char          Values[1], Results[2];
        BOOL          Ok;


        Values[0] = USB8051_CMD_GET_FRAME;
        Ncmd = 1;

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Error );

        if ( Error != USBXCH_ERROR_NONE  ||  BytesSent != Ncmd ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchGetFrameNumber failed with BytesSent %lu\n",
                         BytesSent );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( 0 );
                }

        Nres      = 2;
        BytesRcvd = SrUsbXchReceiveResults( UsbXchHandle, Results, Nres, &Error );

        if ( Error != USBXCH_ERROR_NONE  ||  BytesRcvd != Nres ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchGetFrameNumber failed with BytesRcvd %lu\n",
                         BytesRcvd );
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( 0 );
                }
        
        FrameHigh = (Results[1] & 0x00FF);
        FrameLow  = (Results[0] & 0x00FF);
        Frame     = ( FrameHigh << 8 ) | FrameLow;

        if (FrameNumber)   *FrameNumber = Frame;

//WCT - Choose method for getting frame number

        // Now get frame via standard method

        Frame     = 0L;
        BytesRcvd = 0L;
        Ok        = 0;
#ifdef USE_OLD_STYLE_IOCTL
        Ok = SrUsbXchOsDriverIoctl( UsbXchHandle,
                                  IOCTL_USBXCH_GET_FRAME,
                                  NULL,
                                  0,
                                  &Frame,
                                  sizeof(Frame),
                                  &BytesRcvd );
#endif // USE_OLD_STYLE_IOCTL

        if ( !Ok ) {
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( 0 );
                }

        return( 1 );
}








//*****************************************************************************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSampleDataInit
// PURPOSE: Initialize a sample data structure.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchSampleDataInit( SRUSBXCH_SAMPLEDATA *SampleData ) {

        int i;

        if ( !SampleData )
                return( 0 );
        
        SampleData->PpsCount     = -1;
        SampleData->CurrentPt    = -1;
        SampleData->DigitalIn    = 0x00;
        SampleData->Sanity       = 0x00;
        SampleData->PpsToggle    = 0x00;
        SampleData->GGACount     = 0x00;
        SampleData->OnBoardCount = 0x0000;
        SampleData->SampleTime   = SRDAT_INVALID_DOUBLE;

        for ( i = 0 ; i < USBXCH_MAX_ANALOG_CHANNELS ; i++ )
                SampleData->Channel[i] = 0L;
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchStatusDataInit
// PURPOSE: Initialize a status data structure.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchStatusDataInit( SRUSBXCH_STATUSDATA *StatusData ) {
        
        if ( !StatusData )
                return( 0 );
        
        StatusData->PpsCount    = -1;
        StatusData->CurrentPt   = -1;
        StatusData->PowerInfo   = USBXCH_POWER_UNKNOWN;
        StatusData->DramFlags   = USBXCH_DRAMFLAGS_UNKNOWN;
        StatusData->Temperature = USBXCH_TEMP_UNKNOWN;
        StatusData->NmeaCount   = 0;
        
        memset( StatusData->NmeaMsg, 0, SRDAT_NMEA_MAX_BUFF );
        SrDatNmeaInfoInit( &StatusData->NmeaInfo );

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPacketsToColumns
// PURPOSE: Convert the provided packets into column data.  This function will
//          not return any column data until timing info can be computed because 
//          enough packets have been provided.  nPackets = 0 is allowed.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPacketsToColumns( int FinalProcess,
                             SRUSBXCH_PACKET *PacketArray, int nPackets,
                             SRUSBXCH_SAMPLEDATA *SampleData, int maxSample, int *nSample,
                             SRUSBXCH_STATUSDATA *StatusData, int maxStatus, int *nStatus, 
                             int *Error ) {
        int Result;
        
        Result = SrUsbXchFullPacketsToColumns( FinalProcess, PacketArray, nPackets,
                                              SampleData, maxSample, nSample,
                                              StatusData, maxStatus, nStatus, 
                                              NULL, 0, NULL, Error );

        return( Result );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchFullPacketsToColumns
// PURPOSE: This full version also returns the intermediate TimeStamp data which
//          can be useful for debugging. Convert the provided packets into column 
//          data.  This function will not return any column data until timing 
//          info can be computed because enough packets have been provided.
//          nPackets = 0 is allowed.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchFullPacketsToColumns( int FinalProcess,
                     SRUSBXCH_PACKET *PacketArray, int nPackets,
                     SRUSBXCH_SAMPLEDATA *SampleData, int maxSample, int *nSample,
                     SRUSBXCH_STATUSDATA *StatusData, int maxStatus, int *nStatus, 
                     TS *TsData, int maxTs, int *nTs,
                     int *Error ) {

        int              i, ptype, ErrorCode, Result, nSampleCount, nStatusCount;
        SRUSBXCH_PACKET *pPacket;


        // Set some error defaults

        if ( nSample )  *nSample = 0;
        if ( nStatus )  *nStatus = 0;
        if ( nTs )      *nTs     = 0;


        // Error check inputs

        if ( !SampleData  ||  !nSample ||
             !StatusData  ||  !nStatus ||
             !PacketArray ||  nPackets < 0 ) { // packet array not needed for final processing
                if ( Error )    *Error   = USBXCH_ERROR_CONVERT_PAK_BAD_INPUT;
                sprintf( SrUsbXchLastErrorString, "Error bad input to SrUsbXchConvertPak2Column\n" );
                return( 0 );
                }

        // TsData optional, but if given, size must be at least 2

        if ( TsData && maxTs < 2 ) {
                if ( Error )    *Error   = USBXCH_ERROR_CONVERT_PAK_BAD_OPT_INPUT;
                sprintf( SrUsbXchLastErrorString, "Error bad Ts input to SrUsbXchConvertPak2Column\n" );
                return( 0 );
                }


        // Initialize variables

        AcqRun.SampleData     = SampleData;
        AcqRun.SampleMaxCount = maxSample;
        AcqRun.SampleCount    = 0;

        AcqRun.StatusData     = StatusData;
        AcqRun.StatusMaxCount = maxStatus;
        AcqRun.StatusCount    = 0;

        AcqRun.TsData         = TsData;
        AcqRun.TsMaxCount     = maxTs;
        AcqRun.TsCount        = 0;

        Result                = 1;
        
        nSampleCount          = 0;
        nStatusCount          = 0;
        

        // Copy provided packets into our saved circular buffer packet array

        for ( i = 0 ; i < nPackets ; i++ ) {

                pPacket = &PacketArray[i];


                // Check the packet sanity error codes.

//WCT - is error return needed
                SrUsbXchHelperPktCheckSanity( pPacket, 0 );


                // Get timestamp info based on packet type.

                ptype = SrUsbXchPacketType( pPacket );


                // Save packet in type specific packet array

                if ( ptype == SRDAT_USBPACKET_TYPE_SAMPLE ) {

                        AcqRun.SamplePacketArray[AcqRun.PaSampleWritePos] = *pPacket;
                        
                        AcqRun.PaSampleWritePos++;
                        if ( AcqRun.PaSampleWritePos >= MAX_ACQRUN_SAMPLE_PACKETS ) {
                                AcqRun.PaSampleWritePos -= MAX_ACQRUN_SAMPLE_PACKETS;
                                }

                        nSampleCount++;         // sample packets this time
                        AcqRun.PaSampleCount++; // total sample packets written in array
                        if ( AcqRun.PaSampleCount >= MAX_ACQRUN_SAMPLE_PACKETS ) {
                                sprintf( SrUsbXchLastErrorString,
                                         "P2C Error internal sample packet array is full (#packets = %d)\n",
                                         MAX_ACQRUN_SAMPLE_PACKETS );
                                if ( Error )  *Error = USBXCH_ERROR_CONVERT_PAK_ARRAY_FULL;
                                return( 0 );
                                }
                        }

                else if ( ptype == SRDAT_USBPACKET_TYPE_STATUS ) {
                
                        AcqRun.StatusPacketArray[AcqRun.PaStatusWritePos] = *pPacket;

                        AcqRun.PaStatusWritePos++;
                        if ( AcqRun.PaStatusWritePos >= MAX_ACQRUN_STATUS_PACKETS ) {
                                AcqRun.PaStatusWritePos -= MAX_ACQRUN_STATUS_PACKETS;
                                }

                        nStatusCount++;
                        AcqRun.PaStatusCount++;
                        if ( AcqRun.PaStatusCount >= MAX_ACQRUN_STATUS_PACKETS ) {
                                sprintf( SrUsbXchLastErrorString,
                                         "P2C Error internal status packet array is full (#packets = %d)\n",
                                         MAX_ACQRUN_STATUS_PACKETS );
                                if ( Error )  *Error = USBXCH_ERROR_CONVERT_PAK_ARRAY_FULL;
                                return( 0 );
                                }
                        }

                else { // unrecognized packet type

                        if ( Error )  *Error = USBXCH_ERROR_CONVERT_PAK_TYPE_UNKNOWN;
                        sprintf( SrUsbXchLastErrorString, "P2C Error unknown input packet type found, fatal !!!\n" );
                        return( 0 );
                        }

                } // end for i < nPackets


        // Collect Time Info for sample packets

        while ( nSampleCount > 0 ) {

                // Select current sample packet.  We use PeekPos here
                // instead of ReadPos because we wish to leave the
                // sample packets alone so they will be available to
                // read when processing packets

                pPacket = &AcqRun.SamplePacketArray[AcqRun.PaSamplePeekPos];

                AcqRun.PaSamplePeekPos++;
                if ( AcqRun.PaSamplePeekPos >= MAX_ACQRUN_SAMPLE_PACKETS ) {
                        AcqRun.PaSamplePeekPos -= MAX_ACQRUN_SAMPLE_PACKETS;
                        }

                // Get timestamp info

                Result = SrUsbXchHelperTsFillFromSample( pPacket, &ErrorCode );

                if ( !Result ) {
                        if ( Error )  *Error = ErrorCode;
                        sprintf( SrUsbXchLastErrorString, "TsFillFromSample error %d = %s\n", ErrorCode, USBXCH_ERROR_MSG[ErrorCode] );
                        return( 0 );
                        }

                AcqRun.nSamplePackets++; // sample packets written AND tsfilled
                nSampleCount--;
                }



        // Collect Time Info for status packets 

        while ( nStatusCount > 0 ) {

                // Select current status packet.  It is ok to use
                // ReadPos here since we have saved all the pertinent
                // data into the status array and we won't need to
                // access the status packets again.

                pPacket = &AcqRun.StatusPacketArray[AcqRun.PaStatusReadPos];

                AcqRun.PaStatusCount--;
                AcqRun.PaStatusReadPos++;
                if ( AcqRun.PaStatusReadPos >= MAX_ACQRUN_STATUS_PACKETS ) {
                        AcqRun.PaStatusReadPos -= MAX_ACQRUN_STATUS_PACKETS;
                        }

                // Get timestamp info

                Result = SrUsbXchHelperTsFillFromStatus( pPacket, &ErrorCode );

                if ( !Result ) {
                        if ( Error )  *Error = ErrorCode;
                        sprintf( SrUsbXchLastErrorString, "TsFillFromStatus error %d = %s\n", ErrorCode, USBXCH_ERROR_MSG[ErrorCode] );
                        return( 0 );
                        }

                AcqRun.nStatusPackets++;
                nStatusCount--;
                }


        // Combine sample and status time info into total TS, then process

        Result = SrUsbXchHelperTsCombine( &ErrorCode );
        if ( !Result ) {
                if ( Error )  *Error = ErrorCode;
                sprintf( SrUsbXchLastErrorString, "HelperTsCombine error %d = %s\n", ErrorCode, USBXCH_ERROR_MSG[ErrorCode] );
                return( 0 );
                }


        Result = SrUsbXchHelperTsProcessValid( &ErrorCode );

        if ( Result == 1  &&  AcqRun.nTsValid == 2 ) {
                Result = SrUsbXchHelperProcessPackets( 1, &ErrorCode ); // Bounded
                if ( Result == 0  &&  ErrorCode != USBXCH_ERROR_NONE ) {
                        if ( Error )  *Error = ErrorCode;
                        sprintf( SrUsbXchLastErrorString, "P2C Error could not process packet\n" );
                        return( 0 );
                        }
                }
        else {
                Result = 1;
                }


        if ( FinalProcess ) {
                Result = SrUsbXchHelperProcessPackets( 0, &ErrorCode ); // NOT Bounded
                if ( Result == 0  &&  ErrorCode != USBXCH_ERROR_NONE ) {
                        if ( Error )  *Error = ErrorCode;
                        sprintf( SrUsbXchLastErrorString, "P2C Error could not final process packet\n" );
                        return( 0 );
                        }
                } // end if FinalProcess

//WCT consider adding a variable to indicate if more data is ready and waiting        


        // Return results

        if ( nSample )  *nSample = AcqRun.SampleCount;
        if ( nStatus )  *nStatus = AcqRun.StatusCount;
        if ( nTs )      *nTs     = AcqRun.TsCount;
        if ( Error )    *Error   = USBXCH_ERROR_NONE;

        return( Result );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperTsFillFromSample
// PURPOSE: Extract timestamp info from sample type packet.  This function
//          assumes the packet has already been identified as sample type.
//          Even though we're working with sample data here, we must pass in
//          status data since it might need to be corrected.
//------------------------------------------------------------------------------
int SrUsbXchHelperTsFillFromSample( SRUSBXCH_PACKET *pPacket, int *Error ) {

        SRDAT_USBXCH_ANALOG AnalogValues[2];
        int                 PpsCount;
        int                 ObcCount;
        int                 ObcPt;
        int                 j;
        int                 ErrorCode;
        int                 PpsToggle;
        int                 PpsEvent;
        int                 PpsEventCorrected;
        double              ExtraSec;
        unsigned long       Nsamples;
        TS                  *TsCurrent;

        if ( !pPacket ) {
                sprintf( SrUsbXchLastErrorString, "Bad argument\n" );
                if ( Error )  *Error = USBXCH_ERROR_TS_FILL_SAMPLE_BAD_INPUT;
                return( 0 );
                }

        Nsamples = SrDatUsbAnalogFill( pPacket, 0, AnalogValues, &ErrorCode );

        if ( ( ErrorCode != USBXCH_ERROR_NONE ) || ( Nsamples != 2 ) ) {
                sprintf( SrUsbXchLastErrorString,
                         "TsFillFromSample Processing sample packet failed, error %d, nsamples %lu\n",
                         ErrorCode, Nsamples );
                if ( Error )  *Error = USBXCH_ERROR_TS_FILL_SAMPLE_BAD_PKT;
                return( 0 );
                }
        

        for ( j = 0 ; j < 2 ; j++ ) {

                PpsEvent = AnalogValues[j].GGACount;
                if ( PpsEvent == AcqRun.PpsEventLast )
                        PpsEventCorrected = PpsEvent+1;
                else
                        PpsEventCorrected = PpsEvent;

                PpsCount  = PpsEventCorrected & 0x0F;
                ObcCount  = AnalogValues[j].OnBoardCount;
                PpsToggle = AnalogValues[j].PpsToggle;

                if ( SrUsbXchHelperIsValidOBC( PpsToggle, &ObcCount, PpsCount ) ) {

                    ObcPt    = AcqRun.nPoints;
                    ExtraSec = ((double)ObcCount) / USBXCH_OBC_FREQ;
                    AcqRun.nObc++;

                    // Get current TS and error check

                    TsCurrent = &AcqRun.TsSample[AcqRun.TsSampleWritePos];
                    
                    if ( TsCurrent->Valid & SRDAT_VALID_PPS ) {
                            sprintf( SrUsbXchLastErrorString, "*** ERROR: Sample TS array is full. old PpsCount %d, new PpsCount %d. Valid 0x%02X TS[%d]\n",
                                    TsCurrent->PpsCount, PpsCount, TsCurrent->Valid, AcqRun.TsSampleWritePos );
                            if ( Error )  *Error = USBXCH_ERROR_TS_FILL_SAMPLE_TS_FULL;
                            return( 0 );
                            }

                    if ( PpsCount == AcqRun.ObcPpsCountLast ) {
                            sprintf( SrUsbXchLastErrorString, "*** WARNING: Sample TS PpsCount %d unchanged from last OBC. TS[%d]\n",
                                    PpsCount, AcqRun.TsSampleWritePos );
                            }

                    

                    // Fill in sample timestamp info
                    
                    TsCurrent->Valid     |= SRDAT_VALID_PPS;
                    TsCurrent->PpsCount   = PpsCount;
                    TsCurrent->Sample     = ObcPt;
                    TsCurrent->ExtraSec   = ExtraSec;
                    TsCurrent->ObcCount   = ObcCount;

                    AcqRun.TsSampleCount++;
                    AcqRun.TsSampleWritePos++;
                    if ( AcqRun.TsSampleWritePos >= MAX_ACQRUN_TS )
                            AcqRun.TsSampleWritePos -= MAX_ACQRUN_TS;

                    AcqRun.ObcPpsCountLast = PpsCount;

                    } // end if IsValidOBC

                AcqRun.ObcLast       = ObcCount;
                AcqRun.PpsToggleLast = PpsToggle;
                AcqRun.PpsEventLast  = PpsEvent;
                AcqRun.nPoints++;

                } // end for j

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperTsFillFromStatus
// PURPOSE: Extract timestamp info from status type packet. This function
//          assumes the packet has already been identified as status type.
//------------------------------------------------------------------------------
int SrUsbXchHelperTsFillFromStatus( SRUSBXCH_PACKET *pPacket, int *Error ) {

        int                    Ok;
        int                    ErrorCode;
        int                    PpsCount;
        int                    errNmea;
        int                    iNmea;
        int                    nChar;
        int                    nSecond;
        int                    EqFlag;
        int                    i;
        int                    istart;
        int                    istatus;
        int                    LastNumChar;
        int                    NumChar;
        NI                     NmeaInfo;
        SRDAT_USBXCH_EQUIPMENT EquipData;
        char                   NmeaMsgString[SRDAT_NMEA_MAX_SIZE];
        TS                    *TsCurrent;

        // Error check
        
        if ( !pPacket ) {
                if ( Error )  *Error = USBXCH_ERROR_TS_FILL_STATUS_BAD_INPUT;
                sprintf( SrUsbXchLastErrorString, "TsFillFromStatus error: Must supply a valid packet\n" );
                return( 0 );
                }


        // Put status packet contents into serial buffer

        Ok = SrDatUsbSerialFillBuffer( pPacket, 0, &ErrorCode );

        if (!Ok || ErrorCode != USBXCH_ERROR_NONE ) {
                sprintf( SrUsbXchLastErrorString, "TsFillFromStatus SrDatUsbSerialFillBuffer error %d, ok %d\n", ErrorCode, Ok );
                if ( Error )  *Error = ErrorCode;
                return( 0 );
                }

        // Serial buffer is too full for one more serial packet

        if ( SerBufNumFree() < SRDAT_USBPACKET_STATUS_BYTES ) {
                sprintf( SrUsbXchLastErrorString, "TsFillFromStatus Serial buffer too full\n" );
                if ( Error )  *Error = USBXCH_ERROR_TS_FILL_STATUS_SERIAL_FULL;
                return( 0 );
                }


        NumChar = SerBufNumChar();
        
        while ( NumChar > 0 ) {

                // Parse serial buffer for NMEA string

                errNmea = SrDatSerialReadNmea(
                                        SRDAT_NMEA_MAX_SIZE,
                                        NmeaMsgString,
                                        NULL,                   // don't check MSGID
                                        &nChar,
                                        &nSecond,
                                        &EqFlag,
                                        &EquipData
                                       );

                if ( errNmea == SRDAT_NMEA_ERR_NONE ) {

                        if ( nSecond != -1 ) {              // We found first string in a group

                                if ( (AcqRun.nSecondLast != nSecond )  &&
                                     (AcqRun.nSecondLast != -1)  ) {

                                        SrUsbXchHelperExtractNmeaInfo( AcqRun.NmeaGroup,
                                                                       AcqRun.nNmea, &NmeaInfo );

                                        PpsCount = AcqRun.nSecondLast & 0x0F;

                                        // Fill in status timestamp info

                                        TsCurrent = &AcqRun.TsStatus[AcqRun.TsStatusWritePos];

                                        if ( TsCurrent->Valid & SRDAT_VALID_NMEA ) {
                                                sprintf( SrUsbXchLastErrorString, "*** ERROR: Status TS array is full. old PpsCount %d, new PpsCount %d. Valid 0x%02X TS[%d]\n",
                                                        TsCurrent->PpsCount, PpsCount, TsCurrent->Valid, AcqRun.TsStatusWritePos );
                                                if ( Error )  *Error = USBXCH_ERROR_TS_FILL_STATUS_TS_FULL;
                                                return( 0 );
                                                }


                                        TsCurrent->Valid       |= SRDAT_VALID_NMEA;
                                        TsCurrent->PpsCount     = PpsCount;
                                        TsCurrent->NumSat       = NmeaInfo.Nsat;
                                        TsCurrent->YmdSource    = NmeaInfo.YmdSource;
                                        TsCurrent->HmsSource    = NmeaInfo.HmsSource;
                                        TsCurrent->PowerInfo    = EquipData.VoltageGood;
                                        TsCurrent->Temperature  = EquipData.Temperature;
                                        TsCurrent->SecSince1970 = NmeaInfo.SecSince1970;

                                        AcqRun.TsStatusCount++;
                                        AcqRun.TsStatusWritePos++;
                                        if ( AcqRun.TsStatusWritePos >= MAX_ACQRUN_TS )
                                                AcqRun.TsStatusWritePos -= MAX_ACQRUN_TS;


                                        // Also fill in status array

                                        istatus = AcqRun.SaWritePos;

                                        AcqRun.StatusArray[istatus].CurrentPt   = 0; // Filled in later

                                        AcqRun.StatusArray[istatus].PpsCount    = TsCurrent->PpsCount;
                                        AcqRun.StatusArray[istatus].PowerInfo   = TsCurrent->PowerInfo;
                                        AcqRun.StatusArray[istatus].DramFlags   = EquipData.DramFlags;
                                        AcqRun.StatusArray[istatus].Temperature = TsCurrent->Temperature;
                                        AcqRun.StatusArray[istatus].NmeaInfo    = NmeaInfo;
                                        AcqRun.StatusArray[istatus].NmeaCount   = AcqRun.nNmea;

                                        istart = 0;
                                        for ( i = 0 ; i < AcqRun.StatusArray[istatus].NmeaCount ; i++ ) {
                                                strcpy( &AcqRun.StatusArray[istatus].NmeaMsg[istart],
                                                        &AcqRun.NmeaGroup[istart] );
                                                istart += SRDAT_NMEA_MAX_SIZE;
                                                }

                                        AcqRun.SaWritePos++;
                                        if ( AcqRun.SaWritePos >= MAX_ACQRUN_STATUS_ARRAY )
                                                AcqRun.SaWritePos -= MAX_ACQRUN_STATUS_ARRAY;

                                        AcqRun.SaCount++;
                                        if ( AcqRun.StatusCount >= MAX_ACQRUN_STATUS_ARRAY ) {
                                                sprintf( SrUsbXchLastErrorString, "***** ERROR: SrUsbXch Status Array Full in TsFillFromStatus **** %d\n",
                                                        AcqRun.SaCount );
                                                if ( Error )  *Error = USBXCH_ERROR_TS_FILL_STATUS_STATUS_FULL;
                                                return( 0 );
                                                }


                                        } // end if nLastSecond != nSecond


                                AcqRun.nNmea       = 1;
                                AcqRun.nSecondLast = nSecond;
                                }
                        else              // We found trailing string in a group
                                AcqRun.nNmea++;

                        // Save NmeaMsgString into the NmeaGroup for later processing

                        iNmea = (AcqRun.nNmea-1) * SRDAT_NMEA_MAX_SIZE;
                        if ( AcqRun.nNmea <= SRDAT_NMEA_MAX_TYPE ) {
                                strcpy( &AcqRun.NmeaGroup[iNmea], NmeaMsgString );
                                }
                        else {
                                if ( Error )  *Error = USBXCH_ERROR_TS_FILL_STATUS_NMEA_FULL;
                                sprintf( SrUsbXchLastErrorString,
                                         "SRDAT SrDatUsbSerialParseBuffer too many NMEA messages per second, expecting 2, got at least %d", SRDAT_NMEA_MAX_TYPE );
                                return( 0 );
                                }
                        } // endif errNmea == SRDAT_NMEA_ERR_NONE

                else { // ( errNmea != SRDAT_NMEA_ERR_NONE )

                        // Fatal error, exit from program

                        if ( errNmea == SRDAT_NMEA_ERR_INVALID_PARM ) {
                                sprintf( SrUsbXchLastErrorString, "TsFillFromStatus PeekPackets failed processing NMEA strings (error = %d)", errNmea );
                                if ( Error )  *Error = USBXCH_ERROR_TS_FILL_STATUS_BAD_NMEA;
                                return( 0 );
                                }

                        // Else not enough data so continue and try again later

                        } // end else errNmea != SRDAT_NMEA_ERR_NONE

                LastNumChar = NumChar;
                NumChar     = SerBufNumChar();
        
                if ( NumChar == LastNumChar ) {
                        break;
                        }

                } // end while NumChar > SRDAT_MAX_NMEA_SIZE )

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperTsCombine
// PURPOSE: Combine timestamp info from sample and status packets.
//------------------------------------------------------------------------------
int SrUsbXchHelperTsCombine( int *Error ) {

        int StatusVsSample;
        TS  *TsSample, *TsStatus, *TsTotal;

        if ( Error )  *Error = USBXCH_ERROR_NONE;

        // Loop around combining TS info while both types of time info are available

        while ( AcqRun.TsSampleCount > 0  &&  AcqRun.TsStatusCount > 0 ) {

                TsStatus = &AcqRun.TsStatus[AcqRun.TsStatusReadPos];
                TsSample = &AcqRun.TsSample[AcqRun.TsSampleReadPos];
                TsTotal  = &AcqRun.TsTotal[AcqRun.TsTotalWritePos];

                // If the PpsCounts match, combine the time info,
                // save it in the TsTotal TS array, and update the
                // appropriate positions and counts.

                if ( TsSample->PpsCount == TsStatus->PpsCount ) {

                        TsTotal->Valid        = TsSample->Valid | TsStatus->Valid;
                        TsTotal->PpsCount     = TsSample->PpsCount;
                        TsTotal->Sample       = TsSample->Sample;
                        TsTotal->ExtraSec     = TsSample->ExtraSec;
                        TsTotal->ObcCount     = TsSample->ObcCount;
                        TsTotal->SecSince1970 = TsStatus->SecSince1970;
                        TsTotal->NumSat       = TsStatus->NumSat;
                        TsTotal->YmdSource    = TsStatus->YmdSource;
                        TsTotal->HmsSource    = TsStatus->HmsSource;
                        TsTotal->PowerInfo    = TsStatus->PowerInfo;
                        TsTotal->Temperature  = TsStatus->Temperature;
                        TsTotal->TotalTime    = TsStatus->SecSince1970
                                              + TsSample->ExtraSec;

                        // Clear used TS sample info and update pos + count

                        SrDatTimestampInit( &AcqRun.TsSample[AcqRun.TsSampleReadPos] );
                        AcqRun.TsSampleCount--;
                        AcqRun.TsSampleReadPos++;
                        if ( AcqRun.TsSampleReadPos >= MAX_ACQRUN_TS )
                                AcqRun.TsSampleReadPos -= MAX_ACQRUN_TS;


                        // Clear used TS status info and update pos + count

                        SrDatTimestampInit( &AcqRun.TsStatus[AcqRun.TsStatusReadPos] );
                        AcqRun.TsStatusCount--;
                        AcqRun.TsStatusReadPos++;
                        if ( AcqRun.TsStatusReadPos >= MAX_ACQRUN_TS )
                                AcqRun.TsStatusReadPos -= MAX_ACQRUN_TS;


                        // Update pos + count for newly added TS total info

                        AcqRun.TsTotalWritePos++;
                        if ( AcqRun.TsTotalWritePos >= MAX_ACQRUN_TS )
                                AcqRun.TsTotalWritePos -= MAX_ACQRUN_TS;

                        AcqRun.TsTotalCount++;
                        if ( AcqRun.TsTotalCount >= MAX_ACQRUN_TS ) {
                                sprintf( SrUsbXchLastErrorString, "TsCombine Error: Internal timestamp buffer overflow\n" );
                                if ( Error )  *Error = USBXCH_ERROR_TSCOMBINE_TS_FULL;
                                return( 0 );
                                }

                        }


                // PpsCounts do not match, so adjust the lower one
                // and then continue around the while loop to try again
                
                else { // TsSample->PpsCount != TsStatus->PpsCount

                        StatusVsSample = SrUsbXchHelperPpsCountCompare( TsSample->PpsCount,
                                                                        TsStatus->PpsCount );
                        
                        // If Status PpsCount > Sample PpsCount, update the Sample TS

                        if ( StatusVsSample > 0 ) { // TsSample->PpsCount < TsStatus->PpsCount

                                // Discard old Sample TS

                                SrDatTimestampInit( &AcqRun.TsSample[AcqRun.TsSampleReadPos] );
                                AcqRun.TsSampleCount--;
                                AcqRun.TsSampleReadPos++;
                                if ( AcqRun.TsSampleReadPos >= MAX_ACQRUN_TS )
                                        AcqRun.TsSampleReadPos -= MAX_ACQRUN_TS;

                                // Exit if there are no Sample TS available

                                if ( AcqRun.TsSampleCount <= 0 ) {
                                        sprintf( SrUsbXchLastErrorString, "TsCombine Can't update Sample PpsCount %d to match Status PpsCount %d\n",
                                                 TsSample->PpsCount, TsStatus->PpsCount );
                                        if ( Error )  *Error = USBXCH_ERROR_TSCOMBINE_NO_MORE_SAMPLE;
                                        return( 1 );  // Not a fatal error
                                        }

                                // Get new Sample TS

                                TsSample = &AcqRun.TsSample[AcqRun.TsSampleReadPos];

                                } // end if PpsCount status > sample


                        // If Status PpsCount < Sample PpsCount, update Status TS

                        else if ( StatusVsSample < 0 ) { // TsStatus->PpsCount < TsSample->PpsCount

                                // Discard old Status TS and matching status array entry

                                SrDatTimestampInit( &AcqRun.TsStatus[AcqRun.TsStatusReadPos] );
                                AcqRun.TsStatusCount--;
                                AcqRun.TsStatusReadPos++;
                                if ( AcqRun.TsStatusReadPos >= MAX_ACQRUN_TS )
                                        AcqRun.TsStatusReadPos -= MAX_ACQRUN_TS;

                                AcqRun.SaCount--;
                                AcqRun.SaReadPos++;
                                if ( AcqRun.SaReadPos >= MAX_ACQRUN_STATUS_ARRAY )
                                        AcqRun.SaReadPos -= MAX_ACQRUN_STATUS_ARRAY;

                                // Exit if there are no Status TS available

                                if ( AcqRun.TsStatusCount <= 0 ) {
                                        sprintf( SrUsbXchLastErrorString, "TsCombine Can't update Status PpsCount %d to match Sample PpsCount %d\n",
                                                  TsStatus->PpsCount, TsSample->PpsCount );
                                        if ( Error )  *Error = USBXCH_ERROR_TSCOMBINE_NO_MORE_STATUS;
                                        return( 1 );  // Not a fatal error
                                        }

                                // Get new Status TS

                                TsStatus = &AcqRun.TsStatus[AcqRun.TsStatusReadPos];

                                } // end if PpsCount status < sample

                        
                        // Something unexpected happened.  We should never reach this 
                        // else since the equality case was tested first.
                        else { 
                                sprintf( SrUsbXchLastErrorString, "TsCombine Trouble with alignment\n" );
                                if ( Error )  *Error = USBXCH_ERROR_TSCOMBINE_TS_BAD_ALIGN;
                                return( 0 );
                                }

                        } // else TsSample->PpsCount != TsStatus->PpsCount

                } // end while TsSampleCount > 0 && TsStatusCount > 0
        
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperTsProcessValid
// PURPOSE: Check if newly updated timestamp is valid.  If so, return 1.
//------------------------------------------------------------------------------
int SrUsbXchHelperTsProcessValid( int *Error ) {

        TS  *TsCurrent;

        // Quick exit if there are no timestamps available

        if ( AcqRun.TsTotalCount <= 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_TS_VALID_OUT_OF_RANGE;
                sprintf( SrUsbXchLastErrorString,
                         "TimestampProcessValid no valid TS\n" );
                return( 0 );
                }


        // Get next timestamp
        
        TsCurrent = &AcqRun.TsTotal[AcqRun.TsTotalReadPos];
        
        AcqRun.TsTotalCount--;
        AcqRun.TsTotalReadPos++;
        if ( AcqRun.TsTotalReadPos >= MAX_ACQRUN_TS )
                AcqRun.TsTotalReadPos -= MAX_ACQRUN_TS;

        // Save first valid TS in TsRight

        if ( AcqRun.nTsValid == 0 ) {
                AcqRun.TsRight  = *TsCurrent;
                AcqRun.nTsValid = 1;
                }

        // Shuffle existing valid TS into TsLeft and
        // save new valid TS in TsRight

        else { // nTsValid > 0 ( == 1 or 2 )
                AcqRun.TsLeft   = AcqRun.TsRight;
                AcqRun.TsRight  = *TsCurrent;
                AcqRun.nTsValid = 2;
                }

        
        // Clear the current TS now that it's been saved

        SrDatTimestampInit( TsCurrent );


        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperProcessPackets
// PURPOSE: Using timestamp info, generate columns of data from packet array
//------------------------------------------------------------------------------
int SrUsbXchHelperProcessPackets( int Bounded, int *Error ) {

        int                 i, j, nf, ErrorCode, RoundedR;
        int                 SaveSaCount, SaveSaReadPos;
        unsigned long       nFill;
        double              SampleTime;
        SRDAT_USBXCH_ANALOG AnalogValues[2];
        SRUSBXCH_PACKET    *pPacket;

        // Since there are 2 points per packet, always work with even numbers
        
        RoundedR = 2 * ((AcqRun.TsRight.Sample+1)/2); // round up

        if ( Bounded  &&  (AcqRun.CurrentPoint > AcqRun.TsRight.Sample) ) {
                sprintf( SrUsbXchLastErrorString, "Exceeded timestamp range\n" );
                if ( Error )  *Error = USBXCH_ERROR_PROCESS_PKT_OUT_OF_RANGE;
                return( 0 );
                }


        // Provide timestamp data, if requested

        if ( AcqRun.TsData ) {
                AcqRun.TsData[0] = AcqRun.TsLeft;
                AcqRun.TsData[1] = AcqRun.TsRight;
                AcqRun.TsCount   = 2;
                }


        // Prepare status data - we always send back 2 elements, one for
        // the left and right bounding timestamps

        // Fill left status data

        if ( AcqRun.TsLeft.PpsCount != AcqRun.StatusArray[AcqRun.SaReadPos].PpsCount ) {
                sprintf( SrUsbXchLastErrorString, "**** TsLeft point = %d (pps %d), StatusArray[%d] = %d (pps %d)\n",
                       AcqRun.TsLeft.Sample,
                       AcqRun.TsLeft.PpsCount,
                       AcqRun.SaReadPos, 
                       AcqRun.StatusArray[AcqRun.SaReadPos].CurrentPt, 
                       AcqRun.StatusArray[AcqRun.SaReadPos].PpsCount 
                      );
//WCT - is this debugging check still needed
                }

        // Save some values in case we get an error and have to restore them
        
        SaveSaCount   = AcqRun.SaCount;
        SaveSaReadPos = AcqRun.SaReadPos;
        
        AcqRun.StatusData[0] = AcqRun.StatusArray[AcqRun.SaReadPos];
        AcqRun.StatusData[0].CurrentPt = AcqRun.TsLeft.Sample;
        AcqRun.SaCount--;
        AcqRun.SaReadPos++;
        if ( AcqRun.SaReadPos >= MAX_ACQRUN_STATUS_ARRAY )
                AcqRun.SaReadPos -= MAX_ACQRUN_STATUS_ARRAY;


        // Fill right status data - but don't inc SaReadPos yet since
        // this value will become the left status data next time

        if ( AcqRun.TsRight.PpsCount != AcqRun.StatusArray[AcqRun.SaReadPos].PpsCount ) {
                sprintf( SrUsbXchLastErrorString, "**** TsRight point = %d (pps %d), StatusArray[%d] = %d (pps %d)\n",
                         AcqRun.TsRight.Sample,
                         AcqRun.TsRight.PpsCount,
                         AcqRun.SaReadPos, 
                         AcqRun.StatusArray[AcqRun.SaReadPos].CurrentPt,
                         AcqRun.StatusArray[AcqRun.SaReadPos].PpsCount
                       );
//WCT - is this debugging check still needed
                }

        AcqRun.StatusData[1] = AcqRun.StatusArray[AcqRun.SaReadPos];
        AcqRun.StatusData[1].CurrentPt = AcqRun.TsRight.Sample;

        AcqRun.StatusCount = 2;


        // Prepared sample data from sample packets and timestamp info

        while ( AcqRun.nSamplePackets > 0 ) {

                // Get next packet

                pPacket = &AcqRun.SamplePacketArray[AcqRun.PaSampleReadPos];

                if ( (AcqRun.CurrentPoint < RoundedR)  ||  !Bounded ) {
                                
                        // Early exit if there is no more room for sample data

                        if ( AcqRun.SampleCount+1 >= AcqRun.SampleMaxCount ) {

                                // We must restore some values so we can re-use this status
                                // info again during recovery from this error

                                AcqRun.SaCount     = SaveSaCount;
                                AcqRun.SaReadPos   = SaveSaReadPos;
                                AcqRun.StatusCount = 0;

                                sprintf( SrUsbXchLastErrorString, "SampleData array is full\n" );
                                if ( Error )  *Error = USBXCH_ERROR_PROCESS_PKT_ARRAY_FULL;
                                return( 0 );
                                }


                        // Convert data from packet structure to analog structure

                        nFill = SrDatUsbAnalogFill( pPacket,
                                                    0, // don't check sanity
                                                    AnalogValues,
                                                    &ErrorCode );

//WCT check errorcode and nFill == 2
                        nf = (int)nFill;

                        // Compute time and save with sample data

                        for ( j = 0 ; j < nf ; j++ ) {
                                if ( AcqRun.GpsModel == SRDAT_GPSMODEL_TCXO )
                                        SrUsbXchHelperTcxoComputeTime( AcqRun.StartTime,
                                                AcqRun.SamplePeriod,
                                                AcqRun.CurrentPoint,
                                                &SampleTime );
                                else
                                        SrUsbXchHelperTsComputeTime( &AcqRun.TsLeft,
                                                &AcqRun.TsRight,
                                                AcqRun.CurrentPoint,
                                                &SampleTime );

                                i = AcqRun.SampleCount;

                                AcqRun.SampleData[i].CurrentPt    = AcqRun.CurrentPoint;
                                AcqRun.SampleData[i].Channel[0]   = AnalogValues[j].AnalogData[0];
                                AcqRun.SampleData[i].Channel[1]   = AnalogValues[j].AnalogData[1];
                                AcqRun.SampleData[i].Channel[2]   = AnalogValues[j].AnalogData[2];
                                AcqRun.SampleData[i].Channel[3]   = AnalogValues[j].AnalogData[3];
                                AcqRun.SampleData[i].DigitalIn    = AnalogValues[j].DigitalIn;
                                AcqRun.SampleData[i].Sanity       = AnalogValues[j].Sanity;
                                AcqRun.SampleData[i].PpsToggle    = AnalogValues[j].PpsToggle;
                                AcqRun.SampleData[i].GGACount     = AnalogValues[j].GGACount;
                                AcqRun.SampleData[i].OnBoardCount = AnalogValues[j].OnBoardCount;
                                AcqRun.SampleData[i].SampleTime   = SampleTime;

                                AcqRun.CurrentPoint++;
                                AcqRun.SampleCount++;

                                } // end for j

                        AcqRun.nSamplePackets--;

                        } // end if CurrentPoint < RoundedR || !Bounded

                else { // we're in bounded case and have exceeded bounds
                        break;
                        }


                // Update to indicate packet has been used
                // This MUST be done here and not at the top of the loop
                // since we don't know the packet is really used until
                // we've determined it is not a sample packet exceeding
                // the bounds.

                AcqRun.PaSampleCount--;
                AcqRun.PaSampleReadPos++;
                if ( AcqRun.PaSampleReadPos >= MAX_ACQRUN_SAMPLE_PACKETS )
                        AcqRun.PaSampleReadPos -= MAX_ACQRUN_SAMPLE_PACKETS;
                
                } // end while nSamplePackets > 0

        if ( Error )  *Error = USBXCH_ERROR_NONE;
        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperIsValidOBC
// PURPOSE: Check the specified analog data value to see if the OnBoardCount
//          is valid.  If so, it returns a 1, otherwise 0.
//          Typically, OBC is valid if it is non-zero.
//------------------------------------------------------------------------------
int SrUsbXchHelperIsValidOBC( int PpsToggle, int *Obc, int PpsCount ) {

        // Ignore very first point

        if ( AcqRun.nPoints == 0 ) {
                return( 0 );
                }


        // Look for PpsToggle changed

        else if ( AcqRun.PpsToggleLast != PpsToggle ) {

                // Correct OBC if it occurred 1 sample back

                if ( *Obc == 0 ) {
                        *Obc = AcqRun.ObcLast + AcqRun.ObcCorrect;
                        }

                return( 1 );
                }


        // Otherwise OBC is not valid

        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperPpsCountCompare
// PURPOSE: Compare new PpsCount from current packet with existing PpsCount in TS.
//          Return 0 if counts are equal, -1 or -2 if new is less than existing, 
//          and 1 or 2 if new count is more than existing.  If the counts are
//          close to each other It should be trivial, 
//          but PpsCounts wrap around to 0 after they reach 15.  So, a new count of
//          0 will be considered more than an existing count of 15.  If the
//          two counts differ by more than 4, we don't really know how to 
//          interpret things properly, so we fall back to treating them like
//          regular numbers.  Returns 3 if existing is unset (-1) and -3 if
//          new is unset (-1).
//------------------------------------------------------------------------------
int SrUsbXchHelperPpsCountCompare( int ExistingPpsCount, int NewPpsCount ) {
#define DIFFLIMIT        4
#define HALFSHIFT        8
#define FULLSHIFT       16

        int Diff, DiffPlain, DiffShift;
        int ExistingShift, NewShift;

        // Deal with the easy case where they are identical

        if ( ExistingPpsCount == NewPpsCount ) {
                return( 0 );
                }

        // Check for counts being unset

        if ( ExistingPpsCount == -1 )
                return( 3 );

        if ( NewPpsCount == -1 )
                return( -3 );


        // Shift halfway around

        ExistingShift = ExistingPpsCount + HALFSHIFT;
        if ( ExistingShift >= FULLSHIFT )
                ExistingShift -= FULLSHIFT;

        NewShift = NewPpsCount + HALFSHIFT;
        if ( NewShift >= FULLSHIFT )
                NewShift -= FULLSHIFT;



        // Compute difference between counts plain and shifted

        DiffPlain = ExistingPpsCount - NewPpsCount;
        if ( DiffPlain < 0 )
                DiffPlain = -DiffPlain;

        DiffShift = ExistingShift - NewShift;
        if ( DiffShift < 0 )
                DiffShift = -DiffShift;


        // Work with smaller difference

        if ( DiffShift < DiffPlain )
                Diff = DiffShift;
        else
                Diff = DiffPlain;


        // Difference is large, so treat counts simply

        if ( Diff > DIFFLIMIT ) {

                if ( ExistingPpsCount > NewPpsCount ) {
                        return( -2 );
                        }
                else {
                        return( 2 );
                        }
                }


        // Else compensate for wrap

        else {
                if ( DiffShift < DiffPlain ) {
                        if ( ExistingShift > NewShift ) {
                                return( -1 );
                                }
                        else {
                                return( 1 );
                                }
                        }

                else { // DiffShift >= DiffPlain
                        if ( ExistingPpsCount > NewPpsCount ) {
                                return( -1 );
                                }
                        else {
                                return( 1 );
                                }
                        }
                }
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperAcqRunInit
// PURPOSE: Initialize all acquisition run parameters.
//------------------------------------------------------------------------------
int SrUsbXchHelperAcqRunInit( void ) {

        int    i, Year, Month, Day, Hour, Minute, Second;
        long   MicroSecond;
        double SecSince1970;

        // AcqRun.SamplePeriod set in SrUsbXchOpen
        // AcqRun.GpsModel     set in SrUsbXchOpen


        // Initialize all run parameters

        AcqRun.SamplesPerSecond = (int)( 1.0L / AcqRun.SamplePeriod );
        AcqRun.ObcCorrect       = (int)(USBXCH_OBC_FREQ * AcqRun.SamplePeriod );

        AcqRun.nSamplePackets    = 0; // Packets identified by peek as sample type
        AcqRun.nStatusPackets    = 0; // Packets identified by peek as status type
        
        AcqRun.PaSampleWritePos  = 0;
        AcqRun.PaSampleReadPos   = 0;
        AcqRun.PaSamplePeekPos   = 0;
        AcqRun.PaSampleCount     = 0;
        
        AcqRun.PaStatusWritePos  = 0;
        AcqRun.PaStatusReadPos   = 0;
        AcqRun.PaStatusCount     = 0;
        
        AcqRun.SaWritePos        = 0;
        AcqRun.SaReadPos         = 0;
        AcqRun.SaCount           = 0;

        AcqRun.TsSampleWritePos  = 0;
        AcqRun.TsSampleReadPos   = 0;
        AcqRun.TsSampleCount     = 0;

        AcqRun.TsStatusWritePos  = 0;
        AcqRun.TsStatusReadPos   = 0;
        AcqRun.TsStatusCount     = 0;

        AcqRun.TsTotalWritePos   = 0;
        AcqRun.TsTotalReadPos    = 0;
        AcqRun.TsTotalCount      = 0;
        
        AcqRun.nTsValid          = 0; // Number of valid TS available, 2 required

        AcqRun.CurrentPoint      = 0; // Index of data point since start of run

        AcqRun.nPoints           = 0;
        AcqRun.nObc              = 0;
        AcqRun.PpsEventLast      = 0;
        AcqRun.PpsToggleLast     = 0;
        AcqRun.ObcLast           = 0;
        AcqRun.ObcPpsCountLast   = 0;
                        
        AcqRun.nNmea             = 0;
        AcqRun.nSecondLast       = -1;

        AcqRun.SampleData        = NULL;
        AcqRun.SampleMaxCount    = 0;
        AcqRun.SampleCount       = 0;

        AcqRun.StatusData        = NULL;
        AcqRun.StatusMaxCount    = 0;
        AcqRun.StatusCount       = 0;


//WCT - consider hardwiring or allocating based on requested sampling rate
        
        AcqRun.SamplePacketArray = (SRUSBXCH_PACKET*)malloc(sizeof(SRUSBXCH_PACKET)*MAX_ACQRUN_SAMPLE_PACKETS);
        if ( !AcqRun.SamplePacketArray ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchAcqRunInit could not allocate SamplePacketArray\n" );
                return( 0 );
                }


        AcqRun.StatusPacketArray = (SRUSBXCH_PACKET*)malloc(sizeof(SRUSBXCH_PACKET)*MAX_ACQRUN_STATUS_PACKETS);
        if ( !AcqRun.StatusPacketArray ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchAcqRunInit could not allocate StatusPacketArray\n" );
                return( 0 );
                }


        AcqRun.StatusArray = (SRUSBXCH_STATUSDATA*)malloc(sizeof(SRUSBXCH_STATUSDATA)*MAX_ACQRUN_STATUS_ARRAY);
        if ( !AcqRun.StatusArray ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchAcqRunInit could not allocate StatusArray\n" );
                return( 0 );
                }


        for ( i = 0 ; i < MAX_ACQRUN_TS ; i++ ) {
                SrDatTimestampInit( &AcqRun.TsSample[i] );
                SrDatTimestampInit( &AcqRun.TsStatus[i] );
                SrDatTimestampInit( &AcqRun.TsTotal[i] );
                }

        SrDatTimestampInit( &AcqRun.TsLeft );
        SrDatTimestampInit( &AcqRun.TsRight );

        
        for ( i = 0 ; i < SRDAT_NMEA_MAX_BUFF ; i++ )
                AcqRun.NmeaGroup[i] = '\0';



        // Set approximate start time

        if ( SrUsbXchGetPcTime( &Year, &Month, &Day, 
                                &Hour, &Minute, &Second, &MicroSecond ) &&
             SrDatSecTimeCombine( Year, Month, Day,
                                  Hour, Minute, Second, 
                                  MicroSecond, &SecSince1970 ) )
                AcqRun.StartTime = SecSince1970;
        else
                AcqRun.StartTime = 0.0L;
        

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperAcqRunFree
// PURPOSE: Clean up anything done when initializing acquisition run parameters.
//------------------------------------------------------------------------------
int SrUsbXchHelperAcqRunFree( void ) {

        AcqRun.SampleData        = NULL;
        AcqRun.SampleMaxCount    = 0;
        AcqRun.SampleCount       = 0;

        AcqRun.StatusData        = NULL;
        AcqRun.StatusMaxCount    = 0;
        AcqRun.StatusCount       = 0;


        free( AcqRun.SamplePacketArray );       AcqRun.SamplePacketArray = NULL;
        free( AcqRun.StatusPacketArray );       AcqRun.StatusPacketArray = NULL;
        free( AcqRun.StatusArray );             AcqRun.StatusArray       = NULL;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperExtractNmeaInfo
// PURPOSE: To extract individual parts from the NMEA strings such as year,
//          month, day, number of satellites, etc.
//------------------------------------------------------------------------------
int SrUsbXchHelperExtractNmeaInfo( char *NmeaGroup, int NmeaCount, NI *NmeaInfo ) {

        int Ok;

        if ( !NmeaGroup || NmeaCount < 0 || !NmeaInfo ) {
                sprintf( SrUsbXchLastErrorString,
                         "ExtractNmeaInfo had invalid inputs\n" );
                return( 0 );
                }


        // Get the best NMEA info possible from the provided group of NMEA strings

        Ok = SrDatRequestNmeaInfo2( NmeaGroup, NmeaCount,
                                    &NmeaInfo->Year,
                                    &NmeaInfo->Month,
                                    &NmeaInfo->Day,
                                    &NmeaInfo->YmdSource,
                                    &NmeaInfo->YmdIsValid,
                                    &NmeaInfo->Hour,
                                    &NmeaInfo->Minute,
                                    &NmeaInfo->Second,
                                    &NmeaInfo->MicroSecond,
                                    &NmeaInfo->HmsSource,
                                    &NmeaInfo->HmsIsValid,
                                    &NmeaInfo->Latitude,
                                    &NmeaInfo->Longitude,
                                    &NmeaInfo->LocSource,
                                    &NmeaInfo->LocIsValid,
                                    &NmeaInfo->Altitude,
                                    &NmeaInfo->PosSource,
                                    &NmeaInfo->PosIsValid,
                                    &NmeaInfo->Nsat,
                                    &NmeaInfo->SatSource,
                                    &NmeaInfo->SatIsValid
                                  );

        // If all went well, output the data to the specified file
        
        if ( Ok ) {
                Ok = SrDatSecTimeCombine( NmeaInfo->Year, NmeaInfo->Month, NmeaInfo->Day,
                                          NmeaInfo->Hour, NmeaInfo->Minute, NmeaInfo->Second, 
                                          NmeaInfo->MicroSecond, &NmeaInfo->SecSince1970 );

                NmeaInfo->SecIsValid = Ok;
                
                if ( Ok && NmeaInfo->YmdIsValid && NmeaInfo->HmsIsValid ) {

                        return( 1 );
                        }
                else {
                        sprintf( SrUsbXchLastErrorString, "Time/date not valid or output... %04d/%02d/%02d %02d:%02d:%02d.%06ld \n",
                                         NmeaInfo->Year, NmeaInfo->Month, NmeaInfo->Day,
                                         NmeaInfo->Hour, NmeaInfo->Minute, NmeaInfo->Second,
                                         NmeaInfo->MicroSecond
                                         );
                        }
                }
        else
                sprintf( SrUsbXchLastErrorString, "SrDatRequestNmeaInfo2 failed\n" );

        return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperPktCheckSanity
// PURPOSE: Check the packet sanity to make sure it is the expected value.
//------------------------------------------------------------------------------
int SrUsbXchHelperPktCheckSanity( SRUSBXCH_PACKET *Packet, int nTotalPackets ) {

        int Error;
        int ptype;
        int sanity;


#define STARTUP  -1

        static int ExpectedSampleSanity = STARTUP;
        static int ExpectedStatusSanity = STARTUP;

#define Increment( s )     s = ( (s+1) & SRDAT_USBPACKET_SANITY_MASK )




        // Initialize the error flag.

        Error = 0;


        // Check the sanity for the respective packet type.
        //
        // Note (analog) sample packets have two samples per 64 byte packet.
        // Each of the two samples has its own sanity.
        //
        //

         ptype = SrUsbXchPacketType( Packet );
        sanity = SrUsbXchPacketSanity( Packet );


        if ( ptype == SRDAT_USBPACKET_TYPE_SAMPLE ) { // (analog) sample packet


                // If starting up, init the expected sanity.

                if ( ExpectedSampleSanity == STARTUP )

                        ExpectedSampleSanity = sanity;


                // Check the actual sanity against the expected.

                if ( sanity == ExpectedSampleSanity ) {

                        Increment( ExpectedSampleSanity );


                        // check the second packet sub sample too ...

                        if ( SrUsbXchPacketSanity2( Packet) == ExpectedSampleSanity )

                                Increment( ExpectedSampleSanity );

                        else
                                Error = 2;

                        }

                else
                        Error = 1;

                }


        
        else { // ptype == SRDAT_USBPACKET_TYPE_STATUS, (GPS NMEA) status packet


                // If starting up, init the expected sanity.

                if ( ExpectedStatusSanity == STARTUP )

                        ExpectedStatusSanity = sanity;


                // Check the actual sanity against the expected value.

                if ( sanity == ExpectedStatusSanity  )

                        Increment( ExpectedStatusSanity );

                else
                        Error = 3;
                
                }
        


        // Fatal error if any actual sanity did not match expected.

        if ( Error ) {

            sprintf( SrUsbXchLastErrorString, "sanity error %d for packet %d\n", Error, nTotalPackets );
            
            if ( Error == 3 ) {
                    sprintf( SrUsbXchLastErrorString, "found sanity %d, expected sanity %d", sanity, ExpectedStatusSanity );
                    }
            else {
                    sprintf( SrUsbXchLastErrorString, "found sanity %d, expected sanity %d", sanity, ExpectedSampleSanity );
                    }

                return( 0 );

                }

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperTsComputeTime
// PURPOSE: Compute the time of the specified sample based on the information
//          contained in the timestamp structures.
//------------------------------------------------------------------------------
int SrUsbXchHelperTsComputeTime( TS *TS0, TS *TS1, int CurrentPt, double *SampleTime ) {

        int    Ok;
        double x1, x2, y1, y2, xk, yk;

        if ( !TS0 || !TS1 ) {
//ERR                if ( Error )  *Error = USBXCH_ERROR_22;
                sprintf( SrUsbXchLastErrorString,
                         "ComputeTime had invalid inputs\n" );
                return( 0 );
                }

        x1 = (double)TS0->Sample;
        x2 = (double)TS1->Sample;
        y1 = TS0->TotalTime;
        y2 = TS1->TotalTime;
        xk = (double)CurrentPt;

        Ok = SrDatEquation2pt( x1, y1, x2, y2, xk, &yk );

        if ( SampleTime )  *SampleTime = yk;

        return( Ok );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperTcxoComputeTime
// PURPOSE: Compute the time of the specified sample based on the starting
//          time, sample period, and sample number.
//------------------------------------------------------------------------------
int SrUsbXchHelperTcxoComputeTime( double StartTime, double SamplePeriod,
                                   int CurrentPt, double *SampleTime ) {

        if ( SampleTime ) *SampleTime = StartTime + (SamplePeriod * (double)CurrentPt);

        return( 1 );
}


//******************************************************************************



//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSendStandardRequest
// PURPOSE: This function sends the requested standard command to the USBXCH's
//          offical EP0 command pipe.  The return indicates how many values
//          were actually written.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned long ) SrUsbXchSendStandardRequest( DEVHANDLE UsbXchHandle,
                                                     unsigned char Direction,
                                                     unsigned char Recipient,
                                                     unsigned char Request,
                                                     unsigned short Value,
                                                     unsigned short Index,
                                                     char *Values,
                                                     unsigned long Nvalues,
                                                     int *Error ) {

        SR_STANDARD_REQUEST_DATA RequestData;
        BOOL                     Ok;
        unsigned long            BytesReturned;

        // Quick exit - nothing to do

        if ( Nvalues == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }


        // Error check

        if ( !Values ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }

        if ( UsbXchHandle == BAD_DEVHANDLE ) {
                if ( Error )  *Error = USBXCH_ERROR_DRIVER_NOT_OPEN;
                return( 0 );
                }

//WCT - can we validate request, value, and index


        // Initialize variables

        BytesReturned           = 0;
        RequestData.Direction   = Direction;
        RequestData.Recipient   = Recipient;
        RequestData.Request     = Request;
        RequestData.Value       = Value;
        RequestData.Index       = Index;
        RequestData.Length      = Nvalues;


        Ok = SrUsbXchOsDriverIoctl( UsbXchHandle,
                                  IOCTL_USBXCH_STANDARD_REQUEST,
                                  &RequestData,                     // *Control/Input buffer
                                  sizeof(SR_STANDARD_REQUEST_DATA), // its size
                                  Values,                           // *Data/Output buffer
                                  Nvalues,                          // its size
                                  &BytesReturned
                                );


        if ( Ok ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                }
        else {
                if ( Error )  *Error = USBXCH_ERROR_DRIVER_REQUEST_FAILED;
                BytesReturned = 0;
                }
        return( BytesReturned );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSendVendorRequest
// PURPOSE: This function sends the requested vendor command to the USBXCH's
//          offical EP0 command pipe.  The return indicates how many values
//          were actually written.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned long ) SrUsbXchSendVendorRequest( DEVHANDLE UsbXchHandle,
                                                   unsigned char Request,
                                                   unsigned short Value,
                                                   unsigned short Index,
                                                   char *Values,
                                                   unsigned long Nvalues,
                                                   int *Error ) {

        SR_VENDOR_REQUEST_DATA RequestData;
        BOOL                   Ok;
        unsigned long          BytesReturned;

        // Quick exit - nothing to do

        if ( Nvalues == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }


        // Error check

        if ( !Values ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }

        if ( UsbXchHandle == BAD_DEVHANDLE ) {
                if ( Error )  *Error = USBXCH_ERROR_DRIVER_NOT_OPEN;
                return( 0 );
                }

//WCT - can we validate request, value, and index



        // Initialize variables

        BytesReturned       = 0;
        RequestData.Request = Request;
        RequestData.Value   = Value;
        RequestData.Index   = Index;
        RequestData.Length  = Nvalues;


        Ok = SrUsbXchOsDriverIoctl( UsbXchHandle,
                                  IOCTL_USBXCH_VENDOR_REQUEST,
                                  &RequestData,                   // *Control/Input buffer
                                  sizeof(SR_VENDOR_REQUEST_DATA), // its size
                                  Values,                         // *Data/Output buffer
                                  Nvalues,                        // its size
                                  &BytesReturned
                                );


        if ( Ok ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                }
        else {
                if ( Error )  *Error = USBXCH_ERROR_DRIVER_REQUEST_FAILED;
                BytesReturned = 0;
                }

        return( BytesReturned );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchSendCommand
// PURPOSE: This function sends the requested data to the USBXCH by writing it
//          to the EP1 OUT "command" pipe.  The return indicates how many
//          values were actually written.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) SrUsbXchSendCommand( DEVHANDLE UsbXchHandle,
                                              char *Values,
                                              unsigned long Nvalues,
                                              int *Error
                                            ) {
        BOOL          Ok;
        unsigned long BytesSent;

        // Quick exit - nothing to do

        if ( Nvalues == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }

        // Error check

        if ( !Values ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }

        if ( UsbXchHandle == BAD_DEVHANDLE ) {
                if ( Error )  *Error = USBXCH_ERROR_DRIVER_NOT_OPEN;
                return( 0 );
                }


        // Initialize variables

        BytesSent = 0L;

        Ok = SrUsbXchOsDriverIoctl( UsbXchHandle,
                                    IOCTL_USBXCH_BULK_WRITE_CMD,
                                    Values,                       // *Control/Input buffer
                                    Nvalues,                      // its size
                                    NULL,                         // *Data/Output buffer
                                    0,                            // its size
                                    &BytesSent
                                  );


        if ( !Ok  ||  BytesSent != Nvalues ) {
                if ( Error )  *Error = USBXCH_ERROR_WRITE_FAILED;
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( 0 );
                }

        if ( Error )  *Error = USBXCH_ERROR_NONE;

        return( (unsigned int)BytesSent );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchReceiveResults
// PURPOSE: This function receives the requested data from the USBXCH by
//          reading it from the EP1 IN "command" pipe.  The return indicates
//          how many values were actually read.
//------------------------------------------------------------------------------
FUNCTYPE( unsigned int ) SrUsbXchReceiveResults( DEVHANDLE UsbXchHandle,
                                                 char *Values,
                                                 unsigned long Nvalues,
                                                 int *Error
                                               ) {
        BOOL          Ok;
        unsigned long BytesRcvd;

        // Quick exit - nothing to do

        if ( Nvalues == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }

        // Error check

        if ( !Values ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }

        if ( UsbXchHandle == BAD_DEVHANDLE ) {
                if ( Error )  *Error = USBXCH_ERROR_DRIVER_NOT_OPEN;
                return( 0 );
                }
        

        // Initialize variables

        BytesRcvd = 0;

        // Read data in from USBXCH

        Ok = SrUsbXchOsDriverIoctl( UsbXchHandle,
                                    IOCTL_USBXCH_BULK_READ_CMD,
                                    NULL,                         // *Control/Input buffer
                                    0,                            // its size
                                    Values,                       // *Data/Output buffer
                                    Nvalues,                      // its size
                                    &BytesRcvd
                                  );

        if ( !Ok ) {
                if ( Error )  *Error = USBXCH_ERROR_READ_RESULTS_FAILED;
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( BytesRcvd );
                }

        if ( BytesRcvd < Nvalues ) {
//WCT - decide on proper action if got less than asked for
                }

        if ( BytesRcvd > Nvalues ) {
                if ( Error )  *Error = USBXCH_ERROR_READ_RESULTS_FAILED;
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( BytesRcvd );
                }

        if ( Error )  *Error = USBXCH_ERROR_NONE;

        return( BytesRcvd );
}


//*********************** USBxCH ADVANCED OPEN FUNCTIONS **********************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchMinimalOpen
// PURPOSE: To open the driver for a USBxCH A/D board.  This ONLY opens the
//          driver but does nothing else.  This is useful when things are
//          going wrong because an open driver handle is all that is needed to
//          request the USB port be reset or power cycled.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) SrUsbXchMinimalOpen( char   *DriverName,
                                           int    *Error
                                          ) {

        DEVHANDLE UsbXchHandle;
        int       ErrorCode;


        // Initialize error code

        ErrorCode = USBXCH_ERROR_NONE;



        // Error check incoming parameter

        if ( !DriverName ) {
                ErrorCode = USBXCH_ERROR_OPEN_DRIVER_NAME;
                goto MinimalOpenDone;
                }


        // Open USBxCH driver

        UsbXchHandle = SrUsbXchOsDriverOpen( DriverName );
        if ( UsbXchHandle == BAD_DEVHANDLE ) {
                SrUsbXchOsGetLastError();
                ErrorCode = USBXCH_ERROR_DRIVER_NOT_OPEN;
                goto MinimalOpenDone;
                }


        // Finish and return

MinimalOpenDone:
        
        if ( Error )  *Error = ErrorCode;

        return( UsbXchHandle );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHardwareOpen
// PURPOSE: To open the driver for a USBxCH A/D board with additional options
//          not required by the normal user.
//------------------------------------------------------------------------------
FUNCTYPE( DEVHANDLE ) SrUsbXchHardwareOpen( char   *DriverName,
                                            char  **HwData,
                                            int    *Error
                                           ) {

        DEVHANDLE UsbXchHandle;
        int       ErrorCode, CloseOnError, Trys;

        // Initialize error variables

        ErrorCode    = USBXCH_ERROR_NONE;
        CloseOnError = 0;


        // Open USBxCH driver

        UsbXchHandle = SrUsbXchOsDriverOpen( DriverName );
        if ( UsbXchHandle == BAD_DEVHANDLE ) {
                SrUsbXchOsGetLastError();
                ErrorCode = USBXCH_ERROR_DRIVER_NOT_OPEN;
                goto HardwareOpenError;
                }

        CloseOnError = 1; // Now driver is open, so must close if error occurs



        // Initialize the A/D board ...
        //   Download USBxCH hardware data
        //   Start 8051 code so it can process LED and sample rate requests
        //   Verify that power is good (ie over 7.5 volts)


        if ( ! SrUsbXchHelperDownload( UsbXchHandle, HwData ) )
                goto HardwareOpenPortReset;

        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) )
                goto HardwareOpenPortReset;

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) )
                goto HardwareOpenPortReset;

        goto HardwareOpenSuccess;


        
HardwareOpenPortReset:

        // One of the above initialization functions failed, so
        // reset the USB Port on the PC.  
        
        if ( SrUsbXchPortReset( UsbXchHandle ) )
                ErrorCode = USBXCH_ERROR_OPEN_SUCCESS_PORT_RESET;
        else
                goto HardwareOpenPortCycle;


        // ... and try again ...
        
        if ( ! SrUsbXchHelperDownload( UsbXchHandle, HwData ) )
                goto HardwareOpenPortCycle;

        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) )
                goto HardwareOpenPortCycle;

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) )
                goto HardwareOpenPortCycle;

        goto HardwareOpenSuccess;



HardwareOpenPortCycle:
        
        // One of the above initialization functions failed again, so
        // cycle the power to the USB Port on the PC causing the Device
        // Manager to update.

        if ( SrUsbXchPortCycle( UsbXchHandle ) )
                ErrorCode = USBXCH_ERROR_OPEN_SUCCESS_PORT_CYCLE;
        else
                goto HardwareOpenTryBoardReset;


        // ... and try again ...
        //
        // (but add a loop on the first function since we have to wait a
        // bit for the Device Manager update to complete).
        
        Trys = 0;

        while ( ! SrUsbXchHelperDownload( UsbXchHandle, HwData ) ) {
                
                if ( Trys < MAX_PORT_CYCLE_TRIES )  // Wait longer
                        SrUsbXchOsSleep( 500 );     // half second
                
                else                                // Tried too long
                        goto HardwareOpenTryBoardReset;

                Trys++;
                }


        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) )
                goto HardwareOpenTryBoardReset;

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) )
                goto HardwareOpenTryBoardReset;

        goto HardwareOpenSuccess;




HardwareOpenTryBoardReset:
        
        // One of the above initialization functions failed yet again, so
        // cycle the power to the USBxCH board itself causing the driver
        // to reload and the Device Manager to update.
        
        UsbXchHandle = SrUsbXchPowerReset( UsbXchHandle ); // This closes driver


        // Re-open the driver, but do it in a loop since we have to wait a bit
        // for the driver reload and Device Manager update to complete.
        
        Trys         = 0;
        CloseOnError = 0;
        ErrorCode    = USBXCH_ERROR_OPEN_SUCCESS_BOARD_RESET;

        while ( 1 ) {

                UsbXchHandle = SrUsbXchOsDriverOpen( DriverName );
                if ( UsbXchHandle != BAD_DEVHANDLE )       // Handle is good
                        break;                             // so open is done

                if ( Trys < MAX_BOARD_RESET_TRIES )        // Wait longer
                        SrUsbXchOsSleep( 500 );            // half second

                else {                                     // Tried too long
                        ErrorCode = USBXCH_ERROR_OPEN_BOARD_RESET;
                        goto HardwareOpenError;
                        }

                Trys++;
                } // end while


        CloseOnError = 1; // Now driver is open, so must close if error occurs
        

        // ... and try again ...
        //
        // (but this is the last time, so set the error code to indicate
        // which initialization function still didn't work).
        
        if ( ! SrUsbXchHelperDownload( UsbXchHandle, HwData ) ) {
                ErrorCode = USBXCH_ERROR_OPEN_DOWNLOAD;
                goto HardwareOpenError;
                }

        if ( ! SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_RUN ) ) {
                ErrorCode = USBXCH_ERROR_OPEN_START_8051;
                goto HardwareOpenError;
                }

        if ( ! SrUsbXchIsPowerGood( UsbXchHandle ) ) {
                ErrorCode = USBXCH_ERROR_OPEN_POWER;
                goto HardwareOpenError;
                }
        

        // Finish and return

HardwareOpenSuccess:

        if ( Error )  *Error = ErrorCode;

        return( UsbXchHandle );


HardwareOpenError:

        if ( CloseOnError )
                SrUsbXchClose( UsbXchHandle );

        if ( Error )  *Error = ErrorCode;

        return( BAD_DEVHANDLE );
}


//**************************** USBxCH A/D FUNCTIONS ***************************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchAtodReset
// PURPOSE: This function resets the A/Ds on the USBxCH.  On reset, the A/D's
//          automatically do a self calibration.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchAtodReset( DEVHANDLE UsbXchHandle, int *Error ) {

        unsigned long BytesSent, Ncmd;
        int           Err;
        char          Values[1];


        // Set up command values

        Values[0] = USB8051_CMD_ATOD_RESET;
        Ncmd = 1;

        // Send to USBxCH

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

        if ( Err != USBXCH_ERROR_NONE ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchAtodReset failed with BytesSent %lu\n",
                         BytesSent );
                if ( Error ) *Error = USBXCH_ERROR_ATOD_RESET;
                return( 0 );
                }

        // Success
        
        if ( Error )  *Error = USBXCH_ERROR_NONE;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchAtodWriteReg
// PURPOSE: This function write to the registers of the ADS1255 on the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchAtodWriteReg( DEVHANDLE UsbXchHandle, int FirstReg,
                                    int Nreg, char *RegData, int *Error ) {

        unsigned long BytesSent, Ncmd;
        int           i, Err;
        char          Values[4+ADREG_MAX];


        // Quick exit if nothing to do

        if ( Nreg == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }


        // Error check incoming parameters

        if ( RegData == NULL ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }



        // Set up command values including data bytes

        Values[0] = USB8051_CMD_ATOD_SENDCMD;
        Values[1] = ADCMD_SIZE_RW + Nreg;       // Size of A/D command + data
        Values[2] = (ADCMD_WREG | FirstReg);    // Write starting at FirstReg
        Values[3] = Nreg - 1;                   // For nbytes, say nbytes-1
        Ncmd = 4;

        for ( i = 0 ; i < Nreg ; i++ )          // Fill in register data
                Values[Ncmd++] = RegData[i];




        // Send to USBxCH

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

        if ( Err != USBXCH_ERROR_NONE ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchAtodWriteReg failed with BytesSent %lu\n",
                         BytesSent );




                if ( Error ) *Error = Err;
                return( 0 );
                }

        
        if ( Error ) *Error = USBXCH_ERROR_NONE;

        return( 1 );
}



#ifndef _WIN64     // 'SrDatByteDemux()' with 'asm' won't compile as 64-bt

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchAtodReadReg
// PURPOSE: This function reads to the registers of the ADS1255 on the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchAtodReadReg( DEVHANDLE UsbXchHandle, int FirstReg,
                                    int Nreg, char *RegData, int *Error ) {

        unsigned long BytesSent, BytesRcvd, Ncmd, Nres;
        int           i, j, pos, StartReg, Err;
        int           Nremain, Ndone, Nr, Ndmx;
        char          Values[6], Results[ADREG_MAX*8];
        unsigned char DmxVal[4], *pArray;


        // Quick exit if nothing to do

        if ( Nreg == 0 ) {
                if ( Error )  *Error = USBXCH_ERROR_NONE;
                return( 0 );
                }


        // Error check incoming parameters

        if ( RegData == NULL || Nreg < 0 || Nreg > ADREG_MAX ) {
                if ( Error )  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
                }

        i        = 0;
        Ndone    = 0;
        Nremain  = Nreg;
        StartReg = FirstReg;

        while ( Nremain > 0 ) {

                Nr = Nremain;
                if ( Nr > 8 )
                        Nr = 8;


                // Set to write the read regiser command

                Values[0] = USB8051_CMD_ATOD_SENDCMD;
                Values[1] = ADCMD_SIZE_RW;            // Size of A/D command
                Values[2] = (ADCMD_RREG | StartReg);  // Read starting at FirstReg
                Values[3] = Nr - 1;                   // For nreg, say nreg-1
                Ncmd = 4;

                // Send write A/D command requesting read register to USBxCH

                BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

                if ( Err != USBXCH_ERROR_NONE ) {
                        sprintf( SrUsbXchLastErrorString,
                                 "SrUsbXchAtodReadReg failed with BytesSent %lu\n",
                                 BytesSent );
                        if ( Error )  *Error = Err;
                        return( 0 );
                        }




                // Set up A/D command to read the requested data

                Values[0] = USB8051_CMD_ATOD_READREG;
                Values[1] = (CHAR)Nr;                       // Num registers to read
                Values[2] = (CHAR)(Nr*8);                   // Num bytes to read
                Ncmd = 3;


                // Send to USBxCH

                BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

                if ( Err != USBXCH_ERROR_NONE ) {
                        if ( Error )  *Error = Err;
                        return( 0 );
                        }



                // Read results

                Nres = Nr * 8;
                BytesRcvd = SrUsbXchReceiveResults( UsbXchHandle, Results, Nres, &Err );

                if ( Err != USBXCH_ERROR_NONE ) {
                        if ( Error )  *Error = Err;
                        return( 0 );
                        }

                if ( BytesRcvd != Nres ) {
                        if ( Error )  *Error = USBXCH_ERROR_READ_NO_DATA;
                        return( 0 );
                        }


                // Process results

                pos    = 0;
                pArray = (unsigned char *)Results;

                for ( i = 0 ; i < Nr ; i++ ) {

                        Ndmx = SrDatByteDemux( &pArray, DmxVal ); // incs pArray by 8

                        for ( j = 0 ; j < Ndmx ; j++ )
                                RegData[(Ndone+i)*4+j] = DmxVal[j];

                        pos += 8;

                        } // end for i < Nr

                Ndone    = Ndone + Nr;
                StartReg = FirstReg + Ndone;
                Nremain  = Nreg - Ndone;

                } // end while Nremain > 0


        // Success
        
        if ( Error )   *Error = USBXCH_ERROR_NONE;

        return( 1 );
}

#endif


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchGetDready
// PURPOSE: Get the active low Dready signal value from USBxCH.  Returns 1
//          for success, 0 otherwise.  On success Dready argument is filled with
//          0 when signal is low, and 1 when signal is high.  On failure, Dready
//          argument is filled with -1.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchGetDready( DEVHANDLE UsbXchHandle,
                                   int *Dready, int *Error ) {

        unsigned long BytesSent, BytesRcvd, Ncmd, Nres;
        int           Err, Drdy;
        char          Values[1], Results[1];


        // Send command
        
        Values[0] = USB8051_CMD_GET_DREADY;
        Ncmd = 1;

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

        if ( Err != USBXCH_ERROR_NONE ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchGetDready failed with BytesSent %lu\n",
                         BytesSent );
                if ( Error )   *Error = Err;
                if ( Dready )  *Dready = -1;
                return( 0 );
                }


        // Request results

        Nres      = 1;
        BytesRcvd = SrUsbXchReceiveResults( UsbXchHandle, Results, Nres, &Err );

        if ( Err != USBXCH_ERROR_NONE ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchGetDready failed with BytesRcvd %lu\n",
                         BytesRcvd );
                if ( Error )   *Error = Err;
                if ( Dready )  *Dready = -1;
                return( 0 );
                }




        // Process results
        
        Drdy = (Results[0] & 0x00FF);

        if ( Error )   *Error = USBXCH_ERROR_NONE;
        if ( Dready )  *Dready = Drdy;

        return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchIsOpenSuccess
// PURPOSE: This function returns 1 if the provided error value is one of the
//          four possible values returned by SrUsbXchOpen which indicates a
//          successful open.  It returns 0 otherwise.  A simple check on
//          USBXCH_ERROR_NONE is not sufficient since Open reports differently
//          if it had to reset or cycle the port or reset the board before it 
//          could complete opening the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchIsOpenSuccess( int Error ) {

        if ( (Error == USBXCH_ERROR_NONE)                    ||
             (Error == USBXCH_ERROR_OPEN_SUCCESS_PORT_RESET) ||
             (Error == USBXCH_ERROR_OPEN_SUCCESS_PORT_CYCLE) ||
             (Error == USBXCH_ERROR_OPEN_SUCCESS_BOARD_RESET) )
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchAtodCalibrate
// PURPOSE: This function sends one of the 5 calibration command to the
//          registers of the ADS1255 on the USBxCH.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchAtodCalibrate( DEVHANDLE UsbXchHandle, int CalibrateType,
                                     int *Error ) {

        unsigned long BytesSent, Ncmd;
        int           Dready, Count, Err;
        char          Values[3];

        if ( CalibrateType != ADCMD_SELFCAL  &&
             CalibrateType != ADCMD_SELFOCAL &&
             CalibrateType != ADCMD_SELFGCAL &&
             CalibrateType != ADCMD_SYSOCAL  &&
             CalibrateType != ADCMD_SYSGCAL  ) {
                if (Error)  *Error = USBXCH_ERROR_INVALID_PARAM;
                return( 0 );
        }


        // Set up command values

        Values[0] = USB8051_CMD_ATOD_SENDCMD;
        Values[1] = ADCMD_SIZE_STD;              // Size of A/D command
        Values[2] = (CHAR)CalibrateType;         // Request calibration
        Ncmd = 3;


        // Send to USBxCH

        BytesSent = SrUsbXchSendCommand( UsbXchHandle, Values, Ncmd, &Err );

        if ( Err != USBXCH_ERROR_NONE ) {
                sprintf( SrUsbXchLastErrorString,
                         "SrUsbXchAtodCalibrate failed with BytesSent %lu\n",
                         BytesSent );
                if ( Error )  *Error = Err;
                return( 0 );
                }


        // Wait for calibration to finish as signaled by Dready going low

        Count  = 0;
        Dready = 1;

        while ( Dready == 1 ) {
                
                SrUsbXchGetDready( UsbXchHandle, &Dready, &Err );

                if ( Err != USBXCH_ERROR_NONE ) {       // failed call
                        if ( Error )  *Error = Err;
                        return( 0 );
                        }
                
                Count++;
		if ( Count > 2000 ) {                   // taking too long
                        if ( Error )  *Error = USBXCH_ERROR_OPEN_SAMPLE_RATE;
                        return( 0 );
			}

//WCT - consider adding a timeout function or keypress escape from loop
//                if (_kbhit())
//                        break;
//		if ( SrGetKeyCheck() != EOF )
//			break;
                }


        // Success

        if ( Error )   *Error = USBXCH_ERROR_NONE;

        return( 1 );
}




//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPipeStatus
// PURPOSE: This function asks for the stall status on the specified USB pipe.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPipeStatus( DEVHANDLE UsbXchHandle, int PipeNum ) {

        BOOL          Ret;
        unsigned long BytesSent;

//WCT - Consider using pipe type and direction instead of pipe num

        if ( PipeNum < PIPE_MIN || PipeNum > PIPE_MAX )
                return( 0 );

        Ret = SrUsbXchOsDriverIoctl( UsbXchHandle,
                IOCTL_USBXCH_RESET_PIPE,
                &PipeNum,
                sizeof(int),
                NULL,
                0,
                &BytesSent );


        if (Ret)
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPipeReset
// PURPOSE: This function resets the specified USB pipe.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPipeReset( DEVHANDLE UsbXchHandle, int PipeNum ) {

        BOOL          Ret;
        unsigned long BytesSent;

//WCT - Consider using pipe type and direction instead of pipe num

        if ( PipeNum < PIPE_MIN || PipeNum > PIPE_MAX )
                return( 0 );

        Ret = SrUsbXchOsDriverIoctl( UsbXchHandle,
                IOCTL_USBXCH_RESET_PIPE,
                &PipeNum,
                sizeof(int),
                NULL,
                0,
                &BytesSent );

//WCT - Review possible returns
        if ( Ret )
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPipeAbort
// PURPOSE: This function aborts the specified USB pipe.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPipeAbort( DEVHANDLE UsbXchHandle, int PipeNum ) {

        BOOL Ret;
        unsigned long BytesSent;

//WCT - Consider using pipe type and direction instead of pipe num

        if ( PipeNum < PIPE_MIN || PipeNum > PIPE_MAX )
                return( 0 );

        Ret = SrUsbXchOsDriverIoctl( UsbXchHandle,
                IOCTL_USBXCH_ABORT_PIPE,
                &PipeNum,
                sizeof(int),
                NULL,
                0,
                &BytesSent );

//WCT - Review possible returns
        if ( Ret )
                return( 1 );
        else
                return( 0 );
}



//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPortReset
// PURPOSE: This function resets the USB port connected to this device.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPortReset( DEVHANDLE UsbXchHandle ) {

        BOOL          Ret;
        unsigned long BytesSent;

        Ret = SrUsbXchOsDriverIoctl( UsbXchHandle,
                IOCTL_USBXCH_RESET_PORT,
                NULL,
                0,
                NULL,
                0,
                &BytesSent );

//WCT - Review possible returns
        if ( Ret )
                return( 1 );
        else
                return( 0 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchPortCycle
// PURPOSE: This function cycles the USB port connected to this device.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchPortCycle( DEVHANDLE UsbXchHandle ) {

        BOOL          Ret;
        unsigned long BytesSent;

        Ret = SrUsbXchOsDriverIoctl( UsbXchHandle,
                IOCTL_USBXCH_CYCLE_PORT,
                NULL,
                0,
                NULL,
                0,
                &BytesSent );

//WCT - Review possible returns
        if ( Ret )
                return( 1 );
        else
                return( 0 );
}


//************************** HELPER FUNCTIONS ********************************

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchClearData
// PURPOSE: This function initialize to zero all elements of the SRDAT_USBXCH_ANALOG
//          structure for the specified number of samples.  Returns 1 on
//          success, 0 otherwise
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrUsbXchClearData( SRDAT_USBXCH_ANALOG *Data, int Nsamples ) {

        int i, Ok;

        if ( !Data  ||  Nsamples < 0 )
                return( 0 );

        Ok = 1;
        for ( i = 0 ; i < Nsamples ; i++ ) {
                Ok = SrDatUsbAnalogInit( &Data[i] );
                if ( !Ok )
                        break;
                }

        return( Ok );
}

//------------------------------------------------------------------------------
// ROUTINE: SrGetIntFromHexString
// PURPOSE: This helper function reads an integer from the provided hex code
//          string.
//------------------------------------------------------------------------------
FUNCTYPE( int ) SrGetIntFromHexString( char *HexString, int StartChar, int NumChar,
                                       int *Result ) {

        int i, c, Value;

        // Set default result

        if (Result)  *Result = 0;


        // Error check

        if ( !HexString || (strlen( HexString ) < (unsigned int)(StartChar+NumChar)) )
                return( 0 );


        // Process hex digits building up result value

        Value = 0;
        for ( i = 0 ; i < NumChar ; i++ ) {
                c = HexString[StartChar+i];

                if ( c >= '0' && c <= '9' )
                        Value = Value*16 + (c-'0');

                else if ( c >= 'A' && c <= 'F' )
                        Value = Value*16 + 10 + (c-'A');

                else if ( c >= 'a' && c <= 'f' )
                        Value = Value*16 + 10 + (c-'a');

                else
                        return( 0 );
                }

        if (Result)  *Result = Value;
        return( 1 );
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelperDownload
// PURPOSE: This function downloads the encoded hardware data required for 
//          the USBxCH to perform properly.
//------------------------------------------------------------------------------
// Defines for working with Intel Hex format files

#define HEX_MAXSTR     256   // Maximum length of Intel Hex file string
#define HEX_CNTFIELD     1   // Offsets of fields within hex record
#define HEX_ADDRFIELD    3
#define HEX_RECFIELD     7
#define HEX_DATAFIELD    9

#define HEX_CNTSIZE   (HEX_ADDRFIELD-HEX_CNTFIELD)
#define HEX_ADDRSIZE  (HEX_RECFIELD-HEX_ADDRFIELD)
#define HEX_RECSIZE   (HEX_DATAFIELD-HEX_RECFIELD)

int SrUsbXchHelperDownload( DEVHANDLE UsbXchHandle, char **HwData ) {

        unsigned int Nbytes;
        int          i, ihex, RecType, Cnt, Addr, Pos, Value, Ok, Error;
        char         HexStr[HEX_MAXSTR], DataString[HEX_MAXSTR];



        // Put chip in hold mode

        Ok = SrUsbXchHelper8051RunHold( UsbXchHandle, USB8051_CPU_HOLD );
        if (!Ok) {
                return( 0 );
                }


        // Cycle through reading hex data and sending one line at a time

        ihex  = 0;

        while ( HwData[ihex] != NULL &&
                strncpy( HexStr, HwData[ihex], HEX_MAXSTR ) ) {


                if ( HexStr[0] != ':' ) {
                        sprintf(SrUsbXchLastErrorString, "No : at start of string |%s|\n", HexStr );
//WCT Error = USBXCH_ERROR_NOCOLON
                        return( 0 );
                        }

                Ok = SrGetIntFromHexString( HexStr, HEX_CNTFIELD,  HEX_CNTSIZE,  &Cnt );
                if (!Ok) return( 0 );
                Ok = SrGetIntFromHexString( HexStr, HEX_ADDRFIELD, HEX_ADDRSIZE, &Addr );
                if (!Ok) return( 0 );
                Ok = SrGetIntFromHexString( HexStr, HEX_RECFIELD,  HEX_RECSIZE,  &RecType );
                if (!Ok) return( 0 );

                if (RecType == 1) // End Record
                        break;

                if (RecType == 2) {
                        sprintf( SrUsbXchLastErrorString, "Invalid segment type record\n");
//WCT  Error = USBXCH_ERROR_BADSEG

                        return( 0 );
                        }

                Pos = HEX_DATAFIELD;
                for ( i = 0 ; i < Cnt ; i++ ) {
                        Ok = SrGetIntFromHexString( HexStr, Pos, 2, &Value );
                        if (!Ok) return( 0 );
                        DataString[i] = (unsigned char)Value;
                        Pos += 2;
                        }

                ihex++;



                Nbytes = SrUsbXchSendVendorRequest( UsbXchHandle,
                                              ANCHOR_LOAD_INTERNAL,
                                              (USHORT)Addr,
                                              0,
                                              DataString,
                                              (ULONG)Cnt,
                                              &Error );

                if (Nbytes == 0) {
                        sprintf(SrUsbXchLastErrorString, "Failed download at segment %d\n", ihex );
//WCT  Error = USBXCH_ERROR_HEXDOWN

                        return( 0 );
                        }



        } // end while

        return( 1 );
}


//WCT - Consider providing a user callable function for 8051 Reset

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchHelper8051RunHold
// PURPOSE: This function uses a vendor request to control the run/hold state
//          of the 8051 processor located on the USB chip on the USBxCH.  Pass
//          0 in the RunHold parameter to run and 1 to hold.
//------------------------------------------------------------------------------
int SrUsbXchHelper8051RunHold( DEVHANDLE UsbXchHandle, int RunHold ) {

	unsigned long NbytesToSend, NbytesSent;
	int           Error;
	char          Buffer[1];


        // Set up request data defaults

	Buffer[0]    = (char)RunHold;
	NbytesToSend = 1;


        // Send request

	NbytesSent = SrUsbXchSendVendorRequest( UsbXchHandle,
						ANCHOR_LOAD_INTERNAL,
						CPUCS_REG_FX2,
						0,
						Buffer,
						NbytesToSend,
						&Error );

	if ( NbytesSent == NbytesToSend )
		return( 1 );
	else
		return( 0 );
}





//*****************************************************************
//                     OS DEPENDENT FUNCTIONS
//*****************************************************************



// OS DEPENDENT FUNCTIONS:
//
// These functions help hide OS dependencies from the rest of the
// library by providing a common interface.  They should never be
// called by normal users.  The functions are repeated once for each
// supported OS, but the SROS_xxxxx defines select only one set to be
// compiled in to the code.
//
// Supported OS's include
//
//     SROS_WIN2K  Windows 2000/XP
//     SROS_LINUX  Linux 2.6.9 kernel (Fedora Core 3)
//
//


#define MAXENVSTR 256

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsDriverOpen (Windows version)
// PURPOSE: Open the specified device driver.  Driver names should be of the
//          form SrUsbXch# where # is a digit from 0 to 9.  This name is looked
//          up in the HKLM>Services>SrUsbXch>Parameters key in the registry.
//          Its value should contain the full ugly symbolic link name use to
//          access the driver.  It will look similar to this:
//
//          \??\USB#Vid_15d3&Pid_5504#5&22ee88&0&2#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
//
//          Our VID and PID are visible and followed by the USB port ID.  That
//          is followed by a GUID that appears to be used for general USB devices.
//          In order to use this symbolic link name from user space, we must 
//          change the "\??\" prefix to "\\.\"
//
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE SrUsbXchOsDriverOpen( char *DriverName ) {

        DEVHANDLE SrHandle;
        HANDLE    NtHandle;
        char      DeviceNameBuffer[MAXSTR];
        DWORD     RegType, BufferSize;
        HKEY      ParmKey;
        LONG      ret;
        
        // Error check input

        if ( strlen( DriverName ) >= MAXSTR-5 )
                return( BAD_DEVHANDLE );


        // Look up full GUID symbolic name from registry matching the
        // specified driver name

        ret = RegOpenKeyEx(
                           HKEY_LOCAL_MACHINE,
                           "System\\CurrentControlSet\\Services\\SrUsbXch\\Parameters",
                           0L, // reserved
                           KEY_READ, // samDesired,
                           &ParmKey
                          );
        if ( ret != ERROR_SUCCESS ) {
                sprintf( SrUsbXchLastErrorString,
                         "ERROR: Failed to open services parameter key (err=%ld)\n", ret );
//FIX what should we set last driver error to?
//                        SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( BAD_DEVHANDLE );
                }
        else {
                BufferSize = MAXSTR;
                ret = RegQueryValueEx(
                                    ParmKey,
                                    DriverName,
                                    NULL,
                                    &RegType,
                                    DeviceNameBuffer,
                                    &BufferSize
                                   );

                if ( ret != ERROR_SUCCESS ) {
                        sprintf( SrUsbXchLastErrorString,
                                 "ERROR: Failed to read %s registry value (err=%ld)\n", DriverName, ret );
//FIX what should we set last driver error to?
//                        SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                        RegCloseKey( ParmKey );
                        return( BAD_DEVHANDLE );
                        }
                else {
                        // Change from device name prefix from \??\ style to \\.\ style

                        DeviceNameBuffer[1] = '\\';
                        DeviceNameBuffer[2] = '.';
                        }

                RegCloseKey( ParmKey );
                }

//WCT        sprintf( DeviceNameBuffer, "\\\\.\\%s", DriverName );
//WCT        StringCchPrintf( DeviceNameBuffer, MAXSTR, "\\\\.\\%s", DriverName );

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
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                }
        else {
                SrHandle = (DEVHANDLE)NtHandle;
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                }

        return( SrHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsDriverClose (Windows version)
// PURPOSE: Close the specified device driver.
//------------------------------------------------------------------------------
SRLOCAL int SrUsbXchOsDriverClose( DEVHANDLE UsbXchHandle ) {
        return( CloseHandle( UsbXchHandle ) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsDriverIoctl (Windows version)
// PURPOSE: Send IOCTL command to the specified device driver.
//------------------------------------------------------------------------------
SRLOCAL int SrUsbXchOsDriverIoctl(
                                DEVHANDLE      UsbXchHandle,
                                unsigned long  IoCtlCode,
                                void          *pValueIn,
                                unsigned long  InSize,
                                void          *pValueOut,
                                unsigned long  OutSize,
                                unsigned long *ReturnSize
                               ) {
        int           Result;
        unsigned long BytesReturned;

        BytesReturned = 0L; // WCT was 999L for testing


        // Verify that device driver is valid (open).

        if (UsbXchHandle == BAD_DEVHANDLE) {
                SrUsbXchLastDriverError = ERROR_SERVICE_DISABLED;
                return( 0 );
                }


        // Request specialized services from device driver.

        Result = DeviceIoControl(
                       UsbXchHandle,     // Handle to device
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
                SrUsbXchLastDriverError = 0L;
                return( 1 );
                }
        else {
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
                return( 0 );
                }
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsDriverRead (Windows version)
// PURPOSE: Read data from the SrUsbXch device driver.
//------------------------------------------------------------------------------
SRLOCAL int SrUsbXchOsDriverRead(
				 DEVHANDLE     UsbXchHandle,
				 void          *pValues,
				 unsigned long  BytesToRead,
				 unsigned long *pBytesRead
				) {

	return(
	       ReadFile(
			UsbXchHandle,    // Handle to device
			pValues,         // Buffer to receive data
			BytesToRead,     // Number of bytes to read
			pBytesRead,      // Bytes read
			NULL             // NULL = wait till I/O completes
		       )
	      );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsGetLastError (Windows version)
// PURPOSE: Get the last error message from the driver.
//------------------------------------------------------------------------------
SRLOCAL long SrUsbXchOsGetLastError( void ) {

        return( GetLastError() );  // Provided by the OS
}


//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsSleep (Windows version)
// PURPOSE: Sleep for the specified number of milliseconds.
//------------------------------------------------------------------------------
SRLOCAL void SrUsbXchOsSleep( int ms ) {

        // Sleep for specifed number of milliseconds.

        if ( ms > 0 )
                Sleep( ms );
}


#elif defined( SROS_LINUX )

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsDriverOpen (Linux version)
// PURPOSE: Open the specified device driver.
//------------------------------------------------------------------------------
SRLOCAL DEVHANDLE SrUsbXchOsDriverOpen( char *DriverName ) {

        DEVHANDLE UsbXchHandle;
	char      DeviceNameBuffer[MAXSTR], DeviceNumBuffer[MAXSTR];
	char     *SysDeviceMap;
	FILE     *SysFp;
	int       i, c, ns, DriverNum, SysDeviceNum[MAX_DEVICES];


	// Error check input

	if ( strlen( DriverName ) >= MAXSTR-5 )
		return( BAD_DEVHANDLE );


	// Extract driver number from name and error check

	DriverNum = -1;

	ns = sscanf( DriverName, SRUSBXCH_DRIVER_NAME_D, &DriverNum ); // "SrUsbXch%d"

	if ( ns != 1 || DriverNum < 0 || DriverNum > 9 ) {
		sprintf( SrUsbXchLastErrorString,
			 "ERROR: Driver name must be SrUsbXch# where # is 0 to 9\n" );
		return( BAD_DEVHANDLE );
	}


	// Open sysfs devicemap file prepared by the driver.

	SysDeviceMap = "/sys/bus/usb/drivers/SrUsbXch/devicemap";

	SysFp = fopen( SysDeviceMap, "r" );
	if ( !SysFp ) {

		sprintf( SrUsbXchLastErrorString,
			 "ERROR: Failed to open sysfs %s file\n", SysDeviceMap );
//FIX what should we set last driver error to?
//                        SrUsbXchLastDriverError = SrUsbXchOsGetLastError();
		return( BAD_DEVHANDLE );
	}


	// Initialize device map variables

	memset( DeviceNameBuffer, 0, MAXSTR );
	memset(	DeviceNumBuffer, 0, MAXSTR );

	for ( i = 0 ; i < 10 ; i++ )
		SysDeviceNum[i] = -1;


	// Read and scanf devicemap contents from file

	i = 0;
	c = fgetc( SysFp );
	while ( c != EOF  &&  c != '\n' ) {
		DeviceNumBuffer[i++] = c;
		c = fgetc( SysFp );
	}

	fclose( SysFp );

	/*                              0  1  2  3  4  5  6  7  8  9 */
	ns = sscanf( DeviceNumBuffer, "%d %d %d %d %d %d %d %d %d %d",
		     &SysDeviceNum[0],
		     &SysDeviceNum[1],
		     &SysDeviceNum[2],
		     &SysDeviceNum[3],
		     &SysDeviceNum[4],
		     &SysDeviceNum[5],
		     &SysDeviceNum[6],
		     &SysDeviceNum[7],
		     &SysDeviceNum[8],
		     &SysDeviceNum[9]
		   );

//	if ( ns != 10 )
//		printf("Only read %d device numbers\n", ns );



	// Look up actual device name from devicemap list matching the
	// specified driver name/number.

	if ( SysDeviceNum[DriverNum] == -1 ) {
		sprintf( SrUsbXchLastErrorString, "ERROR: Driver not installed\n" );
		return( BAD_DEVHANDLE );
	}

	sprintf( DeviceNameBuffer, "/dev/%s%d", SRUSBXCH_DEVICE_NAME, SysDeviceNum[DriverNum] );



        UsbXchHandle = open( DeviceNameBuffer, O_RDWR );

        if (UsbXchHandle < 0)
                UsbXchHandle = BAD_DEVHANDLE;

        return( UsbXchHandle );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsDriverClose (Linux version)
// PURPOSE: Close the specified device driver.
//------------------------------------------------------------------------------
SRLOCAL int SrUsbXchOsDriverClose( DEVHANDLE UsbXchHandle ) {
        if ( close( UsbXchHandle ) < 0 )
                return( 0 );
        else
                return( 1 );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsDriverIoctl (Linux version)
// PURPOSE: Send IOCTL command to the specified device driver.
//------------------------------------------------------------------------------
SRLOCAL int SrUsbXchOsDriverIoctl(
                                DEVHANDLE      UsbXchHandle,
                                unsigned long  IoCtlCode,
                                void          *pValueIn,
                                unsigned long  InSize,
                                void          *pValueOut,
                                unsigned long  OutSize,
                                unsigned long *ReturnSize
                               ) {
        int code;
        IRP IoctlData;


        // Verify that device driver is valid (open).

        if (UsbXchHandle == BAD_DEVHANDLE) {
                SrUsbXchLastDriverError = ERROR_SERVICE_DISABLED;
                return( 0 );
                }



        // The ioctl function just takes one buffer.  On the way in,
        // it contains the input data and on the way out, it contains
        // the output data.  So, make sure we are using the largest of
        // the two and that it contains the input data.  The size of the
        // data must be the size defined for that IO control code in
        // SrUsbXchDrv.h.

        IoctlData.Command       = IoCtlCode;
        IoctlData.InBuffer      = pValueIn;
        IoctlData.InSize        = InSize;
        IoctlData.OutBuffer     = pValueOut;
        IoctlData.OutSize       = OutSize;
        IoctlData.ReturnedBytes = 0;
        IoctlData.DataMethod    = METHOD_BUFFERED;
        IoctlData.ErrorCode     = 0;
        IoctlData.UserIrp       =(void *)&IoctlData;


        // Check in SrUsbXchDrv.h for which control codes do not
        // use the more common buffered method of passing data

//      if ((IoCtlCode == IOCTL_??? ) {
//                IoctlData.DataMethod = METHOD_OUT_DIRECT;
//                }


        // Request specialized services from device driver.

        code = ioctl( UsbXchHandle, IoCtlCode, &IoctlData );

        if (ReturnSize)
                *ReturnSize = IoctlData.ReturnedBytes;



        if (code >= 0) {
                SrUsbXchLastDriverError = 0L;
                return( 1 );
                }
        else {
                SrUsbXchLastDriverError = SrUsbXchOsGetLastError();;
                return( 0 );
                }

}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsDriverRead (Linux version)
// PURPOSE: Read data from the SrUsbXch device driver.
//------------------------------------------------------------------------------
SRLOCAL int SrUsbXchOsDriverRead(
				 DEVHANDLE      UsbXchHandle,
				 void          *pValues,
				 unsigned long  BytesToRead,
				 unsigned long *pBytesRead
				) {
	long readreturn;

	readreturn = read( UsbXchHandle, pValues, BytesToRead );

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
// ROUTINE: SrUsbXchOsGetLastError (Linux version)
// PURPOSE: Get the last error message from the driver.
//------------------------------------------------------------------------------
SRLOCAL long SrUsbXchOsGetLastError( void ) {

        return( (long)(-errno) );
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchOsSleep (Linux version)
// PURPOSE: Sleep for the specified number of milliseconds.
//------------------------------------------------------------------------------
SRLOCAL void SrUsbXchOsSleep( int ms ) {

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





//*****************************************************************
//                     ??? FUNCTIONS
//*****************************************************************

// DEFAULT USB MODEL HELPER FUNCTION:
//
// The USBxCH family of 24 bit A/D boards includes the 1, 4, and 8 channel
// USB1CH, USB4CH, USB8CH models.  While the software is designed to work
// with any of the USBxCH models, the application programs often need to
// know which specific device is being used.  Typically, the specific
// model is given as a command line argument.  But for convenience, a
// default choice can be saved (in either the registry or an environment
// variable depending on OS) by using the setmodel utility.  This function
// retrieves this default model information.

FUNCTYPE( int ) SrUsbXchGetDefaultModel( char *DriverName ) {

        int AtodModel;

//        AtodModel = SrUsbXchOsGetDefaultModel( DriverName );
//        if (AtodModel == SRDAT_ATODMODEL_UNKNOWN)

        AtodModel = SRDAT_ATODMODEL_USB4CH;

        return( AtodModel );
}


//WCT - Debugging function

FUNCTYPE( void ) SrUsbXchLogMessage( char *Msg ) {

        char  datestr[16];
        char  timestr[16];
        FILE *ErrFp;
	int   Year, Month, Day, Hour, Minute, Second;
	long  Microsecond, Milliseconds;



	// Get the date and time.


	// Get system time with OS dependent helper function

	SrUsbXchGetPcTime( &Year, &Month, &Day, 
                           &Hour, &Minute, &Second,
                           &Microsecond );

	Milliseconds = Microsecond / 100;


	sprintf( datestr, "%04d/%02d/%02d", Year, Month, Day );
	sprintf( timestr, "%02d:%02d:%02d.%03ld", Hour, Minute, Second, Milliseconds );


	// Prepare message string

        sprintf( DbgStr, "%s %s %s", datestr, timestr, Msg );



        // Append the error to SrUsbXch.err.

        ErrFp = fopen( "SrUsbXch.err", "a" );

        if ( ErrFp ) {

                fprintf( ErrFp, "%s", DbgStr );

                   fclose( ErrFp );

                }

        DbgStr[0] = 'z';
        DbgStr[1] = 'z';
        DbgStr[2] = 'z';
}

FUNCTYPE( void ) SrUsbXchLogMessageV( char *Msg ) {

        char  datestr[16];
        char  timestr[16];
        FILE *ErrFp;
	int   Year, Month, Day, Hour, Minute, Second;
	long  Microsecond, Milliseconds;



	// Get the date and time.


	// Get system time with OS dependent helper function

	SrUsbXchGetPcTime( &Year, &Month, &Day, 
                           &Hour, &Minute, &Second,
                           &Microsecond );

	Milliseconds = Microsecond / 100;


	sprintf( datestr, "%04d/%02d/%02d", Year, Month, Day );
	sprintf( timestr, "%02d:%02d:%02d.%03ld", Hour, Minute, Second, Milliseconds );


        // Prepare message string

        sprintf( DbgStr, "%s %s %s", datestr, timestr, Msg );


        // Append the error to SrUsbXch.err.

        ErrFp = fopen( "SrUsbXch.err", "a" );

        if ( ErrFp ) {

                fprintf( ErrFp, "%s", DbgStr );

                   fclose( ErrFp );

                }


        Msg[0] = 'x';
        Msg[1] = 'x';
        Msg[2] = 'x';

        DbgStr[0] = 'z';
        DbgStr[1] = 'z';
        DbgStr[2] = 'z';
}

FUNCTYPE( void ) SrUsbXchLogMessageT( char *Msg ) {

        char  datestr[16];
        char  timestr[16];
        FILE *ErrFp;
	int   Year, Month, Day, Hour, Minute, Second;
	long  Microsecond, Milliseconds;



	// Get the date and time.


	// Get system time with OS dependent helper function

	SrUsbXchGetPcTime( &Year, &Month, &Day, 
                           &Hour, &Minute, &Second,
                           &Microsecond );

	Milliseconds = Microsecond / 100;


	sprintf( datestr, "%04d/%02d/%02d", Year, Month, Day );
	sprintf( timestr, "%02d:%02d:%02d.%03ld", Hour, Minute, Second, Milliseconds );


        // Prepare message string

        sprintf( DbgStrT, "%s %s %s", datestr, timestr, Msg );


        // Append the error to Scope.err.

        ErrFp = fopen( "SrUsbXchT.err", "a" );

        if ( ErrFp ) {

                fprintf( ErrFp, "%s", DbgStrT );

                   fclose( ErrFp );

                }
}

//------------------------------------------------------------------------------
// ROUTINE: SrUsbXchGetPcTime
// PURPOSE: Return the current PC system time as YMD HMS.
//
// Note:    This is similar to a SrHelper function, but is included here to
//          avoid having to mix libraries.
//------------------------------------------------------------------------------
int SrUsbXchGetPcTime( int *Year, int *Month, int *Day, 
                       int *Hour, int *Minute, int *Second,
                       long *Microsecond ) {

#if defined( SROS_WINXP ) || defined( SROS_WIN2K )

        SYSTEMTIME Systim;
        long       Usec;


        // Get current system time

        GetSystemTime( &Systim );
        Usec = ((long)Systim.wMilliseconds) * SR_USPERMS;

        
        // Return time as YMD HMS for UTC (not local)

        if (Year)         *Year        = Systim.wYear;
        if (Month)        *Month       = Systim.wMonth;
        if (Day)          *Day         = Systim.wDay;
        if (Hour)         *Hour        = Systim.wHour;
        if (Minute)       *Minute      = Systim.wMinute;
        if (Second)       *Second      = Systim.wSecond;
        if (Microsecond)  *Microsecond = Usec;

#else // SROS_LINUX

        time_t    ttNow;
        struct tm tmNow;
        double    dtime;
        long      ltime;

        // Return time as YMD HMS for UTC (not local)

        time( &ttNow );
        tmNow   = *gmtime( &ttNow );
        dtime   = (double)ttNow;
        ltime   = (long)ttNow;

        if (Year)         *Year        = tmNow.tm_year + 1900;
        if (Month)        *Month       = tmNow.tm_mon + 1;
        if (Day)          *Day         = tmNow.tm_mday;
        if (Hour)         *Hour        = tmNow.tm_hour;
        if (Minute)       *Minute      = tmNow.tm_min;
        if (Second)       *Second      = tmNow.tm_sec;
        if (Microsecond)  *Microsecond = (long)((dtime - ltime) * SR_USPERMS);

#endif // SROS_xxx

        return( 1 );

}

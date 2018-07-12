// FILE: SrUsbXch.h
// COPYRIGHT: (c), Symmetric Research, 2009-2010
//
// SR USBxCH C user library include file.
//
// Study this file to see the functions that can be called from user level
// programs and applications to control the USBxCH family of A/D devices.
//
// NOTE: To compile properly, one macro must be defined that specifies the
// target operating system.  See below for more details on the SROS_xxxxx
// macro.
//



#ifndef __USBXCH_H__
#define __USBXCH_H__ (1)


#include "SrDefines.h" // For function type, dev handle, 64 bit, + other defines

#if defined( SROS_LINUX )

#define GUID long 
#define VOID void 
#define CHAR char 
#define SHORT short
#define LONG long 
#define UCHAR unsigned char 
#define USHORT unsigned short 
#define ULONG unsigned long 
#define BOOLEAN unsigned char 
#define BOOL  unsigned char 
#define HANDLE void*

#endif // SROS_xxx



//------------------------------------------------------------------------------

// RELEASE REV:
// 
// This function allows the user to determine the library rev at
// runtime.  The rev is an integer.  A number like 123 would be referred
// to as rev "1.23".
// 


FUNCTYPE( int ) SrUsbXchGetRev( int *Rev );


//------------------------------------------------------------------------------
// Packet defines and data structures

#define SRDAT_USBPACKET_SIZE         64

#define SRDAT_USBPACKET_SAMPLE_SIZE  (SRDAT_USBPACKET_SIZE/2)
#define SRDAT_USBPACKET_SAMPLE_HDR   8
#define SRDAT_USBPACKET_SAMPLE_BYTES (SRDAT_USBPACKET_SAMPLE_SIZE - SRDAT_USBPACKET_SAMPLE_HDR)

#define SRDAT_USBPACKET_STATUS_HDR   1
#define SRDAT_USBPACKET_STATUS_BYTES (SRDAT_USBPACKET_SIZE - SRDAT_USBPACKET_STATUS_HDR)

#define SRDAT_USBPACKET_SANITY_MASK  0x0F  // Bottom nibble of TypeSanity byte specifies sanity
#define SRDAT_USBPACKET_TYPE_MASK    0xF0  // Top nibble of TypeSanity byte specifies data type
#define SRDAT_USBPACKET_TYPE_SAMPLE  0x00  //    as analog/digital sample data
#define SRDAT_USBPACKET_TYPE_STATUS  0x10  //    or serial/equip/gps status data

#define SRDAT_USBPACKET_TYPE_BYTE    0     // Packet type is located in byte index 0 (also first byte)
#define SRDAT_USBPACKET_SANITY_BYTE  0     // Packet sanity is located in byte index 0 (eg first byte)
#define SRDAT_USBPACKET_SANITY_BYTE2 (SRDAT_USBPACKET_SANITY_BYTE+SRDAT_USBPACKET_SAMPLE_SIZE)

struct SrUsbXchSampleLayout {
        unsigned char TypeSanity;
        unsigned char PpsToggle;
        unsigned char DigitalIn;
        unsigned char GGACount;
        unsigned char OnBoardCountBytes[4];
        unsigned char MuxedDataBytes[SRDAT_USBPACKET_SAMPLE_BYTES];
        } ;

struct SrUsbXchStatusLayout {
        unsigned char TypeSanity;
        char NmeaBytes[SRDAT_USBPACKET_STATUS_BYTES];
        };

union SrUsbXchPacketLayout {
        char                        Bytes[SRDAT_USBPACKET_SIZE];  // 64 bytes
        struct SrUsbXchSampleLayout Sample[2];                    //  2 sample structures (32 bytes each)
        struct SrUsbXchStatusLayout Status;                       //  1 status structure (1 hdr + 63 nmea bytes)
        };


typedef struct SrUsbXchSampleLayout SRUSBXCH_SAMPLE;
typedef struct SrUsbXchStatusLayout SRUSBXCH_STATUS;
typedef union  SrUsbXchPacketLayout SRUSBXCH_PACKET;


FUNCTYPE( int ) SrUsbXchPacketType( SRUSBXCH_PACKET *Packet );
FUNCTYPE( int ) SrUsbXchPacketSanity( SRUSBXCH_PACKET *Packet );
FUNCTYPE( int ) SrUsbXchPacketSanity2( SRUSBXCH_PACKET *Packet );

FUNCTYPE( unsigned long ) SrUsbXchPacketShow(
                                        FILE *fp,
                                        SRUSBXCH_PACKET *PacketArray,
                                        unsigned long    StartPacket,
                                        unsigned long    NpacketsToShow,
                                        int             *Error
                                        );

FUNCTYPE( int ) SrUsbXchPacketOutputBytes( FILE *fp, SRUSBXCH_PACKET *Packet );



// This structure contains the state variables maintained in the
// 8051 hex code.  Be sure this structure matches the hex code.

#define SRUSBXCH_STATE_COUNT 8

struct SrUsbXchStateLayout {
        int FirmRev;
        int PowerInfo;
        int CountPps;
        int LedValue;
        int TempHi;
        int TempLo;
        int DigitalIn;
        int DramFlags;
        } ;
typedef struct SrUsbXchStateLayout SRUSBXCH_STATE;



//------------------------------------------------------------------------------
// Basic functions

// SIMPLE USBxCH CONTROL/EXECUTION DEFINES AND FUNCTIONS:
// 
// The following "easy to use" defines and functions provide a basic
// level of control over the USBxCH.  Many applications will not need
// anything else.  The basic pattern of usage is:
// 
// 1) Open and initialize the USBxCH, setting the sampling rate
// 2) Start conversion
// 3) Call GetDataAsPackets to copy ready data from the USBxCH to the PC 
// 4) Loop to continue getting more data
// 5) When done, stop conversion and close the USBxCH
// 
// For more detailed control of the USBxCH see the ADVANCED defines and
// functions below.
//        
// Note that the function SrUsbXchOpen returns a handle to the USBxCH
// device.  This handle must be passed to any function that needs to
// access the USBxCH hardware.  Open also fills in an integer error
// code, which has more information about what may have gone right or
// wrong.  The USBXCH_ERROR ... defines below provide a mapping from
// these error codes to strings.
// 


#define USBXCH_DEFAULT_DRIVER_NAME       "SrUsbXch0"



FUNCTYPE( DEVHANDLE ) SrUsbXchOpen(
                                  char *DriverName,  // USBXCH_DEFAULT_DRIVER_NAME
                                  int UsbXchModel,   // SRDAT_ATODMODEL_USB4CH
                                  int UsbGpsModel,   // SRDAT_GPSMODEL_PCTIME
                                  int UserCfgByte,   // USERCFG_DEFAULT_BYTE
                                  double Sps,        // requested sample rate

                                  double *ActualSps, // actual sample rate
                                  int *Error         // error
                                  );

FUNCTYPE( int ) SrUsbXchClose( DEVHANDLE UsbXchHandle );

FUNCTYPE( int ) SrUsbXchStart( DEVHANDLE UsbXchHandle, int *Error );
FUNCTYPE( int ) SrUsbXchStop( DEVHANDLE UsbXchHandle, int *Error );

FUNCTYPE( unsigned long ) SrUsbXchGetDataAsPackets(
                                DEVHANDLE        UsbXchHandle,
                                SRUSBXCH_PACKET *PacketArray,
                                unsigned long    NpacketsRequested,
                                unsigned long   *NpacketsRead,
                                int             *Error
                                );



// User LED and digital IO functions

#define LED_OFF 0
#define LED_ON  1

FUNCTYPE( void ) SrUsbXchUserLed( DEVHANDLE SrUsbXchHandle, int YellowState, int RedState );

FUNCTYPE( void ) SrUsbXchUserIoRd( DEVHANDLE SrUsbXchHandle, int *Value );
FUNCTYPE( void ) SrUsbXchUserIoWr( DEVHANDLE SrUsbXchHandle, int Value );


// Dram functions

FUNCTYPE( int ) SrUsbXchEmpty( DEVHANDLE UsbXchHandle );
FUNCTYPE( int ) SrUsbXchReady( DEVHANDLE UsbXchHandle );
FUNCTYPE( int ) SrUsbXchOverflow( DEVHANDLE UsbXchHandle );



// Sampling rate functions

// The ADS1255 can ONLY sample at 16 predefined rates.  The function
// SrUsbXchSpsRateValidate takes a requested sample rate and returns the
// closest available rate.

// The available rates are computed based on the crystal clock speed and
// the intergral number of values to average.  These values are taken
// from the spec sheet and are stored in the SampleRateInfo table in
// SrUsbXch.c.

// The function SrUsbXchSpsRateTable returns the sampling rate for the
// specified index number 0 thru 15 in the SampleRateInfo table.  SPS
// means samples per second

#define USBXCH_SPS_DEFAULT               130.2  // default for 10MHz xtal

#define USBXCH_SPS_INDEX_MIN              0     // about 3 Hz
#define USBXCH_SPS_INDEX_DEFAULT          8     // about 130 Hz
#define USBXCH_SPS_INDEX_MAX             15     // about 39 kHz

FUNCTYPE( int ) SrUsbXchSpsRateValidate( double SpsRequested,
                                         double *SpsActual,
                                         int *Error );
FUNCTYPE( int ) SrUsbXchSpsRateTable( int SpsIndex,
                                      double *SpsActual,
                                      int *Error );


// Miscellaneous info functions

#define USBXCH_POWER_CURRENT    0       // POWER_xxx defines MUST match
#define USBXCH_POWER_HISTORY    1       // PWR_xxx values in firmware

#define USBXCH_POWER_BAD        0       // The sense is what a user would
#define USBXCH_POWER_GOOD       1       // expect since the firmware inverted
#define USBXCH_POWER_UNKNOWN    9       // the negative signal for us

FUNCTYPE( int ) SrUsbXchSendTimeStamp( DEVHANDLE UsbXchHandle, int *Error );
FUNCTYPE( int ) SrUsbXchIsPowerGood( DEVHANDLE UsbXchHandle );
FUNCTYPE( int ) SrUsbXchIsPowerBitGood( int PowerInfo, int PowerBit );
FUNCTYPE( int ) SrUsbXchGetRevDriver( DEVHANDLE UsbXchHandle, int *Rev );
FUNCTYPE( int ) SrUsbXchGetRevFirmware( DEVHANDLE UsbXchHandle, int *Rev );
FUNCTYPE( int ) SrUsbXchGetFrameNumber( DEVHANDLE UsbXchHandle, int *FrameNumber );
FUNCTYPE( int ) SrUsbXchGetDefaultModel( char *DriverName );
FUNCTYPE( int ) SrUsbXchGetDready( DEVHANDLE UsbXchHandle, int *Dready, int *Error );



// Use the following defines with the SrUsbXchFillUserCfgByte function
// to control various functions related to the behaviour of signals
// on 25 pin connector on the USBxCH board.
//
// Each UserCfg parameter has two possible settings 0 and 1.  The
// default on power up is alway the 0 option.  The defines have been
// selected to match the desired default.  The result is variability in
// the sense of some defines which means the defines ending in IDLE_HIGH
// are not all the same value.  For example USERCFG_NMEA_TX_IDLE_HIGH is
// 0, while USERCFG_PPS_IDLE_HIGH is 1.
//
// Do NOT change these as they MUST match the values expected by the XCR
// firmware (ABEL code).

#define USERCFG_DEFAULT_BYTE             0x00

#define USERCFG_NMEA_TX_IDLE_HIGH        0 // For UserCfgNmeaTxPolarity parameter
#define USERCFG_NMEA_TX_IDLE_LOW         1
#define USERCFG_NMEA_RX_IDLE_HIGH        0 // For UserCfgNmeaRxPolarity parameter
#define USERCFG_NMEA_RX_IDLE_LOW         1
#define USERCFG_PPS_LED_TOGGLE           0 // For UserCfgPpsLed parameter
#define USERCFG_PPS_LED_PROGRAMMED       1
#define USERCFG_PPS_IDLE_LOW             0 // For UserCfgPpsPolarity parameter
#define USERCFG_PPS_IDLE_HIGH            1
#define USERCFG_DB25_ADRDY_DISABLED      0 // For UserCfgAdrdy parameter
#define USERCFG_DB25_ADRDY_ENABLED       1
#define USERCFG_TRIGGER_IDLE_LOW         0 // For UserCfgTriggerPolarity parameter
#define USERCFG_TRIGGER_IDLE_HIGH        1

//#define USERCFG_PPS_SOURCE_DB25          0 // For UserCfgPpsSource parameter
//#define USERCFG_PPS_SOURCE_USB           1

#define USERCFG_UNUSED                   0 // For UserCfgNotUsed1 parameter


// UserCfg Defaults:

#define USERCFG_NMEA_TX_IDLE_DEFAULT USERCFG_NMEA_TX_IDLE_HIGH   // 0 NmeaTx Idle High (= normal RS232)
#define USERCFG_NMEA_RX_IDLE_DEFAULT USERCFG_NMEA_RX_IDLE_HIGH   // 0 NmeaRx Idle High (= normal RS232)
#define USERCFG_PPS_LED_DEFAULT      USERCFG_PPS_LED_TOGGLE      // 0 Red LED control by PPS
#define USERCFG_PPS_IDLE_DEFAULT     USERCFG_PPS_IDLE_LOW        // 0 PPS active on rising edge
#define USERCFG_DB25_ADRDY_DEFAULT   USERCFG_DB25_ADRDY_DISABLED // 0 A/D DREADY signal disabled on DB25
#define USERCFG_UNUSED_DEFAULT       USERCFG_UNUSED              // 0 Not used
#define USERCFG_TRIGGER_IDLE_DEFAULT USERCFG_TRIGGER_IDLE_LOW    // 0 Trigger active on rising edge
//#define USERCFG_PPS_SOURCE_DEFAULT   USERCFG_PPS_SOURCE_DB25     // 0 PPS signal from GPS antenna (true PPS)


FUNCTYPE( int ) SrUsbXchFillUserCfgByte(
                int NmeaTxPolarity,    // Used to program antenna, idle high or low
                int NmeaRxPolarity,    // Used to read NMEA strings, Idle high or low
                int PpsLed,            // Red LED controled by PPS signal or program
                int PpsPolarity,       // GPS time aligns on rising or falling edge 
                                       // PpsSource: PPS signal real from GPS antenna or faked by PC
                int Adrdy,             // A/D Dready signal enabled or disabled
                int NotUsed1,
                int TriggerPolarity,   // Trigger occurs on rising or falling edge
                int *UserCfgByte
                );









//------------------------------------------------------------------------------
// NMEA defines

// NMEA DEFINES:
// 
// NMEA, the National Marine Electonics Association, has defined a number
// of standard messages to be used with marine instruments.  The SR PARGPS
// is set up to use the RMC or ZDA and GGA messages.
// 
// The USB4CH and the Rev D PARGPS use a Garmin receiver programmed for RMC
// and GGA.  See Chapter 4 of the Garmin GPS 18 LVC manual for a full
// description of the possible NMEA messages.  Sample messages are:
// 
//   $GPRMC,204634,A,3608.9137,N,11517.9635,W,000.0,217.1,300508,013.4,E*65
//   $GPGGA,204634,3608.9137,N,11517.9635,W,1,04,,,M,-25.3,M,,*71
// 
// The Rev C PARGPS uses a Trimble receiver programmed for ZDA and GGA.
// See Appendix E of the Trimble ACE-III GPS receiver manual for a full
// description of the possible NMEA messages.  Sample messages are:
// 
//   $GPZDA,232225.1,03,06,2003,,*55 (UTC,day,month,year)
//   $GPGGA,232225.0,3609.237,N,11518.586,W,1,04,2.45,00751,M,-024,M,,*6A
// 


#define SRDAT_NmeaStartTag  "$"
#define SRDAT_NmeaTalkerId  "GP"
                                // Note: The NMEA message IDs are listed
                                // in the order they come out from Trimble
#define SRDAT_NMEA_MSGID_MIN  -1
#define SRDAT_NMEA_MSGID_ZDA   0
#define SRDAT_NMEA_MSGID_GGA   1
#define SRDAT_NMEA_MSGID_GLL   2
#define SRDAT_NMEA_MSGID_VTG   3
#define SRDAT_NMEA_MSGID_GSA   4
#define SRDAT_NMEA_MSGID_GSV   5
#define SRDAT_NMEA_MSGID_RMC   6
#define SRDAT_NMEA_MSGID_MAX   7

#define SRDAT_NMEA_ERR_NONE           0
#define SRDAT_NMEA_ERR_INVALID_PARM   1
#define SRDAT_NMEA_ERR_NO_DOLLAR      2
#define SRDAT_NMEA_ERR_NO_MSGID       3
#define SRDAT_NMEA_ERR_NO_EQUIPBYTE   4
#define SRDAT_NMEA_ERR_BAD_EQUIPDATA  5
#define SRDAT_NMEA_ERR_NO_CARAT       6
#define SRDAT_NMEA_ERR_NO_DATA        7
#define SRDAT_NMEA_ERR_NO_SEQUENCE    8
#define SRDAT_NMEA_ERR_NO_CR          9
#define SRDAT_NMEA_ERR_NO_CHECKSUM   10
#define SRDAT_NMEA_ERR_NO_LF         11
#define SRDAT_NMEA_ERR_NO_SPACE      12

#define SRDAT_NMEA_MAX_NAME         5   // Each MsgName has 5 char (eg GPGGA)
#define SRDAT_NMEA_MAX_SIZE         83  // Longest possible msg
#define SRDAT_NMEA_MAX_TYPE         SRDAT_NMEA_MSGID_MAX
#define SRDAT_NMEA_MAX_BUFF        (SRDAT_NMEA_MAX_TYPE*SRDAT_NMEA_MAX_SIZE)
#define SRDAT_NMEA_MAX_TOKEN       (2*SRDAT_NMEA_MAX_SIZE)

#define SRDAT_NMEA_MAX_FIELDS       22  // One more than truly needed
#define SRDAT_NMEA_MAX_FIELDSIZE    14  // Normally would be 11, but we make a
                                        // synthetic ZDA with full precision time
                                        // for timestamping with PcTime

typedef struct NMEA_parse {
        int  Nfields;
        int  CheckSum;
        char Field[SRDAT_NMEA_MAX_FIELDS][SRDAT_NMEA_MAX_FIELDSIZE];
        } SRDAT_NMEAPARSE;


struct NmeaInfoLayout {
        int    Nsat;
        int    Year;
        int    Month;
        int    Day;
        int    Hour;
        int    Minute;
        int    Second;
        long   MicroSecond;
        double SecSince1970;
        double Latitude;
        double Longitude;
        double Altitude;
        int    YmdSource;
        int    HmsSource;
        int    SatSource;
        int    LocSource;
        int    PosSource;
        int    YmdIsValid;
        int    HmsIsValid;
        int    SatIsValid;
        int    LocIsValid;
        int    PosIsValid;
        int    SecIsValid;
        };

#define NI struct NmeaInfoLayout

struct NMEA_process; // forward declaration for NMEAPROCESS


FUNCTYPE( int ) SrDatNmeaInfoInit( NI *NmeaInfo );
FUNCTYPE( int ) SrDatNmeaOrderInit( int GpsModel );
FUNCTYPE( int ) SrDatNmeaPcTimeToZDA( char *ZdaString, int Nmax );
FUNCTYPE( int ) SrDatNmeaAddCheckSum( char *NmeaString, int *CheckSum );

FUNCTYPE( int ) SrDatRequestNmeaInfo2( char *NmeaMsgAll, int NmeaCount,
                int *Year, int *Mon, int *Day, int *YmdSource, int *YmdIsValid,
                int *Hour, int *Minute, int *Second,
                long *MicroSecond, int *HmsSource, int *HmsIsValid,
                double *Latitude, double *Longitude, int *LocSource, int *LocIsValid,
                double *Altitude, int *PosSource, int *PosIsValid,
                int *Nsat, int *SatSource, int *SatIsValid );
FUNCTYPE( int ) SrDatParseNmeaData( char *NmeaMsg, SRDAT_NMEAPARSE *ParseData );
FUNCTYPE( int ) SrDatMatchNmeaTable( char *MsgId, struct NMEA_process **t );
FUNCTYPE( int ) SrDatExtractParsedDate( SRDAT_NMEAPARSE *ParseData,
                                         int DateLength, int DateField,
                                         int *pYr, int *pMo, int *pDy );
FUNCTYPE( int ) SrDatExtractParsedTime( SRDAT_NMEAPARSE *ParseData,
                                         int TimeField,
                                         int *pHh, int *pMm, int *pSs,
                                         long *pUsec );
FUNCTYPE( int ) SrDatExtractParsedLoc( SRDAT_NMEAPARSE *ParseData,
                                        int LatLonField,
                                        double *pLat, double *pLon );
FUNCTYPE( int ) SrDatExtractParsedAlt( SRDAT_NMEAPARSE *ParseData,
                                        int AltitudeField,
                                        int AltitudeRank,
                                        double *pAlt );
FUNCTYPE( int ) SrDatExtractParsedSat( SRDAT_NMEAPARSE *ParseData,
                                        int NmeaMsgIndex, int NsatField, int *pSat );
FUNCTYPE( int ) SrDatRoundTimeToPps( int *Year, int *Month, int *Day,
                                      int *Hour, int *Minute, int *Second,
                                      long *MicroSecond );

FUNCTYPE( int ) SrDatPack3( int Top, int Middle, int Bottom, int *Xxx  );
FUNCTYPE( int ) SrDatUnpack3( int Xxx, int *Top, int *Middle, int *Bottom );
FUNCTYPE( int ) SrDatCalcTimeZoneCorr( long StartTime );
FUNCTYPE( int ) SrDatSecTimeSplit( double Time,
                                    int *Year, int *Month, int *Day,
                                    int *Hour, int *Minute, int *Second,
                                    long *Microsecond );
FUNCTYPE( int ) SrDatSecTimeCombine( int Year, int Month, int Day,
                                      int Hour, int Minute, int Second,
                                      long Microsecond, double *Time );



//------------------------------------------------------------------------------
// USB Analog, Serial, Equipment structures


// These structures contain the three types of data which can be
// returned by the USBxCH A/D.  These are 1) analog data, 2) serial
// data, and 3) equipment data.  The analog data arrives at the A/D
// sampling rate which is typically 100 to 5000 Hz and includes not only
// the converted value for each channel but also the value of the
// digital inputs at the time of that sample.  The serial and equipment
// data arrive at a 1 second GPS rate.  The serial data includes the GPS
// NMEA strings with coarse time, date, and location information.  The
// equipment data includes things like the USBxCH voltage good signal
// and the results from the onboard temperature sensor.

// If any changes are made here, be sure to update both the related
// SrUsbXch library (functions such as SrDatUsbEquipInit and
// EquipmentProcessData) and the SrUsbXch hex code (functions such as
// EquipDataSave).  The data being written by the hex code, should be
// the same as what is expected by the library and SrDat code.


#define SRDAT_USBXCH_MAX_ANALOG_CHAN 8         // Can have max of 8 analog channels
                                               // since muxed data is in 8-bit bytes

#define SRDAT_INVALID_PPSEVENT  0

#define SRDAT_VALID_NONE       0x0
#define SRDAT_VALID_PPS        0x1
#define SRDAT_VALID_DREADY     0x2
#define SRDAT_VALID_NMEA       0x4
#define SRDAT_VALID_DATE       0x8

// See SrDatTimeMethodString function for matching strings
                                  // Method for computing time at an arbitrary sample
#define SRDAT_TIME_METHOD_NONE 0  //   Assume perfect sampling
#define SRDAT_TIME_METHOD_SPS  1  //   From timestamp and sampling rate
#define SRDAT_TIME_METHOD_GPS  2  //   From PARGPS and PC counter (10us typical resolution)
#define SRDAT_TIME_METHOD_OBC  3  //   From PARGPS and PAR8CH OnBoardCounter (800ns res)
#define SRDAT_TIME_METHOD_PCT  4  //   From PC system time (10ms resolution on Windows!)
#define SRDAT_TIME_METHOD_MAX  5  //   maximum time method value


typedef struct UsbAnalogData {
        int PpsEventNum;
        int Sanity;
        int PpsToggle;
        int DigitalIn;
        int GGACount;
        unsigned long OnBoardCount;
        long          AnalogData[SRDAT_USBXCH_MAX_ANALOG_CHAN];
        } SRDAT_USBXCH_ANALOG;

typedef struct UsbSerialData {               // USBxCH uses same SRDAT_SERIALDATA
        int   ValidFields;                   // serial structure as the PARxCH hardware
        int   PpsEventNum;
        int   NmeaCount;
        char  NmeaMsg[SRDAT_NMEA_MAX_BUFF];
        } SRDAT_USBXCH_SERIAL;

typedef struct UsbEquipmentData {
        int   Nbytes;      // 1 byte
        int   PpsEventNum; // 1 byte
        int   VoltageGood; // 1 byte
        int   Temperature; // 2 bytes
        int   DramFlags;   // 1 byte
        int   Unused1;     // 1 byte
        int   Unused2;     // 1 byte
        } SRDAT_USBXCH_EQUIPMENT;
                           // 8 bytes total

// Maximum number of equipment data and carat bytes
// Equipment bytes are NumBytes, PpsEventNum, VoltageGood, TempHi, TempLo, DramFlags, etc
// Carat bytes include start and end carats
// Be sure to change firmware if these values change

#define SRDAT_USBXCH_MAX_EQUIPBYTES  8
#define SRDAT_USBXCH_MAX_CARATBYTES (SRDAT_USBXCH_MAX_EQUIPBYTES+2) 




FUNCTYPE( int ) SrDatUsbAnalogInit( SRDAT_USBXCH_ANALOG    *AnalogData );
FUNCTYPE( int ) SrDatUsbSerialInit( SRDAT_USBXCH_SERIAL    *SerialData );
FUNCTYPE( int ) SrDatUsbEquipInit(  SRDAT_USBXCH_EQUIPMENT *EquipData );

FUNCTYPE( int )  SrUsbXchClearData( SRDAT_USBXCH_ANALOG *Data, int Nsamples );





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
                                );
FUNCTYPE( unsigned long ) SrDatUsbPacketCleanUp(
                                        SRUSBXCH_PACKET *PacketArray,
                                        unsigned long    NpacketsRead,
                                        unsigned long    NpacketsProcessed,
                                        int             *Error
                                        );

#define USBXCH_TEMP_UNKNOWN     0xFFFF
#define USBXCH_TEMP_DEG_UNKNOWN (double)999.0

FUNCTYPE( void ) SrDatUsbTemperatureCombine( char TA0, char TA1, int *Temp );
FUNCTYPE( void ) SrDatUsbTemperatureCompute( int Temp, double *TempC, double *TempF );

unsigned long SrDatUsbAnalogFill( SRUSBXCH_PACKET     *Array,
                                  int                 CheckSanity,
                                  SRDAT_USBXCH_ANALOG *DataValues,
                                  int                 *Failure );

FUNCTYPE( unsigned long ) SrDatUsbAnalogDemux( SRUSBXCH_SAMPLE     *Sample,
                                               int                 CheckSanity,
                                               SRDAT_USBXCH_ANALOG *DataValues,
                                               int                 *Failure
                                              );
unsigned long SrDatOnBoardCountDemux( unsigned char *OnBoardCountBytes );

int SrDatByteDemux( unsigned char **Array, unsigned char *DmxValues );

int SrDatUsbSerialFillBuffer( SRUSBXCH_PACKET *Array, int CheckSanity, int *Failure );
int SrDatUsbSerialParseBuffer( int                    *LastMsg,
                               SRDAT_USBXCH_SERIAL    *SerialValues,
                               unsigned long           NserialRequested,
                               unsigned long          *NserialRead,
                               SRDAT_USBXCH_EQUIPMENT *EquipValues,
                               unsigned long           NequipRequested,
                               unsigned long          *NequipRead,
                               int                    *Failure );
int SrDatSerialPrepareStructure( char *NmeaGroup, int NmeaCount, int LastSecondCount,
                            int GotEqData, SRDAT_USBXCH_EQUIPMENT *EquipData,
                            SRDAT_USBXCH_SERIAL *SerialValues,
                            SRDAT_USBXCH_EQUIPMENT *EquipValues );

int SrDatEquipmentProcessData( char *EquipBytes,
                               SRDAT_USBXCH_EQUIPMENT *EquipData,
                               int *Error );

int SrDatSerialReadNmea( int   MaxSize,
                         char *NmeaMsg,
                         int  *CurrMsg,
                         int  *Nchar,
                         int  *SecondCount,
                         int  *GotEqData,
                         SRDAT_USBXCH_EQUIPMENT *EquipData );

// Serial Buffer Helper Functions
void  SerBufInit( void );
void  SerBufClearBytes( void );
void  SerBufFlush( void );
int   SerBufEmpty( void );
int   SerBufFull( void );
int   SerBufNumChar( void );
int   SerBufNumFree( void );
void  SerBufIncUsePtr( void );
void  SerBufIncFillPtr( void );
int   SerBufGetToken( char *Token,
                      char  Sep,
                      int   breakdollar,
                      int  *chksum,
                      int  *numchar,
                      int   maxtoken );
int   SerBufGetTokenByCount( char *Token,
                             int   Count,
                             int   breakdollar,
                             int  *chksum,
                             int  *numchar,
                             int   maxtoken );
int   SerBufGetNextChar( char *c );
int   SerBufPeekOne( char *c );
int   SerBufPutOne( char c );
int   SerBufGetMsgIndex( char *Token );
int   SerAtoix( char *src, int n );



//------------------------------------------------------------------------------
// Timestamp layout

struct TimeStampLayout {
        int          Valid;
        int          PpsCount;
        int          Sample;
        double       TotalTime;
        double       SecSince1970;
        double       ExtraSec;
        unsigned int ObcCount;
        int          NumSat;
        int          YmdSource;
        int          HmsSource;
        int          PowerInfo;
        int          Temperature;
        };

#define TS struct TimeStampLayout

   int SrDatTimestampInit( TS *TimeStamp );
   int SrDatTimestampWriteFile( FILE *Fptr, TS *TimeStamp, int WriteTitle );
   int SrDatTimestampReadFile( FILE *Fptr, TS *TimeStamp, int SkipTitle );
  void SrDatSkipTitleLine( FILE *Fptr );





//------------------------------------------------------------------------------
// DAT Header format

//**************************** SrDat Header ***************************
// 
// The DAT file format is comprised of a header followed by records.
// The header size is fixed at 4096 bytes and is comprised of a C
// structure, struct SrDatHdrLayout, followed by zero padding.  The
// fields in the header structure include items such as sampling rate,
// number of channels and etc.
//
// HEADER FIELD DESCRIPTIONS:
// 
// Following are short descriptions of the fields in the header struct
// SrDatHdrLayout.  They are listed by functional groups.
// 
// 
// GENERAL HEADER PARAMETERS:
// 
// * DatId
// 
// A long integer for identifying a DAT file easily.  Must always equate
// to the string "SRSR".
// 
// * DatRev
// 
// The DAT file format revision number.
// 
// * DatProduct
// 
// An ascii string indicating which SR A/D hardware produced the acquired
// data stored in the DAT file.  Typical examples would be:
// 
//         "SR PAR8CH with GPS"
//         "SR USB4CH"
// 
// * DatApplication
// 
// An ascii string indicating the application program that generated the
// DAT file.  Typical examples would be:
// 
//         "SR SIMP4CH"
//         "SR SCOPE4CH"
//         "SR SCOPEA64"
// 
// * DatAtodModel (was DatUnused1)
// 
// An indicator of which A/D board was used to acquire the data.  Possible
// values are the SRDAT_ATODMODEL_ defines below and include the 1, 4, and 8
// channel members of the PARxCH.
// 
// 
// * DatGpsModel was (DatUnused2)
// 
// An indicator of which GPS receiver was used to time stamp the data.
// Possible values are the SRDAT_GPSMODEL_ defines below and include the
// PARGPS (with Trimble ACE-III receiver) and PCTIME which uses the system
// time from the PC (presumably set using NTP).
// 
// 
// 
// 
// FILE RELATED PARAMETERS:
// 
// * FileName
// 
// The original name of the DAT file when it was created by the
// application.
// 
// * FileCreateTime
// 
// The local time when the DAT file was created, stored as seconds since
// 1/1/1970.
// 
// * FileSeqNum
// 
// A sequential number indicating this is the n-th file written during
// the current run of the application.
// 
// * FileStartPtNum
// 
// The number of the first point in this file, where point 0 was the
// first point in the first file during the current acquisition run.
// 
// * FileCreateTimeHi (was FileUnused1)
// 
// High part of time when it is represented as a 64 bit integer.
// 
// * FileNameType
// 
// Indicates what naming style was used to generate the file names when
// acquiring data.  Possible values are the SRDAT_FILENAMETYPE_ defines
// below.  When one of the sequential types is used, downstream programs
// can automatically determine the name of adjacent files.  Thus allowing
// many files to be processed in order with one command.
// 
// * FileNameInc
// 
// Indicates a numeric increment between the name of one file and the
// following file.  Typically, this would be 1 for sequential files.
// But it might be a larger number if the file names were generated
// based on the TIME naming style.
// 
// 
// 
// 
// 
// DATE AND TIME PARAMETERS:
// 
// * TimePoint[]
// 
// A structure with YMD date and HMS time information for up to four time
// points.  The various fields in the structure are detailed below.
// 
// * Meaning
// 
// An indicator of what the time data represents.  Possible values are
// the SRDAT_TIME_TYPE_ defines below.  Typically the meaning will
// indicate whether the time is for the first or last point in the file
// or the first or last point with a GPS mark.
// 
// * SampleFromFile, SampleFromRun
// 
// Index of the analog data sample the time information is for.  Index 0
// is the first point in the file for SampleFromFile and the first point
// in the current run of the application for SampleFromRun.
// 
// * Year, Month, Day, Hour, Minute, Second, Microsecond
// 
// UTC date (YMD) and time (HMS) for the identified sample point.  Year
// is a standard 4 digit value like 2002, month goes from 1 to 12 and day
// goes from 1 to 31.  Hour goes from 0 to 23, minute and second go from
// 0 to 59, and microsecond goes from 0 to 999999.
// 
// * YMDQuality, HMSQuality
// 
// Indicators of how the YMD and HMS values were determined.  Possible
// values are the SRDAT_SOURCE_ defines below.  Typically the quality
// will indicate whether the source was the PC clock or GPS system.
// 
// 
// 
// 
// 
// 
// CHANNEL RELATED PARAMETERS:
// 
// * Channels
// 
// Total number of channels recorded.  Most of the channels acquired are
// analog data, but one may be special channels.  These special channels
// include packed digital data, the "GPS mark" channel, and the "OBC" or
// counter channel.  The maximum number of channels allowed is 65.
// 
// * ChannelsAnalog
// 
// Total number of analog channels recorded.
// 
// * ChannelsDigital
// 
// Total number of digital channels recorded.  This is the number of longs
// allocated to digital data.  Since each digital signal is represented by
// a single bit, the info from several signals can be packed into a single
// long.  This number is saved in the header as DigitalPerChan.
// 
// * ChannelsGps
// 
// Total number of GPS mark channels recorded.  The GPS mark channel
// contains a count of how many PPS signals have been received since the
// beginning of acquisition.
// 
// * ChannelsCounter
// 
// Total number of counter channels recorded.  The counter channel contains
// the PAR8CH On Board Counter (OBC) count value that indicates how much
// time has elapsed between the PPS signal and the next data sample becoming
// ready.
// 
// * ChannelMarkIndex
// 
// The index of a special "GPS mark" channel.  An example would be a SR
// PAR4CH system with GPS attached.  Such a system has 4 analog channels
// (0,1,2,3) and one "GPS mark" channel (4).  For this system, Channels = 5,
// and ChannelMarkIndex = 4.
// 
// If only analog data has been recorded and there is no GPS mark
// channel, then ChannelMarkIndex = SRDAT_INDEXCHANNEL_NONE.
// 
// * ChannelDigIndex
// 
// The index of a special channel containing the value of the four
// PAR8CH digital inputs packed into a single long integer.  Since the
// PAR1CH and PAR4CH do not continuously recored the digital inputs in
// this way they would use ChannelDigIndex = SRDAT_INDEXCHANNEL_NONE.
// 
// * ChannelObcIndex
// 
// The index of a special channel containing the value of the high-speed
// PAR8CH On Board Counter.  Since the PAR1CH and PAR4CH do not have this
// hardware OBC they would use ChannelObcIndex = SRDAT_INDEXCHANNEL_NONE.
// 
// * ChannelPtsPerRecord
// 
// The number of analog samples per channel in one ANALOG16 or ANALOG24
// data record.  Multiply by Channels to get the total number of samples
// per record.
// 
// * ChannelPtsPerFile
// 
// The number of analog samples per channel in the entire file.  These
// samples may be spread across many ANALOG16 or ANALOG24 data records.
// Note that DAT files are assumed to have only ANALOG16 or ANALOG24
// records, but not both, and all ANALOG records are also assumed to
// have the same number of points.
// 
// * ChannelList
// 
// An array of structures containing an scan list index and short 12
// character title string for each channel.  This array is dimensioned
// to the maximum number of channels, but only contains entries for the
// recorded channels counted in the Channels field.
// 
// * DigitalPerChan
// 
// This is the number digital signals that have been packed into a single
// digital channel.  ChannelsDigital counts the number of these digital
// channels.  For the PAR8CH, 4 digital signals are packed into 1 digital
// channel.
// 
// * DigitalList
// 
// An array of structures containing an scan list index and short 12
// character title string for each digital.  This array is dimensioned
// to a maximum number of digital channel, but only contains entries for the
// recorded digital channels counted in the ChannelsDigital field.
// 
// 
// 
// A/D ANALOG CONVERTER PARAMETERS:
// 
// * AtodWordSize
// 
// Number of bytes per sample for the acquired analog data.  SR 24 bit
// products store their data in 32 bit longs and have AtodWordSize = 4.
// SR 16 bit products have AtodWordSize = 2.
// 
// * AtodDataCoding
// 
// Whether the data uses signed or offset format.  Possible values are
// the SRDAT_DATACODING_ defines below.
// 
// * AtodSamplingRate
// 
// Rate at which the analog data was acquired in samples per second.
// 
// * AtodSamplingRateMeasured
// 
// Actual measured sampling rate.  This may differ slightly from the
// theoretical AtodSamplingRate above, and is determined by counting the
// actual number of sampling points per GPS PPS mark on installations
// with a GPS attached.
// 
// * AtodGain
// 
// Amplification factor applied by the circuitry on the A/D board.
// 
// * AtodFilterScale, AtodFilterCoeff
// 
// Filter scale and smoothing coefficients for the SR DSPA64/HLF
// product.  Will be set to 1.0 and 0.0 respectively for no filtering.
// See the DSPA64/HLF documentation for more information.
// 
// 
// 
// 
// 64-BIT COUNTER FIELDS:
// 
// Most PCs have a high speed 64 bit counter on their motherboard which
// the SR GPS system uses as part of its time recording system.  The
// folowing two fields characterize this counter.  See the description
// of the DAT record types below for more information on how the counter
// is used and recorded.
// 
// 
// * CounterType
// 
// This indicates the OS specific format used for the high-speed 64-bit
// counter for the system on which the DAT file was recorded.  Use the
// SRDAT_COUNTER_ defines below.  Counter is used to maintain a count
// of ticks since the system started.  This counter is not affected if
// the system time is changed by the user or NTP.
// 
// For both Windows and Linux, this counter is a standard 64 bit integer.
// 
// * CounterFreq
// 
// CounterFreq indicates the rate in counts/second at which the 64 bit
// Counter advances.  This value depends on processor speed.
// 
// 
// * CounterFreqHi
// 
// This is the high word of CounterFreq.  It is zero unless the counter
// frequency requires a 64-bit value.
// 
// * Counter2Type
// 
// This indicates the OS specific format used for the high-speed 64-bit
// counter for the system on which the DAT file was recorded.  Use the
// SRDAT_COUNTER_ defines below.  Counter2 is used to maintain a count of
// absolute system time.  This counter changes if the system time is
// changed by the user or NTP.
// 
// For both Windows and Linux, this counter is converted to a standard
// 64 bit integer and where the absolute system time is measured from
// Jan 1, 1970.
// 
// 
// * Counter2Freq
// 
// Counter2Freq indicates the rate in counts/second at which the 64 bit
// Counter2 advances.
// 
// Under Windows, this value is 10000000 which means each count corresponds
// to a 100 nanosecond interval.  Be aware however, that the system time is
// only updated every 10 MILLIseconds (or 100000 counts)!
// 
// Under Linux, this value is always 1000000 counts/second, indicating the
// microsecond part of the 64 bit count structure advances at one microsecond
// per count.
// 
// * Counter2FreqHi
// 
// This is the high word of Counter2Freq.  It is zero unless the counter
// frequency requires a 64-bit value.
// 
// 
// LOCATION PARAMETERS:
// 
// * LocationLatitude
// * LocationLongitude
// * LocationAltitude
// 
// If a GPS system is attached to the SR equipment, then these are the
// corresponding location parameters.  See your GPS documentation for
// more information.
// 
// * LocationSource
// 
// An indicator of how the locations was determined.  Possible values
// are the SRDAT_SOURCE_ defines below.  Normally the location will
// have been determined by the GPS system, but other methods are
// possible.
// 
// 
// TRIGGER PARAMETERS:
// 
// * TriggerType
// 
// This indicates the type of triggering in effect when the data was
// acquired.  Possible values are given in the SRDAT_TRIGGER_ defines
// below.  Each trigger type is further specified by some additional
// parameters.  The meaning of these parameters depends on which
// trigger type is selected.
// 
// * TriggerParm1
// 
// This is the first trigger parameter.  For amplitude triggering,
// it represents the threshold value.  For digital level and digital
// edge triggering, it represents the digital input line to be
// checked.
// 
// * TriggerParm2
// 
// This is the second trigger parameter.  For amplitude triggering,
// it is interpreted as a bit field specifying which A/D channels
// are checked to determine if a trigger has occured.  A 1 bit
// means a channel is used.
// 
// For digital level triggering it indicates the active level
// (eg high or low) which must be matched to declare a trigger.
// Use the SRDAT_TRIGLEVEL_ defines below.
// 
// For digital edge triggering, it indicates the active edge
// (eg rising or falling) which must be matched to declare a
// trigger.  Use the SRDAT_TRIGEDGE_ defines below.
// 
// 
// 
// UNUSED PARAMETERS:
// 
// * LocationUnused1;
// * DigitalUnused1;
// * TriggerUnused1;
// 
// Fields for possible future use and 8 byte alignment.  For Windows and
// Linux compatibility it is important to have doubles aligned on 8 byte
// boundaries.  Unused fields are used as needed in the header structure
// to force such alignment.
// 




// HEADER STRUCTURE AND DEFINES:
//
// Note: If revising the header structure, be sure to
//
// 1) Change the Rev field.
// 2) Keep all data in 4 bytes multiples so Linux stays aligned (ie no
//    shorts).
// 3) Keep an even number of 4 byte items (ie 8 byte alignment) so the
//    doubles don't need alignment padding in Windows.
// 4) Do not let the total size exceed 4096.
//
//
// SRDAT_REV 105   01/30/03  Original version
// SRDAT_REV 106   01/20/06  Added TAGID_GPSPCTIME, AtodModel, GpsModel,
//                 ChannelObcIndex, FileCreateTimeHi, Counter2xxx, TriggerType,
//                 TriggerParmX, FileNameType and FileNameInc header fields.
//                 Some replaced previously unused areas, others added to the
//                 end of the structure.

#if defined( SROS_MSDOS)
#define SRDAT_HDR_ID                    (0x52535253L)    // = "SRSR"
#else
#define SRDAT_HDR_ID                    ((long)('RSRS')) // = "SRSR"
#endif

#define SRDAT_REV                       106

#define SRDAT_INVALID_LONG              -12345
#define SRDAT_INVALID_DOUBLE            ((double)(-12345.0))

// See SrDatTimeTypeString function for matching strings
#define SRDAT_TIME_TYPE_NONE            0       // none
#define SRDAT_TIME_TYPE_FIRSTPT         1       // first point in file
#define SRDAT_TIME_TYPE_LASTPT          2       // last  point in file
#define SRDAT_TIME_TYPE_FIRSTGPS        3       // first GPS marked point
#define SRDAT_TIME_TYPE_LASTGPS         4       // last  GPS marked point
#define SRDAT_TIME_TYPE_FIRSTEVENT      5       // first event occurred
#define SRDAT_TIME_TYPE_LASTEVENT       6       // last  event occurred
#define SRDAT_TIME_TYPE_USER1           7       // first user defined meaning
#define SRDAT_TIME_TYPE_USER2           8       // next  user defined meaning
#define SRDAT_TIME_TYPE_MAX             9       // maximum time type value

// See SrDatSourceString function for matching strings
#define SRDAT_SOURCE_NONE               0       // none
#define SRDAT_SOURCE_CALC               1       // calculated
#define SRDAT_SOURCE_PC                 2       // from the PC clock
#define SRDAT_SOURCE_GPS                3       // from GPS
#define SRDAT_SOURCE_USER               4       // set by the user
#define SRDAT_SOURCE_ACQ                5       // set by acquisition program
#define SRDAT_SOURCE_OBC                6       // from GPS + on board counter
#define SRDAT_SOURCE_MAX                7       // maximum source value

// See SrDatDataCodingString function for matching strings
#define SRDAT_DATACODING_OFFSET         0
#define SRDAT_DATACODING_SIGNED         1
#define SRDAT_DATACODING_MAX            2

//WCT - these SR_COUNTER values are currently defined in SrDefines
//      they are needed in the largeint type functions in SrHelper.c
//      should they be defined here ?
//      should the largeint functions be moved to here ?

// The default PC counter type is now always the standard 64 bit int
// (after 02/15/2006 changes in the PARGPS driver it no longer depends on OS)

// See SrDatCounterTypeString function for matching strings
#define SRDAT_COUNTERTYPE_UNKNOWN          SR_COUNTER_UNKNOWN       // -1
#define SRDAT_COUNTERTYPE_INT64            SR_COUNTER_INT64         // 0 Std 64 bit int
#define SRDAT_COUNTERTYPE_HIGH32LOW32      SR_COUNTER_HIGH32LOW32   // 1 Linux timeval struct
#define SRDAT_COUNTERTYPE_OTHER            SR_COUNTER_OTHER         // 2
#define SRDAT_COUNTERTYPE_MAX              SR_COUNTER_MAX           // 3


// See SrDatAtodModelString function for returning matching strings
#define SRDAT_ATODMODEL_UNKNOWN         0
#define SRDAT_ATODMODEL_PAR1CH          1       // (0x01) These should agree with the
#define SRDAT_ATODMODEL_PAR4CH          4       // (0x04) PARXCH_MODEL_PARxxx defines
#define SRDAT_ATODMODEL_PAR8CH          8       // (0x08) in parxch.h
#define SRDAT_ATODMODEL_USB1CH         17       // (0x11) not designed yet
#define SRDAT_ATODMODEL_USB4CH         20       // (0x14)
#define SRDAT_ATODMODEL_USB8CH         24       // (0x18) not designed yet

#define SRDAT_ATODNAME_UNKNOWN         "ATOD UNKNOWN"
#define SRDAT_ATODNAME_PAR1CH          "PAR1CH"
#define SRDAT_ATODNAME_PAR4CH          "PAR4CH"
#define SRDAT_ATODNAME_PAR8CH          "PAR8CH"
#define SRDAT_ATODNAME_USB1CH          "USB1CH"
#define SRDAT_ATODNAME_USB4CH          "USB4CH"
#define SRDAT_ATODNAME_USB8CH          "USB8CH"

// See SrDatGpsModelString function for matching strings
#define SRDAT_GPSMODEL_UNKNOWN         -1       // Must match GPSMODEL defines in pargps.h
#define SRDAT_GPSMODEL_NONE             0       // No status packets with timing info 
#define SRDAT_GPSMODEL_TRIMBLE          1       // Trimble Ace III (used with SR PARGPS Rev C)
#define SRDAT_GPSMODEL_ONCORE           2       // Motorola Oncore GT+
#define SRDAT_GPSMODEL_PCTIME           3       // time taken from PC
#define SRDAT_GPSMODEL_GARMIN           4       // Garmin GPS 18/18x LVC or 16x HVS (used with SR PARGPS Rev D)
#define SRDAT_GPSMODEL_TCXO             5       // similar to PCTIME, but use sampling rate to compute time
#define SRDAT_GPSMODEL_MAX              6

#define SRDAT_GPSNAME_UNKNOWN           "GPS UNKNOWN"
#define SRDAT_GPSNAME_NONE              "NO GPS"
#define SRDAT_GPSNAME_TRIMBLE           "TRIMBLE"
#define SRDAT_GPSNAME_ONCORE            "ONCORE"
#define SRDAT_GPSNAME_PCTIME            "PCTIME"
#define SRDAT_GPSNAME_GARMIN            "GARMIN"
#define SRDAT_GPSNAME_TCXO              "TCXO"
#define SRDAT_GPSNAME_MAX               "GPS UNKNOWN"

// See SrDatFileNameTypeString function for matching strings
#define SRDAT_FILENAMETYPE_UNKNOWN     -1
#define SRDAT_FILENAMETYPE_NONE         0
#define SRDAT_FILENAMETYPE_SINGLE       1
#define SRDAT_FILENAMETYPE_SEQDECIMAL   2 // Not used for USBxCH
#define SRDAT_FILENAMETYPE_SEQHEX       3 // Not used for USBxCH
#define SRDAT_FILENAMETYPE_TIME         4 // Not used for USBxCH
#define SRDAT_FILENAMETYPE_STDOUT       5 // Not used for USBxCH
#define SRDAT_FILENAMETYPE_YMDHMS       6
#define SRDAT_FILENAMETYPE_MAX          7

// See SrDatTriggerTypeString function for matching strings
#define SRDAT_TRIGGERTYPE_UNKNOWN          -1
#define SRDAT_TRIGGERTYPE_NONE              0
#define SRDAT_TRIGGERTYPE_THRESHOLD         1
#define SRDAT_TRIGGERTYPE_DIGLEVEL          2
#define SRDAT_TRIGGERTYPE_DIGEDGE           3
#define SRDAT_TRIGGERTYPE_MAX               4

// See SrDatTriggerLevelString function for matching strings
#define SRDAT_TRIGGERLEVEL_LOW             0
#define SRDAT_TRIGGERLEVEL_HIGH            1
#define SRDAT_TRIGGERLEVEL_MAX             2

// See SrDatTriggerEdgeString function for matching strings
#define SRDAT_TRIGGEREDGE_FALLING          0
#define SRDAT_TRIGGEREDGE_RISING           1
#define SRDAT_TRIGGEREDGE_MAX              2

#define SRDAT_INDEXCHANNEL_NONE        -1       // MARKCHANNEL_NONE now obsolete
#define SRDAT_MARKCHANNEL_NONE         SRDAT_INDEXCHANNEL_NONE




// Define array dimensions

#define SRDAT_DIM_CHANNELS             65   // Allow for 64ch + GPS PPS (mark)
#define SRDAT_DIM_DIGITAL               8   // Allow for 8 digital channels
#define SRDAT_DIM_TITLE                12
#define SRDAT_DIM_STRING               32
#define SRDAT_DIM_TIME                  4



struct SrDatChannelData {
        long Channel;
        char Title[ SRDAT_DIM_TITLE ];
        };


typedef struct SrDatTimeData {
        long Type;
        long SampleFromRun;
        long SampleFromFile;
        long GpsMarkIndex;
        long Year;
        long Month;
        long Day;
        long YMDSource;
        long Hour;
        long Minute;
        long Second;
        long Microsecond;
        long HMSSource;
        long Unused1;
        } SRDAT_TIMEPT;


typedef struct SrDatHdrLayout {


        // DAT Id, rev, source

        long DatId;             // = "SRSR"
        long DatRev;
        char DatProduct[ SRDAT_DIM_STRING ];
        char DatApplication[ SRDAT_DIM_STRING ];

        long DatAtodModel;      // changed from DatUnused1 in Rev 106
        long DatGpsModel;       // changed from DatUnused2 in Rev 106


        // File name parameters

        char FileName[ SRDAT_DIM_STRING ];
        long FileCreateTime;
        long FileSeqNum;
        long FileStartPtNum;
        long FileCreateTimeHi;  // changed from FileUnused1 in Rev 106


        // Date and Time

        struct SrDatTimeData TimePoint[ SRDAT_DIM_TIME ];


        // Channel layout

        long Channels;
        long ChannelMarkIndex;
        long ChannelPtsPerRecord;
        long ChannelPtsPerFile;

        struct SrDatChannelData ChannelList[ SRDAT_DIM_CHANNELS ];


        // Analog converter parameters

        long AtodWordSize;
        long AtodDataCoding;
        double AtodSamplingRate;
        double AtodSamplingRateMeasured;
        double AtodGain;
        double AtodFilterCoeff;
        double AtodFilterScale;


        // 64-bit counter for PPS data

        long CounterType;
        long CounterFreq;


        // GPS location

        double LocationLatitude;
        double LocationLongitude;
        double LocationAltitude;
        long LocationSource;
        long LocationUnused1;   // align to 8 byte boundary ...


        // 64-bit counter for PCTIME data

        long Counter2Type;      // Added in Rev 106
        long Counter2Freq;      // Added in Rev 106
        long Counter2FreqHi;    // Added in Rev 106


        // High part of PPS data counter frequency

        long CounterFreqHi;     // Added in Rev 106


        // Additional Channel info

        long ChannelsAnalog;    // Added in Rev 106
        long ChannelsDigital;   // Added in Rev 106
        long ChannelsGps;       // Added in Rev 106
        long ChannelsCounter;   // Added in Rev 106

        long ChannelDigIndex;   // Added in Rev 106
        long ChannelObcIndex;   // Added in Rev 106

        long DigitalPerChan;    // Added in Rev 106
        long DigitalUnused1;    // Added in Rev 106 to maintain 8 byte alignment

        struct SrDatChannelData DigitalList[ SRDAT_DIM_DIGITAL ]; // Added in Rev 106


        // Trigger info

        long TriggerType;      // Added in Rev 106
        long TriggerParm1;     // Added in Rev 106
        long TriggerParm2;     // Added in Rev 106
        long TriggerUnused1;   // Added in Rev 106


        // File naming info

        long FileNameType;     // Added in Rev 106
        long FileNameInc;      // Added in Rev 106

        } SRDAT_HDR;





// HEADER PADDING ARRAY:
//
// The DAT file header area is defined to be 4096 bytes long.  Since the
// header layout structure above may not fill the entire 4096 bytes, a
// padding array is used to make up the difference.  The padding array
// itself is defined in srdat.c.
//
//

#define SRDAT_HDR_TOTALSIZE    4096
#define SRDAT_HDR_LAYOUTSIZE   sizeof(SRDAT_HDR)
#define SRDAT_HDR_PADDINGSIZE  (signed int)(SRDAT_HDR_TOTALSIZE-SRDAT_HDR_LAYOUTSIZE)

extern char SrDatHdrPadding[ SRDAT_HDR_PADDINGSIZE ];



// Use these defines to select which group of header time variables
// to set in the SrDatHdrSetDate and SrDatHdrSetTime functions


int SrDatHdrInit( SRDAT_HDR *Hdr );

int SrDatHdrSetFileParm(

                    SRDAT_HDR *Hdr,
                    char *FileName,
                    long FileCreateTime,
                    long FileSeqNum,
                    long FileStartPtNum

                   );

int SrDatHdrSetTimeType(

                    SRDAT_HDR *Hdr,
                    int WhichTime,
                    int Type,
                    int SampleFromRun,
                    int SampleFromFile,
                    int GpsMarkIndex

                    );

int SrDatHdrSetTimeYMD(

                    SRDAT_HDR *Hdr,
                    int WhichTime,
                    int Year,
                    int Month,
                    int Day,
                    int Source

                   );

int SrDatHdrSetTimeHMS(

                    SRDAT_HDR *Hdr,
                    int WhichTime,
                    int hh,
                    int mm,
                    int ss,
                    long usec,
                    int Source

                   );

int SrDatHdrSetLocation(

                    SRDAT_HDR *Hdr,
                    long Latitude,
                    long Longitude,
                    long Altitude,
                    int Source

                   );

int SrDatHdrValidate( SRDAT_HDR *Hdr );
int SrDatHdrUpdateRev( SRDAT_HDR *Hdr );

int SrDatHdrWrite( FILE *fp, SRDAT_HDR *Hdr );
int SrDatHdrRead( FILE *fp, SRDAT_HDR *Hdr );
int SrDatHdrSkip( FILE *fp );


FUNCTYPE( int ) SrDatAtodModelIndex( char *AtodName, int *AtodModel );
FUNCTYPE( int ) SrDatGpsModelIndex( char *GpsName, int *GpsModel );

FUNCTYPE( char *) SrDatAtodModelString( long AtodModel );
FUNCTYPE( char *) SrDatGpsModelString( long GpsModel );
FUNCTYPE( char *) SrDatTimeTypeString( long Type );
FUNCTYPE( char *) SrDatSourceString( long Source );
FUNCTYPE( char *) SrDatDataCodingString( long DataCoding );
FUNCTYPE( char *) SrDatCounterTypeString( long Type );
FUNCTYPE( char *) SrDatFileNameTypeString( long FileNameType );
FUNCTYPE( char *) SrDatTriggerTypeString( long TriggerType );
FUNCTYPE( char *) SrDatTriggerLevelString( long TriggerLevel );
FUNCTYPE( char *) SrDatTriggerEdgeString( long TriggerEdge );
FUNCTYPE( char *) SrDatTimeMethodString( long TimeMethod );


//------------------------------------------------------------------------------
// DAT Record tag + format

//*************************** SrDat Record Tag *************************
//
// The DAT file format is comprised of a header followed by records.
// Records following the header are comprised of a record tag structure
// followed by the record information.  Records can have information
// about items such as analog data or GPS timing information, etc.  The
// record tag structure has an identifier indicating the type of record
// information following the tag structure.  See the comments below for
// more information.
//
// 
// RECORD TAG STRUCTURE AND DEFINES:
// 
// A record tag structure includes TagId and Nbytes indentifing the type
// and size of record information that follows the tag structure.
// 
// Besides analog data, many users will be interested in the format of
// the GPS timing information.  The layout of the GPS data in the
// records reflects the functioning of the underlying hardware.  And is
// as follows:
// 
// GPS systems generally produce two data streams.  One stream is a PPS
// (pulse per second) tick that is very accurately aligned with the GPS
// second.  The second is an RS232 data stream with coarse hours, minutes,
// seconds and location etc.  After starting, the SR GPS system begins
// counting off PPS ticks, 1,2,3,4 ...
// 
// When recording, an additional "GPS mark" channel is added on to the
// analog data streams.  This channel is normally zero except when a PPS
// tick occurs.  It then records the PPS count index of the tick, 1,2,3,4
// ...
// 
// To obtain accurate time information for the analog values at a GPS
// mark index, you must examine both the GPSPPS and GPSSERIAL records.
// Those records will have the accurate high speed 64 bit counts and
// coarse serial information for the corresponding index.  Use the
// CounterType and CounterFrequency values from the DAT header to
// accurately interpolate the sample time with respect to the PPS count.
// Usual accuracy will be better than 100 microseconds and often 1
// microsecond.
// 
// 
// 
// RECORD TAG ID DESCRIPTIONS:
// 
// These tag types indicate the meaning of the data in the record
// information that follows the tag structure.
// 
// 
// * SRDAT_TAGID_ANALOG16
// 
// 16 bit time multiplexed analog data.  The multiplexed data layout is:
// 
//         channel 0, sample 0
//         channel 1, sample 0
//         channel 2, sample 0 ...
//         channel N, sample 0
// 
//         channel 0, sample 1
//         channel 1, sample 1
//         channel 2, sample 1 ...
//         channel N, sample 1
// 
//         channel 0, sample 2
//         channel 1, sample 2 ...
// 
// where channel N may be the "GPS mark" channel if a GPS system is
// attached.
// 
// 
// * SRDAT_TAGID_ANALOG24
// 
// 24 bit time multiplexed analog data.  The samples are packed into 32
// bit longs in the same multiplexed fashion as above.
// 
// 
// * SRDAT_TAGID_GPSPPS - this is obsolete, use SRDAT_TAGID_GPSPPST instead
// 
// A GPS mark index and two high speed 64 bit counter values.  The first
// counter value is the 64 bit count at the PPS tick associated with the
// respective index.  The second is the 64 bit count when the analog data
// was recorded.
// 
// 
// * SRDAT_TAGID_GPSPPST
// 
// A GPS mark index and four high speed 64 bit counter values.  The first
// pair records the value of a high speed PC counter that starts counting
// when the systems is booted.  The second pair records the PC system time
// (eg real world time).  For each pair, the first value represents the
// counter value at the PPS tick associated with the respective index.  The
// second is the counter value when the analog data was recorded.  The
// format of the system time value may vary by OS.  NOTE: Windows currently
// updates the system time only every 10ms.
// 
// 
// * SRDAT_TAGID_GPSSERIAL
// 
// A GPS mark index and the NMEA serial messages that correspond to that
// index.  Typically, two NMEA messages will be received.  The GGA message
// contains time, satellite and location information while the ZDA and RMC
// messages include date info.  See the documentation that came with the
// GPS system for more information.
// 
// 
// * SRDAT_TAGID_USBPACKET
// 
// USB packets with 64 bytes of data.  The exact nature of the data must be
// determined by examining it.
// 
// 
// WCT - Review text for correctness and completeness
// 
// * USBXCH PACKET STRUCTURES
// 
// The USBxCH 8051 firmware generates 64 byte packets of data that it sends
// back to the PC over the USB cable.  It prepares two different types of
// packets.  One contains analog data which arrives at the data sampling
// rate and the other contains character data which arrives at the GPS PPS
// one second rate.  Each packet has some bits in the second byte which
// specify which type of data the packet contains.
// 
// The analog data is packed two samples per packet with each sample using
// 32 bytes.  The first 8 bytes contain special information, while the last
// 24 bytes contain the multiplexed bits for 8 channels.  In other words,
// the first byte has 8 bits, one bit for each of 8 channels.  For each
// channel, its bit from the first byte is the most significant of the 24
// bits for that analog sample for that channel.  Once demuxed, the analog
// sample for each channel is put into a full 32 bit word so it can be
// used as a standard int.
// 
// The 8 special bytes are Sanity, PpsToggle, DigitalIn, GGACount, and
// four bytes of OnBoardCount with the most significant byte first.
// Sanity is a sequential count that goes from 0 to 15 and then wraps
// around to 0 again.  The PpsToggle byte contains both the packet type
// information and a bit that toggles each time a PPS signal is received.
// This signal mirrors the behavior of the red LED when it is controlled
// by PPS.  DigitalIn contains one bit representing the stat of each of
// the four digital input lines.  GGACount is a sequential count going
// from 0 to 15 and then wrapping which increments each time a $GPGGA NMEA
// string is received.  This count number is shared between the 8051 and
// the CoolRunner and helps to tie the GPS and analog data together.
// 
// The character data contains GPS NMEA strings and assorted equipment data
// such as temperature and a voltage good indicator.  It starts with a 4
// byte header containing SerialSanity, DataType, ReadCount, and ZeroCount.
// SerialSanity is a sequential count of serial packets going from 0 to 15
// and wrapping.  It is used to ensure transmission errors resulting in
// missing serial packets can be detected.  DataType identifies this
// packet as containing character data.  ReadCount and ZeroCount give the
// number of valid and empty characters remaining in the packet
// respectively.  Their sum should add up to 60.
// 
// Following the 4 byte serial packet header are 60 remaining bytes which
// contain the characters of a serial NMEA string followed by the GGACount
// and some equipment data.  The equipment data starts with the number of
// equipment bytes to follow.  Currently this is 3.  These 3 bytes contain
// the voltage good signal, and the high and low temperature bytes
// respectively.
// 
// 
// * SRDAT_TAGID_USBANALOG
// 
// USBxCH A/D board analog data extracted from a 64 byte USB packet into a
// SRDAT_USBXCH_ANALOG structure.  Each structure contains one data point
// and has such as sanity, data type/pps toggle, digital input, GGA count,
// and OBC.  Plus 24 bytes of analog data values for each of 8 channels.
// For the USB4CH, the last 4 channels are all zero.
// 
// 
// * SRDAT_TAGID_USBSERIAL
// 
// USBxCH A/D board serial NMEA data extracted from one or more 64 byte USB
// packets into a SRDAT_USBXCH_SERIAL structure.  This contains the same
// info as the GPS_SERIAL tag for the PARxCH/PARGPS series.  Typically each
// structure holds two NMEA strings for a given second.  The NMEA
// characters are the standard $GPGGA style strings.  Typically RMC and
// GGA.
// 
// 
// * SRDAT_TAGID_USBEQUIP
// 
// USBxCH A/D board equipment data extracted from one or more 64 byte USB
// packets into a SRDAT_USBXCH_EQUIPMENT structure.  This contains current
// temperature and voltage status information.  In the original USB
// packets, the equipment data is included after the end of each NMEA
// string.
// 
// 
// * SRDAT_TAGID_EOF
// 
// Used to indicate the end of a DAT file.
// 
//

#if defined( SROS_MSDOS)
#define SRDAT_TAGID_ANALOG16    (0x36314154L)      // = "TA16"
#define SRDAT_TAGID_ANALOG24    (0x34324154L)      // = "TA24"
#define SRDAT_TAGID_GPSPPS      (0x53505054L)      // = "TPPS"
#define SRDAT_TAGID_GPSPPST     (0x54505054L)      // = "TPPT"
#define SRDAT_TAGID_GPSSERIAL   (0x52455354L)      // = "TSER"
#define SRDAT_TAGID_USBPACKET   (0x42535554L)      // = "TUSB"
#define SRDAT_TAGID_USBANALOG   (0x41425554L)      // = "TUBA"
#define SRDAT_TAGID_USBSERIAL   (0x53425554L)      // = "TUBS"
#define SRDAT_TAGID_USBEQUIP    (0x45425554L)      // = "TUBE"
#define SRDAT_TAGID_EOF         (0x464F4554L)      // = "TEOF"
#else
#define SRDAT_TAGID_ANALOG16    ((long)('61AT'))   // = "TA16"
#define SRDAT_TAGID_ANALOG24    ((long)('42AT'))   // = "TA24"
#define SRDAT_TAGID_GPSPPS      ((long)('SPPT'))   // = "TPPS"
#define SRDAT_TAGID_GPSPPST     ((long)('TPPT'))   // = "TPPT"
#define SRDAT_TAGID_GPSSERIAL   ((long)('REST'))   // = "TSER"
#define SRDAT_TAGID_USBPACKET   ((long)('BSUT'))   // = "TUSB"
#define SRDAT_TAGID_USBANALOG   ((long)('ABUT'))   // = "TUBA"
#define SRDAT_TAGID_USBSERIAL   ((long)('SBUT'))   // = "TUBS"
#define SRDAT_TAGID_USBEQUIP    ((long)('EBUT'))   // = "TUBE"
#define SRDAT_TAGID_EOF         ((long)('FOET'))   // = "TEOF"
#endif


typedef struct RecordTag {

        unsigned long TagId;
        unsigned long Nbytes;
        unsigned long Unused1;
        unsigned long Unused2;

        } SRDAT_TAG;


#define SRDAT_TAGSIZE   sizeof( SRDAT_TAG )



// Complete Record Structure
// 
// In order to speed up disk accesses, the record tag and the record data
// need to be written all at once instead of in two separate transactions.
// The easiest way to do this is to allocate the tag and data so they are
// contiguous.
// 
// One problem is that different records types may have different amounts
// of data.  So, there isn't really any logical maximum we can allocate to
// be sure of covering all types.  Instead, we use a single character
// variable as a place holder to the start of the data and rely on function
// calls for most interactions with the record structure.  These include
// allocating, initializing, and determining the size of a complete record.
// 


typedef struct RecordComplete {

        SRDAT_TAG tag;
        char      data;

        } SRDAT_RECORD;



int SrDatRecordAllocate( long DataType, unsigned int Nbytes, SRDAT_RECORD **PtrRecord );
int SrDatRecordFree( SRDAT_RECORD **PtrRecord );
int SrDatRecordPtrTag( SRDAT_RECORD *Record, SRDAT_TAG **PtrTag );
int SrDatRecordPtrData( SRDAT_RECORD *Record, char **PtrData );
int SrDatRecordWriteFast( FILE *fp, SRDAT_RECORD *Record );

int SrDatRecordWrite( FILE *fp, long DataType, void *Buffer, unsigned int Nbytes );
int SrDatRecordRead( FILE *fp, SRDAT_TAG *Tag, void *Buffer );

int SrDatTagRead( FILE *fp, SRDAT_TAG *Tag );

int SrDatDataRead( FILE *fp, void *Buffer, unsigned int Nbytes );
int SrDatDataSkip( FILE *fp, unsigned int Nbytes );

int SrDatDataReadUsbAnalog( FILE *fp, void *Buffer, unsigned int Nbytes, unsigned int *Npts );
int SrDatDataReadUsbSerial( FILE *fp, void *Buffer, unsigned int Nbytes, unsigned int *Npts );
int SrDatDataReadUsbEquip( FILE *fp, void *Buffer, unsigned int Nbytes, unsigned int *Npts );



//------------------------------------------------------------------------------
// Error defines

//WCT - Review error messages and ensure they are all unique

// Error code return values and their associated message strings
// These are combined together in the USBXCH_ERROR_MSG[] structure
// in SrUsbXch.c ...

#define USBXCH_ERROR_NONE                           0
#define USBXCH_ERROR_OPEN_DRIVER_NAME               1
#define USBXCH_ERROR_OPEN_SAMPLE_RATE               2
#define USBXCH_ERROR_OPEN_ATOD_UNKNOWN              3
#define USBXCH_ERROR_OPEN_GPS_UNKNOWN               4
#define USBXCH_ERROR_OPEN_BOARD_RESET               5
#define USBXCH_ERROR_OPEN_DOWNLOAD                  6
#define USBXCH_ERROR_OPEN_START_8051                7
#define USBXCH_ERROR_OPEN_POWER                     8
#define USBXCH_ERROR_OPEN_SUCCESS_PORT_RESET        9
#define USBXCH_ERROR_OPEN_SUCCESS_PORT_CYCLE       10
#define USBXCH_ERROR_OPEN_SUCCESS_BOARD_RESET      11
#define USBXCH_ERROR_START_POWER                   12
#define USBXCH_ERROR_START_COMMAND                 13
#define USBXCH_ERROR_STOP_COMMAND                  14
#define USBXCH_ERROR_EVEN_SAMPLES                  15
#define USBXCH_ERROR_INVALID_PARAM                 16
#define USBXCH_ERROR_INVALID_PACKET                17
#define USBXCH_ERROR_BAD_NMEA                      18
#define USBXCH_ERROR_CONFIGURE                     19
#define USBXCH_ERROR_SPS_INVALID                   20
#define USBXCH_ERROR_DECIMATION                    21
#define USBXCH_ERROR_DRIVER_NOT_OPEN               22
#define USBXCH_ERROR_DRIVER_REQUEST_FAILED         23
#define USBXCH_ERROR_DRAM_READ_COMMAND             24
#define USBXCH_ERROR_DRAM_READ_RESULTS             25
#define USBXCH_ERROR_ATOD_RESET                    26
#define USBXCH_ERROR_CALIBRATION                   27
#define USBXCH_ERROR_OVERFLOW                      28
#define USBXCH_ERROR_KEYPRESS                      29
#define USBXCH_ERROR_SANITY_DATA                   30
#define USBXCH_ERROR_SANITY_STATUS                 31
#define USBXCH_ERROR_DIGITAL_NOT_AVAILABLE         32
#define USBXCH_ERROR_FAILED_SEND                   33
#define USBXCH_ERROR_NO_SPACE_AVAILABLE            34
#define USBXCH_ERROR_READ_NO_DATA                  35
#define USBXCH_ERROR_READ_NOT_ENOUGH_DATA          36
#define USBXCH_ERROR_READ_TOO_LARGE                37
#define USBXCH_ERROR_READ_RESULTS_FAILED           38
#define USBXCH_ERROR_WRITE_TOO_LARGE               39
#define USBXCH_ERROR_WRITE_FAILED                  40
#define USBXCH_ERROR_EEPROM_EXCEEDED               41
#define USBXCH_ERROR_EEPROM_PROTECTED              42
#define USBXCH_ERROR_SERIALBUFFER_FULL             43
#define USBXCH_ERROR_GPS_STATUS_CHANGE             44
#define USBXCH_ERROR_CONVERT_PAK_BAD_INPUT         45
#define USBXCH_ERROR_CONVERT_PAK_BAD_OPT_INPUT     46
#define USBXCH_ERROR_CONVERT_PAK_ARRAY_FULL        47
#define USBXCH_ERROR_CONVERT_PAK_TYPE_UNKNOWN      48
#define USBXCH_ERROR_TS_FILL_SAMPLE_BAD_INPUT      49
#define USBXCH_ERROR_TS_FILL_SAMPLE_BAD_TS         50
#define USBXCH_ERROR_TS_FILL_SAMPLE_BAD_PKT        51
#define USBXCH_ERROR_TS_FILL_SAMPLE_TS_FULL        52
#define USBXCH_ERROR_TS_FILL_SAMPLE_STATUS_FULL    53
#define USBXCH_ERROR_TS_FILL_STATUS_BAD_INPUT      54
#define USBXCH_ERROR_TS_FILL_STATUS_BAD_TS         55
#define USBXCH_ERROR_TS_FILL_STATUS_TS_FULL        56
#define USBXCH_ERROR_TS_FILL_STATUS_STATUS_FULL    57
#define USBXCH_ERROR_TS_FILL_STATUS_SERIAL_FULL    58
#define USBXCH_ERROR_TS_FILL_STATUS_NMEA_FULL      59
#define USBXCH_ERROR_TS_FILL_STATUS_BAD_NMEA       60
#define USBXCH_ERROR_TS_VALID_OUT_OF_RANGE         61
#define USBXCH_ERROR_TS_VALID_NO                   62
#define USBXCH_ERROR_PROCESS_PKT_BAD_INPUT         63
#define USBXCH_ERROR_PROCESS_PKT_OUT_OF_RANGE      64
#define USBXCH_ERROR_PROCESS_PKT_ARRAY_FULL        65
#define USBXCH_ERROR_OUTPUT_COL_BAD_INPUT          66
#define USBXCH_ERROR_OUTPUT_COL_NO_MORE_STATUS     67
#define USBXCH_ERROR_TSCOMBINE_NO_MORE_SAMPLE      68
#define USBXCH_ERROR_TSCOMBINE_NO_MORE_STATUS      69
#define USBXCH_ERROR_TSCOMBINE_TS_FULL             70
#define USBXCH_ERROR_TSCOMBINE_TS_BAD_ALIGN        71
#define USBXCH_ERROR_SERIALPARSE_NMEA_FULL         72
#define USBXCH_ERROR_BAD_GPS_MODEL                 73
#define USBXCH_ERROR_MAX                           74



#define USBXCH_ERROR_MSG_NONE                       "NONE"
#define USBXCH_ERROR_MSG_OPEN_DRIVER_NAME           "INVALID DRIVER NAME"
#define USBXCH_ERROR_MSG_OPEN_SAMPLE_RATE           "INVALID SAMPLE RATE"
#define USBXCH_ERROR_MSG_OPEN_ATOD_UNKNOWN          "ATOD NOT USB1CH,4CH,8CH"
#define USBXCH_ERROR_MSG_OPEN_GPS_UNKNOWN           "GPS NOT NONE,PCTIME,GARMIN,TRIMBLE,ONCORE"
#define USBXCH_ERROR_MSG_OPEN_BOARD_RESET           "CAN'T OPEN DRIVER AFTER USBXCH BOARD RESET"
#define USBXCH_ERROR_MSG_OPEN_DOWNLOAD              "NO CODE DOWNLOAD TO USBXCH"
#define USBXCH_ERROR_MSG_OPEN_START_8051            "CAN'T START FIRMWARE"
#define USBXCH_ERROR_MSG_OPEN_POWER                 "POWER IS TOO LOW FOR OPEN"
#define USBXCH_ERROR_MSG_OPEN_SUCCESS_PORT_RESET    "OPEN SUCCEEDED ONLY AFTER USB PORT RESET"
#define USBXCH_ERROR_MSG_OPEN_SUCCESS_PORT_CYCLE    "OPEN SUCCEEDED ONLY AFTER USB PORT POWER CYCLE"
#define USBXCH_ERROR_MSG_OPEN_SUCCESS_BOARD_RESET   "OPEN SUCCEEDED ONLY AFTER USBXCH BOARD RESET"
#define USBXCH_ERROR_MSG_START_POWER                "POWER IS TOO LOW FOR START"
#define USBXCH_ERROR_MSG_START_COMMAND              "START COMMAND NOT SENT"
#define USBXCH_ERROR_MSG_STOP_COMMAND               "STOP COMMAND NOT SENT"
#define USBXCH_ERROR_MSG_EVEN_SAMPLES               "EVEN NUM OF SAMPLES REQUIRED"
#define USBXCH_ERROR_MSG_INVALID_PARAM              "INVALID PARAMETER PASSED"
#define USBXCH_ERROR_MSG_INVALID_PACKET             "INVALID PACKET"
#define USBXCH_ERROR_MSG_BAD_NMEA                   "BAD NMEA STRING"
#define USBXCH_ERROR_MSG_CONFIGURE                  "FAILED TO CONFIGURE USERCFG,GPS"
#define USBXCH_ERROR_MSG_SPS_INVALID                "SPS OUT OF RANGE"
#define USBXCH_ERROR_MSG_DECIMATION                 "INVALID DECIMATION"
#define USBXCH_ERROR_MSG_DRIVER_NOT_OPEN            "NO DEVICE DRIVER"
#define USBXCH_ERROR_MSG_DRIVER_REQUEST_FAILED      "DRIVER REQUEST"
#define USBXCH_ERROR_MSG_DRAM_READ_COMMAND          "DRAM READ COMMAND NOT SENT"
#define USBXCH_ERROR_MSG_DRAM_READ_RESULTS          "DRAM READ NO RESULTS"
#define USBXCH_ERROR_MSG_ATOD_RESET                 "COULD NOT RESET A/D CHIPS"
#define USBXCH_ERROR_MSG_CALIBRATION                "BAD CALIBRATION"
#define USBXCH_ERROR_MSG_OVERFLOW                   "OVERFLOW"
#define USBXCH_ERROR_MSG_KEYPRESS                   "KEYPRESS"
#define USBXCH_ERROR_MSG_SANITY_DATA                "BAD DATA PACKET (ANALOG) SANITY"
#define USBXCH_ERROR_MSG_SANITY_STATUS              "BAD STATUS PACKET (RS232/EQ) SANITY"
#define USBXCH_ERROR_MSG_DIGITAL_NOT_AVAILABLE      "DIGITAL NOT AVAILABLE"
#define USBXCH_ERROR_MSG_FAILED_SEND                "FAILED TO SEND COMMAND"
#define USBXCH_ERROR_MSG_NO_SPACE_AVAILABLE         "NO SPACE AVAILABLE"
#define USBXCH_ERROR_MSG_READ_NO_DATA               "NO DATA RECEIVED ON READ"
#define USBXCH_ERROR_MSG_READ_NOT_ENOUGH_DATA       "READ NEEDS MORE PACKETS"
#define USBXCH_ERROR_MSG_READ_TOO_LARGE             "READ > TEMP BUFFER"
#define USBXCH_ERROR_MSG_READ_RESULTS_FAILED        "READ RESULTS FAILED"
#define USBXCH_ERROR_MSG_WRITE_TOO_LARGE            "WRITE > TEMP BUFFER"
#define USBXCH_ERROR_MSG_WRITE_FAILED               "WRITE FAILED"
#define USBXCH_ERROR_MSG_EEPROM_EXCEEDED            "EEPROM PAGE SIZE < REQUEST"
#define USBXCH_ERROR_MSG_EEPROM_PROTECTED           "EEPROM PAGE 1 PROTECTED"
#define USBXCH_ERROR_MSG_SERIALBUFFER_FULL          "SERIAL BUFFER FULL"
#define USBXCH_ERROR_MSG_GPS_STATUS_CHANGE          "GPS SATELLITE STATUS CHANGED"
#define USBXCH_ERROR_MSG_CONVERT_PAK_BAD_INPUT      "BAD INPUT CONVERTING PAK TO COLUMN"
#define USBXCH_ERROR_MSG_CONVERT_PAK_BAD_OPT_INPUT  "BAD OPTIONAL INPUT CONVERTING PAK TO COLUMN"
#define USBXCH_ERROR_MSG_CONVERT_PAK_ARRAY_FULL     "ARRAY FULL CONVERTING PAK TO COLUMN"
#define USBXCH_ERROR_MSG_CONVERT_PAK_TYPE_UNKNOWN   "CAN'T CONVERT UNKNOWN PACKET TYPE"
#define USBXCH_ERROR_MSG_TS_FILL_SAMPLE_BAD_INPUT   "BAD INPUT FILLING TS FROM SAMPLE PKT"
#define USBXCH_ERROR_MSG_TS_FILL_SAMPLE_BAD_TS      "BAD TIMESTAMP FROM SAMPLE PKT"
#define USBXCH_ERROR_MSG_TS_FILL_SAMPLE_BAD_PKT     "BAD SAMPLE PACKET WITHOUT 2 POINTS"
#define USBXCH_ERROR_MSG_TS_FILL_SAMPLE_TS_FULL     "TS ARRAY FULL FROM SAMPLE PKT"
#define USBXCH_ERROR_MSG_TS_FILL_SAMPLE_STATUS_FULL "STATUS ARRAY FULL FILLING TS FROM SAMPLE PKT"
#define USBXCH_ERROR_MSG_TS_FILL_STATUS_BAD_INPUT   "BAD INPUT FILLING TS FROM STATUS PKT"
#define USBXCH_ERROR_MSG_TS_FILL_STATUS_BAD_TS      "BAD TIMESTAMP FROM STATUS PKT"
#define USBXCH_ERROR_MSG_TS_FILL_STATUS_TS_FULL     "TS ARRAY FULL FROM STATUS PKT"
#define USBXCH_ERROR_MSG_TS_FILL_STATUS_STATUS_FULL "STATUS ARRAY FULL FILLING TS FROM STATUS PKT"
#define USBXCH_ERROR_MSG_TS_FILL_STATUS_SERIAL_FULL "SERIAL BUFFER FULL FILLING TS FROM STATUS PKT"
#define USBXCH_ERROR_MSG_TS_FILL_STATUS_NMEA_FULL   "TOO MANY NMEA MESSAGES OR MISSING FIRST NMEA STRING ($GPRMC)"
#define USBXCH_ERROR_MSG_TS_FILL_STATUS_BAD_NMEA    "BAD NMEA FILLING TS FROM STATUS PKT"
#define USBXCH_ERROR_MSG_TS_VALID_OUT_OF_RANGE      "TS NOT VALID, OUT OF RANGE"
#define USBXCH_ERROR_MSG_TS_VALID_NO                "TS NOT VALID"
#define USBXCH_ERROR_MSG_PROCESS_PKT_BAD_INPUT      "BAD INPUT PROCESSING PACKETS"
#define USBXCH_ERROR_MSG_PROCESS_PKT_OUT_OF_RANGE   "OUT OF RANGE PROCESSING PACKETS"
#define USBXCH_ERROR_MSG_PROCESS_PKT_ARRAY_FULL     "ARRAY FULL PROCESSING PACKETS"
#define USBXCH_ERROR_MSG_OUTPUT_COL_BAD_INPUT       "BAD INPUT WHEN OUTPUTTING COLUMNS"
#define USBXCH_ERROR_MSG_OUTPUT_COL_NO_MORE_STATUS  "NO MATCHING STATUS WHEN OUTPUTTING COLUMNS"
#define USBXCH_ERROR_MSG_TSCOMBINE_NO_MORE_SAMPLE   "NO MORE SAMPLE TIME INFO WHEN COMBINING"
#define USBXCH_ERROR_MSG_TSCOMBINE_NO_MORE_STATUS   "NO MORE STATUS TIME INFO WHEN COMBINING"
#define USBXCH_ERROR_MSG_TSCOMBINE_TS_FULL          "TS BUFFER FULL WHEN COMBINING"
#define USBXCH_ERROR_MSG_TSCOMBINE_TS_BAD_ALIGN     "CAN'T ALIGN SAMPLE AND STATUS TIME INFO"
#define USBXCH_ERROR_MSG_SERIALPARSE_NMEA_FULL      "TOO MANY NMEA MESSAGES, SERIAL PARSE BUFFER FULL"
#define USBXCH_ERROR_MSG_BAD_GPS_MODEL              "OPERATION NOT ALLOWED WITH SELECTED GPS MODEL"
#define USBXCH_ERROR_MSG_MAX                         NULL

extern VARTYPE( char ) *USBXCH_ERROR_MSG[];
extern VARTYPE( char ) *UsbXchErrorMsgDetailDriverNotOpen[];


//------------------------------------------------------------------------------
// Channel counts

// USBxCH number of channels ...

// Several different types of information can be acquired from the
// USBxCH devices.  These include analog, digital, GPS and counter
// channels.  The number of channels for each type of USBxCH, defined
// below, control what data is saved to the output files.  An additional
// set of defines indicate the channel counts for plotting purposes as
// this may be different than for saving.  For example, the 
// digital information may be stored as 1 channel, but plotted as 4
// traces.


// Channel counts for the USB8CH

#define USB8CH_ANALOG_CHANNELS         8
#define USB8CH_DIGITAL_CHANNELS        1
#define USB8CH_GPS_CHANNELS            1
#define USB8CH_COUNTER_CHANNELS        1

#define USB8CH_ANAPLOT_CHANNELS        8
#define USB8CH_DIGPLOT_CHANNELS        4
#define USB8CH_GPSPLOT_CHANNELS        1
#define USB8CH_CNTPLOT_CHANNELS        0

#define USB8CH_MODELNAME               "USB8CH"



// Channel counts for the USB4CH

#define USB4CH_ANALOG_CHANNELS         4
#define USB4CH_DIGITAL_CHANNELS        1
#define USB4CH_GPS_CHANNELS            1
#define USB4CH_COUNTER_CHANNELS        1

#define USB4CH_ANAPLOT_CHANNELS        4
#define USB4CH_DIGPLOT_CHANNELS        4
#define USB4CH_GPSPLOT_CHANNELS        1
#define USB4CH_CNTPLOT_CHANNELS        0

#define USB4CH_MODELNAME               "USB4CH"



// Maximum channel counts for all USBxCH

#define USBXCH_MAX_ANALOG_CHANNELS     8
#define USBXCH_MAX_DIGITAL_CHANNELS    1
#define USBXCH_MAX_GPS_CHANNELS        1
#define USBXCH_MAX_COUNTER_CHANNELS    1

#define USBXCH_MAX_ANAPLOT_CHANNELS    8
#define USBXCH_MAX_DIGPLOT_CHANNELS    8
#define USBXCH_MAX_GPSPLOT_CHANNELS    1
#define USBXCH_MAX_CNTPLOT_CHANNELS    0


#define USBXCH_MODEL               SRDAT_ATODMODEL_UNKNOWN
#define USBXCH_MODELNAME           "USBxCH"




// ASCII Column data structures and functions

struct SrUsbXchSampleRowLayout {
        int PpsCount;
        int CurrentPt;
        long Channel[USBXCH_MAX_ANALOG_CHANNELS];
        unsigned char DigitalIn;
        unsigned char Sanity;
        unsigned char PpsToggle;
        unsigned char GGACount;
        long OnBoardCount;
        double SampleTime;
        } ;
typedef struct SrUsbXchSampleRowLayout SRUSBXCH_SAMPLEDATA;


struct SrUsbXchStatusRowLayout {
        int PpsCount;
        int CurrentPt;
        int PowerInfo;
        int DramFlags;
        long Temperature;
        NI NmeaInfo;
        int NmeaCount;
        char NmeaMsg[SRDAT_NMEA_MAX_BUFF];
        } ;
typedef struct SrUsbXchStatusRowLayout SRUSBXCH_STATUSDATA;


extern char SrUsbXchLastErrorString[];

FUNCTYPE( int ) SrUsbXchSampleDataInit( SRUSBXCH_SAMPLEDATA *SampleData );
FUNCTYPE( int ) SrUsbXchStatusDataInit( SRUSBXCH_STATUSDATA *StatusData );

FUNCTYPE( int ) SrUsbXchPacketsToColumns( int FinalProcess,
                                SRUSBXCH_PACKET *PacketArray, int nPackets,
                                SRUSBXCH_SAMPLEDATA *SampleData, int maxSample, int *nSample,
                                SRUSBXCH_STATUSDATA *StatusData, int maxStatus, int *nStatus, 
                                int *Error );


//------------------------------------------------------------------------------
// Advanced defines and functions

// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY
// EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY EXPERTS ONLY


// ADVANCED USBxCH CONTROL/EXECUTION DEFINES AND FUNCTIONS:
// 
// These defines and functions allow a more detailed level of control
// over USBxCH execution.  They are intended for users who are familiar
// with the ADS1255 command register bit fields and wish to program them
// directly.
// 
// Note that USBxCH user digital IO will not work until the device has
// been initialized with SrUsbXchOpen.
// 


// Minimal open just opens the driver and nothing else.

// Hardware open also prepares the firmware and checks the power.  If
// any of these steps fails, it trys to recover by repeating those steps
// after doing a port reset, a port cycle and/or a board reset.  Unlike
// the standard open, this one does NOT set the user mode configuration
// and sampling rate.  Normal users should call the standard open.  Only
// diagnositic programs should call these other opens.

FUNCTYPE( DEVHANDLE ) SrUsbXchMinimalOpen(
                                   char   *DriverName,
                                   int    *Error
                                   );

FUNCTYPE( DEVHANDLE ) SrUsbXchHardwareOpen(
                                  char *DriverName,
                                  char **HwData,
                                  int *Error
                                  );



// The On Board Counter has a period of 800 ns, which means it
// counts at rate of 1/800ns or 1,250,000 counts per second.

#define USBXCH_XTAL_SPEED         ((double)10.0E+6)    // 10MHz
#define USBXCH_OBC_FREQ           ((double)1250000.0L)


// Input voltage range is +/- 4v differential so the total range is 16v.
// The lowest value occurs when - pin is +4 and the + pin is -4 for a
// difference between the pins of -8v.  The highest value occurs when the
// + pin is +4 and the - pin is -4 for a difference of +8v.  So the total
// full scale voltage range for computing counts/volt is 16v.

#define USBXCH_VOLTAGE_FULL_SCALE_RANGE   16



// ADS1255 command constants

#define ADCMD_NUM       0x00
#define ADCMD_WAKE1     0x00
#define ADCMD_RDATA     0x01
#define ADCMD_RDATAC    0x03
#define ADCMD_RREG      0x10
#define ADCMD_WREG      0x50
#define ADCMD_SELFCAL   0xF0
#define ADCMD_SELFOCAL  0xF1
#define ADCMD_SELFGCAL  0xF2
#define ADCMD_SYSOCAL   0xF3
#define ADCMD_SYSGCAL   0xF4
#define ADCMD_SYNC      0xFC
#define ADCMD_STANDBY   0xFD
#define ADCMD_RESET     0xFE
#define ADCMD_SDATAC    0x0F
#define ADCMD_WAKE2     0xFF

#define ADCMD_SIZE_STD  0x01
#define ADCMD_SIZE_RW   0x02

// ADS1255 register constants
   
#define ADREG_STATUS    0x00
#define ADREG_MUX       0x01
#define ADREG_ADCON     0x02
#define ADREG_DRATE     0x03
#define ADREG_IO        0x04
#define ADREG_OFC0      0x05
#define ADREG_OFC1      0x06
#define ADREG_OFC2      0x07
#define ADREG_FSC0      0x08
#define ADREG_FSC1      0x09
#define ADREG_FSC2      0x0A
#define ADREG_MAX       0x0B


// A/D configuration functions

FUNCTYPE( int ) SrUsbXchAtodReset( DEVHANDLE UsbXchHandle, int *Error );
FUNCTYPE( int ) SrUsbXchAtodWriteReg( DEVHANDLE UsbXchHandle, int FirstReg,
                                    int Nreg, char *RegData, int *Error );
FUNCTYPE( int ) SrUsbXchAtodReadReg( DEVHANDLE UsbXchHandle, int FirstReg,
                                    int Nreg, char *RegData, int *Error );
FUNCTYPE( int ) SrUsbXchAtodCalibrate( DEVHANDLE UsbXchHandle,
                                     int CalibrateType, int *Error );


// Dram functions

#define USBXCH_DRAMFLAGS_UNKNOWN -1

FUNCTYPE( void ) SrUsbXchDramReset( DEVHANDLE UsbXchHandle );
FUNCTYPE( int )  SrUsbXchDramWrite( DEVHANDLE UsbXchHandle, char *Bytes, int Nbytes, int *Error );
FUNCTYPE( int )  SrUsbXchDramRead( DEVHANDLE UsbXchHandle, char *Bytes, int Nbytes, int *Error );
FUNCTYPE( int )  SrUsbXchDramFlags( DEVHANDLE UsbXchHandle, int *Empty, int *PartFull, int *Overflow );

FUNCTYPE( void ) SrUsbXchDramFlagsSplit( int DramFlags, int *Empty, int *PartFull, int *Overflow );


// USB pipe functions

FUNCTYPE( int ) SrUsbXchPipeStatus( DEVHANDLE UsbXchHandle, int PipeNum );
FUNCTYPE( int ) SrUsbXchPipeReset(  DEVHANDLE UsbXchHandle, int PipeNum );
FUNCTYPE( int ) SrUsbXchPipeAbort(  DEVHANDLE UsbXchHandle, int PipeNum );

// USB port functions

FUNCTYPE( int ) SrUsbXchPortReset( DEVHANDLE UsbXchHandle );
FUNCTYPE( int ) SrUsbXchPortCycle( DEVHANDLE UsbXchHandle );


// Helper functions

int SrDatEquation2pt( double x1, double y1,
                       double x2, double y2,
                       double xk, double *yk );
int SrDatEquationSlope( double m,
                         double x1, double y1,
                         double xk, double *yk );



// Misc reset functions: Sanity, PpsEventNum, and power

#define SRDAT_IGNORE_ANALOG     0
#define SRDAT_RESET_ANALOG      1

#define SRDAT_IGNORE_SERIAL     0
#define SRDAT_RESET_SERIAL      1

FUNCTYPE( void ) SrDatSanityReset( int ResetAnalog, int ResetSerial );
FUNCTYPE( void ) SrDatPpsEventNumReset( int ResetAnalog, int ResetSerial );

FUNCTYPE( DEVHANDLE ) SrUsbXchPowerReset( DEVHANDLE UsbXchHandle );

FUNCTYPE( int )  SrUsbXchGetState( DEVHANDLE UsbXchHandle, SRUSBXCH_STATE *UsbState );


FUNCTYPE( int ) SrUsbXchFullPacketsToColumns( int FinalProcess,
                              SRUSBXCH_PACKET *PacketArray, int nPackets,
                              SRUSBXCH_SAMPLEDATA *SampleData, int maxSample, int *nSample,
                              SRUSBXCH_STATUSDATA *StatusData, int maxStatus, int *nStatus, 
                              TS *TsData, int maxTs, int *nTs, int *Error );


// Driver communication functions

FUNCTYPE( unsigned long ) SrUsbXchSendStandardRequest(

                                DEVHANDLE UsbXchHandle,
                                unsigned char Direction,
                                unsigned char Recipient,
                                unsigned char Request,
                                unsigned short Value,
                                unsigned short Index,
                                char *Values,
                                unsigned long Nvalues,
                                int *Error
                                );

FUNCTYPE( unsigned long ) SrUsbXchSendVendorRequest(

                                DEVHANDLE UsbXchHandle,
                                unsigned char Request,
                                unsigned short Value,
                                unsigned short Index,
                                char *Values,
                                unsigned long Nvalues,
                                int *Error
                                );

FUNCTYPE( unsigned int ) SrUsbXchSendCommand(
                                           
                                DEVHANDLE UsbXchHandle,
                                char *Values,
                                unsigned long Nvalues,
                                int *Error
                                );

FUNCTYPE( unsigned int ) SrUsbXchReceiveResults(
                                           
                                DEVHANDLE UsbXchHandle,
                                char *Values,
                                unsigned long Nvalues,
                                int *Error
                                );

FUNCTYPE( int ) SrUsbXchSerialRead( DEVHANDLE UsbXchHandle, char *Bytes, int Nbytes, int *Error );
FUNCTYPE( int ) SrUsbXchSerialWrite( DEVHANDLE UsbXchHandle, char *Bytes, int Nbytes, int *Error );




//------------------------------------------------------------------------------
// Debugging

FUNCTYPE( void ) SrUsbXchLogMessage( char *Msg );
FUNCTYPE( void ) SrUsbXchLogMessageV( char *Msg );
FUNCTYPE( void ) SrUsbXchLogMessageT( char *Msg );


//------------------------------------------------------------------------------

#endif // __USBXCH_H__

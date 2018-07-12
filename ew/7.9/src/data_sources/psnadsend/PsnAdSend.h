// PsnAdSend.h
	
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>

#ifndef WIN32
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
	
extern "C" {
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <kom.h>
}

#ifdef WIN32
#include <process.h>
#include <conio.h>
#endif

#define VERSION_STRING				"2.7"			// PsnAdBoard version string
#define DLL_VERSION_MAJOR			5				// Minimum DLL Version = 5.6
#define DLL_VERSION_MINOR			6	

/* Commands to control the PSNADBoard DLL/Library */
#define ADC_CMD_EXIT				0
#define ADC_CMD_SEND_STATUS			1
#define ADC_CMD_RESET_GPS			2
#define ADC_CMD_FORCE_TIME_TEST		3
#define ADC_CMD_CLEAR_COUNTERS		4
#define ADC_CMD_GPS_DATA_ON_OFF		5
#define ADC_CMD_GPS_ECHO_MODE		6
#define ADC_CMD_RESET_BOARD			7
#define ADC_CMD_GOTO_BOOTLOADER		8
#define ADC_CMD_SEND_TIME_INFO		9
#define ADC_CMD_RESTART_BOARD		10
#define ADC_CMD_SET_GAIN_REF   		70
#define ADC_CMD_DEBUG_REF          128

/* Commmand to get data data from the DLL */
#define ADC_GET_BOARD_TYPE			0
#define ADC_GET_DLL_VERSION			1
#define ADC_GET_DLL_INFO			2
#define ADC_GET_NUM_CHANNELS		3
#define ADC_GET_LAST_ERR_NUM		4

/* Message types from the DLL */
#define ADC_MSG						0
#define ADC_ERROR					1
#define ADC_AD_MSG					2
#define ADC_AD_DATA					3
#define ADC_GPS_DATA				4
#define ADC_STATUS					5
#define ADC_SAVE_TIME_INFO			6
#define TCP_CHANNEL_INFO			7

#define ADC_BOARD_ERROR				0
#define ADC_NO_DATA					1
#define ADC_GOOD_DATA				2

#define TIME_REF_UNKNOWN			-1
#define TIME_REF_NOT_LOCKED			0
#define TIME_REF_WAS_LOCKED			1
#define TIME_REF_LOCKED				2

#define	MAX_CHANNELS				16			// max channels, both real and derived
#define	MAX_ADC_CHANNELS			8			// max channels on the PSN 16 Bit board below 500 SPS
#define	MAX_500SPS_CHANNELS			4			// max channels on the PSN 16 Bit board for 500 SPS
#define MAX_SPS_RATE				200

#define TIMEUPDATE    				600			// Update PC clock every X times through
#define DEF_NOT_LOCK_TIME			30			// Send not lock message to StatMgr after this amount of time

#define MESSAGE_LEN					128
#define MAX_MESSAGES				10

#define MESSAGE_START_ROW			10
#define MAX_RPT_STR_LEN				4096

/* ADC Board Types */
#define BOARD_UNKNOWN				0
#define BOARD_RABBIT				1
#define BOARD_PICC					2
#define BOARD_VM					3
#define BOARD_DSPIC					4
#define BOARD_SDR24					5

/* Time reference types */
#define TIME_REF_PCTIME				0		// Use PC Time
#define TIME_REF_GARMIN				1		// Garmin GPS 16/18/18x
#define TIME_REF_MOT_NMEA			2		// Motorola NMEA
#define TIME_REF_MOT_BIN			3		// Motorola Binary
#define TIME_REF_WWV				4		// WWV Mode
#define TIME_REF_WWVB				5		// WWVB Mode
#define TIME_REF_SKG				6		// Sure Electronics SKG GPS Board
#define TIME_REF_4800				7		// 4800 Baud OEM GPS Receiver
#define TIME_REF_9600				8		// 9600 Baud OEM GPS Receiver

#define E_OPEN_PORT_ERROR       	1       // Error opening comm port
#define E_NO_CONFIG_INFO        	2       // No configure information
#define E_THREAD_START_ERROR    	3       // Receive thread startup error
#define E_NO_HANDLES            	4       // Too many used handles
#define E_BAD_HANDLE            	5       // Bad Handle ID
#define E_NO_ADC_CONTROL        	6       // Internal Error
#define E_CONFIG_ERROR          	7       // Configuration Error

#ifdef LONG
#undef LONG
#endif

// The following is needed to compile the program under Linux
#ifndef WIN32
#define BYTE						unsigned char
#define WORD						unsigned short
#define DWORD						unsigned int	// longs must be 4 bytes
#define UINT						unsigned int	// longs must be 4 bytes
#define ULONG						unsigned int	// longs must be 4 bytes
#define SLONG						int				// longs must be 4 bytes
#define LONG						int				// longs must be 4 bytes
#define BOOL						int
#define HANDLE						int
#define TRUE						1
#define FALSE						0
#define TIMEVAL						struct timeval

typedef struct  {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME;
#else
#define SLONG						long
#define LONG						int				// longs must be 4 bytes
#endif

#pragma pack(1)

/* Various structures used between this program and the PsnAdBoard DLL/Library */
typedef struct  {
	DWORD maxInQueue;
	DWORD maxUserQueue;
	DWORD maxOutQueue;
	DWORD crcErrors;
	DWORD userQueueFullCount;
	DWORD xmitQueueFullCount;
	DWORD cpuLoopErrors;
} DLLInfo;

typedef struct  {
	ULONG addDropTimer;
	BYTE addTimeFlag;
} AdjTimeInfo;

typedef struct  {
	BYTE hdr[4];
	WORD len;
	BYTE type, flags;
} PreHdr;

typedef struct  {
	SYSTEMTIME packetTime;
	ULONG packetID;
	BYTE timeRefStatus;
	BYTE flags;
} DataHeader;
	
typedef struct  {
	ULONG commPort;
	ULONG commSpeed;
	ULONG numberChannels;
	ULONG sampleRate;
	ULONG timeRefType;
	ULONG addDropTimer;
	ULONG pulseWidth;
	ULONG mode12BitFlags;
	ULONG highToLowPPS;
	ULONG noPPSLedStatus;
	ULONG checkPCTime;
	ULONG setPCTime;
	ULONG tcpPort;
	LONG addDropMode;
	LONG timeOffset;
	char commPortTcpHost[256];
} AdcBoardConfig2;

typedef struct  {
	char addDropFlag, flags;
	WORD pulseWidth;
	LONG addDropCount, timeLocked, adjNum;
	short avgDiff, timeOffset;
} TimeInfo;
	
#define MAX_SDR24_CHANNELS	4
typedef struct {
	double referenceVolts;
	BYTE  adcGains[ MAX_SDR24_CHANNELS ];
} AdcConfig;

typedef struct  {
	BYTE boardType;
	BYTE majorVersion;
	BYTE minorVersion;
	BYTE lockStatus;
	BYTE numChannels;
	BYTE spsRate;
	ULONG crcErrors;
	ULONG numProcessed;
	ULONG numRetran;
	ULONG numRetranErr;
	ULONG packetsRcvd;
	TimeInfo timeInfo;
} StatusInfo;
	
typedef struct  {
	ULONG timeTick;
	ULONG utcTime;
} NewTimeInfo;
	
typedef struct  {
	BYTE boardType;
	BYTE numChannels;
	char msgType;
	char unused;
	WORD sampleRate;
} MuxHdr;

#pragma pack()

typedef struct {
	int type;
	char *name;
} TimeRefInfo;

class CFilter;
class CPeriodExtend;

typedef struct  {
	char sta[6];				// 5 char station ID
	char comp[4];       		// 3 char sensor component ID like BHZ, SHN etc
	char net[3];				// 2 char network ID
	char loc[3];        		// 2 char location ID
	int useChannel;				// -1 = real channel, or channel index 
	int chanNum;				// used for the trace pin number
	int bitsToUse;				// Number of bits to use from the ADC
	int adcGain; 				// SDR24 Board only, can be 1,2,4,8,16,32 or 64
	int shiftNumber;			// number of bits to shift. Not in config file... 
	int filterDelay;			// filter delay in milliseconds
	int invertChannel;			// if TRUE invert data from ADC board for this channel
	int dcOffset;				// DC Offset to apply to the ADC data
	int sendToRing;				// Send channel to data ring
	LONG *dataBuff;				// Holds the demuxed data from the ADC board
	CPeriodExtend *invFilter; 	// Inverse or Period Expending Filter
	CFilter *lpFilter;			// Lowpass Filter;
	CFilter *hpFilter;			// Highpass Filter;
} SCNL;
	
struct Greg  {
	int year;
	int month;
	int day;
	int hour;
	int minute;
};

#define PI  			3.1415926535898
#define PI2 			6.2831853071796
#define IOTA       		1.0E-3   			/* small increment */
#define FREQ_MIN   		1.0E-4   			/* min freq */
#define GAIN_PASS 		-1.0E-2   			/* min passband gain */
#define GAIN_TRAN 		-3.0103   			/* min sb, max pb gain */
#define GAIN_STOP 		-2.0E02   			/* max stopband gain */

typedef struct  { 
	double  re, im;
} complex;

/* Period Extending or Inverse Filter Class */
class CPeriodExtend
{
public:
	
	void FilterData( LONG *data, int samples );
	void Filter( LONG *sample );
	void Setup( double pendulumFreq, double pendulumQ, double hpFreq, double hpQ, double sampleRate );
	void GetParams( double *pendulumFreq, double *pendulumQ, double *hpFreq, double *hpQ );
	void Reset();
		
private:
	double m_omegaF, m_omega2F, m_sigmaF, m_omegaP, m_b1, m_b2, dataSum;
	double m_sum1, m_sum2, m_oldsum2, m_biasRegister0;
	double m_pp, m_qp, m_pf, m_tc0, m_tc1, m_qf, m_spsRate;
	int m_nRep, m_minData, m_maxData;
	double m_pendulumFreq, m_pendulumQ, m_hpFilterFreq, m_hpFilterQ;
};

/* High and Low Pass Filter Class */
class CFilter
{
public:
	
	CFilter();
	~CFilter();
	
	void FilterData( LONG *, int );
	BOOL Setup( char, double, int, int );
	BOOL Check( char, double, int, int );
	void GetParams( double *freq, int *poles )  { *freq = m_Freq; *poles = m_Poles; }
	BOOL TestData();
	BOOL FilterError() { return m_FilterError; }
	void Reset();
	void CleanUp();
	
protected:	
	BOOL CalcFilterCoefs();
	BOOL UnNormalizeCoefs();
	BOOL WarpFreqs();
	BOOL UnWarpFreqs();
	BOOL CalcFilterOrder();
	BOOL CalcNormalCoefs();
	BOOL CalcButterCoefs();
	BOOL UnNormLPCoefs( double );
	BOOL UnNormHPCoefs( double );
	BOOL UnNormBSCoefs( double, double );
	BOOL UnNormBPCoefs( double, double );
	BOOL BilinearTransform();
	BOOL TestDouble( double, double, double );

	complex cadd( complex, complex );
	double cang( complex );
	complex cconj( complex );
	complex cdiv( complex, complex );
	double cimag( complex );
	double cmag( complex );
	complex cmplx( double, double );
	complex cmul( complex, complex );
	complex cneg( complex );
	void cprt( complex );
	void cQuadratic( complex, complex, complex, complex *, complex * );
	double creal( complex );
	complex csqr( complex );
	complex csub( complex,complex );
	
private:
	double m_Apass1, m_Apass2, m_Astop1, m_Astop2, m_Wpass1, m_Wpass2, m_Wstop1, m_Wstop2;
	double *m_pAcoefs, *m_pBcoefs, *m_pC, *m_pM; 
	double m_SampRate, m_Gain, m_Freq;
	int m_Order, m_NumQuads, m_Poles;
	BOOL m_FilterError; 
	char m_Type;
};

/* Global configuration parameters */

extern char CommPortHostStr[];	// Linux Comm port string or TCP/IP Server
extern int  TcpPort;			// Port number if TCP/IP Connection
extern int  TcpMode;			// If 1 CommPortHostStr = TCP/IP Host 
extern int  TotalChans;        	// Number of channels in ChanList (AdcChans + AddChans)
extern int  AdcChans;			// Number of real ADC channels from the "Chan" scnl line
extern int  AddChans;			// Number of additional or derived channels from the "AddChan" scnl line
extern unsigned char ModuleId;  // Data source id placed in the trace header
extern int  ChanRate;          	// Rate in samples per second per channel
extern LONG OutKey;            	// Key to ring where demuxed traces will live
extern LONG MuxKey;            	// Key to ring where mux data will live if enabled
extern int  UpdateSysClock;    	// Update PC clock with good IRIGE time
extern SCNL ChanList[MAX_CHANNELS];	// Array to fill with SCNL values
extern int  CommPort;			// Windows Comm port number
extern LONG	PortSpeed;			// Comm port baud rate
extern int  TimeRefType;		// Time reference type. See .d file for types
extern int  HighToLowPPS;		// Direction of 1PPS signal
extern int  NoPPSLedStatus;		// Turns of LED blinking
extern int  TimeOffset;			// Offset time in milliseconds added/subtracted to trace buffer time
extern int	NoDataTimeout;      // Take action if no data sent for this many seconds
extern int  HeartbeatInt;     	// Heartbeat interval in seconds
extern char TimeFileName[256];	// Path and root file name used to save/read time information
extern int	LogMessages;		// Log messages from the DLL and ADC board
extern int  AdcDataSize;		// Data byte size; Can be 2 or 4
extern int  AdcBoardType;		// Board Type; 1 = 16 Bit Version 1 or 2 board, 2  = VoltsMeter 
extern int  ConsoleDisplay;		// If 1 use Console functions to display status
extern int  ControlCExit;		// if 0 program ignores control-c, if 1 exit program							
extern int  RefreshTime;		// Console refresh time in seconds. ConsoleDisplay must be 1.
extern int  CheckStdin;			// if 1, check for user input
extern int  NoSendBadTime;		// If 1, don't send data packets to the ring if time not locked to GPS time
extern int  ExitOnTimeout;		// if 1, exit on data timeout
extern int  Debug;				// If 1, display/log debug information
extern int  SystemTimeError;	// Time test between A/D board and System time in seconds. 0 = no time test
extern int  PPSTimeError;		// Report error if 1PPS diff exceeds this amount in milliseconds. 0 = no test
extern int  NoLockTimeError;	// Report error if time reference not locked exceeds this amount in minutes
extern int  FilterSysTimeDiff;	// if 1 do not log System to A/D board time difference messages
extern int  FilterGPSMessages;	// if 1 do not log GPSRef messages
extern int  StartTimeError;		// Set system time using GPS at startup if error exceeds this amount in seconds

/* Functions prototypes */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define PSNADBOARD_API __declspec(dllimport)
#else
#define PSNADBOARD_API 
#define __stdcall
#endif

PSNADBOARD_API HANDLE __stdcall PSNOpenBoard();
PSNADBOARD_API BOOL __stdcall PSNCloseBoard( HANDLE );
PSNADBOARD_API BOOL __stdcall PSNConfigBoard( HANDLE, AdcBoardConfig2 *, void (*f)( DWORD, void *, void *, DWORD ) );
PSNADBOARD_API BOOL __stdcall PSNStartStopCollect( HANDLE, DWORD start );
PSNADBOARD_API DWORD __stdcall PSNGetBoardData( HANDLE hBoard, DWORD *type, void *data, void *data1, DWORD *dataLen );
PSNADBOARD_API BOOL __stdcall PSNSendBoardCommand( HANDLE hBoard, DWORD cmd, void *data );
PSNADBOARD_API BOOL __stdcall PSNGetBoardInfo( HANDLE hBoard, DWORD type, void *data );

#ifdef __cplusplus
}
#endif

int  GetArgs( int, char **, char ** );
int  GetConfig( char * );
int  GetData( LONG * );
int  ReadSta( char *, unsigned char, int, SCNL * );
int  InitADC();
void StopADC( void );
void DemuxShiftData( LONG * );
double CalcPacketTime( SYSTEMTIME *st );
void Heartbeat( void );
void InitCon( void );
void LogConfig( void );
void LogSta( int, SCNL * );
void GmtTimeStr( char *to, double tm, int dec );
void LogitGmtime( double, int );
void ReadTimeInfo( TimeInfo *info );
void SaveTimeInfo( TimeInfo *info );
void ReportError( int, char * );
void SetCurPos( int, int );
void ClearToEOL( int, int );
void LogWrite(char *type, char *pszFormat, ...);
void MakeTraceHeader( SCNL *pSCNL, double tm, DataHeader *hdr );
void MakeReport();
void DisplayReport();
void CntlHandler( int sig );
void ClearScreen();
void AddFmtStr( char *pszFormat, ... );
void AddFmtMessage( char *pszFormat, ... );
int FirstPacketCheck( char *errMsg );
void FilterData( SCNL *pSCNL );
int IsGpsRef();
int OkToSendData( SCNL *pSCNL, DataHeader *hdr );
int TestForDupChan( SCNL *pSCNL, int channels );
time_t MakeLTime( int, int, int, int, int, int );
char *GetErrorString( DWORD errNo );
void Shutdown();
int Restart();
void CheckForErrors( char *msg );
void CheckLockStatus( int newStatus );
int FilterLogMsg( char *msg );
void GetRefName( int type, char *to );
void SendMuxedData( DataHeader *hdr, BYTE *data, int dataLen );
void SendLogMessage( char *msg );
void CheckSetSysTime( double dTraceTime );
void SetComputerTime( double now );
#ifndef WIN32
void strupr( char * );
#endif

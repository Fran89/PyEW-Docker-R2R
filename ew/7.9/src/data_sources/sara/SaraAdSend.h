// SaraAdSend.h

#define	MAX_ADC_CHANNELS		8
#define MAX_SPS_RATE			200

#define MESSAGE_LEN				79
#define MAX_MESSAGES			10

#define MESSAGE_START_ROW		10

typedef struct  {
   char sta[6];
   char comp[4];
   char net[3];
   char loc[3];
} SCNL;

#define BYTE 			unsigned char
#define WORD 			unsigned short

/* Global configuration parameters */
extern unsigned char ModuleId;  // Data source id placed in the trace header
extern int  ChanRate;          	// Rate in samples per second per channel
extern long OutKey;            	// Key to ring where traces will live
extern SCNL *ChanList;         	// Array to fill with SCNL values
extern int  Nchan;             	// Number of channels in ChanList
extern int  CommPort;			// Comm port number to use
extern long	PortSpeed;			// Comm port baud rate
extern int	AdcType;      		// Adc Board Type: 0 = Sara24, 1= Sara16
extern int	TimeRefType;        // Time reference type: 0 = use PC, 1 = external ref.

extern int  TimeSyncError;		// Resync error value
extern int  GmtCorrection;		// This correction value is sent to the A/D board at startup
extern int  HeartbeatInt;     	// Heartbeat interval in seconds
extern int  AdcDataSize;		// Data byte size; Can be 2 or 4
extern int  AdcBoardType;		// Board Type; 1 = SADC20 

int OpenPort( int, int, HANDLE *);
int GetArgs( int, char **, char ** );
int GetConfig( char * );
int ReadSta( char *, unsigned char, int, SCNL * );
BOOL InitDAQ();
void SetDateTime( int wait );
void ClosePort();
void FlushComm();
void NewSampleData24();
void NewSampleData16();
void SetSpsRate( int sps );
void Heartbeat( void );
void InitCon( void );
void LogConfig( void );
void LogSta( int, SCNL * );
void PrintGmtime( double, int );
void LogitGmtime( double, int );
void ReportError( int, char * );
void SetCurPos( int, int );
void StopDAQ( void );
void ClearToEOL( int, int );
void logWrite(char *, ...);
void MakePacketTime( SYSTEMTIME *st, BYTE *packetTime );
void SetGmtCorrection( char correction );
void TimeStr( char *to, SYSTEMTIME *t );
time_t MakeTimeFromGMT( struct tm *t );
time_t SystemTimeToTime( SYSTEMTIME *st );
ULONG CalcPacketTime( SYSTEMTIME *st );

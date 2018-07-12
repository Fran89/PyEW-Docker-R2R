// Ws2Ew.h

#ifdef WIN32
#include <process.h>
#endif

#include <mem_circ_queue.h>
#include <socket_ew.h>

#define TIME_REF_NOT_LOCKED		0
#define TIME_REF_WAS_LOCKED		1
#define TIME_REF_LOCKED			2

#define THREAD_STACK			16384
#define MAX_CONNECT_USERS		16
#define MAX_CHAN_LIST			64

#define MESSAGE_LEN				120
#define MAX_MESSAGES			10
#define MAX_RPT_STR_LEN			4096

#ifndef WIN32
#define BYTE					unsigned char
#define WORD					unsigned short
#define INT32					int				// longs must be 4 bytes
#define DWORD					unsigned int	// longs must be 4 bytes
#define UINT					unsigned int	// longs must be 4 bytes
#define UINT32					unsigned int	// longs must be 4 bytes
#define BOOL					int
#define HANDLE					int
#define TRUE					1
#define FALSE					0
#define TIMEVAL					struct timeval

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
#endif

typedef struct  {
	char station[6];
	char location[4];
  	char channel[4];
  	char network[4];
	double sampleRate;
	INT32 boardType;
	INT32 sequenceNumber;
	INT32 numSamples;
	SYSTEMTIME startTime;
	BYTE timeRefStatus;
} DataHeader;

typedef struct  {
	char sta[6];
	char comp[4];
	char net[4];
	char loc[4];
	char send;
	UINT32 sequenceNumber;
	int filterDelay;		// in milliseconds
} SCNL;

typedef struct {
	int inUse;
	int index;
	int exitThread;
	int exitAck;
	int recvPackets;
	UINT totalRecvPackets;
	int connecting;
	int connected;
	int restart;
	int boardType;
	int noDataReport;
	SOCKET sock;
	unsigned tidReceive; 
	time_t updateTime;
	time_t connectTime;
	char inData[ 4096 ];
	char settings[ 256 ];
	char who[ 64 ];
	void *pCI;
} UserInfo;

typedef struct  {
	char who[ 64 ];
	UINT32 connectCount;
	UserInfo *pUI;
} ConnectInfo;
	
/* Global configuration parameters */
extern unsigned char ModuleId;  // Data source id placed in the trace header
extern int  ChanRate;          	// Rate in samples per second per channel
extern long OutKey;            	// Key to ring where traces will live
extern int  UpdateSysClock;    	// Update PC clock with good IRIGE time
extern SCNL ChanList[ MAX_CHAN_LIST]; // Array to fill with SCNL values
extern int  HeartbeatInt;     	// Heartbeat interval in seconds
extern int Port;				// Port number
extern int SocketTimeout;		// Time in milliseconds
extern int NoDataWaitTime;		// Time to wait for no data. If no data reset recv thread. In seconds.
extern int RestartWaitTime;		// Time to wait between reconnects. In seconds.
extern int  AdcDataSize;		// data byte size; Can be 2 or 4
extern char Host[256];			// Host name or IP Address
extern SHM_INFO OutRegion;		// Info structure for output region
extern pid_t MyPid;				// process id, sent with heartbeat
extern HANDLE outHandle;		// The console file handle
extern struct sockaddr_in saddr; // Socket address structure
extern int ListenMode;			// if TRUE listen for connections
extern int SendAck;				// if TRUE send ack packet after receiving n packets
extern int Debug;				// if TRUE print debug info
extern int Nchans;            	// Number of channels in ChanList
extern int ConsoleDisplay;		// Windows Only, if 1 use console functions like SetPos()
								// to display process info.
extern int ControlCExit;		// if 0 program ignores control-c, if 1 exit program							
extern int RefreshTime;			// Console refresh time in seconds. ConsoleDisplay must be 1.
extern int CheckStdin;			// if 1 check for user input

int InitWs2Ew( char *configFile );
int GetArgs( int, char **, char ** );
int GetConfig( char * );
void Heartbeat( time_t now );
void InitCon( void );
void LogConfig( void );
void LogSta( int, SCNL * );
void PrintGmtime( double, int );
int  ReadSta( char *, unsigned char, int, SCNL * );
void ReportError( int errNum, char *errmsg, ... );
double CalcPacketTime( SYSTEMTIME *, int filterDelay );
int InitSocket();
int StartReceiveThread();
int GetInQueue( BYTE * );
BYTE CalcCRC( BYTE *, int );
void NewData();
void CheckStatus();
void LogWrite( char *, char *, ...);
void LogWriteThread( char *, ...);
void KillReceiveThread( UserInfo *pUI );
void KillListenThread();
void CloseUserSock( UserInfo *pUI );
int FindChannel( DataHeader *pHdr );
void ExitWs2Ew( int exitcode );
void GetWinSDRInfo( UserInfo *pUI );
void DisplayReport();
void AddFmtStr( char *pszFormat, ... );
void AddFmtMessage( char *pszFormat, ... );
void MakeReportString();
void CntlHandler( int sig );
void MakeUserStr( UserInfo *pUI );
void ParseFirstChanName( UserInfo *pUI );
void ClearUserData();
ConnectInfo *FindOrAddUser( UserInfo *pUI, int addOk );
void SetCurPos( int, int );
void ClearScreen();
void CheckUsers( ConnectInfo *pCI, time_t now );

thr_ret ReceiveThread( void * );
thr_ret ListenThread( void * );
thr_ret ReceiveLoop( void * );

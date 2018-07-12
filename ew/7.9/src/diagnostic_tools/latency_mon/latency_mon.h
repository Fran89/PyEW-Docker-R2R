/******************************************************************
 *                     File latency_mon.h                         *
 *                                                                *
 *  Include file for latency_monitor module                       *
 ******************************************************************/

#include <trace_buf.h>
//#include "\earthworm\atwc\src\libsrc\filters.h"
//#include "\earthworm\atwc\src\libsrc\geotools.h"

#define MAX_STATIONS     200 /* Max # stations in station file */

/* Definitions
   ***********/
#define MAX_ONOFF          4000     /* Maximum # latency periods allowed */
#define NUM_INTERVALS         5     /* Number of latency intervals */

/* Menu options
   ************/
#define IDM_STATION_STATUS  110     /* Bring up status display */
#define IDM_TRACEPERSCREEN  112     /* Control panel options */
#define IDM_PRINTSUMMARY    114
#define IDM_SAVESUMMARY     116
#define IDM_REFRESH         140	    /* Refresh screen */

/* Entry fields
   ************/
#define EF_NUMSTATODISP     3000    /* For TracesPerScreen dialog */
#define EF_DISPLAYYEAR      3002    /* For StationStatus dialog */
#define EF_DISPLAYMONTH     3003
#define EF_DISPLAYDAY       3004
#define EF_DISPLAYHOUR      3005
#define EF_DISPLAYTOTALTIME 3006

#define ID_NULL            -1       /* Resource file declaration */

/* User defined window messages
   ****************************/
#define WM_NEW_DATA (WM_USER + 10)
#define WM_NEW_PPICKS (WM_USER + 11)


#include <trace_buf.h>  /* TRACE_CHAN_LEN, TRACE_NET_LEN, TRACE_STA_LEN */


#define LATENCY_CUTOFF_COUNT 4

//static double LATENCY_CUTOFFS[] =
//{
//    60.0    /*           interval_0  <=  60.0 seconds */
// , 120.0    /*   60.0 <  interval_1  <= 120.0 seconds */
// , 180.0    /*  120.0 <  interval_2  <= 180.0 seconds */
// , 300.0    /*  180.0 <  interval_3  <= 300.0 seconds */
//};          /*  300.0 <  interval_4                   */

static double LATENCY_CUTOFFS[] =
{
   100.0
 , 200.0
 , 400.0
 , 800.0
};

typedef char   PERIOD_LATENCY;
typedef double PERIOD_TIME;



typedef struct
{
  PERIOD_TIME     stt_time;  // dStart
  PERIOD_TIME     end_time;  // dEnd
  PERIOD_LATENCY  latency;   // cLate
} LATENCY_PERIOD;


/*
** This is a subset of the STATION struct typedef, declared in
** the ATWC file \earthworm\atwc\src\libsrc\filters.h.
** Using this typedef instead significantly reduces memory overhead for stations
*/
typedef struct
{
   char            szChannel[TRACE_CHAN_LEN]; /* Channel identifier (SEED notation) */
   char            szNetID[TRACE_NET_LEN];    /* Network ID */
   char            szStation[TRACE_STA_LEN];  /* Station name */
   double          dSampRate;                 /* Sample rate (samples/sec) */
/*   PERIOD_TIME     dPeriodSttTime;   */         /* 1/1/70 time (sec) that Phase1 was passed [dTrigTime] */
/*   PERIOD_TIME     dPeriodEndTime;   */         /* Time at end of last packet [dEndTime] */
/*   PERIOD_LATENCY  LastLatency;      */

   LATENCY_PERIOD  LastPeriod;
   char            StoreFileName[90];           /* latency filename  */
} LATENCY_STATION;


typedef struct
{
/*  int    iLate[MAX_ONOFF];   */  /* Latency indicator (see code for desc.) */
/*  char    cLate[MAX_ONOFF];  */  /* Latency indicator (see code for desc.) */
/*  double  dStart[MAX_ONOFF]; */  /* Start time (1/1/70 sec) of this period */
/*  double  dEnd[MAX_ONOFF];   */  /* End time (1/1/70 sec) of this period */
/*  long    lIntervals;        */  /* Total number of latency periods */
   LATENCY_PERIOD Periods[MAX_ONOFF];     /* Latency period data  */
   unsigned short UsedPeriods;            /* Number of used latency periods (must contain MAX_ONOFF) */
   double         Percent[NUM_INTERVALS]; /* % of time in each of the latency intervals */
} LATENCY;

typedef struct {
  unsigned char MyModId;            /* Module id of this program */
  long          InKey;              /* Key to ring where waveforms live */
  SHM_INFO      InRegion;           /* Info structure for input region */
  int           HeartbeatInt;       /* Heartbeat interval in seconds */
  short         Debug;              /* If 1, print debug messages */
  int           NumTracePerScreen;  /* # traces to show on screen (rest scroll) */
  unsigned int  FileRowSize;        /* number of line to allocate in circular files */
  char          FindWaveFile[64];	 /* Name of FindWave-style file */
  char          StaFile[64];        /* Name of file with SCN info */
  char          StaDataFile[64];    /* Station information file */
  char          LogPath[64];        /* Path to put latency information */
  char          PrinterPath[64];    /* Where to open printer */
} GPARM;

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char GetThisInstId;   /* Get messages from this inst id */
   unsigned char GetThisModId;    /* Get messages from this module */
   unsigned char TypeHeartBeat;   /* Heartbeat message id */
   unsigned char TypeError;       /* Error message id */
   unsigned char TypeWaveform;    /* Earthworm waveform messages */
} EWH;

/*
** Header to use in bounded-size file
**
**  If next_row == start_row, then the file is full and older data
**  is being overwritten.
*/
typedef struct {
  unsigned short version;      /* +1000, thus 1001 = version 1 */
  unsigned int   row_count;
  unsigned int   start_row;
  unsigned int   next_row;
  PERIOD_TIME    last_time;
} LATN_FILE_HEADER;

#define CURRENT_LATN_FILE_VERSION  (unsigned short)1001


   /* Function declarations for latency_mon
   *************************************/
void    DisplayChannelID( HDC, LATENCY_STATION[], int, long, long, int, int, int, int, int );
void    DisplayLatencyGraph( HDC, int, int, LATENCY_STATION[], LATENCY [], int, int,
                             int, int, int, int );
void    DisplayTitles( HDC, long, long, int, int, LATENCY [], int, int,
                       int, int );
int     GetEwh( EWH * );
void    InitLatency( LATENCY * );
long WINAPI StationStatusDlgProc( HWND, UINT, UINT, long );
long WINAPI TracePerScreenDlgProc( HWND, UINT, UINT, long );
long WINAPI WndProc( HWND, UINT, UINT, long );
thr_ret WThread( void * );

int     GetConfig( char *, GPARM * );                    /* config.c */
void    LogConfig( GPARM * );

int     GetStaList( LATENCY_STATION **, int *, GPARM * );        /* stalist.c */
int     GetFindWaveStaList( LATENCY_STATION **, int *, GPARM * );
int     IsComment( char [] );
int     LoadStationData( LATENCY_STATION *, char * );
void    LogStaList( LATENCY_STATION *, int );

int     WritePeriod2File(       FILE           * p_file
                        , const LATENCY_PERIOD * p_period
                        , const char           * p_filename
                        );

int     InitStoreFile( const char * p_filename );

int     LoadSpanFromFile( HWND p_hwnd );


/******************************************************************
 *                     File pick_wcatwc.h                         *
 *                                                                *
 *  Include file for P-picker used at the West Coast/Alaska       *
 *  Tsunami Warning Center.  Made into Earthworm module 12/2000.  *
 ******************************************************************/
                                     
#include <trace_buf.h>
#include <earlybirdlib.h>

/* Definitions
 *************/
#define PK_RESTART 1        /* Set when time series broken, picker restarted */
#define GLOBAL_PICK_VERSION 1 /* NEIC global pick structure (message type) */

typedef struct {
   char     StaFile[128];         /* Name of file with SCN info */
   char     StaDataFile[128];     /* Station information file */
   char     ResponseFile[128];    /* Broadband station response file */
   char     ATPLineupFileBB[128]; /* Optional command when used with ATPlayer */
   long     InKey;                /* Key to ring where waveforms live */
   long     OutKey;               /* Key to ring where picks will live */
   long     AlarmKey;             /* Key to ring where alarms will live */
   int      HeartbeatInt;         /* Heartbeat interval in seconds */
   int      MaxGap;               /* Maximum gap to interpolate */
   int      Debug;                /* If 1, print debug messages */
   int      iRedoLineupFile;      /* 1-> Reset lineup file when player used; 0->don't */
   double   HighCutFilter;        /* Bandpass high cut in hz */
   double   LowCutFilter;         /* Bandpass low cut in hz */
   unsigned char MyModId;         /* Module id of this program */
   double   LTASeconds;           /* Moving average length of time (seconds) */
   double   MinFreq;              /* Minimum P frequency (hz) of interest */
   double   dMinFLoc;             /* Frequency to identify potential local events */
   double   dSNLocal;             /* S:N which must be exceeded for local P-picks */
   int      MbCycles;             /* # 1/2 cycles after P Mb can be computed */
   int      LGSeconds;            /* # seconds after P in which max LG can be 
                                     computed for Ml (excluding 1st MbCycles) */
   int      MwpSeconds;           /* Max # seconds to evaluate P for Mwp */
   double   MwpSigNoise;          /* Auto-Mwp necessary signal-to-noise ratio */
   int      AlarmOn;              /* 1->Alarm function enabled, 0->Disabled */
   double   AlarmTimeout;         /* Time (sec) to re-start alarm after trig */
   int      AlarmTime;            /* If no data in this many secs, send alarm */
   int      NeuralNet;            /* 1->Run pick through NN, 0->No */
   int      PickOnAcc;            /* 1->Use acceleration data for P-picks */
   int      TwoStnAlarmOn;        /* 1->multo station alarm on, 0->off */
   char     TwoStnAlarmFile[128]; /* File name with two station params */
   SHM_INFO InRegion;             /* Info structure for input region */
   SHM_INFO OutRegion;            /* Info structure for output region */
   SHM_INFO AlarmRegion;          /* Info structure for alarm output region */
} GPARM;

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char GetThisInstId;   /* Get messages from this inst id */
   unsigned char GetThisModId;    /* Get messages from this module */
   unsigned char TypeHeartBeat;   /* Heartbeat message id */
   unsigned char TypeError;       /* Error message id */
   unsigned char TypeAlarm;       /* Tsunami Ctr alarm message id */
   unsigned char TypePickTWC;     /* Tsunami Ctr P-picker message id */
   unsigned char TypePickGlobal;  /* NEIC format P-picker message id */
   unsigned char TypeWaveform;    /* Waveform buffer for data input */
} EWH;

/* Function declarations 
   *********************/
int     GetEwh( EWH * );                                 /* pick_wcatwc.c */
void    Interpolate( STATION *, char *, int );
void    LogStaListP( STATION *, int );
int     ReadAlarmParams( ALARMSTRUCT **, int *, char * ); 

int     GetConfig( char *, GPARM * );                    /* config.c */
void    LogConfig( GPARM * );  


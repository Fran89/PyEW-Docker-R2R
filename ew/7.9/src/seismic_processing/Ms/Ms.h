/******************************************************************
 *  File Ms.h                                                     *
 *                                                                *
 *  Include file for Ms.c code.                                   *
 ******************************************************************/

#include <trace_buf.h>
#include <earlybirdlib.h>

#ifndef _WINNT
typedef void *HANDLE;
#endif

/* Definitions
   ***********/
#define MAX_HYPO_SIZE      512/* Maximum size of TYPE_HYPOTWC */
#define RWAVE_WINDOW      1800/* Rayleigh wave window length (seconds) */

typedef struct {
   double   dHighCutFilter;       /* Bandpass high cut in hz */
   double   dLowCutFilter;        /* Bandpass low cut in hz */
   double   dMagThreshForAuto;    /* Lowest magnitude to autostart Ms proc. */
   double   dSigNoise;            /* LP signal-to-noise ratio for MS */
   int      iAutoStart;           /* If 1, Start based on msgs in HYPO_RING */
   int      iDebug;               /* If 1, print debug messages */
   int      iFileLengthLP;        /* Length of disk_wcatwc LP data files */
   int      iHeartbeatInt;        /* Heartbeat interval in seconds */
   int      iMinutesInBuff;       /* Number of minutes data to save per trace */
   int      iNumStnForAuto;       /* Minimum # of stns to trigger autostart */
   long     lHKey;                /* Key to ring where hypocenters will live */
   long     lInKey;               /* Key to ring where waveforms live */
   long     lPKey;                /* Key to ring where picks will live */
   char     szArchiveDir[128];    /* Archive Data File for archived data */
   char     szATPLineupFileLP[128];/* Optional command when used with ATPlayer*/
   char     szDataDirectory[128]; /* Disk File Folder for old data */
   char     szDummyFile[128];     /* Hypocenter parameter disk file */
   char     szFileSuffix[8];      /* File suffix (disk data) */
   char     szLPRTFile[128];      /* Long period data file for LOCATE */
   char     szQuakeFile[128];     /* Previously located quakes from loc_wcatwc*/
   char     szResponseFile[128];  /* Broadband station response file */
   char     szStaDataFile[128];   /* Station information file */
   char     szStaFile[128];       /* Name of file with SCN info */
   unsigned char ucMyModId;       /* Module id of this program */
   SHM_INFO InRegion;             /* Info structure for input region */
   SHM_INFO PRegion;              /* Info structure for P region */
   SHM_INFO HRegion;              /* Info structure for Hypocenter region */
   SHM_INFO ARegion;              /* Info structure for Alarm region */
} GPARM;

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char GetThisInstId;   /* Get messages from this inst id */
   unsigned char GetThisModId;    /* Get messages from this module */
   unsigned char TypeHeartBeat;   /* Heartbeat message id */
   unsigned char TypeError;       /* Error message id */
   unsigned char TypePickTWC;     /* P-pick message - TWC format */
   unsigned char TypeWaveform;    /* Earthworm waveform messages */
   unsigned char TypeHypoTWC;     /* Hypocenter message - TWC format*/
   unsigned char TypeAlarm;       /* Tsunami Ctr alarm message id */
} EWH;

/* Function declarations for Ms
   ****************************/
int         GetEwh( EWH * );

void        GetLDC( long, int32_t *, double *, long );       /* Ms_processing.c */
int         GetMs( STATION *, int, HYPO *, GPARM *, int );
void        PadBuffer( long, double, long *, int32_t *, long );
int         PatchDummyWithLP( HYPO *, char * );
void        PutDataInBuffer( TRACE2_HEADER *, int32_t *, int32_t *, long *, long );

int         GetConfig( char *, GPARM * );                 /* Ms_config.c */
void        LogConfig( GPARM * );

thr_ret     HThread( void * );                            /* Ms_threads.c */
thr_ret     RTLPThread( void * );
thr_ret     WThread( void * );


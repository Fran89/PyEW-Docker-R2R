/******************************************************************
 *  File Mm.h                                                     *
 *                                                                *
 *  Include file for Mm.c code in MmNew.                          *
 ******************************************************************/

#include <trace_buf.h>
#include <earlybirdlib.h>

#ifndef _WINNT
 typedef void *HANDLE;
#endif

/* Definitions
   ***********/
#define MAX_HYPO_SIZE       512   /* Maximum size of TYPE_HYPOTWC */
#define MAX_FFT_ORDER        25   /* Max order of FFT (2**MAX_FFT_ORDER */

typedef struct {
   double   dMagThreshForAuto;    /* Lowest magnitude to autostart Ms proc. */
   double   dSigNoise;            /* LP signal-to-noise ratio for Mm */
   int      iAutoStart;           /* If 1, Start based on msgs in HYPO_RING */
   int      iDebug;               /* If 1, print debug messages */
   int      iFileLengthLP;        /* Length of disk_wcatwc LP data files */
   int      iHeartbeatInt;        /* Heartbeat interval in seconds */
   int      iMinutesInBuff;       /* Number of minutes data to save per trace */
   int      iNumStnForAuto;       /* Minimum # of stns to trigger autostart */
   long     lHKey;                /* Key to ring where hypocenters will live */
   long     lInKey;               /* Key to ring where waveforms live */
   char     szArchiveDir[128];    /* Archive Data File for archived data */
   char     szATPLineupFileLP[128];/* Optional command when used with ATPlayer*/
   char     szDataDirectory[128]; /* Disk File Folder for old data */
   char     szDummyFile[128];     /* Hypocenter parameter disk file */
   char     szFileSuffix[8];      /* File suffix (disk data) */
   char     szMwFile[128];        /* Mw results file */
   char     szQuakeFile[128];     /* Previously located quakes from loc_wcatwc*/
   char     szRegionFile[128];    /* Mm path correction file */
   char     szResponseFile[128];  /* Broadband station response file */
   char     szStaDataFile[128];   /* Station information file */
   char     szStaFile[128];       /* Name of file with SCN info */
   unsigned char ucMyModId;       /* Module id of this program */
   SHM_INFO InRegion;             /* Info structure for input region */
   SHM_INFO HRegion;              /* Info structure for Hypocenter region */
} GPARM;

typedef struct {
   unsigned char MyInstId;        /* Local installation */
   unsigned char GetThisInstId;   /* Get messages from this inst id */
   unsigned char GetThisModId;    /* Get messages from this module */
   unsigned char TypeHeartBeat;   /* Heartbeat message id */
   unsigned char TypeError;       /* Error message id */
   unsigned char TypeWaveform;    /* Earthworm waveform messages */
   unsigned char TypeHypoTWC;     /* Hypocenter message - TWC format*/
} EWH;

/* Function declarations for Mm
   ****************************/
int         GetEwh( EWH * );

double      CompMm( long, int32_t *, STATION *, double, double, double, int );
void        coolb( int, fcomplex [], double );            /* Mm_processing.c */
int         FillBuff( long *, int32_t *, STATION * );
int         FillBuffBG( long *, int32_t *, STATION *, double, int );
void        GetLDC( long, int32_t *, double *, long );
void        mydtr( float [], int );
void        myfnd( int, int *, int * );
void        mysert( double, double[], int, double *, double *, int *, int * );
void        mytpr( float [], int, double, double );
void        PadBuffer( long, double, long *, int32_t *, long );
int         PatchDummyWithMm( HYPO *, char * );
void        PutDataInBuffer( TRACE2_HEADER *, int32_t *, int32_t *, long *, long );
void        resgeo( double, double, int, int, fcomplex [], fcomplex [], 
                    fcomplex * );
int         ReadRegion( char * );

int         GetConfig( char *, GPARM * );                 /* Mm_config.c */
void        LogConfig( GPARM * );

thr_ret     HThread( void * );                            /* Mm_threads.c */
thr_ret     MomThread( void * );
thr_ret     WThread( void * );


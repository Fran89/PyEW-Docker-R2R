/******************************************************************
 *  File Theta.h                                                  *
 *                                                                *
 *  Include file for theta.c code.                                *
 ******************************************************************/

#include <trace_buf.h>
#include <earlybirdlib.h>

#ifndef _WINNT
 typedef void *HANDLE;
#endif

/* Definitions
   ***********/
#define MAX_FFT_ORDER        25   /* Max order of FFT (2**MAX_FFT_ORDER) */
#define TAPER_WIDTH        0.05   /* Time Fraction window tapered at each end */
#define MAX_SAMPLE_RATE    200.   /* Maximum expected sample rate */
#define MAX_THETABUFF_SIZE 16344  /* Max number samples in data arrays */

typedef struct                    /* GParm structure */
{
   double   dFiltHi;              /* High-cut filter (Hz) */
   double   dFiltLo;              /* Low-cut filter  (Hz) */
   double   dMagThreshForAuto;    /* Lowest magnitude to autostart Theta proc.*/
   double   dMaxDelta;            /* Stn won't be considered if delta > this */
   double   dMinDelta;            /* Stn won't be considered if delta < this */
   SHM_INFO HRegion;              /* Info structure for Hypocenter region */
   SHM_INFO InRegion;             /* Info structure for input region */
   int      iAutoStart;           /* If 1, Start based on msgs in HYPO_RING */
   int      iDebug;               /* If 1, print debug messages */
   int      iFileSize;            /* Length of disk_wcatwc data files */
   int      iHeartbeatInt;        /* Heartbeat interval in seconds */
   int      iMinutesInBuff;       /* Number of minutes data to save per trace */
   int      iNumStnForAuto;       /* Minimum # of stns to trigger autostart */
   int      iWindowLength;        /* # sec following P used for energy calc. */
   long     lHKey;                /* Key to ring where hypocenters will live */
   long     lInKey;               /* Key to ring where waveforms live */
   unsigned char MyModId;         /* Module id of this program */
   char     szArchiveDir[128];    /* Archive Data File for archived data */
   char     szATPLineupFileBB[128];/* Optional command when used with ATPlayer */
   char     szDataDirectory[128]; /* Disk file folder for old data */
   char     szDiskPFile[128];     /* P-time file from EQCentral */
   char     szDummyFile[128];     /* Hypocenter parameter disk file */
   char     szFileSuffix[8];      /* File suffix (disk data) */
   char     szLocFilePath[128];   /* Path for disk logs of phase times */
   char     szAlarmFile[128];     /* File to write alarm to */
   char     szQuakeFile[128];     /* File with recent auto-located quakes */
   char     szResponseFile[128];  /* Broadband station response file */
   char     szStaDataFile[128];   /* Station information file */
   char     szStaFile[128];       /* Name of file with SCN info */
   char     szThetaFile[128];     /* Theta results file */
} GPARM;

typedef struct 
{
   unsigned char GetThisInstId;   /* Get messages from this inst id */
   unsigned char GetThisModId;    /* Get messages from this module */
   unsigned char MyInstId;        /* Local installation */
   unsigned char TypeError;       /* Error message id */
   unsigned char TypeHeartBeat;   /* Heartbeat message id */
   unsigned char TypeHypoTWC;     /* Hypocenter message - TWC format*/
   unsigned char TypeWaveform;    /* Earthworm waveform messages */
} EWH;

/* Function declarations for Theta
   *******************************/
int     GetEwh( EWH * );                                           /* Theta.c */

int     CompTheta( HYPO *, STATION [], int, int, char *, int, char *, 
                   double, double, int, double, double, int );
void    coolb( int, fcomplex [], double );              /* Theta_processing.c */
int     GetEnergy( double, double, int, double *, long, STATION * );
void    GetLDC( long, int32_t *, double *, long );
void    HanningTaper( double *, int, double );
void    LogAlarm( char * );
void    PadBuffer( long, double, long *, int32_t *, long );
int     PatchDummyWithTheta( HYPO *, char * );
void    PutDataInBuffer( TRACE2_HEADER *, int32_t *, int32_t *, long *, long );
void    resgeo( double, STATION *, fcomplex * );

int     GetConfig( char *, GPARM * );                       /* Theta_config.c */
void    LogConfig( GPARM * );

thr_ret HThread( void * );                                 /* Theta_threads.c */
thr_ret ThThread( void * );
thr_ret WThread( void * );


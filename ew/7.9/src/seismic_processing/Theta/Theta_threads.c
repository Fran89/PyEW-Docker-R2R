       /****************************************************************
       *  Theta_threads.c                                              *
       *                                                               *
       *  This file contains the thread routines used by Theta.        *
       *                                                               *
       *  This code is based on thread routines used in the original   *
       *  WCATWC Earlybird module Mm and Engmom.                       *
       *                                                               *
       *  2008: Richard Luckett, British GS, rrl@bgs.ac.uk             *
       *  2012: Paul Whitmore, NOAA-WCATWC - paul.whitmore@noaa.gov    *
       *                                                               *
       ****************************************************************/

#include <stdio.h>
#include <string.h>                               
#include <transport.h>
#include <earthworm.h>                      
#include <swap.h>
#include "Theta.h"

/* Global variable defined in Main */
extern double  dLastEndTime;   /* End time of last packet */
extern EWH     Ewh;            /* Parameters from earthworm.h */
extern MSG_LOGO getlogoW;      /* Logo of requested waveforms */
extern GPARM   Gparm;          /* Configuration file parameters */
extern HANDLE  hEventTheta;    /* Event shared externally to trigger Theta */
extern HANDLE  hEventThetaDone;/* Event shared externally to signal proc. done*/
extern MSG_LOGO hrtlogo;       /* Logo of outgoing heartbeats */
extern HYPO    HStruct;        /* Hypocenter data structure */
extern int     iRunning;       /* 1-> Threads are operating A-OK; 0-> Stop */
extern mutex_t mutsem;         /* Semaphore to protect StaArray adjustments */
extern pid_t   myPid;          /* Process id of this process */
extern int     Nsta;           /* Number of stations to process */
extern STATION *StaArray;      /* Station data array */
extern time_t  then;           /* Previous heartbeat time */
extern char    *WaveBuf;       /* Pointer to waveform buffer */
extern TRACE2_HEADER *WaveHead;/* Pointer to waveform header */
extern int32_t *WaveLong;      /* Long pointer to waveform data */
extern short   *WaveShort;     /* Short pointer to waveform data */

      /*********************************************************
       *                     HThread()                         *
       *                                                       *
       *  This thread gets messages from the hypocenter ring.  *
       *                                                       *
       * Dec., 2012: Theta alarm added.                        *
       *                                                       *
       *********************************************************/
	   
thr_ret HThread( void *dummy )
{
   MSG_LOGO      getlogoH;        /* Logo of requested picks */
   char          HIn[MAX_HYPO_SIZE];/* Pointer to hypocenter from ring */
   HYPO          HStructT;        /* Temporary Hypocenter data structure */
   static int    iVersion;        /* Version of last quake (processed) */
   static time_t lLastAlarmTime;  /* Time of last Theta Alarm */
   MSG_LOGO      logo;            /* Logo of retrieved msg */
   long          MsgLen;          /* Size of retrieved message */
   time_t        now;             /* Current time */
   static char   szQuakeID[32];   /* ID of last quake (processed) */

   iVersion = -1;
   strcpy( szQuakeID, "" );
   lLastAlarmTime = 0;

/* Set up logos for Hypo ring 
   **************************/
   getlogoH.instid = Ewh.GetThisInstId;
   getlogoH.mod    = Ewh.GetThisModId;
   getlogoH.type   = Ewh.TypeHypoTWC;

/* Flush the input ring
   ********************/
   while ( tport_getmsg( &Gparm.HRegion, &getlogoH, 1, &logo, &MsgLen,
                         HIn, MAX_HYPO_SIZE ) != GET_NONE );
						 
/* Loop to read hypocenter messages and load buffer
   ************************************************/
   while ( tport_getflag( &Gparm.HRegion ) != TERMINATE )
   {
      int     rc;               /* Return code from tport_getmsg() */

/* Get a hypocenter from transport region
   **************************************/
      rc = tport_getmsg( &Gparm.HRegion, &getlogoH, 1, &logo, &MsgLen,
                         HIn, MAX_HYPO_SIZE);

      if ( rc == GET_NONE )
      {
         sleep_ew( 1000 );
         continue;
      }

      if ( rc == GET_NOTRACK )
         logit( "et", "Theta: Tracking error.\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "Theta: Got lapped on the ring.\n");

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "Theta: Gap in sequence numbers.\n");

      if ( rc == GET_MISS )
         logit( "et", "Theta: Missed messages.\n");

      if ( rc == GET_TOOBIG )
      {
         logit( "et", "Theta: Retrieved message too big (%d) for msg.\n",
                MsgLen );
         continue;
      }
	  
/* Put hypocenter into temp structure (NOTE: No byte-swapping is performed, so 
   there will be trouble getting hypos from an opposite order machine)
   *******************************************************************/   
      if ( HypoStruct( HIn, &HStructT ) < 0 )
      {
         logit( "t", "Problem in HypoStruct function - 1\n" );
         continue;
      }                              
      if ( Gparm.iDebug )
         logit( "t", "HThread: found hypocenter %s-%d Mwp=%f\n",
                      HStructT.szQuakeID, HStructT.iVersion, HStructT.dMwpAvg );   

/* Process Theta if this is a new location and of sufficient magnitude/# stns.
   ***************************************************************************/
      time( &now );
      if ( (strcmp(  HStructT.szQuakeID, szQuakeID ) ||
           (!strcmp( HStructT.szQuakeID, szQuakeID ) &&
                     HStructT.iVersion >= iVersion + 2)) &&
                     HStructT.dMwpAvg  >  Gparm.dMagThreshForAuto &&
                     HStructT.iNumPs   >= Gparm.iNumStnForAuto &&
                    (long) now-(long) HStructT.dOriginTime <= 1800 ) 
      {
/* Put hypocenter into global structure (NOTE: No byte-swapping is performed, so 
   there will be trouble getting hypos from an opposite order machine)
   *******************************************************************/   
         RequestSpecificMutex( &mutsem );
         if ( HypoStruct( HIn, &HStruct ) < 0 )
         {
            logit( "t", "Problem in HypoStruct function - 2\n" );
            ReleaseSpecificMutex( &mutsem );
            continue;
         }
         strcpy( szQuakeID, HStruct.szQuakeID );
         iVersion = HStruct.iVersion;
         logit( "t", "Theta: Compute: %s-%d\n", HStruct.szQuakeID, iVersion );
         CompTheta( &HStruct, StaArray, 1, Nsta, Gparm.szLocFilePath, 
                     Gparm.iDebug, Gparm.szDiskPFile, Gparm.dMinDelta, 
                     Gparm.dMaxDelta, Gparm.iWindowLength, Gparm.dFiltLo, 
                     Gparm.dFiltHi, 0 );
         ReleaseSpecificMutex( &mutsem );                      /* Release sem */

/* Output results and Compute average theta */
         WriteThetaFile( Nsta, StaArray, Gparm.szThetaFile, &HStruct );
         CalcAverageTheta( &HStruct, StaArray, Nsta );
         if ( fabs( HStruct.dTheta > 0.01 ) ) 
            if ( PatchDummyWithTheta( &HStruct, Gparm.szDummyFile ) == 0 )
               logit( "t", "Write dummy file error in Theta-1\n" );
         if ( HStruct.dTheta        <= -6.0 && HStruct.iNumTheta  > 5 && 
              HStruct.dPreferredMag >   6.0 && now-lLastAlarmTime > 300 ) 
         {                              /* Only trigger alarm every 5 minutes */
            LogAlarm( Gparm.szAlarmFile );
            lLastAlarmTime = now;
         }
      }
   }
   iRunning = 0;                            /* Flag to release memory in main */

   return THR_NULL_RET;
}

      /*********************************************************
       *              ThThread()                               *
       *                                                       *
       *  This thread checks for events set in other modules   *
       *  to force a new ProcessTheta (and stops any existing  *
       *  processes).                                          *
       *                                                       *
       *  Its purpose is to let an external program trigger    *
       *  Theta processing.  Windows semaphore commands        *
       *  are used throughout as named events are not supported*
       *  in sema_ew.c.                                        *
       *                                                       *
       *********************************************************/
	   
#ifdef _WINNT
thr_ret ThThread( void *dummy )
{
   double   dMaxTime;         /* Max time needed for energy calculation */
   static double dTime;       /* Time of file to read */
   HYPO     HypoD;            /* Hypocenter structure from dummy file */
   HYPO     HypoT[MAX_QUAKES];/* Array of previously located hypocenters */
   int      i, j;
   int      iDisk;            /* 0 -> All data is resident on local buffer */
                              /* 1 -> Need data older than buffer, use disk */
   int      iFlag;            /* Flag to set to break out of read loop */
   int      iNumStnDisk;      /* Number of stations in disk file */
   int      iInit;            /* Flag for disk read - Initialize array */
   int      iReturn;          /* Return from disk read */
   time_t   lTime;            /* 1/1/70 present time */
   long     RawBufl;          /* Data buffer size */
   STATION *StaArrayDisk;     /* Disk Station data array */
   char     szDir[64];        /* Data file path */
   char     szFile[MAX_FILE_SIZE];/* File Name which should contain disk data */

   sleep_ew( 10000 );         /* Let things get going */
   StaArrayDisk = NULL;

/* Loop forever, waiting for a message to Process Theta */
   for( ;; )
   {
      WaitForSingleObject( hEventTheta, INFINITE );

/* Get the event hypocenter parameters. */
      ReadDummyData( &HypoD, Gparm.szDummyFile );
      for( i=0; i<32; i++ ) HypoD.szQuakeID[i] = '\0';
      HypoD.iVersion = 0;
      
/* Is this quake in the oldquake file? */
      LoadHypo( Gparm.szQuakeFile, HypoT, MAX_QUAKES );
      for ( i=0; i<MAX_QUAKES; i++ ) 
      {
         if ( IsItSameQuake( &HypoD, &HypoT[i] ) == 1 )
         {
            HypoD.iVersion = HypoT[i].iVersion; 
            strcpy( HypoD.szQuakeID, HypoT[i].szQuakeID );
            break;
         }
      }
      if ( i == MAX_QUAKES ) 
         logit( "et", "Theta Did not find quake in old quake file" );

/* Process the data for Theta */
      logit( "et", "Theta processing started in ThThread for QuakeID: %s Version: %d\n",
             HypoD.szQuakeID, HypoD.iVersion );
      iDisk = 0;
      for ( i=0; i<Nsta; i++ ) 
/* Do we need to get data off disk for processing? */
         if ( HypoD.dOriginTime < StaArray[i].dStartTime ) 
         {
            logit( "et", "%s :: Time dOTime-MMBG: %f dStartTime: %f\n",
                   StaArray[i].szStation, HypoD.dOriginTime-MM_BACKGROUND_TIME, 
                   StaArray[i].dStartTime);
            iDisk = 1;
            break;
         }
	 
/* If data is in local buffer, process Theta with buffer data */
      if ( iDisk == 0 )
      { 
         logit( "et", "Get data for Theta from buffer\n" );

/* Intialize arrays and files */
         RequestSpecificMutex( &mutsem );
         for ( i=0; i<Nsta; i++ ) InitP( &StaArray[i] );

/* Call function to compute theta for all sites */
         CompTheta( &HypoD, StaArray, 0, Nsta, Gparm.szLocFilePath, 
                     Gparm.iDebug, Gparm.szDiskPFile, Gparm.dMinDelta, 
                     Gparm.dMaxDelta, Gparm.iWindowLength, Gparm.dFiltLo, 
                     Gparm.dFiltHi, 0 );
         ReleaseSpecificMutex( &mutsem );                      /* Release sem */

/* Output results and Compute average theta */
         WriteThetaFile( Nsta, StaArray, Gparm.szThetaFile, &HypoD );
         CalcAverageTheta( &HypoD, StaArray, Nsta );
         if ( fabs( HypoD.dTheta > 0.01 ) )
            if ( PatchDummyWithTheta( &HypoD, Gparm.szDummyFile ) == 0 )
               logit( "t", "Write dummy file error in Theta-2\n");
         logit( "et", "Theta processing done in ThThread - all in buffer\n" );
         SetEvent( hEventThetaDone );           /* Respond to calling program */
      }
      else                                              /* Get data from disk */
      {
         if ( StaArrayDisk != NULL ) 
         {
            for( i=0; i<iNumStnDisk; i++ ) 
               if ( StaArrayDisk[i].plRawCircBuff != NULL ) 
               {
                  free( StaArrayDisk[i].plRawCircBuff );
                  free( StaArrayDisk[i].plFiltCircBuff );
               }
            free( StaArrayDisk );
         }
         logit( "et", "Get data for Theta from disk - is older than buffer\n" );
		      
/* Try data file path, first; then try archive data path. */
         strcpy( szDir, Gparm.szDataDirectory );

/* First, find out how many stations are in the disk file */
         dTime  = HypoD.dOriginTime;
         if ( CreateFileName( HypoD.dOriginTime, Gparm.iFileSize, szDir, 
                              Gparm.szFileSuffix, szFile ) == 0 )
         {
            logit( "et", "File name not created-1\n" );
            SetEvent( hEventThetaDone );        /* Respond to calling program */
            goto EndOfFor;
         }
         iNumStnDisk = GetNumStnsInFile( szFile );
         if ( iNumStnDisk < 0 )                      /* Try archive directory */
         {
            logit( "et", "Problem with file read: %s, try archive\n", szFile );
            strcpy( szDir, Gparm.szArchiveDir );
            if ( CreateFileName( HypoD.dOriginTime, Gparm.iFileSize, szDir, 
                                 Gparm.szFileSuffix, szFile ) == 0 )
            {
               logit( "et", "File name not created-2\n" );
               SetEvent( hEventThetaDone );     /* Respond to calling program */
               goto EndOfFor;
            }
            iNumStnDisk = GetNumStnsInFile( szFile );
            if ( iNumStnDisk < 0 )                /* Get out of here; no data */
            {
               logit( "et", "Problem with file read: File %s\n", szFile );
               SetEvent( hEventThetaDone );     /* Respond to calling program */
               goto EndOfFor;
            }
         }
         if ( Gparm.iDebug > 0 ) logit( "", "File=%s, NumStn=%d\n",
                                             szFile, iNumStnDisk );

/* Next, allocate the memory for the Disk StaArray and read in disk header */
         if ( iNumStnDisk > MAX_STATIONS )         /* Give log file head's up */ 
            logit( "et", "Too many stations in file - %s\n", iNumStnDisk );
         if ( StaArrayDisk == NULL ) 
            StaArrayDisk = (STATION *) calloc( iNumStnDisk, sizeof( STATION ) );
         if ( StaArrayDisk == NULL )
         {
            logit( "et", "Cannot allocate the station array\n" );
            SetEvent( hEventThetaDone );        /* Respond to calling program */
            goto EndOfFor;
         }
         if ( ReadDiskHeader( szFile, StaArrayDisk, iNumStnDisk ) < 0 )
         {
            logit( "et", "Problem with LP header read: File %s\n", szFile );
            free( StaArrayDisk );
            SetEvent( hEventThetaDone );        /* Respond to calling program */
            goto EndOfFor;
         }
/* Allocate memory for data buffers within StaArrayDisk */
         for ( i=0; i<iNumStnDisk; i++ )
         {
            StaArrayDisk[i].lRawCircSize = (long) (StaArrayDisk[i].dSampRate * 
             (double) Gparm.iMinutesInBuff*60.+0.1);
            RawBufl = sizeof (int32_t) * StaArrayDisk[i].lRawCircSize;
            if ( StaArrayDisk[i].plRawCircBuff == NULL ) 
               StaArrayDisk[i].plRawCircBuff = 
                (int32_t *) malloc( (size_t) RawBufl );
            if ( StaArrayDisk[i].plRawCircBuff == NULL )
            {
               logit( "t", "Can't allocate raw circ buffer for %s\n",
                      StaArrayDisk[i].szStation );
               free( StaArrayDisk );
               for ( j=0; j<i; j++ )
                  free( StaArrayDisk[j].plRawCircBuff );               
               SetEvent( hEventThetaDone );     /* Respond to calling program */
               goto EndOfFor;
            }
            if ( StaArrayDisk[i].plFiltCircBuff == NULL ) 
               StaArrayDisk[i].plFiltCircBuff = 
                (int32_t *) malloc( (size_t) RawBufl );
            if ( StaArrayDisk[i].plFiltCircBuff == NULL )
            {
               logit( "t", "Can't allocate filt circ buffer for %s\n",
                      StaArrayDisk[i].szStation );
               free( StaArrayDisk );
               for ( j=0; j<iNumStnDisk; j++ )
                  free( StaArrayDisk[j].plRawCircBuff );               
               for ( j=0; j<i; j++ )
                  free( StaArrayDisk[j].plFiltCircBuff );               
               SetEvent( hEventThetaDone );     /* Respond to calling program */
               goto EndOfFor;
            }
         }

/* Load up response info for new station array */
         LoadResponseDataAll( StaArrayDisk, Gparm.szResponseFile, iNumStnDisk );

/* Get max time needed for Theta energy calculation and initialize some stuff.
   Max possible P-time is assumed to be 12 minutes travel time.  This keeps us
   in the direct P-phase without getting any core-mantle diffraction. */
         dMaxTime = HypoD.dOriginTime + (double) Gparm.iWindowLength + 
                    (double) Gparm.iWindowLength*TAPER_WIDTH + 12.*60.;
         for ( i=0; i<iNumStnDisk; i++ )
         {
            InitP( &StaArrayDisk[i] );
            memset( StaArrayDisk[i].plRawCircBuff, 0, 
                    sizeof (int32_t) * StaArrayDisk[i].lRawCircSize );
            memset( StaArrayDisk[i].plFiltCircBuff, 0, 
                    sizeof (int32_t) * StaArrayDisk[i].lRawCircSize );
         }
                        
/* Read in all the data */
         iInit = 1;
         iFlag = 0;
         while ( dTime < (dMaxTime + (double) Gparm.iFileSize*60.) && 
                 iFlag == 0 )
         {                                            
            iReturn = ReadDiskDataNew( szFile, StaArrayDisk, iNumStnDisk, 
                                       iInit, 0, 0, 0, 1., 10., 1. );
            if ( iReturn == -1 )
            {                                                /* Bad data read */
               logit( "et", "Problem in disk file read - %s\n", szFile );
               for ( i=0; i<iNumStnDisk; i++ )
               {
                  free( StaArrayDisk[i].plRawCircBuff );
                  free( StaArrayDisk[i].plFiltCircBuff );
               }
               free( StaArrayDisk );
               SetEvent( hEventThetaDone );     /* Respond to calling program */
               goto EndOfFor;
            }
            if ( iReturn == 1 )
            {                                               /* File not found */
               logit( "et", "File not there - %s\n", szFile );
               break;
            }
            if ( Gparm.iDebug > 0 ) logit( "", "File %s read in\n", szFile );
            iInit = 0;
            dTime += (double) Gparm.iFileSize * 60.;
/* See if this file could have been created */
            time( &lTime );
            if ( dTime > ((double) lTime + ((double) Gparm.iFileSize*60.)) )
               iFlag = 1;
            CreateFileName( dTime, Gparm.iFileSize, szDir, 
                            Gparm.szFileSuffix, szFile );
/* Is there enough room in buffer for read? */
            for ( i=0; i<iNumStnDisk; i++ )
               if ( StaArrayDisk[i].lSampIndexR + (double) Gparm.iFileSize*
                    StaArrayDisk[i].dSampRate > StaArrayDisk[i].lRawCircSize )
               {
                  logit( "", "Disk Read will overwrite array bound - %s\n",
                             StaArrayDisk[i].szStation );
                  iFlag = 1;
               }
         }

/* Find where the data really ends for each stations */
         for ( i=0; i<iNumStnDisk; i++ ) 
         { 
            ComputeDC( &StaArrayDisk[i], 
                       (long) (PRE_P_TIME*StaArrayDisk[i].dSampRate + 0.0001) );
            FindDataEndSWD( &StaArrayDisk[i] );    
         }

/* Call function to compute theta for all sites */
         CompTheta( &HypoD, StaArrayDisk, 0, iNumStnDisk, Gparm.szLocFilePath, 
                     Gparm.iDebug, Gparm.szDiskPFile, Gparm.dMinDelta, 
                     Gparm.dMaxDelta, Gparm.iWindowLength, Gparm.dFiltLo, 
                     Gparm.dFiltHi, 1 );

/* Output results and Compute average theta */
         WriteThetaFile( iNumStnDisk, StaArrayDisk, Gparm.szThetaFile, &HypoD );
         CalcAverageTheta( &HypoD, StaArrayDisk, iNumStnDisk );
         if ( fabs( HypoD.dTheta > 0.01 ) )
            if ( PatchDummyWithTheta( &HypoD, Gparm.szDummyFile ) == 0 )
               logit( "t", "Write dummy file error in Theta-3\n");
         logit( "et", "Theta processing done in ThThread - disk\n" );

/* Set event and free memory */
         SetEvent( hEventThetaDone );           /* Respond to calling program */                     
         for ( i=0; i<iNumStnDisk; i++ )
         {
            free( StaArrayDisk[i].plRawCircBuff );
            StaArrayDisk[i].plRawCircBuff = NULL;
            free( StaArrayDisk[i].plFiltCircBuff );
            StaArrayDisk[i].plFiltCircBuff = NULL;
         }
         free( StaArrayDisk );
         StaArrayDisk = NULL;
      }                 
      EndOfFor: ;
   }

   return THR_NULL_RET;
}
#endif

      /*********************************************************
       *                     WThread()                         *
       *                                                       *
       *  This thread gets earthworm waveform messages.        *
       *                                                       *
       *********************************************************/
	   
thr_ret WThread( void *dummy )
{
   AZIDELT       Azi;
   int           i, iTemp;
   LATLON        ll;
   char          line[40];        /* Heartbeat message */
   int           lineLen;         /* Length of heartbeat message */
   MSG_LOGO      logo;            /* Logo of retrieved msg */
   long          MsgLen;          /* Size of retrieved message */
   long          RawBufl;         /* Raw data buffer size (for Mwp) */

/* Loop to read waveform messages
   ******************************/
   while ( tport_getflag( &Gparm.InRegion ) != TERMINATE )
   {
      long    lGapSize;         /* Number of missing samples (integer) */
      static  time_t  now;      /* Current time */
      int     rc;               /* Return code from tport_getmsg() */
      static  STATION *Sta;     /* Pointer to the station being processed */
      char    szType[3];        /* Incoming data format type */
	  
/* Send a heartbeat to the transport ring
   **************************************/
      time( &now );
      if ( (now - then) >= Gparm.iHeartbeatInt )
      {
         then = now;
         sprintf( line, "%ld %d\n", (long) now, (int)myPid );
         lineLen = (int)strlen( line );
         if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) !=
              PUT_OK )
         {
            logit( "et", "Theta: Error sending heartbeat." );
            break;
         }
      }
      
/* Get a waveform from transport region
   ************************************/
      rc = tport_getmsg( &Gparm.InRegion, &getlogoW, 1, &logo, &MsgLen,
                         WaveBuf, MAX_TRACEBUF_SIZ);

      if ( rc == GET_NONE )
      {
         sleep_ew( 100 );
         continue;
      }

      if ( rc == GET_NOTRACK )
         logit( "et", "Theta: Tracking error.\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "Theta: Got lapped on the ring.\n");

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "Theta: Gap in sequence numbers.\n");

      if ( rc == GET_MISS )
         logit( "et", "Theta: Missed messages.\n");
                            
      if ( rc == GET_TOOBIG )
      {
         logit( "et", "Theta: Retrieved message too big (%d) for msg.\n", MsgLen );
         continue;
      }                

/* If necessary, swap bytes in the message
   ***************************************/
      if ( WaveMsg2MakeLocal( WaveHead ) < 0 )
      {
         logit( "et", "Theta: Unknown waveform type.\n" );
         continue;
      }
      if ( strlen( WaveHead->loc) == 0 ) strcpy( WaveHead->loc, "--" );
      else
      {
         iTemp = atoi( WaveHead->loc );
         if ( iTemp < 0 || iTemp > 99 ) strcpy( WaveHead->loc, "--" );
      }
	  
/* If sample rate is 0, get out of here before it kills program
   ************************************************************/
      if ( WaveHead->samprate == 0. )
      {
         logit( "", "Sample rate=0., %s %s\n", WaveHead->sta, WaveHead->chan );
         continue;
      }
      
 /* If we are in the ATPlayer version of Theta, see if we should re-init
   *********************************************************************/
      if ( strlen( Gparm.szATPLineupFileBB ) > 2 )             /* Then we are */
         if ( fabs( WaveHead->starttime-(int) dLastEndTime ) > 1800. ) 
         {  /* Big gap */
            RequestSpecificMutex( &mutsem );
            for ( i=0; i<Nsta; i++ ) free( StaArray[i].plRawCircBuff );
	    free( StaArray );                                  
            Nsta = ReadLineupFile( Gparm.szATPLineupFileBB, &StaArray, 
                                   MAX_STATIONS );
            logit( "", "Nsta=%d\n", Nsta );
            if ( Nsta < 2 )
            {
               logit( "", "Bad Lineup File %s\n", Gparm.szATPLineupFileBB );
               ReleaseSpecificMutex( &mutsem );
               continue;
            }	    
            ReadDummyData( &HStruct, Gparm.szDummyFile );
            for ( i=0; i<Nsta; i++ )       /* Fill lat/lon part of structure */
            {    
/* Read Station data file and match up with list
   *********************************************/      
               StaArray[i].dTimeCorrection *= (-1.);
               if ( LoadStationData( &StaArray[i], Gparm.szStaDataFile ) == -1 )
               {
                  logit( "et", "No match for scn in station info file.\n" );
                  logit( "e", "file: %s\n", Gparm.szStaDataFile );
                  logit( "e", "scn = :%s:%s:%s:%s: \n", StaArray[i].szStation,
                   StaArray[i].szChannel, StaArray[i].szNetID,
                   StaArray[i].szLocation );    
               }
               InitP( &StaArray[i] );
               ll.dLat = StaArray[i].dLat;
               ll.dLon = StaArray[i].dLon;
               GeoCent( &ll );
               GetLatLonTrig( &ll );
               GetDistanceAz( (LATLON *) &HStruct, &ll, &Azi );
               StaArray[i].dDelta   = Azi.dDelta;
               StaArray[i].dAzimuth = Azi.dAzimuth;
            }		 
/* Load up response info for new station array */
            LoadResponseDataAll( StaArray, Gparm.szResponseFile, Nsta );
            ReleaseSpecificMutex( &mutsem );
         } 	 

/* Look up SCN number in the station list
   **************************************/
      RequestSpecificMutex( &mutsem );
      Sta = NULL;								  
      for ( i=0; i<Nsta; i++ )
         if ( !strcmp( WaveHead->sta,  StaArray[i].szStation ) &&
              !strcmp( WaveHead->chan, StaArray[i].szChannel ) &&
              !strcmp( WaveHead->net,  StaArray[i].szNetID ) )
         {
            Sta = (STATION *) &StaArray[i];
            break;
         }

      if ( Sta == NULL )      /* SCN not found */
      {
         ReleaseSpecificMutex( &mutsem );
         continue;
      }
		 
/* Check if the time stamp is reasonable.  If it is ahead of the present
   1/1/70 time, it is not reasonable. (+1. to account for int).
   *********************************************************************/
      if ( WaveHead->endtime > (double) now+1. )
      {
         ReleaseSpecificMutex( &mutsem );
         continue;
      }

/* Do this the first time we get a message with this SCN
   *****************************************************/
      if ( Sta->iFirst == 1 )
      {
         logit( "", "Init %s %s %s %s \n", Sta->szStation, Sta->szChannel,
                                           Sta->szNetID,   Sta->szLocation );
         Sta->iFirst      = 0;
         Sta->dEndTime    = WaveHead->starttime - 1./WaveHead->samprate;
         Sta->dStartTime  = WaveHead->starttime;
         Sta->dSampRate   = WaveHead->samprate;
         Sta->dAveLDCRaw  = 0.;
         Sta->lSampIndexR = 0;
         ResetFilter( Sta );
		 		 
/* Allocate memory for raw circular buffer (let the buffer be big enough to
   hold MinutesInBuff data samples)
   ************************************************************************/
         Sta->lRawCircSize =
          (long) (WaveHead->samprate*(double)Gparm.iMinutesInBuff*60.+0.1);
         RawBufl = sizeof (int32_t) * Sta->lRawCircSize;
         Sta->plRawCircBuff = (int32_t *) malloc( (size_t) RawBufl );
         if ( Sta->plRawCircBuff == NULL )
         {
            logit( "et", "Theta: Can't allocate raw circ buffer for %s\n",
                         Sta->szStation );
            ReleaseSpecificMutex( &mutsem );
            continue;
         }
      }
      
/* If data is not in order, throw it out
   *************************************/
      if ( Sta->dEndTime >= WaveHead->starttime )
      {
         if ( Gparm.iDebug )
            logit( "e", "%s %s %s %s out of order\n", Sta->szStation,
             Sta->szChannel, Sta->szNetID, Sta->szLocation );
         ReleaseSpecificMutex( &mutsem );
         continue;
      }

/* If the samples are shorts, make them longs
   ******************************************/
      strcpy( szType, WaveHead->datatype );
      if ( (strcmp( szType, "i2" ) == 0) || (strcmp( szType, "s2") == 0) )
         for ( i=WaveHead->nsamp-1; i>-1; i-- )
            WaveLong[i] = (int32_t) WaveShort[i];
			
/* Compute the number of samples since the end of the previous message.
   If (lGapSize == 1), no data has been lost between messages.
   If (1 < lGapSize <= 2), go ahead anyway.
   If (lGapSize > 2), re-initialize filter variables.
   *******************************************************************/
      lGapSize = (long) (WaveHead->samprate *
                        (WaveHead->starttime-Sta->dEndTime) + 0.5);

/* Announce gaps
   *************/
      if ( lGapSize > 2 )
      {
         int      lnLen;
         time_t   errTime;
         char     errmsg[80];
         MSG_LOGO lgo;

         time( &errTime );
         sprintf( errmsg,
               "%ld 1 Found %4ld sample gap. Restarting station %-5s%-2s%-3s %s \n",
               (long) errTime, lGapSize, Sta->szStation, Sta->szNetID,
               Sta->szChannel, Sta->szLocation );
         lnLen = (int)strlen( errmsg );
         lgo.type   = Ewh.TypeError;
         lgo.mod    = Gparm.MyModId;
         lgo.instid = Ewh.MyInstId;
         tport_putmsg( &Gparm.InRegion, &lgo, lnLen, errmsg );
         if ( Gparm.iDebug )
            logit( "t", "Theta: Restarting %-5s%-2s %-3s %s. lGapSize = %d\n",
                   Sta->szStation, Sta->szNetID, Sta->szChannel,
                   Sta->szLocation, lGapSize );

/* For big gaps reset filter 
   *************************/
         ResetFilter( Sta );
         Sta->dSumLDCRaw = 0.;
         Sta->dAveLDCRaw = 0.;
         Sta->dSumLDC    = 0.;
         Sta->dAveLDC    = 0.;
      }

/* For gaps less than the size of the buffer, pad buffers with DC
   **************************************************************/
      if ( lGapSize > 1 && lGapSize <= 2 )
         PadBuffer( lGapSize, Sta->dAveLDCRaw, &Sta->lSampIndexR,
                    Sta->plRawCircBuff, Sta->lRawCircSize );
	  
/* For gaps greater than the size of the buffer, re-init
   *****************************************************/
      if ( lGapSize >= Sta->lRawCircSize )
      {
         Sta->dSumLDCRaw  = 0.;
         Sta->dAveLDCRaw  = 0.;
         Sta->dSumLDC     = 0.;
         Sta->dAveLDC     = 0.;
         Sta->lSampIndexR = 0;
      }
      
/* In this module, lEndData is the first buffer index in this packet
   *****************************************************************/      
      Sta->lEndData = Sta->lSampIndexR;

/* Compute DC offset for raw data
   ******************************/
      GetLDC( WaveHead->nsamp, WaveLong, &Sta->dAveLDCRaw, Sta->lFiltSamps );
			
/* Tuck raw data into proper location in buffer
   ********************************************/			
      PutDataInBuffer( WaveHead, WaveLong, Sta->plRawCircBuff,
                       &Sta->lSampIndexR, Sta->lRawCircSize );
	  
/* Save time of the end of the current message and determine time of oldest
   data in buffer (dStartTime).
   ************************************************************************/
      Sta->dEndTime = WaveHead->endtime;
      if ( WaveHead->endtime > dLastEndTime+5.0 ) 
         dLastEndTime = WaveHead->endtime;       /* For ATPlayer restarts */
      if ( Sta->dStartTime < (Sta->dEndTime - ((double)
	   Sta->lRawCircSize/Sta->dSampRate) + 1./Sta->dSampRate) ) 
        Sta->dStartTime = Sta->dEndTime - ((double)
         Sta->lRawCircSize/Sta->dSampRate) + 1./Sta->dSampRate;

      ReleaseSpecificMutex( &mutsem );
   }   
   iRunning = 0;                            /* Flag to release memory in main */

   return THR_NULL_RET;
}

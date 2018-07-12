       /****************************************************************
       *  Ms_threads.c                                                 *
       *                                                               *
       *  This file contains the thread routines used by Ms.           *
       *                                                               *
       *  This code is based on thread routines used in                *
       *  WCATWC Earlybird module lpproc.                              *
       *                                                               *
       *  2011: Paul Whitmore, NOAA-WCATWC - paul.whitmore@noaa.gov    *
       *                                                               *
       ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <transport.h>
#include <earthworm.h>
#include <swap.h>
#include "Ms.h"

/* Global variable defined in Main */
extern double  dLastEndTime;   /* End time of last packet */
extern EWH     Ewh;            /* Parameters from earthworm.h */
extern MSG_LOGO getlogoW;      /* Logo of requested waveforms */
extern GPARM   Gparm;          /* Configuration file parameters */
extern HANDLE  hEventMs;       /* Event shared externally to trigger LP proc. */
extern HANDLE  hEventMsDone;   /* Event shared externally to signal proc. done*/
extern MSG_LOGO hrtlogo;       /* Logo of outgoing heartbeats */
extern HYPO    HStruct;        /* Hypocenter data structure */
extern int     iLPProcessing;  /* 0->no LP processing going on, 1-> LP going */
extern int     iRunning;       /* 1-> Threads are operating A-OK; 0-> Stop */
extern mutex_t mutsem;         /* Semaphore to protect StaArray adjustments */
extern pid_t   myPid;          /* Process id of this process */
extern int     Nsta;           /* Number of stations to process */
extern STATION *StaArray;      /* Station data array */
extern time_t  then;           /* Previous heartbeat time */
extern char    *WaveBuf;       /* Pointer to waveform buffer */
extern TRACE2_HEADER *WaveHead;/* Pointer to waveform header */
extern int32_t *WaveLong;      /* Long pointer to waveform data */
extern int32_t *WaveLongF;     /* Filtered Waveform data with DC removed */
extern short   *WaveShort;     /* Short pointer to waveform data */

      /*********************************************************
       *                     HThread()                         *
       *                                                       *
       *  This thread gets messages from the hypocenter ring.  *
       *                                                       *
       *********************************************************/
	   
thr_ret HThread( void *dummy )
{
   AZIDELT       Azi;
   static double dMag;            /* Magnitude of quake being processed */
   MSG_LOGO      getlogoH;        /* Logo of requested picks */
   char          HIn[MAX_HYPO_SIZE];/* Pointer to hypocenter from ring */
   HYPO          HStructT;        /* Temporary Hypocenter data structure */
   HYPO          HypoD;           /* Hypocenter structure from dummy file */
   int           i;
   LATLON        ll;
   MSG_LOGO      logo;            /* Logo of retrieved msg */
   long          MsgLen;          /* Size of retrieved message */
   time_t        now;             /* Current time */

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
         logit( "et", "MsT: Tracking error.\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "MsT: Got lapped on the ring.\n");

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "MsT: Gap in sequence numbers.\n");

      if ( rc == GET_MISS )
         logit( "et", "MsT: Missed messages.\n");

      if ( rc == GET_TOOBIG )
      {
         logit( "et", "MsT: Retrieved message too big (%d) for msg.\n",
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
      	  
/* Process Ms if this is a new location and of sufficient magnitude/# stns - or
   is a new quake bigger than previous event.
   ****************************************************************************/
      time( &now );
      if ( ((iLPProcessing          == 0 &&           /* New location - Start */
             HStructT.dPreferredMag >= Gparm.dMagThreshForAuto &&
             HStructT.iNumPs        >= Gparm.iNumStnForAuto) ||
            (iLPProcessing          == 1 &&           /* Bigger quake - Start */
             HStructT.dPreferredMag >= dMag &&
             HStructT.iNumPs        >= Gparm.iNumStnForAuto)) &&
             (long) now-(long) HStructT.dOriginTime <= 1800 ) 
      {
/* First, stop processing if already going */
         if ( iLPProcessing == 1 )           /* Stop it so we can start again */
         {
            RequestSpecificMutex( &mutsem );
            for ( i=0; i<Nsta; i++ ) 
            {
               StaArray[i].iPickStatus = 0;
               StaArray[i].dMSMag = 0.;
               StaArray[i].lHit = 1;      /* lHit = 1 for Init, 0 for no init */
            }
            iLPProcessing = 0;
            logit( "et", "Ms processing stopped in HThread so it can start\n" );
            ReleaseSpecificMutex( &mutsem );
         }
         logit( "et", "Start Ms - %s %d\n",
                HStructT.szQuakeID, HStructT.iVersion );
         RequestSpecificMutex( &mutsem );
         for ( i=0; i<Nsta; i++ )
            if ( StaArray[i].iUseMe >= 0 )
            {
               InitP( &StaArray[i] );
               StaArray[i].iUseMe = 0;
               ll.dLat = StaArray[i].dLat;
               ll.dLon = StaArray[i].dLon;
               GeoCent( &ll );
               GetLatLonTrig( &ll );
               GetDistanceAz( (LATLON *) &HStructT, &ll, &Azi );
               StaArray[i].dDelta   = Azi.dDelta;
               StaArray[i].dAzimuth = Azi.dAzimuth;
            }
			
/* iPickStatus: 1->waiting on data in the Rayleigh wave window */
         for ( i=0; i<Nsta; i++ )
         {
            StaArray[i].iPickStatus = 0;
            StaArray[i].lHit = 1;   /* lHit = 1 for Init, 0 for no init */
         }
	  
/* Put hypocenter into global structure (NOTE: No byte-swapping is performed, so 
   there will be trouble getting hypos from an opposite order machine)
   *******************************************************************/   
         if ( HypoStruct( HIn, &HStruct ) < 0 )
         {
            logit( "t", "Problem in HypoStruct function - 2\n" );
            ReleaseSpecificMutex( &mutsem );
            continue;
         }
         iLPProcessing = 1;
         ComputePRTimeWindows( &HStruct, StaArray, Nsta, 1 );
         ReadDummyData( &HypoD, Gparm.szDummyFile ); 
         for ( i=0; i<Nsta; i++ )
         {
/* Is there data in this buffer covering the R-wave window? */
            if ( StaArray[i].dRStartTime < StaArray[i].dEndTime &&
                 StaArray[i].dREndTime   > StaArray[i].dStartTime ) 
            {		 
/* Process the data */
               if ( GetMs( &StaArray[i], 0, &HStruct, &Gparm, 1 ) == 1 )
               {
                  ReportPick( &StaArray[i], Gparm.ucMyModId,
                   Gparm.PRegion, Ewh.TypePickTWC, Ewh.MyInstId, 1 );
                  WriteLPFile( Nsta, StaArray, Gparm.szLPRTFile, &HStruct );
                  HStruct.dMSAvg = ComputeAvgMS( Nsta, StaArray, &HStruct.iNumMS );
/* If this is the same hypo as in the dummy file, update the Ms */				  
                  if ( IsItSameQuake( &HStruct, &HypoD ) == 1 )
                     if ( PatchDummyWithLP( &HStruct, Gparm.szDummyFile ) == 0 )
                        logit( "t", "Write dummy file error in Ms-1\n" );              
                  logit( "", "Ms reported for %s in HThread\n", 
                     StaArray[i].szStation );
               }
               StaArray[i].lHit = 0;   /* lHit = 1 for Init, 0 for no init */
            }
            else if ( StaArray[i].dRStartTime > StaArray[i].dEndTime )
               StaArray[i].iPickStatus = 1;
         }
         for ( i=0; i<Nsta; i++ )
            if ( StaArray[i].iPickStatus == 1 ) goto LoopEnd;
/* If we get here then all Ms processing was finished */
         for ( i=0; i<Nsta; i++ ) 
         {
            StaArray[i].iPickStatus = 0;
            StaArray[i].dMSMag = 0.;
            StaArray[i].lHit = 1;        /* lHit = 1 for Init, 0 for no init */
         }
         iLPProcessing = 0;
         logit( "et", "LP processing completed in HThread\n" );
LoopEnd:
         dMag = HStruct.dPreferredMag;
         ReleaseSpecificMutex( &mutsem );
      }
   }

  return THR_NULL_RET;
}

      /*********************************************************
       *             RTLPThread()                              *
       *                                                       *
       *  This thread checks for events set in other modules   *
       *  to force a new ProcessLP (and stops any existing     *
       *  processes).                                          *
       *                                                       *
       *  Its purpose is to let an external program trigger    *
       *  LP processing.  Windows specific semaphore commands  *
       *  are used throughout as named events are not supported*
       *  in sema_ew.c.                                        *
       *                                                       *
       *********************************************************/
	   
#ifdef _WINNT
thr_ret RTLPThread( void *dummy )
{
   static double dMaxRTime;   /* Maximum R-wave end time */
   static double dTime;       /* Time of file to read */
   FILE    *hFile;            /* File handle */
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

/* Loop forever, waiting for a message to Process Ms */
   for( ;; )
   {
      WaitForSingleObject( hEventMs, INFINITE );

/* First, stop processing if already going */
      if ( iLPProcessing == 1 )     /* Stop it so we can start again */
      {
         RequestSpecificMutex( &mutsem );
         for ( i=0; i<Nsta; i++ ) 
         {
            StaArray[i].iPickStatus = 0;
            StaArray[i].dMSMag = 0.;
            StaArray[i].lHit = 1;        /* lHit = 1 for Init, 0 for no init */
         }
         iLPProcessing = 0;
         logit( "et", "Ms processing stopped in RTLPThread so it can start\n" );
         ReleaseSpecificMutex( &mutsem );
      }

/* Intialize arrays and files */
      RequestSpecificMutex( &mutsem );
      for ( i=0; i<Nsta; i++ )
      {
         if ( StaArray[i].iUseMe >= 0 )
         {
            InitP( &StaArray[i] );
            StaArray[i].iUseMe = 0;
         }
/* iPickStatus: 1->waiting on data in the Rayleigh wave window */
         StaArray[i].iPickStatus = 0;
         StaArray[i].lHit = 1;   /* lHit = 1 for Init, 0 for no init */
      }
      hFile = fopen( Gparm.szLPRTFile, "w" );
      fclose( hFile );				  				  

/* Get the event hypocenter parameters. */
      ReadDummyData( &HStruct, Gparm.szDummyFile );
      strcpy( HStruct.szQuakeID, "" );
      HStruct.iVersion = 0;
      
/* Is this quake in the oldquake file? */
      LoadHypo( Gparm.szQuakeFile, HypoT, MAX_QUAKES );
      for ( i=0; i<MAX_QUAKES; i++ )
         if ( IsItSameQuake( &HStruct, &HypoT[i] ) == 1 )
         {
            HStruct.iVersion = HypoT[i].iVersion; 
            strcpy( HStruct.szQuakeID, HypoT[i].szQuakeID );
            break;
         }

/* Process the data for Ms */
      logit( "et", "Ms processing started in RTLPThread for %s-%d\n",
                  HStruct.szQuakeID, HStruct.iVersion );
      iDisk         = 0;
      iLPProcessing = 1;
/* Find out what time interval to evaluate for each station */
      ComputePRTimeWindows( &HStruct, StaArray, Nsta, 1 );
      for ( i=0; i<Nsta; i++ )
/* Do we need to get data off disk for processing? */
         if ( StaArray[i].dRStartTime < StaArray[i].dStartTime ) iDisk = 1;
      ReleaseSpecificMutex( &mutsem );
	 
/* If data is in local buffer, process Ms with buffer data */
      if ( iDisk == 0 )
      { 
         logit( "et", "Get data for Ms from buffer\n" );
         RequestSpecificMutex( &mutsem );
         ReadDummyData( &HypoD, Gparm.szDummyFile ); 
         for ( i=0; i<Nsta; i++ )
         {
            if ( StaArray[i].dRStartTime > StaArray[i].dEndTime )
               StaArray[i].iPickStatus = 1;  /* Data nor arrived yet */
            else         /* There is some data to process */
            {
               if ( GetMs( &StaArray[i], 0, &HStruct, &Gparm, 1 ) == 1 )
               {
                  ReportPick( &StaArray[i], Gparm.ucMyModId,
                   Gparm.PRegion, Ewh.TypePickTWC, Ewh.MyInstId, 1 );
                  WriteLPFile( Nsta, StaArray, Gparm.szLPRTFile, &HStruct );
                  HStruct.dMSAvg = ComputeAvgMS( Nsta, StaArray, &HStruct.iNumMS );
/* If this is the same hypo as in the dummy file, update the Ms */				  
                  if ( IsItSameQuake( &HStruct, &HypoD ) == 1 )
                     if ( PatchDummyWithLP( &HStruct, Gparm.szDummyFile ) == 0 )
                        logit( "t", "Write dummy file error in lpproc\n");
               }   
               StaArray[i].lHit = 0;   /* lHit = 1 for Init, 0 for no init */
            }
         }
/* See of all stations have been processed; if not, it will continue in 
   WThread as data arrives */
         for ( i=0; i<Nsta; i++ )
            if ( StaArray[i].iPickStatus == 1 ) 
            {
               logit( "et", "Ms not complete yet, passing to real-time\n" );
               goto LoopEnd;
            }
         for ( i=0; i<Nsta; i++ ) 
         {
            StaArray[i].iPickStatus = 0;
            StaArray[i].dMSMag = 0.;
            StaArray[i].lHit = 1;        /* lHit = 1 for Init, 0 for no init */
         }
         iLPProcessing = 0;
         logit( "et", "Ms processing done in RTLPThread - all in buffer\n" );
LoopEnd: SetEvent( hEventMsDone );  /* Respond to calling program */
         ReleaseSpecificMutex( &mutsem );
      }
      else       /* Get data from disk and then process en masse */
      {
         RequestSpecificMutex( &mutsem );
         logit( "et", "Get data for Ms from disk - it is older than buffer\n" );
		      
/* Try data file path, first; then try archive data path. */
         strcpy( szDir, Gparm.szDataDirectory );

/* First, find out how many stations are in the disk file */
         dTime = HStruct.dOriginTime;
         if ( CreateFileName( dTime, Gparm.iFileLengthLP, szDir, 
                              Gparm.szFileSuffix, szFile ) == 0 )
         {
            logit( "et", "Create File issue - 1\n" );
            iLPProcessing = 0;
            SetEvent( hEventMsDone );           /* Respond to calling program */
            ReleaseSpecificMutex( &mutsem );
            goto EndOfFor;
         }
         iNumStnDisk = GetNumStnsInFile( szFile );
         if ( iNumStnDisk < 0 )
         {
            logit( "et", "Problem with LP read: %s, try archive\n", szFile );
            strcpy( szDir, Gparm.szArchiveDir );
            if ( CreateFileName( dTime, Gparm.iFileLengthLP, 
                                 szDir, Gparm.szFileSuffix, szFile ) == 0 )
            {
               logit( "et", "Create File issue - 2\n" );
               iLPProcessing = 0;
               SetEvent( hEventMsDone );        /* Respond to calling program */
               ReleaseSpecificMutex( &mutsem );
               goto EndOfFor;
            }
            iNumStnDisk = GetNumStnsInFile( szFile );
            if ( iNumStnDisk < 0 )                /* Get out of here; no data */
            {
               logit( "et", "Problem with LP file read: File %s\n", szFile );
               iLPProcessing = 0;
               SetEvent( hEventMsDone );        /* Respond to calling program */
               ReleaseSpecificMutex( &mutsem );
               goto EndOfFor;
            }
         }

/* Next, allocate the memory for the Disk StaArray and read in disk header */
         if ( iNumStnDisk > MAX_STATIONS )   /* Give log a head's up */
            logit( "et", "LOTS of stations in file - %s\n", iNumStnDisk );
         StaArrayDisk = (STATION *) calloc( iNumStnDisk, sizeof( STATION ) );
         if ( StaArrayDisk == NULL )
         {
            logit( "et", "Cannot allocate the station array\n" );
            iLPProcessing = 0;
            SetEvent( hEventMsDone );  /* Respond to calling program */
            ReleaseSpecificMutex( &mutsem );
            goto EndOfFor;
         }
         if ( ReadDiskHeader( szFile, StaArrayDisk, iNumStnDisk ) < 0 )
         {
            logit( "et", "Problem with LP header read: File %s\n", szFile );
            iLPProcessing = 0;
            free( StaArrayDisk );
            SetEvent( hEventMsDone );  /* Respond to calling program */
            ReleaseSpecificMutex( &mutsem );
            goto EndOfFor;
         }

/* Allocate memory for data buffers within StaArrayDisk */
         for ( i=0; i<iNumStnDisk; i++ )
         {
            StaArrayDisk[i].lRawCircSize = (long) (StaArrayDisk[i].dSampRate * 
                                          (double)Gparm.iMinutesInBuff*60.+0.1);
            RawBufl = sizeof (int32_t) * StaArrayDisk[i].lRawCircSize;
            StaArrayDisk[i].plRawCircBuff = (int32_t *) malloc( (size_t) RawBufl );
            if ( StaArrayDisk[i].plRawCircBuff == NULL )
            {
               logit( "t", "Can't allocate raw circ buffer for %s\n",
                      StaArrayDisk[i].szStation );
               iLPProcessing = 0;
               for ( j=0; j<i; j++ )
                  free( StaArrayDisk[j].plRawCircBuff );               
               free( StaArrayDisk );
               SetEvent( hEventMsDone );  /* Respond to calling program */
               ReleaseSpecificMutex( &mutsem );
               goto EndOfFor;
            }
            StaArrayDisk[i].plFiltCircBuff = (int32_t *) malloc( (size_t) RawBufl );
            if ( StaArrayDisk[i].plFiltCircBuff == NULL )
            {
               logit( "t", "Can't allocate filtered circ buffer for %s\n",
                      StaArrayDisk[i].szStation );
               iLPProcessing = 0;
               for ( j=0; j<i-1; j++ ) 
                  free( StaArrayDisk[j].plRawCircBuff );
               free( StaArrayDisk );
               SetEvent( hEventMsDone );  /* Respond to calling program */
               ReleaseSpecificMutex( &mutsem );
               goto EndOfFor;
            }
         }

/* Get Rayleigh wave windows for disk array and save max R-wave time */
         dMaxRTime = 0.;
         ComputePRTimeWindows( &HStruct, StaArrayDisk, iNumStnDisk, 1 );
         for ( i=0; i<iNumStnDisk; i++ )
         {
            if ( StaArrayDisk[i].dREndTime > dMaxRTime ) 
               dMaxRTime = StaArrayDisk[i].dREndTime;
            InitP( &StaArrayDisk[i] );
            StaArrayDisk[i].iUseMe = 0;
            memset( StaArrayDisk[i].plRawCircBuff, 0, 
                    sizeof (int32_t) * StaArrayDisk[i].lRawCircSize );
            memset( StaArrayDisk[i].plFiltCircBuff, 0, 
                    sizeof (int32_t) * StaArrayDisk[i].lRawCircSize );
         }
/* Redo for R times after init */
         ComputePRTimeWindows( &HStruct, StaArrayDisk, iNumStnDisk, 1 );
                        
/* Read in all the data */
         iInit = 1;
         iFlag = 0;
         while ( dTime < (dMaxRTime + (double) Gparm.iFileLengthLP*60.) && 
                 iFlag == 0 )
         {                                            
            iReturn = ReadDiskDataNew( szFile, StaArrayDisk, iNumStnDisk, 
                                       iInit, 1, 1, 0, .05, 1., 1. );
            logit( "t", "Read in %s\n", szFile );
            if ( iReturn == -1 )
            {                                             /* Bad data read */
               logit( "et", "Problem in disk file read\n" );
               iLPProcessing = 0;
               for ( i=0; i<iNumStnDisk; i++ )
               {
                  free( StaArrayDisk[i].plRawCircBuff );
                  free( StaArrayDisk[i].plFiltCircBuff );
               }
               free( StaArrayDisk );
               SetEvent( hEventMsDone );  /* Respond to calling program */
               ReleaseSpecificMutex( &mutsem );
               goto EndOfFor;
            }
            if ( iReturn == 1 )
            {                                             /* File not found */
               logit( "et", "File not there - %s\n", szFile );
               break;
            }
            iInit = 0;
            dTime += (double) Gparm.iFileLengthLP * 60.;
/* See if this file could have been created */
            time( &lTime );
            if ( dTime > ((double) lTime + ((double) Gparm.iFileLengthLP*60.)) )
               iFlag = 1;
            CreateFileName( dTime, Gparm.iFileLengthLP, szDir, 
                            Gparm.szFileSuffix, szFile );
/* Is there enough room in buffer for read? */
            for ( i=0; i<iNumStnDisk; i++ )
               if ( StaArrayDisk[i].lSampIndexR + (double) Gparm.iFileLengthLP*
                    StaArrayDisk[i].dSampRate > StaArrayDisk[i].lRawCircSize )
               {
                  logit( "", "Disk Read will overwrite array bound - %s\n",
                             StaArrayDisk[i].szStation );
                  iFlag = 1;
               }
         }
                     
/* Filter the data and compute offsets */
         for ( i=0; i<iNumStnDisk; i++ )  
         {
            FilterPacket( StaArrayDisk[i].plFiltCircBuff, StaArrayDisk, 
             StaArrayDisk[i].lSampIndexR, StaArrayDisk[i].dSampRate,
             Gparm.dLowCutFilter, Gparm.dHighCutFilter,
             3.*(1./Gparm.dLowCutFilter) );
            GetLDC( (long) ((double) MS_BACKGROUND_TIME*StaArrayDisk[i].dSampRate), 
             StaArrayDisk[i].plRawCircBuff, &StaArrayDisk[i].dAveLDCRaw, 0 );
            GetLDC( (long) ((double) MS_BACKGROUND_TIME*StaArrayDisk[i].dSampRate), 
             StaArrayDisk[i].plFiltCircBuff, &StaArrayDisk[i].dAveLDC, 0 );
         }

/* Compute the Ms if S:N is large enough */
         for ( i=0; i<iNumStnDisk; i++ )  
            GetMs( &StaArrayDisk[i], 0, &HStruct, &Gparm, 1 );

         WriteLPFile( iNumStnDisk, StaArrayDisk, Gparm.szLPRTFile, &HStruct ); 
         HStruct.dMSAvg = ComputeAvgMS( iNumStnDisk, StaArrayDisk, 
                                        &HStruct.iNumMS );
/* If this is the same hypo as in the dummy file, update the Ms */				  
         ReadDummyData( &HypoD, Gparm.szDummyFile ); 
         if ( IsItSameQuake( &HStruct, &HypoD ) == 1 )
            if ( PatchDummyWithLP( &HStruct, Gparm.szDummyFile ) == 0 )
               logit( "t", "Write dummy file error in Ms-3\n");

         iLPProcessing = 0;
         logit( "et", "Ms processing done in RTLPThread - from disk\n" );
         SetEvent( hEventMsDone );  /* Respond to calling program */
         for ( i=0; i<iNumStnDisk; i++ )
         {
            free( StaArrayDisk[i].plRawCircBuff );
            free( StaArrayDisk[i].plFiltCircBuff );
         }
         free( StaArrayDisk );
         ReleaseSpecificMutex( &mutsem );
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
   HYPO          HypoD;           /* Hypocenter structure from dummy file */
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
            logit( "et", "Ms: Error sending heartbeat." );
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
         logit( "et", "Ms: Tracking error.\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "Ms: Got lapped on the ring.\n");

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "Ms: Gap in sequence numbers.\n");

      if ( rc == GET_MISS )
         logit( "et", "Ms: Missed messages.\n");
                            
      if ( rc == GET_TOOBIG )
      {
         logit( "et", "Ms: Retrieved message too big (%d) for msg.\n", MsgLen );
         continue;
      }                

/* If necessary, swap bytes in the message
   ***************************************/
      if ( WaveMsg2MakeLocal( WaveHead ) < 0 )
      {
         logit( "et", "Ms: Unknown waveform type.\n" );
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
      
 /* If we are in the ATPlayer version of Ms, see if we should re-init
   ******************************************************************/
      if ( strlen( Gparm.szATPLineupFileLP ) > 2 )     /* Then we are */
         if ( fabs( WaveHead->starttime-(int) dLastEndTime ) > 1800. ) 
         {  /* Big gap */
            for ( i=0; i<Nsta; i++ )
	    {
               free( StaArray[i].plRawCircBuff );
               free( StaArray[i].plFiltCircBuff );
            }
	    free( StaArray );                                  
            RequestSpecificMutex( &mutsem );
            Nsta = ReadLineupFile( Gparm.szATPLineupFileLP, &StaArray, 
                                   MAX_STATIONS );
            logit( "", "Nsta=%d\n", Nsta );
            if ( Nsta < 2 )
            {
               logit( "", "Bad Lineup File read %s\n",Gparm.szATPLineupFileLP );
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
               StaArray[i].iUseMe = 0;
               StaArray[i].lHit   = 0;    /* lHit = 1 for Init, 0 for no init */
               ll.dLat = StaArray[i].dLat;
               ll.dLon = StaArray[i].dLon;
               GeoCent( &ll );
               GetLatLonTrig( &ll );
               GetDistanceAz( (LATLON *) &HStruct, &ll, &Azi );
               StaArray[i].dDelta   = Azi.dDelta;
               StaArray[i].dAzimuth = Azi.dAzimuth;
            }		 
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
          Sta->szNetID, Sta->szLocation );
         Sta->iFirst = 0;
         Sta->iPickStatus = 0;
         Sta->lHit = 1;              /* lHit = 1 for Init, 0 for no init */
         Sta->dEndTime = WaveHead->starttime - 1./WaveHead->samprate;
         Sta->dStartTime = WaveHead->starttime;
         Sta->dSampRate = WaveHead->samprate;
         ResetFilter( Sta );
		 		 
/* Allocate memory for raw circular buffer (let the buffer be big enough to
   hold iMinutesInBuff data samples)
   ************************************************************************/
         Sta->lRawCircSize =
          (long) (WaveHead->samprate*(double)Gparm.iMinutesInBuff*60.+0.1);
         RawBufl = sizeof (int32_t) * Sta->lRawCircSize;
         Sta->plRawCircBuff = (int32_t *) malloc( (size_t) RawBufl );
         if ( Sta->plRawCircBuff == NULL )
         {
            logit( "et", "Ms: Can't allocate raw circ buffer for %s\n",
                         Sta->szStation );
            ReleaseSpecificMutex( &mutsem );
            continue;
         }
		 		 
/* Allocate memory for filt. circular buffer (let the buffer be big enough to
   hold iMinutesInBuff data samples)
   **************************************************************************/
         Sta->plFiltCircBuff = (int32_t *) malloc( (size_t) RawBufl );
         if ( Sta->plFiltCircBuff == NULL )
         {
            logit( "et", "Ms: Can't allocate filt circ buffer for %s\n",
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
         lgo.mod    = Gparm.ucMyModId;
         lgo.instid = Ewh.MyInstId;
         tport_putmsg( &Gparm.InRegion, &lgo, lnLen, errmsg );
         if ( Gparm.iDebug )
            logit( "t", "Ms: Restarting %-5s%-2s %-3s %s. lGapSize = %d\n",
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
      {
         PadBuffer( lGapSize, Sta->dAveLDCRaw, &Sta->lSampIndexR,
                    Sta->plRawCircBuff, Sta->lRawCircSize );
         PadBuffer( lGapSize, Sta->dAveLDC, &Sta->lSampIndexF,
                    Sta->plFiltCircBuff, Sta->lRawCircSize );
      }
	  
/* For gaps greater than the size of the buffer, re-init
   *****************************************************/
      if ( lGapSize >= Sta->lRawCircSize )
      {
         Sta->dSumLDCRaw  = 0.;
         Sta->dAveLDCRaw  = 0.;
         Sta->dSumLDC     = 0.;
         Sta->dAveLDC     = 0.;
         Sta->lSampIndexR = 0;
         Sta->lSampIndexF = 0;
         Sta->iPickStatus = 0;
         Sta->lHit        = 1;            /* lHit = 1 for Init, 0 for no init */
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
	  
/* Bandpass filter the data if specified in station file (remove DC 1st)
   ******************************************************************/
      for ( i=0; i<WaveHead->nsamp; i++ )
         WaveLongF[i] = WaveLong[i] - (int32_t) (Sta->dAveLDCRaw+0.5);
      if ( Sta->iFiltStatus == 1 )
         FilterPacket( WaveLongF, Sta, WaveHead->nsamp, WaveHead->samprate,
                       Gparm.dLowCutFilter, Gparm.dHighCutFilter,
                       3.*(1./Gparm.dLowCutFilter) );
		 
/* Compute DC offset for filtered data
   ***********************************/
      GetLDC( WaveHead->nsamp, WaveLongF, &Sta->dAveLDC, Sta->lFiltSamps );
			         
/* Tuck filtered data into proper location in buffer
   *************************************************/			
      PutDataInBuffer( WaveHead, WaveLongF, Sta->plFiltCircBuff,
                       &Sta->lSampIndexF, Sta->lRawCircSize );
	  
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

/* Process LP data if this station is still waiting on Rayleigh wave data
   **********************************************************************/
      if ( iLPProcessing == 1 )
      {
         if ( Sta->iPickStatus == 1 )
         {
/* Is there data in this buffer covering the R-wave window? */
            if ( Sta->dRStartTime < Sta->dEndTime &&
                 Sta->dREndTime   > Sta->dStartTime ) 
            {		 
/* iPickStatus: 1->waiting on data in the Rayleigh wave window (set in GetMs) */
               Sta->iPickStatus = 0;

/* Process the data */
               if ( GetMs( Sta, WaveHead->nsamp, &HStruct, &Gparm, 
                           Sta->lHit ) == 1 )
               {
                  ReportPick( Sta, Gparm.ucMyModId,
                   Gparm.PRegion, Ewh.TypePickTWC, Ewh.MyInstId, 1 );
                  WriteLPFile( Nsta, StaArray, Gparm.szLPRTFile, &HStruct ); 
                  HStruct.dMSAvg = ComputeAvgMS( Nsta, StaArray, &HStruct.iNumMS );
/* If this is the same hypo as in the dummy file, update the Ms */				  
                  ReadDummyData( &HypoD, Gparm.szDummyFile ); 
                  if ( IsItSameQuake( &HStruct, &HypoD ) == 1 )
                     if ( PatchDummyWithLP( &HStruct, Gparm.szDummyFile ) == 0 )
                        logit( "t", "Write dummy file error in Ms-3\n");
                  logit( "", "Ms reported for %s in WThread\n", 
                     Sta->szStation );
               }
               Sta->lHit = 0;        /* lHit = 1 for Init, 0 for no init */
            }
         }
		 
/* Is it time to turn off processing */		 
         for ( i=0; i<Nsta; i++ )
            if ( StaArray[i].dREndTime > (double) now ) goto LoopEnd;
         for ( i=0; i<Nsta; i++ )        /* If we get here, then yes it is */
         {
            StaArray[i].dMSMag = 0.;
            StaArray[i].lHit   = 1;      /* lHit = 1 for Init, 0 for no init */
            StaArray[i].iPickStatus = 0;
         }              
         iLPProcessing = 0;
         logit( "et", "LP processing completed in WThread\n" );
LoopEnd:;
      }
      ReleaseSpecificMutex( &mutsem );
   }   
   iRunning = 0;      /* Flag to release memory in main */

  return THR_NULL_RET;
}

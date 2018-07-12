    /******************************************************************
     *                            dummy.c                             * 
     *                                                                *
     * Contains dummy file read/write functions.                      *
     *                                                                *
     * May, 2012: Added Theta to dummy file.                          *
     * December, 2007: Split out Carrick's changes using ifdefs       *
     *                 so that PTWC can also use code in Solaris      *
     *                                                                *
     *   By:   Whitmore - May, 2001                                   *
     ******************************************************************/
#ifdef _WINNT
 #define _CRTDBG_MAP_ALLOC
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>              
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <earthworm.h>
#include "earlybirdlib.h"
#ifdef _WINNT
 #include <crtdbg.h>
#endif


#define BUFSIZE 512 // JMC -- Named Pipe


#ifdef _WINNT
LPTSTR lpszQuakeEWPipe = TEXT("\\\\.\\pipe\\earthvu.dummy"); 
int    totPipeInst = 5;
void logError( char *loc, int line )
{
   LPVOID  lpBuf;	

   FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, GetLastError (), 
                  MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPTSTR) &lpBuf,
                  0, NULL );
   logit( "et", "~%s=%d~ Error Code = 0x%X, %s\n", loc, line, GetLastError(),
          lpBuf);
   LocalFree( lpBuf );
}
#endif

/* This function ensures that only one process has access to a particular file*/

#ifdef _WINNT
HANDLE hMutex = NULL;
void closeFile( FILE *str )
{
   fflush( str );
   fclose( str );
/* Allow queued activities access to the file */
   ReleaseMutex( hMutex );
}
#else
void closeFile( FILE *str )
{
   fflush( str );
   fclose( str );
}
#endif

#ifdef _WINNT
FILE *openFile( char *fName, char *attributes )
{
   double  dw=0;
   FILE   *fp=NULL;
  
   hMutex = CreateMutex( NULL, FALSE, "dummyX" );
   if ( hMutex == NULL )
   {
      if ( GetLastError() != ERROR_ALREADY_EXISTS )
      {
         logError( __FILE__, __LINE__ );
         logit( "t", "fName = [%s]\n", fName );
         return NULL;
      }
      hMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, "dummyX" );	 
   }

/* If another process has the file open, then we will queue here
   -- wait 2 seconds */
   WaitForSingleObject( hMutex, 2000 );
   if ( dw != WAIT_TIMEOUT )     
      fp = fopen( fName, attributes );     
   else
   {
      logError( __FILE__, __LINE__ ); 
      ReleaseMutex( hMutex );      
      return ( NULL );
   } 
   return ( fp );
}
#else
FILE *openFile( char *fName, char *attributes )
{
   FILE   *fp;
  
   fp = fopen( fName, attributes );     
   return ( fp );
}
#endif

      /******************************************************************
       *                       ReadDummyData()                          *
       *                                                                *
       * This function reads hypocenter parameters data from the        *
       * specified dummy file.                                          *
       *                                                                *
       * May, 2012: Added Theta to end of file (Theta, SD, Num).        *
       * December, 2007: Combined with old program ReadDummyFile.       *
       * October, 2003: Read in lat lon as +/- geographic coord.        *
       * February, 2002: Added iNumMwp to read/write                    *
       * October, 2002: Added num magnitudes for each type              *
       *                                                                *
       *  Arguments:                                                    *
       *   pHypo            Computed hypocentral parameters             *
       *   pszDumFile       Parameter file (dummy file) to update       *
       *                                                                *
       *  Returns: 1->ok read, 0->no read                               *
       *                                                                *
       ******************************************************************/	   
int ReadDummyData( HYPO *pHypo, char *pszDumFile )
{
   char    cLat, cLon;
   double  dAzm = 0.0;    /* Azimuthal coverage in degrees */
   double  dDum = 0.0;
   FILE    *hFile;        /* File handle */
   int     iCnt;          /* Dummy file open counter */
   int     iDepth = 0;    /* Quake depth from dummy file */
   int     iDum = 0;
   int     iMonth = 0, iYear = 0;  /* Origin dates */
   int     iOTimeRnd;     /* O-time rounded to nearest minute */
   static  LATLON  ll;    /* Epicentral geographic location */
   struct tm tm;          /* Origin time in structure */
    
/* Initialize the hypocenter structure */
   InitHypo( pHypo );		
	
/* Read Dummy File */
   iCnt = 0; 
   if ( (hFile = openFile( pszDumFile, "r" )) != NULL )
      fscanf( hFile, "%d %lf %lf %d %lf %s %d %d %d %d %d %d "
                     "%d %d %lf %d %lf %d %lf %lf %d %lf %d %lf %d %d "
                     "%lf %lf %lf %s %d %lf %lf %d",
       &pHypo->iBullNo, &pHypo->llEpiGG.dLat, &pHypo->llEpiGG.dLon, &iDepth,
       &pHypo->dPreferredMag, (char *)(&pHypo->szPMagType), &tm.tm_mday, &iMonth, &iYear,
       &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &iDum, &pHypo->iNumPMags,
       &pHypo->dMSAvg, &pHypo->iNumMS, &pHypo->dMwpAvg, &pHypo->iNumMwp,
       &dDum, &pHypo->dMbAvg, &pHypo->iNumMb, &pHypo->dMlAvg, &pHypo->iNumMl,
       &pHypo->dMwAvg, &pHypo->iNumMw, &pHypo->iNumPs, &pHypo->dNearestDist,
       &pHypo->dAvgRes, &dAzm, (char *)(&pHypo->szQuakeID), &pHypo->iUpdateMap,
       &pHypo->dTheta, &pHypo->dThetaSD, &pHypo->iNumTheta );
           
   if ( hFile == NULL )
   {
      logit( "et" , "Dummy file [%s] not opened %s @ %d\n", 
             pszDumFile, __FILE__, __LINE__ );
      return 0;
   }
   closeFile( hFile );

   if ( fabs( pHypo->llEpiGG.dLat ) > 90. )
   {
      logit( "t", "Bad lat read from dummy file = %lf\n", pHypo->llEpiGG.dLat );
      logit( "", "%d %lf %d %lf %s %d %d %d %d %d %d "
                 "%d %d %lf %d %lf %d %lf %lf %d %lf %d %lf %d %d "
                 "%lf %lf %lf %s %d %lf %lf %d",
        pHypo->iBullNo, pHypo->llEpiGG.dLon, iDepth, pHypo->dPreferredMag,
        pHypo->szPMagType, tm.tm_mday, iMonth, iYear,
        tm.tm_hour, tm.tm_min, tm.tm_sec, iDum, pHypo->iNumPMags,
        pHypo->dMSAvg, pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp,
        dDum, pHypo->dMbAvg, pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl,
        pHypo->dMwAvg, pHypo->iNumMw, pHypo->iNumPs, pHypo->dNearestDist,
        pHypo->dAvgRes, dAzm, pHypo->szQuakeID, pHypo->iUpdateMap,
        pHypo->dTheta, pHypo->dThetaSD, pHypo->iNumTheta );
      pHypo->llEpiGG.dLat = 45.;
   }
   dDum = pHypo->llEpiGG.dLon;
   if ( dDum > 180. && dDum < 360. ) dDum -= 360.;
   if ( fabs( dDum ) > 180. )
   {
      logit( "t", "Bad lon read from dummy file = %lf\n", pHypo->llEpiGG.dLon );
      logit( "", "%d %lf %d %lf %s %d %d %d %d %d %d "
                 "%d %d %lf %d %lf %d %lf %lf %d %lf %d %lf %d %d "
                 "%lf %lf %lf %s %d %lf %lf %d",
        pHypo->iBullNo, pHypo->llEpiGG.dLat, iDepth, pHypo->dPreferredMag,
        pHypo->szPMagType, tm.tm_mday, iMonth, iYear,
        tm.tm_hour, tm.tm_min, tm.tm_sec, iDum, pHypo->iNumPMags,
        pHypo->dMSAvg, pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp,
        dDum, pHypo->dMbAvg, pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl,
        pHypo->dMwAvg, pHypo->iNumMw, pHypo->iNumPs, pHypo->dNearestDist,
        pHypo->dAvgRes, dAzm, pHypo->szQuakeID, pHypo->iUpdateMap,
        pHypo->dTheta, pHypo->dThetaSD, pHypo->iNumTheta );
      pHypo->llEpiGG.dLon = 45.;
   }
   
/* Put lat/lon in geocentric form */
   pHypo->dLat = pHypo->llEpiGG.dLat;
   pHypo->dLon = pHypo->llEpiGG.dLon;
   GeoCent ((LATLON *) pHypo);
   GetLatLonTrig( (LATLON *) pHypo );
   
/* Convert location to character coords */
   ConvertLoc( &pHypo->llEpiGG, &ll, &cLat, &cLon );
   sprintf( pHypo->szLat, "%5.1lf%c", ll.dLat, cLat );
   sprintf( pHypo->szLon, "%6.1lf%c", ll.dLon, cLon ); 

/* Convert origin time to 1/1/70 seconds */   
   if ( iMonth < 1 || iMonth > 12 )
   {
      logit( "t", "Bad month read in dummy = %d\n", iMonth );
      iMonth = 1 ;
   }
   tm.tm_mon   = iMonth - 1;
   if ( iYear < 1970 || iYear > 2100 )
   {
      logit( "t", "Bad year read in dummy = %d\n", iYear );
      iYear = 2012 ;
   }
   tm.tm_year  = iYear - 1900;
   if ( tm.tm_mday < 1 || tm.tm_mday > 31 )
   {
      logit( "t", "Bad day of month read in dummy = %d\n", tm.tm_mday );
      tm.tm_mday = 1 ;
   }
   if ( tm.tm_hour < 0 || tm.tm_hour > 24 )
   {
      logit( "t", "Bad hour read in dummy = %d\n", tm.tm_hour );
      tm.tm_hour = 0 ;
   }
   if ( tm.tm_min < 0 || tm.tm_min > 60 )
   {
      logit( "t", "Bad minute read in dummy = %d\n", tm.tm_min );
      tm.tm_min = 0 ;
   }
   if ( tm.tm_sec < 0 || tm.tm_sec > 60 )
   {
      logit( "t", "Bad second read in dummy = %d\n", tm.tm_sec );
      tm.tm_sec = 0 ;
   }
   tm.tm_isdst = 0;
   pHypo->dOriginTime = (double) mktime( &tm );

/* Fill SYSTEMTIME structure and get origin time in Mod. Julian seconds */
   pHypo->stOTime.wYear   = (WORD)iYear;
   pHypo->stOTime.wMonth  = (WORD)iMonth;
   pHypo->stOTime.wDay    = (WORD)tm.tm_mday;
   pHypo->stOTime.wHour   = (WORD)tm.tm_hour;
   pHypo->stOTime.wMinute = (WORD)tm.tm_min;
   pHypo->stOTime.wSecond = (WORD)tm.tm_sec;
   pHypo->stOTime.wMilliseconds = (WORD)0;
/* Convert origin time to modified Julian seconds */
   DateToModJulianSec( &pHypo->stOTime, &pHypo->dOriginTime );
   pHypo->dOriginTime = pHypo->dOriginTime - 3506630400.;/* Then to epochal sec */
   
/* Get O-time rounded to minute for messages */
   NewDateFromModSec( &pHypo->stOTime, pHypo->dOriginTime+3506630400. );
   NewDateFromModSecRounded( &pHypo->stOTimeRnd,
                              pHypo->dOriginTime+3506630400. );
   iOTimeRnd = pHypo->stOTimeRnd.wHour*100 + pHypo->stOTimeRnd.wMinute;
   itoaX( iOTimeRnd, pHypo->szOTimeRnd ); 
   PadZeroes( 4, pHypo->szOTimeRnd );                    /* Add leading zeros */
   
/* Convert a few other items */   
   pHypo->iAzm = (int) dAzm;
   pHypo->dDepth = (double) iDepth;
   if (pHypo->dDepth > 750.) pHypo->dDepth = 750.;
   if (pHypo->dDepth < 0.)   pHypo->dDepth = 0.;
   return 1;
}

      /******************************************************************
       *                     ReadPTimeFile()                            *
       *                                                                *
       * This function reads P-time and magnitude information           *
       * created by loc_wcatwc after a location is made.                *
       *                                                                *
       *  Arguments:                                                    *
       *   pSta             Station structures                          *
       *   pszPTimeFile     P-time file created in loc_wcatwc           *
       *   iNumSta          Number of stations in pSta                  *
       *   pHypo            Hypocenter array                            *
       *                                                                *
       *  Returns: 1->ok read, 0->no read                               *
       *                                                                *
       ******************************************************************/	   
int ReadPTimeFile( STATION *pSta, char *pszPTimeFile, int iNumSta, HYPO *pHypo )
{
   FILE    *hFile;          /* File handle */
   int     i, iCnt;    
   STATION StaT;            /* Temp Station structure */

   iCnt = 0;
Open:
   if ( (hFile = fopen( pszPTimeFile, "r" )) != NULL )
   {
      fscanf( hFile, "%lf %lf %lf %lf\n", &pHypo->dOriginTime, 
        &pHypo->dPreferredMag, &pHypo->dLat, &pHypo->dLon );
      while ( !feof( hFile ) )    /* Read till end of file */
      {
         fscanf( hFile, "%s %s %s %s %lf %s %lf %lf %lf %lf %lf %lf %lf %lf %lf "
                        "%lf %lf %lf %d %ld %c %lf %lf %lf %lf\n",
          StaT.szStation, StaT.szChannel, StaT.szNetID, StaT.szLocation,
          &StaT.dPTime, StaT.szPhase, &StaT.dMbAmpGM, &StaT.dMbPer,
          &StaT.dMbTime, &StaT.dMlAmpGM, &StaT.dMlPer, &StaT.dMlTime,
          &StaT.dMSAmpGM, &StaT.dMSPer, &StaT.dMSTime,
          &StaT.dMwpIntDisp, &StaT.dMwpTime, &StaT.dRes, &StaT.iUseMe,
          &StaT.lPickIndex, &StaT.cFirstMotion, &StaT.dMbMag, &StaT.dMlMag,
          &StaT.dMSMag, &StaT.dMwpMag );

/* Find match in array */
         for ( i=0; i<iNumSta; i++ )
            if ( !strcmp( StaT.szStation, pSta[i].szStation ) &&
                 !strcmp( StaT.szChannel, pSta[i].szChannel ) &&
                 !strcmp( StaT.szNetID, pSta[i].szNetID ) )
            {
/* Update structure */	    
               strcpy( pSta[i].szPhase, StaT.szPhase );
               pSta[i].dPTime = StaT.dPTime;
               pSta[i].dMbAmpGM = StaT.dMbAmpGM;
               pSta[i].dMbPer = StaT.dMbPer;
               pSta[i].dMbTime = StaT.dMbTime;
               pSta[i].dMlAmpGM = StaT.dMlAmpGM;
               pSta[i].dMlPer = StaT.dMlPer;
               pSta[i].dMlTime = StaT.dMlTime;
               pSta[i].dMSAmpGM = StaT.dMSAmpGM;
               pSta[i].dMSPer = StaT.dMSPer;
               pSta[i].dMSTime = StaT.dMSTime;
               pSta[i].dMwpIntDisp = StaT.dMwpIntDisp;
               pSta[i].dMwpTime = StaT.dMwpTime;
               pSta[i].dRes = StaT.dRes;
               pSta[i].iUseMe = StaT.iUseMe;
               pSta[i].lPickIndex = StaT.lPickIndex;
               pSta[i].cFirstMotion = StaT.cFirstMotion;
               pSta[i].dMbMag = StaT.dMbMag;
               pSta[i].dMlMag = StaT.dMlMag;
               pSta[i].dMSMag = StaT.dMSMag;
               pSta[i].dMwpMag = StaT.dMwpMag;
	    
/* See if this is a P or just a Phase */
               if ( pSta[i].dPTime > 0. )
/* Use  same logic as in ANALYZE  */
               {
                  if ( ((pSta[i].szPhase[1] == 'P' || 
                         pSta[i].szPhase[1] == 'p') &&
                         strlen( pSta[i].szPhase ) <= 3) ||
                        (pSta[i].szPhase[1] == '(' &&
                        (pSta[i].szPhase[2] == 'P' || 
                         pSta[i].szPhase[2] == 'p')) ||
                        (strlen( pSta[i].szPhase ) == 1 &&
                         pSta[i].szPhase[0] == 'P') )
                  {
                     ;             /* It is a 1st arrival */
                  }
                  else
                  {                /* It is a phase */
                     pSta[i].dPhaseTime = pSta[i].dPTime;
                     pSta[i].dPTime = 0.;
                  }
               }
               break;
            }
            if ( i == iNumSta )
            {
               logit( "", "Station %s:%s:%s:%s not found in array\n",
                StaT.szStation, StaT.szChannel, StaT.szNetID,
                StaT.szLocation );
            }
      }
      fclose( hFile );
   }
   else                     /* Couldn't open file */
   {
      if ( iCnt == 5 )      /* Quit trying */
      {
         logit ("t", "PFile (%s) could not be opened; 5X\n", pszPTimeFile );
         return ( 0 );
      }
      iCnt++;            /* Try again in 0.1 seconds */
      sleep_ew( 100 );
      goto Open;
   }
   return ( 1 );
}

      /******************************************************************
       *                       WriteDummyData()                         *
       *                                                                *
       * This function writes hypocenter parameters data to the         *
       * specified dummy file.  It also updates the file used by the    *
       * GIS (pszMapFile).                                              *
       *                                                                *
       * May, 2012: Added Theta to end of file (Theta, SD, Num).        *
       * December, 2007: Combined with WriteDummyFile in \seismic.      *
       * October, 2003: Write lat lon as +/- geographic coord.          *
       * February, 2002: Added iNumMwp to read/write                    *
       * October, 2002: Added num magnitudes for each type              *
       *                                                                *
       *  Arguments:                                                    *
       *   pHypo            Computed hypocentral parameters             *
       *   pszDumFile       Parameter file (dummy file) to update       *
       *   iUpdate          iUpdate=1 => force a new map creation in EV *
       *   iGetBull         Read in Bulletin number and quake ID first  *
       *                                                                *
       *  Returns: 1->ok write, 0->no write                             *
       *                                                                *
       ******************************************************************/
	   
int WriteDummyData( HYPO *pHypo, char *pszDumFile, int iUpdate, int iGetBull )
{
   double  dAvgRes;                     /* Average residual */
   double  dDum;
   FILE    *hFile;                      /* File handle */
   static  int     iBullNo;             /* Bulletin number from dummy file */
   int     iDum;
   static  LATLON ll;                   /* Epicentral geographic location */
   long    lTime;                       /* 1/1/70 time */
   char    szDum[8];
   static  char szQuakeID[32];          /* Quake ID from dummy file */
   struct  tm *tm;                      /* Origin time in structure */
#ifdef _WINNT
   int     tInst=0;
#endif
      
/* Get epicenter lat/lon in geographic coordinates */
   GeoGraphic( &ll, (LATLON *) pHypo );
   if ( ll.dLon > 180. ) ll.dLon -= 360.;
   
/* Read in bulletin number and quake ID from existing dummy file */   
   if ( iGetBull == 1 )          /* Only read in if told to */
   {
      hFile = openFile( pszDumFile, "r" );     // jmc
      if ( hFile != NULL )           /* Read bulletin number */
         fscanf( hFile, "%d %lf %lf %d %lf %s %d %d %d %d %d %d "
                        "%d %d %lf %d %lf %d %lf %lf %d %lf %d %lf %d %d "
                        "%lf %lf %lf %s %d %lf %lf %d",
          &iBullNo, &dDum, &dDum, &iDum, &dDum, (char *)(&szDum), &iDum, &iDum, &iDum,
          &iDum, &iDum, &iDum, &iDum, &iDum, &dDum, &iDum, &dDum, &iDum,
          &dDum, &dDum, &iDum, &dDum, &iDum, &dDum, &iDum, &iDum, &dDum,
          &dDum, &dDum, (char *)(&szQuakeID), &iDum, &dDum, &dDum, &iDum );
      else    
      {
         logit( "t" , "1-Dummy file not opened %s\n", pszDumFile );
         return 0;
      }
      closeFile( hFile );
   }
   else
   {
      iBullNo = pHypo->iBullNo;
      strcpy( szQuakeID, pHypo->szQuakeID );
   }

/* Set max average residual */
   dAvgRes = pHypo->dAvgRes;
   if ( dAvgRes > 99.99 ) dAvgRes = 99.99;

/* Get time rounded to nearest second */
   lTime = (long) (pHypo->dOriginTime+0.5);
   tm = TWCgmtime( lTime );
    
/* Update Dummy File */
   if ( (hFile = openFile( pszDumFile, "w" )) != NULL )
   {
#ifdef _WINNT
      TCHAR  chReadBuf[BUFSIZE]; 
      DWORD  cbRead; 
      HANDLE hPipe=NULL; 
#endif
     
      fprintf( hFile, "%d %7.3lf %8.3lf %d %3.1lf %s %d %d %d %d %d %d %d %d "
       "%3.1lf %d %3.1lf %d %3.1lf %3.1lf %d %3.1lf %d %3.1lf %d %d "
       "%7.3lf %5.2lf %5.1lf %s %d %lf %lf %d\n",
       iBullNo, ll.dLat, ll.dLon, (int) (pHypo->dDepth + 0.5),
       pHypo->dPreferredMag, pHypo->szPMagType, tm->tm_mday, tm->tm_mon+1,
       tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec, 0,
       pHypo->iNumPMags, pHypo->dMSAvg, pHypo->iNumMS, pHypo->dMwpAvg,
       pHypo->iNumMwp, 0., pHypo->dMbAvg, pHypo->iNumMb, pHypo->dMlAvg,
       pHypo->iNumMl, pHypo->dMwAvg, pHypo->iNumMw, pHypo->iNumPs,
       pHypo->dNearestDist, dAvgRes, (double) pHypo->iAzm, szQuakeID, iUpdate,
       pHypo->dTheta, pHypo->dThetaSD, pHypo->iNumTheta );
 
#ifdef _WINNT
      if ( iUpdate == 1 )
      {
         for ( tInst=0; tInst<totPipeInst; tInst++ )
         {
            hPipe=CreateFile( lpszQuakeEWPipe, GENERIC_WRITE, FILE_SHARE_WRITE, 
                              NULL, OPEN_EXISTING, 0, NULL );      
            strcpy( chReadBuf, "update" );
            WriteFile( hPipe, chReadBuf, (DWORD)strlen( chReadBuf ), &cbRead, NULL );
            FlushFileBuffers( hPipe ); 
            CloseHandle( hPipe ); 
         }	 
      }	 
#endif
   }
   else                               
   {
      logit( "et" , "2-Dummy file not opened %s\n", pszDumFile );
      return ( 0 );
   }
   closeFile( hFile );
   return 1;
}

      /******************************************************************
       *                          WriteLPFile()                         *
       *                                                                *
       * This function writes the file with Ms information              *
       *                                                                *
       * Dec., 2011: Added first line with quake results.               *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of Stations in this structure        *
       *   pSta             Station structure array                     *
       *   pszFile          Output data file name                       *
       *   pHypo            Computed hypocentral parameters             *
       *                                                                *
       ******************************************************************/
	   
void WriteLPFile( int iNum, STATION pSta[], char *pszFile, HYPO *pHypo )
{
   FILE    *hFile;                   /* File handle */
   int     i;

/* Open pick file */        
   if ( (hFile = fopen( pszFile, "w" )) == NULL )
   {
      logit ("t", "Ms file, %s, not opened for write.\n", pszFile);
      return;
   }
                                                            
/* Dump the picks and magnitude data to disk */   
   fprintf( hFile, "%lf %lf %lf %lf\n", pHypo->dOriginTime, 
            pHypo->dPreferredMag, pHypo->dLat, pHypo->dLon );
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].iUseMe > 0  && strlen( pSta[i].szStation ) > 0 && 
          (pSta[i].dPTime > 0.1 || pSta[i].dMSAmpGM > 0.1) )
         fprintf( hFile, "%s %s %s %s %lf %s %lf %lf %lf %lf %lf %lf %lf %lf %lf "
                         "%E %lf %lf %d %ld %c %lf %lf %lf %lf\n",
          pSta[i].szStation, pSta[i].szChannel, pSta[i].szNetID,
          pSta[i].szLocation, pSta[i].dPTime, pSta[i].szPhase,   
          pSta[i].dMbAmpGM, pSta[i].dMbPer, pSta[i].dMbTime, 
          pSta[i].dMlAmpGM, pSta[i].dMlPer, pSta[i].dMlTime, 
          pSta[i].dMSAmpGM, pSta[i].dMSPer, pSta[i].dMSTime, 
          pSta[i].dMwpIntDisp, pSta[i].dMwpTime, pSta[i].dRes, pSta[i].iUseMe,
          pSta[i].lPickIndex, pSta[i].cFirstMotion, pSta[i].dMbMag,
          pSta[i].dMlMag, pSta[i].dMSMag, pSta[i].dMwpMag );
   fclose( hFile );                  
   return;
}

      /******************************************************************
       *                          WriteMwFile()                         *
       *                                                                *
       * This function writes the file with Mw information              *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of Stations in this structure        *
       *   pSta             Station structure array                     *
       *   pszFile          Output data file name                       *
       *   pHypo            Computed hypocentral parameters             *
       *                                                                *
       ******************************************************************/	   
void WriteMwFile( int iNum, STATION pSta[], char *pszFile, HYPO *pHypo )
{
   FILE    *hFile;                   /* File handle */
   int     i,j;

/* Open pick file */        
   if ( (hFile = fopen( pszFile, "w" )) == NULL )
   {
      logit ("t", "Ms file, %s, not opened for write.\n", pszFile);
      return;
   }
                                                            
/* Dump the Mw data to disk */   
   fprintf( hFile, "%lf\n %lf\n %lf\n %lf\n", pHypo->dLat, pHypo->dLon, 
            pHypo->dOriginTime, pHypo->dPreferredMag );
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].dMwMag > 0. && pSta[i].iUseMe > 0 )
      {
         fprintf( hFile, "%s %s %s %s %lf %d\n", pSta[i].szStation,
                  pSta[i].szChannel, pSta[i].szNetID, 
                  pSta[i].szLocation, pSta[i].dMwMag, pSta[i].iMwNumPers );
         for ( j=0; j<pSta[i].iMwNumPers; j++ )
            fprintf( hFile, "%lf %lf %lf %lf %lf\n", pSta[i].dMwPerSp[j],
                     pSta[i].dMwMagSp[j], pSta[i].dMwAmpSp[j], 
                     pSta[i].dMwMagSpBG[j], pSta[i].dMwAmpSpBG[j] );
      }
   fclose( hFile );                  
   return;
}

      /******************************************************************
       *                          WritePTimeFile()                      *
       *                                                                *
       * This function writes the P and magnitude data to a file.       *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of Stations in this structure        *
       *   pSta             Station structure array                     *
       *   pszFile          Output data file name                       *
       *   pHypo            Hypocenter array                            *
       *                                                                *
       ******************************************************************/
	   
void WritePTimeFile( int iNum, STATION pSta[], char *pszFile, HYPO *pHypo )
{
   FILE    *hFile;                   /* File handle */
   int     i;

/* Open pick file */        
   if ( (hFile = fopen( pszFile, "w" )) == NULL )
   {
      logit ("t", "Pick file, %s, not opened for write (1).\n", pszFile);
      return;
   }

/* Write some quake identifier stuff at top */
   fprintf( hFile, "%lf %lf %lf %lf\n", pHypo->dOriginTime, pHypo->dPreferredMag, 
            pHypo->dLat, pHypo->dLon );

/* Dump the picks and magnitude data to disk */   
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].iUseMe > 0  && strlen( pSta[i].szStation ) > 0 && 
          (pSta[i].dPTime > 0.1 || pSta[i].dMSAmpGM > 0.1) )
         fprintf( hFile, "%s %s %s %s %lf %s %lf %lf %lf %lf %lf %lf %lf %lf %lf "
                         "%E %lf %lf %d %ld %c %lf %lf %lf %lf\n",
          pSta[i].szStation, pSta[i].szChannel, pSta[i].szNetID,
          pSta[i].szLocation, pSta[i].dPTime, pSta[i].szPhase,   
          pSta[i].dMbAmpGM, pSta[i].dMbPer, pSta[i].dMbTime, 
          pSta[i].dMlAmpGM, pSta[i].dMlPer, pSta[i].dMlTime, 
          pSta[i].dMSAmpGM, pSta[i].dMSPer, pSta[i].dMSTime, 
          pSta[i].dMwpIntDisp, pSta[i].dMwpTime, pSta[i].dRes, pSta[i].iUseMe,
          pSta[i].lPickIndex, pSta[i].cFirstMotion, pSta[i].dMbMag,
          pSta[i].dMlMag, pSta[i].dMSMag, pSta[i].dMwpMag );
   fclose( hFile );                  
   return;
}
                             
      /******************************************************************
       *                       WriteThetaFile()                         *
       *                                                                *
       * This function writes the file with Theta information.          *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of Stations in this structure        *
       *   pSta             Station structure array                     *
       *   pszFile          Output data file name                       *
       *   pHypo            Computed hypocentral parameters             *
       *                                                                *
       ******************************************************************/	   
void WriteThetaFile( int iNum, STATION pSta[], char *pszFile, HYPO *pHypo )
{
   FILE    *hFile;                   /* File handle */
   int     i;

/* Open pick file */        
   if ( (hFile = fopen( pszFile, "w" )) == NULL )
   {
      logit ("t", "Theta file, %s, not opened for write.\n", pszFile);
      return;
   }
                                                            
/* Dump the theta data to disk */   
   fprintf( hFile, "%lf\n %lf\n %lf\n %lf\n", pHypo->dLat, pHypo->dLon, 
            pHypo->dOriginTime, pHypo->dPreferredMag );
   for ( i=0; i<iNum; i++ )
      if ( fabs( pSta[i].dTheta ) > 0.01 && pSta[i].iUseMe > 0 )
         fprintf( hFile, "%s %s %s %s %lf\n", pSta[i].szStation,
                  pSta[i].szChannel, pSta[i].szNetID, 
                  pSta[i].szLocation, pSta[i].dTheta );
   fclose( hFile );                  
   return;
}

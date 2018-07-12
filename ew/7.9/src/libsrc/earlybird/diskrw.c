 /************************************************************************
  * DISKRW.C                                                             *
  *                                                                      *
  * This is a group of functions which provide tools for                 *
  * reading disk files created by disk_wcatwc.                           *
  * Format of the files is discussed below.                              *
  *                                                                      *
  * Made into earthworm module 7/2001.                                   *
  *                                                                      *
  ************************************************************************/
  
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>                
#include "earlybirdlib.h"

      /******************************************************************
       *                          CreateFileName()                      *
       *                                                                *
       *  This function computes the name of the file which is needed   *
       *  for a given requested time.  The file name is in the form     *
       *  [pszRootDirectory]\Dyymmdd\Hmdhhmm.[pszSuffix]yy.  Each day   *
       *  gets a new sub-directory.                                     *
       *                                                                *
       *  The given time must be rounded down to the nearest iFileSize  *
       *  minutes.  Files are created when the first packet comes in    *
       *  which would go in that file.  The first sample in the file    *
       *  is the first after an interval of iFileSize minutes.          *
       *  NOTE: Number of minutes in an hour must be a multiple of      *
       *  iFileSize (i.e., iFileSize can be 1,2,3,4,5,6,10, etc.).      *
       *                                                                *
       *  These file names were devised at the National                 *
       *  Tsunami Warning Center for use with the Analyze software.     *
       *                                                                *
       *  Arguments:                                                    *
       *   dTime            Nominal start time of data file (1/1/70 sec)*
       *   iFileSize        File length in minutes                      *
       *   pszRootDir       Root directory of data files                *
       *   pszSuffix        Start of file suffix                        *
       *   pszFile          Created file name                           *
       *                                                                *
       *  Return:           0 if problem, 1 otherwise                   *
       *                                                                *
       ******************************************************************/

int CreateFileName( double dTime, int iFileSize, char *pszRootDirectory,
                    char *pszSuffix, char *pszFile )
{
   time_t    itime;         /* time (1/1/70) at start of file */   
   char      szYear[4], szMon[4], szDay[4], szMonthUnit[2], szDayUnit[2],
             szHour[3], szMin[3];/* Minute, hour, etc. time units for file  */
   struct tm *tm;           /* time structure for the file name time */

/* Check for 0 FileSize */ 
   if ( iFileSize == 0 )
   {
      logit( "", "iFileSize = 0; INVALID !!!\n" );
      return 0;
   }

/* Convert dTime to tm structure (we don't need Milliseconds here) */
   itime = (time_t) (floor( dTime+0.001 ) );
   tm = TWCgmtime( itime );
    
/* When called from Analyze, time is not rounded down. */
   if ( (tm->tm_min % iFileSize) != 0 )    /* Round down to nearest interval */
      tm = TWCgmtime( itime-((tm->tm_min%iFileSize)*60) );

/* Specify start time in filename and directory, convert date/time to char */
   while ( tm->tm_year >= 100 ) tm->tm_year-=100;  /* For Y2K independence */
   itoaX( tm->tm_year, szYear ); 
   itoaX( tm->tm_mon+1, szMon );		
   itoaX( tm->tm_mday, szDay );		
   PadZeroes( 2, szYear );
   PadZeroes( 2, szMon );
   PadZeroes( 2, szDay );

/* Put month in single char form (1-9, then A, B, C) */
   if       ( tm->tm_mon+1 == 10 ) strcpy( szMonthUnit, "A" ); 	
   else if  ( tm->tm_mon+1 == 11 ) strcpy( szMonthUnit, "B" );
   else if  ( tm->tm_mon+1 == 12 ) strcpy( szMonthUnit, "C" );
   else itoaX( tm->tm_mon+1, szMonthUnit );

/* Put day in single character form (1-9, then A, B, ... V) */
   if       ( tm->tm_mday == 10 ) strcpy( szDayUnit, "A" );
   else if  ( tm->tm_mday == 11 ) strcpy( szDayUnit, "B" );
   else if  ( tm->tm_mday == 12 ) strcpy( szDayUnit, "C" );
   else if  ( tm->tm_mday == 13 ) strcpy( szDayUnit, "D" );
   else if  ( tm->tm_mday == 14 ) strcpy( szDayUnit, "E" );
   else if  ( tm->tm_mday == 15 ) strcpy( szDayUnit, "F" );
   else if  ( tm->tm_mday == 16 ) strcpy( szDayUnit, "G" );
   else if  ( tm->tm_mday == 17 ) strcpy( szDayUnit, "H" );
   else if  ( tm->tm_mday == 18 ) strcpy( szDayUnit, "I" );
   else if  ( tm->tm_mday == 19 ) strcpy( szDayUnit, "J" );
   else if  ( tm->tm_mday == 20 ) strcpy( szDayUnit, "K" );
   else if  ( tm->tm_mday == 21 ) strcpy( szDayUnit, "L" );
   else if  ( tm->tm_mday == 22 ) strcpy( szDayUnit, "M" );
   else if  ( tm->tm_mday == 23 ) strcpy( szDayUnit, "N" );
   else if  ( tm->tm_mday == 24 ) strcpy( szDayUnit, "O" );
   else if  ( tm->tm_mday == 25 ) strcpy( szDayUnit, "P" );
   else if  ( tm->tm_mday == 26 ) strcpy( szDayUnit, "Q" );
   else if  ( tm->tm_mday == 27 ) strcpy( szDayUnit, "R" );
   else if  ( tm->tm_mday == 28 ) strcpy( szDayUnit, "S" );
   else if  ( tm->tm_mday == 29 ) strcpy( szDayUnit, "T" );
   else if  ( tm->tm_mday == 30 ) strcpy( szDayUnit, "U" );
   else if  ( tm->tm_mday == 31 ) strcpy( szDayUnit, "V" );
   else itoaX( tm->tm_mday, szDayUnit );

/* Keep hour, minute in numeric form (2 digit) */
   itoaX( tm->tm_hour, szHour );
   itoaX( tm->tm_min, szMin );              
   PadZeroes( 2, szHour );
   PadZeroes( 2, szMin );

/* Create directory/file name */
#ifdef _WINNT
   sprintf( pszFile, "%s%s%s%s%s%s%s%s%s%s%s%s", pszRootDirectory, "\\D", 
            szYear, szMon, szDay, "\\S", szMonthUnit, szDayUnit, szHour, szMin,
            pszSuffix, szYear );
#else
   /* unix file names */
   sprintf( pszFile, "%s%s%s%s%s%s%s%s%s%s%s%s", pszRootDirectory, "/D", 
            szYear, szMon, szDay, "/S", szMonthUnit, szDayUnit, szHour, szMin,
            pszSuffix, szYear );
#endif   
   return 1;
}

      /******************************************************************
       *                     GetNumStnsInFile()                         *
       *                                                                *
       *  This function reads the disk header from a seismic data       *
       *  file created in disk_wcatwc and returns the number of         *
       *  channels in the file.                                         *
       *                                                                *
       *  Arguments:                                                    *
       *   pszFile          File to read                                *
       *                                                                *
       *  Return:           -1 if problem; otherwise # channels         *
       *                                                                *
       ******************************************************************/
	   
int GetNumStnsInFile( char *pszFile )
{
   DISKHEADER dh;                    /* DISKHEADER info (see diskrw.h) */
   FILE	     *hFile;                 /* Handle to seismic data file */

/* First, try to open data file */
   if ( (hFile = fopen( pszFile, "rb" )) == NULL ) 
   {
      logit ("et", "File %s not opened in GetNumStnsInFile\n", pszFile);
      return -1;
   }

/* Read in the DISKHEADER structure */
   if ( fread( &dh, sizeof (DISKHEADER), 1, hFile ) < 1 ) 
   {	
      fclose( hFile );
      logit( "t", "DISKHEADER read failed in GetNumStnsInFile: file %s\n", 
                  pszFile );
      return -1;
   }

   fclose( hFile );
   return dh.iNumChans;
}                                  

      /******************************************************************
       *                     GetTimeFromFileName()                      *
       *                                                                *
       *  This function computes the time at the start of a seismic data*
       *  file given the file name.  The time is returned in seconds    *
       *  since 1/1/1970. The file name must be in the standard NTWC    *
       *  format (see CreateFileName).  This will function until the    *
       *  year 2090.                                                    *
       *                                                                *
       *  Arguments:                                                    *
       *   pszFileName      File name to examine for start time         *
       *                                                                *
       *  Return:           Nominal time at start of data in file       *
       *                                                                *
       ******************************************************************/

double GetTimeFromFileName (char *pszFile)
{
   int        iTemp;
   time_t     lTime;             /* Epochal time (1/1/70) */
   char       *psz;
   char       szYear[3], szMon[3], szDay[3], szHour[3], szMin[3];
   struct     tm *tm;            /* Time in structure format */

/* Load separated strings of year, month, etc. */
   psz = strrchr (pszFile, '\\');  /* Find last \ in string */
   iTemp = (int)(psz - pszFile);
   szYear[0] = pszFile[iTemp-6];   /* Load up year, month, day strings */
   szYear[1] = pszFile[iTemp-5];
   szYear[2] = '\0';
   szMon[0] = pszFile[iTemp-4];
   szMon[1] = pszFile[iTemp-3];
   szMon[2] = '\0';
   szDay[0] = pszFile[iTemp-2];
   szDay[1] = pszFile[iTemp-1];
   szDay[2] = '\0';
   szHour[0] = pszFile[iTemp+4];   /* Load up hour and minute strings */
   szHour[1] = pszFile[iTemp+5];
   szHour[2] = '\0';
   szMin[0] = pszFile[iTemp+6];
   szMin[1] = pszFile[iTemp+7];
   szMin[2] = '\0';
   
/* Convert the year, month, etc. strings to tm structure */
   lTime = 0;
   tm = TWCgmtime( lTime );
   tm->tm_isdst = 0;                         
   tm->tm_sec = 0;
   tm->tm_min = atoi( szMin );     
   tm->tm_hour = atoi( szHour );
   tm->tm_mday = atoi( szDay );
   tm->tm_mon = atoi( szMon ) - 1;
   tm->tm_year = atoi( szYear );
   if ( tm->tm_year < 90 ) tm->tm_year += 100;    /* Good to 2090 */
   else                    tm->tm_year += 0;
#ifdef _WINNT
   lTime = mktime( tm );                          /* Convert to epochal time */
#else
   lTime = timegm( tm );
#endif   
   return ( (double) lTime );
}

      /******************************************************************
       *                  ReadDiskDataForMTSolo()                       *
       *                                                                *
       *  This function reads data from a file created by disk_wcatwc,  *
       *  and fills up a pre-created space in the STATION structure.    *
       *  NOTE: All data is 4 byte data, even if it is 12 bit data.     *
       *                                                                *
       *  The format is an internal NTWC seismic data format.  The      *
       *  format looks like:                                            *
       *                                                                *
       *  structure DISKHEADER;                                         *
       *  structure CHNLHEADER * iNumStations;                          *
       *  chn 0 samp0, chn 0 samp1, ... chn 0 lNumSamps-1;              *
       *  chn 1 samp0, chn 1 samp1, ... chn 1 lNumSamps-1;              *
       *                 .                                              *
       *                 .                                              *
       *                 .                                              *
       *  chn NSta-1 samp0, chn NSta-1 samp1, ... chn NSta-1 lNumSamps-1*
       *                                                                *
       *  Samp0 from each channel is the same time (well, almost), etc. *
       *                                                                *
       *  The data is written in binary (always i4).  The file name     *
       *  describes what time the file holds.  iFileSize is read in     *
       *  from the .d file.  This tells how many minutes of seismic     *
       *  data there is per file.  The names are in the format          *
       *  [szRootDirectory]\Dyymmdd\Smdhhmm[FileSuffix]yy.              *
       *  The first sample in the file is the first sample after the    *
       *  time given in the file name.  The file name time is called    *
       *  the nominal time.                                             *
       *                                                                *
       *  Arguments:                                                    *
       *   iFileSize        File size in minutes                        *
       *   iTotalTime       Time (minutes) of data for each station     *
       *   szPath           Directory path of datafile                  *
       *   szSuffix         Data file suffix start                      *
       *   dStartTime       Start time (seconds) to read                *
       *   iNumStas         Number of stations to read                  *
       *   StaArray         Complete array of station data              *
       *   iNumRead         Number of files read by function            *
       *                                                                *
       *  Return:           0 if data read OK, -1 if problem, 1 if file *
       *                    not there                                   *
       *                                                                *
       ******************************************************************/
       
int ReadDiskDataForMTSolo( int iFileSize, int iTotalTime, char *szPath,
                           char *szSuffix, double dStartTime, int iNumStas,
                           STATION StaArray[], int *iNumRead )
{
   CHNLHEADER ch[MAX_STATIONS];        /* CHNLHEADER info (see diskrw.h) */
   static  FILE    *hFile;             /* Handle to seismic data file */
   DISKHEADER dh;                      /* DISKHEADER info (see diskrw.h) */
   static  double  dFileTime;          /* 1/1/70 time at file start */
   double  dInt;                       /* Time interval between samps */
   int     i, j, k, l, m;
   static  int     iAllRead;           /* 1 -> all needed data files read */
   int     iInt;                       /* # samps since start of second*/
   static  int     iMaxToRead;         /* Number of files to read */
   static  int     iRead[MAX_STATIONS];/* Flag which indicates read status
                                          0   -> No data yet for this station
                                          1-X -> Number of files read for stn */
   time_t  iTime;                      /* time (1/1/70) at min P time */   
   int32_t lStart;                     /* Starting data index of buffer */
   int32_t lTemp[MAX_TEMP];            /* Temp storage for data read in */
   char    szFile[MAX_FILE_SIZE];      /* Data file names */
   struct  tm *tm;                     /* time struct for the file name time */
#if 1
   // JFP --  Adding a SCNL change over date 
   SYSTEMTIME stDate;
   double dSCNLTime;
   double dDataTime;
   //  Date set at 14 Oct 2009 at 12 Noon.
   stDate.wYear = 2009;
   stDate.wMonth = 10;
   stDate.wDay = 14;
   stDate.wHour = 12;
   stDate.wMinute = 0;
   stDate.wSecond = 0;
   stDate.wMilliseconds = 0;
/* Convert time to modified Julian seconds */
   DateToModJulianSec( &stDate, &dSCNLTime );
   dSCNLTime -= 3506630400.;  /* Then to epochal seconds */
   // -------------------------------
#endif
/* Initialize some things */
   iAllRead = 0;
   for ( i=0; i<iNumStas; i++ )
   {
      iRead[i] = 0;
      StaArray[i].lRawCircCtr   = 0;
      StaArray[i].lSampIndexF   = 0;
      StaArray[i].dEndTime      = 0.;
   }
   iMaxToRead = iTotalTime/iFileSize + 1;

/* Compute file time (time at start of file) for the O-time. */
   iTime = (time_t) (floor( dStartTime ));
   tm = TWCgmtime( iTime );
   dInt = 1. / StaArray[0].dSampRate;
   iInt = (int) ((dStartTime - floor( dStartTime )) / dInt + 0.00001);
   dFileTime = dStartTime - ((double) (tm->tm_min % iFileSize)*60.) -
              (double) tm->tm_sec - ((double) iInt*dInt);

/* Loop through all data files; each file is read once, then the StaArray 
   structure is looped through to patch the data */
   for ( k=0; k<iMaxToRead; k++ )
   {
/* Get file name for this time */
      CreateFileName( dFileTime, iFileSize, szPath, szSuffix, szFile );

/* Open the data file */
      if ( (hFile = fopen( szFile, "rb" )) == NULL ) 
      {
         logit( "t", "File %s not opened in ReadDiskDataForMTSolo\n", szFile );
         *iNumRead = k;	 
         if ( k > 0 ) return( 0 );  /* It may have not been recorded yet */
         else         return( 1 );
      }
	  
/* Read in the DISKHEADER structure */
      if ( fread( &dh, sizeof (DISKHEADER), 1, hFile ) < 1 )
      {
         fclose( hFile );
         logit( "t", "DISKHEADER read failed in file %s\n", szFile );
         *iNumRead = k;	 
         return( -1 );
      }
/* Convert time to modified Julian seconds */
      DateToModJulianSec( &dh.stStartTime, &dDataTime );
      dDataTime -= 3506630400.;  /* Then to epochal seconds */
/* Read in the channel headers */
      for ( i=0; i<dh.iNumChans; i++ )
      {
/* Need to initialize this char array. */
/* *********************************** */
         if ( (int) fread( &ch[i], dh.iChnHdrSize, 1, hFile ) < 1 ) 
         {
            fclose( hFile );
            logit( "t", "CHNLHEADER read failed, file %s\n", szFile );
            *iNumRead = k;	 
            return( -1 );
         }
         if ( i == MAX_STATIONS )
         {
            fclose( hFile );
            logit( "t", "Too many stations in file %s, %d\n", szFile,
                   dh.iNumChans );
            *iNumRead = k;	 
            return( -1 );
         }
/* JFP --   Compare the dates of the data being read in (dStartTime) with
             the date of the SCNL change over (dSCNLTime). */
         if( dDataTime < dSCNLTime )
         {
/* This is old scn data, so we need to get the "new" location code and put it
   in place. */
            for ( j=0; j<iNumStas; j++ )
            {               
               if ( !strcmp( ch[i].szStation, StaArray[j].szStation ) &&
                    !strcmp( ch[i].szChannel, StaArray[j].szChannel ) &&
                    !strcmp( ch[i].szNetID, StaArray[j].szNetID ) )
               {              
                  strcpy( ch[i].szLocation, StaArray[j].szLocation );
                  break;
               }
            }
         }
         else
         {   
/* Got some newer data with scnl then we don't have to do anything. */
         }
      }
/* Read data into spare buffer */      
      for ( m=0; m<dh.iNumChans; m++ )
      {
         if ( ch[m].lNumSamps > MAX_TEMP ) 
         {
            logit( "t", "%s lNumSamps (%ld) > MAX_TEMP\n", ch[m].szStation,
                                                           ch[m].lNumSamps );
            fclose( hFile );
            *iNumRead = k;	 
    	    return( -1 );
         }
         if ( (int) fread( &lTemp, ch[m].iBytePerSamp, ch[m].lNumSamps,
                            hFile ) < ch[m].lNumSamps )
         {
            logit( "t", "File %s fread3 fail, i=%d\n", szFile, i );
            fclose( hFile );
            *iNumRead = k;	 
            return( -1 );
         }

/* Does the channel just read in match one in our array? */
         for ( j=0; j<iNumStas; j++ )
            if ( !strcmp( ch[m].szStation, StaArray[j].szStation ) &&	 
                 !strcmp( ch[m].szChannel, StaArray[j].szChannel ) &&
                 !strcmp( ch[m].szLocation, StaArray[j].szLocation ) &&
                 !strcmp( ch[m].szNetID, StaArray[j].szNetID ) )
            {	 
               if ( k == 0 )
               {        
                  lStart = (int32_t) ((dStartTime-dFileTime) /
                                     (1./ch[m].dSampRate) + 0.01);
/* NOTE: Some old files have bad start times encoded in the files, using the
   correction below will give the time within 1 sample */
                  StaArray[j].dEndTime = GetTimeFromFileName( szFile );
                  StaArray[j].dStartTime = dStartTime;
                  NewDateFromModSec( &StaArray[j].stStartTime, 
                                      StaArray[j].dStartTime + 3506630400. );
               }
               else lStart = 0;
/* Copy raw data into proper memory location */
               for ( l=lStart; l<ch[m].lNumSamps; l++ )
               {
                  if ( StaArray[j].lRawCircCtr < StaArray[j].lRawCircSize )
                  {
                     StaArray[j].plRawCircBuff[StaArray[j].lRawCircCtr] =
                      lTemp[l];
                     StaArray[j].lRawCircCtr   += 1;
                     StaArray[j].lSampIndexF   += 1;
                     StaArray[j].dEndTime      += (1./ch[m].dSampRate);
                  }
               }
               break;
			}
      }      
      fclose( hFile );
      dFileTime += ((double) iFileSize*60.);     /* Increment for next read */
   }   
   *iNumRead = k+1;	 
   return( 0 );
}
                                             
      /******************************************************************
       *                    ReadDiskDataNew()                           *
       *                                                                *
       *  This function reads data from a file created by disk_wcatwc,  *
       *  and fills up a pre-created space in the STATION structure.    *
       *  NOTE: All data is 4 byte data, even if it is 12 bit data.     *
       *                                                                *
       *  The format is an internal NTWC seismic data format.  The      *
       *  format looks like:                                            *
       *                                                                *
       *  structure DISKHEADER;                                         *
       *  structure CHNLHEADER * iNumStations;                          *
       *  chn 0 samp0, chn 0 samp1, ... chn 0 lNumSamps-1;              *
       *  chn 1 samp0, chn 1 samp1, ... chn 1 lNumSamps-1;              *
       *                 .                                              *
       *                 .                                              *
       *                 .                                              *
       *  chn NSta-1 samp0, chn NSta-1 samp1, ... chn NSta-1 lNumSamps-1*
       *                                                                *
       *  Samp0 from each channel is the same time (well, almost), etc. *
       *                                                                *
       *  The data is written in binary (always i4).  The file name     *
       *  describes what time the file holds.  iFileSize is read in     *
       *  from the .d file.  This tells how many minutes of seismic     *
       *  data there is per file.  The names are in the format          *
       *  [szRootDirectory]\Dyymmdd\Smdhhmm[FileSuffix]yy.              *
       *  The first sample in the file is the first sample after the    *
       *  time given in the file name.  The file name time is called    *
       *  the nominal time.                                             *
       *                                                                *
       *  Arguments:                                                    *
       *   pszFile          File to read data from                      *
       *   Sta              Complete array of station data              *
       *   iNumStas         Number of stations to read                  *
       *   iInit            1 = Initialize array structure              *
       *   iFlag            1 = memcpy to filtered array; 0 = Don't     *
       *   iLP              1 = LP data; 0 = SP data                    *
       *   iFilter          1 = Filter the data; 0 = Don't              *
       *   dFiltL           Low frequency filter cutoff                 *
       *   dFiltH           High frequency filter cutoff                *
       *   dTaper           Filter Taper length                         *
       *                                                                *
       *  Return:           int   0 if data read OK, -1 if problem,     *
       *                          1 if file not there                   *
       *                                                                *
       ******************************************************************/
int ReadDiskDataNew( char *pszFile, STATION Sta[], int iNumStas, 
                     int iInit, int iFlag, int iLP, int  iFilter, double dFiltL,
                     double dFiltH, double dTaper )
{
   static  CHNLHEADER ch;      /* CHNLHEADER information (see earlybirdlib.h) */
   static  FILE *hFile;        /* Handle to seismic data file */
   static  DISKHEADER dh;      /* DISKHEADER information (see earlybirdlib.h) */
   static  double dPreTime;    /* Time (s) to look for DC offset */
   static  double dStartTime;  /* Nominal File start time in epochal Seconds */
   double  dTemp;
   int     i, iTemp;
   LATLON  ll, llOut;
   int32_t lTemp;
   SYSTEMTIME stDate;
   double dSCNLTime;
   
/*  Here is the date that NTWC started using SCNL versus SCN */
   stDate.wYear         = 2009;
   stDate.wMonth        = 10;
   stDate.wDay          = 14;
   stDate.wHour         = 12;
   stDate.wMinute       = 0;
   stDate.wSecond       = 0;
   stDate.wMilliseconds = 0;
/* Convert time to modified Julian seconds */
   DateToModJulianSec( &stDate, &dSCNLTime );
   dSCNLTime -= 3506630400.;  /* Then to epochal seconds */
/* For filtering... */
   if ( iLP == 1 ) dPreTime = PRE_LP_TIME;
   else            dPreTime = PRE_P_TIME;

/* Try to open data file */
   if ( (hFile = fopen( pszFile, "rb" )) == NULL ) 
   {
      logit( "t", "File %s not opened in ReadDiskData\n", pszFile );
      return 1;
   }

/* Read in the DISKHEADER structure */
   if ( fread( &dh, sizeof( DISKHEADER ), 1, hFile ) < 1 )
   {
      fclose( hFile );
      logit( "t", "DISKHEADER read failed in file %s ReadDiskData\n", pszFile );
      return -1;
   }
   
/* Convert time to modified Julian seconds */
   DateToModJulianSec( &dh.stStartTime, &dStartTime );
   dStartTime -= 3506630400.;  /* Then to epochal seconds */
   if ( dStartTime < 100. )              /* Must be bad file */
   {
      fclose( hFile );
      logit( "t", "dStartTime incorrect (%lf), %s\n", dStartTime, pszFile );
      return -1;
   }
   if ( dh.iNumChans != iNumStas )          /* Number of stations has changed */
   {
      fclose( hFile );
      logit( "t", "%s - # stns read (%d) not expected (%d)\n", pszFile,
             dh.iNumChans, iNumStas );
      return -1;
   }
    
/* For each channel, read in the header info and log to Sta */
   for ( i=0; i<dh.iNumChans; i++ )
   {
/* Need to initialize this char array. */
/* *********************************** */
      strcpy( ch.szLocation, "--" );
      if ( (int) fread( &ch, dh.iChnHdrSize, 1, hFile ) < 1 ) 
      {
         fclose( hFile );
         logit( "t", "CHNLHEADER read failed, file %s ReadDiskData\n", pszFile);
         return -1;
      }
      Sta[i].lSampsInLastPacket = ch.lNumSamps;
      if ( iInit == 1 )                     /* Initialize array on first read */
      {
         strcpy( Sta[i].szStation,  ch.szStation );
         strcpy( Sta[i].szChannel,  ch.szChannel );
         strcpy( Sta[i].szNetID,    ch.szNetID );
         strcpy( Sta[i].szLocation, ch.szLocation );
         Sta[i].dSampRate = ch.dSampRate;
         ll.dLat = ch.dLat;       /* Convert geocentric lat/lon to geographic */
         ll.dLon = ch.dLon;       /*  if file older than 2/10/2001            */
         DateToModJulianSec( &dh.stStartTime, &dTemp );
         if ( dTemp < 4488393600. )
         {
            GeoGraphic( &llOut, &ll );
            Sta[i].dLat       = llOut.dLat;
            Sta[i].dLon       = llOut.dLon;
            Sta[i].dElevation = ch.dElevation*EARTHRAD;
         }
         else
         {
            Sta[i].dLat       = ll.dLat;
            Sta[i].dLon       = ll.dLon;                
            Sta[i].dElevation = ch.dElevation;
         }
         Sta[i].dSens            = ch.dGain;
         Sta[i].dGainCalibration = ch.dGainCalibration;
         Sta[i].dClipLevel       = ch.dClipLevel;
         Sta[i].dTimeCorrection  = ch.dTimeCorrection;
         Sta[i].iStationType     = ch.iStationType;
         Sta[i].iSignalToNoise   = ch.iSignalToNoise;
         Sta[i].iTrigger         = ch.iTrigger;
         Sta[i].iBytePerSamp     = ch.iBytePerSamp;
         Sta[i].dScaleFactor     = ch.dScaleFactor;  
/* Fix the LP scaling for events before 10/10/2000 */
         if ( Sta[i].dSampRate < 3. && Sta[i].dScaleFactor > 0.005 &&
              dStartTime+3506630400. < 4477766400. )
            Sta[i].dScaleFactor = 0.005;
         Sta[i].iDisplayStatus = 1;
/* Set iComputeMwp */
         Sta[i].iComputeMwp = 0;
         if ( (Sta[i].szChannel[0] == 'B'  || 
               Sta[i].szChannel[0] == 'H') &&
               Sta[i].szChannel[1] == 'H'  &&
               Sta[i].szChannel[2] == 'Z'  &&
               Sta[i].dSens > 0. ) Sta[i].iComputeMwp = 1;
      }

/* Here, figure out where in the buffer we should put the data, and the buffer
   indices and start/end times.  There are 4 options here: 1) Re-Initialize the
   buffer, 2) Shift into earlier times, 3) Shift into later times, or 4)
   Buffer already covers this time, patch it in. */
      if ( iInit == 1 )            /* Start of Sta is start of file - rewrite */
      {
         CopyDate( &dh.stStartTime, &Sta[i].stStartTime );
         Sta[i].dStartTime = dStartTime;
         Sta[i].dEndTime = Sta[i].dStartTime + 
	  ((double) (Sta[i].lSampsInLastPacket-1)/Sta[i].dSampRate);
         Sta[i].lIndex = 0;                            /* Index of dStartTime */
         Sta[i].lIndexToStartWrite = 0;
         Sta[i].iHasWrapped = 0;
      }
      else if ( dStartTime < Sta[i].dStartTime )    /* Buffer must shift back */
      {               /* lIndex = index of dStartTime (earliest time in file) */
         Sta[i].lIndex -= (long) ((Sta[i].dStartTime - 
                                   dStartTime) * Sta[i].dSampRate + 0.0001);
         if ( Sta[i].lIndex < 0 ) 
         {
            Sta[i].iHasWrapped = 1;
            Sta[i].lIndex += Sta[i].lRawCircSize;
         }
         CopyDate( &dh.stStartTime, &Sta[i].stStartTime );
         Sta[i].dStartTime = dStartTime;
         Sta[i].lIndexToStartWrite = Sta[i].lIndex;
/* Adjust the end time if we have gotten more data than can fit into the array*/
         lTemp = (long) ((Sta[i].dEndTime-Sta[i].dStartTime)
                  * Sta[i].dSampRate + 0.001) + 1;
         if ( lTemp > Sta[i].lRawCircSize ) Sta[i].dEndTime -= 
             (double) (lTemp-Sta[i].lRawCircSize) / Sta[i].dSampRate;
      }
      else if ( dStartTime + ((double) (Sta[i].lSampsInLastPacket-1) / 
                Sta[i].dSampRate) > Sta[i].dEndTime )
      {                                          /* Buffer must shift forward */
/* Add to dEndTime as we are getting data more in the future */
         Sta[i].dEndTime = dStartTime + ((double)
          (Sta[i].lSampsInLastPacket-1) / Sta[i].dSampRate);
         lTemp = (long) ((Sta[i].dEndTime-Sta[i].dStartTime)
                  * Sta[i].dSampRate + 0.001) + 1;
         if ( lTemp > Sta[i].lRawCircSize )   /* Adjust start time and lIndex */
         {                                /* if we have gotten more data than */
            Sta[i].dStartTime +=                      /* can fit in the array */
             (lTemp-Sta[i].lRawCircSize) / Sta[i].dSampRate;
            NewDateFromModSec( &Sta[i].stStartTime, 
                               Sta[i].dStartTime + 3506630400. );
            Sta[i].lIndex += (lTemp-Sta[i].lRawCircSize);
            if ( Sta[i].lIndex >= Sta[i].lRawCircSize ) 
                 Sta[i].lIndex -= Sta[i].lRawCircSize;
            Sta[i].iHasWrapped = 1;
         }
         Sta[i].lIndexToStartWrite = (long) ((dStartTime-Sta[i].dStartTime) * 
          Sta[i].dSampRate + 0.001) + Sta[i].lIndex;
         if ( Sta[i].lIndexToStartWrite >= Sta[i].lRawCircSize ) 
              Sta[i].lIndexToStartWrite -= Sta[i].lRawCircSize;
      }
      else             /* Patch data into proper part of array */
      {
         Sta[i].lIndexToStartWrite = (long) ((dStartTime - 
          Sta[i].dStartTime) * Sta[i].dSampRate + 0.0001) +
          Sta[i].lIndex;
         if ( Sta[i].lIndexToStartWrite >= Sta[i].lRawCircSize ) 
              Sta[i].lIndexToStartWrite -= Sta[i].lRawCircSize;
      }
      Sta[i].lSampIndexR = Sta[i].lIndex + (long)((Sta[i].dEndTime-
       Sta[i].dStartTime)*Sta[i].dSampRate + 0.001) + 1;
      if ( Sta[i].lSampIndexR > Sta[i].lRawCircSize ) 
         Sta[i].lSampIndexR -= Sta[i].lRawCircSize;
      Sta[i].lSampIndexF   = Sta[i].lSampIndexR;
      if ( i == iNumStas-1 ) break;  /* Don't overwrite array */
   }
      
/* For each channel, read in the data and log to Sta */
   for ( i=0; i<dh.iNumChans; i++ )
   {
      if ( Sta[i].lSampsInLastPacket > Sta[i].lRawCircSize ) 
      {
         logit( "t", "i=%d, # bytes/chn in file > Sta[i].lRawCircSize, "
                     "bytes=%ld Sta[i].lRawCircSize=%ld\n", i, 
                     Sta[i].lSampsInLastPacket, Sta[i].lRawCircSize );
         fclose( hFile );
         return -1;
      }
      
/* Read the file and put the data in the correct spot */
/* Check to make sure that we will not overwrite the buffer */
      if ( Sta[i].lSampsInLastPacket+Sta[i].lIndexToStartWrite >
           Sta[i].lRawCircSize )                  /* Then we must split reads */
      {
         lTemp = (Sta[i].lSampsInLastPacket+Sta[i].lIndexToStartWrite) - 
                  Sta[i].lRawCircSize;
         if ( (int) fread( &Sta[i].plRawCircBuff[Sta[i].lIndexToStartWrite], 
               Sta[i].iBytePerSamp, Sta[i].lSampsInLastPacket-lTemp, hFile ) < 
               Sta[i].lSampsInLastPacket-lTemp )
         {
            logit( "t", "start at %ld, byte=%d, samps=%ld\n",
    	           Sta[i].lIndexToStartWrite, Sta[i].iBytePerSamp,
    	           Sta[i].lSampsInLastPacket );
            logit( "t", "File %s fread3a fail\n", pszFile );
            fclose( hFile );
            return -1;
         }
/* Fill filtered data arrays */
         if ( iFlag == 1 )
            memcpy( &Sta[i].plFiltCircBuff[Sta[i].lIndexToStartWrite], 
                    &Sta[i].plRawCircBuff[Sta[i].lIndexToStartWrite], 
                    (Sta[i].lSampsInLastPacket-lTemp)*sizeof( int32_t ) );
         if ( (int) fread( &Sta[i].plRawCircBuff[0], Sta[i].iBytePerSamp, lTemp, 
                            hFile ) < lTemp )
         {
            logit( "t", "start at %ld, byte=%d, samps=%ld\n",
    	           Sta[i].lIndexToStartWrite, Sta[i].iBytePerSamp,
    	           Sta[i].lSampsInLastPacket );
            logit( "t", "File %s fread3b fail\n", pszFile );
            fclose( hFile );
	    return -1;
         }
/* Fill filtered data arrays */
         if ( iFlag == 1 )
            memcpy( &Sta[i].plFiltCircBuff[0], &Sta[i].plRawCircBuff[0], 
                    lTemp*sizeof( int32_t ) );
      }
      else                                      /* We can make it in one read */
      {
         if ( (iTemp = (int) fread( &Sta[i].plRawCircBuff[Sta[i].lIndexToStartWrite], 
              (size_t) Sta[i].iBytePerSamp, (size_t) Sta[i].lSampsInLastPacket, 
               hFile )) < Sta[i].lSampsInLastPacket )
         {
            logit( "t", "start at %ld, byte=%d, samps=%ld\n",
    	           Sta[i].lIndexToStartWrite, Sta[i].iBytePerSamp,
    	           Sta[i].lSampsInLastPacket );
            logit( "t", "File %s fread3 fail\n", pszFile );
	    fclose( hFile );
    	    return -1;
         }
/* Fill filtered data arrays */
         if ( iFlag == 1 )
            memcpy( &Sta[i].plFiltCircBuff[Sta[i].lIndexToStartWrite], 
                    &Sta[i].plRawCircBuff[Sta[i].lIndexToStartWrite], 
                     Sta[i].lSampsInLastPacket*sizeof( int32_t ) );
      }
      if ( i == iNumStas-1 ) break;                  /* Don't overwrite array */
   }
   fclose( hFile );
   return 0;
}

      /******************************************************************
       *                        ReadDiskHeader()                        *
       *                                                                *
       *  This function reads the disk header from a seismic data       *
       *  file created in disk_wcatwc.  The number of channels of data  *
       *  is returned.                                                  *
       *                                                                *
       *  Arguments:                                                    *
       *   pszFile          File to read                                *
       *   StaArray         Complete array of station data              *
       *   iMaxRead         Max number of channels to allow             *
       *                                                                *
       *  Return:           -1 if problem; otherwise # channels         *
       *                                                                *
       ******************************************************************/
	   
int ReadDiskHeader( char *pszFile, STATION Sta[], int iMaxRead )
{
   CHNLHEADER ch;                    /* CHNLHEADER information (see diskrw.h) */
   DISKHEADER dh;                    /* DISKHEADER info (see diskrw.h) */
   double     dTemp;
   FILE	     *hFile;                 /* Handle to seismic data file */
   int        i;
   LATLON     ll, llOut;


/* Need to initialize this char array. */
/* *********************************** */
   strcpy( ch.szLocation, "--" );

/* First, try to open data file */
   if ( (hFile = fopen( pszFile, "rb" )) == NULL ) 
   {
      logit ("t", "File %s not opened in ReadDiskHeader\n", pszFile);
      return -1;
   }

/* Read in the DISKHEADER structure */
   if ( fread( &dh, sizeof (DISKHEADER), 1, hFile ) < 1 ) 
   {	
      fclose( hFile );
      logit( "t", "DISKHEADER read failed: file %s\n", pszFile );
      return -1;
   }
   
/* Prevent array overflow */
   if ( dh.iNumChans > iMaxRead )   
   {	
      fclose( hFile );
      logit( "t", "Too many channels (%d) in %s\n", dh.iNumChans, pszFile );
      return -1;
   }

/* For each channel, read in the header info and log to Sta */
   for ( i=0; i<dh.iNumChans; i++ )
   {
      if ( (int) fread( &ch, dh.iChnHdrSize, 1, hFile ) < 1 )  
      {
         fclose( hFile );
         logit( "t", "CHNLHEADER read failed, file %s\n", pszFile);
         return -1;
      }
      strcpy( Sta[i].szStation, ch.szStation );
      strcpy( Sta[i].szChannel, ch.szChannel );
      strcpy( Sta[i].szNetID, ch.szNetID );
      strcpy( Sta[i].szLocation, ch.szLocation );
      ll.dLat = ch.dLat;   /* Convert geocentric lat/lon to geographic */
      ll.dLon = ch.dLon;   /*  if file older than 2/10/2001 */
      DateToModJulianSec( &dh.stStartTime, &dTemp );
      if ( dTemp < 4488393600. )
      {
         GeoGraphic( &llOut, &ll );
         Sta[i].dLat = llOut.dLat;
         Sta[i].dLon = llOut.dLon;
         Sta[i].dElevation = ch.dElevation*EARTHRAD;
      }
      else
      {
         Sta[i].dLat = ll.dLat;
         Sta[i].dLon = ll.dLon;                
         Sta[i].dElevation = ch.dElevation;
      }
      Sta[i].lSampsInLastPacket = ch.lNumSamps;
      Sta[i].dSampRate = ch.dSampRate;
      Sta[i].dSens = ch.dGain;
      Sta[i].dGainCalibration = ch.dGainCalibration;
      Sta[i].dClipLevel = ch.dClipLevel;
      Sta[i].dTimeCorrection = ch.dTimeCorrection;
      Sta[i].dScaleFactor = ch.dScaleFactor;  
      Sta[i].iStationType = ch.iStationType;
      Sta[i].iSignalToNoise = ch.iSignalToNoise;
      Sta[i].iPickStatus = ch.iPickStatus;
      Sta[i].iTrigger = ch.iTrigger;
      Sta[i].iDisplayStatus = 1;
/* Fix the LP scaling for events before 10/10/2000 */
      if ( dTemp < 4477766400. &&
           Sta[i].dSampRate < 3. && Sta[i].dScaleFactor > 0.005 )
         Sta[i].dScaleFactor = 0.005;
   }
   fclose( hFile );
   return dh.iNumChans;
}                                  

     /**************************************************************
      *                  ReadLineupFile()                          *
      *                                                            *
      * This function reads a file which has station information   *
      * created in ATPlayer.                                       *
      *                                                            *
      * Arguments:                                                 *
      *  pszFile           File name to create                     *
      *  Sta               Station array                           *
      *  iMaxStn           Max num of stations to allow            *
      *                                                            *
      * Returns: int Number of stations read                       *
      *                                                            *
      **************************************************************/
      
int ReadLineupFile( char *pszFile, STATION **Sta, int iMaxStn )
{
   FILE   *hFile;               /* File handle */
   int     i;
   int     iNumSta;             /* Number of stations in the Player file */
   STATION *sta;
   STATION StaTemp;             /* Temp used while counting number of stns */

/* Open file */   
   hFile = fopen( pszFile, "r" );
   if ( hFile == NULL )
   {
      logit( "", "%s could not be opened in ReadLineupFile\n", pszFile );
      return (-1);
   }
   
/* First time through, count number of stations */
   i=0;
   while ( !feof( hFile ) )
   {
      fscanf( hFile, "%s %s %s %s %lf %lf %lf %lf %lf %lf %lf %lf %lf %d %d %d %d %d %lf %lf %lf %d\n",
       StaTemp.szStation,
       StaTemp.szChannel,
       StaTemp.szNetID,
       StaTemp.szLocation,
       &StaTemp.dLat,
       &StaTemp.dLon,
       &StaTemp.dElevation,
       &StaTemp.dSampRate,
       &StaTemp.dSens,
       &StaTemp.dGainCalibration,
       &StaTemp.dClipLevel,
       &StaTemp.dTimeCorrection,
       &StaTemp.dScaleFactor,
       &StaTemp.iStationType,
       &StaTemp.iSignalToNoise,
       &StaTemp.iPickStatus,
       &StaTemp.iFiltStatus,
       &StaTemp.iAlarmStatus,
       &StaTemp.dAlarmAmp,
       &StaTemp.dAlarmDur,
       &StaTemp.dAlarmMinFreq,
       &StaTemp.iComputeMwp );
      i++;
   }       
   iNumSta = i;
   rewind( hFile );

/* Allocate the station array
   **************************/
   if ( iNumSta > iMaxStn )   /* But first see if there are too many stns */
   {
      logit( "et", "Too many stations in Station file - %s\n", iNumSta );
      return -1;
   }   
   sta = (STATION *) calloc( iNumSta, sizeof( STATION ) );
   if ( sta == NULL )
   {
      logit( "et", "Cannot allocate the station array\n" );
      return -1;
   }
   
/* Read file */
   i=0;
   for ( i=0; i<iNumSta; i++ )
   {
      InitVar( &sta[i] );	  
      sta[i].dEndTime = 0.;
      sta[i].iFirst   = 1;
      fscanf( hFile, "%s %s %s %s %lf %lf %lf %lf %lf %lf %lf %lf %lf %d %d %d %d %d %lf %lf %lf %d\n",
       sta[i].szStation,
       sta[i].szChannel,
       sta[i].szNetID,
       sta[i].szLocation,
       &sta[i].dLat,
       &sta[i].dLon,
       &sta[i].dElevation,
       &sta[i].dSampRate,
       &sta[i].dSens,
       &sta[i].dGainCalibration,
       &sta[i].dClipLevel,
       &sta[i].dTimeCorrection,
       &sta[i].dScaleFactor,
       &sta[i].iStationType,
       &sta[i].iSignalToNoise,
       &sta[i].iPickStatus,
       &sta[i].iFiltStatus,
       &sta[i].iAlarmStatus,
       &sta[i].dAlarmAmp,
       &sta[i].dAlarmDur,
       &sta[i].dAlarmMinFreq,
       &sta[i].iComputeMwp );
   }       
   fclose( hFile );
   *Sta   = sta;
   return( iNumSta );
}


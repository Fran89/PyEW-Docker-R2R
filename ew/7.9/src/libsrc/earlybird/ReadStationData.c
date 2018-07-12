
 /************************************************************************
  * ReadStationData.c                                                    *
  *                                                                      *
  * These functions read in meta-data and controld data for the seismic  *
  * stations used in Earlybird.  The data comes from several files,      *
  * mainly *.sta, station.dat, calibs, and the screen.ini file.          *
  *                                                                      *
  * Moved into libsrc folder in generalized in 5/2008.                   *
  *                                                                      *
  ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "earlybirdlib.h"      

  /***************************************************************
   *                GetSavedStationData()                        *
   *                                                             *
   * This function will attempt to load the station array from a *
   * previously saved file. If the station savefile cannot be    *
   * loaded - return a -1, so that regular station load code     *
   * will execute - this will create the save file for the next  *
   * start.                                                      *    
   *                                                             *
   * Sta        Pointer to array of STATION structures with data *
   * piNSta     Number of stations in file                       *
   * pszStaList Binary file with station meta-data               *
   *                                                             *
   *  Returns: -1 means the file could not be read, 0 if ok.     *
   ***************************************************************/

int GetSavedStationData( STATION **Sta, int *piNSta, char *pszStaList )
{
   FILE    *hFile = NULL;    
   int      iNumBytes = 0;         /* Number of bytes in bunary file */
   int      iStaCnt   = 0;         /* Station counter */
   STATION *sta = NULL;
   char     szFileName[256];       /* JMC 12.30.13 - File name of binary file */

/* Create file name based on ...sta file */
   sprintf( szFileName, "%s.bin", pszStaList );
   if ( (hFile = fopen( szFileName, "rb" )) == NULL )
   {
      logit( "et", "%s(%d) %s could not be opened.\n", 
             __FILE__, __LINE__, szFileName );
      return -1;
   }

/* Get number of bytes in binary file and compute number of stations */
   fseek( hFile, 0, SEEK_END );
   iNumBytes = ftell( hFile );
   rewind( hFile );
   iStaCnt = iNumBytes / sizeof( STATION );
   logit( "et", "%s(%d) byte count %d - saved recs %d on %s.\n", 
          __FILE__, __LINE__, iNumBytes, iStaCnt, szFileName );

/* Allocate memory for station array */
   sta = (STATION *) calloc( iStaCnt, sizeof( STATION ) );
   if ( (hFile = fopen( szFileName, "rb" )) != NULL )
   {
      fread( (STATION *)sta, sizeof( STATION ), iStaCnt, hFile );
      fclose( hFile );
     *Sta    = sta;
     *piNSta = iStaCnt;
      return 0;
   }
   return -1;
}

  /***************************************************************
   *                     GetStationName()                        *
   *                                                             *
   * Given a station/channel/net, get the station name.          *
   *                                                             *
   * pszStaFile File with statioN data (station.dat)             *
   * sta        Pointer to array of STATION structures with data *
   * iMaxStn    Maximum number of stations to allow              *
   *                                                             *
   *  Returns -1 if an error is encountered, 0 if ok.            *
   ***************************************************************/

int GetStationName( char *pszStaFile, STATION *pSta, int iMaxStn, char *pszSta )
{
   FILE    *hFile;
   int     i;
   int     iNDecoded;                   /* Number of fields in line read */
   STATION StaTemp;                     /* Temp Station Structure */
   char    szString[256];               /* Line from file with information */
   char    FullTablePath[512];

#ifdef _WINNT
   char   *paramdir;

   paramdir = getenv( "EW_PARAMS" ); 
   if ( paramdir == (char *) NULL ) return '\0';
   sprintf( FullTablePath, "%s\\%s", paramdir, pszStaFile );
#else
   sprintf( FullTablePath, "%s", pszStaFile );                    /* For unix */
#endif
   strcpy( pszSta, "\0" );

/* Open the station data file
   **************************/
   if ( ( hFile = fopen( FullTablePath, "r") ) == NULL )
   {
      logit( "et", "GSN-Error opening station data file <%s>.\n", pszStaFile );
      return( -1 );
   }
   
/* Read station data from the station data file
   ********************************************/
   i = 0;
   while ( fgets( szString, 256, hFile ) != NULL )
   {
      iNDecoded = sscanf( szString, "%s %s %s %s %lf %lf %lf %lf %lf %lf %lf %d %d %s",
       StaTemp.szStation, StaTemp.szChannel, StaTemp.szNetID, 
       StaTemp.szLocation, &StaTemp.dSens, &StaTemp.dGainCalibration, 
       &StaTemp.dLat, &StaTemp.dLon, &StaTemp.dElevation,
       &StaTemp.dClipLevel, &StaTemp.dTimeCorrection,					
       &StaTemp.iStationType, &StaTemp.iAgency, StaTemp.szStationName );
      if ( iNDecoded < 14 )
      {
         logit( "et", "GSN-Error decoding station file-%s.\n", pszStaFile );
         logit( "e", "ndecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szString );
         fclose( hFile );
         return( -1 );
      }

      if ( !strcmp( pSta->szStation, StaTemp.szStation ) &&
           !strcmp( pSta->szChannel, StaTemp.szChannel ) &&
           !strcmp( pSta->szNetID,   StaTemp.szNetID ) )
      {
         fclose( hFile );
         strcpy( pszSta, StaTemp.szStationName );
         return( 0 );
      }
      
      i++;
      if ( i == iMaxStn )   /* See if there are too many stns */
      {
         logit( "", "GSN-Max number of stations (%d) reached in %s\n",
                i, FullTablePath );
         break;
      }
   }
   fclose( hFile );
   return( -1 );
}

    /*************************************************************************
     *                             IsComment()                               *
     *                                                                       *
     *  Accepts: String containing one line from a mtinver station list      *
     *  Returns: 1 if it's a comment line                                    *
     *           0 if it's not a comment line                                *
     *************************************************************************/

int IsComment( char string[] )
{
   int i;

   for ( i=0; i<(int) strlen( string ); i++ )
   {
      char test = string[i];
      if ( test != ' ' && test != '\t' && test != '\n' )
      {
         if ( test == '#'  )
            return 1;          /* It's a comment line */
         else
            return 0;          /* It's not a comment line */
      }
   }
   return 1;                   /* It contains only whitespace */
}

  /***************************************************************
   *                       LoadResponseData()                    *
   *                                                             *
   *  Read response information in poles/zeroes form.            *
   *                                                             *
   *  Returns -1 if an error is encountered, 0 if no match found.*
   ***************************************************************/

int LoadResponseData( STATION *Sta, char *pszRespFile )
{
   double  dTemp;
   double  dAmp0;             /* Station stage 0 sensitivity */ 
   FILE    *hFile;
   int     i, iTemp, j, iLine;
   int     iNDecoded;         /* Number of fields successfully read in sscanf */
   int     iNPole, iNZero;    /* Number of poles and number of zeroes */
   int     iUse;              /* Calibs flag indicating whether or not to use */
   char    szChannel[TRACE_CHAN_LEN], szStation[TRACE_STA_LEN],
           szNetID[TRACE_NET_LEN], szLocation[TRACE_STA_LEN];
   char    szLine[128];
   fcomplex zPoles[MAX_ZP];   /* poles of response function */
   fcomplex zZeros[MAX_ZP];   /* zeros of response function */
   char  FullTablePath[512];

#ifdef _WINNT
   char *paramdir;

   paramdir = getenv( "EW_PARAMS" ); 
   if ( paramdir == (char *) NULL ) return -1;
   sprintf( FullTablePath, "%s\\%s", paramdir, pszRespFile );
#else
   sprintf( FullTablePath, "%s", pszRespFile );                    /* For unix */
#endif

/* First, intialize information in the structure
   *********************************************/
   Sta->dAmp0  = 0.;
   Sta->iNZero = 0;
   Sta->iNPole = 0;
   for ( i=0; i<MAX_ZP; i++ )
   {
      Sta->zPoles[i] = Complex( 0., 0. );
      Sta->zZeros[i] = Complex( 0., 0. );   
   }
         
/* Open the response file
   **********************/
   if ( (hFile = fopen( FullTablePath, "r" )) == NULL )
   {
      logit( "et", "Error opening response file <%s>.\n", pszRespFile );
      return -1;
   }

/* Read response data from the response file 
   *****************************************/
   iLine = 0;
   while ( !feof( hFile ) )
   {
      if ( fgets( szLine, 126, hFile ) == NULL ) break;
      iNDecoded = sscanf( szLine, "%*s %s %s %s %s %*s %*f %*f %*f %*s %*s %d",
                          szStation, szChannel, szNetID, szLocation, &iUse );
      if ( iNDecoded < 4 )
      {
         logit( "et", "Error decoding response file - 1.\n" );
         logit( "e", "ndecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szLine );
         logit( "e", "Line # = %d\n", iLine );
         fclose( hFile );
         return -1;
      }
      if ( fgets( szLine, 126, hFile ) == NULL ) break;
      sscanf( szLine, "%lf %lf %d", &dTemp, &dTemp, &iTemp );
      if ( fgets( szLine, 126, hFile ) == NULL ) break;
      iNDecoded = sscanf( szLine, "%*s %*d %*c %lf %d %d", &dAmp0, &iNPole,
                          &iNZero );			  
      iLine += 4;
      if ( iNDecoded < 3 )
      {
         logit( "et", "Error decoding response file - 2.\n" );
         logit( "e", "ndecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szLine );
         logit( "e", "Line # = %d\n", iLine );
         fclose( hFile );
         return -1;
      }
      if ( iNPole > MAX_ZP || iNZero > MAX_ZP )
      {
         logit( "et", "Too many poles or zeroes - p=%d, z=%d\n", iNPole,
                                                                 iNZero );
         logit( "e", "Max poles/zeroes: %d\n", MAX_ZP );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szLine );
         logit( "e", "Line # = %d\n", iLine );
         fclose( hFile );
         return -1;
      }
      for ( j=0; j<iNPole; j++ )
      {
         if ( fgets( szLine, 126, hFile ) == NULL ) break;
         sscanf( szLine, "%e %e", &zPoles[j].r, &zPoles[j].i );
      }
      for ( j=0; j<iNZero; j++ )
      {
         if ( fgets( szLine, 126, hFile ) == NULL ) break;
         sscanf( szLine, "%e %e", &zZeros[j].r, &zZeros[j].i );
      }
      iLine += (iNPole+iNZero);
      dAmp0 *= 1.0e9;                 /* To match Benz's cal file */
      if ( !strcmp( szStation, Sta->szStation ) &&  /* Match resp file with */
           !strcmp( szChannel, Sta->szChannel ) &&  /*  station             */
           !strcmp( szLocation, Sta->szLocation ) && /*  Location             */
           !strcmp( szNetID, Sta->szNetID ) && iUse == 1 )
      {
         Sta->dAmp0  = dAmp0;
         Sta->iNPole = iNPole;
         Sta->iNZero = iNZero;
         for ( j=0; j<iNPole; j++ )
         {
            Sta->zPoles[j].r = zPoles[j].r;
            Sta->zPoles[j].i = zPoles[j].i;
         }
         for ( j=0; j<iNZero; j++ )
         {
            Sta->zZeros[j].r = zZeros[j].r;
            Sta->zZeros[j].i = zZeros[j].i;
         }
         fclose( hFile );
         return 1;
      }
      if ( fgets( szLine, 126, hFile ) == NULL ) break;
   }	  
   fclose( hFile );
   return 0;
}

  /***************************************************************
   *                    LoadResponseDataAll()                    *
   *                                                             *
   *  Read response information in poles/zeroes form for all stns*
   *                                                             *
   *  Returns -1 if an error is encountered, 0 if ok.            *
   ***************************************************************/

int LoadResponseDataAll( STATION Sta[], char *pszRespFile, int iNumSta )
{
   double  dTemp;
   double  dAmp0;             /* Station stage 0 sensitivity */ 
   FILE    *hFile;
   int     i, iTemp, j, iLine;
   int     iNDecoded;         /* Number of fields successfully read in sscanf */
   int     iNPole, iNZero;    /* Number of poles and number of zeroes */
   int     iUse;              /* Calibs flag indicating whether or not to use */
   char    szChannel[TRACE_CHAN_LEN], szStation[TRACE_STA_LEN],
           szNetID[TRACE_NET_LEN], szLocation[TRACE_STA_LEN];
   char    szLine[128];
   fcomplex zPoles[MAX_ZP];   /* poles of response function */
   fcomplex zZeros[MAX_ZP];   /* zeros of response function */
   char  FullTablePath[512];

#ifdef _WINNT
   char *paramdir;

   paramdir = getenv( "EW_PARAMS" ); 
   if ( paramdir == (char *) NULL ) return -1;
   sprintf( FullTablePath, "%s\\%s", paramdir, pszRespFile );
#else
   sprintf( FullTablePath, "%s", pszRespFile );                    /* For unix */
#endif

/* First initialize response array to 0 */
   for ( i=0; i<iNumSta; i++ )
   {   
      Sta[i].dAmp0 = 0.;
      Sta[i].iNZero = 0;
      Sta[i].iNPole = 0;
      for ( j=0; j<MAX_ZP; j++ )
      {
         Sta[i].zPoles[j] = Complex( 0., 0. );
         Sta[i].zZeros[j] = Complex( 0., 0. );   
      }
   }
         
/* Open the response file
   **********************/
   if ( (hFile = fopen( FullTablePath, "r")) == NULL )
   {
      logit( "et", "Error opening response file 2 <%s>.\n", pszRespFile );
      return -1;
   }

/* Loop through enture response file and patch into structure response info
   ************************************************************************/
   while ( !feof( hFile ) )
   {
/* Read response data from the response file 
   *****************************************/
      iLine = 0;
      if ( fgets( szLine, 126, hFile ) == NULL ) break;
      iNDecoded = sscanf( szLine, "%*s %s %s %s %s %*s %*f %*f %*f %*s %*s %d",
                          szStation, szChannel, szNetID, szLocation, &iUse );
      if ( iNDecoded < 4 )
      {
         logit( "et", "Error decoding response file - 1a.\n" );
         logit( "e", "ndecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szLine );
         logit( "e", "Line # = %d\n", iLine );
         fclose( hFile );
         return -1;
      }
      if ( fgets( szLine, 126, hFile ) == NULL ) break;
      sscanf( szLine, "%lf %lf %d", &dTemp, &dTemp, &iTemp );
      if ( fgets( szLine, 126, hFile ) == NULL ) break;
      iNDecoded = sscanf( szLine, "%*s %*d %*c %lf %d %d", &dAmp0, &iNPole,
                          &iNZero );			  
      iLine += 4;
      if ( iNDecoded < 3 )
      {
         logit( "et", "Error decoding response file - 2a.\n" );
         logit( "e", "ndecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szLine );
         logit( "e", "Line # = %d\n", iLine );
         fclose( hFile );
         return -1;
      }
      if ( iNPole > MAX_ZP || iNZero > MAX_ZP )
      {
         logit( "et", "Too many poles or zeroes a - p=%d, z=%d\n", iNPole,
                                                                   iNZero );
         logit( "e", "Max poles/zeroes: %d\n", MAX_ZP );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szLine );
         logit( "e", "Line # = %d\n", iLine );
         fclose( hFile );
         return -1;
      }
      for ( j=0; j<iNPole; j++ )
      {
         if ( fgets( szLine, 126, hFile ) == NULL ) break;
         sscanf( szLine, "%e %e", &zPoles[j].r, &zPoles[j].i );
      }
      for ( j=0; j<iNZero; j++ )
      {
         if ( fgets( szLine, 126, hFile ) == NULL ) break;
         sscanf( szLine, "%e %e", &zZeros[j].r, &zZeros[j].i );
      }
      iLine += (iNPole+iNZero);
      dAmp0 *= 1.0e9;                 /* To match Benz's cal file */
/* Now see if there is match in station array */
      for ( i=0; i<iNumSta; i++ )
      {   
         if ( !strcmp( szStation, Sta[i].szStation )   &&  /* Match resp file with */
              !strcmp( szChannel, Sta[i].szChannel )   &&  /*  station             */
              !strcmp( szLocation, Sta[i].szLocation ) &&  /*  Location             */
              !strcmp( szNetID, Sta[i].szNetID ) && iUse == 1 )
         {
            Sta[i].dAmp0  = dAmp0;
            Sta[i].iNPole = iNPole;
            Sta[i].iNZero = iNZero;
            for ( j=0; j<iNPole; j++ )
            {
               Sta[i].zPoles[j].r = zPoles[j].r;
               Sta[i].zPoles[j].i = zPoles[j].i;
            }
            for ( j=0; j<iNZero; j++ )
            {
               Sta[i].zZeros[j].r = zZeros[j].r;
               Sta[i].zZeros[j].i = zZeros[j].i;
            }
            break;
         }
      }	  
      if ( fgets( szLine, 126, hFile ) == NULL ) break;
   }
   fclose( hFile );
   return 0;
}

  /***************************************************************
   *                       LoadStationData()                     *
   *                                                             *
   *       Get data on station from information file             *
   *                                                             *
   *  Returns -1 if an error is encountered or no match is found.*
   ***************************************************************/

int LoadStationData( STATION *Sta, char *pszInfoFile )
{
   int     iNDecoded;                   /* Number of fields in line read */
   FILE    *hFile;
   char    szChannel[TRACE_CHAN_LEN], szStation[TRACE_STA_LEN],
           szNetID[TRACE_NET_LEN], szLocation[TRACE_STA_LEN];
   char    szString[256];               /* Line from file with information */
   char  FullTablePath[512];

#ifdef _WINNT
   char *paramdir;

   paramdir = getenv( "EW_PARAMS" ); 
   if ( paramdir == (char *) NULL ) return -1;
   sprintf( FullTablePath, "%s\\%s", paramdir, pszInfoFile );
#else
   sprintf( FullTablePath, "%s", pszInfoFile );                    /* For unix */
#endif

/* Open the station data file
   **************************/
   if ( ( hFile = fopen( FullTablePath, "r") ) == NULL )
   {
      logit( "et", "Error opening station data file <%s>.\n", FullTablePath );
      return -1;
   }

/* Read station data from the station data file 
   ********************************************/
   while ( fgets( szString, 256, hFile ) != NULL )
   {
      iNDecoded = sscanf( szString, "%s %s %s %s", szStation, szChannel,
                          szNetID, szLocation );
	  if ( iNDecoded < 4 )
      {
         logit( "et", "Error decoding station data file-1 %s.\n", pszInfoFile );
         logit( "e", "ndecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szString );
         fclose( hFile );
         return -1;
      }
	  
/* Compare SCNs
   ************/	  
      if ( !strcmp( szStation, Sta->szStation ) &&
           !strcmp( szChannel, Sta->szChannel ) &&
           !strcmp( szLocation, Sta->szLocation ) &&  /*  Location             */
           !strcmp( szNetID, Sta->szNetID ) )
      {     /* We have a match */
         iNDecoded = sscanf( szString, "%s %s %s %s %lf %lf %lf %lf %lf %lf %lf %d %d %s",
          Sta->szStation, Sta->szChannel, Sta->szNetID, Sta->szLocation,
          &Sta->dSens, &Sta->dGainCalibration, &Sta->dLat, &Sta->dLon,
          &Sta->dElevation, &Sta->dClipLevel, &Sta->dTimeCorrection,					
          &Sta->iStationType, &Sta->iAgency, Sta->szStationName );
         if ( iNDecoded < 14 )
         {
            logit( "et", "Error decoding station data file-2 %s.\n",
                   pszInfoFile );
            logit( "e", "ndecoded: %d\n", iNDecoded );
            logit( "e", "Offending line:\n" );
            logit( "e", "%s\n", szString );
            fclose( hFile );
            return -1;
         }
         fclose( hFile );
         return 0;
      }
   }
   fclose( hFile );
   return -1;
}

 /***********************************************************************
  *                             LogStaList()                            *
  *                                                                     *
  *                         Log the station list                        *
  ***********************************************************************/

void LogStaList( STATION *Sta, int iNSta )
{
   int i;

   logit( "", "\nStation List:\n" );
   for ( i=0; i<iNSta; i++ )
   {
      logit( "", "%4s",    Sta[i].szStation );
      logit( "", " %3s",   Sta[i].szChannel );
      logit( "", " %2s ",  Sta[i].szNetID );
      logit( "", " %4s\n", Sta[i].szLocation );
   }
   logit( "", "\n" );
}

  /***************************************************************
   *                 ReadStationDataAlloc()                      *
   *                                                             *
   * Read the entire array of stations in the data base and then *
   * add response info.  Allocate the STATION structure          *
   *                                                             *
   * pszStaFile File with statioN data (station.dat)             *
   * pszStaResp File with station calibration info (calibs)      *
   * Sta        Pointer to array of STATION structures with data *
   * iMaxStn    Maximum number of stations to allow              *
   * piNumStn   Number of stations read in and allocated         *
   *                                                             *
   *  Returns -1 if an error is encountered; 1 if ok             *
   ***************************************************************/
   
int ReadStationDataAlloc( char *pszStaFile, char *pszStaResp, STATION **Sta,
                          int iMaxStn, int *piNumStn )
{
   FILE    *hFile;
   int     i, j;
   int     iNDecoded;                   /* Number of fields in line read */
   int     iStaCnt;                     /* Station counter */
   STATION *sta;
   STATION StaTemp;
   char    szString[256];               /* Line from file with information */
   char    szFullTablePath[512];

#ifdef _WINNT
   char   *paramdir;

   paramdir = getenv( "EW_PARAMS" ); 
   if ( paramdir == (char *) NULL ) return -1;
   sprintf( szFullTablePath, "%s\\%s", paramdir, pszStaFile );
#else
   sprintf( szFullTablePath, "%s", pszStaFile );                    /* For unix */
#endif

/* Open the station data file
   **************************/
   if ( ( hFile = fopen( szFullTablePath, "r") ) == NULL )
   {
      logit( "et", "Error opening station data file <%s>.\n", szFullTablePath );
      return -1;
   }

/* Count channels in the station meta-data file.
   Ignore comment lines and lines consisting of all whitespace.
   ************************************************************/
   iStaCnt = 0;
   while ( fgets( szString, 256, hFile ) != NULL )
   {
      if ( IsComment( szString ) ) continue;
      iNDecoded = sscanf( szString, 
          "%s %s %s %s %lf %lf %lf %lf %lf %lf %lf %d %d %s",
          StaTemp.szStation, StaTemp.szChannel, StaTemp.szNetID, 
          StaTemp.szLocation, &StaTemp.dSens, &StaTemp.dGainCalibration, 
          &StaTemp.dLat, &StaTemp.dLon, &StaTemp.dElevation,
          &StaTemp.dClipLevel, &StaTemp.dTimeCorrection,					
          &StaTemp.iStationType, &StaTemp.iAgency, 
          StaTemp.szStationName );
      if ( iNDecoded < 14 )
      {
         logit( "et", "Error decoding stn data file-%s.\n", szFullTablePath );
         logit( "e", "iNDecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szString );
         fclose( hFile );
         return -1;
      }
      iStaCnt++;
   }
   rewind( hFile );

/* Allocate the station array
   **************************/
   if ( iStaCnt > iMaxStn )   /* But first see if there are too many stns */
   {
      logit( "et", "Too many stations in Station file - %d\n", iStaCnt );
      fclose( hFile );
      return -1;
   }   
   sta = (STATION *) calloc( iStaCnt, sizeof( STATION ) );
   if ( sta == NULL )
   {
      logit( "et", "Cannot allocate the station array\n" );
      fclose( hFile );
      return -1;
   }

/* Initalize the char arrays. */
   for( i=0; i<iStaCnt; i++  )
   {
      for( j=0; j<TRACE_STA_LEN; j++ )  sta[i].szStation[j]  = '\0';
      for( j=0; j<TRACE_NET_LEN; j++ )  sta[i].szNetID[j]    = '\0';
      for( j=0; j<TRACE_CHAN_LEN; j++ ) sta[i].szChannel[j]  = '\0';
      for( j=0; j<TRACE_STA_LEN; j++ )  sta[i].szLocation[j] = '\0';
   }

/* Read stations from the station list file into the station array.
   ****************************************************************/
   i = 0;
   while ( fgets( szString, 256, hFile ) != NULL )
   {
      if ( IsComment( szString ) ) continue;
      iNDecoded = sscanf( szString, 
          "%s %s %s %s %lf %lf %lf %lf %lf %lf %lf %d %d %s",
          sta[i].szStation, sta[i].szChannel, sta[i].szNetID, 
          sta[i].szLocation, &sta[i].dSens, &sta[i].dGainCalibration, 
          &sta[i].dLat, &sta[i].dLon, &sta[i].dElevation, &sta[i].dClipLevel, 
          &sta[i].dTimeCorrection, &sta[i].iStationType, &sta[i].iAgency, 
          sta[i].szStationName );
      if ( iNDecoded < 14 )
      {
         logit( "et", "Error decoding stn data file2-%s.\n", szFullTablePath );
         logit( "e", "iNDecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szString );
         fclose( hFile );
         return -1;
      }
      sta[i].dTimeCorrection *= (-1.);
      InitVar( &sta[i] );	  
      i++;
      if ( i == iMaxStn )   /* See if there are too many stns */
      {
         logit( "", "Max number of stations (%d) reached in %s\n",
                i, szFullTablePath );
         break;
      }
   }
   fclose( hFile );

/* Add in poles/zeroes for all stations */
   LoadResponseDataAll( sta, pszStaResp, iStaCnt );

   *Sta      = sta;
   *piNumStn = iStaCnt;   
   return 1;
}

  /***************************************************************
   *                    ReadStationList()                        *
   *                                                             *
   * Read the list of stations used in this module and assign    *
   * station data to the list.                                   *
   *                                                             *
   * Jan., 2013: JC - Added call to read info from binary file.  *
   * May, 2008: Combined all station reads and structures in EB. *
   *                                                             *
   * Sta     Pointer to array of STATION structures with data    *
   * piNSta  Number of stations to process                       *
   * pszStaList  File with stations to process (...sta)          *
   * pszStaData  File with station data (station.dat)            *
   * pszStaResp  File with station calibration info (calibs)     *
   * iMaxStn     Maximum number of stations to allow             *
   * iPickerCall 1 if called from pick_wcatwc; 0 otherwise       *
   *                                                             *
   *  Returns -1 if an error is encountered; 1 if ok             *
   ***************************************************************/

int ReadStationList( STATION **Sta, int *piNSta, char *pszStaList,
                     char *pszStaData, char *pszStaResp, int iMaxStn,
                     int iPickerCall )
{
   double   dTemp;               /* Values in file, not needed here */
   FILE    *hFile;
   int      i;
   int      iCounter = 0;
   int      iNDecoded;           /* Number of fields read in .sta file */
   int      iStaCnt = 0;         /* Station counter */
   char     szFileName[256];     /* JMC 12.30.13 - Name of output binary file */
   char     szFullTablePath[256];
   char     szString[512];
   STATION *sta = NULL;
   STATION  StaTemp;

#ifdef _WINNT
   char    *paramdir;

   paramdir = getenv( "EW_PARAMS" ); 
   if ( paramdir == (char *) NULL ) 
   {
      logit( "et", "EW_PARAMS must be an Environment variable.\n" );
      return -1;
   }
   sprintf( szFullTablePath, "%s\\%s", paramdir, pszStaList );
#else
   sprintf( szFullTablePath, "%s", pszStaList );                  /* For unix */
#endif

/* JMC 12.30.13 - Use a saved copy of the station file - if possible */
   logit( "et", "%s(%d) Read station data - %s.\n", 
          __FILE__, __LINE__, szFullTablePath );
   if ( GetSavedStationData( Sta, piNSta, szFullTablePath ) == 0 )
   {
      logit( "et", "%s(%d) station data count from %s is %d.\n", 
             __FILE__, __LINE__, szFullTablePath, *piNSta );
      return 1;
   }

/* Open the station list file
   **************************/
   if ( ( hFile = fopen( szFullTablePath, "r") ) == NULL )
   {
      logit( "et", "Error opening station list file <%s>.\n", szFullTablePath );
      return -1;
   }

/* Count channels in the station file.
   Ignore comment lines and lines consisting of all whitespace.
   ************************************************************/
   iStaCnt = 0;
   while ( fgets( szString, 256, hFile ) != NULL )
   {
      if ( IsComment( szString ) ) continue;
      iNDecoded = sscanf( szString,
       "%s %s %s %s %d %d %d %d %lf %lf %lf %d %lf %lf\n",
       StaTemp.szStation, StaTemp.szChannel, StaTemp.szNetID, StaTemp.szLocation,
       &StaTemp.iPickStatus, &StaTemp.iFiltStatus, &StaTemp.iSignalToNoise,
       &StaTemp.iAlarmStatus, &StaTemp.dAlarmAmp, &StaTemp.dAlarmDur,
       &StaTemp.dAlarmMinFreq, &StaTemp.iComputeMwp, &dTemp, &dTemp );
      if ( iNDecoded < 14 )
      {
         logit( "et", "Error decoding station file - 1.\n" );
         logit( "e", "iNDecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szString );
         fclose( hFile );
         return -1;
      }
      if ( iPickerCall == 1 ) 
         { if ( StaTemp.iPickStatus == 1 ) iStaCnt++; }
      else                                 iStaCnt++;
   }
   rewind( hFile );

/* Allocate the station array
   **************************/
   if ( iStaCnt > iMaxStn )   /* But first see if there are too many stns */
   {
      logit( "et", "Too many stations in Station file - %d\n", iStaCnt );
      fclose( hFile );
      return -1;
   }   
   sta = (STATION *) calloc( iStaCnt, sizeof( STATION ) );
   if ( sta == NULL )
   {
      logit( "et", "Cannot allocate the station array\n" );
      fclose( hFile );
      return -1;
   }

/* Read stations from the station list file into the station array.
   ****************************************************************/
   i = 0;
   while ( fgets( szString, 256, hFile ) != NULL )
   {
      if ( IsComment( szString ) ) continue;
      iNDecoded = sscanf( szString,"%s %s %s %s %d %d %d %d %lf %lf %lf %d %lf %lf\n",
       sta[i].szStation, sta[i].szChannel, sta[i].szNetID, sta[i].szLocation,
       &sta[i].iPickStatus, &sta[i].iFiltStatus, &sta[i].iSignalToNoise,
       &sta[i].iAlarmStatus, &sta[i].dAlarmAmp, &sta[i].dAlarmDur,
       &sta[i].dAlarmMinFreq, &sta[i].iComputeMwp, &sta[i].dSampRate,
       &sta[i].dScaleFactor );
      if ( iNDecoded < 14 )
      {
         logit( "et", "Error decoding station file - 2.\n" );
         logit( "e", "ndecoded: %d\n", iNDecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", szString );
         fclose( hFile );
         return -1;
      }

/* Set Alarm Status and InitP here instead of calling program */
      sta[i].iAlarm = 0;
      if ( sta[i].iAlarmStatus > 0 )
      {
         sta[i].iAlarm = 1;	   
         sta[i].iAlarmStatus = 1;
      }
      InitP( &sta[i] );           /* Initialize P pick part of Station struct */

      if ( iPickerCall == 1 )
         if ( sta[i].iPickStatus == 0 ) continue;      
      
/* Read Station data file and match up with list
   *********************************************/      
      sta[i].dTimeCorrection *= (-1.);
      if ( LoadStationData( &sta[i], pszStaData ) == -1 )
      {
         logit( "et", "No match for scn in station info file.\n" );
         logit( "e", "file: %s\n", pszStaData );
         logit( "e", "scn = :%s:%s:%s:%s: \n", sta[i].szStation,
          sta[i].szChannel, sta[i].szNetID, sta[i].szLocation );
         iCounter++;
      }
      InitVar( &sta[i] );	  
      sta[i].dEndTime = 0.;
      sta[i].iFirst   = 1;
      i++;
      if ( i == iMaxStn )   /* See if there are too many stns */
      {
         logit( "", "Max number of stations (%d) reached in %s\n",
                i, szFullTablePath );
         break;
      }
   }
   fclose( hFile );

/* Add in poles/zeroes for all stations */
   LoadResponseDataAll( sta, pszStaResp, iStaCnt );

/* If iCounter > 0 then return -1; to get all stations not found. */
   if ( iCounter > 0 ) return( -1 );
   *Sta    = sta;
   *piNSta = iStaCnt;

/* JMC 12.30.13 - Save off the station array if we get this far */
   sprintf( szFileName, "%s.bin", szFullTablePath );
   if ( (hFile = fopen( szFileName, "wb" )) == NULL )
   {
      logit( "et", "%s(%d) Could not open %s for write.\n", 
             __FILE__, __LINE__, szFileName );
      return 1;
   }
   fwrite( sta, sizeof(STATION), iStaCnt, hFile );
   fclose( hFile );

   logit( "et", "%s(%d) Done with station data from %s.\n", 
          __FILE__, __LINE__, szFullTablePath );
   return 1;
}

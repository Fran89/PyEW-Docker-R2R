#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "latency_mon.h"

  /***************************************************************
   *                         GetStaList()                        *
   *                                                             *
   *                     Read the station list                   *
   *                                                             *
   *  Returns -1 if an error is encountered.                     *
   ***************************************************************/

int GetStaList( LATENCY_STATION **Sta, int *Nsta, GPARM *Gparm )
{
   char              string[180];
   int               station_count;
   LATENCY_STATION * stations;
/*   LATENCY_STATION   StaTemp;  */
   FILE            * fp = NULL;

   /* Open the station list file
   ****************************/
   if ( ( fp = fopen( Gparm->StaFile, "r") ) == NULL )
   {
      logit( "et", "latency_mon: Error opening station list file <%s>.\n",
             Gparm->StaFile );
      return -1;
   }

   /* Count station-channel-network lines in the station file.
   ** Ignore comment lines and lines consisting of all whitespace.
   **************************************************************/
   station_count = 0;
   while ( fgets( string, 160, fp ) != NULL )
   {
      if ( IsComment( string ) ) continue;

/*
      if ( sscanf( string
                 , "%s %s %s %*d %*d %*d %*d %*lf %*lf %*lf %*d %lf"
                 ,  StaTemp.szStation
                 ,  StaTemp.szChannel
                 ,  StaTemp.szNetID
                    , &StaTemp.iPickStatus
                    , &StaTemp.iFiltStatus
                    , &StaTemp.iSignalToNoise
                    , &StaTemp.iAlarmStatus
                    , &StaTemp.dAlarmAmp
                    , &StaTemp.dAlarmDur
                    , &StaTemp.dAlarmMinFreq
                    , &StaTemp.iComputeMwp
                 , &sta[i].dSampRate
                 ) < 4 )
      {
                       );
         logit( "et", "latency_mon: Error decoding station file. - 1\n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         fclose(fp);
         return -1;
      }
*/
      station_count++;
   }

   /* Allocate the station list
   ***************************/
   if ( MAX_STATIONS < station_count )
   {
      logit( "et", "Too many stations in Station file\n" );
      fclose(fp);
      return -1;
   }

   if ( (stations = (LATENCY_STATION *) calloc( station_count, sizeof(LATENCY_STATION) )) == NULL )
   {
      logit( "et", "latency_mon: Cannot allocate the station array\n" );
      fclose(fp);
      return -1;
   }

   /* Read stations from the station list file into the station
   ** array, including parameters used by the picking algorithm
   ************************************************************/
   rewind( fp );
   station_count = 0;
   while ( fgets( string, 160, fp ) != NULL )
   {
      if ( IsComment( string ) ) continue;

      if ( sscanf( string
                 , "%s %s %s %*d %*d %*d %*d %*lf %*lf %*lf %*d %lf"  /* %lf"  */
                 ,  stations[station_count].szStation
                 ,  stations[station_count].szChannel
                 ,  stations[station_count].szNetID
/*
                 , &stations[station_count].iPickStatus
                 , &stations[station_count].iFiltStatus
                 , &stations[station_count].iSignalToNoise
                 , &stations[station_count].iAlarmStatus
                 , &stations[station_count].dAlarmAmp
                 , &stations[station_count].dAlarmDur
                 , &stations[station_count].dAlarmMinFreq
                 , &stations[station_count].iComputeMwp
*/
                 , &stations[station_count].dSampRate
/*
                 , &stations[station_count].dScaleFactor
*/
                 ) < 4 )
      {
         logit( "et", "latency_mon: Error decoding station file.\n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         fclose(fp);
         free( stations );
         return -1;
      }

/*
      if ( LoadStationData( &stations[station_count], Gparm->StaDataFile ) == -1 )
      {
         logit( "et", "latency_mon: No match for scn in station info file.\n" );
         logit( "e", "file: %d\n", Gparm->StaDataFile );
         logit( "e"
              , "scn = %s %s %s\n"
              , stations[station_count].szStation
              , stations[station_count].szChannel
              , stations[station_count].szNetID
              );
         fclose(fp);
         return -1;
      }
      InitVar( &stations[station_count] );
*/

/*      stations[station_count].iFirst = 1;      */

      stations[station_count].LastPeriod.stt_time = 0.0;
      stations[station_count].LastPeriod.end_time = 0.0;
      stations[station_count].LastPeriod.latency  = 0;

      /* Create latency file name */
      strcpy( stations[station_count].StoreFileName, Gparm->LogPath );
      strcat( stations[station_count].StoreFileName, stations[station_count].szStation );
      strcat( stations[station_count].StoreFileName, stations[station_count].szChannel );
      strcat( stations[station_count].StoreFileName, ".LAT" );

      station_count++;
   }
   fclose( fp );
   *Sta  = stations;
   *Nsta = station_count;
   return 0;
}

  /***************************************************************
   *                       LoadStationData()                     *
   *                                                             *
   *       Get data on station from information file             *
   *                                                             *
   *  Returns -1 if an error is encountered or no match is found.*
   ***************************************************************/

int LoadStationData( LATENCY_STATION *Sta, char *pszInfoFile )
{
   char    string[180];
   FILE    *fp;
   char    szChannel[TRACE_CHAN_LEN]
      ,    szStation[TRACE_STA_LEN]
      ,    szNetID[TRACE_NET_LEN]
      ;

/* Open the station data file
   **************************/
   if ( ( fp = fopen( pszInfoFile, "r") ) == NULL )
   {
      logit( "et", "latency_mon: Error opening station data file <%s>.\n",
             pszInfoFile );
      return -1;
   }

/* Read station data from the station data file
   ********************************************/
   while ( fgets( string, 160, fp ) != NULL )
   {
      if ( sscanf( string
                 , "%s %s %s"
                 , szStation
                 , szChannel
                 , szNetID
                 ) < 3 )
      {
         logit( "et", "latency_mon: Error decoding station data file.\n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         fclose( fp );
         return -1;
      }

/* Compare SCNs
   ************/
	  if (   ! strcmp(szStation, Sta->szStation)
	      && ! strcmp(szChannel, Sta->szChannel)
	      && ! strcmp(szNetID  , Sta->szNetID  )
       )
    {     /* We have a match */

         if ( sscanf( string
                    , "%s %s %s" /* %lf %lf %lf %lf %lf %lf %lf %d" */
                    , Sta->szStation
                    , Sta->szChannel
                    , Sta->szNetID
/*
                    , &Sta->dSens
                    , &Sta->dGainCalibration
                    , &Sta->dLat
                    , &Sta->dLon
                    , &Sta->dElevation
                    , &Sta->dClipLevel
                    , &Sta->dTimeCorrection
                    , &Sta->iStationType
*/
                    ) < 3 )
         {
            logit( "et", "latency_mon: Error decoding station data file-2.\n" );
            logit( "e", "Offending line:\n" );
            logit( "e", "%s\n", string );
            fclose( fp );
            return -1;
         }

         fclose( fp );
         return 0;
      }
   } /* while( each-line-from-file ) */

   fclose( fp );

   /* didn't find station in file */
   logit( "et"
        , "latency_mon.LoadStationData(): missing scn from station file: %s : %s : %s\n"
        , Sta->szStation
        , Sta->szChannel
        , Sta->szNetID
        );
   return -1;
}

 /***********************************************************************
  *                             LogStaList()                            *
  *                                                                     *
  *                         Log the station list                        *
  ***********************************************************************/

void LogStaList( LATENCY_STATION *Sta, int Nsta )
{
   int i;

   logit( "", "\nStation List:\n" );
   for ( i = 0; i < Nsta; i++ )
   {
      logit( ""
           , "%4s %3s %2s\n"
           , Sta[i].szStation
           , Sta[i].szChannel
           , Sta[i].szNetID
           );
/*
      logit( "", "%4s",     Sta[i].szStation );
      logit( "", " %3s",    Sta[i].szChannel );
      logit( "", " %2s",    Sta[i].szNetID );
      logit( "", " %1d",    Sta[i].iComputeMwp );
      logit( "", " %lf",    Sta[i].dScaleFactor );
      logit( "", "\n" );
*/
   }
   logit( "", "\n" );
}

    /*************************************************************************
     *                             IsComment()                               *
     *                                                                       *
     *  Accepts: String containing one line from a latency_mon station list  *
     *  Returns: 1 if it's a comment line                                    *
     *           0 if it's not a comment line                                *
     *************************************************************************/

int IsComment( char string[] )
{
   int i;

   for ( i = 0; i < (int)strlen( string ); i++ )
   {
      char test = string[i];

      switch( test )
      {
        case ' ':
        case '\t':
        case '\n':
             break;

        case '#':
             return 1;  /* It's a comment line */

        default:
             return 0;  /* It's not a comment line */
      }
   }
   return 1;                   /* It contains only whitespace */
}


/* --------------------------------- alex ----------------------------------------------*/
  /***************************************************************
   *                    GetFindWaveStaList()                     *
   *                                                             *
   *            Read the FindWave style station list             *
   *                                                             *
   *  Returns -1 if an error is encountered.                     *
   ***************************************************************/

int GetFindWaveStaList( LATENCY_STATION **Sta, int *Nsta, GPARM *Gparm )
{
   char              string[180];
   int               station_count;
   LATENCY_STATION * stations;
   LATENCY_STATION   StaTemp;
   FILE            * fp;

/* Open the station list file
   **************************/
   if ( ( fp = fopen( Gparm->FindWaveFile, "r") ) == NULL )
   {
      logit( "et", "latency_mon: Error opening station list file <%s>.\n",
             Gparm->FindWaveFile );
      return -1;
   }

   /* Count channels in the station file.
   *************************************/
   station_count = 0;

   /*step over two title lines
   ***************************/
   fgets( string, 160,fp );
   fgets( string, 160,fp );

   while ( fgets( string, 160, fp ) != NULL )
   {
      if ( sscanf( string
                 , "%s %s %s"
                 , StaTemp.szStation
                 , StaTemp.szChannel
                 , StaTemp.szNetID
                 ) < 3 )
      {
         logit( "et", "latency_mon: Error decoding station file. - 1\n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         fclose(fp);
         return -1;
      }
      station_count++;
   }

/* Allocate the station list
   *************************/
   if ( MAX_STATIONS < station_count )
   {
      logit( "et", "Too many stations in Station file\n" );
      fclose(fp);
      return -1;
   }

   if ( (stations = (LATENCY_STATION *) calloc( station_count, sizeof(LATENCY_STATION) )) == NULL )
   {
      logit( "et", "latency_mon: Cannot allocate the station array\n" );
      fclose(fp);
      return -1;
   }

   /* Read stations from the station list file into the station array
   *****************************************************************/

   rewind( fp );

   /*step over two title lines
   ***************************/
   fgets( string, 160, fp );
   fgets( string, 160, fp );

   station_count = 0;

   while ( fgets( string, 160, fp ) != NULL )
   {
       if ( sscanf( string
                  , "%s %s %s %*d %*d %lf\n"
                  , stations[station_count].szStation
                  , stations[station_count].szChannel
                  , stations[station_count].szNetID
/*
                  , &stations[i].iPickStatus
                  , &stations[i].iFiltStatus
*/
                  , &stations[station_count].dSampRate
                  ) < 4 )
      {
         logit( "et", "latency_mon: Error decoding station file.\n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         free( stations );
         fclose(fp);
         return -1;
      }

/*      InitVar( &stations[i] );    */

/*      sta[i].iFirst = 1;          */
      stations[station_count].LastPeriod.stt_time = 0.0;
      stations[station_count].LastPeriod.end_time = 0.0;
      stations[station_count].LastPeriod.latency  = 0;

      /* Create latency file name */
      strcpy( stations[station_count].StoreFileName, Gparm->LogPath );
      strcat( stations[station_count].StoreFileName, stations[station_count].szStation );
      strcat( stations[station_count].StoreFileName, stations[station_count].szChannel );
      strcat( stations[station_count].StoreFileName, ".LAT" );

      station_count++;
   }
   fclose( fp );
   *Sta  = stations;
   *Nsta = station_count;
   return 0;
}

      /*****************************************************************
       *                          loc_wcatwc.c                         *    
       *                                                               *     
       *  This program locates earthquakes given P-picks placed by     *
       *  pick_wcatwc in the InRing.  Some simple logic is used to     *
       *  discriminate P-picks of different quakes.  The P-time        *
       *  between stations is compared to the distance between the two.*
       *  If the time is larger than can be possible given the P travel*     
       *  time table values, the latest pick is moved into the next    *
       *  open P buffer.  As more P-picks come in, they are put in a   *     
       *  buffer based on their P-time.  If MaxTimeBetweenPicks expires*
       *  between P-picks, the next P's are put into a new buffer.     *
       *  After a buffer has enough P-picks to locate a quake (MinPs), *
       *  the solution is computed.  If a good solution is made, P's   *
       *  from other buffers are compared to this solution and added   *
       *  back into the buffer if they fit (only P's that were thrown  *
       *  out due to the MaxTimeBetweenPicks critieria, not those      *
       *  eliminated due to excessively large P-time differences).     *
       *                                                               *
       *  Quake locations are computed using Geiger's method given an  *    
       *  initial location computed by a technique developed at the    *
       *  National Tsunami Warning Center.  An initial guess           *
       *  is first assigned to the location of the first P-time in the *
       *  buffer.  If a solution can not be computed from this initial *
       *  location, a routine is called to compute the initial location*
       *  from azimuth and distance determined from a quadrapartite of *
       *  stations.  If a location can still not be determined, a bad  *
       *  P-pick discriminator is called.  This simply throws out      *
       *  stations one-at-a-time (up to three stations at once) and    *
       *  re-computes the location.  Good solutions are verified by    *
       *  total residual.  The quake locator was first introduced in   *
       *  tsunami warning centers by Sokolowski in the 1970's.  I      *
       *  I think it was originally developed at NEIC before that.     *
       *                                                               *
       *  The IASPEI91 travel times are used as the basis for quake    *
       *  locations in this program.  A time/distance/depth table has  *
       *  been created from software provided by the National          *
       *  Earthquake Information Center.  Locations with this set of P *
       *  times have been been compared to those made with the         *
       *  Jefferey's-Bullen set of times and were found to be superior *
       *  in regards to depth discrimination and epicentral location   *
       *  with poor azimuthal control.  The P-table is arranged on 10km*
       *  depth increments and 0.5 degree distance increments.         *
       *                                                               *
       *  After a good location has been computed, magnitude is output *
       *  based on the amplitude/periods reported by the P-picker.  Mb,*
       *  Ml, MS, and Mwp magnitudes are computed depending on         *
       *  epicentral distance.                                         *
       *                                                               *
       *  The locations/magnitudes are sent to the OutRing.  Alarms    *
       *  based on location and magnitude can also be issued to the    *
       *  AlarmRing if desired.  The page_alarm module will send these *
       *  to a pager.                                                  *
       *                                                               *
       *  Sep., 2013: Modified FindDepth for improved speed            *
       *  Oct., 2010: Combined PPICK and STATION structures            *
       *  June, 2007: Paul Huang's new associator logic was added.     *
       *              This provides a better way to sort p-picks into  *
       *              buffers.                                         *
       *  March, 2006: Nyland's FindDepth function added so that       *
       *               average depth in region is used as fixed depth  *
       *               and depth can't float beyond max + 50km         *
       *  Sept., 2004: Respond to special PPick from hypo_display      *
       *               which causes an immediate relocate here.        *
       *  Sept., 2004: Completely remove picks from all buffers if they*
       *               appear to be Bad picks.                         *
       *  July, 2004: Update Mwps and screen data every x seconds      *
       *              automatically after a new location is made.      *
       *                                                               *
       *  2001: Paul Whitmore, NOAA-NTWC - paul.whitmore@noaa.gov      *
       *                                                               *
       ****************************************************************/
	   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include "loc_wcatwc.h"

/* Global Variables (those needed in threads)
   ******************************************/
CITY   city[NUM_CITIES];       /* Array of reference city locations */
CITY   cityEC[NUM_CITIES_EC];  /* Array of eastern reference city locations */
GPARM  Gparm;                  /* Configuration file parameters */
HYPO   Hypo[MAX_PBUFFS];       /* Hypocenter information for each P buffer */
int    iActiveBuffer;          /* Last buffer in which hypocenter computed */
int    iDepthAvg[180][360]; /* Array of average depth (km) for all lat/lons */
int    iDepthMax[180][360]; /* Array of maximum depths (km) for all lat/lons */
int    iLastBuffCnt[MAX_PBUFFS];/* Previous number of Ps/buffer */
int    iNumPBufRem[MAX_PBUFFS];/* # of stns (permanently) removed from buffer */
int    iPBufCnt[MAX_PBUFFS];   /* Buffer P-pick Counter */
int    iPBufForceLoc[MAX_PBUFFS]; /* Flag to force a re-location in this buff */
int    Nsta;                   /* Number of stations in data file */
EWH    Ewh;                    /* Parameters from earthworm.h */
mutex_t mutsem1;               /* Semaphore to protect iPBufCnt adjustments */
STATION *StaArray[MAX_PBUFFS]; /* Station array (from .sta file or disk file) */
char   szPStnArray[MAX_STATIONS][MAX_NUM_NEAR_STN][TRACE_STA_LEN];/* Near stn lookup table */
char   szStnRem[MAX_PBUFFS][MAX_STN_REM][TRACE_STA_LEN];/* Removed stns */

      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of configuration file                *
       ***********************************************************/

int main( int argc, char **argv )
{
   int           i, j;            /* Loop counters */
   int           iRC;
   char          PIn[MAX_PICKTWC_SIZE];/* Pointer to P-pick from ring */
   int           lineLen;         /* Length of heartbeat message */
   char          line[40];        /* Heartbeat message */
   long          MsgLen;          /* Size of retrieved message */
   MSG_LOGO      getlogo;         /* Logo of requested picks */
   MSG_LOGO      logo;            /* Logo of retrieved msg */
   MSG_LOGO      hrtlogo;         /* Logo of outgoing heartbeats */
   time_t        then;            /* Previous heartbeat time */
   char          *configfile;     /* Pointer to name of config file */
   pid_t         myPid;           /* Process id of this process */
   static unsigned tidLocate;     /* Quake Location Thread */
   char          *paramdir;
   char          FullTablePath[512];
   STATION       StaTemp;         /* Temp P-pick data structure */

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: loc_wcatwc <configfile>\n" );
      return -1;
   }
   configfile = argv[1];
   
/* Environmental Parameter to Read Stations 
   ****************************************/
#ifdef _WINNT
   paramdir = getenv( "EW_PARAMS" ); 
   if ( paramdir == (char *) NULL ) return -1;
   sprintf( FullTablePath, "%s\\%s", paramdir, configfile );
#else
   sprintf( FullTablePath, "%s", configfile );                    /* For unix */
#endif

/* Get parameters from the configuration files
   *******************************************/
   if ( GetConfig( FullTablePath, &Gparm ) == -1 )
   {
      fprintf( stderr, "loc_wcatwc: GetConfig() failed. Exiting.\n" );
      return -1;
   }

/* Look up info in the earthworm.h tables
   **************************************/
   if ( GetEwh( &Ewh ) < 0 )
   {
      fprintf( stderr, "loc_wcatwc: GetEwh() failed. Exiting.\n" );
      return -1;
   }

/* Specify logos of incoming P-picks and outgoing heartbeats
   *********************************************************/
   getlogo.instid = Ewh.GetThisInstId;
   getlogo.mod    = Ewh.GetThisModId;
   getlogo.type   = Ewh.TypePickTWC;
   hrtlogo.instid = Ewh.MyInstId;
   hrtlogo.mod    = Gparm.MyModId;
   hrtlogo.type   = Ewh.TypeHeartBeat;

/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, Gparm.MyModId, 512, 1 );

/* Check number of P buffers
   *************************/
   if ( Gparm.NumPBuffs > MAX_PBUFFS )
   {
      logit( "e", "loc_wcatwc: Too many P buffers: %ld, max=%ld\n",
             Gparm.NumPBuffs, MAX_PBUFFS );
      return -1;
   }

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "loc_wcatwc: Can't get my pid. Exiting.\n" );
      return -1;
   }

/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm );

/* Load reference city coordinates used in littoral locations
  ***********************************************************/
   if ( LoadCities( city, 1, Gparm.CityFileWUC, Gparm.CityFileWLC ) < 0 )
   {
      logit( "t", "LoadCities failed, exiting\n" );
      return -1;
   }

/* Load eastern reference city coordinates used in littoral locations
  *******************************************************************/
   if ( LoadCitiesEC( cityEC, 1, Gparm.CityFileEUC, Gparm.CityFileELC ) < 0 )
   {
      logit( "t", "LoadCitiesEC failed, exiting\n" );
      return -1;
   }
	
/* Load Richter B-values for use in Mb magnitudes 
   **********************************************/
   if ( LoadBVals( Gparm.szBValFile ) < 0 )
   {
      logit( "t", "LoadBVals failed, exiting\n" );
      return -1;
   }

/* Load Avg and max depth data
   ***************************/
   if ( LoadEQDataNew( Gparm.szDepthDataFile, iDepthAvg, iDepthMax ) == -1 )
   {
      logit( "t", "LoadEQDataNew failed\n" );
      return -1;
   }

/* Read the station list and return the number of stations found.
   Allocate the station list array NumPBuffs times.
   **************************************************************/
   if ( ReadStationList( &StaArray[0], &Nsta, Gparm.StaFile,
         Gparm.StaDataFile, Gparm.ResponseFile, MAX_STATIONS, 0 ) == -1 )
   {
      logit( "", "loc_wcatwc: ReadStationList() failed. Exiting.\n" );
      return -1;
   }
   if ( Nsta == 0 )
   {
      logit( "et", "loc_wcatwc: Empty station list. Exiting." );
      return -1;
   }
   logit( "t", "loc_wcatwc: Displaying %d stations.\n", Nsta );

/* Dupicate the station list NumPBuffs times.
   ******************************************/
   for ( i=1; i<Gparm.NumPBuffs; i++ )
   {
      StaArray[i] = (STATION *) calloc( Nsta, sizeof( STATION ) );
      if ( StaArray[i] == NULL )
      {
         logit( "et", "Cannot allocate the station array; i=%ld\n", i );
         for ( j=0; j<i-1; j++ ) free( StaArray[j] );
         return -1;
      }
      memcpy( StaArray[i], StaArray[0], Nsta*sizeof( STATION ) );
   }      

/* Initialize P buffers
   ********************/   
   for ( i=0; i<Gparm.NumPBuffs; i++ )
   {
      for ( j=0; j<Nsta; j++ ) InitP( &StaArray[i][j] );
      iPBufCnt[i]    = 0;
      iNumPBufRem[i] = 0;
      InitHypo( &Hypo[i] );
      itoaX( i+1, Hypo[i].szQuakeID );
      PadZeroes( 6, Hypo[i].szQuakeID );
      Hypo[i].iVersion     = 1;
      Hypo[i].iAlarmIssued = 0;
   }
   iActiveBuffer = 0;

/* Create a table which lists the nearest NumNearStn stations which
   are sending P-picks to the locator.
   ****************************************************************/
   time( &then );
   logit( "t", "Create NBS Lookup table\n" );
   CreateNearbyStationLookupTable( StaArray[0], szPStnArray, Nsta,
                                   Gparm.iNumNearStn );
   logit( "t", "Done Create NBS Lookup table\n" );
   
/* Create a mutex for protecting adjustments of iPBufCnt
   *****************************************************/
   CreateSpecificMutex( &mutsem1 );

/* Attach to existing transport rings
   **********************************/
   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_attach( &Gparm.InRegion,  Gparm.InKey );
      tport_attach( &Gparm.OutRegion, Gparm.OutKey );
      tport_attach( &Gparm.AlarmRegion, Gparm.AlarmKey );
   }
   else
   {
      tport_attach( &Gparm.InRegion, Gparm.InKey );
      Gparm.OutRegion = Gparm.InRegion;
   }

/* Flush the input ring
   ********************/
   while ( tport_getmsg( &Gparm.InRegion, &getlogo, 1, &logo, &MsgLen,
                         PIn, MAX_PICKTWC_SIZE) != GET_NONE );

/* Send 1st heartbeat to the transport ring
   ****************************************/
   sprintf( line, "%ld %d\n", (long) then, myPid );
   lineLen = strlen( line );
   if ( tport_putmsg( &Gparm.OutRegion, &hrtlogo, lineLen, line ) != PUT_OK )
   {
      logit( "et", "loc_wcatwc: Error sending 1st heartbeat. Exiting." );
      if ( Gparm.OutKey != Gparm.InKey )
      {
         tport_detach( &Gparm.InRegion );
         tport_detach( &Gparm.OutRegion );
         tport_detach( &Gparm.AlarmRegion );
      }
      else
         tport_detach( &Gparm.InRegion );
      for ( i=0; i<Gparm.NumPBuffs; i++ ) free( StaArray[i] );
      return 0;
   }

/* Start the quake location thread
   *******************************/
   if ( StartThread( LocateThread, 8192, &tidLocate ) == -1 )
   {
      for ( i=0; i<Gparm.NumPBuffs; i++ ) free( StaArray[i] );
      if ( Gparm.OutKey != Gparm.InKey )
      {
         tport_detach( &Gparm.InRegion );
         tport_detach( &Gparm.OutRegion );
         tport_detach( &Gparm.AlarmRegion );
      }
      else
         tport_detach( &Gparm.InRegion );
      logit( "et", "Error starting Locate thread; exiting!\n" );
      return -1;
   }

/* Initialize the temp buffer 
   ************************** */
   StaTemp.dLat = 0.;
   StaTemp.dLon = 0.;
   InitP( &StaTemp );

/* Loop to read picker messages and invoke the locator
   ***************************************************/
   while ( tport_getflag( &Gparm.InRegion ) != TERMINATE )
   {
      int     rc;               /* Return code from tport_getmsg() */
      time_t  now;              /* Current time */
      

      time( &now );
      
#ifdef _WINNT
/* If we are in the ATPlayer version of loc_, see if we should re-init
   *******************************************************************/
      if ( (now - then) > 1800 )                                   /* Big gap */
      {        
         sleep_ew( 5000 );                  /* Let ATPlayer fill up the files */
         logit( "t", "Large gap noted in locator\n" );
         if ( strlen( Gparm.ATPLineupFileBB ) >  2 && 
                      Gparm.iRedoLineupFile   == 1 )           /* Then we are */
         {
            logit( "", "reset StaArray Nsta = %d\n", Nsta );	 
            for ( i=0; i<Gparm.NumPBuffs; i++ ) free( StaArray[i] );
            Nsta = ReadLineupFile( Gparm.ATPLineupFileBB, &StaArray[0], 
                                   MAX_STATIONS );
            logit( "", "New StaArray Nsta = %d\n", Nsta );	 
            for ( j=0; j<Nsta; j++ ) InitP( &StaArray[0][j] );
            iPBufCnt[0]    = 0;
            iNumPBufRem[0] = 0;
/* Duplicate the station list NumPBuffs times.
   *******************************************/
            for ( i=1; i<Gparm.NumPBuffs; i++ )
            {
               StaArray[i] = (STATION *) calloc( Nsta, sizeof( STATION ) );
               if ( StaArray[i] == NULL )
               {
                  logit( "et", "Cannot allocate the station array; i=%ld\n", i );
                  for ( j=0; j<i-1; j++ ) free( StaArray[j] );
                  continue;
               }
               memcpy( StaArray[i], StaArray[0], Nsta*sizeof( STATION ) );
               iPBufCnt[i]    = 0;
               iNumPBufRem[i] = 0;
            }                          
/* Create a table which lists the nearest NumNearStn stations which
   are sending P-picks to the locator based stations in data file.
   ****************************************************************/
            CreateNearbyStationLookupTable( StaArray[0], szPStnArray, Nsta,
                                            Gparm.iNumNearStn );
         } 	 
      }
#endif

/* Send a heartbeat to the transport ring
   **************************************/
      if ( (now - then) >= Gparm.HeartbeatInt )
      {
         then = now;
         sprintf( line, "%ld %d\n", (long) now, myPid );
         lineLen = strlen( line );
         if ( tport_putmsg( &Gparm.OutRegion, &hrtlogo, lineLen, line ) !=
              PUT_OK )
         {
            logit( "et", "loc_wcatwc: Error sending heartbeat." );
            break;
         }
      }

/* Get a p-pick from transport region
   **********************************/
      rc = tport_getmsg( &Gparm.InRegion, &getlogo, 1, &logo, &MsgLen,
                         PIn, MAX_PICKTWC_SIZE );

      if ( rc == GET_NONE )
      {
         sleep_ew( 200 );
         continue;
      }
      if ( rc == GET_NOTRACK )
         logit( "et", "loc_wcatwc: Tracking error.\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "loc_wcatwc: Got lapped on the ring.\n");

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "loc_wcatwc: Gap in sequence numbers.\n");

      if ( rc == GET_MISS )
         logit( "et", "loc_wcatwc: Missed messages.\n");

      if ( rc == GET_TOOBIG )
      {
         logit( "et", "loc_wcatwc: Retrieved message too big (%d) for msg.\n",
                MsgLen );
         continue;
      }

/* Put P-pick into structure (NOTE: No byte-swapping is performed, so 
   there will be trouble getting picks from an opposite order machine)
   *******************************************************************/
      if ( PPickStruct( PIn, &StaTemp, Ewh.TypePickTWC ) < 0 ) continue;
	  
/* See if this "PPick" is actually a trigger to force a location in a 
   specified buffer.  If so, re-locate that buffer.
   ******************************************************************/
      if ( atoi( StaTemp.szHypoID ) > 0 && 
           !strcmp( StaTemp.szStation, "LOC" ) &&
           !strcmp( StaTemp.szChannel, "ATE" ) )                /* Locate now */
      {
         logit( "et", "Force location sent from hypo_display.\n" );
         RequestSpecificMutex( &mutsem1 ); /* Semaphore protect buffer writes */
         for ( i=0; i<Gparm.NumPBuffs; i++ )
            if ( !strcmp( Hypo[i].szQuakeID, StaTemp.szHypoID ) )
            {        
               logit( "", "Relocate ID %s\n", StaTemp.szHypoID );
               iRC = LocateQuake( Nsta, StaArray[i], &iPBufCnt[i], &Gparm,
                &Hypo[i], i, &Ewh, city, 1, Hypo, iPBufCnt, cityEC,
                iDepthAvg, iDepthMax, Gparm.szIndexFile, Gparm.szLatFile, 
                Gparm.szNameFile, Gparm.szPathDistances, Gparm.szPathDirections,
                Gparm.szPathCities, Gparm.szPathRegions );						
               if ( iRC == -1 )
                  logit( "et", "Problem in LocateQuake-2\n" );
/* Set active buffer to the one which has had the latest good location (or the
   buffer which has the same quake with more picks) */			   
               if ( iRC >= 0 && Hypo[i].iGoodSoln >= 2 ) iActiveBuffer = iRC;
               break;
            }
         ReleaseSpecificMutex( &mutsem1 );   /* Let someone else have sem */
         continue;
      }

/* Load P-pick into proper buffer    
   ******************************/
      RequestSpecificMutex( &mutsem1 );   /* Semaphore protect buffer writes */
      LoadUpPBuff( &StaTemp, StaArray, iPBufCnt, Hypo, &iActiveBuffer, &Gparm,
       &Ewh, city, iLastBuffCnt, iNumPBufRem, cityEC, iDepthAvg, iDepthMax,
       szPStnArray, Nsta, Gparm.iNumNearStn, Gparm.dMaxDist, iPBufForceLoc, 
       Gparm.szIndexFile, Gparm.szLatFile, Gparm.szNameFile, 
       Gparm.szPathDistances, Gparm.szPathDirections, Gparm.szPathCities, 
       Gparm.szPathRegions );
      ReleaseSpecificMutex( &mutsem1 );   /* Let someone else have sem */
   }

/* Detach from the ring buffers
   ****************************/
   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.OutRegion );
      tport_detach( &Gparm.AlarmRegion );
   }
   else
      tport_detach( &Gparm.InRegion );
   for ( i=0; i<Gparm.NumPBuffs; i++ ) free( StaArray[i] );
   logit( "t", "Termination requested. Exiting.\n" );
   return 0;
}

  /***************************************************************
   *        CreateNearbyStationLookupTable()                     *
   *                                                             *
   * This function creates the nearby station lookup table used  *
   * in the associator.                                          *
   *                                                             *
   *  Arguments:                                                 *
   *     Sta              Pointer to station data array          *
   *     pszPStnArray     Array with nearest station lookup table*
   *     iNsta            Number of stations in Sta array        *
   *     iNumNearStn      Number of near stations to compare     *
   *                                                             *
   *  Returns -1 if an error is encountered; 1 otherwise         *
   ***************************************************************/

int CreateNearbyStationLookupTable( STATION *Sta,
     char pszPStnArray[][MAX_NUM_NEAR_STN][TRACE_STA_LEN], int iNSta,
     int iNumNearStn  )
{
   AZIDELT azidelt;
   int     i, j, jj;
   LATLON  ll[MAX_STATIONS], ll2;
   char    szSta[MAX_STATIONS][8], szChn[MAX_STATIONS][8],
           szNet[MAX_STATIONS][8], szLoc[MAX_STATIONS][8];
   
   for ( i=0; i<iNSta; i++ )
   {
      strcpy( szSta[i], Sta[i].szStation );
      strcpy( szChn[i], Sta[i].szChannel );
      strcpy( szNet[i], Sta[i].szNetID );
      strcpy( szLoc[i], Sta[i].szLocation );
      ll[i].dLat = Sta[i].dLat;       /* Sta array in geographic */
      ll[i].dLon = Sta[i].dLon;
      GeoCent( &ll[i] );              /* Convert to geocentric */
      GetLatLonTrig( &ll[i] );   
   }
   
   for ( i=0; i<iNSta; i++ )
   {
      for ( j=0; j<iNSta; j++ )
      {    
         if ( szChn[j][2] == 'Z' )  /* Only use vertical in near stn table */
            GetDistanceAz( &ll[i], &ll[j], &azidelt );
         else
            azidelt.dDelta = 179.;	    
         if ( azidelt.dDelta < 0.0000001 && 
              strcmp( szSta[i], szSta[j] ) )
            logit( "", "Warning: %s and %s are 0. apart\n", 
             szSta[i], szSta[j] );
         Sta[j].dDelta = azidelt.dDelta;
         strcpy( Sta[j].szStation, szSta[j] );
         strcpy( Sta[j].szChannel, szChn[j] );
         strcpy( Sta[j].szNetID, szNet[j] );
         strcpy( Sta[j].szLocation, szLoc[j] );
         Sta[j].dLat = ll[j].dLat;
         Sta[j].dLon = ll[j].dLon;
         Sta[j].dCoslat = ll[j].dCoslat;
         Sta[j].dSinlat = ll[j].dSinlat;
         Sta[j].dCoslon = ll[j].dCoslon;
         Sta[j].dSinlon = ll[j].dSinlon;
         GeoGraphic( &ll2, &ll[j] );
         Sta[j].dLat = ll2.dLat;
         Sta[j].dLon = ll2.dLon;
         if (Sta[j].dLon > 180. ) Sta[j].dLon -= 360.;
      }	    	  
/* Sort by distance (nearest first) */	  
      qsort( Sta, iNSta, sizeof (STATION), struct_cmp_by_EpicentralDistance );
      j  = 0;
      jj = 0;
      while ( jj < iNumNearStn ) 
      {
         if ( (jj == 0) ||
              (jj > 0   && strcmp( Sta[j].szStation, pszPStnArray[i][jj-1] )) )
         {
            strcpy( pszPStnArray[i][jj], Sta[j].szStation );
            jj++;
         }
         j++;
      }
   }
   for ( i=0; i<iNSta; i++ )
   {
      logit( "", "%s - ", szSta[i] );
      for ( j=0; j<iNumNearStn; j++ )
         logit( "", "%s ", pszPStnArray[i][j] );
      logit( "", "\n" );
      if ( strcmp( szSta[i], pszPStnArray[i][0] ) )
         logit( "", "Warning; %s not nearest stn to self\n", szSta[i] );
   }
/* Resort by original order */
   qsort( Sta, iNSta, sizeof (STATION), struct_cmp_by_StationSortIndex );
   return ( 1 );
}

      /*******************************************************
       *                      GetEwh()                       *
       *                                                     *
       *      Get parameters from the earthworm.d file.      *
       *******************************************************/

int GetEwh( EWH *Ewh )
{
   if ( GetLocalInst( &Ewh->MyInstId ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting MyInstId.\n" );
      return -1;
   }

   if ( GetInst( "INST_WILDCARD", &Ewh->GetThisInstId ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting GetThisInstId.\n" );
      return -2;
   }                                              
   if ( GetModId( "MOD_WILDCARD", &Ewh->GetThisModId ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting GetThisModId.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_ALARM", &Ewh->TypeAlarm ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting TypeAlarm.\n" );
      return -6;
   }
   if ( GetType( "TYPE_PICKTWC", &Ewh->TypePickTWC ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting TypePickTWC.\n" );
      return -7;
   }
   if ( GetType( "TYPE_H71SUM2K", &Ewh->TypeH71Sum2K ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting TYPE_H71SUM2K.\n" );
      return -8;
   }
   if ( GetType( "TYPE_HYPOTWC", &Ewh->TypeHypoTWC ) != 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error getting TypeHypoTWC.\n" );
      return -9;
   }
   return 0;
}

      /*********************************************************
       *                   LocateThread()                      *
       *                                                       *
       *  This thread checks each P buffer to see if enough    *
       *  picks have been entered to make a location.  Each    *
       *  new pick added to a buffer triggers another location.*
       *  Location/magnitude are checked with alarm criteria.  *
       *  After location, the thread checks whether any Ps from*
       *  other buffers should be added to this buffer.        *
       *  At the end of each loop, the thread checks time of   *
       *  Ps in each buffer to see if that buffer should be    *
       *  zeroed.                                              *
       *                                                       *
       *  September, 2002: Changed when new locations are made;*
       *   new locations now made whenever # Ps in buffer      *
       *   change and there are more than MinP picks.          *
       *                                                       *
       *********************************************************/
	   
thr_ret LocateThread( void *dummy )
{
   double  dMin;                             /* Oldest P-time in buffers */
   int     i, j, iTemp;
   int     iMin;                             /* Index of oldest P buffer */
   int     iRC;                              /* Return from Locate */ 
   time_t  lTime;                            /* Present 1/1/70 time */

   for ( i=0; i<Gparm.NumPBuffs; i++ ) iLastBuffCnt[i] = 0;

/* Loop every X seconds to check P buffer status */
   for (;;)
   {   
/* Check each buffer to see if there are enough Ps to locate and/or there is a
   new or changed P */   
      time( &lTime );
      for ( i=0; i<Gparm.NumPBuffs; i++ )
      {
         if ( iPBufCnt[i] >= Gparm.MinPs && iPBufCnt[i] != iLastBuffCnt[i] ||
              iPBufForceLoc[i] == 1 )
         {		 	   
/* Semaphore protect buffer writes since this thread and others adjust
   buffers */			   
            if ( iPBufForceLoc[i] == 1 ) logit( "t", "Force reloc\n" );
            RequestSpecificMutex( &mutsem1 );
            iPBufForceLoc[i] = 0;
            iLastBuffCnt[i]  = iPBufCnt[i];   
/* Try to locate a quake with this buffer's picks */			
            iRC = LocateQuake( Nsta, StaArray[i], &iPBufCnt[i], &Gparm,
                   &Hypo[i], i, &Ewh, city, 1, Hypo, iPBufCnt, cityEC,
                   iDepthAvg, iDepthMax, Gparm.szIndexFile, Gparm.szLatFile, 
                   Gparm.szNameFile, Gparm.szPathDistances, Gparm.szPathDirections,
                   Gparm.szPathCities, Gparm.szPathRegions );			
            if ( iRC == -1 )
               logit( "et", "Problem in LocateQuake\n" );
            else
/* Check other P buffers to see if any of their Ps should go with this quake */
               CheckPBuffTimes( StaArray, iPBufCnt, Hypo, i, &Gparm,
                                iLastBuffCnt, iNumPBufRem, szPStnArray,
                                Nsta, Gparm.iNumNearStn, Nsta );
                  
/* Set active buffer to the one which has had the latest good location (or the
   buffer which has the same quake with more picks) */			   
            if ( iRC >= 0 && Hypo[i].iGoodSoln >= 2 ) iActiveBuffer = iRC;			   			
            ReleaseSpecificMutex( &mutsem1 );   /* Let someone else have sem */
         }
      }
		 
/* Reset oldest P Buffer to zero when all are filled */
      for ( i=0; i<Gparm.NumPBuffs; i++ )
         if ( iPBufCnt[i] == 0 ) goto Sleeper;   /* Then, at least 1 is empty */
		 
/* If we get here, all P buffers must have some Ps in them, so one buffer must
   be cleared out. Find the one with the oldest P's and re-init that one. */
      iMin = 0;
      dMin = 1.E20;
      
      for ( i=0; i<Gparm.NumPBuffs; i++ )
/* Save this if MS could still be updating, or P's are still being added */
         if ( Hypo[i].dMSAvg == 0. ||
            ((double) lTime-Hypo[i].dOriginTime) > 5400. )
            if ( ((double) lTime-Hypo[i].dOriginTime) > 1260. || 
               Hypo[i].iNumPs < Gparm.MinPs || Hypo[i].iGoodSoln < 2 )
               for ( j=0; j<Nsta; j++ )
                  if ( StaArray[i][j].dPTime < dMin && 
                       StaArray[i][j].dPTime > 0.0 )
                  {
                     dMin = StaArray[i][j].dPTime;
                     iMin = i;
                  }
      logit( "", "Re-init buffer %ld\n", iMin );
      RequestSpecificMutex( &mutsem1 );   /* Semaphore protect buffer writes */
      iPBufCnt[iMin]     = 0;
      iNumPBufRem[iMin]  = 0;
      iLastBuffCnt[iMin] = 0;
      for ( i=0; i<Nsta; i++ ) InitP( &StaArray[iMin][i] );
      InitHypo( &Hypo[iMin] );
      iTemp =  atoi( Hypo[iMin].szQuakeID );
      iTemp += Gparm.NumPBuffs;
      if ( iTemp >= 900000 ) iTemp -= 900000;
      itoaX( iTemp, Hypo[iMin].szQuakeID );
      PadZeroes( 6, Hypo[iMin].szQuakeID );
      Hypo[iMin].iVersion     = 1;
      Hypo[iMin].iAlarmIssued = 0;
      ReleaseSpecificMutex( &mutsem1 );   /* Let someone else have sem */
			
/* Wait a bit before looping again */	 
Sleeper:
      sleep_ew( 3000 );    
   }
}

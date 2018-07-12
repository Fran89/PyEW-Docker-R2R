      /*****************************************************************
       *                         pick_wcatwc.c                         *
       *                                                               *
       *  This program is a conversion of the P-picker used at         *
       *  at the National Tsunami Warning Center since the             *
       *  early 1980s.  The basic algorithm for the P-picker was       *
       *  written by Veith in 1978 and is described in Technical Note  *
       *  1/78, "Seismic Signal Detection Algorithm" by Karl F. Veith, *
       *  Teledyne GeoTech.  This technique is an adaptation of the    *
       *  original Rex Allen picker.  Several variations to the        *
       *  Veith method have been added over the years, such as         *
       *  magnitude determinations and auto-calibrations.              *
       *                                                               *
       *  In general, the signal must go through                       *
       *  three processing stages before a pick is declared.  The      *
       *  first stage is a simple test which looks for higher than     *
       *  normal signal amplitudes.  Actually, it is not the amplitude *
       *  that is tested, but the cummulative difference between       *
       *  samples (the MDF).  This is compared to the background MDF   *
       *  every sample.                                                *
       *  The background MDF is computed with a moving averages        *
       *  technique.  Every LTASeconds the average MDF is computed and *
       *  this is averaged with the existing MDF to produce a new MDF. *
       *  The second phase of signal processing consists of two tests. *
       *  Test 1 specifies that the MDF must exceed a trigger          *
       *  threshold for a period of time after Phase 1 is passed twice *
       *  as often as it is under the threshold.  Test 2 states        *
       *  that the true amplitude must exceed the signal-to-noise      *
       *  ratio at some time while Test 1 is being conducted.  Phase 3 *
       *  consists of three tests.  Test 1 requires MDF greater than   *
       *  the trigger threshold at least 6 times in opposing directions*
       *  (i.e. it must see three full cycles of signal).  Test 2      *
       *  requires the frequency of the above oscillations to be       *
       *  greater than MinFreq.  Test 3 requires the MDF to be above   *
       *  the trigger for at least half the time it takes to pass the  *
       *  above two tests.  If these three Phases are passed, a P-pick *
       *  is declared.                                                 *
       *                                                               *
       *  At this point, the original MDF start time is saved as       *
       *  the event's P-time.  The picker continues to evaluate the    *
       *  signal for magnitude data.  Both Mb and Ml's are computed    *
       *  here along with sine-wave calibration processing.  (Sine-wave*
       *  cals are automatically fed into NTWC network analog          *
       *  stations twice a day.  These cals are analyzed to update     *
       *  station magnification at each cal.)  After the magnitude     *
       *  processing has finished, evaluation on this channel ceases   *
       *  and all variables are reinitialized.                         *
       *                                                               *
       *  The picker evaluates data at the incoming sample rate.  No   *
       *  re-sampling is necessary.  However, the picker does not work *
       *  well on anything but short-period data.  So, broadband data  *
       *  must be filtered before P-picking.  Filtering is provided by *
       *  this program.   Response for the default filter has been     *
       *  computed and is used to convert to ground motion.  If other  *
       *  filter parameters are used, the response must be calculated  *
       *  and used in the ground motion calculations.  The raw         *
       *  broadband data is still used, though, for Mwp computations.  *
       *                                                               *
       *  In this re-write of the P-picker, first-motion determination *
       *  is attempted.  Coda magnitude information is not computed    *
       *  here.  Gap interpolation is performed as in pick_ew.         *
       *                                                               *
       *  One other feature was added in this implementaion of the     *
       *  Veith picker.  That is an alarm feature.  This feature looks *
       *  for potentially large quakes recorded at any site.  When     *
       *  the alarm criteria is passed, an alarm messages is fed       *
       *  into the alarm ring.                                         *
       *                                                               *
       *  A no data alarm was also added to pick_wcatwc in 8/01.  An   * 
       *  alarm time is specified in the .d file.  If this time passes *
       *  with data data arriving in the ring, an alarm message will   *
       *  be issued to the alarm ring.                                 *
       *                                                               *
       *  The structure of this program was modeled after pick_ew.     *
       *  Many of the functions which are applicable to any P-picker   *
       *  were also taken from pick_ew.                                *
       *                                                               *
       *  May, 2009: Added Matlab neural net initialization for P-pick *
       *  November, 2007: Expanded no data or bad time alarms          *
       *  December, 2004: Added multi-station alarm feature and        *
       *                  PStrength computations.                      *
       *  October, 2005: Added options to run this module in           *
       *                 conjunction with ATPlayer.                    *
       *                                                               *
       *  Written by Paul Whitmore, December, 2000                     *
       *                                                               *
       ****************************************************************/
	   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#ifdef _WINNT
 #include <malloc.h>
#endif

#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <swap.h>
#include <kom.h>
#include "pick_wcatwc.h"
#ifdef _USE_NEURALNET_
 #include <mat.h>
 #include <libNeuralNet.h>
#endif

      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of picker configuration file         *
       ***********************************************************/

/* Global Variables 
   ****************/
GPARM			Gparm;           /* Configuration file parameters */
STATION			*StaArray;       /* Station array                 */
char			*WaveBuf;        /* Pointer to waveform buffer    */
ALARMSTRUCT		*AS;             /* Pointer to Alarm buffer       */
int                      Nsta;           /* Number of stations in list    */

int main( int argc, char **argv )
{
   char          *configfile;     /* Pointer to name of config file */
   static double dLastEndTime;    /* End time of last packet */
   EWH           Ewh;             /* Parameters from earthworm.h */
   MSG_LOGO      getlogo;         /* Logo of requested waveforms */
   MSG_LOGO      hrtlogo;         /* Logo of outgoing heartbeats */
   int           i, iTemp;        /* Loop counter */
   long          InBufl;          /* Maximum message size in bytes */
   static int    iNoDataAlarmIssued=0; /* 1, 2... after "no data" alarm issued*/
   static int    iNumRegions;     /* Number of alarm regions */
   char          line[40];        /* Heartbeat message */
   int           lineLen;         /* Length of heartbeat message */
   time_t        lLastData;       /* 1/1/70 time of last data provided */
   MSG_LOGO      logo;            /* Logo of retrieved msg */
   long          MsgLen;          /* Size of retrieved message */
   pid_t         myPid;           /* Process id of this process */
   long          RawBufl;         /* Raw data buffer size (for Mwp) */
   time_t        then;            /* Previous heartbeat time */
   time_t        diffTime;
   int32_t       WaveAcc[MAX_TRACEBUF_SIZ];/* Long ptr to acc data */
   TRACE2_HEADER *WaveHead;       /* Pointer to waveform header */
   int32_t       *WaveLong;       /* Long pointer to waveform data */
   int32_t       WaveRaw[MAX_TRACEBUF_SIZ];/* Unfiltered waveform data */
   short         *WaveShort;      /* Short pointer to waveform data */
#ifdef _WINNT
   char *paramdir;
#endif
   char  FullTablePath[512];
   
/* Call the MCR and library initialization functions (Added 5/09 dln) */
#ifdef _USE_NEURALNET_
   static int    MclInitFlag = 0;

   if (MclInitFlag == 0 )
   {
      MclInitFlag = 1;
      if ( !mclInitializeApplication( NULL, 0 ) )
      {
         fprintf( stderr, "Could not initialize the matlab NN application.\n" );
         exit(1);                
      }
      if ( !libNeuralNetInitialize() )
      {
         fprintf( stderr, "Could not initialize the matlab nn library.\n" );
         logit( "", "Could not initialize the matlab nn library.\n" );
         logit( "" , " errno: %d  \n", errno );
      }
   } 
#endif

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: pick_wcatwc <configfile>\n" );
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
      fprintf( stderr, "pick_wcatwc: GetConfig() failed. Exiting.\n" );
      return -1;
   }

/* Look up info in the earthworm.h tables
   **************************************/
   if ( GetEwh( &Ewh ) < 0 )
   {
      fprintf( stderr, "pick_wcatwc: GetEwh() failed. Exiting.\n" );
      return -1;
   }

/* Specify logos of incoming waveforms and outgoing heartbeats
   ***********************************************************/
   getlogo.instid = Ewh.GetThisInstId;
   getlogo.mod    = Ewh.GetThisModId;
   getlogo.type   = Ewh.TypeWaveform;
   hrtlogo.instid = Ewh.MyInstId;
   hrtlogo.mod    = Gparm.MyModId;
   hrtlogo.type   = Ewh.TypeHeartBeat;

/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, Gparm.MyModId, 512, 1 );

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "pick_wcatwc: Can't get my pid. Exiting.\n" );
      return -1;
   }
  
/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm );
   
/* Read in Alarm parameters if turned on
   *************************************/
   if ( Gparm.TwoStnAlarmOn == 1 )
      if ( ReadAlarmParams( &AS, &iNumRegions, Gparm.TwoStnAlarmFile ) == 0 )
      {
         logit( "e", "pick_wcatwc: can't read alarm file %s; Exiting.\n",
                Gparm.TwoStnAlarmFile );
         return -1;
      }             
	  
/* Allocate the waveform buffer
   ****************************/
   InBufl = MAX_TRACEBUF_SIZ*2 + sizeof(int32_t)*(Gparm.MaxGap-1);
   WaveBuf = (char *) malloc( (size_t) InBufl );
   if ( WaveBuf == NULL )
   {
      logit( "et", "pick_wcatwc: Cannot allocate waveform buffer\n" );
      free( AS );
      return -1;
   }

/* Point to header and data portions of waveform message
   *****************************************************/
   WaveHead  = (TRACE2_HEADER *) WaveBuf;
   WaveLong  = (int32_t *)  (WaveBuf + sizeof( TRACE2_HEADER ));
   WaveShort = (short *) (WaveBuf + sizeof( TRACE2_HEADER ));

/* Read the station list and return the number of stations found.
   Allocate the station list array.
   **************************************************************/
   if ( ReadStationList( &StaArray, &Nsta, Gparm.StaFile, Gparm.StaDataFile,
                         Gparm.ResponseFile, MAX_STATIONS, 1 ) == -1 )
   {
      logit( "", "pick_wcatwc: ReadStationList() failed. Exiting.\n" );
      free( WaveBuf );
      free( AS );
      return -1;
   }
   if ( Nsta == 0 )
   {
      logit( "et", "pick_wcatwc: Empty station list. Exiting." );
      free( AS );
      free( WaveBuf );
      free( StaArray );
      return -1;
   }
   logit( "t", "pick_wcatwc: Picking %d stations", Nsta );

/* Log the station list
   ********************/
   LogStaListP( StaArray, Nsta );

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
                          WaveBuf, MAX_TRACEBUF_SIZ) != GET_NONE );

/* Send 1st heartbeat to the transport ring
   ****************************************/
   time( &then );
   sprintf( line, "%ld %d\n", (long) then, (int)myPid );
   lineLen = (int)strlen( line );
   if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) !=
        PUT_OK )
   {
      logit( "et", "pick_wcatwc: Error sending 1st heartbeat. Exiting." );
      if ( Gparm.OutKey != Gparm.InKey )
      {
         tport_detach( &Gparm.InRegion );
         tport_detach( &Gparm.OutRegion );
         tport_detach( &Gparm.AlarmRegion );
      }
      else
         tport_detach( &Gparm.InRegion );
      free( AS );
      free( WaveBuf );
      free( StaArray );
      return 0;
   }                                                                  
   lLastData = then;

/* Loop to read waveform messages and invoke the picker
   ****************************************************/
   dLastEndTime = 0.;   

   while ( tport_getflag( &Gparm.InRegion ) != TERMINATE )
   {
      char    type[3];
      static  STATION *Sta;     /* Pointer to the station being processed */
      int     rc;               /* Return code from tport_getmsg() */
      static  time_t  now;      /* Current time */
      double  GapSizeD;         /* Number of missing samples (double) */
      long    GapSize;          /* Number of missing samples (integer) */

/* Send a heartbeat to the transport ring
   **************************************/
      time( &now );
      if ( (now - then) >= Gparm.HeartbeatInt )
      { 
/* Have we received data through ring without AlarmTime seconds elapsing?
   **********************************************************************/
         then = now;
         sprintf( line, "%ld %d\n", (long) now, (int)myPid );
         lineLen = (int)strlen( line );
         if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) !=
              PUT_OK )
         {
            logit( "et", "pick_wcatwc: Error sending heartbeat. Exiting." );
            break;
         }                            
      }                     
/* Send No Data Alarm if the clock was set way back in time, or no data
   has been received in a long time.
   ********************************************************************/
      if ( strlen( Gparm.ATPLineupFileBB ) < 3 )  
	  {
         diffTime = now - lLastData;
         if ( diffTime < 0 ) diffTime = diffTime * -1;
         if ( diffTime > Gparm.AlarmTime )
         {
            if ( iNoDataAlarmIssued == 0 )     
            {
               logit( "t", "No Data Alarm Activated\n" );
               ReportAlarm( StaArray, Gparm.MyModId, Gparm.AlarmRegion,
                Ewh.TypeAlarm, Ewh.MyInstId, 5, "", 0 );
            }
            iNoDataAlarmIssued++;
         }
	  }
/* Get a waveform buffer from transport region
   *******************************************/
      rc = tport_getmsg( &Gparm.InRegion, &getlogo, 1, &logo, &MsgLen,
                         WaveBuf, MAX_TRACEBUF_SIZ);

      if ( rc == GET_NONE )
      {
         sleep_ew( 200 );
         continue;
      }

      if ( rc == GET_NOTRACK )
         logit( "et", "pick_wcatwc: Tracking error.\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "pick_wcatwc: Got lapped on the ring.\n");

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "pick_wcatwc: Gap in sequence numbers.\n");

      if ( rc == GET_MISS )
         logit( "et", "pick_wcatwc: Missed messages.\n");

      if ( rc == GET_TOOBIG )
      {
         logit( "et", "pick_wcatwc: Retrieved message too big (%d) for msg.\n",
                MsgLen );
         continue;
      }

/* If necessary, swap bytes in the message
   ***************************************/
      if ( WaveMsg2MakeLocal( WaveHead ) < 0 )
      {
         logit( "et", "pick_wcatwc: Unknown waveform type.\n" );
         continue;
      }

/* If we are in the ATPlayer version of pick_, see if we should re-init
   ********************************************************************/
#ifdef _WINNT
      if ( strlen( Gparm.ATPLineupFileBB ) > 2 && 
                   Gparm.iRedoLineupFile == 1 )                /* Then we are */
         if ( fabs( WaveHead->starttime-(int) dLastEndTime ) > 1800 ) 
         {
            free( StaArray );                                  
            Nsta = ReadLineupFile( Gparm.ATPLineupFileBB, &StaArray, 
                                   MAX_STATIONS );
            logit( "", "reset StaArray Nsta = %d\n", Nsta );	 
            if ( Nsta < 2 )
            {
               logit( "", "Bad Lineup File read %s\n", Gparm.ATPLineupFileBB );
               continue;
            }	    
            for ( i=0; i<Nsta; i++ )
            {
               free( StaArray[i].plRawData );
               free( StaArray[i].pdRawDispData );
               free( StaArray[i].pdRawIDispData );
               free( StaArray[i].plPickBuf );
               free( StaArray[i].plPickBufRaw );
            }
        } 	 
#endif

/* Look up SCN number in the station list
   **************************************/
      Sta = NULL;								  
      for ( i=0; i<Nsta; i++ )
         if ( !strcmp( WaveHead->sta,  StaArray[i].szStation ) &&
              !strcmp( WaveHead->chan, StaArray[i].szChannel ) &&
              !strcmp( WaveHead->loc,  StaArray[i].szLocation ) &&
              !strcmp( WaveHead->net,  StaArray[i].szNetID ) )
         {
            Sta = (STATION *) &StaArray[i];
            break;
         }

      if ( Sta == NULL )      /* SCN not found */
         continue;

/* Is this a station we want to pick?
   **********************************/
      if ( Sta->iPickStatus == 0 ) continue;

/* Check if the time stamp is reasonable.  If it is ahead of the present
   1/1/70 time, it is not reasonable. (+1. to account for int).
   *********************************************************************/
      if ( WaveHead->endtime > (double) now+1. )
      {
         logit( "t", "%s %s %s %s endtime (%lf) ahead of present (%ld)\n",
                Sta->szStation, Sta->szChannel, Sta->szNetID, 
                Sta->szLocation, WaveHead->endtime, (long) now );
         continue;
      }

/* Do this the first time we get a message with this SCN
   *****************************************************/
      if ( Sta->iFirst == 1 )
      {
         Sta->lPickIndex   = 1;
         Sta->dSampRate    = WaveHead->samprate;
         Sta->dEndTime     = WaveHead->endtime;
         Sta->dStartTime   = WaveHead->starttime;
         Sta->dDataEndTime = WaveHead->endtime;
         Sta->iFirst       = 0;
         Sta->lEndData     = WaveLong[0];
         ResetFilter( Sta );
		 
/* Allocate memory for raw data buffer (assume samprate will not change for
   each station and that samprate is greater than 1)
   *************************************************/
         RawBufl = sizeof (int32_t) * (long) (WaveHead->samprate+0.001) *
                   Gparm.MwpSeconds;
         if ( RawBufl/sizeof (int32_t) > MAXMWPARRAY )
         {
            logit( "et", "Mwp array too large for %s\n", Sta->szStation );
            RawBufl = MAXMWPARRAY * sizeof (int32_t);
         }
         Sta->plRawData = (int32_t *) malloc( (size_t) RawBufl );
         if ( Sta->plRawData == NULL )
         {
            logit( "et", "pick_wcatwc: Can't allocate raw data buffer for %s\n",
			       Sta->szStation );
            free( AS );
            free( WaveBuf );              /* Free-up allocated memory */
            free( StaArray );
            return -1;
         }

         RawBufl = (MAX_NN_SAMPS) * sizeof (int32_t);
         Sta->plPickBuf = (int32_t *) malloc( (size_t) RawBufl );
         if ( Sta->plPickBuf == NULL )
         {
            logit( "et", "pick_wcatwc: Can't allocate pick data buff for %s\n",
			       Sta->szStation );
            free( AS );
            free( WaveBuf );              /* Free-up allocated memory */
            for ( i=0; i<Nsta; i++ ) free( StaArray[i].plRawData );
            free( StaArray );
            return -1;
         }

         RawBufl = (MAX_NN_SAMPS) * sizeof (int32_t);
         Sta->plPickBufRaw = (int32_t *) malloc( (size_t) RawBufl );
         if ( Sta->plPickBufRaw == NULL )
         {
            logit( "et", "pick_wcatwc: Can't allocate pick data buff for %s\n",
			       Sta->szStation );
            free( AS );
            free( WaveBuf );              /* Free-up allocated memory */
            for ( i=0; i<Nsta; i++ ) free( StaArray[i].plRawData );
            for ( i=0; i<Nsta; i++ ) free( StaArray[i].plPickBuf );
            free( StaArray );
            return -1;
         }
		 
/* Allocate memory for Mwp displacement data buffer 
   ************************************************/
         RawBufl = sizeof (double) * (long) (WaveHead->samprate+0.001) *
                   Gparm.MwpSeconds;
         if ( RawBufl/sizeof (double) > MAXMWPARRAY )
         {
            logit( "et", "Mwp array too large for %s\n", Sta->szStation );
            RawBufl = MAXMWPARRAY * sizeof (double);
         }
         Sta->pdRawDispData = (double *) malloc( (size_t) RawBufl );
         if ( Sta->pdRawDispData == NULL )
         {
            logit( "et", "pick_wcatwc: Can't allocate disp. data buff for %s\n",
			       Sta->szStation );
            free( AS );
            free( WaveBuf );              /* Free-up allocated memory */
            for ( i=0; i<Nsta; i++ ) 
            {
               free( StaArray[i].plRawData );
               free( StaArray[i].plPickBuf );
               free( StaArray[i].plPickBufRaw );
            }
            free( StaArray );
            return -1;
         }
		 
/* Allocate memory for Mwp integrated displacement data buffer 
   ***********************************************************/
         Sta->pdRawIDispData = (double *) malloc( (size_t) RawBufl );
         if ( Sta->pdRawIDispData == NULL )
         {
            logit( "et", "pick_wcatwc: Cannot allocate int. disp. data buffer for %s\n",
			       Sta->szStation );
            free( AS );
            free( WaveBuf );              /* Free-up allocated memory */
            for ( i=0; i<Nsta; i++ ) 
            {
               free( StaArray[i].plRawData );
               free( StaArray[i].pdRawDispData );
               free( StaArray[i].plPickBuf );
               free( StaArray[i].plPickBufRaw );
            }
            free( StaArray );
            return -1;
         }		 		 
         Sta->plFiltCircBuff = (int32_t *) malloc( 100 * sizeof( int32_t) );
         Sta->plRawCircBuff  = (int32_t *) malloc( 100 * sizeof( int32_t) );
         dLastEndTime = WaveHead->endtime;
         continue;
      }
      
/* If data is not in order, throw it out
   *************************************/
      if ( Sta->dEndTime > WaveHead->starttime+0.001 ) continue;
      lLastData = now;         /* We got some data, so update last alarm time */
      iNoDataAlarmIssued = 0;

/* If the samples are shorts, make them longs
   ******************************************/
      strcpy( type, WaveHead->datatype );
      if ( (strcmp( type,"i2" ) == 0) || (strcmp( type,"s2" ) == 0) )
         for ( i=WaveHead->nsamp-1; i>-1; i-- )
            WaveLong[i] = (int32_t) WaveShort[i];

/* Save int32 data as raw, unfiltered data
   **************************************/
      for ( i=WaveHead->nsamp-1; i>-1; i-- ) WaveRaw[i] = WaveLong[i];      
	  
/* Compute the number of samples since the end of the previous message.
   If (GapSize == 1), no data has been lost between messages.
   If (1 < GapSize <= Gparm.MaxGap), data will be interpolated.
   If (GapSize > Gparm.MaxGap), the picker will go into restart mode.
   *******************************************************************/
      GapSizeD = WaveHead->samprate * (WaveHead->starttime - Sta->dEndTime);
      if ( GapSizeD < 0. )          /* Invalid. Time going backwards. */
         GapSize = 0;
      else
         GapSize  = (long) (GapSizeD + 0.5);

/* Interpolate missing samples and prepend them to the current message
   *******************************************************************/
      if ( (GapSize > 1) && (GapSize <= Gparm.MaxGap) )
         Interpolate( Sta, WaveBuf, GapSize );

/* Announce large sample gaps
   **************************/
      if ( GapSize > Gparm.MaxGap )
      {
         int      lnLen;
         time_t   errTime;
         char     errmsg[80];
         MSG_LOGO msgLogo;

         time( &errTime );
         sprintf( errmsg,
               "%ld %d Found %4ld sample gap. Restarting station %-5s%-2s%-3s %s\n",
               (long)errTime, PK_RESTART, GapSize, Sta->szStation, Sta->szNetID,
               Sta->szChannel, Sta->szLocation );
         lnLen = (int)strlen( errmsg );
         msgLogo.type   = Ewh.TypeError;
         msgLogo.mod    = Gparm.MyModId;
         msgLogo.instid = Ewh.MyInstId;
         tport_putmsg( &Gparm.InRegion, &msgLogo, lnLen, errmsg );
         if ( Gparm.Debug )
            logit( "t", "pick_wcatwc: Restarting %-5s%-2s %-3s %s. GapSize = %d\n",
             Sta->szStation, Sta->szNetID, Sta->szChannel, Sta->szLocation, GapSize ); 
      }

/* For big gaps, enter restart mode. In restart mode, calculate
   moving averages without picking.  Start picking again after a
   specified number of samples has been processed.
   *************************************************************/
      if ( GapSize > Gparm.MaxGap )
      {
         if ( Sta->iPickStatus > 0 ) Sta->iPickStatus = 1;
         ResetFilter( Sta );
         InitVar( Sta );
         if ( Sta->iAlarmStatus >= 1 ) Sta->iAlarmStatus = 1;
      }
	  
/* Short-period bandpass filter the data if specified in station file -
   Here filter the data without removing DC offset.  This offset is taken out
   later.  This may occasionally cause pick at data start or after gaps if
   a station has a large DC offset (filter response to offset).
   (This is usually necessary for broadband data, not so for SP data.
   Taper the signal for restarts, but not for continuous signal.).
   ******************************************************************/
      if ( Sta->iFiltStatus == 1 )
         FilterPacket( WaveLong, Sta, WaveHead->nsamp, WaveHead->samprate,
                       Gparm.LowCutFilter, Gparm.HighCutFilter,
                       3.*(1./Gparm.LowCutFilter) );

/* Convert signal to acceleration data and pick on that.
   *****************************************************/
      if ( Gparm.PickOnAcc == 1 )
	  {
         WaveAcc[0] = (int32_t)
          ((double) (WaveLong[0]-Sta->lEndData)*WaveHead->samprate);
         for ( i=1; i<WaveHead->nsamp; i++ ) 
            WaveAcc[i] = (int32_t)
             ((double) (WaveLong[i]-WaveLong[i-1])*WaveHead->samprate);      
	  }

/* Save time and amplitude of the end of the current message
   *********************************************************/
      Sta->lEndData = WaveLong[WaveHead->nsamp-1];
      Sta->dEndTime = WaveHead->endtime;
      Sta->dDataEndTime = WaveHead->endtime;
      Sta->dDataStartTime = WaveHead->starttime;
      dLastEndTime = WaveHead->endtime;
	  
/* Run data through MovingAvg function and picker
   **********************************************/	  
      PickV( Sta, WaveHead->starttime, Gparm.AlarmOn, Gparm.TwoStnAlarmOn,
             Gparm.AlarmTimeout, Gparm.MinFreq, Gparm.LTASeconds,
             Gparm.MwpSeconds, Gparm.MwpSigNoise, Gparm.LGSeconds,
             Gparm.MbCycles, Gparm.dSNLocal, Gparm.dMinFLoc, Gparm.MyModId,
             Gparm.AlarmRegion, Gparm.OutRegion, Ewh.TypeAlarm, Ewh.TypePickTWC,
             Ewh.MyInstId, WaveRaw, WaveLong, AS, iNumRegions, 1, &iTemp,
             Gparm.NeuralNet, Gparm.PickOnAcc, WaveAcc );
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
   free( AS );
   free( WaveBuf );                     /* Free-up allocated memory */
   for ( i=0; i<Nsta; i++ ) 
   {
      free( StaArray[i].plRawData );
      free( StaArray[i].pdRawDispData );
      free( StaArray[i].pdRawIDispData );
      free( StaArray[i].plPickBuf );
      free( StaArray[i].plPickBufRaw );
   }
   
#ifdef _USE_NEURALNET_
   libNeuralNetTerminate();    //added 5/5/2009 by dln
   mclTerminateApplication();  //added 5/5/2009 by dln
#endif

   free( StaArray );
   logit( "t", "Termination requested. Exiting.\n" );
   return 0;
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
      fprintf( stderr, "pick_wcatwc: Error getting MyInstId.\n" );
      return -1;
   }

   if ( GetInst( "INST_WILDCARD", &Ewh->GetThisInstId ) != 0 )
   {
      fprintf( stderr, "pick_wcatwc: Error getting GetThisInstId.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &Ewh->GetThisModId ) != 0 )
   {
      fprintf( stderr, "pick_wcatwc: Error getting GetThisModId.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      fprintf( stderr, "pick_wcatwc: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      fprintf( stderr, "pick_wcatwc: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_ALARM", &Ewh->TypeAlarm ) != 0 )
   {
      fprintf( stderr, "pick_wcatwc: Error getting TypeAlarm.\n" );
      return -6;
   }
   if ( GetType( "TYPE_PICKTWC", &Ewh->TypePickTWC ) != 0 )
   {
      fprintf( stderr, "pick_wcatwc: Error getting TypePickTWC.\n" );
      return -7;
   }
   if ( GetType( "TYPE_TRACEBUF2", &Ewh->TypeWaveform ) != 0 )
   {
      fprintf( stderr, "pick_wcatwc: Error getting TYPE_TRACEBUF.\n" );
      return -8;
   }
   if ( GetType( "TYPE_PICK_GLOBAL", &Ewh->TypePickGlobal ) != 0 )
   {
      fprintf( stderr, "pick_wcatwc: Error getting TypePickGlobal.\n" );
      return -9;
   }
   return 0;
}

   /*************************************************************
    *                       Interpolate()                       *
    *                                                           *
    *  Interpolate samples and insert them at the beginning of  *
    *  the waveform.                                            *
    *************************************************************/

void Interpolate( STATION *Sta, char *WavBuf, int GapSize )
{
   int      i;
   int      j = 0;
   int      nInterp = GapSize-1;
   TRACE2_HEADER *WaveHead  = (TRACE2_HEADER *) WavBuf;
   int32_t  *WaveLong  = (int32_t *) (WavBuf + sizeof(TRACE2_HEADER));
   double   SampleInterval  = 1./WaveHead->samprate;
   double   delta = (double) (WaveLong[0]-Sta->lEndData) / GapSize;

   for ( i=WaveHead->nsamp-1; i>=0; i-- )
      WaveLong[i+nInterp] = WaveLong[i];

   for ( i=0; i<nInterp; i++ )
      WaveLong[i] = (int32_t) (Sta->lEndData + (++j*delta) + 0.5);

   WaveHead->nsamp += nInterp;
   WaveHead->starttime = Sta->dEndTime + SampleInterval;
}

 /***********************************************************************
  *                             LogStaListP()                           *
  *                                                                     *
  *                         Log the station list                        *
  ***********************************************************************/

void LogStaListP( STATION *Sta, int NumSta )
{
   int i;

   logit( "", "\nStation List:\n" );
   for ( i=0; i<NumSta; i++ )
   {
      logit( "", "%4s",      Sta[i].szStation );
      logit( "", " %3s",     Sta[i].szChannel );
      logit( "", " %2s",     Sta[i].szNetID );
      logit( "", " %2s",     Sta[i].szLocation );
      logit( "", " %1d",     Sta[i].iPickStatus );
      logit( "", " %1d",     Sta[i].iFiltStatus );
      logit( "", " %2d",     Sta[i].iSignalToNoise );
      logit( "", " %1d",     Sta[i].iAlarmStatus );
      logit( "", " %10.8lf", Sta[i].dAlarmAmp );
      logit( "", " %6.1lf",  Sta[i].dAlarmDur );
      logit( "", " %6.3lf",  Sta[i].dAlarmMinFreq );
      logit( "", " %1d",     Sta[i].iComputeMwp );
      logit( "", " %1d",     Sta[i].iFirst );
      logit( "", "\n" );
   }
   logit( "", "\n" );
}

   /*************************************************************
    *                     ReadAlarmParams()                     *
    *                                                           *
    *  Fill up AlarmStructur with parameters from control file. *
    *                                                           *
    *  Variables:                                               *
    *   pAS             Alarm Structure                         *
    *   piNumRegs       Number of alarm regions                 *
    *   pszFile         Alarm parameter file                    *
    *                                                           *
    *  Return:                                                  *
    *   1=ok, 0=problem                                         *
    *                                                           *
    *************************************************************/

int ReadAlarmParams( ALARMSTRUCT **pAS, int *piNumRegs, char *pszFile )
{
   int     i, ii, j, k, nfiles;
   long    InBufl;                    /* buffer size */
   static  ALARMSTRUCT *ptAS;         /* temp pointer */
   
/* Open file and read if open ok */   
   nfiles = k_open( pszFile );
   if ( nfiles == 0 )
   {
      logit( "", "Read problem in ReadAlarmParams (%s)\n", pszFile );
      return( 0 );
   }
   
   while ( nfiles > 0 )          /* While there are config files open */
   {
      while ( k_rd() )           /* Read next line */
      {
         char *com;
         char *str;

         com = k_str();          /* Get the first token from line */

         if ( !com ) continue;             /* Ignore blank lines */
         if ( com[0] == '#' ) continue;    /* Ignore comments */

/* Read configuration parameters
   *****************************/
         if ( k_its( "iNumRegions" ) ) 
         {
            *piNumRegs = k_int();
/* Allocate and init the Alarm buffer
   **********************************/
            InBufl = sizeof( ALARMSTRUCT ) * *piNumRegs; 
            ptAS = (ALARMSTRUCT *) calloc( *piNumRegs, sizeof( ALARMSTRUCT ) );
            if ( ptAS == NULL )
            {
               logit( "et", "pick_wcatwc: Cannot allocate alarm buffer\n");
               nfiles = k_close();
               return( 0 );
            }

            for ( i=0; i<*piNumRegs; i++ )
            {
               for ( ii=0; ii<MAX_ALARM_STN; ii++ )
                  strcpy( ptAS[i].szStnAlarm[ii], "\0" );
               ptAS[i].dLastTime = 0.;
               ptAS[i].iNumPicksCnt = 0;
               for ( j=0; j<5; j++ )
               {
                  k_rd();
                  com = k_str();                    /* Read more */
                  if ( k_its( "Region" ) )
                  { if ( (str = k_str()) != NULL ) strcpy( ptAS[i].szRegionName, str ); }
                  else if ( k_its( "AlarmThresh" ) )
                  {  ptAS[i].iAlarmThresh = k_int(); }
                  else if ( k_its( "PStrength" ) )
                  {  ptAS[i].dThresh = k_val(); }
                  else if ( k_its( "MaxTime" ) )
                  {  ptAS[i].dMaxTime = k_val(); }
                  else if ( k_its( "NumStnInReg" ) )
                  {
                     ptAS[i].iNumStnInReg = k_int();
                     if ( ptAS[i].iNumStnInReg >= MAX_ALARM_STN )
                     {
                        logit( "et", "Too many alarm stns - %d\n",
                                ptAS[i].iNumStnInReg );
                        nfiles = k_close();
                        free( ptAS );
                        return( 0 );
                     }
                     for ( k=0; k<ptAS[i].iNumStnInReg; k++ )
                     {
                        k_rd();
                        com = k_str();                    /* Read more */
                        if ( k_its( "Station" ) )
                        {  if ( (str = k_str()) != NULL ) strcpy( ptAS[i].szStation[k], str ); }
                     }
                  }
               }				  
            }
         }
      }              
      nfiles = k_close();
   }
   
/* Log input data
   **************/   
   logit( "", "NumAlarmRegions = %d\n", *piNumRegs );
   for ( i=0; i<*piNumRegs; i++ )
   {
      logit( "", "%s\n", ptAS[i].szRegionName );
      logit( "", "Alarm Threshold %d\n", ptAS[i].iAlarmThresh );
      logit( "", "PStrength Threshold %lf\n", ptAS[i].dThresh );
      logit( "", "MaxTime %lf\n", ptAS[i].dMaxTime );
      logit( "", "NumStnInReg %d\n", ptAS[i].iNumStnInReg );
      for ( k=0; k<ptAS[i].iNumStnInReg; k++ )
         logit( "", "%s\n", ptAS[i].szStation[k] );
   }
   *pAS = ptAS;
     
   return( 1 );
}


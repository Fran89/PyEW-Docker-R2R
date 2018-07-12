  /*************************************************************************
   *                            Theta.c                                    *
   *                                                                       *
   *   EarlyBird module that calculates theta for events with Mwp above    *
   *   a certain threshold where:                                          *
   *                                                                       *
   *        theta = log10(energy/moment)                                   *
   *                                                                       *
   *  This parameter indicates when energy release is anomalously low,     *
   *  which occurs when rupture times are longer than normal. Such long    *
   *  rupture times can be indicative of tsunami earthquakes. Theta of     *
   *   -4.5 to -5.5 is normal with -6 typical for tsunami earthquakes.     *
   *  However, strike-slip events can have low values of theta as no       *
   *  correction is made for radiation pattern - it is assumed that the    *
   *  focal mechanism is unknown.                                          *
   *                                                                       *
   *  Hypocenters entered to HYPO_RING are obtained by this module.        *
   *  If the hypocenter has a magnitude greater than specified,            *
   *  this module, if configured, will compute Theta once data is          *
   *  sufficient.                                                          *
   *                                                                       *
   *  The incoming real-time signal is specified in the pick.sta           *
   *  file.  If taken from disk, the Station Array is set up based         *
   *  on contents of the disk file.                                        *
   *                                                                       *
   *  The method used for energy release calculation is that documented in *
   *                                                                       *
   *  Newman, A., and Okal, E., 1998. Teleseismic estimates of radiated    *
   *  seismic energy: the E/M0 discriminant for tsunami earthquakes,       *
   *  J. Geophys. Res., 103(11):26885-26898                                *
   *                                                                       *
   *  The moment is found from Mwp using:                                  *
   *                                                                       *
   *       log10(Mo) = 1.5*Mwp + 9.1                                       *
   *                                                                       *
   *  2008: Richard Luckett, British Geological Survey, rrl@bgs.ac.uk      *
   *  Dec., 2012: Added alarm sent to summary for very low theta.          *
   *  May, 2012: Re-written as non-GUI program by Whitmore (based on       *
   *             original Luckett code and MmNew schema).                  *
   *                                                                       *
   *************************************************************************/
	   
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>  
#include <swap.h>
#include "Theta.h"

/* Global Variables 
   ****************/
double  dLastEndTime;          /* End time of last data */
EWH     Ewh;                   /* Parameters from earthworm.h */
MSG_LOGO getlogoW;             /* Logo of requested waveforms */
GPARM   Gparm;                 /* Configuration file parameters */
HANDLE  hEventTheta;           /* Event shared externally to trigger Theta */
HANDLE  hEventThetaDone;       /* Event shared externally to signal proc. done*/
MSG_LOGO hrtlogo;              /* Logo of outgoing heartbeats */
HYPO    HStruct;               /* Hypocenter data structure */
int     iRunning;              /* 1-> Threads are operating A-OK; 0-> Stop */
mutex_t mutsem;                /* Semaphore to protect StaArray mods. */
pid_t   myPid;                 /* Process id of this process */
int     Nsta;                  /* Number of stations to process */
STATION *StaArray;             /* Station data array */
time_t  then;                  /* Previous heartbeat time */
char    *WaveBuf;              /* Pointer to waveform buffer */
TRACE2_HEADER *WaveHead;       /* Pointer to waveform header */
int32_t *WaveLong;             /* Int32 pointer to waveform data */
short   *WaveShort;            /* Short pointer to waveform data */

      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of configuration file                *
       ***********************************************************/

int main( int argc, char **argv )
{
   char         *configfile;      /* Name of config file */
   char          FullTablePath[512];
   int           i;
   long          InBufl;          /* Maximum message size in bytes */
   char          line[40];        /* Heartbeat message */
   int           lineLen;         /* Length of heartbeat message */
   MSG_LOGO      logo;            /* Logo of retrieved msg */
   long          MsgLen;          /* Size of retrieved message */
#ifdef _WINNT
   char         *paramdir;
   static unsigned tidC;          /* Theta semaphore checker Thread */
#endif
   static unsigned tidH;          /* Hypocenter getter Thread */
   static unsigned tidW;          /* Waveform getter Thread */

   dLastEndTime      = 0.;
   iRunning          = 1;
   
/* Get config file name (format "Theta Theta.d")
   *********************************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Need configfile in start line.\n" );
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
      fprintf( stderr, "GetConfig() failed. file %s.\n", configfile );
      return -1;
   }

/* Look up info in the earthworm.h tables
   **************************************/
   if ( GetEwh( &Ewh ) < 0 )
   {
      fprintf( stderr, "Theta: GetEwh() failed. Exiting.\n" );
      return -1;
   }

/* Specify logos of incoming waveforms and outgoing heartbeats
   ***********************************************************/
   getlogoW.instid = Ewh.GetThisInstId;
   getlogoW.mod    = Ewh.GetThisModId;
   getlogoW.type   = Ewh.TypeWaveform;
   hrtlogo.instid  = Ewh.MyInstId;
   hrtlogo.mod     = Gparm.MyModId;
   hrtlogo.type    = Ewh.TypeHeartBeat;

/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, Gparm.MyModId, 512, 1 );

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "Theta: Can't get my pid. Exiting.\n" );
      return -1;
   }

/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm );

/* Allocate the waveform buffers
   *****************************/
   InBufl = MAX_TRACEBUF_SIZ*2;
   WaveBuf = (char *) malloc( (size_t) InBufl );
   if ( WaveBuf == NULL )
   {
      logit( "et", "Theta: Cannot allocate waveform buffer\n" );
      return -1;
   }

/* Point to header and data portions of waveform message
   *****************************************************/
   WaveHead  = (TRACE2_HEADER *) WaveBuf;
   WaveLong  = (int32_t *) (WaveBuf + sizeof (TRACE2_HEADER));
   WaveShort = (short *) (WaveBuf + sizeof (TRACE2_HEADER));

/* Read the station list and return the number of stations found.
   Allocate the station list array.
   **************************************************************/
   if ( ReadStationList( &StaArray, &Nsta, Gparm.szStaFile, Gparm.szStaDataFile,
                         Gparm.szResponseFile, MAX_STATIONS, 0 ) == -1 )
   {
      logit( "", "Theta: ReadStationList() failed. Exiting.\n" );
      free( WaveBuf );
      return -1;
   }
   if ( Nsta == 0 )
   {
      logit( "et", "Theta: Empty station list. Exiting." );
      free( WaveBuf );
      free( StaArray );
      return -1;
   }
   logit( "t", "Theta: Processing %d stations.\n", Nsta );

/* Log the station list
   ********************/
   LogStaList( StaArray, Nsta );
   
#ifdef _WINNT
/* Create event to share with another module; this event
   is used to trigger Theta processing here.
   *****************************************************/
   hEventTheta = CreateEvent( NULL,      /* Default security */
                              FALSE,     /* Auto-reset event */
                              FALSE,     /* Initially not set */
                              "Theta" ); /* Share with other modules */
   if ( hEventTheta == NULL )            /* If event not created */
   {
      logit( "t", "failed to create theta event" );
      free( WaveBuf );
      free( StaArray );
      return -1;
   }
   
/* Create another event to share with other modules; this event signals 
   to the other module that the Theta data file has been filled. 
   ********************************************************************/
   hEventThetaDone = CreateEvent( NULL,         /* Default security */
                                  FALSE,        /* Auto-reset event */
                                  FALSE,        /* Initially not set */
                                  "ThetaDone" );/* Share with LOCATE */
   if ( hEventThetaDone == NULL )               /* If event not created */
   {                              
      logit( "t", "failed to create Theta done event" );
      free( WaveBuf );
      free( StaArray );
      CloseHandle( hEventTheta );
      return -1;
   }
#endif
   
/* Attach to existing transport rings
   **********************************/
   tport_attach( &Gparm.InRegion, Gparm.lInKey );
   tport_attach( &Gparm.HRegion,  Gparm.lHKey );

/* Flush the input waveform ring
   *****************************/
   while ( tport_getmsg( &Gparm.InRegion, &getlogoW, 1, &logo, &MsgLen,
                         WaveBuf, MAX_TRACEBUF_SIZ ) != GET_NONE );

/* Send 1st heartbeat to the transport ring
   ****************************************/
   time( &then );
   sprintf( line, "%ld %d\n", (long) then, (int)myPid );
   lineLen = (int)strlen( line );
   if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) != PUT_OK )
   {
      logit( "et", "Theta: Error sending 1st heartbeat. Exiting." );
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.HRegion );
      free( WaveBuf );
      free( StaArray );
#ifdef _WINNT
      CloseHandle( hEventTheta );
      CloseHandle( hEventThetaDone );
#endif
      return -1;
   }
   
/* Create a mutex for protecting adjustments of StaArray
   *****************************************************/
   CreateSpecificMutex( &mutsem );

#ifdef _WINNT
/* Start the Theta compute semaphore checker thread
   ************************************************/
   if ( StartThread( ThThread, 8192, &tidC ) == -1 )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.HRegion );
      free( WaveBuf );
      free( StaArray );
      CloseHandle( hEventTheta );
      CloseHandle( hEventThetaDone );
      logit( "et", "Error starting ThThread; exiting!\n" );
      return -1;
   }
#endif
                     
/* Start the Hypocenter getter thread
   **********************************/
   if ( Gparm.iAutoStart == 1 )/* Only use this thread if Theta is auto-started */
      if ( StartThread( HThread, 8192, &tidH ) == -1 )
      {
         tport_detach( &Gparm.InRegion );
         tport_detach( &Gparm.HRegion );
         free( WaveBuf );
         free( StaArray );
#ifdef _WINNT
         CloseHandle( hEventTheta );
         CloseHandle( hEventThetaDone );
#endif
         logit( "et", "Error starting Hthread; exiting!\n" );
         return -1 ;
      }

/* Start the waveform getter tread
   *******************************/
   if ( StartThread( WThread, 8192, &tidW ) == -1 )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.HRegion );
      free( WaveBuf );
      free( StaArray );
#ifdef _WINNT
      CloseHandle( hEventTheta );
      CloseHandle( hEventThetaDone );
#endif
      logit( "et", "Error starting Wthread; exiting!\n" );
      return -1;
   }                 

/* Stay here until one of the threads says to exit
   ***********************************************/
   while ( iRunning == 1 ) sleep_ew( 2000 );

/* Detach from the ring buffers
   ****************************/
   tport_detach( &Gparm.InRegion );
   tport_detach( &Gparm.HRegion );
   free( WaveBuf );
#ifdef _WINNT
   CloseHandle( hEventTheta );
   CloseHandle( hEventThetaDone );
#endif
   for ( i=0; i<Nsta; i++ ) free( StaArray[i].plRawCircBuff );
   free( StaArray );
   logit( "t", "Termination requested. Exiting.\n" );
   return 0;
}

      /*******************************************************
       *                      GetEwh()                       *
       *                                                     *
       *      Get parameters from the earthworm.d file.      *
       *******************************************************/

int GetEwh( EWH *EwhVal )
{
   if ( GetLocalInst( &EwhVal->MyInstId ) != 0 )
   {
      fprintf( stderr, "Theta: Error getting MyInstId.\n" );
      return -1;
   }
   if ( GetInst( "INST_WILDCARD", &EwhVal->GetThisInstId ) != 0 )
   {
      fprintf( stderr, "Theta: Error getting GetThisInstId.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &EwhVal->GetThisModId ) != 0 )
   {
      fprintf( stderr, "Theta: Error getting GetThisModId.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &EwhVal->TypeHeartBeat ) != 0 )
   {
      fprintf( stderr, "Theta: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &EwhVal->TypeError ) != 0 )
   {
      fprintf( stderr, "Theta: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_TRACEBUF2", &EwhVal->TypeWaveform ) != 0 )
   {
      fprintf( stderr, "Theta: Error getting TYPE_TRACEBUF2.\n" );
      return -6;
   }
   if ( GetType( "TYPE_HYPOTWC", &EwhVal->TypeHypoTWC ) != 0 )
   {
      fprintf( stderr, "Theta: Error getting TYPE_HYPOTWC.\n" );
      return -7;
   }
   return 0;
}

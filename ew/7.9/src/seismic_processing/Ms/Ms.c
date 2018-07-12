      /*****************************************************************    
       *                        Ms.c                                   *
       *                                                               *
       *  This program processes seismic data to determine             *
       *  the traditional Ms magnitude.                                *
       *                                                               *
       *  Data are obtained from either an earthworm ring or from disk *
       *  if data are no longer in the ring.                           *
       *                                                               *
       *  The data format of the disk data is that which is output     *
       *  by the WC&ATWC Earthworm module disk_wcatwc.  This is an     *
       *  internal format described in disk_wcatwc and in ReadDiskData.*
       *                                                               *
       *  Hypocenters entered to HYPO_RING are obtained by this module.*
       *  If the hypocenter has a magnitude greater than specified,    *
       *  this module, if configured, will compute expected            *
       *  Rayleigh wave times for the quake and will evaluate the data *
       *  for Ms.  When an Ms is computed, the data is sent in the     *
       *  PICK_TWC message format to PICK_RING with P-time= 0.         *
       *                                                               *
       *  This module is based on the WCATWC Earlybird lpproc module.  *
       *  Graphics have been stripped out and left in                  *
       *  SeismicWaveformDisplay.  This module is activated either     *
       *  automatically by a Hypocenter with a certain magnitude or    *
       *  from another module through an Event.                        *
       *                                                               *
       *  The incoming real-time signal is specified in the pick.sta   *
       *  file.  If taken from disk, the Station Array is set up based *
       *  on contents of the disk file.                                *
       *                                                               *
       *  2011: Paul Whitmore, NOAA-WCATWC - paul.whitmore@noaa.gov    *
       *                                                               *
       ****************************************************************/
	   
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
#include "Ms.h"

/* Global Variables 
   ****************/
double  dLastEndTime;          /* End time of last packet */
EWH     Ewh;                   /* Parameters from earthworm.h */
MSG_LOGO getlogoW;             /* Logo of requested waveforms */
GPARM   Gparm;                 /* Configuration file parameters */
HANDLE  hEventMs;              /* Event shared externally to trigger LP proc. */
HANDLE  hEventMsDone;          /* Event shared externally to signal proc. done*/
MSG_LOGO hrtlogo;              /* Logo of outgoing heartbeats */
HYPO    HStruct;               /* Hypocenter data structure */
int     iLPProcessing;         /* 0->no LP processing going on, 1-> LP going */
int     iRunning;              /* 1-> Threads are operating A-OK; 0-> Stop */
mutex_t mutsem;                /* Semaphore to protect StaArray mods. */
pid_t   myPid;                 /* Process id of this process */
int     Nsta;                  /* Number of stations to process */
STATION *StaArray;             /* Station data array */
time_t  then;                  /* Previous heartbeat time */
char    *WaveBuf;              /* Pointer to waveform buffer */
char    *WaveBufFilt;          /* Pointer to filtered waveform buffer */
TRACE2_HEADER *WaveHead;       /* Pointer to waveform header */
int32_t *WaveLong;             /* Long pointer to waveform data */
int32_t *WaveLongF;            /* Filtered Waveform data with DC removed */
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
   static unsigned tidC;          /* LP semaphore checker Thread */
#endif
   static unsigned tidH;          /* Hypocenter getter Thread */
   static unsigned tidW;          /* Waveform getter Thread */

   dLastEndTime      = 0.;
   iLPProcessing     = 0;
   iRunning          = 1;
   
/* Get config file name (format "Ms Ms.D")
   ***************************************/
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
      fprintf( stderr, "Ms: GetEwh() failed. Exiting.\n" );
      return -1;
   }

/* Specify logos of incoming waveforms and outgoing heartbeats
   ***********************************************************/
   getlogoW.instid = Ewh.GetThisInstId;
   getlogoW.mod    = Ewh.GetThisModId;
   getlogoW.type   = Ewh.TypeWaveform;
   hrtlogo.instid  = Ewh.MyInstId;
   hrtlogo.mod     = Gparm.ucMyModId;
   hrtlogo.type    = Ewh.TypeHeartBeat;

/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, Gparm.ucMyModId, 256, 1 );

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "Ms: Can't get my pid. Exiting.\n" );
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
      logit( "et", "Ms: Cannot allocate waveform buffer\n" );
      return -1;
   }
   WaveBufFilt = (char *) malloc( (size_t) InBufl );
   if ( WaveBufFilt == NULL )
   {
      logit( "et", "Ms: Cannot allocate waveform buffer2\n" );
      return -1;
   }

/* Point to header and data portions of waveform message
   *****************************************************/
   WaveHead  = (TRACE2_HEADER *) WaveBuf;
   WaveLong  = (int32_t *) (WaveBuf + sizeof (TRACE2_HEADER));
   WaveLongF = (int32_t *) (WaveBufFilt + sizeof (TRACE2_HEADER));
   WaveShort = (short *) (WaveBuf + sizeof (TRACE2_HEADER));

/* Read the station list and return the number of stations found.
   Allocate the station list array.
   **************************************************************/
   if ( ReadStationList( &StaArray, &Nsta, Gparm.szStaFile, Gparm.szStaDataFile,
                         Gparm.szResponseFile, MAX_STATIONS, 0 ) == -1 )
   {
      logit( "", "Ms: ReadStationList() failed. Exiting.\n" );
      free( WaveBuf );
      free( WaveBufFilt );
      return -1;
   }
   if ( Nsta == 0 )
   {
      logit( "et", "Ms: Empty station list. Exiting." );
      free( WaveBuf );
      free( WaveBufFilt );
      free( StaArray );
      return -1;
   }
   logit( "t", "Ms: Processing %d stations.\n", Nsta );
	  
/* Initialize P section of Sta array
   *********************************/
   for ( i=0; i<Nsta; i++ ) InitP( &StaArray[i] );

/* Log the station list
   ********************/
   LogStaList( StaArray, Nsta );
   
#ifdef _WINNT
/* Create event to share with another module; this event
   is used to trigger LP processing here.
   *****************************************************/
   hEventMs = CreateEvent( NULL,      /* Default security */
                           FALSE,     /* Auto-reset event */
                           FALSE,     /* Initially not set */
                           "Ms" );    /* Share with other modules */
   if ( hEventMs == NULL )            /* If event not created */
   {
      logit( "t", "failed to create Ms event" );
      free( WaveBuf );
      free( WaveBufFilt );
      free( StaArray );
      return -1;
   }
   
/* Create another event to share with other modules; this event signals 
   to the other module that the LP data file has been filled. 
   ********************************************************************/
   hEventMsDone = CreateEvent( NULL,        /* Default security */
                               FALSE,       /* Auto-reset event */
                               FALSE,       /* Initially not set */
                               "MsDone" );  /* Share with other modules */
   if ( hEventMsDone == NULL )              /* If event not created */
   {
      logit( "t", "failed to create MsDone event" );
      free( WaveBuf );
      free( WaveBufFilt );
      free( StaArray );
      CloseHandle( hEventMs );
      return -1;
   }
#endif
   
/* Attach to existing transport rings
   **********************************/
   tport_attach( &Gparm.InRegion,  Gparm.lInKey );
   tport_attach( &Gparm.PRegion,   Gparm.lPKey );
   tport_attach( &Gparm.HRegion,   Gparm.lHKey );

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
      logit( "et", "Ms: Error sending 1st heartbeat. Exiting." );
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.PRegion );
      tport_detach( &Gparm.HRegion );
      free( WaveBuf );
      free( WaveBufFilt );
      free( StaArray );
#ifdef _WINNT
      CloseHandle( hEventMs );
      CloseHandle( hEventMsDone );
#endif
      return 0;
   }
   
/* Create a mutex for protecting adjustments of StaArray
   *****************************************************/
   CreateSpecificMutex( &mutsem );

#ifdef _WINNT
/* Start the LP compute semaphore checker thread
   *********************************************/
   if ( StartThread( RTLPThread, 8192, &tidC ) == -1 )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.PRegion );
      tport_detach( &Gparm.HRegion );
      free( WaveBuf );
      free( WaveBufFilt );
      free( StaArray );
      CloseHandle( hEventMs );
      CloseHandle( hEventMsDone );
      logit( "et", "Error starting RTLPThread; exiting!\n" );
      return -1;
   }
#endif

/* Start the Hypocenter getter thread
   **********************************/
   if ( Gparm.iAutoStart == 1 ) /* Only use this thread if Ms is auto-started */
      if ( StartThread( HThread, 8192, &tidH ) == -1 )
      {
         tport_detach( &Gparm.InRegion );
         tport_detach( &Gparm.PRegion );
         tport_detach( &Gparm.HRegion );
         free( WaveBuf );
         free( WaveBufFilt );
         free( StaArray );
#ifdef _WINNT
         CloseHandle( hEventMs );
         CloseHandle( hEventMsDone );
#endif
         logit( "et", "Error starting H thread; exiting!\n" );
         return( -1 );
      }

/* Start the waveform getter tread
   *******************************/
   if ( StartThread( WThread, 8192, &tidW ) == -1 )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.PRegion );
      tport_detach( &Gparm.HRegion );
      free( WaveBuf );
      free( WaveBufFilt );
      free( StaArray );
#ifdef _WINNT
      CloseHandle( hEventMs );
      CloseHandle( hEventMsDone );
#endif
      logit( "et", "Error starting W thread; exiting!\n" );
      return -1;
   }

/* Stay here until one of the threads says to exit
   ***********************************************/
   while ( iRunning == 1 ) sleep_ew( 2000 );

/* Detach from the ring buffers
   ****************************/
   tport_detach( &Gparm.InRegion );
   tport_detach( &Gparm.PRegion );
   tport_detach( &Gparm.HRegion );
   free( WaveBuf );
   free( WaveBufFilt );
#ifdef _WINNT
   CloseHandle( hEventMs );
   CloseHandle( hEventMsDone );
#endif
   for ( i=0; i<Nsta; i++ ) 
   {
      free( StaArray[i].plRawCircBuff );
      free( StaArray[i].plFiltCircBuff );
   }
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
      fprintf( stderr, "Ms: Error getting MyInstId.\n" );
      return -1;
   }
   if ( GetInst( "INST_WILDCARD", &EwhVal->GetThisInstId ) != 0 )
   {
      fprintf( stderr, "Ms: Error getting GetThisInstId.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &EwhVal->GetThisModId ) != 0 )
   {
      fprintf( stderr, "Ms: Error getting GetThisModId.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &EwhVal->TypeHeartBeat ) != 0 )
   {
      fprintf( stderr, "Ms: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &EwhVal->TypeError ) != 0 )
   {
      fprintf( stderr, "Ms: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_PICKTWC", &EwhVal->TypePickTWC ) != 0 )
   {
      fprintf( stderr, "Ms: Error getting TYPE_PICKTWC.\n" );
      return -7;
   }
   if ( GetType( "TYPE_TRACEBUF2", &EwhVal->TypeWaveform ) != 0 )
   {
      fprintf( stderr, "Ms: Error getting TYPE_TRACEBUF.\n" );
      return -8;
   }
   if ( GetType( "TYPE_HYPOTWC", &EwhVal->TypeHypoTWC ) != 0 )
   {
      fprintf( stderr, "Ms: Error getting TYPE_HYPOTWC.\n" );
      return -9;
   }
   return 0;
}

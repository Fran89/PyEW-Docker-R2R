
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_ew.c,v 1.3 2002/05/16 16:59:42 patton Exp $
 *
 *    Revision history:
 *     $Log: pick_ew.c,v $
 *
 *     Revision 1.4  2013/05/07  nromeu
 *     Gets new message type TYPE_PROXIES 
 *     to report proxies computation
 * 
 *     Revision 1.3  2002/05/16 16:59:42  patton
 *     Made logit changes
 *
 *     Revision 1.2  2001/05/09 22:40:47  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or myPid.
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

      /*****************************************************************
       *                           pick_ew.c                           *
       *                                                               *
       *  This is the new Earthworm picker.  The program uses          *
       *  demultiplexed waveform data with header blocks consistent    *
       *  with those in the CSS format, used by Datascope.  This       *
       *  program can be used with analog or digital data sources.     *
       *                                                               *
       *  Written by Will Kohler, January, 1997                        *
       *  Modified to use SCNs instead of pin numbers. 3/20/98 WMK     *
       *                                                               *
       *  Parameter names:                                             *
       *                                                               *
       *  Old name   New name                                          *
       *  --------   --------                                          *
       *     i5       Itr1                                             *
       *     i6       MinSmallZC                                       *
       *     i7       MinBigZC                                         *
       *     i8       MinPeakSize                                      *
       *     c1       RawDataFilt                                      *
       *     c2       CharFuncFilt                                     *
       *     c3       StaFilt                                          *
       *     c4       LtaFilt                                          *
       *     c5       EventThresh                                      *
       *     c6       DeadSta                                          *
       *     c7       CodaTerm                                         *
       *     c8       AltCoda                                          *
       *     c9       PreEvent                                         *
       *     C4       RmavFilt                                         *
       *   MAXMINT    MaxMint                                          *
       *    EREFS     Erefs                                            *
       *                                                               *
       *****************************************************************/

/* Y2K changes: The new pick and coda formats are PICK2K and CODA2K.
   The PICK2K format contains a four-digit number for pick year.
   The CODA2K format contains the SNC of the coda. (CODA2 codas didn't
   contain SNC) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <swap.h>
#include "pick_ew.h"

/* Function prototypes
   *******************/
int  GetConfig( char *, GPARM * );
void LogConfig( GPARM * );
int  GetStaList( STATION **, int *, GPARM * );
void LogStaList( STATION *, int );
void PickRA( STATION *, char *, GPARM *, EWH * );
int  CompareSCNs( const void *, const void * );
int  Restart( STATION *, GPARM *, int, int );
void Interpolate( STATION *, char *, int );
int  GetEwh( EWH * );
void Sample( long, STATION *, double );


      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of picker configuration file         *
       ***********************************************************/

int main( int argc, char **argv )
{
   int           i;               /* Loop counter */
   STATION       *StaArray;       /* Station array */
   char          *WaveBuf;        /* Pointer to waveform buffer */
   TRACE_HEADER  *WaveHead;       /* Pointer to waveform header */
   long          *WaveLong;       /* Long pointer to waveform data */
   short         *WaveShort;      /* Short pointer to waveform data */
   long          MsgLen;          /* Size of retrieved message */
   MSG_LOGO      getlogo;         /* Logo of requested waveforms */
   MSG_LOGO      logo;            /* Logo of retrieved msg */
   MSG_LOGO      hrtlogo;         /* Logo of outgoing heartbeats */
   int           Nsta;            /* Number of stations in list */
   time_t        then;            /* Previous heartbeat time */
   long          InBufl;          /* Maximum message size in bytes */
   GPARM         Gparm;           /* Configuration file parameters */
   EWH           Ewh;             /* Parameters from earthworm.h */
   char          *configfile;     /* Pointer to name of config file */
   pid_t         myPid;           /* Process id of this process */

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: pick_ew <configfile>\n" );
      return -1;
   }
   configfile = argv[1];

/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, 0, 256, 1 );

/* Get parameters from the configuration files
   *******************************************/
   if ( GetConfig( configfile, &Gparm ) == -1 )
   {
      fprintf( stderr, "pick_ew: GetConfig() failed. Exiting.\n" );
      return -1;
   }

/* Look up info in the earthworm.h tables
   **************************************/
   if ( GetEwh( &Ewh ) < 0 )
   {
      fprintf( stderr, "pick_ew: GetEwh() failed. Exiting.\n" );
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

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "pick_ew: Can't get my pid. Exiting.\n" );
      return -1;
   }

/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm );

/* Allocate the waveform buffer
   ****************************/
   InBufl = MAX_TRACEBUF_SIZ*2 + sizeof(long)*(Gparm.MaxGap-1);
   WaveBuf = (char *) malloc( (size_t) InBufl );
   if ( WaveBuf == NULL )
   {
      logit( "et", "pick_ew: Cannot allocate waveform buffer\n" );
      return -1;
   }

/* Point to header and data portions of waveform message
   *****************************************************/
   WaveHead  = (TRACE_HEADER *) WaveBuf;
   WaveLong  = (long *) (WaveBuf + sizeof(TRACE_HEADER));
   WaveShort = (short *) (WaveBuf + sizeof(TRACE_HEADER));

/* Read the station list and return the number of stations found.
   Allocate the station list array.
   *************************************************************/
   if ( GetStaList( &StaArray, &Nsta, &Gparm ) == -1 )
   {
      fprintf( stderr, "pick_ew: GetStaList() failed. Exiting.\n" );
      return -1;
   }

   if ( Nsta == 0 )
   {
      logit( "et", "pick_ew: Empty station list. Exiting." );
      return -1;
   }

   logit( "t", "pick_ew: Picking %d station", Nsta );
   if ( Nsta != 1 ) logit( "", "s" );
   logit( "", ".\n" );

/* Sort the station list by SCN number
   ***********************************/
   qsort( StaArray, Nsta, sizeof(STATION), CompareSCNs );

/* Log the station list
   ********************/
   LogStaList( StaArray, Nsta );

/* Attach to existing transport rings
   **********************************/
   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_attach( &Gparm.InRegion,  Gparm.InKey );
      tport_attach( &Gparm.OutRegion, Gparm.OutKey );
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

/* Get the time when we start reading messages.
   This is for issuing heartbeats.
   *******************************************/
   time( &then );

/* Loop to read waveform messages and invoke the picker
   ****************************************************/
   while ( tport_getflag( &Gparm.InRegion ) != TERMINATE  &&
           tport_getflag( &Gparm.InRegion ) != myPid )
   {
      char    type[3];
      STATION key;              /* Key for binary search */
      STATION *Sta;             /* Pointer to the station being processed */
      int     rc;               /* Return code from tport_getmsg() */
      time_t  now;              /* Current time */
      double  GapSizeD;         /* Number of missing samples (double) */
      long    GapSize;          /* Number of missing samples (integer) */

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
         logit( "et", "pick_ew: Tracking error.\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "pick_ew: Got lapped on the ring.\n");

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "pick_ew: Gap in sequence numbers.\n");

      if ( rc == GET_MISS )
         logit( "et", "pick_ew: Missed messages.\n");

      if ( rc == GET_TOOBIG )
      {
         logit( "et", "pick_ew: Retrieved message too big (%d) for msg.\n",
                MsgLen );
         continue;
      }

/* If necessary, swap bytes in the message
   ***************************************/
      if ( WaveMsgMakeLocal( WaveHead ) < 0 )
      {
         logit( "et", "pick_ew: Unknown waveform type.\n" );
         continue;
      }

/* Look up SCN number in the station list
   **************************************/
      {
         int j;

         for ( j = 0; j < 5; j++ ) key.sta[j] = WaveHead->sta[j];
         key.sta[5] = '\0';

         for ( j = 0; j < 3; j++ ) key.chan[j] = WaveHead->chan[j];
         key.chan[3] = '\0';

         for ( j = 0; j < 2; j++ ) key.net[j]  = WaveHead->net[j];
         key.net[2] = '\0';
      }

      Sta = (STATION *) bsearch( &key, StaArray, Nsta, sizeof(STATION),
                                 CompareSCNs );

      if ( Sta == NULL )      /* SCN not found */
         continue;

/* Do this the first time we get a message with this SCN
   *****************************************************/
      if ( Sta->first == 1 )
      {
         Sta->endtime = WaveHead->endtime;
         Sta->first = 0;
         continue;
      }

/* If the samples are shorts, make them longs
   ******************************************/
      strcpy( type, WaveHead->datatype );

//logit("t", "tracebuf header packet: <%s.%s.%s> %s %f %.1f %d\n", 
//WaveHead->sta, WaveHead->chan, WaveHead->net, 
//WaveHead->datatype,WaveHead->starttime, WaveHead->samprate, WaveHead->nsamp );
//for ( i = 0; i < WaveHead->nsamp; i++ )
//logit("t", "data packet (tracebuf): %d \n",WaveLong[i] );

      if ( (strcmp(type,"i2")==0) || (strcmp(type,"s2")==0) )
      {
         for ( i = WaveHead->nsamp - 1; i > -1; i-- )
            WaveLong[i] = (long) WaveShort[i];
      }

/* Compute the number of samples since the end of the previous message.
   If (GapSize == 1), no data has been lost between messages.
   If (1 < GapSize <= Gparm.MaxGap), data will be interpolated.
   If (GapSize > Gparm.MaxGap), the picker will go into restart mode.
   *******************************************************************/
      GapSizeD = WaveHead->samprate * (WaveHead->starttime - Sta->endtime);

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
         int      lineLen;
         time_t   errTime;
         char     errmsg[80];
         MSG_LOGO logo;

         time( &errTime );
         sprintf( errmsg,
               "%d %d Found %4d sample gap. Restarting station %-5s%-2s%-3s\n",
               errTime, PK_RESTART, GapSize, Sta->sta, Sta->net, Sta->chan );
         lineLen = strlen( errmsg );
         logo.type   = Ewh.TypeError;
         logo.mod    = Gparm.MyModId;
         logo.instid = Ewh.MyInstId;
         tport_putmsg( &Gparm.OutRegion, &logo, lineLen, errmsg );

/*       logit( "t", "pick_ew: Restarting %-5s%-2s%-3s. GapSize = %d\n",
                  Sta->sta, Sta->net, Sta->chan, GapSize ); */
      }

/* For big gaps, enter restart mode. In restart mode, calculate
   STAs and LTAs without picking.  Start picking again after a
   specified number of samples has been processed.
   *************************************************************/
      if ( Restart( Sta, &Gparm, WaveHead->nsamp, GapSize ) )
      {
         for ( i = 0; i < WaveHead->nsamp; i++ )
			Sample( WaveLong[i], Sta, WaveHead->samprate );
      }
      else
         PickRA( Sta, WaveBuf, &Gparm, &Ewh );

/* Save time and amplitude of the end of the current message
   *********************************************************/
      Sta->enddata = WaveLong[WaveHead->nsamp - 1];
      Sta->endtime = WaveHead->endtime;

/* Send a heartbeat to the transport ring
   **************************************/
      time( &now );
      if ( (now - then) >= Gparm.HeartbeatInt )
      {
         int  lineLen;
         char line[40];

         then = now;

         sprintf( line, "%d %d\n", now, myPid );
         lineLen = strlen( line );

         if ( tport_putmsg( &Gparm.OutRegion, &hrtlogo, lineLen, line ) !=
              PUT_OK )
         {
            logit( "et", "pick_ew: Error sending heartbeat. Exiting." );
            break;
         }
      }
   }

/* Detach from the ring buffers
   ****************************/
   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.OutRegion );
   }
   else
      tport_detach( &Gparm.InRegion );

   logit( "t", "Termination requested. Exiting.\n" );
   return 0;
}


      /*******************************************************
       *                      GetEwh()                       *
       *                                                     *
       *      Get parameters from the earthworm.h file.      *
       *******************************************************/

int GetEwh( EWH *Ewh )
{
   if ( GetLocalInst( &Ewh->MyInstId ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting MyInstId.\n" );
      return -1;
   }

   if ( GetInst( "INST_WILDCARD", &Ewh->GetThisInstId ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting GetThisInstId.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &Ewh->GetThisModId ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting GetThisModId.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_PICK2K", &Ewh->TypePick2k ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TypePick2k.\n" );
      return -6;
   }
   if ( GetType( "TYPE_CODA2K", &Ewh->TypeCoda2k ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TypeCoda2k.\n" );
      return -7;
   }
   if ( GetType( "TYPE_PROXIES", &Ewh->TypeProxies ) != 0 )	/***/ // NR
   {
      fprintf( stderr, "pick_ew: Error getting TypeProxies.\n" );
      return -8;
   }
   if ( GetType( "TYPE_TRACEBUF", &Ewh->TypeWaveform ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TYPE_TRACEBUF.\n" );
      return -9;
   }
   return 0;
}


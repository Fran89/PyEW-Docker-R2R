
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_ew.c 6233 2015-02-17 19:55:38Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.14  2007/12/16 19:18:43  paulf
 *     fixed an improper use of long for 4 byte sample data, some OS have long as 8bytes.
 *
 *     Revision 1.13  2007/03/13 14:42:08  paulf
 *     fixed another time_t printing
 *
 *     Revision 1.12  2007/02/26 14:01:10  paulf
 *     fixed a heartbeat fix for long casting of time_t
 *
 *     Revision 1.11  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.10  2006/09/20 22:44:07  dietz
 *     Modified to be able to process multiple "StaFile" commands for setting
 *     per-channel picking parameters.
 *
 *     Revision 1.9  2006/03/17 18:35:57  dietz
 *     Added message logo and sequence number information to transport error
 *     logging.
 *
 *     Revision 1.8  2005/04/08 23:57:19  dietz
 *     Added new config command "GetLogo" so pick_ew can select which logos
 *     to process. If no "GetLogo" commands are included, the default behavior
 *     is to process all TYPE_TRACEBUF and TYPE_TRACEBUF2 messages in InRing.
 *
 *     Revision 1.7  2004/05/17 19:51:59  kohler
 *     Fixed dangling-else bug.  WMK
 *
 *     Revision 1.6  2004/04/30 20:45:26  dietz
 *     oops, forgot a semicolon
 *
 *     Revision 1.5  2004/04/30 20:43:36  dietz
 *     made TypeTracebuf2 the primary logo msgtype
 *
 *     Revision 1.4  2004/04/29 22:44:51  kohler
 *     Pick_ew now produces new TYPE_PICK_SCNL and TYPE_CODA_SCNL messages.
 *     The station list file now contains SCNLs, rather than SCNs.
 *     Input waveform messages may be of either TYPE_TRACEBUF or TYPE_TRACEBUF2.
 *     If the input waveform message is of TYPE_TRACEBUF (without a location code),
 *     the location code is assumed to be "--".  WMK 4/29/04
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
#include <trheadconv.h>
#include "pick_ew.h"

/* Function prototypes
   *******************/
int  GetConfig( char *, GPARM * );
void LogConfig( GPARM * );
int  GetStaList( STATION **, int *, GPARM * );
void LogStaList( STATION *, int );
void PickRA( STATION *, char *, GPARM *, EWH * );
int  CompareSCNL( const void *, const void * );
int  Restart( STATION *, GPARM *, int, int );
void Interpolate( STATION *, char *, int );
int  GetEwh( EWH * );
void Sample( int, STATION * );


/* version introduced with 1.0.1  */
/* version 1.0.2 2012-12-07 NoCodaHorizontal added */
/* version 1.0.3 2013-02-02 more debug logging added */
/* version 1.0.4 2014-05-27 changed to use consistent tport commands */
/* version 1.0.5 2014-06-04 no longer rolls over pick-id at 999999, only at MAX INT */
/* version 1.0.6 2015-02-17 made DeadSta check be ignored if value is set <= 0.0 */
#define PICKEW_VERSION "1.0.6 2015-02-17"
   
      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of picker configuration file         *
       ***********************************************************/

int main( int argc, char **argv )
{
   int           i;                /* Loop counter */
   STATION       *StaArray = NULL; /* Station array */
   char          *TraceBuf;        /* Pointer to waveform buffer */
   TRACE_HEADER  *TraceHead;       /* Pointer to trace header w/o loc code */
   TRACE2_HEADER *Trace2Head;      /* Pointer to header with loc code */
   int          *TraceLong;       /* Long pointer to waveform data */
   short         *TraceShort;      /* Short pointer to waveform data */
   long          MsgLen;           /* Size of retrieved message */
   MSG_LOGO      logo;             /* Logo of retrieved msg */
   MSG_LOGO      hrtlogo;          /* Logo of outgoing heartbeats */
   int           Nsta = 0;         /* Number of stations in list */
   time_t        then;             /* Previous heartbeat time */
   long          InBufl;           /* Maximum message size in bytes */
   GPARM         Gparm;            /* Configuration file parameters */
   EWH           Ewh;              /* Parameters from earthworm.h */
   char          *configfile;      /* Pointer to name of config file */
   pid_t         myPid;            /* Process id of this process */
   unsigned char seq;        /* msg sequence number from tport_copyfrom() */

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: pick_ew <configfile>\n" );
      fprintf( stderr, "Version: %s\n", PICKEW_VERSION);
      return -1;
   }
   configfile = argv[1];

/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, 0, 256, 1 );
   logit( "t", "pick_ew: Starting Version:%s\n", PICKEW_VERSION );

/* Get parameters from the configuration files
   *******************************************/
   if ( GetConfig( configfile, &Gparm ) == -1 )
   {
      logit( "e", "pick_ew: GetConfig() failed. Exiting.\n" );
      return -1;
   }

/* Look up info in the earthworm.h tables
   **************************************/
   if ( GetEwh( &Ewh ) < 0 )
   {
      logit( "e", "pick_ew: GetEwh() failed. Exiting.\n" );
      return -1;
   }

/* Specify logos of incoming waveforms and outgoing heartbeats
   ***********************************************************/
   if( Gparm.nGetLogo == 0 ) 
   {
      Gparm.nGetLogo = 2;
      Gparm.GetLogo  = (MSG_LOGO *) calloc( Gparm.nGetLogo, sizeof(MSG_LOGO) );
      if( Gparm.GetLogo == NULL ) {
         logit( "e", "pick_ew: Error allocating space for GetLogo. Exiting\n" );
         return -1;
      }
      Gparm.GetLogo[0].instid = Ewh.InstIdWild;
      Gparm.GetLogo[0].mod    = Ewh.ModIdWild;
      Gparm.GetLogo[0].type   = Ewh.TypeTracebuf2;

      Gparm.GetLogo[1].instid = Ewh.InstIdWild;
      Gparm.GetLogo[1].mod    = Ewh.ModIdWild;
      Gparm.GetLogo[1].type   = Ewh.TypeTracebuf;
   }

   hrtlogo.instid = Ewh.MyInstId;
   hrtlogo.mod    = Gparm.MyModId;
   hrtlogo.type   = Ewh.TypeHeartBeat;

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "pick_ew: Can't get my pid. Exiting.\n" );
      free( Gparm.GetLogo );
      free( Gparm.StaFile );
      return -1;
   }

/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm );

/* Allocate the waveform buffer
   ****************************/
   InBufl = MAX_TRACEBUF_SIZ*2 + sizeof(int)*(Gparm.MaxGap-1);
   TraceBuf = (char *) malloc( (size_t) InBufl );
   if ( TraceBuf == NULL )
   {
      logit( "et", "pick_ew: Cannot allocate waveform buffer\n" );
      free( Gparm.GetLogo );
      free( Gparm.StaFile );
      return -1;
   }

/* Point to header and data portions of waveform message
   *****************************************************/
   TraceHead  = (TRACE_HEADER *)TraceBuf;
   Trace2Head = (TRACE2_HEADER *)TraceBuf;
   TraceLong  = (int *) (TraceBuf + sizeof(TRACE_HEADER));
   TraceShort = (short *) (TraceBuf + sizeof(TRACE_HEADER));

/* Read the station list and return the number of stations found.
   Allocate the station list array.
   *************************************************************/
   if ( GetStaList( &StaArray, &Nsta, &Gparm ) == -1 )
   {
      logit( "e", "pick_ew: GetStaList() failed. Exiting.\n" );
      free( Gparm.GetLogo );
      free( Gparm.StaFile );
      free( StaArray );
      return -1;
   }

   if ( Nsta == 0 )
   {
      logit( "et", "pick_ew: Empty station list(s). Exiting." );
      free( Gparm.GetLogo );
      free( Gparm.StaFile );
      free( StaArray );
      return -1;
   }

/* Sort the station list by SCNL
   *****************************/
   qsort( StaArray, Nsta, sizeof(STATION), CompareSCNL );

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
   while ( tport_copyfrom( &Gparm.InRegion, Gparm.GetLogo, (short)Gparm.nGetLogo, 
                         &logo, &MsgLen, TraceBuf, MAX_TRACEBUF_SIZ, &seq) != 
                         GET_NONE );

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
      int     rc;               /* Return code from tport_copyfrom() */
      time_t  now;              /* Current time */
      double  GapSizeD;         /* Number of missing samples (double) */
      int    GapSize;          /* Number of missing samples (integer) */
      int wave_swap_return;	/* return from WaveMsg2MakeLocal */

/* Get tracebuf or tracebuf2 message from ring
   *******************************************/
      rc = tport_copyfrom( &Gparm.InRegion, Gparm.GetLogo, (short)Gparm.nGetLogo, 
                           &logo, &MsgLen, TraceBuf, MAX_TRACEBUF_SIZ, &seq );

      if ( rc == GET_NONE )
      {
         sleep_ew( 100 );
         continue;
      }

      if ( rc == GET_NOTRACK )
         logit( "et", "pick_ew: Tracking error (NTRACK_GET exceeded)\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "pick_ew: Missed msgs (lapped on ring) "
                "before i:%d m:%d t:%d seq:%d\n",
                (int)logo.instid, (int)logo.mod, (int)logo.type, (int)seq );

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "pick_ew: Gap in sequence# before i:%d m:%d t:%d seq:%d\n",
                (int)logo.instid, (int)logo.mod, (int)logo.type, (int)seq );

      if ( rc == GET_TOOBIG )
      {
         logit( "et", "pick_ew: Retrieved msg is too big: i:%d m:%d t:%d len:%d\n",
                (int)logo.instid, (int)logo.mod, (int)logo.type, MsgLen );
         continue;
      }

/* If necessary, swap bytes in tracebuf message
   ********************************************/
      if ( logo.type == Ewh.TypeTracebuf )
      {
         if ( (wave_swap_return = WaveMsgMakeLocal( TraceHead )) < 0 )
         {
            logit( "et", "pick_ew: WaveMsgMakeLocal() error.\n" );
            continue;
         }
      }
      else
         if ( (wave_swap_return = WaveMsg2MakeLocal( Trace2Head )) < 0 )
         {
            logit( "et", "pick_ew: WaveMsg2MakeLocal error. %s.%s.%s.%s error=%d\n",
		Trace2Head->sta, Trace2Head->net, Trace2Head->chan, Trace2Head->loc, wave_swap_return );
            continue;
         }

/* Convert TYPE_TRACEBUF messages to TYPE_TRACEBUF2
   ************************************************/
      if ( logo.type == Ewh.TypeTracebuf )
         Trace2Head = TrHeadConv( TraceHead );

/* Look up SCNL number in the station list
   ***************************************/
      {
         int j;
         for ( j = 0; j < 5; j++ ) key.sta[j]  = Trace2Head->sta[j];
         key.sta[5] = '\0';
         for ( j = 0; j < 3; j++ ) key.chan[j] = Trace2Head->chan[j];
         key.chan[3] = '\0';
         for ( j = 0; j < 2; j++ ) key.net[j]  = Trace2Head->net[j];
         key.net[2] = '\0';
         for ( j = 0; j < 2; j++ ) key.loc[j]  = Trace2Head->loc[j];
         key.loc[2] = '\0';
      }

      Sta = (STATION *) bsearch( &key, StaArray, Nsta, sizeof(STATION),
                                 CompareSCNL );

      if ( Sta == NULL )      /* SCNL not found */
         continue;

/* Do this the first time we get a message with this SCNL
   ******************************************************/
      if ( Sta->first == 1 )
      {
         Sta->endtime = Trace2Head->endtime;
         Sta->first = 0;
         continue;
      }

/* If the samples are shorts, make them longs (actually just int's now since long could be 8 bytes!)
   ******************************************/
      strcpy( type, Trace2Head->datatype );

      if ( (strcmp(type,"i2")==0) || (strcmp(type,"s2")==0) )
      {
         for ( i = Trace2Head->nsamp - 1; i > -1; i-- )
            TraceLong[i] = (int)TraceShort[i];
      }

/* Compute the number of samples since the end of the previous message.
   If (GapSize == 1), no data has been lost between messages.
   If (1 < GapSize <= Gparm.MaxGap), data will be interpolated.
   If (GapSize > Gparm.MaxGap), the picker will go into restart mode.
   *******************************************************************/
      GapSizeD = Trace2Head->samprate * (Trace2Head->starttime - Sta->endtime);

      if ( GapSizeD < 0. )          /* Invalid. Time going backwards. */
         GapSize = 0;
      else
         GapSize  = (int) (GapSizeD + 0.5);

/* Interpolate missing samples and prepend them to the current message
   *******************************************************************/
      if ( (GapSize > 1) && (GapSize <= Gparm.MaxGap) )
         Interpolate( Sta, TraceBuf, GapSize );

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
               "%ld %d Found %4d sample gap. Restarting channel %s.%s.%s.%s\n",
               (long) errTime, PK_RESTART, GapSize, Sta->sta, Sta->chan, Sta->net, Sta->loc );
         lineLen = strlen( errmsg );
         logo.type   = Ewh.TypeError;
         logo.mod    = Gparm.MyModId;
         logo.instid = Ewh.MyInstId;
         tport_putmsg( &Gparm.OutRegion, &logo, lineLen, errmsg );
      }

/* For big gaps, enter restart mode. In restart mode, calculate
   STAs and LTAs without picking.  Start picking again after a
   specified number of samples has been processed.
   *************************************************************/
      if ( Restart( Sta, &Gparm, Trace2Head->nsamp, GapSize ) )
      {
         for ( i = 0; i < Trace2Head->nsamp; i++ )
            Sample( TraceLong[i], Sta );
      }
      else
         PickRA( Sta, TraceBuf, &Gparm, &Ewh );

/* Save time and amplitude of the end of the current message
   *********************************************************/
      Sta->enddata = TraceLong[Trace2Head->nsamp - 1];
      Sta->endtime = Trace2Head->endtime;

/* Send a heartbeat to the transport ring
   **************************************/
      time( &now );
      if ( (now - then) >= Gparm.HeartbeatInt )
      {
         int  lineLen;
         char line[40];

         then = now;

         sprintf( line, "%ld %d\n", (long) now, (int) myPid );
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
   free( Gparm.GetLogo );
   free( Gparm.StaFile );
   free( StaArray );
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
      logit( "e", "pick_ew: Error getting MyInstId.\n" );
      return -1;
   }

   if ( GetInst( "INST_WILDCARD", &Ewh->InstIdWild ) != 0 )
   {
      logit( "e", "pick_ew: Error getting InstIdWild.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &Ewh->ModIdWild ) != 0 )
   {
      logit( "e", "pick_ew: Error getting ModIdWild.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      logit( "e", "pick_ew: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      logit( "e", "pick_ew: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_PICK_SCNL", &Ewh->TypePickScnl ) != 0 )
   {
      logit( "e", "pick_ew: Error getting TypePickScnl.\n" );
      return -6;
   }
   if ( GetType( "TYPE_CODA_SCNL", &Ewh->TypeCodaScnl ) != 0 )
   {
      logit( "e", "pick_ew: Error getting TypeCodaScnl.\n" );
      return -7;
   }
   if ( GetType( "TYPE_TRACEBUF", &Ewh->TypeTracebuf ) != 0 )
   {
      logit( "e", "pick_ew: Error getting TYPE_TRACEBUF.\n" );
      return -8;
   }
   if ( GetType( "TYPE_TRACEBUF2", &Ewh->TypeTracebuf2 ) != 0 )
   {
      logit( "e", "pick_ew: Error getting TYPE_TRACEBUF2.\n" );
      return -9;
   }
   return 0;
}


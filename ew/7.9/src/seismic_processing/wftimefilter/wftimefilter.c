/*
 *   THIS FILE IS UNDER CVS - 
 *   DO NOT MODIFY UNLESS YOU HAVE CHECKED IT OUT.
 *
 *    $Id: wftimefilter.c 2710 2007-02-26 13:44:40Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.6  2007/02/14 00:52:32  dietz
 *     Modified samplerate check to allow for a bit of slop in case a datasource
 *     is using actual sample rate instead of nominal sample rate.
 *
 *     Revision 1.5  2007/02/14 00:17:57  dietz
 *     Reworded logic in if()s that limit logging of bad packets & warnings.
 *
 *     Revision 1.4  2007/02/13 22:11:34  dietz
 *     Added check on number of samples. Reject packet if nsamp<=0.
 *
 *     Revision 1.3  2006/03/27 17:54:05  davek
 *     Updated to include optional logging and rejection of packets with
 *      unexpected sample rates.
 *
 *     Revision 1.2  2005/06/10 23:17:51  dietz
 *     Modified to pay attention to WaveMsg2MakeLocal return code and to
 *     ignore the packet if it's non-zero.
 *
 *     Revision 1.1  2005/05/10 22:54:34  dietz
 *     New module to filter out time overlaps and bogus future timestamps
 *     in waveform data.
 *
 */

/*
 * wftimefilter.c
 *
 * Reads waveform data (compressed or uncompressed) from one 
 * transport ring and writes it to another ring, filtering
 * out time overlaps and bogus future timestamped data.   
 * Timestamp checks are done for each channel independently.
 * Check for bogus future timestamps requires that the system clock 
 * is set to network time!.
 *
 * Does not alter the contents of the message in any way.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>
#include <trheadconv.h>

#define ABS(X) (((X) >= 0) ? (X) : -(X))

/* Globals
 *********/
static SHM_INFO  InRegion;    /* shared memory region to use for input  */
static SHM_INFO  OutRegion;   /* shared memory region to use for output */
static pid_t	 MyPid;       /* Our process id is sent with heartbeat  */

/* Things to read or derive from configuration file
 **************************************************/
static int     LogSwitch;              /* 0 if no logfile should be written      */
static long    HeartbeatInt;           /* seconds between heartbeats             */
static long    MaxMessageSize = MAX_TRACEBUF_SIZ; /* size (bytes) of largest msg */
static MSG_LOGO *GetLogo = NULL;       /* logo(s) to get from shared memory      */
static short   nLogo     = 0;          /* # logos we're configured to get        */
static double  LogGap    = 0.0;        /* minimum gap duration (sec) to log      */
static double  TimeJumpTolerance=-1.;  /* used in comparing packet time to       */
                                       /* system time when looking for bogus pkt */
                                       /* timetamps. -1 means don't do check.    */
static int UseOriginalInstId = 0;  /* 0=use wftimefilter's own instid on output  */
                                   /* non-zero=use original instid on output     */
static int UseOriginalModId  = 0;  /* 0=use wftimefilter's own moduleId on output*/
                                   /* non-zero=use original moduleId on output   */

static time_t timeLastLoggingOfBadChannelsReInit =0;
static int    bLimitLoggingOfBadChannels=0;
static int    iMaxBadPacketsToLogPerChannelPerDay=3;
static int    iMaxWarningsToLogPerChannelPerDay=3;
static int    bAllowChangesInSampleRate=1;
static int    bLogSummary=0;
static int    bLogOnlyChannelsWithProblems=0;
static int    bStatusBadPackets=0;

/* Store next expected timestamp for each SCNL
**********************************************/
typedef struct _WFHISTORY {
   char     sta[TRACE2_STA_LEN];   /* sta code we're tracking            */
   char     cmp[TRACE2_CHAN_LEN];  /* cmp code we're tracking            */
   char     net[TRACE2_NET_LEN];   /* net code we're tracking            */
   char     loc[TRACE2_NET_LEN];   /* loc code we're tracking            */
   double   texp;                  /* expected time of next trace data   */
   double   samprate;              /* samples/second for this channel    */
   double   halfinterval;          /* half of one sample interval (s)    */
   int      badpackets;            /* disallowed packets                 */
   int      warnings;              /* warnings (Gap)                     */
   int      goodpackets;           /* allowed packets                    */
} WFHISTORY;

static WFHISTORY *WfHist;          /* dynamically allocated list to track */
                                   /*   waveform timestamps per SCNL      */
static int        nHist = 0;       /* number of channels in WfHist        */
static WFHISTORY  WfKey;           /* key for looking up SCNLs in WfHist  */
  

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;    /* key of transport ring for input     */
static long          OutRingKey;   /* key of transport ring for output    */
static unsigned char InstId;       /* local installation id               */
static unsigned char MyModId;      /* Module Id for this program          */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeTrace;
static unsigned char TypeTrace2;
static unsigned char TypeCompress;
static unsigned char TypeCompress2;

/* Error messages used by wftfilter
 *********************************/
#define  ERR_MISSGAP       0   /* sequence gap in transport ring         */
#define  ERR_MISSLAP       1   /* missed messages in transport ring      */
#define  ERR_TOOBIG        2   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       3   /* msg retreived; tracking limit exceeded */
#define  ERR_CHANHIST      4   /* couldn't add SCNL to channel history   */
#define  ERR_FILTERPACKET  5   /* filtered atleast one bad packet today  */
static char Text[256];         /* string for log/error messages          */

/* Functions in this source file
 *******************************/
int        wftfilter_msg( TRACE2_HEADER *thd );
WFHISTORY *wftfilter_findchan( TRACE2_HEADER *thd );
WFHISTORY *wftfilter_addchan( TRACE2_HEADER *thd );
void       wftfilter_lookup( void );
void       wftfilter_config( char * );
void       wftfilter_logparm( void );
void       wftfilter_status( unsigned char, short, char * );
int        wftfilter_compare( const void *s1, const void *s2 );
int        wftfilter_managesummary(time_t timeNow);

int main( int argc, char **argv )
{
   char         *msgbuf;           /* buffer for msgs from ring     */
   TracePacket   wf;               /* trace packet                  */
   time_t        timeNow;          /* current time                  */
   time_t        timeLastBeat;     /* time last heartbeat was sent  */
   long          recsize;          /* size of retrieved message     */
   MSG_LOGO      reclogo;          /* logo of retrieved message     */
   MSG_LOGO      putlogo;          /* logo to use putting message into ring */
   int           res;
   unsigned char seq;
   WFHISTORY    *tlist;

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 1024, 1 );

/* Check command line arguments
 ******************************/
   if( argc != 2 )
   {
      logit( "e", "Usage: wftimefilter <configfile>\n" );
      exit( 0 );
   }

/* Look up important info from earthworm*d tables
 ************************************************/
   wftfilter_lookup();

/* Read the configuration file(s)
 ********************************/
   wftfilter_config( argv[1] );

/*  Set logit to LogSwitch read from configfile
 **********************************************/
   logit_init( argv[1], 0, 1024, LogSwitch );
   logit( "" , "wftimefilter: Read command file <%s>\n", argv[1] );
   wftfilter_logparm();

/* Check for different in/out rings
 **********************************/
   if( InRingKey==OutRingKey ) 
   {
      logit ("e", "wftimefilter: InRing and OutRing must be different;"
                  " exiting!\n");
      free( GetLogo );
      exit( -1 );
   }

/* Get our own process ID for restart purposes
 *********************************************/
   if( (MyPid = getpid()) == -1 )
   {
      logit ("e", "wftimefilter: Call to getpid failed. Exiting.\n");
      free( GetLogo );
      exit( -1 );
   }

/* Allocate the message input buffer 
 ***********************************/
  if ( !( msgbuf = (char *) malloc( (size_t)MaxMessageSize ) ) )
  {
      logit( "e", "wftimefilter: failed to allocate %d bytes"
             " for message buffer; exiting!\n", MaxMessageSize );
      free( GetLogo );
      exit( -1 );
  }

/* Initialize outgoing logo
 **************************/
   putlogo.instid = InstId;
   putlogo.mod    = MyModId;

/* Attach to shared memory rings
 *******************************/
   tport_attach( &InRegion, InRingKey );
   logit( "", "wftimefilter: Attached to public memory region: %ld\n",
          InRingKey );
   tport_attach( &OutRegion, OutRingKey );
   logit( "", "wftimefilter: Attached to public memory region: %ld\n",
          OutRingKey );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartbeatInt - 1;

/* Flush the incoming transport ring on startup
 **********************************************/ 
   while( tport_copyfrom( &InRegion, GetLogo, nLogo,  &reclogo,
          &recsize, msgbuf, MaxMessageSize, &seq ) != GET_NONE );

/*----------------------- setup done; start main loop -------------------------*/

  while( tport_getflag( &InRegion ) != TERMINATE  &&
         tport_getflag( &InRegion ) != MyPid )
  {
     /* send wftimefilter's heartbeat
      *******************************/
        if( HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= HeartbeatInt )
        {
            timeLastBeat = timeNow;
            wftfilter_status( TypeHeartBeat, 0, "" );             
        }

     /* manage logging, status, and general reporting of bad packets */
        wftfilter_managesummary(timeNow);

     /* Get msg & check the return code from transport
      ************************************************/
        res = tport_copyfrom( &InRegion, GetLogo, nLogo, &reclogo, 
                              &recsize, msgbuf, MaxMessageSize, &seq );

        switch( res )
        {
        case GET_OK:      /* got a message, no errors or warnings         */
             break;

        case GET_NONE:    /* no messages of interest, check again later   */
             sleep_ew(50); /* milliseconds */
             continue;

        case GET_NOTRACK: /* got a msg, but can't tell if any were missed */
             sprintf( Text,
                     "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                      reclogo.instid, reclogo.mod, reclogo.type );
             wftfilter_status( TypeError, ERR_NOTRACK, Text );
             break;

        case GET_MISS_LAPPED:     /* got a msg, but also missed lots      */
             sprintf( Text,
                     "Missed msg(s) from logo (i%u m%u t%u)",
                      reclogo.instid, reclogo.mod, reclogo.type );
             wftfilter_status( TypeError, ERR_MISSLAP, Text );
             break;

        case GET_MISS_SEQGAP:     /* got a msg, but seq gap               */
             sprintf( Text,
                     "Saw sequence# gap for logo (i%u m%u t%u s%u)",
                      reclogo.instid, reclogo.mod, reclogo.type, seq );
             wftfilter_status( TypeError, ERR_MISSGAP, Text );
             break;

       case GET_TOOBIG:  /* next message was too big, resize buffer      */
             sprintf( Text,
                     "Retrieved msg[%ld] (i%u m%u t%u) too big for msgbuf[%ld]",
                      recsize, reclogo.instid, reclogo.mod, reclogo.type,
                      MaxMessageSize );
             wftfilter_status( TypeError, ERR_TOOBIG, Text );
             continue;

       default:         /* Unknown result                                */
             sprintf( Text, "Unknown tport_copyfrom result:%d", res );
             wftfilter_status( TypeError, ERR_TOOBIG, Text );
             continue;
       }

    /* Prepare info to pass to the filter.
     * Make working copy of header, SCNLize it, put in local byte order
     ******************************************************************/
       memcpy( wf.msg, msgbuf, sizeof(TRACE2_HEADER) );  
       if     ( reclogo.type == TypeTrace    ) TrHeadConv( &(wf.trh) );
       else if( reclogo.type == TypeCompress ) TrHeadConv( &(wf.trh) );
       if( (res = WaveMsg2MakeLocal(&(wf.trh2))) != 0 )
       {
          if( res == -1 ) 
          {
             logit("et","%s.%s.%s.%s unknown datatype reported by WaveMsg2MakeLocal; "
                   "rejecting packet\n", 
                    wf.trh2.sta, wf.trh2.chan, wf.trh2.net, wf.trh2.loc );
          }
          if( res == -2 ) 
          {
             logit("et","%s.%s.%s.%s bad header reported by WaveMsg2MakeLocal; "
                   "rejecting packet\n", 
                    wf.trh2.sta, wf.trh2.chan, wf.trh2.net, wf.trh2.loc );
          }


          /* See if this channel is already in the list
          ********************************************/
          tlist = wftfilter_findchan( &wf.trh2 );
          if(tlist)
            tlist->badpackets++;
          
          continue;       
       }

    /* Decide if we should pass this message on or not
     *************************************************/
       if( wftfilter_msg( &(wf.trh2) ) == FALSE ) continue;

       if( UseOriginalInstId ) putlogo.instid = reclogo.instid;
       if( UseOriginalModId  ) putlogo.mod    = reclogo.mod;
       putlogo.type = reclogo.type;

       if( tport_putmsg( &OutRegion, &putlogo, recsize, msgbuf ) != PUT_OK )
       {
          logit("et","wftimefilter: Error writing %d-byte msg to ring; "
                     "original logo (i%u m%u t%u)\n", recsize,
                      reclogo.instid, reclogo.mod, reclogo.type );
       }
   }

/*-----------------------------end of main loop-------------------------------*/

/* free allocated memory */
   free( GetLogo );
   free( msgbuf  );
   free( WfHist  );  

/* detach from shared memory */
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
           
/* write a termination msg to log file */
   logit( "t", "wftimefilter: Termination requested; exiting!\n" );
   fflush( stdout );
   return( 0 );
}


/******************************************************************************
 *  wftfilter_msg()  Look at the new message and decide if we should pass it  *
 ******************************************************************************/
int wftfilter_msg( TRACE2_HEADER *thd )
{
   WFHISTORY *wf = (WFHISTORY *)NULL;
   double     dt = 0; 

/* Find/add channel in list
 **************************/
   wf = wftfilter_addchan( thd );

   if( wf == (WFHISTORY *)NULL ) 
   {
      sprintf( Text, "Error adding %s.%s.%s.%s to channel history; "
              "can only check timestamp against system clock!\n",
               thd->sta, thd->chan, thd->net, thd->loc );
      wftfilter_status( TypeError, ERR_CHANHIST, Text );
      WfKey.texp         = 0.0;               /* allow validation of timestamp */
      WfKey.samprate     = thd->samprate;     /* using the Key as the "history */
      WfKey.halfinterval = 0.5/thd->samprate; /* for this channel              */
      wf = &WfKey;
   }

/* Reject packets with bad nsample value
 ***************************************/
   if( thd->nsamp <= 0 )
   {
      logit("et","%s.%s.%s.%s rejecting packet with bad nsamp value: %d"
            " (starttime %.3lf)\n",
            wf->sta, wf->cmp, wf->net, wf->loc, thd->nsamp, 
            thd->starttime );
      wf->badpackets++;
      return( FALSE );
   }

/* If timestamp matches expected time, use it
 ********************************************/
   dt = thd->starttime - wf->texp;
   if( ABS(dt) <= wf->halfinterval )
   {
      /* normal case; nothing to do */ 
   }

/* Or if there's a time overlap, skip it
 ***************************************/
   else if( dt < 0.0 ) 
   {
   /* log this packet unless we've hit the max threshold for this channel */
      if(!bLimitLoggingOfBadChannels || wf->badpackets < iMaxBadPacketsToLogPerChannelPerDay)
         logit("et","%s.%s.%s.%s %.3lf %.3lf s overlap detected; "
               "rejecting out-of-sequence packet (starttime %.3lf)\n",
               wf->sta, wf->cmp, wf->net, wf->loc, wf->texp, ABS(dt),
               thd->starttime );
      wf->badpackets++;
      if(bLimitLoggingOfBadChannels && wf->badpackets == iMaxBadPacketsToLogPerChannelPerDay)
         logit("et","%s.%s.%s.%s too many bad packets. Further problems will not be logged.\n",
               wf->sta, wf->cmp, wf->net, wf->loc);

      return( FALSE );
   }

/* Otherwise, there's a forward time jump.
 * Check timestamp against system clock before passing it.
 * If starttime is later than system-clock (with slop), it's
 * an "abnormal" time jump (bogus timestamp -> ignore packet!)
 *************************************************************/
   else 
   {
      double dtsys;
      dtsys = thd->starttime - (double)time(NULL);

      if( dtsys >= TimeJumpTolerance &&   /* if it's a BIG time jump and         */
          TimeJumpTolerance != -1.0     ) /* bogus timestamp checking is enabled */ 
      {
        if(!bLimitLoggingOfBadChannels || wf->badpackets < iMaxBadPacketsToLogPerChannelPerDay)
        {
          if( wf->texp == 0.0 ) {          /* it's the 1st pkt for this SCNL */
            logit("et","%s.%s.%s.%s dtsys %.0lf s; "
                  "rejecting packet with probable bogus starttime %.3lf\n",
                  wf->sta, wf->cmp, wf->net, wf->loc, 
                  dtsys, thd->starttime );
          }
          else {          
            logit("et","%s.%s.%s.%s %.3lf %.3lf s gap detected; "
                  "dtsys %.0lf s; rejecting packet with probable bogus starttime %.3lf\n",
                  wf->sta, wf->cmp, wf->net, wf->loc, 
                  wf->texp, dt, dtsys, thd->starttime );
          }
        }
        wf->badpackets++;
        if(bLimitLoggingOfBadChannels && wf->badpackets == iMaxBadPacketsToLogPerChannelPerDay)
          logit("et","%s.%s.%s.%s too many bad packets. Further problems will not be logged.\n",
                wf->sta, wf->cmp, wf->net, wf->loc);
        return( FALSE );
      }
      if( dt >= LogGap    &&     /* it's a big enough gap to log and  */
          wf->texp != 0.0    )   /* it's not 1st packet for this SCNL */
      {
        if(!bLimitLoggingOfBadChannels || wf->warnings < iMaxWarningsToLogPerChannelPerDay)
           logit("et","%s.%s.%s.%s %.3lf %.3lf s gap detected\n",
                 wf->sta, wf->cmp, wf->net, wf->loc, wf->texp, dt );
        wf->warnings++;
        if(bLimitLoggingOfBadChannels && wf->warnings == iMaxWarningsToLogPerChannelPerDay)
          logit("et","%s.%s.%s.%s too many warnings. Further warnings will not be logged.\n",
                wf->sta, wf->cmp, wf->net, wf->loc);
      }
   }

/* Check for and record changes (allowing a bit of slop) in sampling rate
 ************************************************************************/
   if( ABS(thd->samprate - wf->samprate) > 0.5 ) 
   {
      if(bAllowChangesInSampleRate)
      {
        if(!bLimitLoggingOfBadChannels || wf->warnings < iMaxWarningsToLogPerChannelPerDay)
          logit("et","%s.%s.%s.%s samplerate changed from %.1lf to %.1lf; packet passed.\n",
                wf->sta, wf->cmp, wf->net, wf->loc, 
                wf->samprate, thd->samprate );
        wf->warnings++;
        if(bLimitLoggingOfBadChannels && wf->warnings == iMaxWarningsToLogPerChannelPerDay)
          logit("et","%s.%s.%s.%s too many warnings. Further warnings will not be logged.\n",
                wf->sta, wf->cmp, wf->net, wf->loc);
        wf->samprate     = thd->samprate;     /* store new sample rate      */
        wf->halfinterval = 0.5/thd->samprate; /* half a sample interval (s) */
      }
      else
      {
        if(!bLimitLoggingOfBadChannels || wf->badpackets < iMaxBadPacketsToLogPerChannelPerDay)
          logit("et","%s.%s.%s.%s rejecting packet (starttime %.3lf);"
                " samplerate changed from %.1lf to %.1lf\n",
                wf->sta, wf->cmp, wf->net, wf->loc, thd->starttime,
                wf->samprate, thd->samprate );
        wf->badpackets++;
        if(bLimitLoggingOfBadChannels && wf->badpackets == iMaxBadPacketsToLogPerChannelPerDay)
          logit("et","%s.%s.%s.%s too many bad packets. Further problems will not be logged.\n",
                wf->sta, wf->cmp, wf->net, wf->loc);
        return(FALSE);
      }
   }

/* Store expected time for next packet
 *************************************/
   wf->texp = thd->starttime + (double)thd->nsamp/thd->samprate;
   wf->goodpackets++;
   return( TRUE );
}


/******************************************************************************
 *  wftfilter_findchan()  find channel in list of SCNLs we're tracking        *
 ******************************************************************************/
WFHISTORY *wftfilter_findchan( TRACE2_HEADER *thd )
{
   WFHISTORY *wf = NULL;

/* Find station in our list
 **************************/
   if( nHist ) {
      strcpy( WfKey.sta,  thd->sta  );
      strcpy( WfKey.cmp,  thd->chan );
      strcpy( WfKey.net,  thd->net  );
      strcpy( WfKey.loc,  thd->loc  );

      wf = (WFHISTORY *)bsearch( &WfKey, WfHist, nHist, sizeof(WFHISTORY), 
                                  wftfilter_compare );
   }
   return( wf );
}


/******************************************************************************
 *  wftfilter_addchan()  Find channel or add channel to tracking list as      *
 *     appropriate.  Return pointer to slot for this channel.                 * 
 ******************************************************************************/
WFHISTORY *wftfilter_addchan( TRACE2_HEADER *thd )
{
   WFHISTORY *tlist = (WFHISTORY *)NULL;

/* See if this channel is already in the list
 ********************************************/
   tlist = wftfilter_findchan( thd );
   if( tlist != (WFHISTORY *)NULL ) return( tlist );

/* Not in list; allocate space for tracking one more channel 
 ***********************************************************/
   tlist = (WFHISTORY *)realloc( WfHist, (nHist+1)*sizeof(WFHISTORY) );
   if( tlist == (WFHISTORY *)NULL )
   {
      logit( "et", "wftfilter_addchan: error reallocing WfHist for"
             " %d channels\n", nHist+1  );
      return( NULL );
   }
   WfHist = tlist;

/* Load new station in last slot of WfHist
 *****************************************/
   tlist = &WfHist[nHist];               /* now point to last slot in WfHist */
   memset(tlist, 0, sizeof(WfHist[nHist]));
   strcpy( tlist->sta, thd->sta  );      /* copy info from this trace header */
   strcpy( tlist->cmp, thd->chan );
   strcpy( tlist->net, thd->net  );
   strcpy( tlist->loc, thd->loc  );
   tlist->texp         = 0.0;            /* force validation of 1st timetamp */
   tlist->samprate     = thd->samprate;  /* store sample rate for this SCNL  */
   tlist->halfinterval = 0.5/thd->samprate; /* half a sample interval (s)    */
   nHist++;

/* Re-sort entire history list
 *****************************/
   qsort( WfHist, nHist, sizeof(WFHISTORY), wftfilter_compare );

   logit("t","%s.%s.%s.%s samplerate %.1lf added; tracking %d channels\n",
          thd->sta, thd->chan, thd->net, thd->loc, thd->samprate, nHist );

/* Find the new channel in the sorted list
 *****************************************/
   return( wftfilter_findchan(thd) );
}


/******************************************************************************
 *  wftfilter_lookup( )   Look up important info from earthworm tables        *
 ******************************************************************************/
void wftfilter_lookup( void )
{

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      logit( "e",
             "wftimefilter: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e",
             "wftimefilter: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e",
             "wftimefilter: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF", &TypeTrace ) != 0 ) {
      logit( "e",
             "wftimefilter: Invalid message type <TYPE_TRACEBUF>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF2", &TypeTrace2 ) != 0 ) {
      logit( "e",
             "wftimefilter: Invalid message type <TYPE_TRACEBUF2>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRACE_COMP_UA", &TypeCompress ) != 0 ) {
      logit( "e",
             "wftimefilter: Invalid message type <TYPE_TRACE_COMP_UA>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRACE2_COMP_UA", &TypeCompress2 ) != 0 ) {
      logit( "e",
             "wftimefilter: Invalid message type <TYPE_TRACE2_COMP_UA>; exiting!\n" );
      exit( -1 );
   }
   return;
}


/******************************************************************************
 *  wftfilter_config() processes command file(s) using kom.c functions;       *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define ncommand 11        /* # of required commands you expect to process   */
void wftfilter_config( char *configfile )
{
   char  init[ncommand];   /* init flags, one byte for each required command */
   int   nmiss;            /* number of required commands that were missed   */
   char *com;
   char *str;
   int   nfiles;
   int   success;
   int   i;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
               "wftimefilter: Error opening command file <%s>; exiting!\n",
                configfile );
        exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  logit( "e",
                         "wftimefilter: Error opening command file <%s>; exiting!\n",
                          &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                if( LogSwitch<0 || LogSwitch>2 ) {
                   logit( "e",
                          "wftimefilter: Invalid <LogFile> value %d; "
                          "must = 0, 1 or 2; exiting!\n", LogSwitch );
                   exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("MyModuleId") ) {
                if( str=k_str() ) {
                   if( GetModId( str, &MyModId ) != 0 ) {
                      logit( "e",
                             "wftimefilter: Invalid module name <%s> "
                             "in <MyModuleId> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("InRing") ) {
                if( str=k_str() ) {
                   if( ( InRingKey = GetKey(str) ) == -1 ) {
                      logit( "e",
                             "wftimefilter: Invalid ring name <%s> "
                             "in <InRing> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("OutRing") ) {
                if( str=k_str() ) {
                   if( ( OutRingKey = GetKey(str) ) == -1 ) {
                      logit( "e",
                             "wftimefilter: Invalid ring name <%s> "
                             "in <OutRing> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[3] = 1;
            }

  /*4*/     else if( k_its("HeartbeatInt") ) {
                HeartbeatInt = k_long();
                init[4] = 1;
            }

         /* Enter installation/module/msgtype to process
          **********************************************/
  /*5*/     else if( k_its("GetLogo") ) {
                int       oktype = 0;
                MSG_LOGO *tlogo  = NULL;
                tlogo = (MSG_LOGO *)realloc( GetLogo, (nLogo+1)*sizeof(MSG_LOGO) );
                if( tlogo == NULL )
                {
                   logit( "e", "wftimefilter: GetLogo: error reallocing"
                           " %d bytes; exiting!\n",
                           (nLogo+1)*sizeof(MSG_LOGO) );
                   exit( -1 );
                }
                GetLogo = tlogo;

                if( str=k_str() ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e",
                              "wftimefilter: Invalid installation name <%s>"
                              " in <GetLogo> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                   if( str=k_str() ) {
                      if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                          logit( "e",
                                 "wftimefilter: Invalid module name <%s>"
                                 " in <GetLogo> cmd; exiting!\n", str );
                          exit( -1 );
                      }

                      if( str=k_str() ) {
                         if( GetType( str, &GetLogo[nLogo].type ) != 0 ) {
                             logit( "e",
                                    "wftimefilter: Invalid message type <%s>"
                                    " in <GetLogo> cmd; exiting!\n", str );
                             exit( -1 );
                         }
                         if     ( GetLogo[nLogo].type == TypeTrace     ) oktype = 1;
                         else if( GetLogo[nLogo].type == TypeTrace2    ) oktype = 1;
                         else if( GetLogo[nLogo].type == TypeCompress  ) oktype = 1;
                         else if( GetLogo[nLogo].type == TypeCompress2 ) oktype = 1;
                         else                                            oktype = 0;
                         if( !oktype ) {
                            logit( "e",
                                  "wftimefilter: Cannot process message type <%s>;"
                                  " exiting!\n", str );
                           exit( -1 );
                         }
                         nLogo++;
                         init[5] = 1;
                      } /* end type */
                   } /* end modid */
                } /* end instid */
            }

  /*6*/     else if( k_its("LogGap") ) {
                LogGap  = k_val();
                init[6] = 1;
            }

  /*7*/     else if( k_its("TimeJumpTolerance") ) {
                TimeJumpTolerance = k_val();
                if( TimeJumpTolerance < 0.0  &&  TimeJumpTolerance != -1.0 )
                {
                   logit( "e", "wftimefilter: <TimeJumpTolerance %.1lf> outside "
                          "valid range (-1.0 or >=0.0) in <%s>; exiting!\n", 
                          TimeJumpTolerance, configfile );
                   exit( -1 );
                }
                init[7] = 1;
            }

  /*8*/     else if( k_its("MaxMessageSize") ) {
                MaxMessageSize = k_long();
                init[8] = 1;
            }

  /*9*/     else if( k_its("UseOriginalInstId") )
            {
                UseOriginalInstId = k_int();
                init[9] = 1;
            }

  /*10*/    else if( k_its("UseOriginalModId") )
            {
                UseOriginalModId = k_int();
                init[10] = 1;
            }
  /*opt.*/  else if( k_its("LimitLoggingOfBadChannels") )
            {
                bLimitLoggingOfBadChannels = k_int();
            }
  /*opt.*/  else if( k_its("MaxBadPacketsToLogPerChannelPerDay") )
            {
                iMaxBadPacketsToLogPerChannelPerDay = k_int();
            }
  /*opt.*/  else if( k_its("MaxWarningsToLogPerChannelPerDay") )
            {
                iMaxWarningsToLogPerChannelPerDay = k_int();
            }
  /*opt.*/  else if( k_its("AllowChangesInSampleRate") )
            {
                bAllowChangesInSampleRate = k_int();
            }
  /*opt.*/  else if( k_its("LogDailySummary") )
            {
                bLogSummary = k_int();
            }
  /*opt.*/  else if( k_its("LogOnlyChannelsWithProblems") )
            {
                bLogOnlyChannelsWithProblems = k_int();
            }
  /*opt.*/  else if( k_its("IssueDailyStatusWithSummaryOfBadPackets") )
            {
                bStatusBadPackets = k_int();
            }

         /* Unknown command
          *****************/
            else {
                logit( "e", "wftimefilter: <%s> Unknown command in <%s>.\n",
                       com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                      "wftimefilter: Bad <%s> command in <%s>; exiting!\n",
                       com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "wftimefilter: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "            );
       if ( !init[1] )  logit( "e", "<MyModuleId> "         );
       if ( !init[2] )  logit( "e", "<InRing> "             );
       if ( !init[3] )  logit( "e", "<OutRing> "            );
       if ( !init[4] )  logit( "e", "<HeartbeatInt> "       );
       if ( !init[5] )  logit( "e", "<GetLogo> "            );
       if ( !init[6] )  logit( "e", "<LogGap> "             );
       if ( !init[7] )  logit( "e", "<TimeJumpTolerance> "  );
       if ( !init[8] )  logit( "e", "<MaxMessageSize> "     );
       if ( !init[9] )  logit( "e", "<UseOriginalInstId> "  );
       if ( !init[10] ) logit( "e", "<UseOriginalModId> "   );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   return;
}

/******************************************************************************
 *  wftfilter_logparm( )   Log operating params                                *
 ******************************************************************************/
void wftfilter_logparm( void )
{
   int i;
   logit("","MyModuleId:         %u\n",        MyModId );
   logit("","InRing key:         %ld\n",       InRingKey );
   logit("","OutRing key:        %ld\n",       OutRingKey );
   logit("","HeartbeatInt:       %ld sec\n",   HeartbeatInt );
   logit("","LogFile:            %d\n",        LogSwitch );
   logit("","MaxMessageSize:     %d bytes\n",  MaxMessageSize );
   for(i=0;i<nLogo;i++)  logit("","GetLogo[%d]:         i%u m%u t%u\n", i,
                               GetLogo[i].instid, GetLogo[i].mod, GetLogo[i].type );
   logit("","UseOriginalInstId:  %d\n",        UseOriginalInstId );
   logit("","UseOriginalModId:   %d\n",        UseOriginalModId  );
   logit("","LogGap:             %.1lf sec\n", LogGap );
   logit("","TimeJumpTolerance:  %.1lf sec\n", TimeJumpTolerance );
   logit("","AllowChangesInSampleRate:  %d\n", bAllowChangesInSampleRate );

   return;
}


/******************************************************************************
 * wftfilter_status() builds a heartbeat or error message & puts it into      *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void wftfilter_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[1024];
   long        size;
   time_t      t;

/* Build the message
 *******************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "wftimefilter: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","wftimefilter:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","wftimefilter:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

/******************************************************************************
 *  wftfilter_compare()  This function is passed to qsort() * bsearch() so    *
 *     we can sort the channel list by sta, net, component & location codes,  *
 *     and then look up a channel efficiently in the list.                    *
 ******************************************************************************/
int wftfilter_compare( const void *s1, const void *s2 )
{
   int rc;
   WFHISTORY *t1 = (WFHISTORY *) s1;
   WFHISTORY *t2 = (WFHISTORY *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if( rc != 0 ) return rc;
   rc = strcmp( t1->net, t2->net );
   if( rc != 0 ) return rc;
   rc = strcmp( t1->cmp, t2->cmp );
   if( rc != 0 ) return rc;
   rc = strcmp( t1->loc, t2->loc );
   return rc;
}


/******************************************************************************
 *  wftfilter_managesummary()  This function manages the channel summary info *
 *     used for logging and status messages.                                  *
 ******************************************************************************/
int wftfilter_managesummary(time_t timeNow)
{
  int           i;
  WFHISTORY    *pwfh;
  char         szStatusOut[1024];
  int          iNumBadPackets;
  int          iStatusLen;
  int          rc;

  if((timeNow - timeLastLoggingOfBadChannelsReInit) > 60*60*24)  /* do every 24 hours */
  {
    if(bLogSummary)
    {
      logit("et", "Packet summary:\n"
        "%16s  Good Warn  Bad\n",
        "STA .CMP.NT.LC");
      logit("","************************************************************\n");
    }
    if(bStatusBadPackets)
    {
      strcpy(szStatusOut, "wftimefilter filtered out bad packets for the following channels:\n");
      iStatusLen = strlen(szStatusOut);
      iNumBadPackets=0;
    }
    for(i=0; i < nHist; i++)
    {
      pwfh = &WfHist[i];
      if(bLogSummary)
      {
        if(pwfh->badpackets || (!bLogOnlyChannelsWithProblems))
        {
          logit("","%4s.%3s.%2s.%2s  %5d %4d %4d\n",
            pwfh->sta, pwfh->cmp, pwfh->net, pwfh->loc, 
            pwfh->goodpackets, pwfh->warnings, pwfh->badpackets);
        }
      }
      if(bStatusBadPackets && pwfh->badpackets)
      {
        rc=snprintf(&szStatusOut[iStatusLen], sizeof(szStatusOut)-1-iStatusLen, "%4s.%3s.%2s.%2s\n",
                    pwfh->sta, pwfh->cmp, pwfh->net, pwfh->loc);
        if(rc>0)
          iStatusLen+=rc;
        iNumBadPackets++;
      }
      
      pwfh->goodpackets = 0;
      pwfh->warnings = 0;
      pwfh->badpackets = 0;
    }
    if(bLogSummary)
      logit("","************************************************************\n");
    
    if(bStatusBadPackets && iNumBadPackets)
    {
      szStatusOut[iStatusLen] = 0x00;
      wftfilter_status( TypeError, ERR_FILTERPACKET, szStatusOut );
    }
    
    
    timeLastLoggingOfBadChannelsReInit = timeNow;
  }
  return(0);
}  /* end wftfilter_managesummary() */

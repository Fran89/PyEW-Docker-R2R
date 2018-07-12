/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: naqs2ew.c 4512 2011-08-09 18:25:58Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2009/06/05 17:33:06  dietz
 *     Changed debug logging in packet-reading code (nmx_api.c) and bundle-reading
 *     code (nmxp_packet.c) from compile-time definition to run-time configurable
 *     option.
 *
 *     Revision 1.6  2005/11/23 22:44:29  dietz
 *     Added new command <RepackageNmx> to control how Nmx packets are converted
 *     into EW msgs (either as 1-second EW msgs, or 1 EW msg per Nmx packet).
 *     Default is 1-second EW msgs.
 *
 *     Revision 1.5  2005/11/23 00:19:04  dietz
 *     Modified to output all packets (regardless of time-order) to Earthworm
 *     when RequestChanSCNL "delay" is -1.  This will allow out-of-chronological
 *     order data and possibly duplicate data into the EW system.
 *
 *     Revision 1.4  2004/04/14 20:07:09  dietz
 *     modifications to support location code
 *
 *     Revision 1.3  2002/07/09 18:10:33  dietz
 *     logit changes
 *
 *     Revision 1.2  2002/03/15 23:10:09  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/06/20 22:35:07  dietz
 *     Initial revision
 *
 *
 *
 */

/*
 *   naqs2ew.c:  Program to receive data packets from a NaqsServer
 *               via socket and put them into a transport ring as
 *               Earthworm TYPE_TRACEBUF2 messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <kom.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "naqs2ew.h"

/* Functions used in this source file
 ************************************/
int IsValidLocChar( char );
int naqs2ew_shipdata( NMX_DECOMP_DATA * );

/* Globals declared in naqschassis.c
 ***********************************/
extern char          RingName[];  /* name of transport ring for output          */
extern char          ProgName[];  /* name of program - used in logging          */
extern int           Debug;       /* Debug logging flag (optionally configured) */
extern unsigned char MyInstId;    /* local installation id                      */
extern unsigned char MyModId;     /* Module Id for this program                 */
extern SHM_INFO      Region;      /* shared memory region to use for output     */

/* Globals specific to this naqsclient
 *************************************/
static double    TimeJumpTolerance = -9.; /* used in comparing packet time to   */
                                      /* system time when looking for bogus pkt */
                                      /* timetamps. -1 means don't do check     */
static int       MaxSamplePerMsg = 0; /* max# samples to pack in a tracebuf msg */
static int       MaxReqSCN       = 0; /* working limit on max # scn's in ReqSCN */ 
static int       RepackageNmx    = 0; /* control how NMX packets are repackaged */
static int       nReqSCN = 0;         /* # of scn's found in config file        */
static SCN_INFO *ReqSCN  = NULL;      /* SCNs requested in config file          */
static NMX_DECOMP_DATA  NaqsBuf;      /* struct containing one Naqs data packet */
static unsigned char    OutputMsgType=0;
static unsigned char    TypeTraceBuf;
static unsigned char    TypeTraceBuf2;


static NMX_CHANNEL_INFO *ChanInf = NULL;  /* channel info structures from NAQS  */
static int                nChan = 0;      /* actual # channels in ChanInf array */

/*******************************************************************************
 * IsValidLocChar() returns 1 if arg is a valid location code character;       *
 *                  returns 0 if arg is not a valid location code character    *
 *******************************************************************************/
int IsValidLocChar( char c )
{
   if( c >= '0'  &&  c <= '9' ) return( 1 );
   if( c >= 'A'  &&  c <= 'Z' ) return( 1 );
   if( c == '-' )               return( 1 );
   return( 0 );
}

/*******************************************************************************
 * naqsclient_com() read config file commands specific to this client          *
 *   Return 1 if the command was processed, 0 if it wasn't                     *
 *******************************************************************************/
int naqsclient_com( char *configfile )
{
   char *str;

/* Add an entry to the request-list
 **********************************/
   if(k_its("RequestChannel") ) {
      int badarg = 0;
      char *argname[] = {"","sta","comp","net",
                            "pinno","delay","format","sendbuffer"};
      if( nReqSCN >= MaxReqSCN )
      {
         size_t size;
         MaxReqSCN += 10;
         size     = MaxReqSCN * sizeof( SCN_INFO );
         ReqSCN   = (SCN_INFO *) realloc( ReqSCN, size );
         if( ReqSCN == NULL )
         {
            logit( "e",
                   "%s: Error allocating %d bytes"
                   " for requested channel list; exiting!\n", ProgName, size );
            exit( -1 );
         }
      }
      memset( &ReqSCN[nReqSCN], 0, sizeof(SCN_INFO) );

      str=k_str();
      if( !str || strlen(str)>(size_t)STATION_LEN ) {
         badarg = 1; goto EndRequestChannel;
      }
      strcpy(ReqSCN[nReqSCN].sta,str);

      str=k_str();
      if( !str || strlen(str)>(size_t)CHAN_LEN) {
         badarg = 2; goto EndRequestChannel;
      }
      strcpy(ReqSCN[nReqSCN].chan,str);

      str=k_str();
      if( !str || strlen(str)>(size_t)NETWORK_LEN) {
         badarg = 3; goto EndRequestChannel;
      }
      strcpy(ReqSCN[nReqSCN].net,str);

      strcpy(ReqSCN[nReqSCN].loc,LOC_NULL_STRING);  /* default blank location code */

      ReqSCN[nReqSCN].pinno = k_int();
      if( ReqSCN[nReqSCN].pinno <= 0 || ReqSCN[nReqSCN].pinno > 32767 ) {
         badarg = 4; goto EndRequestChannel;
      }

      ReqSCN[nReqSCN].delay = k_int();
      if( ReqSCN[nReqSCN].delay < -1 || ReqSCN[nReqSCN].delay > 300 ) {
         badarg = 5; goto EndRequestChannel;
      }

      ReqSCN[nReqSCN].format = k_int();
      if( ReqSCN[nReqSCN].format < -1 ) {
         badarg = 6; goto EndRequestChannel;
      }

      ReqSCN[nReqSCN].sendbuffer = k_int();
      if( ReqSCN[nReqSCN].sendbuffer!=0 && ReqSCN[nReqSCN].sendbuffer!=1 ) {
         badarg = 7; goto EndRequestChannel;
      }

   EndRequestChannel:
      if( badarg ) {
         logit( "e", "%s: Argument %d (%s) bad in <RequestChannel> "
                "command (too long, missing, or invalid value):\n"
                "   \"%s\"\n", ProgName, badarg, argname[badarg], k_com() );
         logit( "e", "%s: exiting!\n", ProgName );
         free( ReqSCN );
         exit( -1 );
      }
      nReqSCN++;
      return( 1 );

   } /* end of RequestChannel */

   if(k_its("RequestChanSCNL") ) {
      int badarg = 0;
      char *argname[] = {"","sta","comp","net","loc",
                            "pinno","delay","format","sendbuffer"};
      if( nReqSCN >= MaxReqSCN )
      {
         size_t size;
         MaxReqSCN += 10;
         size     = MaxReqSCN * sizeof( SCN_INFO );
         ReqSCN   = (SCN_INFO *) realloc( ReqSCN, size );
         if( ReqSCN == NULL )
         {
            logit( "e",
                   "%s: Error allocating %d bytes"
                   " for requested channel list; exiting!\n", ProgName, size );
            exit( -1 );
         }
      }
      memset( &ReqSCN[nReqSCN], 0, sizeof(SCN_INFO) );

      str=k_str();
      if( !str || strlen(str)>(size_t)STATION_LEN ) {
         badarg = 1; goto EndRequestChanSCNL;
      }
      strcpy(ReqSCN[nReqSCN].sta,str);

      str=k_str();
      if( !str || strlen(str)>(size_t)CHAN_LEN) {
         badarg = 2; goto EndRequestChanSCNL;
      }
      strcpy(ReqSCN[nReqSCN].chan,str);

      str=k_str();
      if( !str || strlen(str)>(size_t)NETWORK_LEN) {
         badarg = 3; goto EndRequestChanSCNL;
      }
      strcpy(ReqSCN[nReqSCN].net,str);

      str=k_str();
      if( !str || strlen(str)!=(size_t)LOC_LEN) {
         badarg = 4; goto EndRequestChanSCNL;
      }
      if( !IsValidLocChar(str[0]) ||
          !IsValidLocChar(str[1])    ) {
         logit("e","Valid location code characters are 0-9,A-Z,'-'\n");
         badarg = 4; goto EndRequestChanSCNL;
      }
      strcpy(ReqSCN[nReqSCN].loc,str);
      
      ReqSCN[nReqSCN].pinno = k_int();
      if( ReqSCN[nReqSCN].pinno <= 0 || ReqSCN[nReqSCN].pinno > 32767 ) {
         badarg = 5; goto EndRequestChanSCNL;
      }

      ReqSCN[nReqSCN].delay = k_int();
      if( ReqSCN[nReqSCN].delay < -1 || ReqSCN[nReqSCN].delay > 300 ) {
         badarg = 6; goto EndRequestChanSCNL;
      }

      ReqSCN[nReqSCN].format = k_int();
      if( ReqSCN[nReqSCN].format < -1 ) {
         badarg = 7; goto EndRequestChanSCNL;
      }

      ReqSCN[nReqSCN].sendbuffer = k_int();
      if( ReqSCN[nReqSCN].sendbuffer!=0 && ReqSCN[nReqSCN].sendbuffer!=1 ) {
         badarg = 8; goto EndRequestChanSCNL;
      }

   EndRequestChanSCNL:
      if( badarg ) {
         logit( "e", "%s: Argument %d (%s) bad in <RequestChanSCNL> "
                "command (too long, missing, or invalid value):\n"
                "   \"%s\"\n", ProgName, badarg, argname[badarg], k_com() );
         logit( "e", "%s: exiting!\n", ProgName );
         free( ReqSCN );
         exit( -1 );
      }
      nReqSCN++;
      return( 1 );

   } /* end of RequestChanSCNL */

   if(k_its("MaxSamplePerMsg") ) {
      int max;
      MaxSamplePerMsg = k_int();
      max = (MAX_TRACEBUF_SIZ-sizeof(TRACE2_HEADER))/sizeof(int);
      if( MaxSamplePerMsg > max )
      {
         logit( "e", "%s: <MaxSamplePerMsg %d> outside valid range "
                "(0 to %d) in <%s>; exiting!\n", ProgName, MaxSamplePerMsg, 
                 max, configfile );
         exit( -1 );
      }
      return( 1 );
   }

   if(k_its("TimeJumpTolerance") ) {
      TimeJumpTolerance = k_val();
      if( TimeJumpTolerance < 0.0  &&  TimeJumpTolerance != -1.0 )
      {
         logit( "e", "%s: <TimeJumpTolerance %.1lf> outside valid range "
                "(-1.0 or >=0.0) in <%s>; exiting!\n", ProgName,  
                 TimeJumpTolerance, configfile );
         exit( -1 );
      }
      return( 1 );
   }

   if(k_its("RepackageNmx") ) {
      RepackageNmx = k_int();
      if( RepackageNmx == NAQS2EW_1MSGPERPKT ) return( 1 );
      if( RepackageNmx == NAQS2EW_1SECMSG    ) return( 1 );      
      logit( "e", "%s: <RepackageNmx %d> invalid; must be either "
             "%d or %d; exiting!\n", ProgName, RepackageNmx, 
              (int)NAQS2EW_1MSGPERPKT, (int)NAQS2EW_1SECMSG );
      exit( -1 );
   }

   if(k_its("OutputMsgType") ) {
      str=k_str();
      if( !str || 
         (strcmp(str,"TYPE_TRACEBUF") !=0 &&
          strcmp(str,"TYPE_TRACEBUF2")!=0    ) ) 
      {
         logit( "e","%s: <OutputMsgType %s> invalid; must be either "
                "TYPE_TRACEBUF or TYPE_TRACEBUF2; exiting!\n", 
                ProgName, str );
         exit( -1 );
      }
      if( GetType( str, &OutputMsgType ) != 0 ) {
         logit("e","%s: Invalid message type %s!\n",
               ProgName, str );
         exit( -1 );
      }
      return( 1 );
   }

   return( 0 );
}


/*******************************************************************************
 * naqsclient_init() initialize client stuff                                   *
 *******************************************************************************/
int naqsclient_init( void )
{
/* Look up message types in earthworm.d tables
 *********************************************/
   if ( GetType( "TYPE_TRACEBUF", &TypeTraceBuf ) != 0 ) {
      logit("e","%s: Invalid message type <TYPE_TRACEBUF>!\n",
             ProgName );
      return( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 ) != 0 ) {
      logit("e","%s: Invalid message type <TYPE_TRACEBUF2>!\n",
             ProgName );
      return( -1 );
   }

/* Check that all commands were read from config file
 ****************************************************/
   if( nReqSCN == 0 )
   {
     logit("e", "%s: No <RequestChanSCNL> or <RequestChannel> commands in config file!\n",
            ProgName );
     return( -1 );
   }
   if( MaxSamplePerMsg == 0 )
   {
     logit("e", "%s: No <MaxSamplePerMsg> commands in config file!\n",
            ProgName );
     return( -1 );
   }
   if( TimeJumpTolerance == -9.0 )
   {
     logit("e", "%s: No <TimeJumpTolerance> commands in config file!\n",
            ProgName );
     return( -1 );
   }
   if( OutputMsgType == 0 ) 
   {
     OutputMsgType = TypeTraceBuf2;
   }
   if( RepackageNmx == 0 )
   {
     RepackageNmx = NAQS2EW_1SECMSG;
   }
   

/* Allocate memory 
 *****************/
   NaqsBuf.maxdatalen = MAX_TRACEBUF_SIZ; 
   if( ( NaqsBuf.data = (int *) malloc(NaqsBuf.maxdatalen) ) == NULL )
   {
     logit("e", "%s: Cannot allocate %ld bytes for NaqsBuf.data!\n",
                 ProgName, NaqsBuf.maxdatalen );
     return( -1 );
   }

   return( 0 );
} 


/*******************************************************************************
 * naqsclient_process() handle interesting packets from NaqsServer             *
 *  Returns 1 if packet was processed successfully                             *
 *          0 if packet was NOT processed                                      *
 *         -1 if there was trouble processing the packet                       *
 *******************************************************************************/
int naqsclient_process( long rctype, NMXHDR *pnaqshd, 
                        char **pbuf, int *pbuflen )
{
   char *sockbuf = *pbuf;  /* working pointer into socket buffer */
   int   rdrc;
   int   rc;

/* Process waveform data packets 
 *******************************/
   if( rctype == NMXMSG_DECOMPRESSED_DATA  ||
       rctype == NMXMSG_COMPRESSED_DATA  )  
   {
      if(Debug) logit("et","%s: got a %d-byte data message\n",
                       ProgName, pnaqshd->msglen );

   Read_Waveform:
      /* read the message */
      if( rctype==NMXMSG_DECOMPRESSED_DATA ) {
         rdrc = nmx_rd_decompress_data( pnaqshd, sockbuf, &NaqsBuf );
      } else {
         rdrc = nmx_rd_compressed_data( pnaqshd, sockbuf, &NaqsBuf );
      }
      if( rdrc < 0 )
      {
         logit("et","%s: trouble reading waveform message\n", ProgName );
         return( -1 );
      }

   /* data buffer wasn't big enough, realloc and try again */
      else if( rdrc > 0 )
      {
         int *ptmp;
         if( (ptmp=(int *)realloc( NaqsBuf.data, rdrc)) == NULL )
         {
            logit("et","%s: error reallocing NaqsBuf.data from %d to %d "
                    "bytes\n", ProgName, NaqsBuf.maxdatalen, rdrc );
            return( -1 );
         }
         logit("et","%s: realloc'd NaqsBuf.data from %d to %d "
                 "bytes\n", ProgName, NaqsBuf.maxdatalen, rdrc );
         NaqsBuf.data       = ptmp;
         NaqsBuf.maxdatalen = rdrc;
         goto Read_Waveform;
      }

      if(Debug>=2)
      {
         int id;
         logit("et","chankey:%d  starttime:%.2lf  nsamp:%d  isamprate:%d\n",
                NaqsBuf.chankey, NaqsBuf.starttime, NaqsBuf.nsamp,
                NaqsBuf.isamprate );
         for( id=0; id<NaqsBuf.nsamp; id++ ) {
            logit("e", "%8d", NaqsBuf.data[id] );
            if(id%10==9) logit("e", "\n");
         }
      }

   /* write it to the transport ring */
      rc = naqs2ew_shipdata( &NaqsBuf );

   } /* end if waveform data */

/* got a list of all possible channels, continue startup protocol
 *  2) recv "channel list" message
 *  3) send "add channel" message(s)
 ****************************************************************/
   else if( rctype == NMXMSG_CHANNEL_LIST )
   {
      int msglen = 0;
      int nadd   = 0;
      int ir;

      nChan = 0;
      free( ChanInf );

   /* read the channel list message */
      if( nmx_rd_channel_list( pnaqshd, sockbuf, &ChanInf, &nChan ) < 0 )
      {
         logit("e", "%s: trouble reading channel list message\n", ProgName );
         return( -1 );
      }

   /* compare the Naqs channel list again the requested channels */

      rc = SelectChannels( ChanInf, nChan, ReqSCN, nReqSCN );

   /* build and send "add channel" message for each available channel */
          
      for( ir=0; ir<nReqSCN; ir++ )
      {
         if( ReqSCN[ir].flag == SCN_NOTAVAILABLE ) continue;
         if( nmx_bld_add_timeseries( pbuf, pbuflen, &msglen,
                  &(ReqSCN[ir].info.chankey), 1, ReqSCN[ir].delay,
                  ReqSCN[ir].format, ReqSCN[ir].sendbuffer ) != 0 )
         {
            logit("et","%s: trouble building add_timeseries message\n", ProgName);
            continue;
         }
         nmxsrv_sendto( sockbuf, msglen );
         if(Debug) logit("et","sent add_timeseries message %d\n", ++nadd );
      }
      
   } /* end if channel list msg */

/* Not a message type we're interested in
 ****************************************/
   else {
      return( 0 );
   }

   return( 1 );

} /* end of naqsclient_process */


/*******************************************************************************
 * naqsclient_shutdown()  frees malloc'd memory, etc                           *
 *******************************************************************************/
void naqsclient_shutdown( void )
{
   int ir;

/* Send any data in carryover space
 **********************************/
   for( ir=0; ir<nReqSCN; ir++ ) {
      NaqsBuf.chankey   = ReqSCN[ir].info.chankey;
      NaqsBuf.starttime = -1.0;
      NaqsBuf.nsamp     = 0;
      NaqsBuf.isamprate = 0;
      naqs2ew_shipdata( &NaqsBuf );
   }

/* Free all allocated memory
 ***************************/
   free( ReqSCN );
   free( NaqsBuf.data );
   free( ChanInf );
}


/*****************************************************************************
 * naqs2ew_shipdata() takes a buffer of Naqs data, translates it into        *
 *   TYPE_TRACEBUF2 message(s) and writes them to the transport region       *
 *****************************************************************************/
int naqs2ew_shipdata( NMX_DECOMP_DATA *nbuf )
{
   SCN_INFO     *scn;                      /* pointer to channel information */
   char          tbuf[MAX_TRACEBUF_SIZ];   /* buffer for outgoing message    */
   TRACE2_HEADER thd;
   MSG_LOGO      logo;
   double        dt=0.0;   
   double        dtsys=0.0;  
   int           nshipped;
   int           nleft;
   int           samplepermsg;
   int           minpermsg;       
   int           npreload    = 0;
   int           newsamprate = 0;
   int           datagap     = 0;

/* Initialize some things 
 ************************/
   logo.instid = MyInstId;
   logo.mod    = MyModId;
   logo.type   = OutputMsgType;
   memset( &tbuf, 0, MAX_TRACEBUF_SIZ );
   memset( &thd,  0, sizeof(TRACE2_HEADER) );

/* Find this channel in the subscription list
 ********************************************/
   scn = FindChannel( ReqSCN, nReqSCN, nbuf->chankey );
   if( scn == NULL )
   {
      logit("et","%s: Received unknown chankey:%d "
                "(not in subscription list)\n", ProgName, nbuf->chankey );
      /* probably should report this to statmgr */
      return( NMX_SUCCESS );
   }
   if( Debug ) logit("e","\n%s.%s.%s.%s  new data starttime: %.3lf nsamp:%d isamprate:%d\n",
                      scn->sta,scn->chan,scn->net,scn->loc,nbuf->starttime,
                      nbuf->nsamp, nbuf->isamprate );
   
/* First data for this channel.  
   Allocate space for carryover data (1-sec maximum, with limits)
   Initialize fields for gap/overlap testing (lastrate,texp)
 *****************************************************************/
   if( scn->carryover.data == NULL ) 
   {
      int maxsamp = nbuf->isamprate;
      if( maxsamp > MaxSamplePerMsg ) maxsamp = MaxSamplePerMsg;
      scn->carryover.maxdatalen = sizeof(int) * maxsamp;
      if( (scn->carryover.data = (int *)malloc(scn->carryover.maxdatalen)) == NULL )
      {
         logit("e", "%s: Cannot allocate %ld bytes for carryover data; "
                    "exiting!\n", ProgName, scn->carryover.maxdatalen );
         naqschassis_shutdown( -1 );
      }
      memset( scn->carryover.data, 0, scn->carryover.maxdatalen );
      logit("et","%s:  %s.%s.%s.%s carryover space allocated "
            "(%d samples, %ld bytes)\n", ProgName, scn->sta, 
             scn->chan, scn->net, scn->loc, maxsamp, scn->carryover.maxdatalen );
      scn->lastrate = nbuf->isamprate;
      scn->texp     = nbuf->starttime;
   }

/* Check for time tears (overlaps/abnormal forward jumps). 
   If any are found, ignore the whole packet (unless delay=-1)!
 **************************************************************/
   dt = nbuf->starttime - scn->texp;
   if(Debug) logit("e","%s.%s.%s.%s dt=%.3lf\n",scn->sta,scn->chan,scn->net,scn->loc,dt );
   if( ABS(dt) > (double)0.5/scn->lastrate ) 
   {
      if( nbuf->starttime == -1.0 ) { /* happens on shutdown; not really time tear, */
          nbuf->nsamp = 0;            /* but flag is needed to ship carryover data  */
          datagap = 1;             
      }
      else if( dt > 0.0 ) /* time gap */
      {
      /* If starttime later than system-clock (with slop), it's "abnormal" */
      /* time jump.  Probably a bad timestamp - ignore the packet!         */
         dtsys = nbuf->starttime - (double)time(NULL);
         if( dtsys >= TimeJumpTolerance  &&  TimeJumpTolerance != -1.0 ) {
            logit("e","%s: %s.%s.%s.%s %.3lf %.3lf s gap detected; "
                  "dtsys %.0lf s; ignoring packet with probable bogus timestamp\n",
                   ProgName, scn->sta, scn->chan, scn->net, scn->loc, scn->texp, 
                   dt, dtsys );
            return NMX_SUCCESS;
         }
         logit("e","%s: %s.%s.%s.%s %.3lf %.3lf s gap detected\n",
                ProgName, scn->sta, scn->chan, scn->net, scn->loc, scn->texp, dt );
         datagap = 1;
      }
      else if( scn->delay == -1 )  /* time overlap: configured to expect */
      {                            /* out-of-chronological-order packets */
         logit("e","%s: %s.%s.%s.%s %.3lf %.3lf s overlap detected; "
               "shipping out-of-sequence packet\n",
               ProgName, scn->sta, scn->chan, scn->net, scn->loc, scn->texp, ABS(dt) );
         datagap = 1;
      }
      else  /* time overlap: not expecting out-of-order data!         */
      {     /* Keep existing carryover data and ignore the new packet */
         logit("e","%s: %s.%s.%s.%s %.3lf %.3lf s overlap detected; "
               "ignoring out-of-sequence packet\n",
               ProgName, scn->sta, scn->chan, scn->net, scn->loc, scn->texp, ABS(dt) );
         return NMX_SUCCESS;
      }
   }

/* Fill out static parts of the tracebuf header
 **********************************************/
   strncpy( thd.sta,  scn->sta,  STATION_LEN+1 );
   strncpy( thd.chan, scn->chan, CHAN_LEN+1    );
   strncpy( thd.net,  scn->net,  NETWORK_LEN+1 );

   if( OutputMsgType == TypeTraceBuf2 )
   {
      strncpy( thd.loc,  scn->loc,  LOC_LEN+1 );
      thd.version[0] = TRACE2_VERSION0;
      thd.version[1] = TRACE2_VERSION1;
   }

   thd.pinno    = scn->pinno;
#ifdef _INTEL
   strncpy( thd.datatype, "i4", 3 );
#else
   strncpy( thd.datatype, "s4", 3 );
#endif
   thd.quality[0] = thd.quality[1] = 0;
   thd.pad[0]     = thd.pad[1]     = 0;

/* Deal with carryover data - compare it with new data.
 ******************************************************/
   if( scn->carryover.nsamp )
   {
   /* Check for change in sample rate 
    *********************************/
      if( nbuf->isamprate != scn->carryover.isamprate )
      {
         if( nbuf->isamprate ) { 
            logit("e","%s: %s.%s.%s.%s sample rate changed from %d to %d sps.\n",
               ProgName, scn->sta, scn->chan, scn->net, scn->loc, 
               scn->carryover.isamprate, nbuf->isamprate );
         }
         newsamprate  = 1;
      }

   /* Fill out variable header values & load all carried-over data 
    **************************************************************/
      thd.samprate  = (double)scn->carryover.isamprate;
      thd.starttime = scn->carryover.starttime;
      thd.endtime   = thd.starttime + (double)(scn->carryover.nsamp-1)/thd.samprate;
      thd.nsamp     = scn->carryover.nsamp;
      memcpy( tbuf, &thd, sizeof(TRACE2_HEADER) );
      memcpy( tbuf+sizeof(TRACE2_HEADER),
              scn->carryover.data, thd.nsamp*sizeof(int) );
      npreload = scn->carryover.nsamp;

   /* Ship it now if there was a tear in the data stream 
    ****************************************************/
      if( newsamprate || datagap )
      {
         int msglen = sizeof(TRACE2_HEADER) + thd.nsamp*sizeof(int);
         if( tport_putmsg( &Region, &logo, msglen, tbuf ) != PUT_OK )
         {
             logit("et", "%s: Error putting %d-byte tracebuf msg in %s\n",
                 ProgName, msglen, RingName );
         }
         npreload = 0;
         scn->lastrate = scn->carryover.isamprate;      
         scn->texp     = thd.endtime + (double)1.0/thd.samprate;
         if( Debug ) logit("e","%s.%s.%s.%s texp=%.3lf after carryover shipped\n",
                           scn->sta,scn->chan,scn->net,scn->loc,scn->texp );
      }

   /* Reallocate carryover space if there was a sample rate change 
    **************************************************************/
      if( newsamprate )
      {
         int *ptmp;
         int  maxlen;
         int  maxsamp = nbuf->isamprate; 
         if( maxsamp > MaxSamplePerMsg ) maxsamp = MaxSamplePerMsg;
         maxlen = sizeof(int) * maxsamp;
         if( (ptmp = (int *)realloc(scn->carryover.data, maxlen)) == NULL 
             &&  maxlen != 0 )
         {
            logit("e", "%s: %s.%s.%s.%s cannot reallocate carryover space "
                  "from %ld to %ld bytes; exiting!\n", ProgName, scn->sta,  
                   scn->chan, scn->net, scn->loc, scn->carryover.maxdatalen, maxlen );
            naqschassis_shutdown( -1 );
         }
         logit("et","%s:  %s.%s.%s.%s carryover space reallocated "
               "from %ld to %ld bytes.\n", ProgName, scn->sta, scn->chan, 
                scn->net, scn->loc, scn->carryover.maxdatalen, maxlen );
         scn->carryover.maxdatalen = maxlen;
         scn->carryover.data       = ptmp;
      }

   /* reset carryover space 
    ***********************/
      memset( scn->carryover.data, 0, scn->carryover.maxdatalen );
      scn->carryover.nsamp     = 0;
      scn->carryover.starttime = 0.0;
   }

/* Set desired message size
 **************************/
   thd.samprate = (double)nbuf->isamprate;

/* Create 1-second EW msgs if possible; some samples     */
/* may be carried over until receipt of next Nmx packet  */
   if( RepackageNmx==NAQS2EW_1SECMSG )
   {                        
     samplepermsg = nbuf->isamprate;
     if( samplepermsg > MaxSamplePerMsg ) samplepermsg = MaxSamplePerMsg;
     minpermsg = samplepermsg;     
   } 

/* Create one EW msg (within MaxSamplePerMsg limit) from */
/* each Nmx packet; all samples will be converted now!   */
   else  /* NAQS2EW_1MSGPERPKT */                   
   {                       
     samplepermsg = nbuf->nsamp;
     if( samplepermsg > MaxSamplePerMsg ) samplepermsg = MaxSamplePerMsg;
     minpermsg = 1;    
   } 

/* Deal with new data buffer
 ***************************/
   nshipped = 0;
   nleft    = nbuf->nsamp;
   while( nleft+npreload >= minpermsg   &&   samplepermsg )
   {
      int nload = nleft;
      int msglen;
      if( nload > samplepermsg-npreload ) nload = samplepermsg-npreload;

      if( !npreload ) {  /* starttime is already set if data was preloaded */
         thd.starttime = nbuf->starttime + (double)nshipped/thd.samprate;
      }
      thd.endtime = thd.starttime + (double)(nload+npreload-1)/thd.samprate;
      thd.nsamp   = nload + npreload;

      memcpy( tbuf, &thd, sizeof(TRACE2_HEADER) );
      memcpy( tbuf+sizeof(TRACE2_HEADER)+npreload*sizeof(int),
              &(nbuf->data[nshipped]), nload*sizeof(int) );
      msglen = sizeof(TRACE2_HEADER) + thd.nsamp*sizeof(int);

      if( tport_putmsg( &Region, &logo, msglen, tbuf ) != PUT_OK )
      {
          logit("et", "%s: Error putting %d-byte tracebuf msg in %s\n",
                 ProgName, msglen, RingName );
      }
      nshipped += nload;
      nleft    -= nload;
      npreload  = 0;
      scn->lastrate = nbuf->isamprate;      
      scn->texp     = thd.endtime + (double)1.0/thd.samprate;
      if( Debug ) logit("e","%s.%s.%s.%s texp=%.3lf after new data shipped\n",
                        scn->sta,scn->chan,scn->net,scn->loc,scn->texp );
   }

/* Store any leftovers in the carryover space
 ********************************************/
   if( nleft ) 
   {
   /* Preloaded data remains!  
      The new buffer plus carryover still didn't equal a whole second; 
      save the preloaded data first, then the entire new buffer */
      if( npreload ) { 
         scn->carryover.chankey   = nbuf->chankey;
         scn->carryover.starttime = thd.starttime;
         scn->carryover.nsamp     = npreload + nleft;
         scn->carryover.isamprate = nbuf->isamprate;
         memcpy( scn->carryover.data, tbuf+sizeof(TRACE2_HEADER), npreload*sizeof(int) );
         memcpy( scn->carryover.data+npreload, nbuf->data, nleft*sizeof(int) );
      } 
   /* All leftover data is from the new buffer */
      else {
         scn->carryover.chankey   = nbuf->chankey;
         scn->carryover.starttime = nbuf->starttime + (double)nshipped/thd.samprate;
         scn->carryover.nsamp     = nleft;
         scn->carryover.isamprate = nbuf->isamprate;
         memcpy( scn->carryover.data, &(nbuf->data[nshipped]), nleft*sizeof(int) );
      }
      if( Debug ) logit("e","%s.%s.%s.%s carryover starttime: %.2lf nsamp:%d isamprate:%d\n",
                         scn->sta,scn->chan,scn->net,scn->loc,scn->carryover.starttime,
                         scn->carryover.nsamp, scn->carryover.isamprate );
      scn->lastrate = scn->carryover.isamprate;      
      scn->texp     = scn->carryover.starttime + 
                      (double)(scn->carryover.nsamp)/scn->carryover.isamprate;
      if( Debug ) logit("e","%s.%s.%s.%s texp=%.3lf after carryover saved\n",
                        scn->sta,scn->chan,scn->net,scn->loc,scn->texp );
   }

   return NMX_SUCCESS;
}



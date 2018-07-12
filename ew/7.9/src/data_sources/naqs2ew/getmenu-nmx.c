/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getmenu-nmx.c 4512 2011-08-09 18:25:58Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2009/06/05 17:33:06  dietz
 *     Changed debug logging in packet-reading code (nmx_api.c) and bundle-reading
 *     code (nmxp_packet.c) from compile-time definition to run-time configurable
 *     option.
 *
 *     Revision 1.3  2007/02/26 20:55:24  paulf
 *     fixed time_t issues for windows
 *
 *     Revision 1.2  2003/10/07 18:00:33  dietz
 *     Added handling of ConnectResponse message
 *
 *     Revision 1.1  2003/01/30 23:11:44  lucky
 *     Initial revision
 *
 *     Revision 1.1  2002/03/15 23:10:09  dietz
 *     Initial revision
 *
 */

/*
 *   getnmxdata.c:  Simple program to connect to NMX DataServer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <time_ew.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <trace_buf.h>
#include "nmx_api.h"

#define DEBUG 0
int     Debug = DEBUG;

/* Globals:
 **********/
char    *DataServerIPAddr;      /* DataServer's IP address, in dot notation   */
int      DataServerPort;        /* DataServer's well-known port number        */
char     ProgName[40];          /* program name for logging purposes          */

unsigned int SocketTimeoutLength;  /* Length of timeouts on SOCKET_ew calls */
int      SOCKET_ewDebug;           /* set to 1 in for socket debug messages */
NMX_CONNECT_REQUEST ConReq;        /* Connect Request structure */

static char *SockBuf   = NULL;  /* buffer for incoming & outgoing socket msgs */
static int   SockBufLen   = 0;  /* length (bytes) of SockBuf (may be dynamic) */

NMX_PRECIS_INFO  *PrecList;      /* Precis List of channels from DataServer */
int               nPrec = 0;
NMX_CHANNEL_INFO *ChanList;      /* Channel List of channels from DataServer */
int               nChan = 0;


int main( int argc, char **argv )
{
   NMXHDR         naqshd;              /* NMX message header structure    */
   int            msglen;
   int            nreq = 0;            /* number of data requests so far   */
   long           rctype;
   char          *c;

/* Set variables from command line arguments
 *******************************************/
   if ( argc != 5 )
   {
        fprintf( stderr, "Usage: %s <DateServerIP> <DataServerPort> <user> <password>\n", 
                 argv[0] );
        return -1;
   }
   strcpy( ProgName, argv[0] );
   c = strchr( ProgName, '.' );
   if( c ) *c = '\0';

   DataServerIPAddr = argv[1];
   DataServerPort   = atoi(argv[2]);

/* Set up connection request structure */
   memset( &ConReq, 0, sizeof(NMX_CONNECT_REQUEST) );
   ConReq.DAPversion = 0;

   if( strlen(argv[3]) >= NMX_DS_LEN ) {
       fprintf(stderr,"Username %s too long, must be < %d chars\n",
               argv[3], NMX_DS_LEN );
       return(-1);
   }   
   strcpy(ConReq.user,argv[3]);

   if( strlen(argv[4]) >= NMX_DS_LEN ) {
       fprintf(stderr,"Password %s too long, must be < %d chars\n",
               argv[4], NMX_DS_LEN );
       return(-1);
   }   
   strcpy(ConReq.pswd,argv[4]);


/* Initialize logit, even though we won't write a file
 *****************************************************/
   logit_init( argv[1], 0, 1024, 0 );

/* Allocate message buffers
 **************************/
   SockBufLen = MAX_TRACEBUF_SIZ;
   if( ( SockBuf = (char *) malloc(SockBufLen) ) == (char *) NULL )
   {
      logit("e", "%s: Cannot allocate %ld bytes for SockBuf; exiting!\n",
            ProgName, SockBufLen );
      return( -1 ); 
   }

/* Start DataServer client protocol:
 * 1) Get a connection to DataServer
 ***********************************/
   if( nmxsrv_connect( DataServerIPAddr, DataServerPort ) != NMX_SUCCESS )
   {
      logit("et","%s: failed to connect to DataServer %s:%d\n",
             ProgName, DataServerIPAddr, DataServerPort );
      return( -1 );
   }

/* 2) read connection time from socket (4-byte int)
 **************************************************/
   if( nmxsrv_recvconnecttime( &ConReq.tconnect ) != NMX_SUCCESS )
   {
      logit("et","%s: trouble reading connection time from DataServer.\n");
      nmxsrv_disconnect();
      return( -1 );
   }

/* 3) send ConnectRequest message to DataServer
 ***********************************************/
   if(DEBUG) {
      logit("e","DataServer User/Password: %s %s\n", ConReq.user, ConReq.pswd );
      logit("e","DataServer Protocol Version: %d\n", ConReq.DAPversion );
      logit("e","tconnect: %ld  %s", 
                 ConReq.tconnect, ctime( (time_t*)&ConReq.tconnect) );
   }
   nmx_bld_connectrequest( &SockBuf, &SockBufLen, &msglen, &ConReq );
   nmxsrv_sendto( SockBuf, msglen );


/*---------------------Main loop-------------------------*/

/* Loop on reading/requesting from DataServer;
   read a message from the socket and process it accordingly.
   4) wait for a "Ready" msg from the DataServer
   5) send a "Request" msg to the DataServer,
   6) read & process the response msg(s), until receiving another 
      "Ready" msg. The "Ready" msg indicates the end of the requested
       data.  Each request may elicit 0 or more response msgs.
 *********************************************************************/
   rctype = NMX_SUCCESS;
   while( rctype != NMX_FAILURE )
   {
       rctype = nmxsrv_recvmsg( &naqshd, &SockBuf, &SockBufLen );

    /* Got a ready message!  Response to data request complete.
     **********************************************************/
       if( rctype == NMXMSG_CONNECT_RESPONSE )
       {
          NMX_CONNECT_RESPONSE cresp;
          if(DEBUG) logit("et","Received ConnectResponse from DataServer!\n" ); 
          nmx_rd_connect_response( &naqshd, SockBuf, &cresp );

          logit("e","Connected to DataServer using Data Access Protocol version %ld\n", 
                cresp.DAPversion);
       }

       else if( rctype == NMXMSG_READY )
       {
          if(DEBUG) logit("et","Received ready message from DataServer!\n" ); 

       /* Request the precise channel list */
          if( nreq == 0 ) {
             nmx_bld_precislist_request( &SockBuf, &SockBufLen, &msglen, 
                                         -1, -1, -1  );
             nmxsrv_sendto( SockBuf, msglen );
             nreq++;
          }
       /* else if( nreq == 1 ) {
             nmx_bld_channellist_request( &SockBuf, &SockBufLen, &msglen );
             nmxsrv_sendto( SockBuf, msglen ); 
             nreq++;
          }  */
          else {
             break;
          }
       }

       else if( rctype == NMXMSG_PRECIS_LIST )
       {
          int ip;
          struct tm ts, te;
          if(DEBUG) logit("et","Received PrecisList message from DataServer:\n" ); 
          nmx_rd_precis_list( &naqshd, SockBuf, &PrecList, &nPrec );
          logit("e","RingBuffer contents:\n");
          for( ip=0; ip<nPrec; ip++ ) {
            gmtime_ew( (time_t *) &PrecList[ip].tstart, &ts );
            gmtime_ew( (time_t *) &PrecList[ip].tend,   &te );

            logit("e","% -12s  Start: %d%02d%02d_%02d%02d_%02d.00  "
                             "End: %d%02d%02d_%02d%02d_%02d.00\n", 
                   PrecList[ip].stachan, 
                   ts.tm_year+1900,ts.tm_mon+1,ts.tm_mday,
                   ts.tm_hour,ts.tm_min,ts.tm_sec,
                   te.tm_year+1900,te.tm_mon+1,te.tm_mday,
                   te.tm_hour,te.tm_min,te.tm_sec);
          }  
       }

       else if( rctype == NMXMSG_CHANNEL_LIST )
       {
          int ip;
          if(DEBUG) logit("et","Received ChannelList message from DataServer:\n" ); 
          nmx_rd_channel_list( &naqshd, SockBuf, &ChanList, &nChan );
          logit("e","RingBuffer contents:\n");
          for( ip=0; ip<nChan; ip++ ) {
            logit("e","%s %d\n", ChanList[ip].stachan, ChanList[ip].subtype );
          }  
       }

    /* error reported by DataServer
     ******************************/
       else if( rctype == NMXMSG_ERROR )
       {
          logit("et","Received error message from DataServer:\n" );
          logit("e", "         %s\n", SockBuf );
       }

    /* termination notice from DataServer
     ************************************/
       else if( rctype == NMXMSG_TERMINATE )
       {
          NMX_TERMINATE terminate;

          nmx_rd_terminate( &naqshd, SockBuf, &terminate );
          if( terminate.note != NULL ) {
             logit("et","DataServer terminated connection; reason:%d (%s)\n",
                     terminate.reason, terminate.note );
          }
          else {
             logit("et","DataServer terminated connection; reason:%d\n",
                     terminate.reason );
          }
          nmxsrv_disconnect();
          return( -1 );
       }

    /* some kind of trouble with socket
     **********************************/
       else if( rctype == NMX_FAILURE )
       {
          logit( "et", "Trouble with socket to DataServer; disconnecting!\n" );
          nmxsrv_disconnect();
          return( -1 );
       }

    /* unknown message type or return code
     *************************************/
       else
       {
          int im;
          logit("et", "Unknown return (%d) from nmxsrv_recvmsg()\n",
                 rctype );
          logit("e","  naqshd sig: 0x%x  msgtype: %d  msglen: %d\n  ",
                     naqshd.signature, naqshd.msgtype, naqshd.msglen );
          for(im=0;im<naqshd.msglen;im++) logit("e","%d ", (int)SockBuf[im]);
          logit("e","\n" );
       }

   } /* end of while( ) */

   nmxsrv_disconnect();
   free( SockBuf  );
   free( PrecList );
   free( ChanList );

   return( 0 );
}


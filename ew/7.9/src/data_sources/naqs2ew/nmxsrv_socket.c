/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nmxsrv_socket.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/07/09 18:13:38  dietz
 *     Initial revision
 *
 *     Revision 1.2  2002/03/15 23:10:09  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/06/20 22:35:32  dietz
 *     Initial revision
 *
 *
 *
 */

   /*************************************************************
    *                     nmxsrv_socket.c                       *
    * Contains all the socket-related functions used by clients *
    * of Nanometrics programs such as NmxSrvServer & DataServer *
    *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <socket_ew.h>
#ifdef _WINNT
#include <winsock.h>
#else
#include <netdb.h>
#endif
#include "nmx_api.h"

static SOCKET   sd;            /* Socket identifier */

#define CONNECT_TIMEOUT  15    /* timeout (s) for connection attempts */
#define BLOCKING         -1    /* use blocking socket calls           */

/************************************************************
 *                     nmxsrv_connect()                     *
 * Open a socket connection to a Nanometrics Server program *
 ************************************************************/
int nmxsrv_connect( char *ipadr, int port )
{
   struct sockaddr_in server;      /* Server socket address structure */
   int addr;
   const int optVal = 1;
   int rc;
   struct hostent* hp;


/* Get a new socket descriptor
 *****************************/
   sd = socket_ew( AF_INET, SOCK_STREAM, 0 );
   if ( sd == INVALID_SOCKET )
   {
      logit( "et", "naqs2ew: socket_ew() error\n" );
      return NMX_FAILURE;
   }

/* Allow reuse of socket addresses
 *********************************/
   if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal,
                    sizeof(int) ) == SOCKET_ERROR )
   {
      logit( "et", "naqs2ew: setsockopt() error\n" );
      closesocket_ew( sd, SOCKET_CLOSE_SIMPLY_EW );
      return NMX_FAILURE;
   }

/* Fill in socket address structure
 **********************************/
   memset( (char *)&server, '\0', sizeof(server) );
   server.sin_family           = AF_INET;
   server.sin_port             = htons( (unsigned short)port );

   if ( (addr = inet_addr(ipadr)) != INADDR_NONE )
   {
      server.sin_addr.s_addr = addr;
   }
   else
   {       /* it's not a dotted quad IP address */
     if ( (hp = gethostbyname(ipadr)) == NULL)
     {
       logit("et", "naqs2ew: invalid ip address <%s>\n", ipadr);
       return NMX_FAILURE;
     }
     memcpy((void *) &server.sin_addr, (void*)hp->h_addr, hp->h_length);
   }


/* Connect to the server, with a timeout
 ***************************************/
   rc = connect_ew( sd, (struct sockaddr *)&server, sizeof(server),
                    CONNECT_TIMEOUT * 1000 );

   if( rc == 0 ) return NMX_SUCCESS;
   else          return NMX_FAILURE;
}


/************************************************************
 *                     nmxsrv_sendto                        *
 * Write a msg to a Nanometrics Server program via a socket *
 ************************************************************/
int nmxsrv_sendto( char *msg, int msglen )
{
   int    rc;

   rc = send_ew( sd, msg, msglen, 0, BLOCKING );
   if( rc != msglen )
   {
      logit("et", "nmxsrv_sendto: sent only %d bytes of %d-byte message\n",
             rc, msglen );
      return NMX_FAILURE;
   }
   /*logit("et", "nmxsrv_sendto: wrote %d byte message to socket\n",
          msglen );*/ /*DEBUG*/

   return NMX_SUCCESS;
}


/************************************************************
 *                 nmxsrv_recvconnecttime                   *
 * Receive a 4-byte time from a Nanometrics Server program  *
 ************************************************************/
int nmxsrv_recvconnecttime( int32_t *tconnect )
{
   int   rc;
   int32_t  tmp;
   int   len = sizeof(int32_t);

/* Read the connection time (4-byte int) from socket
 ***************************************************/
   rc = recv_all( sd, (char *)&tmp, len, 0, BLOCKING );
   if( rc != len )
   {
      logit("et", "nmxsrv_recvconnecttime: recv'd only %d bytes "
                  "(%d bytes expected)\n", rc, len );
      return NMX_FAILURE;
   }
   
  *tconnect = ntohl( tmp );

   return NMX_SUCCESS;
}


/************************************************************
 *                    nmxsrv_recvmsg                        *
 * Receive a NMX message from a Nanometrics Server program  *
 ************************************************************/
int nmxsrv_recvmsg( NMXHDR *nhd, char **buf, int *buflen )
{
   char           chd[NMXHDR_LEN];
   int            rc;

/* Read the NMX header, put it into a structure, validate it
 ************************************************************/
   rc = recv_all( sd, chd, NMXHDR_LEN, 0, BLOCKING );
   if( rc != NMXHDR_LEN )
   {
      logit("et", "nmxsrv_recvmsg: recv'd only %d bytes of %d-byte header\n",
             rc, NMXHDR_LEN );
      return NMX_FAILURE;
   }
   nmx_unloadhdr( nhd, chd );

   if( nhd->signature != NMXHDR_SIGNATURE )
   {
      logit("et","nmxsrv_recvmsg: bad header; first 4-bytes (0x%0x) "
            "do not contain expected signature (0x%0x)!\n",
             nhd->signature, NMXHDR_SIGNATURE );
      return NMX_FAILURE;
   }

/* Make sure the target address is big enough
 ********************************************/
   if( nmx_checklen( buf, buflen, nhd->msglen+1 ) != 0 )
   {
      logit("et","nmxsrv_recvmsg: could not increase buffer size to %d\n",
             nhd->msglen+1 );
      return NMX_FAILURE;
   }

/* Recv the message, given the length from the header
 ****************************************************/
   rc = recv_all( sd, *buf, nhd->msglen, 0, BLOCKING );
   if( rc != nhd->msglen )
   {
      logit("et", "nmxsrv_recvmsg: recv'd only %d bytes of %d-byte msg\n",
             rc, nhd->msglen );
      return NMX_FAILURE;
   }

/* Block commented below is recv_all the hard way */
/* ntoget = nhd->msglen;
   pbuf   = *buf;
   while( ntoget )
   {
     rc = recv_ew( sd, pbuf, ntoget, 0, BLOCKING );
     if( rc < 0 )
     {
        logit("et", "nmxsrv_recvmsg: trouble reading %d-byte message "
                    "from socket\n", nhd->msglen );
        return NMX_FAILURE;
     }
     ntoget -= rc;
     pbuf   += rc;
   }
*/

   (*buf)[nhd->msglen] = 0; /*null terminate for fun*/

   return nhd->msgtype;
}

/************************************************************
 *                  nmxsrv_disconnect                       *
 *           Close socket and clean up things               *
 ************************************************************/
int nmxsrv_disconnect( void )
{
   closesocket_ew( sd, SOCKET_CLOSE_GRACEFULLY_EW );
   return NMX_SUCCESS;
}


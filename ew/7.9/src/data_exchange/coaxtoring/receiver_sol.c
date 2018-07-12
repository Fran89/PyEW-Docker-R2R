
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: receiver_sol.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2000/08/08 17:31:08  lucky
 *     Lint cleanup
 *
 *     Revision 1.1  2000/02/14 16:16:56  lucky
 *     Initial revision
 *
 *
 */

         /********************************************************
          *                    receiver_sol.c                    *
          *                                                      *
          *             Functions to grab UDP packets.           *
          *                                                      *
          *  This is the Solaris version (Also works with OS/2)  *
          ********************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <earthworm.h>
#include <ew_packet.h>

#ifdef _OS2
#include <utils.h>
#endif

void SocketClose( SOCKET );         /* socket_ew.c  system-dependent */
static struct sockaddr_in name;
static SOCKET Soko;                 /* Socket identifier */


       /*******************************************************
        *                     socketInit                      *
        *                                                     *
        *  Returns -1 on error                                *
        *******************************************************/

int socketInit( char *inAddress,    /* IP address of data source */
                int inPortNumber,   /* Port number of data source */
                int RcvBufSize )    /* Size of the IP receive buffer */
{
   int length;
   const int optVal = 1;            /* Must be non-zero in Windows NT */

/* Open a socket
   *************/
   Soko = socket( AF_INET, SOCK_DGRAM, 0 );
   if ( Soko == -1 )
   {
      logit( "et", "coaxtoring: Can't open the socket.\n" );
      return -1;
   }

/* Reset the socket receive buffer size
   ************************************/
   if ( RcvBufSize > 0 )
   {
      if ( setsockopt( Soko, SOL_SOCKET, SO_RCVBUF, (char *)&RcvBufSize,
                           sizeof(int) ) == -1 )
      {
         logit( "et", "coaxtoring: Error resetting receive buffer size.\n" );
         logit( "et", "coaxtoring: %s\n", strerror(errno) );
         SocketClose( Soko );
         return -1;
      }
   }
   else                      /* or else use the system default */
   {
      int newsize;
      int optlen = sizeof(int);

      if ( getsockopt( Soko, SOL_SOCKET, SO_RCVBUF, (char *)&newsize,
                           &optlen ) == -1 )
      {
         logit( "et", "coaxtoring: Error getting receive buffer size.\n" );
         logit( "et", "coaxtoring: %s\n", strerror(errno) );
         SocketClose( Soko );
         return -1;
      }
      logit( "et", "coaxtoring: Using system default receive buffer size (%d)\n",
             newsize );
   }

/* Allow reuse of socket addresses
   *******************************/
   if ( setsockopt( Soko, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal,
           sizeof(int) ) == -1 )
   {
      logit( "et", "coaxtoring: Error permitting socket address reuse.\n" );
      logit( "et", "coaxtoring: %s\n", strerror(errno) );
      SocketClose( Soko );
      return -1;
   }

/* Fill in server's socket address structure
   *****************************************/
   memset( (char *) &name, '\0', sizeof(name) );
   name.sin_family      = AF_INET;
   name.sin_port        = htons( (unsigned short) inPortNumber );
   name.sin_addr.s_addr = inet_addr( inAddress );

/* Bind the socket to the port number
   **********************************/
   if ( bind( Soko, (struct sockaddr *)&name, sizeof(name) ) == -1 )
   {
      logit( "et", "coaxtoring: Can't bind address to socket.\n" );
      logit( "et", "coaxtoring: %s\n", strerror(errno) );
      SocketClose( Soko );
      return -1;
   }

/* Log the assigned address and port number
   ****************************************/
   length = sizeof( name );
   if ( getsockname( Soko, (struct sockaddr *)&name, &length ) == -1 )
   {
      logit( "et", "coaxtoring: Error getting socket name.\n" );
      SocketClose( Soko );
      return -1;
   }
   return 0;
}


       /******************************************************
        *                      receiver                      *
        *                 Gets one UDP packet.               *
        *                                                    *
        *  Returns -1 if an error occurred.                  *
        *  Otherwise, returns the number of bytes received.  *
        ******************************************************/

int receiver( PACKET *packet )
{
   static struct sockaddr_in source;
   int    lenSource = sizeof( source );

   int rc = recvfrom( Soko, (char *)packet, UDP_SIZ, 0,
                (struct sockaddr *)&source, &lenSource );

   if ( rc == -1 )
      printf( "coaxtoring: recvfrom() failed. errno: %d\n", errno );

   return rc;
}


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: receiver_nt.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:16:56  lucky
 *     Initial revision
 *
 *
 */

         /****************************************************************
          *                         receiver_nt.c                        *
          *                                                              *
          *                  Functions to grab UDP packets.              *
          *                                                              *
          *  This is the Windows NT (Winsock) version.                   *
          *  It uses Windows-specific constants.                         *
          *  Winsock handles are of type SOCKET (unsigned), whereas      *
          *  Solaris sockets are of type int (signed).                   *
          *  Another difference between Windows and Solaris:             *
          *     In Windows, inAddress must be the complete local IP      *
          *     address eg 192.168.4.107                                 *
          *     In Solaris, inAddress must be of the form 192.168.4.255  *
          ****************************************************************/

#include <stdio.h>
#include <windows.h>
#include <earthworm.h>
#include <ew_packet.h>

void   SocketClose( SOCKET );       /* socket_ew.c  system-dependent */
static SOCKET Soko;                 /* Socket identifier */


       /*******************************************************
        *                     socketInit                      *
        *  Returns: -1 if an error occurred; 0 if no error    *
        *******************************************************/

int socketInit( char *inAddress,    /* IP address of data source */
                int inPortNumber,   /* Port number of data source */
                int RcvBufSize )    /* Size of the IP receive buffer */
{
   int length;
   const int optVal = 1;    /* optVal must be non-zero in Windows NT */
   struct sockaddr_in name;

/* Open a socket
   *************/
   Soko = socket( AF_INET, SOCK_DGRAM, 0 );
   if ( Soko == INVALID_SOCKET )
   {
      int rc = WSAGetLastError();
      logit( "et", "coaxtoring: socket() error. Error code: %d\n", rc );
      return -1;
   }

/* Reset the socket receive buffer size
   ************************************/
   if ( RcvBufSize > 0 )
   {
      if ( setsockopt( Soko, SOL_SOCKET, SO_RCVBUF, (char *)&RcvBufSize,
                           sizeof(int) ) == SOCKET_ERROR )
      {
         logit( "et", "coaxtoring: setsockopt() SO_RCVBUF error %d\n",
                WSAGetLastError() );
         SocketClose( Soko );
         return -1;
      }
   }
   else                      /* or else use the system default */
   {
      int newsize;
      int optlen = sizeof(int);

      if ( getsockopt( Soko, SOL_SOCKET, SO_RCVBUF, (char *)&newsize,
                           &optlen ) == SOCKET_ERROR )
      {
         logit( "et", "coaxtoring: getsockopt() SO_RCVBUF error %d\n",
                WSAGetLastError() );
         SocketClose( Soko );
         return -1;
      }
      logit( "et", "coaxtoring: Using system default receive buffer size (%d)\n",
             newsize );
   }

/* Allow reuse of socket addresses
   *******************************/
   if ( setsockopt( Soko, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal,
                    sizeof(int) ) == SOCKET_ERROR )
   {
      logit( "et", "coaxtoring: setsockopt() SO_REUSEADDR error %d\n",
             WSAGetLastError() );
      SocketClose( Soko );
      return -1;
   }

/* Fill in server's socket address structure.
   inAddress is the complete local IP address,
   eg 192.168.4.107
   *******************************************/
   memset( (char *) &name, '\0', sizeof(name) );
   name.sin_family      = AF_INET;
   name.sin_port        = htons( (unsigned short) inPortNumber );
   name.sin_addr.s_addr = inet_addr( inAddress );

/* Bind the socket to the port number
   **********************************/
   if ( bind( Soko, (struct sockaddr *)&name, sizeof(name) )
        == SOCKET_ERROR )
   {
      int rc = WSAGetLastError();
      logit( "et", "coaxtoring: bind() error. Error code: %d\n", rc );

      if ( rc == WSAENETDOWN )
         logit( "et", "coaxtoring: Network is down.\n" );
      if ( rc == WSAEADDRINUSE )
         logit( "et", "coaxtoring: Address already in use.\n" );
      if ( rc == WSAEFAULT )
         logit( "et", "coaxtoring: Bad address.\n" );
      if ( rc == WSAEADDRNOTAVAIL )
         logit( "et", "coaxtoring: Cannot assign requested address.\n" );

      SocketClose( Soko );
      return -1;
   }

/* Log the assigned address and port number
   ****************************************/
   length = sizeof( name );
   if ( getsockname( Soko, (struct sockaddr *)&name, &length )
      == SOCKET_ERROR )
   {
      int rc = WSAGetLastError();
      logit( "et", "coaxtoring: getsockname() error. Error code: %d\n", rc );
      SocketClose( Soko );
      return -1;
   }
   logit( "et", "coaxtoring: Receiving from %s  Port: %u\n",
          inet_ntoa( name.sin_addr ), htons(name.sin_port) );
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

   if ( rc == SOCKET_ERROR )
   {
      int rcWSA = WSAGetLastError();
      logit( "et", "coaxtoring: recvfrom() error. Error code: %d\n", rcWSA );
      return -1;
   }

   return rc;
}

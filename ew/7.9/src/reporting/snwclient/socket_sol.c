

      /*************************************************************
       *                        socket_sol.c                       *
       *                                                           *
       *  This file contains functions that are specific to        *
       *  Solaris.                                                 *
       *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/filio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "snwclient.h"

void log( char *, char *, ... );

static int sd;                  /* Socket descriptor */


      /**********************************************************
       *                     SocketSysInit()                    *
       *  Dummy in Solaris.                                     *
       **********************************************************/
void SocketSysInit( void )
{
   return;
}


      /**********************************************************
       *                 CloseSocketConnection()                *
       **********************************************************/
void CloseSocketConnection( void )
{
   close( sd );
   return;
}


      /************************************************************
       *                    ConnectToSNWAgent()                   *
       *                                                          *
       *          Get a connection to the remote system.          *
       ************************************************************/

int ConnectToSNWAgent( void )
{
   extern char ServerIP[20];     /* IP address of system to receive msg's */
   extern int  ServerPort;       /* The well-known port number */
   struct sockaddr_in server;    /* Server socket address structure */
   const int optVal = 1;
   unsigned long address;
   unsigned long lOnOff=1;

/* Get a new socket descriptor
   ***************************/
   sd = socket( AF_INET, SOCK_STREAM, 0 );
   if ( sd == -1 )
   {
      log( "et", "socket() error: %s\n", strerror(errno) );
      return SENDFILE_FAILURE;
   }

/* Get the send buffer size
   ************************/
/* {
      int sendBufSize;
      int optLen = sizeof(int);
      if ( getsockopt( sd, SOL_SOCKET, SO_SNDBUF, (char *)&sendBufSize,
                       &optLen ) == -1 )
      {
         log( "et", "getsockopt() error: %s\n", strerror(errno) );
         close( sd );
         return SENDFILE_FAILURE;
      }
      log( "et", "sendBufSize: %d\n", sendBufSize );
   } */

/* Allow reuse of socket addresses
   *******************************/
   if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal,
                    sizeof(int) ) == -1 )
   {
      log( "et", "setsockopt() error: %s\n", strerror(errno) );
      close( sd );
      return SENDFILE_FAILURE;
   }

/* Fill in socket address structure
   ********************************/
   address = inet_addr( ServerIP );
   if ( address == -1 )
   {
      log( "et", "Bad server IP address: %s  Exiting.\n", ServerIP );
      close( sd );
      return SENDFILE_FAILURE;
   }
   memset( (char *)&server, '\0', sizeof(server) );
   server.sin_family = AF_INET;
   server.sin_port   = htons((unsigned short)ServerPort);
   server.sin_addr.S_un.S_addr = address;

/* Connect to the server.
   The connect call blocks if SNWCollectionAgent is not available.
   ***************************************************************/
   log( "e", "\n" );
   log( "et", "Connecting to %s  port %d\n", ServerIP, ServerPort );
   if ( connect( sd, (struct sockaddr *)&server, sizeof(server) ) == -1 )
   {
      log( "et", "connect() error: %s\n", strerror(errno) );
      close( sd );
      return SENDFILE_FAILURE;
   }

/* Set socket to non-blocking mode
   *******************************/
  /*
   */
   if ( ioctl( sd, FIONBIO, &lOnOff ) == -1 )
   {
      log( "et", "ioctl() FIONBIO error: %s\n", strerror(errno) );
      close( sd );
      return SENDFILE_FAILURE;
   }
   return SENDFILE_SUCCESS;
}


  /*********************************************************************
   *                              Send_all()                           *
   *                                                                   *
   *  Sends the first nbytes characters in buf to the socket sock.     *
   *  The data is sent in blocks of up to 4096 bytes.                  *
   *  The function will time out if the data can't be sent within      *
   *  TimeOut seconds.                                                 *
   *                                                                   *
   *  Returns 0 if all ok.                                             *
   *  Returns -1 if an error or timeout occurred.                      *
   *********************************************************************/

int Send_all( int sock, char buf[], int nbytes )
{
   extern int TimeOut;             /* Send/receive timeout, in seconds */
   int    nBytesToWrite = nbytes;
   int    nwritten      = 0;

      int    selrc;
      fd_set writefds;
      struct timeval selectTimeout;

/* See if the socket is writable.  If the socket is not
   writeable within TimeOut seconds, return an error.
   ****************************************************/
      selectTimeout.tv_sec  = TimeOut;
      selectTimeout.tv_usec = 0;
      FD_ZERO( &writefds );
      FD_SET( sock, &writefds );
      selrc = select( sock+1, 0, &writefds, 0, &selectTimeout );
      if ( selrc == -1 )
      {
         log( "et", "select() error: %s", strerror(errno) );
         return -1;
      }
      if ( selrc == 0 )
      {
         log( "et", "select() timed out\n" );
         return -1;
      }
      if ( FD_ISSET( sock, &writefds ) )    /* A writeable event occurred */
      {
         int rc;
         int sockerr;
         int optlen = sizeof(int);

/* send() will crash if a socket error occurs
   and we don't detect it using getsockopt.
   ******************************************/
         if ( getsockopt( sock, SOL_SOCKET, SO_ERROR, (char *)&sockerr,
              &optlen ) == -1 )
         {
            log( "et", "getsockopt() error: %s", strerror(errno) );
            return -1;
         }
         if ( sockerr != 0 )
         {
            log( "et", "Error detected by getsockopt(): %s\n",
                     strerror(sockerr) );
            return -1;
         }

/* Send the data.  It's unlikely that send() would block,
   since we know that a writeable event occurred.
   *****************************************************/
         rc = send( sock, &buf[nwritten], nBytesToWrite, 0 );
         if ( rc == -1 )
         {
            if ( errno == EWOULDBLOCK )    
            {
               log( "et", "send() would block\n" );
               return -1;
            }
            else
            {
               log( "et", "send() error: %d", strerror(errno) );
               return -1;
            }
         }
         if ( rc == 0 )           
         {
            log( "et", "Error: send() == 0\n" );
            return -1;
         }
         nwritten += rc;
         nBytesToWrite -= rc;
      }
   
   return 0;
}


     /****************************************************************
      *                      SendBlockToSocket()                     *
      *                                                              *
      *  Send a block to the remote system.                          *
      *  Blocks can be from 1 to 999999 bytes long.                  *
      *  A zero-length block is sent when the transmission is over.  *
      ****************************************************************/

int SendBlockToSocket( char *buf, int blockSize )
{

/* Sanity check
   ************/
   if ( blockSize > 999999 )
   {
      log( "et", "Error. Block is too large to send.\n" );
      return SENDFILE_FAILURE;
   }

/* Send block bytes
   ****************/
   if ( Send_all( sd, buf, blockSize ) == -1 )
   {
      log( "et", "Send_all() error.\n" );
      return SENDFILE_FAILURE;
   }
   return SENDFILE_SUCCESS;
}

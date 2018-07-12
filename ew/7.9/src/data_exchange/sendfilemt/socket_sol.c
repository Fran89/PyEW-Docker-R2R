
/*********************************************************
 *                      socket_sol.c                     *
 *                                                       *
 *  These functions that are specific to Solaris.        *
 *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/filio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <earthworm.h>
#include "sendfilemt.h"


/*******************************************************
 *                    SocketSysInit                    *
 *                                                     *
 *             Dummy function in Solaris.              *
 *******************************************************/

void SocketSysInit( void )
{
    return;
}


/********************************************************
 *                  ConnectToGetfile()                  *
 *                                                      *
 *        Get a connection to the remote system.        *
 ********************************************************/

int ConnectToGetfile( char ServerIP[], int ServerPort, int *sd )
{
   struct sockaddr_in server;    // Server socket address structure
   const int optVal = 1;
   unsigned long address;
   unsigned long lOnOff = 1;

   signal(SIGPIPE, SIG_IGN);

/* Get a new socket descriptor
   ***************************/
   *sd = socket( AF_INET, SOCK_STREAM, 0 );
   if ( *sd == -1 )
   {
      logit( "et", "socket() error: %s\n", strerror(errno) );
      return SENDFILE_FAILURE;
   }

/* Fill in socket address structure
   ********************************/
   address = inet_addr( ServerIP );
   if ( address == -1 )
   {
      logit( "et", "Bad server IP address: %s  Exiting.\n", ServerIP );
      CloseSocketConnection( *sd );
      return SENDFILE_FAILURE;
   }
   memset( (char *)&server, '\0', sizeof(server) );
   server.sin_family = AF_INET;
   server.sin_port   = htons((unsigned short)ServerPort);
   server.sin_addr.s_addr = address;

/* Connect to the getfileII server
   *******************************/
   if ( connect( *sd, (struct sockaddr *)&server, sizeof(server) ) == -1 )
   {
//    logit( "et", "connect() error: %s\n", strerror(errno) );
      CloseSocketConnection( *sd );
      return SENDFILE_FAILURE;
   }

/* Set socket to non-blocking mode
   *******************************/
   if ( ioctl( *sd, FIONBIO, &lOnOff ) == -1 )
   {
      logit( "et", "ioctl() FIONBIO error: %s\n", strerror(errno) );
      CloseSocketConnection( *sd );
      return SENDFILE_FAILURE;
   }
   return SENDFILE_SUCCESS;
}


/*********************************************************************
 *                            Send_all()                             *
 *                                                                   *
 *  Sends the first nbytes characters in buf to socket sd.           *
 *  The data is sent in blocks of up to 4096 bytes.                  *
 *  The function will time out if the data can't be sent within      *
 *  TimeOut seconds.                                                 *
 *                                                                   *
 *  Returns 0 if all ok.                                             *
 *  Returns -1 if an error or timeout occurred.                      *
 *********************************************************************/

int Send_all( int sd, char buf[], int nbytes )
{
   extern int TimeOut;             // Send/receive timeout, in seconds
   int    nBytesToWrite = nbytes;
   int    nwritten      = 0;

   while ( nBytesToWrite > 0 )
   {
      int    selrc;
      fd_set writefds;
      struct timeval selectTimeout;

/* See if the socket is writable.  If the socket is not
   writeable within TimeOut seconds, return an error.
   ****************************************************/
      selectTimeout.tv_sec  = TimeOut;
      selectTimeout.tv_usec = 0;
      FD_ZERO( &writefds );
      FD_SET( sd, &writefds );
      selrc = select( sd+1, 0, &writefds, 0, &selectTimeout );
      if ( selrc == -1 )
      {
         logit( "et", "select() error: %s", strerror(errno) );
         return -1;
      }
      if ( selrc == 0 )
      {
         logit( "et", "select() timed out\n" );
         return -1;
      }
      if ( FD_ISSET( sd, &writefds ) )    // A writeable event occurred
      {
         int rc;
         int sockerr;
         int optlen = sizeof(int);

/* send() will crash if a socket error occurs
   and we don't detect it using getsockopt.
   ******************************************/
         if ( getsockopt( sd, SOL_SOCKET, SO_ERROR, (char *)&sockerr,
                          &optlen ) == -1 )
         {
            logit( "et", "getsockopt() error: %s", strerror(errno) );
            return -1;
         }
         if ( sockerr != 0 )
         {
            logit( "et", "Error detected by getsockopt(): %s\n",
                 strerror(sockerr) );
            return -1;
         }

/* Send the data.  It's unlikely that send() would block,
   since we know that a writeable event occurred.
   *****************************************************/
         rc = send( sd, &buf[nwritten], nBytesToWrite, 0 );
         if ( rc == -1 )
         {
            if ( errno == EWOULDBLOCK )    // Unlikely
            {
               logit( "et", "send() would block\n" );
               continue;
            }
            else
            {
               logit( "et", "send() error: %d", strerror(errno) );
               return -1;
            }
         }
         if ( rc == 0 )           // Shouldn't see this
         {
            logit( "et", "Error: send() == 0\n" );
            return -1;
         }
         nwritten += rc;
         nBytesToWrite -= rc;
      }
   }
   return 0;
}


/*********************************************************************
 *                            Recv_all()                             *
 *                                                                   *
 *  Reads nbyte characters from socket sd into buf.                  *
 *  The function will time out if the data can't be read within      *
 *  TimeOut seconds.                                                 *
 *                                                                   *
 *  Returns 0 if all ok.                                             *
 *  Returns -1 if an error or timeout occurred.                      *
 *********************************************************************/

int Recv_all( int sd, char buf[], int nbytes, int TimeOut )
{
   int nBytesToRead = nbytes;
   int nread        = 0;

   while ( nBytesToRead > 0 )
   {
      int    selrc;
      fd_set readfds;
      struct timeval selectTimeout;

/* See if the socket is readable.  If the socket is not
   readeable within TimeOut seconds, return an error.
   ****************************************************/
      selectTimeout.tv_sec  = TimeOut;
      selectTimeout.tv_usec = 0;
      FD_ZERO( &readfds );
      FD_SET( sd, &readfds );
      selrc = select( sd+1, &readfds, 0, 0, &selectTimeout );
      if ( selrc == -1 )
      {
         logit( "et", "select() error: %s", strerror(errno) );
         return -1;
      }
      if ( selrc == 0 )
      {
         logit( "et", "select() timed out\n" );
         return -1;
      }
      if ( FD_ISSET( sd, &readfds ) )    // A readable event occurred
      {
         int rc;
         int sockerr;
         int optlen = sizeof(int);

/* Check for socket errors using getsockopt()
   ******************************************/
         if ( getsockopt( sd, SOL_SOCKET, SO_ERROR, (char *)&sockerr,
                          &optlen ) == -1 )
         {
            logit( "et", "getsockopt() error: %s", strerror(errno) );
            return -1;
         }
         if ( sockerr != 0 )
         {
            logit( "et", "Error detected by getsockopt(): %s\n",
                 strerror(sockerr) );
            return -1;
         }

/* Get the data.  It's unlikely that recv() would block,
   since we know that a readable event occurred.
   ****************************************************/
         rc = recv( sd, &buf[nread], nBytesToRead, 0 );
         if ( rc == -1 )
         {
            if ( errno == EWOULDBLOCK )      // Unlikely
                continue;
            else
            {
               logit( "et", "recv() error: %s", strerror(errno) );
               return -1;
            }
         }
         if ( rc == 0 )
         {
            logit( "et", "Error. recv()==0\n" );
            return -1;
         }
         nread += rc;
         nBytesToRead -= rc;
      }
   }
   return 0;
}


/**************************************************
 *             CloseSocketConnection()            *
 **************************************************/

void CloseSocketConnection( int sd )
{
   if ( close(sd) != 0 )
      logit( "et", "Error closing socket: %s\n", strerror(errno));
   return;
}



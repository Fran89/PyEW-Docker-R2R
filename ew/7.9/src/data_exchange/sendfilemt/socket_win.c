
/*********************************************************
 *                     socket_win.c                      *
 *                                                       *
 *  These functions that are specific to Windows.        *
 *********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <earthworm.h>
#include "sendfilemt.h"


/*******************************************************
 *                    SocketSysInit                    *
 *                                                     *
 *     Initialize the socket system using Windows      *
 *     socket version 2.2.                             *
 *******************************************************/

void SocketSysInit( void )
{
    WSADATA Data;
    int     status = WSAStartup( MAKEWORD(2,2), &Data );
    if ( status != 0 )
    {
        logit( "et", "WSAStartup failed.\n" );
        logit( "et", "Can't initialize socket system.\n" );
        logit( "et", "sendfilemt: Exiting.\n" );
        exit( -1 );
    }
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

/* Get a new socket descriptor
   ***************************/
    *sd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( *sd == INVALID_SOCKET )
    {
       logit( "et", "socket() error: %d\n", WSAGetLastError() );
       return SENDFILE_FAILURE;
    }

/* Fill in socket address structure
   ********************************/
    address = inet_addr( ServerIP );
    if ( address == INADDR_NONE )
    {
       logit( "et", "Bad server IP address: %s  Exiting.\n", ServerIP );
       CloseSocketConnection( *sd );
       return SENDFILE_FAILURE;
    }
    memset( (char *)&server, '\0', sizeof(server) );
    server.sin_family = AF_INET;
    server.sin_port   = htons( (unsigned short)ServerPort );
    server.sin_addr.S_un.S_addr = address;

/* Connect to the getfileII server
   *******************************/
    if ( connect(*sd, (struct sockaddr *)&server, sizeof(server))
         == SOCKET_ERROR )
    {
//     int rc = WSAGetLastError();
//     if ( rc == WSAETIMEDOUT )
//         logit( "et", "connect() timed out.\n" );
//     else
//         logit( "et", "connect() error: %d\n", rc );
       CloseSocketConnection( *sd );
       return SENDFILE_FAILURE;
    }

/* Set socket to non-blocking mode
   *******************************/
    if ( ioctlsocket(*sd, FIONBIO, &lOnOff) == SOCKET_ERROR )
    {
        logit( "et", "Error %d, changing socket to non-blocking mode\n",
             WSAGetLastError() );
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
      if ( selrc == SOCKET_ERROR )
      {
          logit( "et", "select() error: %d", WSAGetLastError() );
          return -1;
      }
      if ( selrc == 0 )
      {
          logit( "et", "select() timed out\n" );
          return -1;
      }
      if ( FD_ISSET( sd, &writefds ) )    // A writeable event occurred
      {

/* Send the data
   *************/
         int rc = send( sd, &buf[nwritten], nBytesToWrite, 0 );
         if ( rc == SOCKET_ERROR )
         {
            int winError = WSAGetLastError();
            if ( winError == WSAEWOULDBLOCK )
               continue;
            else if ( winError == WSAECONNRESET )
            {
               logit( "et", "send() error: Connection reset by peer.\n" );
               return -1;
            }
            else
            {
               logit( "et", "send() error: %d\n", winError );
               return -1;
            }
         }
         if ( rc == 0 )     /* Shouldn't see this */
         {
            logit( "et", "Error: send()== 0\n" );
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
 *  Reads nbyte characters from socket sock into buf.                *
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
      if ( selrc == SOCKET_ERROR )
      {
         logit( "et", "select() error: %d", WSAGetLastError() );
         return -1;
      }
      if ( selrc == 0 )
      {
         logit( "et", "select() timed out\n" );
         return -1;
      }
      if ( FD_ISSET( sd, &readfds ) )    // A readable event occurred
      {
         int rc = recv( sd, &buf[nread], nBytesToRead, 0 );
         if ( rc == SOCKET_ERROR )
         {
            int winError = WSAGetLastError();

            if ( winError == WSAEWOULDBLOCK )
                continue;
            else if ( winError == WSAECONNRESET )
            {
               logit( "et", "recv() error: Connection reset by peer.\n" );
               return -1;
            }
            else
            {
               logit( "et", "recv() error: %d\n", winError );
               return -1;
            }
         }
         if ( rc == 0 )
         {
            logit( "et", "Error: recv()==0\n" );
            return -1;
         }
         nread += rc;
         nBytesToRead -= rc;
      }
   }
   return 0;
}


/*************************************************
 *            CloseSocketConnection()            *
 *************************************************/

void CloseSocketConnection( int sd )
{
   if ( closesocket(sd) != 0 )
      logit( "et", "Error closing socket\n" );
   return;
}


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rb_win.c 570 2001-04-26 22:30:55Z kohler $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2001/04/26 22:30:55  kohler
 *     Port changed from 23 to 7000.
 *
 *     Revision 1.1  2001/04/25 23:44:07  kohler
 *     Initial revision
 *
 *
 *
 */
   /*************************************************************
    *                         rb_win.c                          *
    *                                                           *
    *  This file contains functions that are specific to        *
    *  Windows.                                                 *
    *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <earthworm.h>

static SOCKET sd;                      /* Socket identifier */
extern int Quiet;


      /***********************************************************
       *                     SocketSysInit()                     *
       *               Initialize the socket system              *
       *         We are using Windows socket version 2.2.        *
       ***********************************************************/

void SocketSysInit( void )
{
   WSADATA Data;
   int     status = WSAStartup( MAKEWORD(2,2), &Data );
   if ( status != 0 )
   {
      if ( !Quiet ) printf( "WSAStartup failed. Exiting.\n" );
      exit( -1 );
   }
   return;
}


       /*********************************************************
        *                CloseSocketConnection()                *
        *********************************************************/

void CloseSocketConnection( void )
{
   closesocket( sd );
   return;
}


       /************************************************************
        *                      ConnectToMSS()                      *
        *                                                          *
        *         Get a socket connection to the MSS100.           *
        *                                                          *
        *  Returns  0 if the connection was successful             *
        *          -1 if an error occurred                         *
        ************************************************************/

int ConnectToMSS( char ServerIP[] )
{
   int  ServerPort = 7000;       /* The MSS100 remote console port */
   struct sockaddr_in server;    /* Server socket address structure */
   const int optVal = 1;
   unsigned long address;

/* Get a new socket descriptor
   ***************************/
   sd = socket( AF_INET, SOCK_STREAM, 0 );
   if ( sd == INVALID_SOCKET )
   {
      if ( !Quiet ) printf( "socket() error: %d\n", WSAGetLastError() );
      return -1;
   }

/* Allow reuse of socket addresses
   *******************************/
// if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal,
//                  sizeof(int) ) == SOCKET_ERROR )
// {
//    if ( !Quiet ) printf( "setsockopt() error: %d\n", WSAGetLastError() );
//    CloseSocketConnection();
//    return -1;
// }

/* Fill in socket address structure
   ********************************/
   address = inet_addr( ServerIP );
   if ( address == INADDR_NONE )
   {
      if ( !Quiet ) printf( "Bad server IP address: %s  Exiting.\n", ServerIP );
      CloseSocketConnection();
      return -1;
   }
   memset( (char *)&server, '\0', sizeof(server) );
   server.sin_family = AF_INET;
   server.sin_port   = htons( (unsigned short)ServerPort );
   server.sin_addr.S_un.S_addr = address;

/* Connect to the server.
   The connect call will block for a while.
   ***************************************/
   if ( connect( sd, (struct sockaddr *)&server, sizeof(server) )
        == SOCKET_ERROR )
   {
      int rc = WSAGetLastError();
      if ( rc == WSAECONNREFUSED )
         if ( !Quiet ) printf( "Connection refused.\n" );
      else if ( rc == WSAETIMEDOUT )
         if ( !Quiet ) printf( "Connection timed out.\n" );
      else
         if ( !Quiet ) printf( "connect() error: %d\n", rc );

      CloseSocketConnection();
      return -1;
   }
   return 0;     /* Success */
}


  /*********************************************************************
   *                            SendToMSS()                            *
   *                                                                   *
   *      Sends the first nbytes characters in buf to the socket.      *
   *                                                                   *
   *  Returns  0 if all ok.                                            *
   *          -1 if an error occurred.                                 *
   *********************************************************************/

int SendToMSS( char buf[], int nbytes )
{
   int nBytesToWrite = nbytes;
   int nwritten      = 0;

   while ( nBytesToWrite > 0 )
   {
      int rc = send( sd, &buf[nwritten], nBytesToWrite, 0 );

      if ( rc == SOCKET_ERROR )
      {
         int winError = WSAGetLastError();

         if ( winError == WSAECONNRESET )
         {
            if ( !Quiet ) printf( "send() error: Connection reset by peer.\n" );
            return -1;
         }
         else if ( winError == WSAECONNABORTED )
         {
            if ( !Quiet ) printf( "send() error: Connection aborted.\n" );
            return -1;
         }
         else
         {
            if ( !Quiet ) printf( "send() error: %d\n", winError );
            return -1;
         }
      }
      if ( rc == 0 )     /* We should never see this */
      {
         if ( !Quiet ) printf( "Error: send()== 0\n" );
         return -1;
      }
      nwritten += rc;
      nBytesToWrite -= rc;
   }
   return 0;
}


       /****************************************************************
        *                          GetFromMSS()                        *
        *                                                              *
        *             Get a block of data from the MSS100.             *
        *                                                              *
        *  Returns  0 if all ok                                        *
        *          -1 if an error occurred                             *
        ****************************************************************/

int GetFromMSS( char *buf, int bufSize, int *nBytesReceived )
{
   int    selrc;
   fd_set readfds;
   struct timeval selectTimeout;

/* See if the socket is readable
   *****************************/
   selectTimeout.tv_sec  = 0;         /* Do not wait for timeout */
   selectTimeout.tv_usec = 0;
   FD_ZERO( &readfds );
   FD_SET( sd, &readfds );

   selrc = select( sd+1, &readfds, 0, 0, &selectTimeout );
   if ( selrc == SOCKET_ERROR )
   {
      if ( !Quiet ) printf( "select() error: %d", WSAGetLastError() );
      return -1;
   }
   if ( selrc == 0 )                  /* No readable events occurred on socket */
   {
      *nBytesReceived = 0;
      return 0;                       /* Success */
   }

/* A readable event occurred
   *************************/
   if ( FD_ISSET( sd, &readfds ) )
   {
      int rc = recv( sd, buf, bufSize, 0 );

/* We got some bytes
   *****************/
      if ( rc > 0 )
      {
         *nBytesReceived = rc;
         return 0;                    /* Success */
      }

/* Did the MSS100 close the socket connection?
   ******************************************/
      else if ( rc == 0 )
      {
         if ( !Quiet ) printf( "The MSS100 closed the socket connection.\n" );
         return -1;
      }

/* Error detected by recv()
   ************************/
      else if ( rc == SOCKET_ERROR )
      {
         int winError = WSAGetLastError();
         if ( !Quiet ) printf( "recv() error: %d", winError );
         return -1;
      }
      else              /* We should never see this */
      {
         if ( !Quiet ) printf( "Unknown return code from recv(): %d", rc );
         return -1;
      }
   }

   else                 /* We should never see this */
   {
      if ( !Quiet ) printf( "A nonreadable event occurred (?????).\n" );
      return -1;
   }

   if ( !Quiet ) printf( "Unknown return from select(). selrc: %d\n", selrc );
   return -1;
}


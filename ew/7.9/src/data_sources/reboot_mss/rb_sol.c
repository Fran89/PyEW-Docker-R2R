
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rb_sol.c 3762 2010-01-02 00:39:51Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2010/01/02 00:39:51  kress
 *     bookeeping on gcf2ew.  reboot_mss and reboot_mss_ew now both work in linux.
 *
 *     Revision 1.2  2001/04/26 22:30:22  kohler
 *     Port changed from 23 to 7000.
 *
 *     Revision 1.1  2001/04/25 23:43:57  kohler
 *     Initial revision
 *
 *
 *
 */
      /*************************************************************
       *                          rb_sol.c                         *
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
#include <netdb.h>

static int sd;                  /* Socket descriptor */
extern int Quiet;


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
       *                      ConnectToMSS()                      *
       *                                                          *
       *             Get a connection to the MSS100.              *
       *                                                          *
       *  Returns  0 if the connection was successful             *
       *          -1 if an error occurred                         *
       ************************************************************/

int ConnectToMSS( char ServerIP[] )
{
   int  ServerPort = 7000;       /* The MSS100 remote console port */
   struct sockaddr_in server;    /* Server socket address structure */
   struct hostent *hostEnt;
   const int optVal = 1;
   unsigned long *addrp;
   unsigned long lOnOff=1;

/* Get a new socket descriptor
   ***************************/
   sd = socket( AF_INET, SOCK_STREAM, 0 );
   if ( sd == -1 )
   {
      if ( !Quiet ) printf( "socket() error: %s\n", strerror(errno) );
      return -1;
   }

/* Allow reuse of socket addresses
   *******************************/
/* if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal,
                    sizeof(int) ) == -1 )
   {
      if ( !Quiet ) printf( "setsockopt() error: %s\n", strerror(errno) );
      close( sd );
      return -1;
   } */

/* Fill in socket address structure
   ********************************/
   hostEnt=gethostbyname(ServerIP);
   if (hostEnt == NULL){
     if ( !Quiet ) printf( "Bad server IP address: %s  Exiting.\n", ServerIP );
     close( sd );
     return -1;
   }
   addrp=(unsigned long *)hostEnt->h_addr;
   memset( (char *)&server, '\0', sizeof(server) );
   server.sin_family = AF_INET;
   server.sin_port   = htons((unsigned short)ServerPort);
   server.sin_addr.S_un.S_addr = *addrp;

/* Connect to the server.
   The connect call will block for a while.
   ***************************************/
   if ( connect( sd, (struct sockaddr *)&server, sizeof(server) ) == -1 )
   {
      if ( !Quiet ) printf( "connect() error: %s\n", strerror(errno) );
      close( sd );
      return -1;
   }
   return 0;     /* Success */
}


  /*********************************************************************
   *                             SendToMSS()                           *
   *                                                                   *
   *  Sends the first nbytes characters in buf to the socket.          *
   *                                                                   *
   *  Returns  0 if all ok.                                            *
   *  Returns -1 if an error occurred.                                 *
   *********************************************************************/

int SendToMSS( char buf[], int nbytes )
{
   int    nBytesToWrite = nbytes;
   int    nwritten      = 0;

   while ( nBytesToWrite > 0 )
   {
      int rc = send( sd, &buf[nwritten], nBytesToWrite, 0 );

      if ( rc == -1 )
      {
         if ( !Quiet ) printf( "send() error: %d", strerror(errno) );
         return -1;
      }
      if ( rc == 0 )        /* We should never see this */
      {
         if ( !Quiet ) printf( "Error: send() == 0\n" );
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
   if ( selrc == -1 )
   {
      if ( !Quiet ) printf( "select() error: %s", strerror(errno) );
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
      int rc;
      int sockerr;
      int optlen = sizeof(int);

/* Check for socket errors using getsockopt()
   ******************************************/
      if ( getsockopt( sd, SOL_SOCKET, SO_ERROR, (char *)&sockerr,
           &optlen ) == -1 )
      {
         if ( !Quiet ) printf( "getsockopt() error: %s", strerror(errno) );
         return -1;
      }
      if ( sockerr != 0 )
      {
         if ( !Quiet ) printf( "Error detected by getsockopt(): %s\n",
                  strerror(sockerr) );
         return -1;
      }

/* Get the data.  It's unlikely that recv() would block,
   since we know that a readable event occurred.
   ****************************************************/
      rc = recv( sd, buf, bufSize, 0 );

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
      else if ( rc == -1 )
      {
         if ( !Quiet ) printf( "recv() error: %s", strerror(errno) );
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
}


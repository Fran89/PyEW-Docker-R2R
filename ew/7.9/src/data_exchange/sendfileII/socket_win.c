/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: socket_win.c 6858 2016-10-28 17:47:35Z kevin $
 *
 *    Revision history:
 *     $Log: socket_win.c,v $
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 *
 *
 */

/*************************************************************
 *                       socket_win.c                        *
 *                                                           *
 *  This file contains functions that are specific to        *
 *  Windows.                                                 *
 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "sendfileII.h"

static SOCKET sd;                      /* Socket identifier */
static int socket_open = 0;


/********************** SocketSysInit **********************
 *               Initialize the socket system              *
 *         We are using Windows socket version 2.2.        *
 ***********************************************************/
void SocketSysInit( void )
{
    WSADATA Data;
    int     status = WSAStartup( MAKEWORD(2,2), &Data );
    if ( status != 0 )
    {
	log( "et", "WSAStartup failed. Exiting.\n" );
	exit( -1 );
    }
    return;
}


/************************************************************
 *                    ConnectToGetfile()                    *
 *                                                          *
 *          Get a connection to the remote system.          *
 ************************************************************/

int ConnectToGetfile( void )
{
    extern char ServerIP[20];     /* IP address of system to receive msg */
    extern int  ServerPort;       /* The well-known port number */
    struct sockaddr_in server;    /* Server socket address structure */
    const int optVal = 1;
    unsigned long address;
    unsigned long lOnOff=1;

    /* Get a new socket descriptor
     ***************************/
    if (socket_open) {
	log("et", "logic error: socket already open\n");
    } else {
	sd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sd == INVALID_SOCKET )
	{
	    log( "et", "socket() error: %d\n", WSAGetLastError() );
	    return SENDFILE_FAILURE;
	}
	socket_open = 1;
    }

    /* Fill in socket address structure
     ********************************/
    address = inet_addr( ServerIP );
    if ( address == INADDR_NONE )
    {
	log( "et", "Bad server IP address: %s  Exiting.\n", ServerIP );
	CloseSocketConnection();
	return SENDFILE_FAILURE;
    }
    memset( (char *)&server, '\0', sizeof(server) );
    server.sin_family = AF_INET;
    server.sin_port   = htons( (unsigned short)ServerPort );
    server.sin_addr.S_un.S_addr = address;

    /* Connect to the server.
       The connect call may block if getfileII is not available.
    ******************************************************/
    log( "e", "\n" );
    log( "et", "Connecting to %s port %d\n", ServerIP, ServerPort );
    if ( connect( sd, (struct sockaddr *)&server, sizeof(server) )
	 == SOCKET_ERROR )
    {
	int rc = WSAGetLastError();
	if ( rc == WSAETIMEDOUT )
	    log( "et", "Error: connect() timed out.\n" );
	else
	    log( "et", "connect() error: %d\n", rc );
	CloseSocketConnection();
	return SENDFILE_FAILURE;
    }

    /* Set socket to non-blocking mode
     *******************************/
    if ( ioctlsocket( sd, FIONBIO, &lOnOff ) == SOCKET_ERROR )
    {
	log( "et", "Error %d, changing socket to non-blocking mode\n",
	     WSAGetLastError() );
	CloseSocketConnection();
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

static int Send_all( SOCKET sock, char buf[], int nbytes )
{
    extern int TimeOut;             /* Send/receive timeout, in seconds */
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
	FD_SET( sock, &writefds );
	selrc = select( sock+1, 0, &writefds, 0, &selectTimeout );
	if ( selrc == SOCKET_ERROR )
	{
	    log( "et", "select() error: %d", WSAGetLastError() );
	    return -1;
	}
	if ( selrc == 0 )
	{
	    log( "et", "select() timed out\n" );
	    return -1;
	}
	if ( FD_ISSET( sock, &writefds ) )    /* A writeable event occurred */
	{

	    /* Send the data
             *************/
	    int rc = send( sock, &buf[nwritten], nBytesToWrite, 0 );
	    if ( rc == SOCKET_ERROR )
	    {
		int winError = WSAGetLastError();

		if ( winError == WSAEWOULDBLOCK )
		    continue;
		else if ( winError == WSAECONNRESET )
		{
		    log( "et", "send() error: Connection reset by peer.\n" );
		    return -1;
		}
		else
		{
		    log( "et", "send() error: %d\n", winError );
		    return -1;
		}
	    }
	    if ( rc == 0 )     /* Shouldn't see this */
	    {
		log( "et", "Error: send()== 0\n" );
		return -1;
	    }
	    nwritten += rc;
	    nBytesToWrite -= rc;
	}
    }
    return 0;
}


/****************************************************************
 *                      SendBlockToSocket()                     *
 *                                                              *
 *  Send a block of data to the remote system.                  *
 *  Blocks can be from 1 to 999999 bytes long.                  *
 *  A zero-length block is sent when the transmission is over.  *
 ****************************************************************/

int SendBlockToSocket( char *buf, int blockSize )
{
    char blockSizeAsc[7];

    /* Sanity check
     ************/
    if ( blockSize > 999999 )
    {
	log( "et", "Error. Block is too large to send.\n" );
	return SENDFILE_FAILURE;
    }

    /* Write block size to socket
     **************************/
    sprintf( blockSizeAsc, "%6u", blockSize );
    if ( Send_all( sd, blockSizeAsc, 6 ) == -1 )
    {
	log( "et", "Send_all() error\n" );
	return SENDFILE_FAILURE;
    }

    /* Send block bytes
     ****************/
    if ( Send_all( sd, buf, blockSize ) == -1 )
    {
	log( "et", "Send_all() error\n" );
	return SENDFILE_FAILURE;
    }
    return SENDFILE_SUCCESS;
}



/*********************************************************************
 *                              Recv_all()                           *
 *                                                                   *
 *  Reads nbyte characters from socket sock into buf.                *
 *  The function will time out if the data can't be read within      *
 *  TimeOut seconds.                                                 *
 *                                                                   *
 *  Returns 0 if all ok.                                             *
 *  Returns -1 if an error or timeout occurred.                      *
 *********************************************************************/

static int Recv_all( SOCKET sock, char buf[], int nbytes, int TimeOut )
{
    int    nBytesToRead = nbytes;
    int    nread        = 0;

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
	FD_SET( sock, &readfds );
	selrc = select( sock+1, &readfds, 0, 0, &selectTimeout );
	if ( selrc == SOCKET_ERROR )
	{
	    log( "et", "select() error: %d", WSAGetLastError() );
	    return -1;
	}
	if ( selrc == 0 )
	{
	    log( "et", "select() timed out\n" );
	    return -1;
	}
	if ( FD_ISSET( sock, &readfds ) )    /* A readable event occurred */
	{
	    int rc = recv( sock, &buf[nread], nBytesToRead, 0 );
	    if ( rc == SOCKET_ERROR )
	    {
		int winError = WSAGetLastError();

		if ( winError == WSAEWOULDBLOCK )
		    continue;
		else if ( winError == WSAECONNRESET )
		{
		    log( "et", "recv() error: Connection reset by peer.\n" );
		    return -1;
		}
		else
		{
		    log( "et", "recv() error: %d\n", winError );
		    return -1;
		}
	    }
	    if ( rc == 0 )
	    {
		log( "et", "Error: recv()==0\n" );
		return -1;
	    }

	    nread += rc;
	    nBytesToRead -= rc;
	}
    }
    return 0;
}


/****************************************************************
 *                   GetAckFromSocket()                         *
 *  Get an "ACK" from the remote system.                        *
 *  Return SENDFILE_SUCCESS on success, otherwise               *
 *     return SENDFILE_FAILURE.                                 *
 ****************************************************************/

int GetAckFromSocket( int timeout )
{
    char Ack[4];
    
    /* Read the "ACK" from the socket.
     *************************************************************/
    if ( Recv_all( sd, Ack, 3, timeout ) == -1)
    {
	log( "et", "Recv_all() error.\n" );
	return SENDFILE_FAILURE;
    }
    
    Ack[3] = '\0';            /* Null terminate */

    if ( strcmp( Ack, "ACK" ) == 0 )
    {
	log( "et", "ACK received\n" );
	return SENDFILE_SUCCESS;
    }

    log( "et", "Bad ACK string received: %s\n", Ack);
   
    return SENDFILE_FAILURE;
}



/*********************************************************
 *                CloseSocketConnection()                *
 *********************************************************/
void CloseSocketConnection( void )
{
    int ret;

    if (socket_open) {
	ret = closesocket( sd );
	if (ret != 0)
	    log( "et", "Error closing socket\n");
	socket_open = 0;
    }
    
    return;
}



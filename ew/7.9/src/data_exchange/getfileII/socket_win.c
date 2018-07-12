/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: socket_win.c 6858 2016-10-28 17:47:35Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/12/20 02:39:11  lombard
 *     Initial revision
 *
 *
 *
 */

/*************************************************************
 *                        socket_win.c                       *
 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "getfileII.h"

static SOCKET sd;                      /* Socket identifier */
static SOCKET ns;                      /* Read from this socket */
static int rsocket_open = 0;

static unsigned int trustedClient[MAXCLIENT];


/********************** SocketSysInit **********************
 *               Initialize the socket system              *
 *                                                         *
 *  We are using Windows socket version 2.2.               *
 ***********************************************************/

void SocketSysInit( void )
{
    extern int    nClient;               /* Number of trusted clients */
    extern CLIENT Client[MAXCLIENT];     /* Addresses of trusted clients */
    int         i;  
    int         status;
    WSADATA     Data;

    status = WSAStartup( MAKEWORD(2,2), &Data );
    if ( status != 0 )
    {
	my_log( "et", "WSAStartup failed. Exiting.\n" );
	exit( -1 );
    }

    /* Convert client IP address to unsigned int
     *****************************************/
    for ( i = 0; i < nClient; i++ ) 
    { 
	trustedClient[i] = inet_addr( Client[i].ip );
	if ( trustedClient[i] == INADDR_NONE )
	{
	    my_log( "et", "Error. Bad client ip: %s Exiting.\n", Client[i].ip );
	    exit( -1 );
	}
    }
    return;
}


/***********************************************
 *              InitServerSocket()             *
 ***********************************************/

void InitServerSocket( void )
{
    extern char ServerIP[20];     /* IP address of system to receive msg */
    extern int  ServerPort;       /* The well-known port number */
    struct sockaddr_in server;    /* Server socket address structure */
    const int optVal = 1;
    unsigned long address;

    /* Get a new socket descriptor
     ***************************/
    sd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sd == INVALID_SOCKET )
    {
	my_log( "et", "socket() error: %d\n", WSAGetLastError() );
	exit( -1 );
    }

    /* Allow reuse of socket addresses
     *******************************/
    if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal,
		     sizeof(int) ) == SOCKET_ERROR )
    {
	my_log( "et", "setsockopt() error: %d\n", WSAGetLastError() );
	CloseSock();
	exit( -1 );
    }

    /* Fill in socket address structure
     ********************************/
    address = inet_addr( ServerIP );
    if ( address == INADDR_NONE )
    {
	my_log( "et", "Bad server IP address: %s  Exiting.\n", ServerIP );
	CloseSock();
	exit( -1 );
    }
    memset( (char *)&server, '\0', sizeof(server) );
    server.sin_family      = AF_INET;
    server.sin_port        = htons( (unsigned short)ServerPort );
    server.sin_addr.S_un.S_addr = address;

    /* Bind a local address to the socket descriptor
     *********************************************/
    if ( bind( sd, (struct sockaddr *)&server, sizeof(server) ) ==
	 SOCKET_ERROR )
    {
	my_log( "et", "bind() error: %d\n", WSAGetLastError() );
	CloseSock();
	exit( -1 );
    }

    /* Set the maximum number of pending connections
     *********************************************/
    if ( listen( sd, 5 ) == SOCKET_ERROR )
    {
	my_log( "et", "listen() error: %d\n", WSAGetLastError() );
	CloseSock();
	exit( -1 );
    }
    return;
}


/***********************************************
 *             AcceptConnection()              *
 *       Accept a TCP socket connection.       *
 *                                             *
 *  This function blocks on accept().          *
 ***********************************************/

int AcceptConnection( int *clientIndex )
{
    extern int nClient;                /* Number of trusted clients */
    struct sockaddr_in client;         /* Client socket address structure */
    int clientlen = sizeof(client);    /* Client socket structure length */
    unsigned long lOnOff=1;
    int i;
    int trusted = 0;                   /* 1 if client is trusted */

    ns = accept( sd, (struct sockaddr *)&client, &clientlen );
    if ( ns == INVALID_SOCKET )
    {
	my_log( "et", "accept() error: %d\n", WSAGetLastError() );
	return GETFILE_FAILURE;
    }
    rsocket_open = 1;

    /* Accept data only from trusted clients
     *************************************/
    my_log( "e", "\n" );

    for ( i = 0; i < nClient; i++ )
	if ( client.sin_addr.S_un.S_addr == trustedClient[i] )
	{
	    trusted = 1;
	    break;
	}

    if ( !trusted )
    {
	my_log( "et", "Rejected connection attempt by %s\n",
	     inet_ntoa(client.sin_addr) );
	CloseReadSock();
	return GETFILE_FAILURE;
    }

    *clientIndex = i;
    my_log( "et", "Accepted connection from %s\n", inet_ntoa(client.sin_addr) );

    /* Set socket to non-blocking mode
     *******************************/
    if ( ioctlsocket( ns, FIONBIO, &lOnOff ) == SOCKET_ERROR )
    {
	my_log( "et", "ioctl() FIONBIO error: %d\n", WSAGetLastError() );
	CloseReadSock();
	return GETFILE_FAILURE;
    }
    return GETFILE_SUCCESS;
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

int Recv_all( SOCKET sock, char buf[], int nbytes )
{
    extern int TimeOut;             /* Send/receive timeout, in seconds */
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
	    my_log( "et", "select() error: %d", WSAGetLastError() );
	    return -1;
	}
	if ( selrc == 0 )
	{
	    my_log( "et", "select() timed out\n" );
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
		    my_log( "et", "recv() error: Connection reset by peer.\n" );
		    return -1;
		}
		else
		{
		    my_log( "et", "recv() error: %d\n", winError );
		    return -1;
		}
	    }
	    if ( rc == 0 )
	    {
		my_log( "et", "Error: recv()==0\n" );
		return -1;
	    }

	    nread += rc;
	    nBytesToRead -= rc;
	}
    }
    return 0;
}


/**********************************************************************
 *                        GetBlockFromSocket()                        *
 *  This function reads a block of bytes from the sendfileII program. *
 *  Blocks can be from 1 to 999999 bytes long.                        *
 *  If a zero length block is received, it means the transmission     *
 *  is over.                                                          *
 **********************************************************************/

int GetBlockFromSocket( char buf[], int *nbytes )
{
    char blockSizeAsc[7];
    int  blockSize;

    /* Read the block size from the socket.  The size is a six ASCII
       characters long, right justified and padded with blanks.
    *************************************************************/
    if ( Recv_all( ns, blockSizeAsc, 6 ) == -1)
    {
	my_log( "et", "Recv_all() error.\n" );
	return GETFILE_FAILURE;
    }

    blockSizeAsc[6] = '\0';            /* Null terminate */

    if ( sscanf( blockSizeAsc, "%u", &blockSize ) < 1 )
    {
	my_log( "et", "Error decoding block size.\n" );
	return GETFILE_FAILURE;
    }

    if ( blockSize > BUFLEN )
    {
	my_log( "et", "Error. Block is too large for buffer.\n" );
	return GETFILE_FAILURE;
    }

    /* The sendfileII program sends a blocksize of zero
       when it has finished sending the file.
    **********************************************/
    if ( blockSize == 0 ) return GETFILE_DONE;

    /* Read the block from the socket
     ******************************/
    if ( Recv_all( ns, buf, blockSize ) == -1)
    {
	my_log( "et", "Recv_all() error.\n" );
	return GETFILE_FAILURE;
    }
    *nbytes = blockSize;        /* Return to calling program */
    return GETFILE_SUCCESS;
}



/*********************************************************************
 *                              Send_all()                           *
 *                                                                   *
 *  Sends the first nbytes characters in buf to the socket sock.     *
 *  The function will time out if the data can't be sent within      *
 *  TimeOut seconds.                                                 *
 *                                                                   *
 *  Returns 0 if all ok.                                             *
 *  Returns -1 if an error or timeout occurred.                      *
 *********************************************************************/

static int Send_all( SOCKET sock, char buf[], int nbytes )
{
    extern int TimeOut;
    int    nBytesToWrite = nbytes;
    int    nwritten      = 0;

    while ( nBytesToWrite > 0 ) {
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
	    my_log( "et", "select() error: %d", WSAGetLastError() );
	    return -1;
	}
	if ( selrc == 0 )
	{
	    my_log( "et", "select() timed out\n" );
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
		    my_log( "et", "send() error: Connection reset by peer.\n" );
		    return -1;
		}
		else
		{
		    my_log( "et", "send() error: %d\n", winError );
		    return -1;
		}
	    }
	    if ( rc == 0 )     /* Shouldn't see this */
	    {
		my_log( "et", "Error: send()== 0\n" );
		return -1;
	    }
	    nwritten += rc;
	    nBytesToWrite -= rc;
	}
    }
    return 0;
}


/****************************************************************
 *                      SendAckToSocket()                       *
 *                                                              *
 *  Send an "ACK" to the remote system.                         *
 ****************************************************************/

int SendAckToSocket( void )
{
    char *Ack = "ACK";


    /* Write Ack to socket
     **************************/
    if ( Send_all( ns, Ack, 3 ) == -1 )
    {
	my_log( "et", "Send_all() error\n" );
	return GETFILE_FAILURE;
    }

    return GETFILE_SUCCESS;
}



/*********************** CloseSock ***********************
 *                    Close a Socket                     *
 *********************************************************/

void CloseSock( void )
{
    if ( closesocket( sd ) == SOCKET_ERROR )
	my_log( "et", "closesocket() error: %d\n", WSAGetLastError() );
    return;
}


/******************** CloseReadSock **********************
 *       Close the socket we actually read from.         *
 *********************************************************/

void CloseReadSock( void )
{
    if (rsocket_open) {
	if ( closesocket( ns ) == SOCKET_ERROR )
	    my_log( "et", "closesocket() error: %d\n", WSAGetLastError() );
	rsocket_open = 0;
    }
    
    return;
}



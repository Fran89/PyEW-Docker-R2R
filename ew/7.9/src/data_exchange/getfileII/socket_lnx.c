/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: socket_lnx.c 4543 2011-08-10 14:54:23Z paulf $
 *
 *
 */


/*************************************************************
 *                        socket_lnx.c                       *
 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "getfileII.h"

static int sd;        /* Accept connections on this socket */
static int ns;        /* Read from this socket */
static int rsocket_open = 0;

static unsigned int trustedClient[MAXCLIENT];


/********************** SocketSysInit *********************
 *   Convert client IP address to unsigned int.           *
 **********************************************************/

void SocketSysInit( void )
{
    extern int    nClient;               /* Number of trusted clients */
    extern CLIENT Client[MAXCLIENT];     /* Addresses of trusted clients */
    int         i;

    for ( i = 0; i < nClient; i++ )
    {
	trustedClient[i] = inet_addr( Client[i].ip );
	if ( trustedClient[i] == 0xffffffff )
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
    extern char ServerIP[20];      /* IP address of system to receive msg's */
    extern int  ServerPort;        /* The well-known port number */
    struct sockaddr_in server;     /* Server socket address structure */
    const int optVal = 1;
    unsigned long address;

    signal(SIGPIPE,SIG_IGN);

    /* Get a new socket descriptor
     ***************************/
    sd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sd == -1 )
    {
	my_log( "et", "socket() error\n" );
	exit( -1 );
    }

    /* Allow reuse of socket addresses
     *******************************/
    if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal,
		     sizeof(int) ) == -1 )
    {
	my_log( "et", "setsockopt() error\n" );
	close( sd );
	exit( -1 );
    }

    /* Fill in socket address structure
     ********************************/
    address = inet_addr( ServerIP );
    if ( address == -1 )
    {
	my_log( "et", "Bad server IP address: %s  Exiting.\n", ServerIP );
	close( sd );
	exit( -1 );
    }
    memset( (char *)&server, '\0', sizeof(server) );
    server.sin_family = AF_INET;
    server.sin_port   = htons( (unsigned short)ServerPort );
    server.sin_addr.s_addr = address;

    /* Bind a local address to the socket descriptor
     *********************************************/
    if ( bind( sd, (struct sockaddr *)&server, sizeof(server) ) == -1 )
    {
	my_log( "et", "bind() error: %s\n", strerror(errno) );
	close( sd );
	exit( -1 );
    }

    /* Set the maximum number of pending connections
     *********************************************/
    if ( listen( sd, 5 ) == -1 )
    {
	my_log( "et", "listen() error: %s\n", strerror(errno) );
	close( sd );
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
    if ( ns == -1 )
    {
	my_log( "et", "accept() error: %s\n", strerror(errno) );
	return GETFILE_FAILURE;
    }
    rsocket_open = 1;
    
    /* Accept data only from trusted clients
     *************************************/
    my_log( "e", "\n" );
 
    for ( i = 0; i < nClient; i++ ) 
	if ( client.sin_addr.s_addr == trustedClient[i] )
	{ 
	    trusted = 1;
	    break; 
	} 

    if ( !trusted )
    {
	my_log( "et", "Rejected connection attempt by %s\n",
	     inet_ntoa(client.sin_addr) );
	CloseReadSock( );
	return GETFILE_FAILURE;
    }

    *clientIndex = i;
    my_log( "et", "Accepted connection from %s\n", inet_ntoa(client.sin_addr) );

    /* Set socket to non-blocking mode
     *******************************/
    if ( fcntl( ns, F_SETFL, O_NONBLOCK ) == -1 )
    {
	my_log( "et", "fcntl() NONBLOCK error: %s\n", strerror(errno) );
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

static int Recv_all( int sock, char buf[], int nbytes )
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
	if ( selrc == -1 )
	{
	    my_log( "et", "select() error: %s", strerror(errno) );
	    return -1;
	}
	if ( selrc == 0 )
	{
	    my_log( "et", "select() timed out\n" );
	    return -1;
	}
	if ( FD_ISSET( sock, &readfds ) )    /* A readable event occurred */
	{
	    int rc;
	    int sockerr;
	    int optlen = sizeof(int);

	    /* Check for socket errors using getsockopt()
             ******************************************/
	    if ( getsockopt( sock, SOL_SOCKET, SO_ERROR, (char *)&sockerr,
			     &optlen ) == -1 )
	    {
		my_log( "et", "getsockopt() error: %s", strerror(errno) );
		return -1;
	    }
	    if ( sockerr != 0 )
	    {
		my_log( "et", "Error detected by getsockopt(): %s\n",
                     strerror(sockerr) );
		return -1;
	    }

	    /* Get the data.  It's unlikely that recv() would block,
	       since we know that a readable event occurred.
	    ****************************************************/
	    rc = recv( sock, &buf[nread], nBytesToRead, 0 );
	    if ( rc == -1 )
	    {
		if ( errno == EWOULDBLOCK )      /* Unlikely */
		    continue;
		else
		{
		    my_log( "et", "recv() error: %s", strerror(errno) );
		    return -1;
		}
	    }
	    if ( rc == 0 )
	    {
		my_log( "et", "Error. recv()==0\n" );
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

static int Send_all( int sock, char buf[], int nbytes )
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
	if ( selrc == -1 )
	{
	    my_log( "et", "select() error: %s", strerror(errno) );
	    return -1;
	}
	if ( selrc == 0 )
	{
	    my_log( "et", "select() timed out\n" );
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
		my_log( "et", "getsockopt() error: %s", strerror(errno) );
		return -1;
	    }
	    if ( sockerr != 0 )
	    {
		my_log( "et", "Error detected by getsockopt(): %s\n",
                     strerror(sockerr) );
		return -1;
	    }

	    /* Send the data.  It's unlikely that send() would block,
	       since we know that a writeable event occurred.
	    *****************************************************/
	    rc = send( sock, &buf[nwritten], nBytesToWrite, 0 );
	    if ( rc == -1 )
	    {
		if ( errno == EWOULDBLOCK )    /* Unlikely */
		{
		    my_log( "et", "send() would block\n" );
		    continue;
		}
		else
		{
		    my_log( "et", "send() error: %d", strerror(errno) );
		    return -1;
		}
	    }
	    if ( rc == 0 )           /* Shouldn't see this */
	    {
		my_log( "et", "Error: send() == 0\n" );
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
	my_log( "et", "Send_all() error.\n" );
	return GETFILE_FAILURE;
    }

    return GETFILE_SUCCESS;
}


/********************* CloseReadSock *********************
 *       Close the socket we actually read from.         *
 *********************************************************/

void CloseReadSock( void )
{
    if (rsocket_open) {
	close( ns );
	rsocket_open = 0;
    }
    
    return;
}

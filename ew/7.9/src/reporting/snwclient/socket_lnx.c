/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: socket_lnx.c 693 2012-01-25 19:59:02Z paulf $
 *
 *
 *
 */

/*************************************************************
 *                        socket_lnx.c                       *
 *                                                           *
 *  This file contains functions that are specific to        *
 *  Linux.                                                   *
 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "snwclient.h"
#include "my_log.h"

void CloseSocketConnection( void );

static int sd;                  /* Socket descriptor */
static int socket_open = 0;


/**********************************************************
 *                     SocketSysInit()                    *
 *  Dummy in Linux.                                       *
 **********************************************************/
void SocketSysInit( void )
{
    return;
}


/************************************************************
 *                    ConnectToGetfile()                    *
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

    signal(SIGPIPE, SIG_IGN);
    
    /* Get a new socket descriptor
     ***************************/
    if (socket_open) {
	log("et", "logic error: socket already open\n");
    } else {
	sd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sd == -1 )
	{
	    log( "et", "socket() error: %s\n", strerror(errno) );
	    return SENDFILE_FAILURE;
	}
	socket_open = 1;
    }

    /* Fill in socket address structure
     ********************************/
    address = inet_addr( ServerIP );
    if ( address == -1 )
    {
	log( "et", "Bad server IP address: %s  Exiting.\n", ServerIP );
	CloseSocketConnection();
	return SENDFILE_FAILURE;
    }
    memset( (char *)&server, '\0', sizeof(server) );
    server.sin_family = AF_INET;
    server.sin_port   = htons((unsigned short)ServerPort);
    server.sin_addr.s_addr = address;
    
    /* Connect to the server.
       The connect call blocks if getfileII is not available.
    ***************************************************/
    log( "e", "\n" );
    log( "et", "Connecting to %s  port %d\n", ServerIP, ServerPort );
    if ( connect( sd, (struct sockaddr *)&server, sizeof(server) ) == -1 )
    {
	log( "et", "connect() error: %s\n", strerror(errno) );
	CloseSocketConnection();
	return SENDFILE_FAILURE;
    }

    /* Set socket to non-blocking mode
     *******************************/
    if ( fcntl( sd, F_SETFL, O_NONBLOCK ) == -1 )
    {
	log( "et", "fnctl() NONBLOCK error: %s\n", strerror(errno) );
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
		if ( errno == EWOULDBLOCK )    /* Unlikely */
		{
		    log( "et", "send() would block\n" );
		    continue;
		}
		else
		{
		    log( "et", "send() error: %d", strerror(errno) );
		    return -1;
		}
	    }
	    if ( rc == 0 )           /* Shouldn't see this */
	    {
		log( "et", "Error: send() == 0\n" );
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
 *  Send a block to the remote system.                          *
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

    /* Send block bytes
     ****************/
    if ( Send_all( sd, buf, blockSize ) == -1 )
    {
	log( "et", "Send_all() error.\n" );
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

static int Recv_all( int sock, char buf[], int nbytes, int TimeOut )
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
		log( "et", "getsockopt() error: %s", strerror(errno) );
		return -1;
	    }
	    if ( sockerr != 0 )
	    {
		log( "et", "Error detected by getsockopt(): %s\n",
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
		    log( "et", "recv() error: %s", strerror(errno) );
		    return -1;
		}
	    }
	    if ( rc == 0 )
	    {
		log( "et", "Error. recv()==0\n" );
		return -1;
	    }
	    nread += rc;
	    nBytesToRead -= rc;
	}
    }
    return 0;
}


/****************************************************************
 *                      GetAckFromSocket()                      *
 *                                                              *
 *  Get an "ACK" from the remote system.                        *
 *  Return SENDFILE_SUCCESS on success, otherwise               *
 *     return SENDFILE_FAILURE.                                 *
 ****************************************************************/
int GetAckFromSocket( int timeout )
{
    char Ack[4];
    int  blockSize;

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



/**********************************************************
 *                 CloseSocketConnection()                *
 **********************************************************/
void CloseSocketConnection( void )
{
    int ret;
    
    if (socket_open) {
	ret = close( sd );
	if (ret != 0)
	    log( "et", "Error closing socket: %s\n", strerror(errno));
	socket_open = 0;
    }
    return;
}



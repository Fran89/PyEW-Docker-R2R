/*************************************************************
 *                        socket_win.c                       *
 *                        for getfile_cl                     *
 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "getfile_cl.h"

static SOCKET sd;                      /* Read from this socket */
static int socket_open = 0;

static unsigned int trustedClient;


/********************** SocketSysInit **********************
 *               Initialize the socket system              *
 *                                                         *
 *  We are using Windows socket version 2.2.               *
 ***********************************************************/

void SocketSysInit( void )
{
    extern char  ClientIP[20];     /* Address of trusted client */
    int          status;
    WSADATA      Data;

    status = WSAStartup( MAKEWORD(2,2), &Data );
    if ( status != 0 ) {
		log( "et", "WSAStartup failed. Exiting.\n" );
		exit( -1 );
    }

    /* Convert client IP address to unsigned int
     *****************************************/
     
	trustedClient = inet_addr( ClientIP );
	if ( trustedClient == INADDR_NONE ) {
	    log( "et", "Error. Bad client ip: %s Exiting.\n", ClientIP );
	    exit( -1 );
	}
    
    return;
}


/************************************************************
 *                    ConnectToSendfile()                   *
 *                                                          *
 *          Get a connection to the remote system.          *
 ************************************************************/

int ConnectToSendfile( void )
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
		if ( sd == INVALID_SOCKET ) {
			log( "et", "socket() error: %d\n", WSAGetLastError() );
			return GETFILE_FAILURE;
		}
		socket_open = 1;
    }

    /* Fill in socket address structure
     ********************************/
    address = inet_addr( ServerIP );
    if ( address == INADDR_NONE ) {
		log( "et", "Bad server IP address: %s  Exiting.\n", ServerIP );
		CloseSocketConnection();
		return GETFILE_FAILURE;
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
    if ( connect( sd, (struct sockaddr *)&server, sizeof(server) ) == SOCKET_ERROR ) {
		int rc = WSAGetLastError();
		if ( rc == WSAETIMEDOUT )
			log( "et", "Error: connect() timed out.\n" );
		else
			log( "et", "connect() error: %d\n", rc );
		CloseSocketConnection();
		return GETFILE_FAILURE;
    }

    /* Set socket to non-blocking mode
     *******************************/
    if ( ioctlsocket( sd, FIONBIO, &lOnOff ) == SOCKET_ERROR ) {
		log( "et", "Error %d, changing socket to non-blocking mode\n", WSAGetLastError() );
		CloseSocketConnection();
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

int Recv_all( int sock, char buf[], int nbytes )
{
    extern int TimeOut;             /* Send/receive timeout, in seconds */
    int    nBytesToRead = nbytes;
    int    nread        = 0;

    while ( nBytesToRead > 0 ) {
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
		if ( selrc == SOCKET_ERROR ) {
			log( "et", "select() error: %d", WSAGetLastError() );
			return -1;
		}
		if ( selrc == 0 ) {
			log( "et", "select() timed out\n" );
			return -1;
		}
		if ( FD_ISSET( sock, &readfds ) ) {   /* A readable event occurred */
			int rc = recv( sock, &buf[nread], nBytesToRead, 0 );
			if ( rc == SOCKET_ERROR ) {
				int winError = WSAGetLastError();
		
				if ( winError == WSAEWOULDBLOCK )
					continue;
				else if ( winError == WSAECONNRESET ) {
					log( "et", "recv() error: Connection reset by peer.\n" );
					return -1;
				}
				else {
					log( "et", "recv() error: %d\n", winError );
					return -1;
				}
			}
			if ( rc == 0 ) {
				log( "et", "Error: recv()==0\n" );
				return -1;
			}
	
			nread += rc;
			nBytesToRead -= rc;
		}
    }
    return 0;
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
		if ( selrc == SOCKET_ERROR ) {
			log( "et", "select() error: %d", WSAGetLastError() );
			return -1;
		}
		if ( selrc == 0 ) {
			log( "et", "select() timed out\n" );
			return -1;
		}
		if ( FD_ISSET( sock, &writefds ) ) {   /* A writeable event occurred */
			/* Send the data
				 *************/
			int rc = send( sock, &buf[nwritten], nBytesToWrite, 0 );
			if ( rc == SOCKET_ERROR ) {
				int winError = WSAGetLastError();
		
				if ( winError == WSAEWOULDBLOCK )
					continue;
				else if ( winError == WSAECONNRESET ) {
					log( "et", "send() error: Connection reset by peer.\n" );
					return -1;
				}
				else {
					log( "et", "send() error: %d\n", winError );
					return -1;
				}
			}
			if ( rc == 0 ) {    /* Shouldn't see this */
				log( "et", "Error: send()== 0\n" );
				return -1;
			}
			nwritten += rc;
			nBytesToWrite -= rc;
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
    if ( Recv_all( sd, blockSizeAsc, 6 ) == -1) {
		log( "et", "Recv_all() error.\n" );
		return GETFILE_FAILURE;
    }

    blockSizeAsc[6] = '\0';            /* Null terminate */

    if ( sscanf( blockSizeAsc, "%u", &blockSize ) < 1 ) {
		log( "et", "Error decoding block size.\n" );
		return GETFILE_FAILURE;
    }

    if ( blockSize > BUFLEN ) {
		log( "et", "Error. Block is too large for buffer.\n" );
		return GETFILE_FAILURE;
    }

    /* The sendfileII program sends a blocksize of zero
       when it has finished sending the file.
    **********************************************/
    if ( blockSize == 0 ) return GETFILE_DONE;

    /* Read the block from the socket
     ******************************/
    if ( Recv_all( sd, buf, blockSize ) == -1) {
		log( "et", "Recv_all() error.\n" );
		return GETFILE_FAILURE;
    }
    *nbytes = blockSize;        /* Return to calling program */
    return GETFILE_SUCCESS;
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
    if ( Send_all( sd, Ack, 3 ) == -1 ) {
		log( "et", "Send_all() error\n" );
		return GETFILE_FAILURE;
    }

    return GETFILE_SUCCESS;
}



/******************** CloseReadSock **********************
 *       Close the socket we actually read from.         *
 *********************************************************/

void CloseReadSock( void )
{
    if (socket_open) {
		if ( closesocket( sd ) == SOCKET_ERROR )
			log( "et", "closesocket() error: %d\n", WSAGetLastError() );
		socket_open = 0;
    }
    
    return;
}

/**********************************************************
 *                 CloseSocketConnection()                *
 **********************************************************/
void CloseSocketConnection( void )
{
    int ret;
    
    if (socket_open) {
		if ( closesocket( sd ) == SOCKET_ERROR )
			log( "et", "closesocket() error: %d\n", WSAGetLastError() );
		socket_open = 0;
    }
    return;
}



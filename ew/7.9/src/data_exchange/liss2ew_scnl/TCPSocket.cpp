/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: TCPSocket.cpp 2194 2006-05-25 16:21:58Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 16:21:58  paulf
 *     added these from Hydra
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:24  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.7  2005/04/21 17:13:41  mark
 *     Changed InitFor"Send and "Receive" to "Connect" and "Listen"; added DNS lookup; added default constructor
 *
 *     Revision 1.6  2005/03/22 17:23:15  mark
 *     Added better logging for connect timeouts
 *
 *     Revision 1.5  2005/03/08 23:02:49  mark
 *     Added separate Connect() function; sockets now close immediately
 *
 *     Revision 1.4  2005/03/03 22:13:58  mark
 *     Added m_SendSocket
 *
 *     Revision 1.3  2005/03/03 17:46:54  mark
 *     Added CloseReadSocket
 *
 *     Revision 1.2  2005/02/24 23:09:43  mark
 *     Added debug log for connections
 *
 *     Revision 1.1  2005/02/24 21:23:40  mark
 *     Initial checkin
 *
 *
 */

#include "TCPSocket.h"

CTCPSocket::CTCPSocket()
{
	m_bInitialized = false;
}

/**
  * Create a TCP socket object and opens the datagram socket and port.
  *
  * @param ipAddress - string containing the IP address to connect to
  * @param port - TCP port to connect to
  */

CTCPSocket::CTCPSocket( char * ipAddress, unsigned short port )
{
	CreateSocket(ipAddress, port);
}

/**
  * Create a TCP socket object and opens the datagram socket and port.
  *
  * @param ipAddrValue - in_addr struct containing the IP address to connect to
  * @param port - TCP port to connect to
  */

CTCPSocket::CTCPSocket ( struct in_addr ipAddrValue, unsigned short port )
{
    char * ipAddress = inet_ntoa(ipAddrValue);

	CreateSocket(ipAddress, port);
}

/**
  * Destroys a CTCPSocket object and closes the socket.
  */

CTCPSocket::~CTCPSocket()
{
    if(m_ipAddress)
        delete [] m_ipAddress;
	if(m_szBindIPAddress)
		delete [] m_szBindIPAddress;

	if (m_bReceiving && m_ReadSocket != INVALID_SOCKET)
	{
		closesocket_ew(m_ReadSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
	}
    if (m_socket != INVALID_SOCKET)
	{
		closesocket_ew(m_socket, SOCKET_CLOSE_IMMEDIATELY_EW);
	}
}

void CTCPSocket::SetAddress(char * ipAddress, unsigned short port)
{
	if (m_bInitialized)
	{
		if (strcmp(m_ipAddress, ipAddress) != 0 || m_port != port)
		{
			reportError(WD_WARNING_ERROR, GENFATERR, "CTCPSocket: SetAddress attempting to alter IP and port for this socket!\n");
		}
		return;
	}

	CreateSocket(ipAddress, port);
}

void CTCPSocket::SetAcceptingPort(unsigned short port)
{
	if (m_bInitialized)
	{
		if (m_port != port)
		{
			reportError(WD_WARNING_ERROR, GENFATERR, "CTCPSocket: SetAcceptingPort attempting to alter port for this socket!\n");
		}
		return;
	}

	CreateSocket(UNKNOWN_IP, port);
}

/**
  * Helper function used by each constructor.  This allocates memory for storing the
  * IP address and opens the socket.
  *
  * @param pIPAddr - string containing the IP address to connect to
  * @param port - TCP port to connect to
  */

void CTCPSocket::CreateSocket(char *pIPAddr, unsigned short port)
{
	int errorval;

	// Set a default timeout value.
	m_Timeout = 500;
	m_ReadSocket = m_WriteSocket = INVALID_SOCKET;

    m_ipAddress = new char [strlen( pIPAddr )+1];
    strcpy( m_ipAddress, pIPAddr );
    m_port = port;

	m_szBindIPAddress = new char [16];

	m_socket = socket_ew( AF_INET, SOCK_STREAM, 0 );
    if ( m_socket == SOCKET_ERROR || m_socket == INVALID_SOCKET ) 
    {
		errorval = socketGetError_ew();
		reportError(WD_FATAL_ERROR, GENFATERR, "Unable to create socket:  Error %d.\n",
						errorval);
        m_socket = INVALID_SOCKET;
    }

	m_bAcceptIP = false;
	if (strcmp(m_ipAddress, UNKNOWN_IP) == 0)
		m_bAcceptIP = true;

	m_bInitialized = true;
}

/**
  * Initializes the socket for connecting.  This call is required before calling Send.
  *
  * @return 0 if all is well, or SOCKET_ERROR for an error.
  */

int CTCPSocket::InitForConnect()
{
	if (!m_bInitialized)
	{
		reportError(WD_FATAL_ERROR, GENFATERR, "Error: Cannot create socket until address is set via SetAddress\n");
		return SOCKET_ERROR;
	}
	m_bReceiving = false;

	return InitSocket();
}

/**
  * Connects to the given IP and port.
  *
  * @return 0 if all is well, or an error code if no connection was made:
  *		WOULDBLOCK_EW - connection timed out
  *     other - more serious problem
  */
int CTCPSocket::Connect()
{
    struct sockaddr_in	sockAddr;
	int errorval;

	if (!m_bInitialized)
	{
		reportError(WD_FATAL_ERROR, GENFATERR, "Error: Cannot connect socket until address is set via SetAddress\n");
		return SOCKET_ERROR;
	}

    memset( &sockAddr, 0, sizeof(sockAddr) );
    sockAddr.sin_family      = AF_INET;
    sockAddr.sin_addr.s_addr = m_ConnectingToQuadAddr.s_addr;
    sockAddr.sin_port        = htons( (unsigned short)m_port );

	if (connect_ew(m_socket, (sockaddr *)&sockAddr, sizeof(sockAddr), m_Timeout) 
					== SOCKET_ERROR)
	{
		errorval = socketGetError_ew();
		if (errorval == WOULDBLOCK_EW)
		{
			reportError(WD_INFO, 0, "Unable to connect to %s: Connection timeout (nobody home)\n",
						m_ipAddress);
		}
		else
		{
			reportError(WD_FATAL_ERROR, GENFATERR, "Unable to connect to %s:  Error %d.\n",
							m_ipAddress, errorval);
		}
		if (errorval == 0)
			errorval = SOCKET_ERROR;
		return errorval;
	}

	reportError(WD_DEBUG, 0, "Connection to %s successful\n", m_ipAddress);

	// Since we're initiating a connection, we will use the socket we already set up to
	// read from.
	m_ReadSocket = m_socket;
	m_WriteSocket = m_socket;

	return 0;
}

/**
  * Sends a TCP packet over this object's socket.
  *
  * @param cBuffer - points to the data to send
  * @param nLen - number of bytes to send
  * @return number of bytes sent, or SOCKET_ERROR if an error occurs.
  */
int CTCPSocket::Send(const unsigned char * cBuffer, unsigned long nLen)
{
    int nResult = send_ew(m_WriteSocket, (char *)cBuffer, nLen, 0, m_Timeout);

    return nResult;
}

/**
  * Initializes a socket for receiving connections.  This must be called before calling AcceptConnections.
  *
  * @return 0 if all is well, or SOCKET_ERROR for an error.
  */

int CTCPSocket::InitForListen()
{
	int retval;
	int errorval;

	if (!m_bInitialized)
	{
		reportError(WD_FATAL_ERROR, GENFATERR, "Error: Cannot create socket until address is set via SetAddress\n");
		return SOCKET_ERROR;
	}

	m_bReceiving = true;

	retval = InitSocket();
	if (retval != 0)
		return retval;

	if (listen_ew(m_socket, 1) == SOCKET_ERROR)
	{
		errorval = socketGetError_ew();
		reportError(WD_FATAL_ERROR, GENFATERR, "Call to listen_ew failed for %s:  Error %d.\n",
						m_ipAddress, errorval);
		return SOCKET_ERROR;
	}

	return 0;
}

/**
  * Accepts a connection from a remote socket.  Call this function after calling InitForReceive.
  * If the socket is blocking (timeout == -1), then this function will block until a connection
  * is made.  Otherwise, if no connections were requested it will return WOULDBLOCK_EW.
  *
  * @return error code if no connection was made, or 0 if a connection was made.
  *		error codes:	WOULDBLOCK_EW -  all is well, but no connections were requested
  *								(only happens if m_Timeout != -1)
  *						anything else - more serious problem
  */
int CTCPSocket::AcceptConnections()
{
	int socklen = sizeof(sockaddr_in);
	int err;

	if (!m_bInitialized)
	{
		reportError(WD_FATAL_ERROR, GENFATERR, "Error: Cannot accept connections until address is set via SetAddress\n");
		return SOCKET_ERROR;
	}

	m_ReadSocket = accept_ew(m_socket, (sockaddr *)&m_ConnectedAddr, &socklen, m_Timeout);
	if (m_ReadSocket == INVALID_SOCKET)
	{
		err = socketGetError_ew();
		if (err != WOULDBLOCK_EW)
		{
			reportError( WD_FATAL_ERROR, GENFATERR, "Call to accept_ew failed:  Error %d.\n",
							err );
		}
		return err;
	}

	m_WriteSocket = m_ReadSocket;

	strcpy(m_ipAddress, inet_ntoa(m_ConnectedAddr.sin_addr));
	reportError(WD_DEBUG, 0, "Connection accepted from %s\n", m_ipAddress);

	return 0;
}

/**
  * Receives data from the socket.
  *
  * NOTE: This is by default a non-blocking call, and times out after the time specified in
  *		SetTimeout (in ms).  To make this blocking, call SetTimeout(-1) before calling this.
  *
  * @param cBuffer - points to data being received
  * @param nLen - size of cBuffer (max number of bytes that can be received)
  * @return number of bytes received, or SOCKET_ERROR for an error.
  */
int CTCPSocket::Receive( unsigned char * cBuffer, unsigned long nLen )
{
	int nBytes, err;

	nBytes = recv_ew( m_ReadSocket, (char *)cBuffer, nLen, 0, m_Timeout );
	if (nBytes == SOCKET_ERROR)
	{
		err = socketGetError_ew();
	}

    return nBytes;
}

/**
  * Receives data from the socket.  This attempts to retrieve the entire buffer's worth
  * of data before returning.
  *
  * NOTE: This is by default a non-blocking call, and times out after the time specified in
  *		SetTimeout (in ms).  To make this blocking, call SetTimeout(-1) before calling this.
  *
  * @param cBuffer - points to data being received
  * @param nLen - number of bytes to receive.  (cBuffer must be at least nLen + 1 bytes in size)
  * @return number of bytes received, or SOCKET_ERROR for an error.
  */
int CTCPSocket::ReceiveAll( unsigned char * cBuffer, unsigned long nLen )
{
	int nBytes;

	nBytes = recv_all( m_ReadSocket, (char *)cBuffer, nLen, 0, m_Timeout );

    return nBytes;
}

/**
  * Helper function for initialization.  This will perform any initialization that is common
  * for both sending and receiving sockets.
  *
  * @return 0 if all is well, or SOCKET_ERROR for an error.
  */
int CTCPSocket::InitSocket()
{
    struct sockaddr_in bindAddr;
	hostent *pHDns;
	unsigned long addr;
	BOOL bValue;
	int errorval;

	// Allow binding to a local address which is already in use.
	bValue = TRUE;
	if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&bValue, sizeof(bValue))
			== SOCKET_ERROR)
	{
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Error %d attempting to allow multiple bindings to same address.\n",
						errorval );
		return SOCKET_ERROR;
	}

	if (!m_bAcceptIP)
	{
		// Extract the IPv4 address of our connect-to address.  Start by assuming we're given
		// a dotted quad (a.b.c.d) address...
		m_ConnectingToQuadAddr.s_addr = inet_addr(m_ipAddress);
		if (m_ConnectingToQuadAddr.s_addr == INADDR_NONE)
		{
			// This isn't a valid dotted quad.  It could be a DNS name (computer.usgs.gov, etc.)...
			pHDns = gethostbyname(m_ipAddress);
			if (pHDns == NULL)
			{
				// We can't resolve this IP address.  Since we were given an IP address to
				// connect to (not just UNKNOWN_IP), we have to treat this as a fatal error.
				errorval = socketGetError_ew();
				reportError( WD_FATAL_ERROR, GENFATERR, "Error %d calling gethostbyname(); can't resolve %s.\n",
								errorval, m_ipAddress );
				return SOCKET_ERROR;
			}
			else
			{
				memcpy((void *)&m_ConnectingToQuadAddr.s_addr, pHDns->h_addr, pHDns->h_length);
			}
		}
	}
	else
		m_ConnectingToQuadAddr.s_addr = INADDR_NONE;

	if (FindBestAddress() == SOCKET_ERROR)
		return SOCKET_ERROR;

	// Bind the socket to our address.
	addr = inet_addr(m_szBindIPAddress);
    memset( &bindAddr, 0, sizeof(bindAddr));
    bindAddr.sin_family      = AF_INET;
    bindAddr.sin_addr.s_addr = (unsigned long)addr;
    bindAddr.sin_port        = htons( (unsigned short)m_port );
    
    if (bind( m_socket, (struct sockaddr *)&bindAddr, sizeof( bindAddr ) )
			== SOCKET_ERROR) 
    {
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Unable to bind to address %s: Error %d\n",
						m_ipAddress, errorval );
        return SOCKET_ERROR;
    }

	return 0;
}

void CTCPSocket::CloseSocket()
{
	if (m_ReadSocket != INVALID_SOCKET)
	{
		closesocket_ew(m_ReadSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
	}
	
	m_ReadSocket = INVALID_SOCKET;
	m_WriteSocket = INVALID_SOCKET;
}

/**
  * Determines which interface (i.e. network card) is best to use for this socket.
  * The address is stored in m_szBindIPAddress.
  *
  * @return 0 if all is well, or SOCKET_ERROR for an error.
  */
int CTCPSocket::FindBestAddress()
{
	hostent *pHLocal;
    char szLocal[256];
	struct in_addr *pAddr;
	int i, BestSoFar, errorval;

    // Determine our local address.  Since we may have more than one network interface (and
	// hence more than one address), we'll need to determine the best one to use.
    if (gethostname( szLocal, 256 ) == SOCKET_ERROR)
	{
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Error %d calling gethostname().\n",
						errorval );
		return SOCKET_ERROR;
	}
    pHLocal = gethostbyname( szLocal );
	if (pHLocal == NULL)
	{
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Error %d calling gethostbyname() for host '%s'.\n",
						errorval, szLocal );
		return SOCKET_ERROR;
	}

	// Find the first address to look at.
	pAddr = (struct in_addr *)pHLocal->h_addr_list[0];
	i = 0;
	BestSoFar = -1;
	
	// Step through each address until we find the best one.
	while (pAddr != NULL)
	{
		// First, look for addresses on this subnet.
		if (pAddr->S_un.S_un_b.s_b1 == m_ConnectingToQuadAddr.S_un.S_un_b.s_b1 
				&& pAddr->S_un.S_un_b.s_b2 == m_ConnectingToQuadAddr.S_un.S_un_b.s_b2 
				&& pAddr->S_un.S_un_b.s_b3 == m_ConnectingToQuadAddr.S_un.S_un_b.s_b3 )
		{
			BestSoFar = i;
			break;
		}

		// Next, look for addresses that don't start w/ 192.168 (i.e. not a local network)
		if (pAddr->S_un.S_un_b.s_b1 != 192 || pAddr->S_un.S_un_b.s_b2 != 168)
		{
			BestSoFar = i;
		}

		// Next, look for addresses that aren't 192.168.0.*, which are most likely not connected.
		if (pAddr->S_un.S_un_b.s_b1 == 192 && pAddr->S_un.S_un_b.s_b2 == 168
				&& pAddr->S_un.S_un_b.s_b3 != 0)
		{
			BestSoFar = i;
		}

		// Get the next address to look at.
		i++;
		pAddr = (struct in_addr *)pHLocal->h_addr_list[i];
	}
	// See if we found any likely candidates...
	if (BestSoFar == -1)
	{
		reportError( WD_FATAL_ERROR, GENFATERR, "Unable to find network connection.\n" );
		return SOCKET_ERROR;
	}

	// Extract the address for export.
    strcpy(m_szBindIPAddress, inet_ntoa( *(struct in_addr *)pHLocal->h_addr_list[BestSoFar] ));
	return 0;
}

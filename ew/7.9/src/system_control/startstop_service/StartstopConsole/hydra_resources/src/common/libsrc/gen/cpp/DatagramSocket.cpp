/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: DatagramSocket.cpp 2344 2006-06-14 23:01:12Z stefan $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2006/06/14 22:57:49  stefan
 *     *** empty log message ***
 *
 *     Revision 1.1  2006/06/12 04:55:39  stefan
 *     hydra resources
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:24  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.5  2004/07/01 16:44:11  mark
 *     Moved functionality here from CMulticastSocket
 *
 */

#include "DatagramSocket.h"


/**
  * Create a datagram socket object and opens the datagram socket and port.
  *
  * @param ipAddress - string containing the IP address to connect to
  * @param port - UDP port to connect to
  */


CDatagramSocket::CDatagramSocket( char * ipAddress, unsigned short port )
{
	CreateSocket(ipAddress, port);
}

/**
  * Create a datagram socket object and opens the datagram socket and port.
  *
  * @param ipAddrValue - in_addr struct containing the IP address to connect to
  * @param port - UDP port to connect to
  */

CDatagramSocket::CDatagramSocket ( struct in_addr ipAddrValue, unsigned short port )
{
    char * ipAddress = inet_ntoa(ipAddrValue);

	CreateSocket(ipAddress, port);
}

/**
  * Destroys a CDatagramSocket object and closes the socket.
  */

CDatagramSocket::~CDatagramSocket()
{
    if(m_ipAddress)
        delete [] m_ipAddress;
	if(m_szBindIPAddress)
		delete [] m_szBindIPAddress;

    if (m_socket != INVALID_SOCKET)
		closesocket_ew(m_socket, SOCKET_CLOSE_GRACEFULLY_EW);
}

/**
  * Helper function used by each constructor.  This allocates memory for storing the
  * IP address and opens the socket.
  *
  * @param pIPAddr - string containing the IP address to connect to
  * @param port - UDP port to connect to
  */

void CDatagramSocket::CreateSocket(char *pIPAddr, unsigned short port)
{
	int errorval;

	// Set a default timeout value.
	m_Timeout = 500;

    m_ipAddress = new char [strlen( pIPAddr )+1];
    strcpy( m_ipAddress, pIPAddr );
    m_port = port;

	m_szBindIPAddress = new char [strlen( pIPAddr )+1];
    strcpy( m_ipAddress, pIPAddr );

	m_socket = socket_ew( AF_INET, SOCK_DGRAM, 0 );
    if ( m_socket == SOCKET_ERROR || m_socket == INVALID_SOCKET ) 
    {
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Unable to create socket:  Error %d.\n",
						errorval );
        m_socket = INVALID_SOCKET;
    }
}

/**
  * Initializes the socket for sending packets.  This call is required before calling Send.
  *
  * @return 0 if all is well, or SOCKET_ERROR for an error.
  */

int CDatagramSocket::InitForSend()
{
	int retval;

	retval = InitSocket();
	if (retval != 0)
		return retval;

    memset( &m_sockAddr, 0, sizeof(m_sockAddr) );
    m_sockAddr.sin_family      = AF_INET;
    m_sockAddr.sin_addr.s_addr = inet_addr( m_ipAddress );
    m_sockAddr.sin_port        = htons( (unsigned short)m_port );

	return 0;
}

/**
  * Sends a UDP packet over this object's socket.
  *
  * @param cBuffer - points to the data to send
  * @param nLen - number of bytes to send
  * @return number of bytes sent, or SOCKET_ERROR if an error occurs.
  */
int CDatagramSocket::Send(const unsigned char * cBuffer, unsigned long nLen)
{
    int nResult = sendto_ew( m_socket, (char *)cBuffer, nLen, MSG_DONTROUTE, (struct sockaddr *) &m_sockAddr, 
                      sizeof( m_sockAddr ), m_Timeout );

    return nResult;
}

/**
  * Initializes a socket for receiving data.  This must be called before calling Receive.
  *
  * @return 0 if all is well, or SOCKET_ERROR for an error.
  */

int CDatagramSocket::InitForReceive()
{
	return InitSocket();
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
int CDatagramSocket::Receive( unsigned char * cBuffer, unsigned long nLen )
{
	int nAddrlen = sizeof( m_sockAddr );
    memset( &m_sockAddr, 0, sizeof( m_sockAddr ));

    int nBytes = recvfrom_ew( m_socket, (char *)cBuffer, nLen, 0, (struct sockaddr *)&m_sockAddr, 
                   &nAddrlen, m_Timeout );

    return nBytes;
}

/**
  * Helper function for initialization.  This will perform any initialization that is common
  * for both sending and receiving sockets.
  *
  * @return 0 if all is well, or SOCKET_ERROR for an error.
  */
int CDatagramSocket::InitSocket()
{
    struct sockaddr_in bindAddr;
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

	// Allow broadcasting.
	bValue = TRUE;
	if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (char *)&bValue, sizeof(bValue))
			== SOCKET_ERROR)
	{
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Error %d attempting to allow broadcasting.\n",
						errorval );
		return SOCKET_ERROR;
	}

	// Disable routing.  (Don't know if this is strictly necessary, but it doesn't hurt.)
	bValue = TRUE;
	if (setsockopt(m_socket, SOL_SOCKET, SO_DONTROUTE, (char *)&bValue, sizeof(bValue))
			== SOCKET_ERROR)
	{
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Error %d attempting to allow multiple bindings to same address.\n",
						errorval );
		return SOCKET_ERROR;
	}

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

/**
  * Determines which interface (i.e. network card) is best to use for this socket.
  * The address is stored in m_szBindIPAddress.
  *
  * @return 0 if all is well, or SOCKET_ERROR for an error.
  */
int CDatagramSocket::FindBestAddress()
{
    char *szLocal;
	struct in_addr *pAddr;
	hostent * ph;
	struct in_addr connAddr;
	int i, BestSoFar, errorval;

    // Determine our local address.  Since we may have more than one network interface (and
	// hence more than one address), we'll need to determine the best one to use.
	szLocal = (char *)malloc(256);
    if (gethostname( szLocal, 256 ) == SOCKET_ERROR)
	{
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Error %d calling gethostname().\n",
						errorval );
		free(szLocal);
		return SOCKET_ERROR;
	}
    ph = gethostbyname( szLocal );
	if (ph == NULL)
	{
		errorval = socketGetError_ew();
		reportError( WD_FATAL_ERROR, GENFATERR, "Error %d calling gethostbyname() for host '%s'.\n",
						errorval, szLocal );
		free(szLocal);
		return SOCKET_ERROR;
	}
	free(szLocal);

	connAddr.s_addr = inet_addr(m_ipAddress);

	// Find the first address to look at.
	pAddr = (struct in_addr *)ph->h_addr_list[0];
	i = 0;
	BestSoFar = -1;
	
	// Step through each address until we find the best one.
	while (pAddr != NULL)
	{
		// First, look for addresses on this subnet.
		if (pAddr->S_un.S_un_b.s_b1 == connAddr.S_un.S_un_b.s_b1 
				&& pAddr->S_un.S_un_b.s_b2 == connAddr.S_un.S_un_b.s_b2 
				&& pAddr->S_un.S_un_b.s_b3 == connAddr.S_un.S_un_b.s_b3 )
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
		pAddr = (struct in_addr *)ph->h_addr_list[i];
	}
	// See if we found any likely candidates...
	if (BestSoFar == -1)
	{
		reportError( WD_FATAL_ERROR, GENFATERR, "Unable to find network connection.\n" );
		return SOCKET_ERROR;
	}

	// Extract the address for export.
    strcpy(m_szBindIPAddress, inet_ntoa( *(struct in_addr *)ph->h_addr_list[BestSoFar] ));
	return 0;
}

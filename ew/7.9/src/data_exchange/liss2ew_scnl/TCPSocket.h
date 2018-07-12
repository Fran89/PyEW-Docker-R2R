/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: TCPSocket.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 16:21:58  paulf
 *     added these from Hydra
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:37  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.5  2005/04/21 17:14:04  mark
 *     Changed InitFor"Send and "Receive" to "Connect" and "Listen"; added DNS lookup; added default constructor
 *
 *     Revision 1.4  2005/03/08 23:03:24  mark
 *     Added separate Connect() function; sockets now close immediately
 *
 *     Revision 1.3  2005/03/03 22:13:40  mark
 *     Added m_SendSocket
 *
 *     Revision 1.2  2005/03/03 17:47:49  mark
 *     Added CloseReadSocket
 *
 *     Revision 1.1  2005/02/24 21:24:32  mark
 *     Initial checkin
 *
 *     Revision 1.5  2004/08/03 21:38:51  mark
 *     Accessor functions for IP address and port we're connected to
 *
 *     Revision 1.4  2004/07/01 16:42:43  mark
 *     Moved functionality here from CMulticastSocket
 *
 */

//
// DatagramSocket.h
//
// header file defining the class which handles datagram sockets
//
//

#ifndef TCPSOCKET_H
#define TCPSOCKET_H

extern "C"
{
 #include <socket_ew.h>
 #include <watchdog_client.h>
}

#ifndef WIN32
 extern "C"
 {
  #include <strings.h>
 }
#endif

#define UNKNOWN_IP	"0.0.0.0"

class CTCPSocket
{
public:

	CTCPSocket();
	CTCPSocket(char * ipAddress,unsigned short port);
	CTCPSocket(struct in_addr ipAddrValue,unsigned short port);
	virtual void SetAddress(char * ipAddress, unsigned short port);
	virtual void SetAcceptingPort(unsigned short port);
	virtual ~CTCPSocket();

	virtual int InitForListen();
	virtual int InitForConnect();
	virtual int AcceptConnections();
	virtual int Connect();
	virtual int Send(const unsigned char * buffer, unsigned long len);
	virtual int Receive(unsigned char * buffer, unsigned long len);
	virtual int ReceiveAll( unsigned char * cBuffer, unsigned long nLen );
	virtual void CloseSocket();

	void SetTimeout( int msec )
	{ m_Timeout = msec; }

	const char * GetLocalIP()
	{ return m_szBindIPAddress; }
	const char * GetIP()
	{ return m_ipAddress; }
	const unsigned short GetPort()
	{ return m_port; }

protected:
	virtual int InitSocket();
	virtual int FindBestAddress();

	sockaddr_in			m_ConnectedAddr;
	struct in_addr		m_ConnectingToQuadAddr;
	char *				m_ipAddress;
	char *				m_szBindIPAddress;
	unsigned short		m_port;
	SOCKET					m_socket;
	SOCKET					m_ReadSocket;
	SOCKET					m_WriteSocket;
	int					m_Timeout;
	bool				m_bReceiving;
	bool				m_bInitialized;
	bool				m_bAcceptIP;

private:
	void CreateSocket(char *pIPAddr, unsigned short port);
};


#endif // TCPSOCKET_H

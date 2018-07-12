/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: DatagramSocket.h 2322 2006-06-12 04:36:36Z stefan $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/06/12 04:36:36  stefan
 *     hydra resources
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:37  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.7  2005/03/08 18:06:14  mark
 *     Removed heartbeats
 *
 *     Revision 1.6  2005/02/24 21:25:07  mark
 *     Removed unused member var
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

#ifndef DATAGRAMSOCKET_H
#define DATAGRAMSOCKET_H

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

class CDatagramSocket
{
public:

	CDatagramSocket(char * ipAddress,unsigned short port);
	CDatagramSocket(struct in_addr ipAddrValue,unsigned short port);
	virtual ~CDatagramSocket();

	virtual int InitForReceive();
	virtual int InitForSend();
	virtual int Send(const unsigned char * buffer, unsigned long len);
	virtual int Receive(unsigned char * buffer, unsigned long len);

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

	char *				m_ipAddress;
	char *				m_szBindIPAddress;
	unsigned short		m_port;
	int					m_socket;
    struct sockaddr_in	m_sockAddr;
	unsigned long		m_ConnectedAddr;
	bool				m_bBroadcast;

	int					m_Timeout;

private:
	void CreateSocket(char *pIPAddr, unsigned short port);
};


#endif // DATAGRAMSOCKET_H

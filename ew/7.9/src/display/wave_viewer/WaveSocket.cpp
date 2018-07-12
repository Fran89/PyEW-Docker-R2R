// Wave.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSocket.h"
#include "Wave.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CWaveSocket, CAsyncSocket)

/* Useless default Constructor
********************************************************************/
CWaveSocket::CWaveSocket()
{
}

/* Useless default Destructor
********************************************************************/
CWaveSocket::~CWaveSocket()
{
}


/* Callback method used by the framework to alert us that are
   connect request has been handled(with success or error).
********************************************************************/
void CWaveSocket::OnConnect(int err) 
{
	if(err) 
		Error("OnConnect", err);
  else
		TRACE("OnConnect: Connection succeeded.\n");
  CAsyncSocket::OnConnect(err);

  TRACE("CWaveSocket:OnSocketConnect(): SocketConnected!\n");
  // Call our CWave object and alert it to the connection
	this->pWave->OnSocketConnect(err);
}


CString *CWaveSocket::Error(char *mod) 
{
  return(Error(mod,GetLastError()));
}


// We keep a single error string.  We could have declared it
// as static within Error()
CString cErr;

CString *CWaveSocket::Error(char *mod, int err) 
{
	char txt[80];

  // Convert the error to a string!!
	switch(err) {
	case WSANOTINITIALISED:	strcpy(txt, "Not initialized"); break;
	case WSAENETDOWN:		strcpy(txt, "Network down"); break;
	case WSAEAFNOSUPPORT:	strcpy(txt, "Not supported"); break;
	case WSAEINPROGRESS:	strcpy(txt, "Blocked"); break;
	case WSAEMFILE:			strcpy(txt, "No file desc."); break;
	case WSAENOBUFS:		strcpy(txt, "No buffer space"); break;
	case WSAEPROTONOSUPPORT: strcpy(txt, "No port support"); break;
	case WSAEPROTOTYPE:		strcpy(txt, "Wrong port type"); break;
	case WSAESOCKTNOSUPPORT: strcpy(txt, "Port / family conflict"); break;
	case WSAEADDRINUSE:		strcpy(txt, "Address in use"); break;
	case WSAEADDRNOTAVAIL:  strcpy(txt, "Address no available"); break;
	case WSAECONNREFUSED:   strcpy(txt, "Connection refused."); break;
	case WSAEDESTADDRREQ:   strcpy(txt, "Destination required."); break;
	case WSAEFAULT:			strcpy(txt, "nSockAddrLen incorrect."); break;
	case WSAEINVAL:			strcpy(txt, "Invalid."); break;
	case WSAEISCONN:		strcpy(txt, "Already connected."); break;
	case WSAENETUNREACH:	strcpy(txt, "Unreachable."); break;
	case WSAENOTSOCK:		strcpy(txt, "Not a socket."); break;
	case WSAETIMEDOUT:		strcpy(txt, "Timed out."); break;
	case WSAEWOULDBLOCK:	strcpy(txt, "Would block."); break;
	case WSAECONNRESET:		strcpy(txt, "Connection reset"); break;
	case WSAECONNABORTED:	strcpy(txt, "Connection aborted"); break;
	default: sprintf(txt, "Unknown error = %d\n", err); break;
	}
	cErr = CString(txt);
  // log the error as a debug statement, and also return the
  // generated error string.
	TRACE("%s: %s\n", mod, txt);
	return &cErr;
}  // end CWaveSocket::Error()


/* Method to create the socket.  This allows the MFC socket framework
   and kernel to perform neccessary initialization (outside of the 
   CAsyncSocket class before we use the socket.
********************************************************************/
BOOL CWaveSocket::Create(CWave *pWave) 
{
	this->pWave = pWave;
  // call our Super's method
	BOOL b = CAsyncSocket::Create(0, SOCK_STREAM,
		FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
	if(!b)
		Error("Create", GetLastError());
	return b;
}


/* Method to connect to the server's listening socket.
********************************************************************/
BOOL CWaveSocket::Connect(const char *ip, int port) 
{
	int err;
  int nValue;

  // Call our Super's connect method
	BOOL b = CAsyncSocket::Connect(ip, port);
  // handle any resulting errors
	if(!b) 
  {
		err = GetLastError();
		switch(err) {
		case WSAEWOULDBLOCK:
			TRACE("Connect: Connection pending\n");
			return TRUE;;
		default:
			Error("Connect", err);
			break;
		}
	}
  // Set the socket to Non-blocking!!
  this->SetSockOpt(SO_DONTLINGER,&nValue,sizeof(nValue));

  // return the result of the connect request
	return b;
}


/* Callback method used by the framework to alert us that the socket
   is ready and that we may send data over it, or that something has
   happened to the socket, and it is thus broken
********************************************************************/
void CWaveSocket::OnSend(int err) 
{
  // record the error if there is one!
	if(err)
		Error("OnSend", err);
	else
		TRACE("OnSend: Ready to send\n");
}

// Declare a small static buffer that we can use
// to grab things out of the socket-buffer, one
// chunk at a time.
static char Buf[8192];


/* Callback method used by the framework to alert us that their is
   data from the server sitting in the socket buffer.  We can call
   Receive() to get the data.
   Alternatively, the socket may be broken, in which case, we will
   be passed the socket error code.
********************************************************************/
void CWaveSocket::OnReceive(int err) 
{
	int n= sizeof(Buf) - 1;

  // make sure there is no error.
	if(err)
  {
		Error("OnReceive", err);
  }
	else 
  {
    // grab data, until we've got it all
    while(n == sizeof(Buf) - 1)
    {
      // Grab the data out of the socket buffer.
      n = Receive(Buf, sizeof(Buf)-1);
      if(n > 0) 
      {
        Buf[n] = 0;
        // Pass the data to our CWave object
        pWave->OnSocketReceiveData(Buf,n);
      } else 
      {
        Error("Receive", GetLastError());
      }
    }  // end while we haven't grabbed it all yet.
	}
}


/* Callback method used by the framework to alert us that the socket
   has been closed.  It works sporadically, and we try not to depend
   on it.
********************************************************************/
void CWaveSocket::OnClose(int err) 
{
	if(err) 
		Error("OnClose", err);
  else 
		TRACE("OnClose: Connection closed.\n");

  // Notify our super, and our CWave object.
  CAsyncSocket::OnClose(err);
  pWave->OnSocketClose(err);
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSocket message handlers


#ifndef _WAVESOCKET_H_
#define _WAVESOCKET_H_
// Wave.h : header file
//
class CWave;

/////////////////////////////////////////////////////////////////////////////
// CWaveSocket command target

class CWaveSocket : public CAsyncSocket
{
	DECLARE_DYNCREATE(CWaveSocket)

	CWaveSocket();           // protected constructor used by dynamic creation

// Attributes
public:
	CWave *pWave;

protected:
//  Move to CWaveSocket socket class
	int		nSock;		// Socket status
	CString	cServer;	// Server IP address
	int		nPort;		// Server port number
	int		nReceive;	// Receive state
//	DWORD	lRetry;		// Tick count to retry connection

// Operations
public:
	CString *Error(char *mod);
	CString *Error(char *mod, int err);
	BOOL Create(CWave *pWave);
	BOOL Connect(const char *ip, int port);
	virtual void OnConnect(int err);
	virtual void OnClose(int err);
	virtual void OnReceive(int err);
	virtual void OnSend(int err);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSocket)
	//}}AFX_VIRTUAL

// Implementation
	virtual ~CWaveSocket();

};

/////////////////////////////////////////////////////////////////////////////
#endif
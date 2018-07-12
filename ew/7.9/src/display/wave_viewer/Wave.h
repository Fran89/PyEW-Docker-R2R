
#ifndef _WAVE_H_
#define _WAVE_H_
// Wave.h : header file
//

#include <afxmt.h>

#define CLIENT_SOCKET_TIMEOUT 15
class CWaveDoc;
class CWaveSocket;
class CSite;
/////////////////////////////////////////////////////////////////////////////
// CWave command target

class CWave : public CObject
{
	DECLARE_DYNCREATE(CWave)

	CWave();           // protected constructor used by dynamic creation

// Attributes
public:

#define WAVESOCKET_STATUS_CLOSED 0
#define WAVESOCKET_STATUS_READY 1
#define WAVESOCKET_STATUS_NOT_CREATED -1
#define WAVESOCKET_STATUS_CONNECTION_PENDING 2
protected:
	CWaveSocket *pWaveSocket;
	int		iSock;		// Socket status
  int   iMsgLen;
	CString	sServer;	// Server IP address
	int		nPort;		// Server port number
	int		nReceive;	// Receive state
  BOOL  bWaitingForReply;
  int   iMsg;
  char  sStatus[200];
 	CMutex m_mutex;  
  CSingleLock * psHNRLock, * psGMLock, * psGSCNLLock;
  time_t tWaitingForReply;

  CWaveDoc * pWaveDoc;
  void Connect(void);
  void HandleNextRequest(void);

// Operations
public:
  ~CWave();
  CWave(CString * cServer, int nPort, CWaveDoc * pWaveDoc);
	void GetSCNLTraceData(CSite * pSite, double tStart, double tEnd);
  void GetMenu(void);
  void OnSocketConnect(int err);
	void OnSocketClose(int err);
  void OnSocketReceiveData(char * pBuffer, int BufLen);
  void ClearRequestQueue(void);
  int  GetRequestQueueSize(void);
  void SetRequestQueueLen(int iLen);
  
};

/////////////////////////////////////////////////////////////////////////////
#endif
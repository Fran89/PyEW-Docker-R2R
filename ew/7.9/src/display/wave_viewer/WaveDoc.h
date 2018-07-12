// WaveDoc.h : header file
//
#include "comfile.h"
#include "trace.h"

class CWave;

#define MAXTOK 80



// STRING__NO_LOC for blank location code.  
//  also defined in CGroup
#define STRING__NO_LOC "--"

// States of message parsing
enum {
	MSG_IDLE,
	MSG_MENU,
	MSG_GETSCNL,
	MSG_FAIL
};

// Socket states
enum {
	SOCK_IDLE,
	SOCK_CONNECT,
	SOCK_PENDING,
	SOCK_CONNECTED,
	SOCK_MENU,
	SOCK_ACTIVE,
	SOCK_OFFLINE
};

// States of Receive()
enum {
	RECEIVE_IDLE,
	RECEIVE_WHITE,
	RECEIVE_TOKEN
};

// Define constants for dealing with wave_serverV types
#define WAVE_SERVER_TYPE_UNDEFINED 0
#define WAVE_SERVER_TYPE_SCN  1
#define WAVE_SERVER_TYPE_SCNL 2


/////////////////////////////////////////////////////////////////////////////
// CWaveDoc document
class CWaveDoc : public CDocument
{
protected:
	CWaveDoc();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWaveDoc)

// Attributes
public:
	CWave	*pWave;

  
//	double	dBegin;		// Time of beginning of wave tank
//	double	dEnd;		// Time at end of wave tank
	CString	cServer;	// Server IP address
	int		nPort;		// Server port number
	long	nCache;		// Default trace cache size
	int		iMsg;		// Last request id (serial)
	CObArray	arrTrace;	// Trace array
  int iWaveServerType;
  bool bWaveServerTypeDefined;

//	int		nMsg;		// Last message type requested (lame protocol!)
//  should be replaced with an array of requests, length 20

//  int		iToken;		// Token index (first is 0)
//	int		nToken;		// Part length
//	char	sToken[MAXTOK];	// Token
//	CObArray	arrMenu;	// Site list from menu command

protected:
	double	dGulp;		// Maximum duration of server requests (sec)
  int     iRefreshMenuOnly;
	int			nMaxTrace;	// Maximum number of traces to display

private:
  BOOL  bTraceWasUpdated;

// Operations
public:
  void SetTitle(LPCTSTR lpszTitle);
	BOOL Com(CComfile *cf);
//	void Poll();
	void CleanupTraceArray();
//	Send(char *txt);
//	BOOL Menu(int dummy);
//	void Menu();
//	BOOL GetScn(int itrace, double t1, double t2);
//	void GetScn();
//	virtual void Connect(BOOL connected);
//	virtual void Receive(int n, char *buf);
  BOOL TraceHasBeenUpdated(void);
  void SetTraceHasBeenUpdated(BOOL);
  virtual void HandleWSMenuReply(CString sReply);
  void HandleWSSCNLReply(CString sReply, CSite * pSite);
  void MakeTraceRequest(double dStart, double dEnd, const CSite * pSiteIn);
  void UnblockWaitingTraces(void);
 	CMutex m_mutex;  // Must be public


private:
  int  HandleReplyFlags(CString cScnFlags, CTrace * pTrace);
  CTrace * LocateTraceForSite(CSite * pSite);

//	BOOL Request();

protected:
  void Open(CSite &Site);
  CSingleLock * psLock;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWaveDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

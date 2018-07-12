#ifndef _TRACE_H_
#define _TRACE_H_
// Trace.h : header file
//

#include "site.h"

#define TRACE_BLOCK_UNBLOCK            0
#define TRACE_BLOCK_WAITING_FOR_QUEUE  1
#define TRACE_BLOCK_LEFT_OF_TANK       2
#define TRACE_BLOCK_RIGHT_OF_TANK      3
#define TRACE_BLOCK_NO_DATA_AVAILABLE  4
#define TRACE_BLOCK_ERROR_GETTING_DATA 5

enum {
	TRACE_BOGUS,
	TRACE_INITIALIZE,
	TRACE_ACTIVE
};

class CWaveDoc;

/////////////////////////////////////////////////////////////////////////////
// CTrace command target

class CTrace : public CObject
{
	DECLARE_DYNCREATE(CTrace)

	CTrace();           // protected constructor used by dynamic creation

  friend class CBand;
public:
// Methods
	CTrace(CSite & Site, int nBuf, CWaveDoc * pWaveDoc);
	long Index(double t);
	double T(long index);
	int Append(double t, double step, int n, long *buf);
  void Block(int iBlockCode);
  void RemoveBlock(void);
  BOOL CheckForNeededTrace(double dStart, double dEnd);
  int GetBlockCode(void);
  BOOL bCheckAgain;
  double GetLeftOfTank(void);
  double GetRightOfTank(void);
  void GetSite(CSite * pSite);
  void UpdateMenuBounds(double dMenuStart, double dMenuEnd);
  double GetSampleRate(void);

  // Implementation
	virtual ~CTrace();

// Attributes
protected:
  int iTrace;   // Index in Trace Array
	int nFlag;		// Update flag
	int nBuf;		// Buffer size (samples)
	double	dStart;	// Earliest time in buffer
	double	dStop;	// Latest time in buffer
	double	dStep;	// Step size (sampling interval) seconds
	double	dTotal;	// Buffer length (seconds)
	HGLOBAL	hBuf;	// Buffer pointer (dereference with GlobalLock
	int iBlock;		// Non zero if blocked by end of tank condition
  bool bBlockStateChanged;
  CWaveDoc * pWaveDoc;
	CSite Site;		// Site index in menu array
  BOOL bNewTraceRecvd;
  double dNewTraceStart;
  double dNewTraceEnd;
  BOOL CheckAndSetCacheBoundaries(double dStart, double dEnd);


};

// FILL_VALUE is the value passed back by the waveserver as fill for
// data samples that are not available.  FILL_VALUE's should not be
// mathematically evaluated or drawn.  DK 20000525
#define FILL_VALUE "X"

/////////////////////////////////////////////////////////////////////////////
#endif


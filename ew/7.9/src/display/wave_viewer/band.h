#ifndef _BAND_H_
#define _BAND_H_
// Band.h : header file
//

#include "trace.h"

// Band states
enum {
  BAND_BOGUS,
	BAND_INIT,	// Initialize
	BAND_SETUP,	// Calculate bias and gain
	BAND_ACTIVE	// Initialized
};

/////////////////////////////////////////////////////////////////////////////
// CBand Object

class CDisplay;
class CBand : public CObject
{
	DECLARE_DYNCREATE(CBand)

	CBand();           // protected constructor used by dynamic creation

 friend class CDisplay;

// Operations
public:
	CBand(CTrace * pTrace);
	void Calibrate(CDisplay *dsp);
	void Frame(CDC *pdc, CDisplay *dsp, RECT *rect);
	void Head(CDC *pdc, CDisplay *dsp, RECT *rect);
	BOOL DrawTrace(CDC *pdc, CDisplay *dsp, RECT *rect,
		double tstart, double tstop, BOOL bForceRedraw);
  BOOL DrawBand(CDC *pDC, CDisplay *pDisplay, RECT *pRect,
                double tstart, double tstop, BOOL bDrawHeader,
                BOOL bForceRedraw=TRUE);
  void UnblockTrace(void);
  BOOL CheckForNeededTrace(double dStart, double dEnd);
  void DrawBlock(CDC * pDC, CDisplay *pDisplay, RECT *pRect);

  CBand& operator=(const CBand& cbIn);

// Implementation
	virtual ~CBand();


// Attributes
protected:
	int    nState;		// Band state
	long   nBias;		// Bias
	int    nScale;	// Scale factor (this is the maximum +- value we can draw 
                  // within the band
	double dTrigger;	// Trigger time (or 0.0)
  CTrace * pTrace;
  int  iCalibrated;

};

/////////////////////////////////////////////////////////////////////////////
#endif
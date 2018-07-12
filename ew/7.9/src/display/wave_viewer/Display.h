#ifndef _DISPLAY_H_
#define _DISPLAY_H_
// Display.h : header file
//

#include "band.h"
#define MAX_NUM_BANDS 50

class CSurfView;
/////////////////////////////////////////////////////////////////////////////
// CDisplay command target

class CDisplay : public CObject
{
	DECLARE_DYNCREATE(CDisplay)

public:
	CDisplay();           // protected constructor used by dynamic creation
	CDisplay(CSurfView * pView);  // construct that includes a pointer to our view
  double GetStartTime();  // Get the starttime of the display (seconds since 1970)
  double GetEndTime();    // Get the endtime   of the display (seconds since 1970)
  void SetEndTime(double dEndTime);     // Set the endtime of the display (seconds since 1970)
  void SetDuration(double dDuration);   // Set the duration of the display (seconds)
  void SetScreenDelay(double dDelay);   // Set the delay of the display from realtime
  int  GetBandHeight();  // Get the height of each band in Pixels.
  void SetBandHeight(int);  // Set the height of each band in Pixels.
  int  GetHeaderWidth();    // Get the Width of the header for the bands
  double GetPixelsPerSecond(); // Get the pixels/time ratio for the display
  double   ShiftStartTime(double dNumSeconds);
  int    ScrollBands(int iNumBands);
  int    GetNumberOfBands(void);
  BOOL   GetInfoForBand(int iBand, int * piTrace, int * pnScale, int * pnBias);
  void   GetBandRect(int iBand, RECT * pRect);
  void   AdjustGain(int iBand, float dGainMultiplier);
  int    AdjustNumberOfBands(int nAdjustment);
  void   RecalibrateBand(int iBand);
  void   AdjustDuration(float dTimeMultiple);
  int    GetTopBand(void);
  BOOL   CheckForNeededTrace();
  void   SomethingHappened();
  void   ChangeDisplaySize(void);
  BOOL   Yes_SomethingDidHappen(void);
  void   SetStartTime(double dStartTime); // Set the starttime of the display (seconds since 1970)
  void   DrawDisplay(CDC *pdc, BOOL bForceRedraw);
  void   ResetBandBlocks();
  double CalcShiftStartTime(double dNumSeconds);

  void   nlSetTraceArray(CObArray * parrTrace);
	void    SetScale(int iScale);
  void    SetbHumanScale(void);
  BOOL    GetbHumanScale(void);
// Attributes
protected:
	int		  nBandHeight;			// Pixels per trace
	int		  nHeaderWidth;			// Pixels on left;
	double	dPixsPerSec;		  	// Time scale factor (pixels / second)
	double	dStart;			// Starting time (Microsoft, secs past 1970)
	double	dDuration;	// Ending time
	int			iBand1;		  // Index of first visible trace
	CObArray	arrBand;	// Band array
  RECT    rDisplay;  // Trace rectangle
 	CObArray * parrTrace;	// Trace array
  CSurfView * pView;
  BOOL bSomethingHappened;
  CSingleLock * psLock;
  CSingleLock * psDDLock;
  int     iScale;
  BOOL    bHumanScale;
  void    nlInvalidateAllBands(void);
  void    nlReDrawBand(int iBand);
  void    nlDrawDisplay(CDC *pdc, BOOL bForceRedraw);
  void    nlInvalidateBand(int iBand);
  void    nlResetBandBlocks();
  void    nlSetStartTime(double dStartTime); // Set the starttime of the display (seconds since 1970)
  void    nlReDrawAllBands();
  int     nlSetNumberOfBands(int iNumBands);
  void    nlSetHeaderWidth(int); // Set the Width of the header for the bands

private:
  void   SetNumberOfBands_internal(int iNumBands);
	int	   iNumBands;	// Index of last visible trace
  double dDelayFudgeFactor;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDisplay)
	//}}AFX_VIRTUAL

// Implementation
	virtual ~CDisplay();

};

/////////////////////////////////////////////////////////////////////////////
#endif
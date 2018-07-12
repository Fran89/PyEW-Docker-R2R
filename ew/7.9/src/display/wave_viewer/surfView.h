// surfView.h : interface of the CSurfView class
//
/////////////////////////////////////////////////////////////////////////////
#include "display.h"

// Time till the block flags on trace will be reset (if no remaining requests)
# define  BLOCK_RESET_TIME_DELAY 10



#define DISPLAY_TIME_TANK_START   -2    
#define DISPLAY_TIME_TANK_END     -3    

class CComfile;
class CSurfView : public CView
{
protected: // create from serialization only
	CSurfView();
	DECLARE_DYNCREATE(CSurfView)

// Attributes
protected:
//	UINT		iTimer;		// Timer id while continuous scroll active
//	int			nFrmInt;	// Scroll frame interval (ms)
//	double		dSpeed;		// Scroll speed (s/s)
	RECT		rScroll;	// Scrolling rectangle
	CDisplay	*pDsp;		// Display scaling attributs
//	BOOL		bRequest;	// True when request active 
	// Members used to control auto advance tracking realtime
	BOOL		bAdvance;	// Autoscroll (on or off)
	double		dAdvLast;	// Time (seconds) at autoscroll start
	double		dAdvNow;	// Current scroll time
	double		dAdvStep;	// Autoscroll update interval (seconds)
  CFont   HeadingFont;  // For displaying heading of each band

// Operations
public:
	CSurfDoc* GetDocument();
	BOOL Com(CComfile *cf);
	double Seconds();
	void Reset();
	void StatusBar(int pane, const char *txt);
//	BOOL Triage(int itrace, int priority, double t1, double t2);
//	BOOL Request();
	void Panel(int iband, RECT *r);
	void Trace(CDC *pdc, int iband, double start, double stop);
//	void Scroll(double start);
  double HScroll(double tscroll);
	void VScroll(int iup);
  void ForceRedrawOfBand(int iBand);
  int SetViewFont(int FontSize, int Initialization);
  CDisplay * GetDisplay();

// Added DavidK 19990708
private:
  BOOL HandleAutoAdvance(void);
  int tTimeTillUnblock;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSurfView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSurfView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSurfView)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFunctionEvent();
	afx_msg void OnFunctionSelect();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnFunctionNext();
	afx_msg void OnFunctionLast();
	afx_msg void OnFunctionAddATrace();
	afx_msg void OnFunctionDropATrace();
	afx_msg void OnFunctionRecalibrate();
	afx_msg void OnFunctionStretch();
	afx_msg void OnFunctionShrink();
	afx_msg void OnFunctionLouder();
	afx_msg void OnFunctionSofter();
	afx_msg void OnFunctionGoNew();
	afx_msg void OnFunctionGoOld();
	afx_msg void OnFunctionAdvance();
	afx_msg void OnUpdateFunctionAdvance(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in surfView.cpp
inline CSurfDoc* CSurfView::GetDocument()
   { return (CSurfDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

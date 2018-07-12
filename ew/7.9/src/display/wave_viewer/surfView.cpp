// surfView.cpp : implementation of the CSurfView class
//

/* DavidK 19990707 added a Y2K fix to a TRACE debugging statement */
/* DavidK 19990708 added a time delay in seconds, to prevent wave_viewers 
   from constantly requesting non-obtainable data.  Now wave_viewer will 
   only retry after BLOCK_RESET_TIME_DELAY seconds (in general). ****/
/* DavidK 19990708 added ResetBandBlocks() calls to several of the function
   to bypass the BLOCK_RESET_TIME_DELAY delay when the user changes the
   trace being viewed.  */


#include "stdafx.h"
#include "surf.h"
#include "eventdlg.h"
#include "surfDoc.h"
#include "select.h"
#include "mainfrm.h"
#include "surfView.h"
#include "comfile.h"
#include "band.h"
#include "date.h"
#include "catalog.h"
#include "quake.h"
#include "phase.h"
#include "site.h"
#include <math.h>
#include <sys\timeb.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CSurfApp *pApp;
static CMainFrame *pFrm;
RECT rPanel;		// Static temporary storage for panel rectangles
static bConnected = FALSE;
static char Txt[80];
static CDate c1970(1970, 1, 1, 0, 0, 0.0);	// MSDOS time base
static CSelect *pSelect = NULL;
static CEventDlg *pDlg;
static CEventDlg cDlg;
static time_t tmStart;


// DK CHANGE 011602
static double dSpeed;
static int    iTimer;

// END DK CHANGE 011602


#define TIMER1 200
#define SB_STATUS	0
#define SB_EVENT	5
#define SB_GROUP	2
#define SB_RECEIVE	3
#define SB_TIME		4
#define SB_SCN		1

/////////////////////////////////////////////////////////////////////////////
// CSurfView

IMPLEMENT_DYNCREATE(CSurfView, CView)

BEGIN_MESSAGE_MAP(CSurfView, CView)
	//{{AFX_MSG_MAP(CSurfView)
	ON_WM_TIMER()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_COMMAND(ID_FUNCTION_EVENT, OnFunctionEvent)
	ON_COMMAND(ID_FUNCTION_SELECT, OnFunctionSelect)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_FUNCTION_NEXT, OnFunctionNext)
	ON_COMMAND(ID_FUNCTION_LAST, OnFunctionLast)
	ON_COMMAND(ID_FUNCTION_ADDATRACE, OnFunctionAddATrace)
	ON_COMMAND(ID_FUNCTION_DROPATRACE, OnFunctionDropATrace)
	ON_COMMAND(ID_FUNCTION_RECALIBRATE, OnFunctionRecalibrate)
	ON_COMMAND(ID_FUNCTION_STRETCH, OnFunctionStretch)
	ON_COMMAND(ID_FUNCTION_SHRINK, OnFunctionShrink)
	ON_COMMAND(ID_FUNCTION_LOUDER, OnFunctionLouder)
	ON_COMMAND(ID_FUNCTION_SOFTER, OnFunctionSofter)
	ON_COMMAND(ID_FUNCTION_GO_NEW, OnFunctionGoNew)
	ON_COMMAND(ID_FUNCTION_GO_OLD, OnFunctionGoOld)
	ON_COMMAND(ID_FUNCTION_ADVANCE, OnFunctionAdvance)
	ON_UPDATE_COMMAND_UI(ID_FUNCTION_ADVANCE, OnUpdateFunctionAdvance)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSurfView construction/destruction

CSurfView::CSurfView()
{

	pApp = (CSurfApp *)AfxGetApp();
//	bRequest = FALSE;
	bAdvance = FALSE;
	dAdvStep = 0.5;
  tTimeTillUnblock = 25;
//	iTimer = 0;
//	dSpeed = 0.0;
//	nFrmInt = 100;	// 100 sets scroll update rate to 10hz = 100 / 1000 (ms)
  //SetViewFont(0,1);
  time(&tmStart);
  pDsp = new CDisplay(this);
}

CSurfView::~CSurfView()
{
}

BOOL CSurfView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_HSCROLL;
	cs.style |= WS_VSCROLL;
	cs.style |= WS_CLIPCHILDREN;
	cs.style |= WS_MAXIMIZE;
	CSurfApp *app = (CSurfApp *)AfxGetApp();
	app->pView = this;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CSurfView drawing

BOOL CSurfView::Com(CComfile *cf) {
	CString sta;
	CString chn;
	CString net;
	CSurfDoc *doc = GetDocument();

	if(cf->Is("start")) {
		pDsp->SetStartTime(cf->Double());
		return TRUE;
	}
	if(cf->Is("band")) 
  {
		pDsp->SetBandHeight((int)cf->Long());
		return TRUE;
	}
  
	if(cf->Is("ScreenDelay")) 
  {
		pDsp->SetScreenDelay(cf->Double());
		return TRUE;
	}
	else if(cf->Is("TimeTillUnblock")) 
  {
		tTimeTillUnblock = cf->Long();
		return TRUE;
	}
	else if(cf->Is("auto_scroll")) 
  {
		dAdvStep = cf->Double();
		return TRUE;
	}
	else if(cf->Is("scale")) {
		pDsp->SetScale(cf->Long());
    pDsp->SetbHumanScale();
		return TRUE;
	}
	return FALSE;
}

//____________________________________________________________Seconds
// Calculate current time in seconds since 1970
double CSurfView::Seconds() {
  struct _timeb tb;
  static double tnow;
  
  _ftime( &tb );
  tnow = (double)tb.time + (double)tb.millitm*0.001;
  return(tnow);
}

// Recalculate window parameters, also set vertical scroll bar
void CSurfView::Reset() 
{
	CDC *pdc = GetDC();

  pDsp->DrawDisplay(pdc, TRUE);
	ReleaseDC(pdc);
}

void CSurfView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	pFrm = (CMainFrame *)GetParentFrame();
	
	// Schedule interrupt every second to check socket status
    // Also used during continuous scrolling of active tank
	if(!SetTimer(1, TIMER1, NULL)) {
		AfxMessageBox("Cannot start timer, exitting");
		AfxAbort();
	}

	CSurfDoc *pDoc = (CSurfDoc *)GetDocument();
	SetScrollRange(SB_HORZ, 0, 100);
	SetScrollPos(SB_HORZ, 50, TRUE);
	SetScrollRange(SB_VERT, 0, 100);
	SetScrollPos(SB_VERT, 50, TRUE);

//	GetClientRect(&rScroll);
//	rScroll.left += pDsp->GetHeaderWidth();
//	CDC *pdc = GetDC();
//	pDsp->SetResolution(0.3937 * pdc->GetDeviceCaps(LOGPIXELSX));
//	ReleaseDC(pdc);
	StatusBar(SB_EVENT, "Trg: None");
}

static double dShift;
void CSurfView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
//	CDC *pdc;
//	CTrace *tr;
	CQuake *qk;
//	CPhase *ph;
//	CSite *site;
	CString *str;
	char *s;
//	int itrace;
//	int iphase;
//	int i;
	char txt[80];

//	TRACE("CSurfView::OnUpdate, lHint=%ld, pHint=%ld\n", lHint, pHint);
	CSurfDoc *doc = GetDocument();
//	TRACE("OnUpdate %d\n", lHint);
	switch(lHint) {
	case ON_UPDATE_WINDOW_CREATION:		// From window creation, ignore
		break;
	case ON_UPDATE_DISPLAY_INITIALIZATION:	// First initialization of display
		Reset();
		// Check if trigger selected, update marks
		if(doc->iQuake >= 0 && doc->iQuake < doc->cCat.GetSize())
			doc->UpdateAllViews(NULL, 3000, NULL);
//		else
//			Request();
//		CView::OnUpdate(pSender, lHint, pHint);
//		Invalidate(TRUE);
		break;
	case ON_UPDATE_MOVE_DISPLAY_TO_TANK_START:
    //doc->ResetDataRequestQueue();
    pDsp->SetStartTime(DISPLAY_TIME_TANK_START);
    break;
  case ON_UPDATE_MOVE_DISPLAY_TO_TANK_END:	
    //doc->ResetDataRequestQueue();
    pDsp->SetStartTime(DISPLAY_TIME_TANK_END);
    break;
  case ON_UPDATE_MOVE_DISPLAY_TO_TIME:	
    doc->ResetDataRequestQueue();
    pDsp->SetStartTime(*(double *)pHint);
    break;
	case ON_UPDATE_SB_STATUS:	// Update statusbar status field
		str = (CString *)pHint;
		s = str->GetBuffer(80);
		StatusBar(SB_STATUS, (const char *)s);
		break;
	case ON_UPDATE_SB_RECEIVE:	// Update status bar receive field
		str = (CString *)pHint;
		s = str->GetBuffer(80);
		StatusBar(SB_RECEIVE, (const char *)s);
		break;
	case ON_UPDATE_SB_MOUSE_TIME: // Update time under mouse
		str = (CString *)pHint;
		s = str->GetBuffer(80);
		StatusBar(SB_TIME, (const char *)s);
		break;
	case ON_UPDATE_SB_GROUP: // Update group field in status bar
		str = (CString *)pHint;
		s = str->GetBuffer(80);
		StatusBar(SB_GROUP, (const char *)s);
		break;
	case ON_UPDATE_SB_EVENT: // Update event field in status bar
		str = (CString *)pHint;
		s = str->GetBuffer(80);
		StatusBar(SB_EVENT, (const char *)s);
		break;
	case ON_UPDATE_GOTO_QUAKE: // Shift view to selected quake
    {
      TRACE("Quake %d requested, pSelect = %d\n", doc->iQuake, pSelect);
      if(pSelect) 
      {
        pSelect->DestroyWindow();
        delete pSelect;
        pSelect = NULL;
      }
      if(doc->iQuake < 0 || doc->iQuake >= doc->cCat.GetSize()) 
      {
        MessageBeep(MB_OK);
        break;
      }
      qk = (CQuake *)doc->cCat.GetAt(doc->iQuake);
      sprintf(txt, "Trg: %d/%d %s\n", doc->iQuake+1, doc->cCat.GetSize(),
        (const char *)(CDate(qk->dTime).Date18()).Left(12));
      StatusBar(SB_EVENT, txt);
      // Need some real triggers to figure out how to offset trigger times
      pDsp->SetStartTime(qk->dTime - c1970.Time() - 6.0);
      //iBand1 = 0;
      // Set trigger time if triggered
      //for(iBand=0; iBand<arrBand.GetSize(); iBand++) {
      //	band = (CBand *)arrBand.GetAt(iBand);
      //	tr = (CTrace *)doc->arrTrace.GetAt(band->iTrace);
      //	TRACE("tr: %s %s %s\n", tr->cSta, tr->cChn, tr->cNet);
      //	for(iphase=0; iphase<qk->GetSize(); iphase++) {
      //		ph = (CPhase *)qk->GetAt(iphase);
      //		TRACE("ph: %s %s %s\n", ph->cSite, ph->cComp, ph->cNet);
      //		if(ph->cSite != tr->cSta)
      //			continue;
      //		if(ph->cComp != tr->cChn)
      //			continue;
      //		if(ph->cNet != tr->cNet)
      //			continue;
      //		band->dTrigger = ph->dTime - c1970.Time();
      //		TRACE("dTrigger <- %.2f\n", band->dTrigger);
      //		break;
		  //Reset();
		  //bRequest = FALSE; // Force request
	  	//Request();
  		//Invalidate(TRUE);
      break;
    }
	case ON_UPDATE_REDRAW_TRACE: // Trace was modified
    {
    // We should probably be doing some sort of redraw here
      CDC * pDC = GetDC();
      pDsp->DrawDisplay(pDC, TRUE);
      ReleaseDC(pDC);
      break;
    }
	default:	
    TRACE("CSurfView::OnUpdate, invalid hint = %d\n", lHint);
    break;
	}
}

// Update panes in status bar
void CSurfView::StatusBar(int pane, const char *txt) {
	if(pFrm) {
		pFrm->m_wndStatusBar.SetPaneText(pane, txt, TRUE);
	}
}


void CSurfView::OnDraw(CDC* pDC)
{

//	TRACE("CSurfView::OnDraw\n");
	CWaveDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pDsp->DrawDisplay(pDC, TRUE);
}


BOOL CSurfView::HandleAutoAdvance(void) 
{
  double dTempStep;
  if(!bAdvance)
    return(FALSE);

  if(dAdvStep * pDsp->GetPixelsPerSecond() < 2)
  {
    dTempStep = 2.0/pDsp->GetPixelsPerSecond(); 
  }
  else
  {
    dTempStep = dAdvStep;
  }
  dAdvNow = Seconds();
  if((dAdvNow - dAdvLast) > dTempStep)
  {
    // figure out how many steps to advance.
    int nNumSteps;
    double dTempScrollSecs;

    nNumSteps = (int)((dAdvNow - dAdvLast) / dTempStep);
    dTempScrollSecs = HScroll(nNumSteps * dTempStep);
    dAdvLast += dTempScrollSecs;
    return(TRUE);
  }
  //else
  return(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CSurfView message handlers

void CSurfView::OnTimer(UINT nIDEvent) 
{
  BOOL bUpdatePerformed = FALSE;
  static time_t tLast = 0;
  static int iCount = 0;

  if(nIDEvent == 1)
  {
  // How about we use a generic timer, that runs 10 times a second
  // Each run through, we check a standard set of things:
  //  1) Was trace updated
  //  2) Was display changed
  //  3) Should we auto-scroll
  iCount++;
  if(GetDocument()->TraceHasBeenUpdated())
  {
    GetDocument()->SetTraceHasBeenUpdated(FALSE);
    pDsp->SomethingHappened();
//    CDC * pDC = GetDC();
    RECT rTemp;
    GetClientRect(&rTemp);
    rTemp.left += pDsp->GetHeaderWidth();
    //InvalidateRect(&rTemp,FALSE);
    CDC * pDC = GetDC();
    pDsp->DrawDisplay(pDC, FALSE);
    ReleaseDC(pDC);
  }
  else
  {
    pDsp->CheckForNeededTrace();
  }

  if(time(NULL) > tLast + tTimeTillUnblock)
  {
    time(&tLast);
    pDsp->SomethingHappened();
    pDsp->ResetBandBlocks();
  }

  if(pDsp->Yes_SomethingDidHappen())
  {
    GetDocument()->HACK_CheckForEmptyQueue();
    CDC * pDC = GetDC();
    pDsp->DrawDisplay(pDC, FALSE);
    ReleaseDC(pDC);
  }


  if(!(iCount%5))
    HandleAutoAdvance();
  }
  else if(nIDEvent == 2)
  {
    HScroll(dSpeed);
  }


}



double CSurfView::HScroll(double tscroll) 
{
	RECT r;
  RECT rInvalid;
  int iNumPixelsScrolled;
  double tTempScroll;

  tTempScroll = pDsp->CalcShiftStartTime(tscroll);

  if(tTempScroll < 0)
    iNumPixelsScrolled = (int)(tTempScroll * pDsp->GetPixelsPerSecond() - 0.5);
  else
    iNumPixelsScrolled = (int)(tTempScroll * pDsp->GetPixelsPerSecond() + 0.5);

	GetClientRect(&r);
  r.left+=pDsp->GetHeaderWidth();
  rInvalid = r;

  if(tscroll > 0)
  {
    r.right -= iNumPixelsScrolled;
    rInvalid.left = r.right;
  }
  else
  {
    r.left -= iNumPixelsScrolled;
    rInvalid.right = r.left;
  }

	if(iNumPixelsScrolled == 0) 
  {
//		MessageBeep(MB_OK);
		return(0.0);
	}

//  TRACE("HScroll:  We think we're scrolling %d pixels\n", iNumPixelsScrolled);
	ScrollWindow(-iNumPixelsScrolled/*DK Cleanup*/, 0, &r, &r);
  pDsp->ShiftStartTime(iNumPixelsScrolled/pDsp->GetPixelsPerSecond());
	InvalidateRect(&rInvalid,TRUE);

  //DK Cleanup
  CDC * pDC = GetDC();
  pDC->MoveTo(rInvalid.left,rInvalid.top);
  pDC->LineTo(rInvalid.right,rInvalid.top);
  pDC->LineTo(rInvalid.right,rInvalid.bottom);
  pDC->LineTo(rInvalid.left,rInvalid.bottom);
  pDC->LineTo(rInvalid.left,rInvalid.top);
  ReleaseDC(pDC);

  return(iNumPixelsScrolled / pDsp->GetPixelsPerSecond());
}
  

// Scroll up/down by nband
void CSurfView::VScroll(int nband) 
{
	int iNumBandsScrolled;
	RECT r;

	if(nband == 0) 
  {
		MessageBeep(MB_OK);
		return;
	}

  iNumBandsScrolled = pDsp->ScrollBands(nband);

	GetClientRect(&r);

	if(iNumBandsScrolled == 0) 
  {
		MessageBeep(MB_OK);
		return;
	}

	SetScrollPos(SB_VERT, pDsp->GetTopBand(), TRUE);
	ScrollWindow(0, -iNumBandsScrolled * pDsp->GetBandHeight(), &r, &r);
	Invalidate(FALSE);

  //19990708 DK added for quicker trace response after screen change
  pDsp->ResetBandBlocks();
}

// Horizontal scroll behaviour
void CSurfView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	double twid = (rScroll.right - rScroll.left) / pDsp->GetPixelsPerSecond();

	switch(nSBCode) {
	case SB_LEFT:
		break;
	case SB_RIGHT:
		break;
	case SB_LINELEFT:
		HScroll(-0.2*twid);
		break;
	case SB_LINERIGHT:
		HScroll( 0.2*twid);  // DK Cleanup
		break;
	case SB_PAGELEFT:
		HScroll(-0.8*twid);
		break;
	case SB_PAGERIGHT:
		HScroll( 0.8*twid);
		break;
    // CHANGE DK 011601
	case SB_THUMBPOSITION:
		if(iTimer) {
			KillTimer(iTimer);
			iTimer = 0;
		}
		break;
	case SB_THUMBTRACK:
		if(iTimer) {
			dSpeed = ((int)nPos - 50) / 5.0;	// Max rate 10 s/s
		} else {
			iTimer = SetTimer(2, 200, NULL);
			dSpeed = 0.0;
		}
		break;
    // END CHANGE DK 011601
	}
}

void CSurfView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	switch(nSBCode) {
	case SB_LINEUP:	// Scroll up one band
		VScroll(-1);
		break;
	case SB_LINEDOWN:	// Scroll down one band
		VScroll(1);
		break;
	case SB_PAGEUP:	// Scroll up all but one band
		VScroll(0 - (pDsp->GetNumberOfBands() - 1));
		break;
	case SB_PAGEDOWN:	// Scroll down all but one band
		VScroll(pDsp->GetNumberOfBands() - 1);
		break;
//	case SB_THUMBPOSITION:
//		break;
//	case SB_THUMBTRACK:
//		break;
	}
}

void CSurfView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

  pDsp->ChangeDisplaySize();

  // reset our scrolling rectangle
  GetClientRect(&this->rScroll);
  rScroll.left += pDsp->GetHeaderWidth();

	Invalidate();
	Reset();
//	Request();
	
}

void CSurfView::OnFunctionEvent() 
{
	char evt[20];
	sprintf(evt, "%.2f", pDsp->GetStartTime());
	pDlg = new CEventDlg;
	pDlg->cTime = evt;
	pDlg->Create(IDD_DIALOG1, this);
	pDlg->ShowWindow(SW_SHOW);
}

void CSurfView::OnFunctionSelect() 
{
	pSelect = new CSelect();
	CRect r(100, 100, 200, 300);
	pSelect->Create(TVS_HASLINES | WS_OVERLAPPEDWINDOW, r, this, 12345);
	pSelect->ShowWindow(SW_SHOW);
	pSelect->Invalidate();
}

void CSurfView::OnMouseMove(UINT nFlags, CPoint point) 
{

  static char szTemp[100];
 	CTrace *tr;
  int iBand, iTrace;
  int nScale, nBias;
  BOOL bRetCode;
  CSurfDoc * pDoc = GetDocument();


  if(point.x < pDsp->GetHeaderWidth()) 
  {
    iBand = point.y / pDsp->GetBandHeight();
    bRetCode = pDsp->GetInfoForBand(iBand, &iTrace, &nScale, &nBias);
    if(!bRetCode)
    {
      TRACE("ERROR IN CSurfView:OnMouseMove().  Can't get BandInfo for band(%d)\n", iBand);
      return;
    }

		tr = (CTrace *)pDoc->arrTrace.GetAt(iTrace);
    CSite Site;
    tr->GetSite(&Site);
    sprintf(szTemp,"%s %s %s %s +/-%6d B:%6d", 
              Site.cSta, Site.cChn, Site.cNet, Site.cLoc, nScale, nBias);
    StatusBar(SB_SCN, szTemp);
  }
  else
  {
    double t = c1970.Time() + pDsp->GetStartTime() + 
               (point.x - pDsp->GetHeaderWidth()) / pDsp->GetPixelsPerSecond();
    CDate d(t);
    StatusBar(SB_TIME, (const char *)d.Date20());
  }
	
	CView::OnMouseMove(nFlags, point);
}




void CSurfView::ForceRedrawOfBand(int iBand)
{
  RECT r;
  pDsp->GetBandRect(iBand,&r);
  InvalidateRect(&r,TRUE);
}

void CSurfView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int iBand;
  int nBandHeight;

	CSurfDoc *doc = GetDocument();
  nBandHeight = pDsp->GetBandHeight();

	iBand = point.y / nBandHeight;
	if(point.x < pDsp->GetHeaderWidth()) 
  {
    if(point.y - (iBand * nBandHeight) < nBandHeight/2)
      pDsp->AdjustGain(iBand, 2);
    else
      pDsp->AdjustGain(iBand, (float)0.5);
	}
  ForceRedrawOfBand(iBand);
		
	CView::OnLButtonDown(nFlags, point);
}


void CSurfView::OnFunctionNext() 
{
	CSurfDoc *doc = GetDocument();
	int iq = doc->iQuake + 1;
	if(iq < 0 || iq >= doc->cCat.GetSize()) {
		MessageBeep(MB_OK);
		return;
	}
	doc->iQuake = iq;
	doc->UpdateAllViews(NULL, 3000, NULL);

  //19990708 DK added for quicker trace response after screen change
  pDsp->ResetBandBlocks();

}

void CSurfView::OnFunctionLast() 
{
	CSurfDoc *doc = GetDocument();
	int iq = doc->iQuake - 1;
	if(iq < 0 || iq >= doc->cCat.GetSize()) {
		MessageBeep(MB_OK);
		return;
	}
	doc->iQuake = iq;
	doc->UpdateAllViews(NULL, 3000, NULL);

  //19990708 DK added for quicker trace response after screen change
  pDsp->ResetBandBlocks();

}


void CSurfView::OnFunctionAddATrace() 
{
  BOOL bRetCode;

  bRetCode = pDsp->AdjustNumberOfBands(1);

  if(bRetCode)
  {
    SetViewFont(800/pDsp->GetNumberOfBands(),0);
    Invalidate();
  }
  else
  {
    MessageBeep(MB_OK);
  }

  //19990708 DK added for quicker trace response after screen change
  pDsp->ResetBandBlocks();
}

void CSurfView::OnFunctionDropATrace() 
{
  BOOL bRetCode;

  bRetCode = pDsp->AdjustNumberOfBands(-1);

  if(bRetCode)
  {
    SetViewFont(800/pDsp->GetNumberOfBands(),0);
    Invalidate();
  }
  else
  {
    MessageBeep(MB_OK);
  }

  //19990708 DK added for quicker trace response after screen change
  pDsp->ResetBandBlocks();
}

void CSurfView::OnFunctionRecalibrate() 
{
  pDsp->RecalibrateBand(-1);
	Invalidate();
}

// Halve time scale
void CSurfView::OnFunctionStretch() 
{
  pDsp->AdjustDuration(0.5);
	Invalidate();

  //19990708 DK added for quicker trace response after screen change
  pDsp->ResetBandBlocks();
}

// Double time scale
void CSurfView::OnFunctionShrink() 
{
  pDsp->AdjustDuration(2.0);
	Invalidate();

  //19990708 DK added for quicker trace response after screen change
  pDsp->ResetBandBlocks();
}

void CSurfView::OnFunctionLouder() 
{
  pDsp->AdjustGain(-1, 2.0);
	Invalidate();
}

void CSurfView::OnFunctionSofter() 
{
  pDsp->AdjustGain(-1, 0.5);
	Invalidate();
}

void CSurfView::OnFunctionGoNew() 
{
  GetDocument()->RefreshMenu(ON_UPDATE_MOVE_DISPLAY_TO_TANK_END);
  OnUpdate(this, ON_UPDATE_MOVE_DISPLAY_TO_TANK_END, NULL);
}

void CSurfView::OnFunctionGoOld() 
{
  GetDocument()->RefreshMenu(ON_UPDATE_MOVE_DISPLAY_TO_TANK_START);
  OnUpdate(this, ON_UPDATE_MOVE_DISPLAY_TO_TANK_START, NULL);
}


void CSurfView::OnFunctionAdvance() 
{
	if(bAdvance) 
  {
		bAdvance = FALSE;
	} 
  else 
  {
		bAdvance = TRUE;
		dAdvLast = Seconds();
	}
}

void CSurfView::OnUpdateFunctionAdvance(CCmdUI* pCmdUI) 
{
	if(bAdvance)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}


CDisplay * CSurfView::GetDisplay(void)
{
  return(pDsp);
}


int CSurfView::SetViewFont(int FontSize, int Initialization)
{
   // get current font description
   CFont* pFont = GetFont();
   LOGFONT lf;


   if (pFont != NULL)
   {
	   pFont->GetObject(sizeof(LOGFONT), &lf);
   }
   else
   {
	   ::GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);
   }

   if(!Initialization)
   {
     HeadingFont.DeleteObject();
   }
   HeadingFont.CreatePointFont(FontSize*10,lf.lfFaceName,NULL);
   // switch to new font.
   SetFont(&HeadingFont);

   return(0);
}


/////////////////////////////////////////////////////////////////////////////
// CSurfView printing
// Don't discard this crap.  We need it because they are virtual funcs from
//  the base class.   DavidK 082001.

BOOL CSurfView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CSurfView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CSurfView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


/////////////////////////////////////////////////////////////////////////////
// CSurfView diagnostics

#ifdef _DEBUG
void CSurfView::AssertValid() const
{
	CView::AssertValid();
}

void CSurfView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSurfDoc* CSurfView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSurfDoc)));
	return (CSurfDoc*)m_pDocument;
}
#endif //_DEBUG


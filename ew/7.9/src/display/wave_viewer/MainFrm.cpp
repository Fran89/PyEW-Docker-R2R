// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "surf.h"
#include "group.h"
#include "site.h"
#include "surfdoc.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MENU_BASE 6000
static CSurfApp *pApp;
static CMenu cMenu;
static CString cGroup;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_INITMENUPOPUP()
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(MENU_BASE+2, MENU_BASE+32, OnGroup)
END_MESSAGE_MAP()

static UINT indicators[] =
{
//	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_REQUEST,
  ID_INDICATOR_STATION,
	ID_INDICATOR_GROUP,
	ID_INDICATOR_RECEIVE,
	ID_INDICATOR_TIME,
	ID_INDICATOR_EVENT,
};

static double dShift;	// Window shift time (for update message)

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	pApp = (CSurfApp *)AfxGetApp();
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	MoveWindow(0, 0, 600, 400, TRUE);
	ShowWindow(SW_SHOWMAXIMIZED);
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	return 0;
}

// Reset frame window, menu, etc
void CMainFrame::Reset() {
	int i;

	if(!cMenu.LoadMenu(IDR_MAINFRAME)) {
		TRACE("Cannot load menu IDR_MAINFRAME\n");
		AfxAbort();
	}
	char menu[40];
	int n = cMenu.GetMenuItemCount();
	TRACE("Menu contains %d items\n", n);
	for(i=0; i<n; i++) {
		cMenu.GetMenuString(i, menu, sizeof(menu)-1, MF_BYPOSITION);
		TRACE("Menu[%d]: %s\n", i, menu);
	}
	CMenu *pop = new CMenu();
	CGroup *grp;
	pop->CreatePopupMenu();
	pop->AppendMenu(MF_STRING, MENU_BASE+0, "Triggers");
	pop->AppendMenu(MF_STRING, MENU_BASE+1, "Auto");
	for(i=0; i<pApp->arrGroup.GetSize(); i++) {
		grp = (CGroup *)pApp->arrGroup.GetAt(i);
		pop->AppendMenu(MF_STRING, MENU_BASE+i+2, grp->cName);
	}
	cMenu.AppendMenu(MF_POPUP, (unsigned int)pop->m_hMenu, "Groups");
	SetMenu(&cMenu);
	pop->Detach();
	delete pop;
}

void CMainFrame::Shift(double t) {
	dShift = t;
	CSurfDoc *doc = pApp->pDoc;
	doc->UpdateAllViews(NULL, ON_UPDATE_MOVE_DISPLAY_TO_TIME,
                      (CObject *)&dShift);
}

afx_msg void CMainFrame::OnGroup(UINT id) 
{
	char txt[80];

	int igrp = id - MENU_BASE - 2;
	CSurfDoc *doc = pApp->pDoc;
	TRACE("CMainFrame::OnGroup: igrp = %d\n", igrp);
	CGroup *grp = (CGroup *)pApp->arrGroup.GetAt(igrp);
	TRACE("Group <%s> selected\n", grp->cName);
	sprintf(txt, "Grp: %s\n", (const char *)grp->cName);
	cGroup = CString(txt);
	doc->UpdateAllViews(NULL, ON_UPDATE_SB_GROUP, (CObject *)&cGroup);
  if(igrp)
    doc->ApplyGroupFilter(&(grp->arrSite), TRUE);
  else
    doc->ApplyGroupFilter(&(grp->arrSite), FALSE);
    
//	doc->UpdateAllViews(NULL, 1001, NULL);
//	doc->UpdateAllViews(NULL, 1005, NULL);	// Prime the pump
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_MAXIMIZE;

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
	TRACE("CMainFrame::OnInitMenuPopup called\n");
	// TODO: Add your message handler code here
	
}

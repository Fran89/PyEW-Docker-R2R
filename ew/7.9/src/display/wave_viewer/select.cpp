// Select.cpp : implementation file
//

#include "stdafx.h"
#include "surf.h"
#include "surfDoc.h"
#include "surfView.h"
#include "select.h"
#include "date.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelect

IMPLEMENT_DYNCREATE(CSelect, CTreeCtrl)

CSelect::CSelect()
{
	TRACE("CSelect constructed\n");
}

CSelect::~CSelect()
{
}


BEGIN_MESSAGE_MAP(CSelect, CTreeCtrl)
	//{{AFX_MSG_MAP(CSelect)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_CREATE()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelect diagnostics

#ifdef _DEBUG
void CSelect::AssertValid() const
{
	CTreeCtrl::AssertValid();
}

void CSelect::Dump(CDumpContext& dc) const
{
	CTreeCtrl::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSelect message handlers

CSurfDoc *CSelect::GetDocument() {
	CSurfView *pvew = (CSurfView *)GetParent();
	return (CSurfDoc *)pvew->GetDocument();
}

BOOL CSelect::PreCreateWindow(CREATESTRUCT& cs) 
{
	TRACE("CSelect::PreCreateWindow called\n");
	cs.style |= TVS_HASLINES;
	cs.style |= TVS_LINESATROOT;
	
	return CTreeCtrl::PreCreateWindow(cs);
}

void CSelect::OnLButtonUp(UINT nFlags, CPoint point) 
{
	HTREEITEM hitem = GetNextItem(NULL, TVGN_CARET);
	long l = GetItemData(hitem);
	int iq = HIWORD(l);
	int ip = LOWORD(l);
	TRACE("CSelect::OnRButtonUp: iq=%d, ip=%d\n", iq, ip);
	
	CTreeCtrl::OnLButtonUp(nFlags, point);
}

void CSelect::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	TRACE("CSelect::OnLButtonDblClk (%d,%d)\n", point.x, point.y);
	
	CTreeCtrl::OnLButtonDblClk(nFlags, point);
}

void CSelect::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CQuake *q;

	HTREEITEM hitem = GetNextItem(NULL, TVGN_CARET);
	long l = GetItemData(hitem);
	int iq = HIWORD(l);
	int ip = LOWORD(l);
	TRACE("CSelect::OnRButtonDown: iq=%d, ip=%d\n", iq, ip);
	CSurfDoc *doc = GetDocument();
	q = (CQuake *)doc->cCat.GetAt(iq);
	doc->iQuake = iq;
	doc->iPhase = ip;
	doc->UpdateAllViews(NULL, 3000, NULL);	// Force quake selection
	
//	CTreeCtrl::OnRButtonDown(nFlags, point);
}

void CSelect::OnRButtonUp(UINT nFlags, CPoint point) 
{
	HTREEITEM hitem = GetNextItem(NULL, TVGN_CARET);
	long l = GetItemData(hitem);
	int iq = HIWORD(l);
	int ip = LOWORD(l);
	TRACE("CSelect::OnRButtonUp: iq=%d, ip=%d\n", iq, ip);
	
	CTreeCtrl::OnRButtonUp(nFlags, point);
}

int CSelect::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	TRACE("CSelect::OnCreate called\n");
	MessageBeep(MB_OK);
	int i;
	int j;
	CDate *cd;
	CString cd18;
	CQuake *q;
	CPhase *p;
	char txt[80];

//	TRACE("CSelect::OnInitialUpdate\n");
	HTREEITEM hquake;
	HTREEITEM hphase;
	CSurfDoc *doc = GetDocument();
//	CTreeCtrl::OnInitialUpdate();
	for(i=0; i<doc->cCat.GetSize(); i++) {
		q = (CQuake *)doc->cCat.GetAt(i);
		cd = new CDate(q->dTime);
		cd18 = cd->Date18();
		delete cd;
		hquake = InsertItem((const char *)cd18);
		SetItemData(hquake, MAKELONG(1000, i));
		for(j=0; j<q->GetSize(); j++) {
			p = (CPhase *)q->GetAt(j);
			sprintf(txt, "%s %s %s %s: %s",
				p->cSite, p->cNet, p->cComp, p->cLoc, CDate(p->dTime).Date18());
			TRACE("CSelect: %s\n", txt);
			hphase = InsertItem(txt, hquake);
			SetItemData(hphase, MAKELONG(j, i));
		}
	}
	
	return 0;
}

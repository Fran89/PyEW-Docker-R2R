#include <windows.h>
#include <stdio.h>
#include "SummaryScroll.h"
#include "IGlint.h"
#include "SummaryMod.h"
#include "date.h"
#include "str.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

extern CMod *pMod;
#define CSCROLL_TEXT_SIZE 16
#define RAD2DEG  57.29577951308


static int PickDistComp(const void *elem1, const void *elem2) {
	PICK *pck1;
	PICK *pck2;
	pck1 = (PICK *)elem1;
	pck2 = (PICK *)elem2;
//	CDebug::Log(DEBUG_MINOR_INFO,"Comp() %.2f %.2f\n", pck1->dDelta, pck2->dDelta);
	if(pck1->dDelta < pck2->dDelta)
		return -1;
	if(pck1->dDelta > pck2->dDelta)
		return 1;
	return 0;
}

//---------------------------------------------------------------------------------------CScroll
CScroll::CScroll() {
	hFont = 0;
	hText = CSCROLL_TEXT_SIZE;
	pGlint = 0;
	sCat[0] = 0;
	sDelta[0] = 0;
	sFEStr[0] = 0;
  DeltaLen  = 0;
	sFer[0] = 0;
	nPhs = 0;
	tOrigin = -1.0;  // DK 072803

  // DK CLEANUP hack to set window size
  this->iX = 0;
	this->iY = 400;
	this->nX = 800;
	this->nY = 355;
}

//---------------------------------------------------------------------------------------~CScroll
CScroll::~CScroll() {
	if(hFont)
		DeleteObject(hFont);
}

//---------------------------------------------------------------------------------------Init
void CScroll::Init(HINSTANCE hinst, char *title) {
	iStyle |= WS_VSCROLL;
	CWin::Init(hinst, title);
	ScrollBar();
	Refresh();
}

//---------------------------------------------------------------------------------------Refresh
// Redraw map
void CScroll::Refresh() {
	InvalidateRect(hWnd, NULL, true);
	UpdateWindow(hWnd);
}

//---------------------------------------------------------------------------------------Size
void CScroll::Size(int w, int h) {
	nScr = h / hText - 6;
	ScrollBar();
	Refresh();
}

//---------------------------------------------------------------------------------------Draw
void CScroll::Draw(HDC hdc) {
	char txt[128];
	RECT r;
	int h;
	int ipos;
	int ilst;
	int i;

	if(!hFont) {
		hFont = CreateFont(CSCROLL_TEXT_SIZE, (int)(CSCROLL_TEXT_SIZE/1.5), 0, 0, FW_NORMAL,
			false, false, false, OEM_CHARSET,
			OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH, NULL);
      DeltaLen = CSCROLL_TEXT_SIZE * 15;  // hack hack
		if(!hFont)
			return;
		SelectObject(hdc, hFont);
	}

	SelectObject(hdc, hFont);
	GetClientRect(hWnd, &r);

	h = 10;
	ipos = GetScrollPos(hWnd, SB_VERT);
	ilst = ipos + nScr;
	if(ilst > nPhs)
		ilst = nPhs;
	strcpy(txt, " Seq Origin Time           Latitude  Longitude  Depth Neq   RMS Gap   Aff AfGp AfNeq");
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;
	TextOut(hdc, 10, h, sCat, strlen(sCat));
	h += hText;
	TextOut(hdc, 10, h, sDelta, strlen(sDelta));
  if(DeltaLen < 0 || DeltaLen > 500)
  {
    CDebug::Log(DEBUG_MINOR_WARNING,"DeltaLen not right(%d).  Probably corrupted\n", DeltaLen);
    DeltaLen = CSCROLL_TEXT_SIZE * 15;  // hack hack
  }

	SetTextColor(hdc, RGB(255,0,0));
	TextOut(hdc, 10+DeltaLen, h, sFEStr, strlen(sFEStr));
	SetTextColor(hdc, RGB(0,0,0));
  h += 2*hText;
	strcpy(txt, "nPh Sta  Cmp Nt Lc Phase  Delta Azm Toa   Res DeltaT  Aff Dis/Res/PPD  Tag");
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;

	for(i=ipos; i<ilst; i++) {
		sprintf(txt, "%3d %-4s %-3s %-2s %-2s %-6s %5.1f %3.0f %3.0f %5.2f %6.1f %4.1f %3.1f/%3.1f/%3.1f  %s",
			i, Phs[i].sSite, Phs[i].sComp, Phs[i].sNet, Phs[i].sLoc, Phs[i].sPhase,
			Phs[i].dDelta, Phs[i].dAzm, Phs[i].dToa * RAD2DEG, Phs[i].tRes, Phs[i].dT - tOrigin, 
      Phs[i].dAff, Phs[i].dAffDis, Phs[i].dAffRes, Phs[i].dAffPPD, Phs[i].sTag);  // DK 072803
		TextOut(hdc, 10, h, txt, strlen(txt));
		h += hText;
	}

}

//---------------------------------------------------------------------------------------Quake
void CScroll::Quake(char *ent) {
	IMessage *m;
	PICK *pck;
  ORIGIN *org;
	char *str;
	int code;

	if(!pGlint)
		return;

	if(!(org = pGlint->getOrigin(ent)))
  {
    // Must be a deleted quake.  Do nothing
    return;
  }

  CDate dt(org->dT);

	// save the origin time, so we can use it for each pick's deltaT  // DK 072803
	tOrigin = org->dT; 

	sprintf(sCat, "%4d %s %9.4f %10.4f %6.2f%4d %5.2f %3.0f %5.2f %4.2f  %4.2f", org->iOrigin,
		dt.Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
		org->nEq, org->dRms, org->dGap, org->dAff, org->dAffGap, org->dAffNumArr);
	m = pMod->CreateMessage("FlinnEngdahl");
	m->setDbl("Lat", org->dLat);
	m->setDbl("Lon", org->dLon);
	if(pMod->Dispatch(m)) {
		code = m->getInt("Code");
		str = m->getStr("Region");
		if(str)
			sprintf(sFer, "%d:%s", code, str);
	}
	m->Release();
	sprintf(sDelta, "     Delta(%.1f %.1f %.1f)",
		      org->dDelMin, org->dDelMod, org->dDelMax);
	sprintf(sFEStr, "  Flynn-Engdahl: %s",
		      sFer);

	nPhs = 0;
  size_t iPickRef=0;
	while(nPhs < MAXPHS) {
		pck = pGlint->getPicksFromOrigin(org, &iPickRef);
		CDebug::Log(DEBUG_MINOR_INFO,"Quake():%s %d %d\n", ent, nPhs, pck);
		if(!pck)
			break;
    if(!pck->bTTTIsValid)
       continue;
    memcpy(&Phs[nPhs],pck,sizeof(PICK));
		nPhs++;
	}
  pGlint->endPickList(&iPickRef);

  if(nPhs > 0)
    qsort(Phs, nPhs, sizeof(Phs[nPhs]), PickDistComp);


	ScrollBar();
	Refresh();
}

//---------------------------------------------------------------------------------------ScrollBar
// Calculate and instantiate appropriate scroll bar parameters
void CScroll::ScrollBar() {
	CDebug::Log(DEBUG_MINOR_INFO,"ScrollBar():    nPhs:%d nScr:%d\n", nPhs, nScr);
	if(nPhs <= nScr) {
		SetScrollRange(hWnd, SB_VERT, 0, 100, FALSE);
		SetScrollPos(hWnd, SB_VERT, 0, FALSE);
		ShowScrollBar(hWnd, SB_VERT, FALSE);
		return;
	}
	SetScrollRange(hWnd, SB_VERT, 0, nPhs-nScr, FALSE);
	SetScrollPos(hWnd, SB_VERT, 0, FALSE);
	ShowScrollBar(hWnd, SB_VERT, TRUE);
}

//---------------------------------------------------------------------------------------VScroll
void CScroll::VScroll(int code, int pos) {
	CDebug::Log(DEBUG_MINOR_INFO,"VScroll():Scroll %d %d\n", code, pos);
	int ipos = GetScrollPos(hWnd, SB_VERT);
	switch(code) {
	case SB_LINEUP:
		ipos--;
		break;
	case SB_LINEDOWN:
		ipos++;
		break;
	case SB_PAGEUP:
		ipos-=5;
		break;
	case SB_PAGEDOWN:
		ipos+=5;
		break;
	case SB_THUMBPOSITION:
		ipos = pos;
		break;
	}
	if(ipos < 0)
		ipos = 0;
	SetScrollPos(hWnd, SB_VERT, ipos, TRUE);
	Refresh();
}

//---------------------------------------------------------------------------------------LeftDown
void CScroll::LeftDown(int x, int y) {
}

#include <windows.h>
#include <stdio.h>
#include "scroll.h"
#include "IGlint.h"
#include "SummaryMod.h"
#include "date.h"
#include "str.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

extern CMod *pMod;

//---------------------------------------------------------------------------------------CScroll
CScroll::CScroll() {
	hFont = 0;
	hText = 18;
	pGlint = 0;
	sCat[0] = 0;
	sAdd[0] = 0;
	sFer[0] = 0;
	nPhs = 0;
	tOrigin = -1.0;  // DK 072803

  // DK CLEANUP hack to set window size
  this->iX = 0;
	this->iY = 400;
	this->nX = 624;
	this->nY = 340;
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
		hFont = CreateFont(18, 0, 0, 0, FW_NORMAL,
			false, false, false, OEM_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH, NULL);
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
	strcpy(txt, " Seq Origin Time           Latitude  Longitude  Depth Neq   RMS");
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;
	TextOut(hdc, 10, h, sCat, strlen(sCat));
	h += hText;
	TextOut(hdc, 10, h, sAdd, strlen(sAdd));
	h += 2*hText;
	strcpy(txt, "nPh Site  Comp Net  Phase    Delta Azm Toa   Res   Aff  DeltaT  Tag");
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;

	for(i=ipos; i<ilst; i++) {
		sprintf(txt, "%3d %-6s%-5s%-5s%-8s %5.1f %3.0f %3.0f %5.2f%6.1f %7.1f  %s",
			i, Phs[i].sSite, Phs[i].sComp, Phs[i].sNet, Phs[i].sPhase,
			Phs[i].dDelta, Phs[i].dAzm, Phs[i].dToa, Phs[i].dRes, Phs[i].dAff,
			Phs[i].dT - tOrigin, Phs[i].sTag);  // DK 072803
		TextOut(hdc, 10, h, txt, strlen(txt));
		h += hText;
	}

}

//---------------------------------------------------------------------------------------Quake
void CScroll::Quake(char *ent) {
	IMessage *m;
	PICK *pck;
	char *str;
	int code;

	if(!pGlint)
		return;
	ORIGIN *org = pGlint->getOrigin(ent);
	CDate dt(org->dT);

	// save the origin time, so we can use it for each pick's deltaT  // DK 072803
	tOrigin = org->dT; 

	sprintf(sCat, "%4d %s %9.4f %10.4f %6.2f%4d%6.2f", org->iOrigin,
		dt.Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
		org->nEq, org->dRms);
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
	sprintf(sAdd, "     Delta(%.1f %.1f %.1f)  Flynn-Engdahl:%s",
		org->dDelMin, org->dDelMod, org->dDelMax, sFer);

	nPhs = 0;
  size_t iPickRef=0;
	while(nPhs < MAXPHS) {
		pck = pGlint->getPicksFromOrigin(org, &iPickRef);
		CDebug::Log(DEBUG_MINOR_INFO,"Quake():%s %d %d\n", ent, nPhs, pck);
		if(!pck)
			break;
		strcpy(Phs[nPhs].sSite,  pck->sSite);
		strcpy(Phs[nPhs].sComp,  pck->sComp);
		strcpy(Phs[nPhs].sNet,   pck->sNet);
		strcpy(Phs[nPhs].sPhase, pck->sPhase);
		Phs[nPhs].dT = pck->dT;
		Phs[nPhs].dDelta = pck->dDelta;
		Phs[nPhs].dRes = pck->dT - org->dT - pck->dTrav;
		Phs[nPhs].dAzm = pck->dAzm;
		Phs[nPhs].dToa = pck->dToa;
		Phs[nPhs].dAff = pck->dAff;
		strncpy(Phs[nPhs].sTag, pck->sTag, sizeof(Phs[nPhs].sTag));
		Phs[nPhs].sTag[sizeof(Phs[nPhs].sTag)-1] = 0x00;   // DK 072803
		nPhs++;
	}
  pGlint->endPickList(&iPickRef);


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

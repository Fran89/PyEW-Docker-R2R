#include <windows.h>
#include <stdio.h>
#include "canvas.h"
#include "IGlint.h"
#include "ResidualsMod.h"
#include "date.h"
#include "str.h"
extern "C" {
#include "utility.h"
}

extern CMod *pMod;

//---------------------------------------------------------------------------------------CCanvas
CCanvas::CCanvas() {
	hFont = 0;
	hText = 18;
	pGlint = 0;
	nPhs = 0;
  iX = 800;
  iY = 755;
	nX = 480;
	nY = 235;

  int i;
  int iSize = GetNumberOfPhases();
  for(i=0;i < iSize; i++)
  {
	  Pen[i] = CreatePen(PS_SOLID, 0, RGB((Phases[i].iColor & 0x0000ff),
                                        (Phases[i].iColor & 0x00ff00)>>8,
                                        (Phases[i].iColor & 0xff0000)>>16));
  }
  nPen = i;
  BlackPen    = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
  GrayPen     = CreatePen(PS_SOLID, 0, RGB(128, 128, 128));  // was supposed to be for unassoc


  // DK CHANGE 060303  initialize params
	dDelMax = dDelMod = dDelMin = 0.0;  
  memset(Phs,0,sizeof(Phs));

}

//---------------------------------------------------------------------------------------~CCanvas
CCanvas::~CCanvas() {
	if(hFont)
		DeleteObject(hFont);
	for(int i=0; i<nPen; i++)
		DeleteObject(Pen[i]);

	DeleteObject(BlackPen);
	DeleteObject(GrayPen);

}

//---------------------------------------------------------------------------------------Init
void CCanvas::Init(HINSTANCE hinst, char *title) {
	CWin::Init(hinst, title);
	Refresh();
}

//---------------------------------------------------------------------------------------Refresh
// Redraw map
void CCanvas::Refresh() {
	InvalidateRect(hWnd, NULL, true);
	UpdateWindow(hWnd);
}

//---------------------------------------------------------------------------------------Size
void CCanvas::Size(int w, int h) {
	Refresh();
}

//---------------------------------------------------------------------------------------Draw
void CCanvas::Draw(HDC hdc) {
	RECT r;
	SIZE sz;
	int i;
	char txt[16];

	if(!hFont) {
		hFont = CreateFont(hText, 0, 0, 0, FW_NORMAL,
			false, false, false, OEM_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH, NULL);
		if(!hFont)
			return;
		SelectObject(hdc, hFont);
	}
	GetClientRect(hWnd, &r);

	// Frame
	sprintf(txt, "-10");
	GetTextExtentPoint32(hdc, txt, strlen(txt), &sz);
	int xmin = r.left + sz.cx + 10;
	int	xmax = r.right;
	int ymin = r.bottom - 2*sz.cy;
	int ymax = r.top + sz.cy/2+2;
	MoveToEx(hdc, xmax, ymax, 0);
	LineTo(hdc, xmin, ymax);
	LineTo(hdc, xmin, ymin);
	LineTo(hdc, xmax, ymin);
	MoveToEx(hdc, xmin, (ymin+ymax)/2, 0);
	LineTo(hdc, xmax, (ymin+ymax)/2);

	// y axis
	int x;
	int y;
	int h = ymin - ymax;
	int w = xmax - xmin;
	double res;
	for(res=-10.0; res<11.0; res+=5.0) {
		y = (int)(ymin - h*(res + 10.0)/20.0);
		sprintf(txt, "%.0f", res);
		GetTextExtentPoint32(hdc, txt, strlen(txt),&sz);
		TextOut(hdc, xmin-sz.cx-8, y-sz.cy/2, txt, strlen(txt));
		MoveToEx(hdc, xmin-5, y, 0);
		LineTo(hdc,   xmin+5, y);
		MoveToEx(hdc, xmax-5, y, 0);
		LineTo(hdc,   xmax-1, y);
	}

	// x axis
	double del;
	double delmax = dDelMax;
	double delstp = 30.0;
	if(delmax < 100.0)
		delstp = 20.0;
	if(delmax < 60.0)
		delstp = 15.0;
	if(delmax < 40.0)
		delstp = 10.0;
	if(delmax < 30.0)
		delstp = 5.0;
	if(delmax < 10.0)
		delstp = 2.0;
	if(delmax < 5.0)
		delstp = 1.0;
	if(delmax < 2.0) {
		delmax = 2.0;
		delstp = 0.5;
	}

	for(del=0.0; del<delmax+0.1; del+=delstp) {
		x = xmin + (int)(w*del/dDelMax);
		sprintf(txt, "%.0f", del);
		GetTextExtentPoint32(hdc, txt, strlen(txt), &sz);
		TextOut(hdc, x-sz.cx/2, ymin+6, txt, strlen(txt));
		MoveToEx(hdc, x, ymin+5, 0);
		LineTo(hdc,   x, ymin-5);
		MoveToEx(hdc, x, ymax+5, 0);
		LineTo(hdc,   x, ymax-5);
	}

	int tick = 4;
	
  HPEN penOld = (HPEN)SelectObject(hdc, BlackPen);

	for(i=0; i<nPhs; i++) {
		res = Phs[i].dRes;
		del = Phs[i].dDelta;
		x = xmin + (int)(w*del/dDelMax);
		y = (int)(ymin - h*(res + 10.0)/20.0);
		MoveToEx(hdc, x-tick, y, 0);

    /* select the pen color */
    if(Phs[i].ptPhase >= 0 && Phs[i].ptPhase < nPen)
      SelectObject(hdc, Pen[Phs[i].ptPhase]);
    else
      SelectObject(hdc, BlackPen);

		LineTo(hdc, x+tick, y);
		MoveToEx(hdc, x, y-tick, 0);
		LineTo(hdc, x, y+tick);
	}
	SelectObject(hdc, penOld);
}

//---------------------------------------------------------------------------------------Quake
void CCanvas::Quake(char *ent) {

  PICK *pck;
	ORIGIN *org;

	if(!pGlint)
		return;
	if(!(org = pGlint->getOrigin(ent)))
  {
    // Must be a deleted quake.  Do nothing
    return;
  }

	dDelMin = org->dDelMin;
	dDelMod = org->dDelMod;
	dDelMax = org->dDelMax;

	nPhs = 0;
  size_t iPickRef=0;
  PhaseType ptPhase;

	while(nPhs < MAXPHS) {
		pck = pGlint->getPicksFromOrigin(org, &iPickRef);
		if(!pck)
			break;
    if(!pck->bTTTIsValid)
       continue;
		Phs[nPhs].dDelta = pck->dDelta;
		Phs[nPhs].dRes = pck->dT - org->dT - pck->dTrav;
    ptPhase = GetPhaseType(pck->ttt.szPhase);
    Phs[nPhs].ptPhase = ptPhase;
		nPhs++;
	}
  pGlint->endPickList(&iPickRef);


	Refresh();
}

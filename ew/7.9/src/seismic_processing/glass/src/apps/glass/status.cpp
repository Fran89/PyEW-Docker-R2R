#include <windows.h>
#include <stdio.h>
#include "status.h"
extern "C" {
#include "utility.h"
}
#include <Debug.h>
extern CDebug Debug;

//---------------------------------------------------------------------------------------CStatus
CStatus::CStatus() {
	int i;
	nStat = iStat = 0; // DK CHANGE 061003
	for(i=0; i<MAXSTAT; i++) {
		sStat[i] = new char[32];
		sAction[i] = new char[128];
		strcpy(sStat[i], ".");
		strcpy(sAction[i], ".");
	}
}

//---------------------------------------------------------------------------------------~CStatus
CStatus::~CStatus() {
	int i;
	for(i=0; i<MAXSTAT; i++) {
		delete [] sStat[i];
		delete [] sAction[i];
	}
}

//---------------------------------------------------------------------------------------Status
void CStatus::Status(int istat, char *mod, char *msg) {
	if(istat < 0)
		return;
	if(istat >= MAXSTAT)
		return;
	if(istat >= nStat)
		nStat = istat + 1;
	iStat = istat;
	strcpy(sAct, msg);
	strcpy(sStat[iStat], mod);
	Refresh();
	return;
}

//---------------------------------------------------------------------------------------Set
void CStatus::Set(char *msg) {
	if(iStat < 0)
		return;
	if(iStat >= nStat)
		return;
	strcpy(sAct, msg);
	sAct[120] = 0;
	strcpy(sAction[iStat], sAct);
}

//---------------------------------------------------------------------------------------Init
void CStatus::Init(HINSTANCE hinst, char *title) {
	HDC scr = GetDC(NULL);
	int w = GetDeviceCaps(scr, HORZRES);
	int h = GetDeviceCaps(scr, VERTRES);
	iX = 0;
	iY = 0;
	nX = w/2;
	nY = 8*h/10;
	ReleaseDC(NULL, scr);
	CWin::Init(hinst, title);
}

//---------------------------------------------------------------------------------------Draw
void CStatus::Draw(HDC hdc) {
	char txt[256];
	char *str;
	int i;

	HFONT hfont = CreateFont(18, 0, 0, 0, FW_NORMAL,
		false, false, false, OEM_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FIXED_PITCH, NULL);
	if(!hfont) {
		Debug.Log(DEBUG_MINOR_ERROR,"CreateFont failed\n");
		return;
	}
	HFONT *hfold = (HFONT *)SelectObject(hdc, hfont);
	strcpy(txt, " Module Message...");
	SetTextColor(hdc, RGB(0, 0, 0));
	TextOut(hdc, 10, 10, txt, strlen(txt));
	for(i=0; i<nStat; i++) {
		str = sStat[i];
		if(i == iStat) {
			sprintf(txt, "%8s %s", str, sAct);
			SetTextColor(hdc, RGB(255, 0, 0));
		} else {
			sprintf(txt, "%8s %s", str, sAction[i]);
			SetTextColor(hdc, RGB(0, 0, 0));
		}
		TextOut(hdc, 10, 34 + 24*i, txt, strlen(txt));
	}
	SelectObject(hdc, hfold);
	DeleteObject(hfont);
}

//---------------------------------------------------------------------------------------Refresh
// Refresh status display
void CStatus::Refresh() {
	InvalidateRect(hWnd, NULL, true);
	UpdateWindow(hWnd);
}


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: Monitor.cpp 2772 2007-03-02 22:09:52Z stefan $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/03/02 22:09:52  stefan
 *     declared as int lin 49
 *
 *     Revision 1.2  2006/05/22 16:01:25  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.4  2004/09/16 01:00:17  davidk
 *     Cleaned up logging messages.
 *     Downgraded some debugging messages.
 *
 *     Revision 1.3  2004/08/06 00:24:06  davidk
 *     Changed GUI font size. (downsized)
 *     Removed some of the (less useful at this point) status text lines from the display.
 *
 *     Revision 1.2  2004/04/01 22:04:53  davidk
 *     v1.11 update
 *     Changed default window size
 *
 *     Revision 1.3  2003/11/07 22:33:27  davidk
 *     Added RCS Header.
 *     Added UpdateStatus() function, to more efficiently update the
 *     Glass Monitor during periods of inactivity.
 *
 *
 **********************************************************/

#include <windows.h>
#include <stdio.h>
#include "monitor.h"
#include "GlassMod.h"
#include "glass.h"
extern "C" {
#include "utility.h"
}

extern CMod *pMod;
FILE *fCatOut;

#define CSCROLL_TEXT_SIZE 14

static int bUpdatePickTimeOnly = false;

//---------------------------------------------------------------------------------------CMonitor
CMonitor::CMonitor() {
	hFont = 0;
	hText = 18;
  iX = 450;
  iY = 755;
	nX = 350;
	nY = 235;
}

//---------------------------------------------------------------------------------------~CMonitor
CMonitor::~CMonitor() {
	if(hFont)
		DeleteObject(hFont);
}

//---------------------------------------------------------------------------------------Init
void CMonitor::Init(HINSTANCE hinst, char *title) {
	CWin::Init(hinst, title);
}

//---------------------------------------------------------------------------------------Refresh
// Redraw map
void CMonitor::Refresh() {
    InvalidateRect(hWnd, NULL, true);
	UpdateWindow(hWnd);
}

//---------------------------------------------------------------------------------------Refresh
// Redraw map
void CMonitor::UpdateStatus() 
{
	RECT r={0,3*hText,3*hText,4*hText};  // DK Done to improve graphic performance and VNC
	InvalidateRect(hWnd, &r, true);
    bUpdatePickTimeOnly = true;
	UpdateWindow(hWnd);
}

//---------------------------------------------------------------------------------------Size
void CMonitor::Size(int w, int h) {
	Refresh();
}

//---------------------------------------------------------------------------------------Draw
void CMonitor::Draw(HDC hdc) {
	char txt[128];
	RECT r;
	double a = 0.9;
	double b = 1.0 - a;
	MON *mon = &pGlass->Mon[0];
	MON *avg = &pGlass->Mon[1];

	if(hdc) {
		if(!hFont) {
		hFont = CreateFont(CSCROLL_TEXT_SIZE, (int)(CSCROLL_TEXT_SIZE/1.5), 0, 0, FW_NORMAL,
	  		false, false, false, OEM_CHARSET,
  			OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				FIXED_PITCH, NULL);
			if(!hFont)
				return;
			SelectObject(hdc, hFont);
		}
		SelectObject(hdc, hFont);
		GetClientRect(hWnd, &r);
	}

	if(bUpdatePickTimeOnly)
	{
		hLine = 2 * hText;
		sprintf(txt, "%5d", pGlass->iLapse);
	    Line(hdc, txt);
		bUpdatePickTimeOnly = false;
		return;
	}

	hLine = 0;
	Line(hdc, pGlass->sPick);
	Line(hdc, pGlass->sDisp);
	sprintf(txt, "%5d seconds since last pick received.", pGlass->iLapse);
	Line(hdc, txt);

	if(!pGlass->nPick)
		return;

	double total = avg->tAsn + avg->tAss + avg->tLoc + avg->tOther;

  /*
	Line(hdc, "        Idle  Assign   Assoc  Locate   Other");
	sprintf(txt, "Mon %8.4f%8.4f%8.4f%8.4f%8.4f",
		mon->tIdle, mon->tAsn, mon->tAss, mon->tLoc, mon->tOther);
	Line(hdc, txt);
	sprintf(txt, "Avg %8.4f%8.4f%8.4f%8.4f%8.4f",
		avg->tIdle, avg->tAsn, avg->tAss, avg->tLoc, avg->tOther);
	Line(hdc, txt);
	sprintf(txt, "Pct         %8.4f%8.4f%8.4f%8.4f",
		100.0*avg->tAsn/total, 100.0*avg->tAss/total,
		100.0*avg->tLoc/total, 100.0*avg->tOther/total);
	Line(hdc, txt);
  ********************************/
	sprintf(txt, "%10.2f Percent of system capacity",
		100.0*total/(total+avg->tIdle));
	Line(hdc, txt);
	sprintf(txt, "%10.2f Picks per second (throughput)",
		1.0/(total + avg->tIdle));
	Line(hdc, txt);
/*	sprintf(txt, "%10.2f Seconds per pick (throughput)",
		total + avg->tIdle);
	Line(hdc, txt);
*/
	sprintf(txt, "%10.2f Picks per second (associator)",
		1.0/total);
	Line(hdc, txt);
/*
	sprintf(txt, "%10.2f Seconds per pick (associator)",
		total);
	Line(hdc, txt);
*/
	sprintf(txt, "%10.2f Percent picks associated",
		100.0*pGlass->nAssoc/pGlass->nPick);
	Line(hdc, txt);
	sprintf(txt, "%10d Picks processed", pGlass->nPick);
	Line(hdc, txt);
	sprintf(txt, "%10d Picks associated", pGlass->nAssoc);
	Line(hdc, txt);
	sprintf(txt, "%10d Quakes associated", pGlass->nQuake);
	Line(hdc, txt);
}

//---------------------------------------------------------------------------------------Line
void CMonitor::Line(HDC hdc, char *txt) {
	if(hdc) {
		hLine += hText;
		TextOut(hdc, 10, hLine, txt, strlen(txt));
	} else {
    if(fCatOut)
		  fprintf(fCatOut, "%s\n", txt);
	}
}

//---------------------------------------------------------------------------------------CatOut
void CMonitor::CatOut() {
	CDebug::Log(DEBUG_FUNCTION_TRACE, "CMonitor::CatOut Entering\n");

#define szMONITOR_CATALOG "monitor_catalog.txt"
	if(!(fCatOut = fopen(szMONITOR_CATALOG, "a")))
  {
    CDebug::Log(DEBUG_MINOR_WARNING,"CMonitor::CatOut():  ERROR! Could not open output file <%s>\n",
                szMONITOR_CATALOG);
		return;
  }

	Draw(0);
	fclose(fCatOut);
  fCatOut = NULL;
	CDebug::Log(DEBUG_FUNCTION_TRACE, "CMonitor::CatOut Completed\n");
}

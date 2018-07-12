#include <windows.h>
#include <stdio.h>
#include <IGlint.h>
#include <ITravelTime.h>
#include "canvas.h"
#include "FocusGUIMod.h"
//#include "date.h"
//#include "str.h"
extern "C" {
#include "utility.h"
}

#define CSCROLL_TEXT_SIZE 12
#define SECONDS_SEARCH_WINDOW  60.0

extern CMod *pMod;

//---------------------------------------------------------------------------------------Zerg
// Calculate the Zerg function of x such that Zerg(0) = 1.0,
// zerg(-w) = zerg(w) = 0.0. dZerg/dX(0) = dZerg/dX(1) = 0.0.
// Zerg is symetric about 0.0, and is roughly gaussian with a
// half-width of 1. For x < -1 and x > 1, Zerg is defined as 0.
// Zerg is usually called as Zerg(x/w), where w is the half width
// of the distrubution desired.
double Zerg(double x) {
	if(x <= -1.0)
		return 0.0;
	if(x > 1.0)
		return 0.0;
	if(x > 0.001)
		return 2.0*x*x*x - 3.0*x*x + 1;
	if(x < -0.001)
		return -2.0*x*x*x - 3.0*x*x + 1;
	return 1.0;
}

//---------------------------------------------------------------------------------------CCanvas
CCanvas::CCanvas() {
	hFont = 0;
	dcShadow = 0;
	bmShadow = 0;
	hText = CSCROLL_TEXT_SIZE;
	pGlint = 0;
  // DK CLEANUP hack to set window size
  this->iX = 800;
	this->iY = 400;
	this->nX = 480;
	this->nY = 355;
	nPck = 0;
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

  memset(&Org,0,sizeof(Org));

  TTTUsedArray = (char*)calloc(GetNumberOfPhases(),1);
}

//---------------------------------------------------------------------------------------~CCanvas
CCanvas::~CCanvas() {
	if(hFont)
		DeleteObject(hFont);
	if(dcShadow)
  {
		DeleteDC(dcShadow);
    dcShadow = NULL;
  }
	if(bmShadow)
  {
		DeleteObject(bmShadow);
    bmShadow = NULL;
  }
	for(int i=0; i<nPen; i++)
		DeleteObject(Pen[i]);

	DeleteObject(BlackPen);
	DeleteObject(GrayPen);

  if(TTTUsedArray)
  {
    free(TTTUsedArray);
    TTTUsedArray = NULL;
  }

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
void CCanvas::Size(int w, int h) 
{
  this->nX = w;
  this->nY = h;
	if(dcShadow)
  {
		DeleteDC(dcShadow);
    dcShadow = NULL;
  }
	if(bmShadow)
  {
		DeleteObject(bmShadow);
    bmShadow = NULL;
  }

	Refresh();
}

//---------------------------------------------------------------------------------------Draw
void CCanvas::Draw(HDC hdc) {
	RECT r;

	if(!dcShadow) {
		GetClientRect(hWnd, &r);
		dcShadow = CreateCompatibleDC(hdc);
		bmShadow = CreateCompatibleBitmap(hdc, r.right, r.bottom);
		SelectObject(dcShadow, bmShadow);
		if(hFont)
  		SelectObject(dcShadow, hFont);
		PatBlt(dcShadow, 0, 0, 1000, 1000, BLACKNESS);
		return;
	}

	BitBlt(hdc, 0, 0, 1000, 1000, dcShadow, 0, 0, SRCCOPY);
}

void GetTTTName(char *szTTTName, char * szPhaseName, double dDist);

//---------------------------------------------------------------------------------------Generate
void CCanvas::Focus() {
	RECT r;
	SIZE sz;
	char txt[16];
	int i;
	HPEN penold;

  memset(TTTUsedArray,0,GetNumberOfPhases());
	if(!dcShadow)
		return;
	GetClientRect(hWnd, &r);

	if(!hFont) {
		hFont = CreateFont(CSCROLL_TEXT_SIZE, (int)(CSCROLL_TEXT_SIZE/1.5), 0, 0, FW_NORMAL,
			false, false, false, OEM_CHARSET,
			OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH, NULL);
		if(!hFont)
			return;
		SelectObject(dcShadow, hFont);
	}
	PatBlt(dcShadow, 0, 0, 1000, 1000, WHITENESS);

	// Frame
	sprintf(txt, "800");
	GetTextExtentPoint32(dcShadow, txt, strlen(txt), &sz);
	int xmin = r.left + sz.cx + 10;
// DK original	int	xmax = r.right - sz.cx/2 - 2;
	int	xmax = r.right - sz.cx * 6;
	int ymin = r.top + 2*sz.cy;
	int ymax = r.bottom - sz.cy/2 - 2;
	MoveToEx(dcShadow, xmax, ymin, 0);
	LineTo(dcShadow, xmin, ymin);
	LineTo(dcShadow, xmin, ymax);
	LineTo(dcShadow, xmax, ymax);
	LineTo(dcShadow, xmax, ymin);

	// y axis
	int x;
	int y;
	int h = ymin - ymax;
	int w = xmax - xmin;
	double z;
	double zmin = 0.0;
	double zmax;
  double tWindowHalfWidth;

  if(Org.dZ < 100.0)
  {
    zmax = 200.0;
    tWindowHalfWidth = SECONDS_SEARCH_WINDOW / 4.0;
  }
  else
  {
    zmax = 800.0;
    tWindowHalfWidth = SECONDS_SEARCH_WINDOW;
  }
	for(z=zmin; z<zmax+0.1; z+=100.0) {
		y = (int)(ymin - h*z/zmax);
		sprintf(txt, "%.0f", z);
		GetTextExtentPoint32(dcShadow, txt, strlen(txt),&sz);
		TextOut(dcShadow, xmin-sz.cx-8, y-sz.cy/2, txt, strlen(txt));
		MoveToEx(dcShadow, xmin-5, y, 0);
		LineTo(dcShadow,   xmin+5, y);
		MoveToEx(dcShadow, xmax-5, y, 0);
		LineTo(dcShadow,   xmax+5, y);
	}

	// x axis
	double tmin = 0.0 - tWindowHalfWidth;
	double tmax = tWindowHalfWidth;
	double tdel = (tmax-tmin)/8;
	double t;
	for(t=tmin; t<tmax+0.1; t+=tdel) {
		x = xmin + (int)(w*(t-tmin)/(tmax-tmin));
		sprintf(txt, "%.0f", t);
		GetTextExtentPoint32(dcShadow, txt, strlen(txt), &sz);
		TextOut(dcShadow, x-sz.cx/2, ymin-sz.cy, txt, strlen(txt));
		MoveToEx(dcShadow, x, ymin+5, 0);
		LineTo(dcShadow,   x, ymin-5);
		MoveToEx(dcShadow, x, ymax+5, 0);
		LineTo(dcShadow,   x, ymax-5);
	}

	// Plot traveltime curves
	ITravelTime *trv = pMod->pTrv;
	TTEntry ttt, *pttt;
	PICK *pck;
	int nttt;
	int ittt;
	double d;
	double tres;
	bool penup = true;


	if(!trv)
		return;

  nttt = GetNumberOfPhases();

	double zmesh = 5.0;	// Depth resolution of focus mesh
	int nz = (int)((zmax - zmin)/zmesh + 0.1);
	int iz;
  double dDistKM, dAzm;
	penold = (HPEN)SelectObject(dcShadow, Pen[0]);

  // for each pick
	for(i=0; i<nPck; i++) 
  {
		pck = pPck[i];

    if(pck->iOrigin == Org.iOrigin)
      d = pck->dDelta;
    else
   		geo_to_km_deg(Org.dLat, Org.dLon,pck->dLat, pck->dLon, &dDistKM, &d, &dAzm);
    
    if(d > 150.0)  // DK CLEANUP changed from 75.0
      continue;

    // for each traveltime phase
		for(ittt=nttt-1; ittt>PHASE_Unknown; ittt--) 
    {

      /* only process P-Class phases and pP(for depth) */
      if(Phases[ittt].iClass != PHASECLASS_P && Phases[ittt].iNum != PHASE_pP)
         continue;

      //pTTT = trv->vTableList[ittt]->pTable;

      /*********************
			if(d < trv->DMin(ittt))
				continue;
			if(d > trv->DMax(ittt))
				continue;
       *********************/

      penup = true;
			if(ittt < nPen)
      {
				SelectObject(dcShadow, Pen[ittt]);
      }
      else
      {
        DebugBreak();
				SelectObject(dcShadow, penold);
      }
      // for each depth iteration
			for(iz=0; iz<nz; iz++) {
				z = zmin + zmesh*iz;

				pttt = trv->TBestByPhase(z,d,0.0,&ttt,(PhaseType)ittt);
				if(!pttt)
        {
					penup = true;
					continue;
        }

				tres = pck->dT - Org.dT - pttt->dTPhase;
				if(tres < tmin || tres > tmax) 
        {
					penup = true;
					continue;
				}
				x = xmin + (int)(w*(tres-tmin)/(tmax-tmin));
				y = (int)(ymin - h*z/(zmax-zmin));
				if(penup) {
					MoveToEx(dcShadow, x, y, 0);
					penup = false;
				} else {
          if(!TTTUsedArray[ittt])
            TTTUsedArray[ittt] = true;
					LineTo(dcShadow, x, y);
				}
      }  // end for each depth iteration
    }  // end for each phase type

  }  // end for each pick

  int xText = xmax+5;
  int yText = ymin;
  // print legend
  for(i=0; i < nttt; i++)
  {
    if(TTTUsedArray[i])
    {
	    TextOut(dcShadow, xText, yText, Phases[i].szName, strlen(Phases[i].szName));
	    SelectObject(dcShadow, Pen[i]);
	    MoveToEx(dcShadow, xText+1, yText+ 1.25 * sz.cy, 0);
	    LineTo(dcShadow, xText+1+sz.cx*1.5, yText+ 1.25 * sz.cy);
      yText+= 2*sz.cy;
      if(yText > ymax)
      {
        yText = ymin;
        xText += 2+sz.cx*3;
      }
    }
  }  /* end for each phase type in key. */

 	SelectObject(dcShadow, penold);
  /*

  // Legend - Current Origin
  TextOut(dcShadow, xmax+5, ymin+(2*i*sz.cy), "Current", strlen("Current"));
  TextOut(dcShadow, xmax+5, ymin+(2*(i+(1*.6))*sz.cy), "Org ( )", strlen("Org ( )"));

    // This is a mess  - sorry - draw x
	MoveToEx(dcShadow, xmax+54-6, ymin+(2*(i+(.85)))*sz.cy-6, 0);
	LineTo(dcShadow, xmax+54+6, ymin+(2*(i+(.85)))*sz.cy+6);
	MoveToEx(dcShadow, xmax+54+6, ymin+(2*(i+(.85)))*sz.cy-6, 0);
	LineTo(dcShadow, xmax+54-6, ymin+(2*(i+(.85)))*sz.cy+6);

  // Legend - Re-Focused Origin
  TextOut(dcShadow, xmax+5, ymin+(2*(i+(2*.6))*sz.cy)+5, "Focused", strlen("Focused"));
  TextOut(dcShadow, xmax+5, ymin+(2*(i+(3*.6))*sz.cy)+5, "Org ( )", strlen("Org ( )"));

    // This is a mess  - sorry  - draw +
	MoveToEx(dcShadow, xmax+54 - 7, ymin+(2*(i+(2.2)))*sz.cy, 0);
	LineTo(dcShadow, xmax+54 + 7, ymin+(2*(i+(2.2)))*sz.cy);
	MoveToEx(dcShadow, xmax+54, ymin+(2*(i+(2.2)))*sz.cy-7, 0);
	LineTo(dcShadow, xmax+54, ymin+(2*(i+(2.2)))*sz.cy+7);

  */

  /****
	// Plot new hypocenter
	z = zmin + izfocus*zmesh;
	tres = tmin + itfocus*tmesh;
	x = xmin + (int)(w*(tres-tmin)/(tmax-tmin));
	y = (int)(ymin - h*z/(zmax-zmin));
	SelectObject(dcShadow, penold);
	MoveToEx(dcShadow, x-10, y, 0);
	LineTo(dcShadow, x+10, y);
	MoveToEx(dcShadow, x, y-10, 0);
	LineTo(dcShadow, x, y+10);

  // Plot old hypocenter
	Org.dZ;
	x = xmin + (int)(w*(0-tmin)/(tmax-tmin));
	y = (int)(ymin - h*Org.dZ/(zmax-zmin));
	SelectObject(dcShadow, penold);
	MoveToEx(dcShadow, x-10, y-10, 0);
	LineTo(dcShadow, x+10, y+10);
	MoveToEx(dcShadow, x+10, y-10, 0);
	LineTo(dcShadow, x-10, y+10);

  *******/
	// Plot focused hypocenter
	z = Org.dFocusedZ;
	tres = Org.dFocusedT - Org.dT;
	x = xmin + (int)(w*(tres-tmin)/(tmax-tmin));
	y = (int)(ymin - h*z/(zmax-zmin));
	SelectObject(dcShadow, penold);
	MoveToEx(dcShadow, x-10, y, 0);
	LineTo(dcShadow, x+10, y);
	MoveToEx(dcShadow, x, y-10, 0);
	LineTo(dcShadow, x, y+10);

  // Plot current hypocenter
	z = Org.dZ;
	tres = 0.0;
	x = xmin + (int)(w*(tres-tmin)/(tmax-tmin));
	y = (int)(ymin - h*z/(zmax-zmin));
	SelectObject(dcShadow, penold);
	MoveToEx(dcShadow, x-10, y-10, 0);
	LineTo(dcShadow, x+10, y+10);
	MoveToEx(dcShadow, x+10, y-10, 0);
	LineTo(dcShadow, x-10, y+10);


}

//---------------------------------------------------------------------------------------Quake
void CCanvas::Quake(char *ent) {
	PICK *pck;
	char title[80];
  ORIGIN * pOrg;

	if(!pGlint)
		return;
	if(!(pOrg = pGlint->getOrigin(ent)))
  {
    // We could not find the Origin in glint, it must be a deleted quake!
    // Do nothing
    return;
  }

  sprintf(title, "Focus: %s %5.2f/%6.2f/%3.0f - %.2f  %3d", 
          ent, pOrg->dLat, pOrg->dLon, pOrg->dZ, pOrg->dT, pOrg->nPh);
	SetWindowText(hWnd, title);

	nPck = 0;
  size_t iPickRef=0;

  // DK trying deep focus 013004
  double t1,t2;
  t1 = pOrg->dT;
  t2 = t1 + 1680.0;
	while(pck = pGlint->getPicksForTimeRange(t1,t2,&iPickRef)) 

	//while(pck = pGlint->getPicksFromOrigin(pOrg,&iPickRef)) 
  {
		
    if(nPck >= MAXPCK)
    {
      pGlint->endPickList(&iPickRef);
			break;
    }
		pPck[nPck] = pck;
		nPck++;
	}

  // Copy the Origin properties to our private Origin.
  memcpy(&Org,pOrg, sizeof(Org));

	Focus();
	Refresh();
}





//      /* only process P-Class phases and pP(for depth) */
//      if(Phases[ittt].iClass != PHASECLASS_P && Phases[ittt].iNum != PHASE_pP)
//        continue;


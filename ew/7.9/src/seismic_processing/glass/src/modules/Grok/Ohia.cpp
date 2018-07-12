// ohia.cpp: Represent ohia plot of quake and all associated phases
#include <windows.h>
#include "ohia.h"
#include "mapwin.h"
extern "C" {
#include "utility.h"
#include <phase.h>
}
// DK 012904  Added for color-coded phases
static int nPen;
static HPEN Pen[60];
static int bPensInitialized = false;
//---------------------------------------------------------------------------------------CEntity
// Constructor
COhia::COhia() {
	nRay = 0;
	iRed = 255;
	iGrn = 0;
	iBlu = 0;

  if(!bPensInitialized)
  {
    int iSize = GetNumberOfPhases();
    int i;
    // DK 012904  Added for color-coded phases
    bPensInitialized = true;
    for(i=0;i < iSize; i++)
    {
	  Pen[i] = CreatePen(PS_SOLID, 0, RGB((Phases[i].iColor & 0x0000ff),
                                        (Phases[i].iColor & 0x00ff00)>>8,
                                        (Phases[i].iColor & 0xff0000)>>16));
    }
    nPen = i;
  }

}

//---------------------------------------------------------------------------------------~CEntity
// Destructor
//		CStr *ent = new CEntity();
//		delete ent;
COhia::~COhia() {

//    // DK 012904  Added for color-coded phases
//  	for(int i=0; i<nPen; i++)
//		DeleteObject(Pen[i]);

}

//---------------------------------------------------------------------------------------Color
void COhia::Color(int red, int grn, int blu) {
	iRed = red;
	iGrn = grn;
	iBlu = blu;
}

//---------------------------------------------------------------------------------------Ray
void COhia::Ray(double lat, double lon, int ittt) {
	if(nRay >= 1000)
		return;
	dLat[nRay] = lat;
	dLon[nRay] = lon;


  // DK 012904  Added for color-coded phases
	ttt[nRay] = ittt;

	nRay++;
}

//---------------------------------------------------------------------------------------Render
// Render entity
void COhia::Render(HDC hdc, CMapWin *map) {
	HPEN hnew;
	HPEN hold;
	int x;
	int y;
	int i;

//	TRACE("******************************************************COhia::Render\n");
	if(nRay < 2)
		return;
	hnew = CreatePen(PS_SOLID, 0, RGB(0,255,255));
//	hnew = CreatePen(PS_SOLID, 0, RGB(iRed, iGrn, iBlu));
	hold = (HPEN)SelectObject(hdc, hnew);
	int x0 = map->X(dLon[0]);
	int y0 = map->Y(dLat[0]);
//	TRACE("Org     %.4f %.4f\n", dLat[0], dLon[0]); 
	for(i=1; i<nRay; i++) 
  {

    // DK 012904  Added for color-coded phases
    if(ttt[i] < nPen && ttt[i] >= 0)
    {
      SelectObject(hdc, Pen[ttt[i]]);
    }
    else
      SelectObject(hdc, hold);

		x = map->X(dLon[i]);
		y = map->Y(dLat[i]);
//		TRACE("Ray[%2d] %.4f %.4f\n", i, dLat[i], dLon[i]);
		MoveToEx(hdc, x0, y0, 0);
		LineTo(hdc, x, y);
	}
	SelectObject(hdc, hold);
	DeleteObject(hnew);
}

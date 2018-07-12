// visualpick.cpp: Represent visualpick plot of quake and all associated phases
#include <windows.h>
#include "visualpick.h"
#include "mapwin.h"
extern "C" {
#include "utility.h"
}

//---------------------------------------------------------------------------------------CEntity
// Constructor
CVisualPick::CVisualPick(double tPickTime, double dLat, double dLon) 
{
	time_t tNow;
	this->tPick = tPickTime;
	this->dLat = dLat;
	this->dLon = dLon;
	this->tPickRecord = (double)time(&tNow);
  SetPickColor((time_t)tPickTime);
}

//---------------------------------------------------------------------------------------~CEntity
// Destructor
//		CStr *ent = new CEntity();
//		delete ent;


CVisualPick::~CVisualPick() 
{
}

//---------------------------------------------------------------------------------------Color

void CVisualPick::SetPickTime(double IN_tPick)
{
	tPick = IN_tPick;
}

COLORREF CVisualPick::GetPickColor()
{
	return(colorPick);
}

COLORREF CVisualPick::SetPickColor(time_t tNow)
{
  int iTimeCoefficient = 10 * ((tNow - (int)tPick) / 20);
  if(iTimeCoefficient > 255)
      iTimeCoefficient = 255;
  int iGreen = 255 - iTimeCoefficient;
  int iRed = 0 + iTimeCoefficient/2;

	return( colorPick = RGB(iRed,iGreen,0));
}


//---------------------------------------------------------------------------------------Render
// Render entity
void CVisualPick::Draw(HDC hdc, CMapWin *map) 
{
	int x0, xMin, xMax;
	int y0, yMin,yMax;
	int iCrossSize;
  time_t tNow;

  time(&tNow);

  iCrossSize = 2 * (3 - ((tNow - (int)this->tPickRecord) / 60));
  if(iCrossSize < 0)
    iCrossSize = 0;

//	TRACE("******************************************************CVisualPick::Render\n");
	x0 = map->X(dLon);
	y0 = map->Y(dLat);
	xMin = x0 - iCrossSize / 2;
	  if(xMin < 0)
		  xMin = 12;
	xMax = x0 + iCrossSize / 2;
	  if(xMax > map->wMap)
		  xMax = map->wMap;

	yMin = y0 - iCrossSize / 2;
	  if(yMin < 0)
		  yMin = 12;
	yMax = y0 + iCrossSize / 2;
	  if(yMax > map->hMap)
		  yMax = map->hMap;

		MoveToEx(hdc, xMin, y0, NULL);
		LineTo(hdc, xMax+1, y0);
		MoveToEx(hdc, x0, yMin, NULL);
		LineTo(hdc, x0, yMax+1);
}


void CVisualPick::Render(HDC hdc, CMapWin *map) 
{
  time_t tNow;
  
	HPEN hnew = CreatePen(PS_SOLID, 0, SetPickColor(time(&tNow)));
	HPEN hold = (HPEN)SelectObject(hdc, hnew);
  Draw(hdc,map);
	SelectObject(hdc, hold);
	DeleteObject(hnew);
}

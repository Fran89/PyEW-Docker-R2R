// visualpick.cpp: Represent visualpick plot of quake and all associated phases
#include <windows.h>
#include "mapwin.h"
extern "C" {
#include "utility.h"
}
#include "visualpicklist.h"

//---------------------------------------------------------------------------------------CEntity
// Constructor
CVisualPickList::CVisualPickList() 
{
  colorCurrentPick= (COLORREF)0;
  memset(cvpArray,0,sizeof(cvpArray));
  iArrayStart=iArrayEnd=0;
}

//---------------------------------------------------------------------------------------~CEntity
// Destructor
//		CStr *ent = new CEntity();
//		delete ent;


CVisualPickList::~CVisualPickList() 
{
}

//---------------------------------------------------------------------------------------Color

//---------------------------------------------------------------------------------------Render
// Render Picks
void CVisualPickList::Render(HDC hdc, CMapWin *map)
{
	time_t tNow;

	time(&tNow);

  if(iArrayStart == iArrayEnd)
    return;

	HPEN hnew = CreatePen(PS_SOLID, 0, RGB(0,0,0));
	HPEN hold = (HPEN)SelectObject(hdc, hnew);

  // DK Added 062403 to try to eliminate All-Red picks
  colorCurrentPick= (COLORREF)0;

	for(int i=iArrayStart; i != iArrayEnd; i++)
	{
		if(i == CVP_ARRAY_SIZE)
		{
			i = -1;
			continue;
		}

		if(tNow - cvpArray[i]->tPickRecord > PICK_THRESHHOLD_TIME)
    {
      delete(cvpArray[i]);
      cvpArray[i] = NULL;
      if((iArrayStart = i+1) == CVP_ARRAY_SIZE)
        iArrayStart = 0;
    }
    else
    {
      if(colorCurrentPick != cvpArray[i]->SetPickColor(tNow))
      {
        SelectObject(hdc, hold);
        DeleteObject(hnew);
        hnew = CreatePen(PS_SOLID, 0, cvpArray[i]->GetPickColor());
        hold = (HPEN)SelectObject(hdc, hnew);
        colorCurrentPick = cvpArray[i]->GetPickColor();
      }
      cvpArray[i]->Render(hdc,map);
    }
	}
		
	SelectObject(hdc, hold);
	DeleteObject(hnew);
}  // end Render()



//---------------------------------------------------------------------------------------Render
// Add Pick
int CVisualPickList::AddPick(CVisualPick * pPick)
{

  // delete the item currently at cvpArray, since we are going to overwrite it.
  if(cvpArray[iArrayEnd])
    delete(cvpArray[iArrayEnd]);

	cvpArray[iArrayEnd] = pPick;
	if(++iArrayEnd == CVP_ARRAY_SIZE)
		iArrayEnd = 0;
	if(iArrayEnd == iArrayStart)
      if(++iArrayStart == CVP_ARRAY_SIZE)
	    iArrayStart = 0;
  return(0);
}
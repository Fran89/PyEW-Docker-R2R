// visualpick.cpp: Represent visualpick plot of quake and all associated phases
#include <windows.h>
#include "mapwin.h"
extern "C" {
#include "utility.h"
}
#include "pvisualpicklist.h"

//---------------------------------------------------------------------------------------CEntity
// Constructor
CpVisualPickList::CpVisualPickList() 
{
  pcvpList = NULL;
}

//---------------------------------------------------------------------------------------CEntity
// Constructor
CpVisualPickList::CpVisualPickList(CVisualPickList * IN_pcvpList) 
{
  pcvpList = IN_pcvpList;
}

//---------------------------------------------------------------------------------------~CEntity
// Destructor
//		CStr *ent = new CEntity();
//		delete ent;


CpVisualPickList::~CpVisualPickList() 
{
}

//---------------------------------------------------------------------------------------Render
// Render Picks
void CpVisualPickList::Render(HDC hdc, CMapWin *map)
{
  if(pcvpList)
    pcvpList->Render(hdc,map);
	
}  // end Render()




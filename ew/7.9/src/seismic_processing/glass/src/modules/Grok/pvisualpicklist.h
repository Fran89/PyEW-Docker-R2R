// pvisualpick.h
#ifndef PVISUALPICKLIST_H
#define PVISUALPICKLIST_H

//#include "entity.h"
//extern "C" {
//#include <time.h>
//}

#include "visualpicklist.h"

class CpVisualPickList : public CEntity {


public:
// Attributes
  CVisualPickList * pcvpList;

// Methods
  CpVisualPickList();
  CpVisualPickList(CVisualPickList * IN_pcvpList);
	virtual ~CpVisualPickList();
	void Render(HDC hdc, CMapWin *map);

};

#endif

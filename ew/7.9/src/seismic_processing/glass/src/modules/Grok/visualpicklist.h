// visualpick.h
#ifndef VISUALPICKLIST_H
#define VISUALPICKLIST_H

#include "entity.h"
extern "C" {
#include <time.h>
}

#include "visualpick.h"

class CVisualPickList : public CEntity {

#define CVP_ARRAY_SIZE 8000
#define PICK_THRESHHOLD_TIME 600

public:
// Attributes
  CVisualPick * cvpArray[CVP_ARRAY_SIZE];
  int iArrayStart;
  int iArrayEnd;
  COLORREF colorCurrentPick;

// Methods
  CVisualPickList();
	virtual ~CVisualPickList();
	void Render(HDC hdc, CMapWin *map);
  int AddPick(CVisualPick * pPick);

};

#endif

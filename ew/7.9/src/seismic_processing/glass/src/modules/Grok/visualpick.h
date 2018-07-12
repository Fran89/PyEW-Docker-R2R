// visualpick.h
#ifndef VISUALPICK_H
#define VISUALPICK_H

#include "entity.h"
extern "C" {
#include <time.h>
}

class CMapWin;
class CVisualPick : public CEntity {

public:
// Attributes
  COLORREF colorPick;
  double dLat;
  double dLon;
  double tPick;
  double tPickRecord;

// Methods
  CVisualPick(double tPickTime, double dLat, double dLon);
	virtual ~CVisualPick();
	void SetPickTime(double tPick);
	void Render(HDC hdc, CMapWin *map);


	COLORREF SetPickColor(time_t tNow);
	COLORREF GetPickColor();

  void Draw(HDC hdc, CMapWin *map);

};

#endif

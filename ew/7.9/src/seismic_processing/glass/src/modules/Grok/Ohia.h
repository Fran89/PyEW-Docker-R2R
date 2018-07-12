// ohia.h
#ifndef OHIA_H
#define OHIA_H

#include "entity.h"

class CMapWin;
class COhia : public CEntity {
public:
// Attributes
	int iRed;
	int iGrn;
	int iBlu;
	int nRay;
	double dLat[1000];
	double dLon[1000];
	int     ttt[1000];

// Methods
	COhia();
	virtual ~COhia();
	void Color(int red, int grn, int blu);
	void Ray(double lat, double lon, int ittt = 0);
	void Render(HDC hdc, CMapWin *map);
};

#endif

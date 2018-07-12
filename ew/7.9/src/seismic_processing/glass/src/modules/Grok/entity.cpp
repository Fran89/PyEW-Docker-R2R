// entity.cpp: Implements general array of objects
#include <windows.h>
#include "entity.h"

//---------------------------------------------------------------------------------------CEntity
// Constructor
CEntity::CEntity() {
}

//---------------------------------------------------------------------------------------~CEntity
// Destructor
//		CStr *ent = new CEntity();
//		delete ent;
CEntity::~CEntity() {
}

void CEntity::Render(HDC hdc, CMapWin *map) {
	MoveToEx(hdc, 50, 50, NULL);
	LineTo(hdc, 100, 50);
	LineTo(hdc, 100, 100);
	LineTo(hdc, 50, 100);
	LineTo(hdc, 50, 50);
}

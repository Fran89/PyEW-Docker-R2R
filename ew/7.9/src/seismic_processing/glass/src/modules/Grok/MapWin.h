#ifndef _MAPWIN_H_
#define _MAPWIN_H_
#include "win.h"
#include "array.h"

class CEntity;
class CMapWin : public CWin {
public:	
// Attributes
  // Dimensions of the display bitmap (not bitmap loaded from file)
	int				  hMap;		
	int				  wMap;

	CArray			arrEnt;		// Array of entities to plot

  // Handle to the DIBitmap loaded from file
  HBITMAP     hBitmap;

// Methods
	CMapWin();
	~CMapWin();
	void Init(HINSTANCE hinst);
	void Refresh();
	int Load(char *file);
	void Build(HDC hdc);
	void Purge();
	void Entity(CEntity *ent);
	int X(double lon);
	int Y(double lat);
	void Size(int w, int h);
	void Draw(HDC hdc);
	void LeftDown(int x, int y);
	void RightDown(int x, int y);
	void KeyDown(int key);
	void KeyUp(int key);
	void Char(int key);
};

#endif

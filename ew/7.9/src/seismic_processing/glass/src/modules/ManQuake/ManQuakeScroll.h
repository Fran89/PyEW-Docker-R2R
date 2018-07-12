#ifndef _SCROLL_H_
#define _SCROLL_H_
#include "win.h"
#include <IGlint.h>

#define MAX_PHASES 2000

struct IGlint;
class CScroll : public CWin {
public:
	
// Attributes
	HFONT	hFont;
	int		hText;
	IGlint	*pGlint;
	char	sCat[128];
	char	sAdd[128];
	char	sFer[128];
	int		nPhs;
  ORIGIN Origin;
  PICK  Phs[MAX_PHASES];


// Methods
	CScroll();
	virtual ~CScroll();
	virtual void Init(HINSTANCE hinst, char *title);
	virtual void Refresh();
	virtual void Size(int w, int h);
	virtual void Draw(HDC hdc);
	virtual void Quake(char *ent);
	virtual void ScrollBar();
	virtual void VScroll(int code, int pos);
	virtual void LeftDown(int x, int y);
  virtual bool Message(HWND hwnd, UINT mess, WPARAM wparam, LPARAM lparam);

protected:
  bool GetLocationParams(double * pdLat, double * pdLon, double * pdZ, double * pdTime);
  void AssociateLocation();
  void CreateControls();

  HWND hwndButton;
  HWND hwndEditLat;
  HWND hwndEditLon;
  HWND hwndEditDepth;
  HWND hwndEditTime;


};

#endif

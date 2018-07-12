#ifndef _SCROLL_H_
#define _SCROLL_H_
#include "win.h"
#include <IGlint.h>

#define MAXPHS 2000

struct IGlint;
class CScroll : public CWin {
public:
	
// Attributes
	HFONT	hFont;
	int		hText;
	IGlint	*pGlint;
	char	sCat[128];
	char	sDelta[64];
	char	sFEStr[256];
  int   DeltaLen;
	char	sFer[128];
	int		nPhs;
  double tOrigin;
  PICK  Phs[MAXPHS];

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
};

#endif

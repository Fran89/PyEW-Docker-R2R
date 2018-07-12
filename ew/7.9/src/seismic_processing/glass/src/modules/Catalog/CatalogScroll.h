#ifndef _SCROLL_H_
#define _SCROLL_H_
#include "win.h"

#define MAXCAT 100

typedef struct {
	int iOrigin;
	char idQuake[18];
	char sCat[128];
  bool bIsValid;
} CAT;

struct IGlint;
class CScroll : public CWin {
public:
	
// Attributes
	HFONT	hFont;
	int		hText;
	IGlint	*pGlint;
	int		nCat;
	int		iCat;	// Last entry modified
	CAT		Cat[MAXCAT];
  int   bLogOrigins;
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
	virtual void CatOut();
  int LogOrigin(int iCat, char * txt);
  int SetOriginLogging(int bLog);
};

#endif

#ifndef _SCROLL_H_
#define _SCROLL_H_
#include "win.h"

#define MAXPHS 2000

typedef struct _PHASE {
	char	sSite[6];
	char	sComp[6];
	char	sNet[5];
	char	sPhase[8];
	double	dT;
	double	dDelta;
	double	dRes;
	double	dAzm;
	double	dToa;
	double	dAff;
	char	sTag[32];		// Earthworm designator (type.mod.inst.seq)
} PHASE;

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
	PHASE	Phs[MAXPHS];
	double  tOrigin;

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

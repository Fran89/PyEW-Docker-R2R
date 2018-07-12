#ifndef _CANVAS_H_
#define _CANVAS_H_
#include "win.h"
#include <ITravelTime.h>

#define MAXPHS 2000

typedef struct _PHASE {
	double	dDelta;
	double	dRes;
  PhaseType  ptPhase;
} PHASE;

struct IGlint;
class CCanvas : public CWin {
public:
	
// Attributes
	HFONT	hFont;
	int		hText;
	IGlint	*pGlint;
	double	dDelMin;
	double	dDelMod;
	double	dDelMax;
	int		nPhs;
	PHASE	Phs[MAXPHS];

// Methods
	CCanvas();
	virtual ~CCanvas();
	virtual void Init(HINSTANCE hinst, char *title);
	virtual void Refresh();
	virtual void Size(int w, int h);
	virtual void Draw(HDC hdc);
	virtual void Quake(char *ent);

private:
  HPEN BlackPen;
  HPEN GrayPen;
  int nPen;
  HPEN Pen[60];

};

#endif

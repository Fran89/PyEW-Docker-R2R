#ifndef _CANVAS_H_
#define _CANVAS_H_
#include <win.h>
#include <IGlint.h>

#define MAXPCK 2000

struct IGlint;
class CCanvas : public CWin {
public:
	
// Attributes
	HFONT	hFont;
	HDC		dcShadow;
	HBITMAP	bmShadow;
	int		hText;
	IGlint	*pGlint;
	int		nPck;
	ORIGIN	Org;
	PICK	*pPck[MAXPCK];
  char * TTTUsedArray;

// Methods
	CCanvas();
	virtual ~CCanvas();
	virtual void Init(HINSTANCE hinst, char *title);
	virtual void Refresh();
	virtual void Size(int w, int h);
	virtual void Draw(HDC hdc);
	virtual void Focus();
	virtual void Quake(char *ent);

private:
  HPEN BlackPen;
  HPEN GrayPen;
  int nPen;
  HPEN Pen[60];

};

#endif

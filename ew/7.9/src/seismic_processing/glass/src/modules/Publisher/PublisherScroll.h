#ifndef _SCROLL_H_
#define _SCROLL_H_
#include "win.h"
#include "publish.h"

#define MAXPUB 100
#define PUBSTATUSSTRINGSIZE 100

struct IGlint;
class CScroll : public CWin {
public:
	
// Attributes
	HFONT	 hFont;
	int		 hText;
	int		 nPub;
	int		 iSelectedPub;	// Last entry modified
  time_t tListBuilt;
  char   PubStatusList[MAXPUB][PUBSTATUSSTRINGSIZE];
	IGlint	*pGlint;


// Methods
	CScroll();
	virtual ~CScroll();
	virtual void Init(HINSTANCE hinst, char *title);
	virtual void Refresh();
	virtual void Size(int w, int h);
	virtual void Draw(HDC hdc);
	virtual void ScrollBar();
	virtual void VScroll(int code, int pos);
	virtual void LeftDown(int x, int y);
  void         UpdateList();
};

#endif

#ifndef _WIN_H_
#define _WIN_H_

//LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#define MOUSE_CONTROL	1
#define MOUSE_LEFT		2
#define MOUSE_MIDDLE	4
#define MOUSE_RIGHT		8
#define MOUSE_SHIFT		16

class CWin {
public:
	
// Attributes
	HWND	hWnd;
	int		hTxt;		// Height of a text line
	int		nScr;		// Number of lines in scrollable portion of window
	int		iStyle;		// Window style (possible set in override of Init
	int		iX;			// Origin and size of window
	int		iY;
	int		nX;
	int		nY;
	char	sMap[128];	// Map title

// Methods
	CWin();
	virtual ~CWin();
	virtual void Init(HINSTANCE hinst, char *name);
	virtual LRESULT CALLBACK Proc(HWND hwnd, UINT mess, WPARAM wparam, LPARAM lparam);
	virtual bool Message(HWND hwnd, UINT mess, WPARAM wparam, LPARAM lparam);
	virtual void Draw(HDC hdc);
	virtual void Size(int w, int h);
	virtual void LeftDown(int x, int y);
	virtual void LeftUp(int x, int y);
	virtual void RightDown(int x, int y);
	virtual void RightUp(int x, int y);
	virtual void MouseMove(int flag, int x, int y);
	virtual void KeyDown(int key);
	virtual void KeyUp(int key);
	virtual void Char(int key);
	virtual void VScroll(int code, int pos);
	virtual void HScroll(int code, int pos);
	virtual bool MinMax(POINT *ptmin, POINT *ptmax);
};

#endif

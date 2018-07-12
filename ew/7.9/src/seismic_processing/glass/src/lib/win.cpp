#include <windows.h>
#include "win.h"
extern "C" {
#include "utility.h"
}

int nWin = 0;
CWin *pWin[10];

//---------------------------------------------------------------------------------------WndProc
static LRESULT CALLBACK WndProc(HWND hwnd, UINT mess, WPARAM wparam, LPARAM lparam) {
	CWin *win;
	int iwin;
	for(iwin=0; iwin<nWin; iwin++) {
		win = pWin[iwin];
		if(win->hWnd == hwnd) {
			return win->Proc(hwnd, mess, wparam, lparam);
		}
	}
	return DefWindowProc(hwnd, mess, wparam, lparam);
}

//---------------------------------------------------------------------------------------Win
CWin::CWin() {
	hWnd = 0;
	iStyle = WS_OVERLAPPEDWINDOW;
	iX = CW_USEDEFAULT;
	iY = CW_USEDEFAULT;
	nX = CW_USEDEFAULT;
	nY = CW_USEDEFAULT;
}

//---------------------------------------------------------------------------------------~Win
CWin::~CWin() {
}

//---------------------------------------------------------------------------------------Init
void CWin::Init(HINSTANCE hinst, char *name) {

//	static TCHAR szAppName[] = TEXT("HelloWin");
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinst;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
//	wc.lpszClassName = szAppName;
	wc.lpszClassName = name;
	strcpy(sMap, name);

	if(!RegisterClass(&wc)) {
		MessageBox(NULL, TEXT("Register failed"), name, MB_ICONERROR);
	}

	hWnd = CreateWindow(name,
		name,
		iStyle,
		iX,
		iY,
		nX,
		nY,
		NULL,
		NULL,
		hinst,
		NULL);
	pWin[nWin++] = this;
	ShowWindow(hWnd, TRUE);
	UpdateWindow(hWnd);
}

//---------------------------------------------------------------------------------------Proc
LRESULT CALLBACK CWin::Proc(HWND hwnd, UINT mess, WPARAM wparam, LPARAM lparam) {
	HDC hdc;
	PAINTSTRUCT ps;
	LPMINMAXINFO pmmi;
	int flag;
	int key;
	int x;
	int y;

	switch(mess) {
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		Draw(hdc);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		Size(LOWORD(lparam), HIWORD(lparam));
		return 0;
	case WM_KEYDOWN:
		key = wparam;
		if(lparam & 0x40000000)
			break;
		if(!(GetKeyState(VK_CAPITAL)&1) && !(GetKeyState(VK_SHIFT)&80000000))
			key += 'a' - 'A';
//		TRACE("KeyDown %d <%c> state:%x lparam:%x\n", wparam, key, GetKeyState(VK_SHIFT), lparam);
		KeyDown(key);
		break;
	case WM_KEYUP:
		break;
	case WM_CHAR:
		key = wparam;
		Char(key);
		break;
	case WM_LBUTTONDOWN:
		x = LOWORD(lparam);
		y = HIWORD(lparam);
		LeftDown(x, y);
		break;
	case WM_LBUTTONUP:
		x = LOWORD(lparam);
		y = HIWORD(lparam);
		LeftUp(x, y);
		break;
	case WM_RBUTTONDOWN:
		x = LOWORD(lparam);
		y = HIWORD(lparam);
		RightDown(x, y);
		break;
	case WM_RBUTTONUP:
		x = LOWORD(lparam);
		y = HIWORD(lparam);
		RightUp(x, y);
		break;
	case WM_MOUSEMOVE:
		x = LOWORD(lparam);
		y = HIWORD(lparam);
		flag = 0;
		if(wparam & MK_CONTROL)	flag |= MOUSE_CONTROL;
		if(wparam & MK_LBUTTON)	flag |= MOUSE_LEFT;
		if(wparam & MK_MBUTTON)	flag |= MOUSE_MIDDLE;
		if(wparam & MK_RBUTTON)	flag |= MOUSE_RIGHT;
		if(wparam & MK_SHIFT)	flag |= MOUSE_SHIFT;
		MouseMove(flag, x, y);
		break;
	case WM_VSCROLL:
		VScroll(LOWORD(wparam), HIWORD(wparam));
		break;
	case WM_HSCROLL:
		HScroll(LOWORD(wparam), HIWORD(wparam));
		break;
	case WM_GETMINMAXINFO:
		pmmi = (LPMINMAXINFO)lparam;
		if(MinMax(&pmmi->ptMinTrackSize, &pmmi->ptMaxTrackSize))
			return 0;
		break;
	default:
		Message(hwnd, mess, wparam, lparam);
		break;
	}
	return DefWindowProc(hwnd, mess, wparam, lparam);
}

//---------------------------------------------------------------------------------------Message
bool CWin::Message(HWND hwnd, UINT mess, WPARAM wparam, LPARAM lparam) {
	return false;
}

//---------------------------------------------------------------------------------------Draw
void CWin::Draw(HDC hdc) {
	RECT r;
	HPEN pen;
	GetClientRect(hWnd, &r);
	MoveToEx(hdc, r.left, r.top, NULL);
	LineTo(hdc, r.right, r.bottom);
	MoveToEx(hdc, r.right, r.top, NULL);
	LineTo(hdc, r.left, r.bottom);
	pen = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
	SelectObject(hdc, pen);
	MoveToEx(hdc, r.left, r.top, NULL);
	LineTo(hdc, r.right-1, r.top);
	LineTo(hdc, r.right-1, r.bottom-1);
	LineTo(hdc, r.left, r.bottom-1);
	LineTo(hdc, r.left, r.top);
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	DeleteObject(pen);
}

//---------------------------------------------------------------------------------------Size
void CWin::Size(int w, int h) {
}

//---------------------------------------------------------------------------------------LeftDown
void CWin::LeftDown(int x, int y) {
}

//---------------------------------------------------------------------------------------LeftUp
void CWin::LeftUp(int x, int y) {
}

//---------------------------------------------------------------------------------------RightDown
void CWin::RightDown(int x, int y) {
}

//---------------------------------------------------------------------------------------RightUp
void CWin::RightUp(int x, int y) {
}

//---------------------------------------------------------------------------------------MouseMove
void CWin::MouseMove(int flag, int x, int y) {
}

//---------------------------------------------------------------------------------------KeyDown
void CWin::KeyDown(int key) {
}

//---------------------------------------------------------------------------------------KeyUp
void CWin::KeyUp(int key) {
}

//---------------------------------------------------------------------------------------Char
void CWin::Char(int key) {
}

//---------------------------------------------------------------------------------------VScroll
void CWin::VScroll(int code, int pos) {
}

//---------------------------------------------------------------------------------------HScroll
void CWin::HScroll(int code, int key) {
}

//---------------------------------------------------------------------------------------MinMax
// Set minimum and maximum dimensions of window client region during resizing
bool CWin::MinMax(POINT *ptmin, POINT *ptmax) {
	return false;
}


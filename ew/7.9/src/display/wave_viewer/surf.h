// surf.h : main header file for the wave surfer application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#define ON_UPDATE_WINDOW_CREATION                0     
#define ON_UPDATE_DISPLAY_INITIALIZATION      1001 
#define ON_UPDATE_MOVE_DISPLAY_TO_TANK_START  1002 
#define ON_UPDATE_MOVE_DISPLAY_TO_TANK_END    1003 
#define ON_UPDATE_MOVE_DISPLAY_TO_TIME        1004 
#define ON_UPDATE_REDRAW_TRACE                1006 
#define ON_UPDATE_SB_STATUS                   2000 
#define ON_UPDATE_SB_RECEIVE                  2001 
#define ON_UPDATE_SB_MOUSE_TIME               2002 
#define ON_UPDATE_SB_GROUP                    2003 
#define ON_UPDATE_SB_EVENT                    2004 
#define ON_UPDATE_GOTO_QUAKE                  3000 

      /******** BEGIN FUNCTIONAL CHANGE DK 20000525**********/

// FILL_NUMBER is the value initially put into the wave_viewer buffer.
// before there is data retrieved for that buffer time.
// If data cannot be retrieved for that time, due to a GAP in the 
// wave_server tanks, then the FILL_NUMBER should be used as a 
// replacement.
#define UNTOUCHED_FILL_NUMBER   1234567888
// FILL_NUMBER is the value put into the wave_viewer buffer in place
// of FILL_VALUE
#define FILL_NUMBER             1234567890

      /********   END FUNCTIONAL CHANGE DK 20000525**********/

/////////////////////////////////////////////////////////////////////////////
// CSurfApp:
// See surf.cpp for the implementation of this class
//

class CSurfDoc;
class CSurfView;
class CSurfApp : public CWinApp
{
public:
	CSurfApp();

// Attributes
	CSurfDoc	*pDoc;
	CSurfView	*pView;
	CObArray arrGroup;	// Group array

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSurfApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CSurfApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

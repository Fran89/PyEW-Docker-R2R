// StartstopConsole.h : main header file for the STARTSTOPCONSOLE application
//

#if !defined(AFX_STARTSTOPCONSOLE_H__BE5C723B_A912_4C3E_A2BF_83CC2E9D8F3E__INCLUDED_)
#define AFX_STARTSTOPCONSOLE_H__BE5C723B_A912_4C3E_A2BF_83CC2E9D8F3E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CStartstopConsoleApp:
// See StartstopConsole.cpp for the implementation of this class
//

class CStartstopConsoleApp : public CWinApp
{
public:
	CStartstopConsoleApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStartstopConsoleApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CStartstopConsoleApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARTSTOPCONSOLE_H__BE5C723B_A912_4C3E_A2BF_83CC2E9D8F3E__INCLUDED_)

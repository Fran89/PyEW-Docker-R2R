// StartstopConsole.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "StartstopConsole.h"
#include "StartstopConsoleDlg.h"
extern "C"
{
 #include "watchdog_client.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStartstopConsoleApp

BEGIN_MESSAGE_MAP(CStartstopConsoleApp, CWinApp)
	//{{AFX_MSG_MAP(CStartstopConsoleApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartstopConsoleApp construction

CStartstopConsoleApp::CStartstopConsoleApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CStartstopConsoleApp object

CStartstopConsoleApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CStartstopConsoleApp initialization

BOOL CStartstopConsoleApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	reportErrorInit(1024, 1, AfxGetAppName());

	CStartstopConsoleDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	reportErrorCleanup();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

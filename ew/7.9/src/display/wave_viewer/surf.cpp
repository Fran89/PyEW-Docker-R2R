// surf.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "surf.h"
#include "comfile.h"
#include "group.h"

#include "MainFrm.h"
#include "surfDoc.h"
#include "surfView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SURF_CFN_LENGTH 80
#define SURF_DEFAULT_CONFIG_FILENAME "wave_viewer.d"
/////////////////////////////////////////////////////////////////////////////
// CSurfApp

BEGIN_MESSAGE_MAP(CSurfApp, CWinApp)
	//{{AFX_MSG_MAP(CSurfApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSurfApp construction

CSurfApp::CSurfApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSurfApp object

CSurfApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSurfApp initialization

BOOL CSurfApp::InitInstance()
{
	char txt[120];

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CSurfDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CSurfView));
	AddDocTemplate(pDocTemplate);


  // Process general configuration commands
	CComfile cf;
	int nc;
	CString com;
	CString cname;
	CString cmodule;
	CString ctask;
	CString clink;
	CString cpar;
	CGroup *grp;
	char   szConfigFileName[SURF_CFN_LENGTH];
	char   szMessageBoxError[SURF_CFN_LENGTH + 40];
	CCommandLineInfo cmdInfo;


  /* DavidK 19990719  This was a hack.  In order to keep program 
  changes to a minimum, we(I) bypassed the MFC/AFX way of handling
  command line parameters, and regressed command line handling back
  to the dark ages of C based argc and argv.  The code wants to
  open the config file on its own, and that just becomes too much
  of a pain, because ProcessShellCommand, takes a lot of action based
  on cmdInfo which is setup during multiple calls from ParseCommandLine()
  to ParseCommand.  So wuallah!  Please forgive me for what I have done.
  */
	
	// Parse command line for standard shell commands, DDE, file open
	//ParseCommandLine(cmdInfo);
  // We just care about a possible config file name being passed
  // as argv[1]  DK 1999/07/19


  // Dispatch commands specified on the command line
  if (!ProcessShellCommand(cmdInfo))
  	return FALSE;

	if(__argc > 1)
	{
    strncpy(szConfigFileName,__targv[1],SURF_CFN_LENGTH-1);
	}
  else
  {
    strncpy(szConfigFileName,SURF_DEFAULT_CONFIG_FILENAME,SURF_CFN_LENGTH-1);
  }

	// Null terminate the string
	szConfigFileName[SURF_CFN_LENGTH-1] = 0;


  // Add the special <ALL> group, that includes everything the
  // Wave Server's got.
  CGroup * pAllGroup = new CGroup((char *)"<ALL>");
  arrGroup.Add(pAllGroup);

	TRACE("Processing command file\n");
	if(!cf.Open(szConfigFileName)) 
	{
		sprintf(szMessageBoxError,"File %s not found!",szConfigFileName);
		AfxMessageBox(szMessageBoxError);
		AfxAbort();
	}
	while((nc = cf.Read()) >= 0) {
		if(nc < 2)
			continue;
		com = cf.Token();
		if(cf.StartsWith("#"))
			continue;
		if(cf.Is("group")) {
			grp = new CGroup(&cf);
			arrGroup.Add(grp);
			continue;
		}
		if(com == "")
			continue;
		if(pView->Com(&cf))
			continue;
		if(pDoc->Com(&cf))
			continue;

		sprintf(txt, "Unknown command <%s>", com);
		AfxMessageBox(txt);
	}
	cf.Close();
	CMainFrame *frm = (CMainFrame *)m_pMainWnd;
//	frm->ShowWindow(SW_SHOWMAXIMIZED);
	frm->Reset();	// Force menu update
	pDoc->UpdateAllViews(NULL, 1001, NULL);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CSurfApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CSurfApp commands

int CSurfApp::ExitInstance() 
{
	int i;

	for(i=0; i<arrGroup.GetSize(); i++)
		delete arrGroup.GetAt(i);
	arrGroup.RemoveAll();
	
	return CWinApp::ExitInstance();
}

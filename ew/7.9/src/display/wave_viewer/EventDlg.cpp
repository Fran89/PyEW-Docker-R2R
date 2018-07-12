// EventDlg.cpp : implementation file
//

  /* DavidK 19990713  Changed the the month comparisons to be
                      case insensitive.  From strcmp() to stricmp().
  *******************************************************************/

#include "stdafx.h"
#include "surf.h"
#include "mainfrm.h"
#include "EventDlg.h"
#include "date.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CDate c1970(1970, 1, 1, 0, 0, 0.0);	// MSDOS time base
static int nSetYear = 0;
static int nSetMonth = 0;
static int nSetDay = 0;
static int nSetHRMN = 0;
static int nSetSeconds = 0;
static int nSetTime = 0;

/////////////////////////////////////////////////////////////////////////////
// CEventDlg dialog


CEventDlg::CEventDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEventDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEventDlg)
	cTime = _T("");
	//}}AFX_DATA_INIT
}


void CEventDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEventDlg)
	DDX_Text(pDX, IDC_EDIT1, cTime);
	//}}AFX_DATA_MAP
}

// Initialize human time from offset since 1970
void CEventDlg::UpdateHuman() {
	char time[40];
	char txt[40];

	TRACE("CEventDlg::UpdateHuman called...\n");
	GetDlgItemText(IDC_EDIT1, time, sizeof(time)-1);
	double t = atof(time) + c1970.Time();
	CDate date(t);
	sprintf(txt, "%d", date.Year());
	nSetYear++;
	SetDlgItemText(IDC_EVENT_YEAR, txt);
	switch(date.Month()) {
	case  1: strcpy(txt, "Jan"); break;
	case  2: strcpy(txt, "Feb"); break;
	case  3: strcpy(txt, "Mar"); break;
	case  4: strcpy(txt, "Apr"); break;
	case  5: strcpy(txt, "May"); break;
	case  6: strcpy(txt, "Jun"); break;
	case  7: strcpy(txt, "Jul"); break;
	case  8: strcpy(txt, "Aug"); break;
	case  9: strcpy(txt, "Sep"); break;
	case 10: strcpy(txt, "Oct"); break;
	case 11: strcpy(txt, "Nov"); break;
	case 12: strcpy(txt, "Dec"); break;
	}
	nSetMonth++;
	SetDlgItemText(IDC_EVENT_MONTH, txt);
	sprintf(txt, "%2d", date.Day());
	nSetDay++;
	SetDlgItemText(IDC_EVENT_DAY, txt);
	sprintf(txt, "%02d%02d", date.Hour(), date.Minute());
	nSetHRMN++;
	SetDlgItemText(IDC_EVENT_HRMN, txt);
	nSetSeconds++;
	sprintf(txt, "%5.2f", date.Seconds());
	SetDlgItemText(IDC_EVENT_TIME, txt);
}

// Update offset since 1970 from human time
void CEventDlg::UpdateTime() {
	char year[20];
	GetDlgItemText(IDC_EVENT_YEAR, year, sizeof(year)-1);
	int yr = (int)atoi(year);
	if(yr < 1970 || yr > 3000)
		return;

	char month[20];
	GetDlgItemText(IDC_EVENT_MONTH, month, sizeof(month)-1);
	int mo = 0;

  /* DavidK 19990713  Changed the the month comparisons to be
                      case insensitive.  From strcmp() to stricmp().
  *******************************************************************/
	if(stricmp(month, "Jan") == 0) mo = 1;
	if(stricmp(month, "Feb") == 0) mo = 2;
	if(stricmp(month, "Mar") == 0) mo = 3;
	if(stricmp(month, "Apr") == 0) mo = 4;
	if(stricmp(month, "May") == 0) mo = 5;
	if(stricmp(month, "Jun") == 0) mo = 6;
	if(stricmp(month, "Jul") == 0) mo = 7;
	if(stricmp(month, "Jly") == 0) mo = 7;
	if(stricmp(month, "Aug") == 0) mo = 8;
	if(stricmp(month, "Sep") == 0) mo = 9;
	if(stricmp(month, "Spt") == 0) mo = 9;
	if(stricmp(month, "Oct") == 0) mo = 10;
	if(stricmp(month, "Nov") == 0) mo = 11;
	if(stricmp(month, "Dec") == 0) mo = 12;
	if(mo == 0)
		return;

	char day[20];
	GetDlgItemText(IDC_EVENT_DAY, day, sizeof(day)-1);
	int da = (int)atoi(day);
	if(da < 1 || da > 31)
		return;

	char hrmn[20];
	GetDlgItemText(IDC_EVENT_HRMN, hrmn, sizeof(hrmn)-1);
	int i = (int)atoi(hrmn);
	int hr = i / 100;
	int mn = i - 100 * hr;
	if(hr < 0 || hr > 23)
		return;
	if(mn < 0 || mn > 59)
		return;

	char seconds[20];
	GetDlgItemText(IDC_EVENT_TIME, seconds, sizeof(seconds)-1);
	double sex = atof(seconds);
	if(sex < 0.0 || sex > 59.9999)
		return;

	CDate date(yr, mo, da, hr, mn, sex);

	char time[40];
	sprintf(time, "%.2f", date.Time() - c1970.Time());
	nSetTime++;
	SetDlgItemText(IDC_EDIT1, time);
}


BEGIN_MESSAGE_MAP(CEventDlg, CDialog)
	//{{AFX_MSG_MAP(CEventDlg)
	ON_WM_KEYUP()
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_EN_CHANGE(IDC_EVENT_YEAR, OnChangeEventYear)
	ON_EN_CHANGE(IDC_EVENT_MONTH, OnChangeEventMonth)
	ON_EN_CHANGE(IDC_EVENT_DAY, OnChangeEventDay)
	ON_EN_CHANGE(IDC_EVENT_HRMN, OnChangeEventHrmn)
	ON_EN_CHANGE(IDC_EVENT_TIME, OnChangeEventTime)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventDlg message handlers

void CEventDlg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	TRACE("*********CEventDlg::OnKeyUp called\n");
	
	CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CEventDlg::OnOK() 
{
	TRACE("********* CEventDlg::OnOK called\n");
	CMainFrame *frm = (CMainFrame *)GetParent();
	char time[20];
	GetDlgItemText(IDC_EDIT1, time, sizeof(time)-1);
	double t = atof(time);
	frm->Shift(t);
	
	CDialog::OnOK();
}

BOOL CEventDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UpdateHuman();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEventDlg::PostNcDestroy() 
{
	delete this;
	
	CDialog::PostNcDestroy();
}

void CEventDlg::OnChangeEdit1() 
{
	if(nSetTime > 0)
		nSetTime--;
	else
		UpdateHuman();
}

void CEventDlg::OnChangeEventYear() 
{
	if(nSetYear > 0)
		nSetYear--;
	else
		UpdateTime();
}

void CEventDlg::OnChangeEventMonth() 
{
	if(nSetMonth > 0)
		nSetMonth--;
	else
		UpdateTime();
}

void CEventDlg::OnChangeEventDay() 
{
	if(nSetDay > 0)
		nSetDay--;
	else
		UpdateTime();
}

void CEventDlg::OnChangeEventHrmn() 
{
	if(nSetHRMN > 0)
		nSetHRMN--;
	else
		UpdateTime();
}

void CEventDlg::OnChangeEventTime() 
{
	if(nSetSeconds > 0)
		nSetSeconds--;
	else
		UpdateTime();
}

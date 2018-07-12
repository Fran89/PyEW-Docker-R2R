// EventDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventDlg dialog

class CEventDlg : public CDialog
{
// Construction
public:
	CEventDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEventDlg)
	enum { IDD = IDD_DIALOG1 };
	CString	cTime;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateHuman();
	void UpdateTime();

	// Generated message map functions
	//{{AFX_MSG(CEventDlg)
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnOK();
	afx_msg void OnChangeEdit1();
	afx_msg void OnChangeEventYear();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEventMonth();
	afx_msg void OnChangeEventDay();
	afx_msg void OnChangeEventHrmn();
	afx_msg void OnChangeEventTime();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

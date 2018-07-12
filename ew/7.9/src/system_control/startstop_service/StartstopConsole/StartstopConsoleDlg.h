// StartstopConsoleDlg.h : header file
//

#if !defined(AFX_STARTSTOPCONSOLEDLG_H__39F28FA7_AF4C_42A8_BE8C_468BFAEDAF85__INCLUDED_)
#define AFX_STARTSTOPCONSOLEDLG_H__39F28FA7_AF4C_42A8_BE8C_468BFAEDAF85__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CStartstopConsoleDlg dialog

class CStartstopConsoleDlg : public CDialog
{
// Construction
public:
	CStartstopConsoleDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CStartstopConsoleDlg)
	enum { IDD = IDD_STARTSTOPCONSOLE_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStartstopConsoleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	HANDLE m_hNamedPipe;

	// Generated message map functions
	//{{AFX_MSG(CStartstopConsoleDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnConsole();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARTSTOPCONSOLEDLG_H__39F28FA7_AF4C_42A8_BE8C_468BFAEDAF85__INCLUDED_)

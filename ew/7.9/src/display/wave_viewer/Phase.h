#ifndef _PHASE_H_
#define _PHASE_H_
// Phase.h : header file
//

#define PHASE_TRIGGER 1

/////////////////////////////////////////////////////////////////////////////
// CPhase command target

class CPhase : public CCmdTarget
{
	DECLARE_DYNCREATE(CPhase)

public:
	CPhase();           // protected constructor used by dynamic creation
	virtual ~CPhase();

// Attributes
public:
	int			nType;	// Phase type
	double		dTime;	// Phase time
	CString		cSite;	// Station name
	CString		cNet;	// Network name
	CString		cComp;	// Component
	CString		cLoc;	// Location name

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPhase)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPhase)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif

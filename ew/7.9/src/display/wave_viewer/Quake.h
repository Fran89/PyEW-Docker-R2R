#ifndef _QUAKE_H_
#define _QUAKE_H_
// Quake.h : header file
//

#define QUAKE_TRIGGER	1

/////////////////////////////////////////////////////////////////////////////
// CQuake command target

class CQuake : public CCmdTarget
{
	DECLARE_DYNCREATE(CQuake)

	CQuake();           // protected constructor used by dynamic creation

// Attributes
public:
	int	nType;					// Event type
	double dTime;				// Associated time (trigger, hypo, etc)
	CObArray arrPhase;			// Phase array

// Operations
public:
	int Add(CObject *q);		// Append phase to event
	CObject *GetAt(int index);	// Retrieve ith phase
	int GetSize();				// Get size of phase list
	void SetAt(int index, CObject *q);	// Modify phase

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQuake)
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CQuake();

	// Generated message map functions
	//{{AFX_MSG(CQuake)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif
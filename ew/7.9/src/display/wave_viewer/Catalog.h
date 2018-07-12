#ifndef _CATALOG_H_
#define _CATALOG_H_
// Catalog.h : header file
//
#include "quake.h"
#include "phase.h"


/////////////////////////////////////////////////////////////////////////////
// CCatalog command target

class CCatalog : public CCmdTarget
{
	DECLARE_DYNCREATE(CCatalog)

public:
	CCatalog();           // protected constructor used by dynamic creation
	virtual ~CCatalog();

// Attributes
public:
	CObArray arrCat;	// Ye olde trusty catalog

// Operations
public:
	int Add(CObject *q);		// Append event to catalog
	CObject *GetAt(int index);	// Retrieve ith event
	int GetSize();				// Get size of catalog
	void SetAt(int index, CObject *q);	// Modify event

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCatalog)
	//}}AFX_VIRTUAL

// Implementation

	// Generated message map functions
	//{{AFX_MSG(CCatalog)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif

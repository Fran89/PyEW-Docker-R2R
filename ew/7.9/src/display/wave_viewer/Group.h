// Group.h : header file
//

// STRING__NO_LOC for blank location code.  
//  also defined in CWaveDoc
#define STRING__NO_LOC "--"

/////////////////////////////////////////////////////////////////////////////
// CGroup command target
class CComfile;
class CGroup : public CCmdTarget
{
	DECLARE_DYNCREATE(CGroup)

	CGroup();           // protected constructor used by dynamic creation
  CGroup(CString sGroupName);
// Attributes
public:
	CString cName;		// Name of the group
	CObArray arrSite;	// Site array for this group

// Operations
public:
	CGroup(CComfile *cf);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroup)
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CGroup();

	// Generated message map functions
	//{{AFX_MSG(CGroup)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

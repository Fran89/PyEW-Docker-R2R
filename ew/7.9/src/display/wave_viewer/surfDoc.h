// surfDoc.h : interface of the CSurfDoc class
//
/////////////////////////////////////////////////////////////////////////////
#include "wavedoc.h"
#include "comfile.h"
#include "catalog.h"

class CComfile;
class CSurfDoc : public CWaveDoc
{
protected: // create from serialization only
	CSurfDoc();
	DECLARE_DYNCREATE(CSurfDoc)


// Attributes
public:
	int			iQuake;	// Selected quake (used by child windows)
	int			iPhase;	// Selected phase (used by child windows)
	CCatalog	cCat;	// Catalog of events
  BOOL bApplyGroupFilter;
  CObArray * parrGroupSite;


// Operations
public:
	BOOL Com(CComfile *cf);
	double TimeCrack(CComfile *cf);
  void ApplyGroupFilter(CObArray * pSiteArray, BOOL bApplyFilter);
  void HandleWSMenuReply(CString sReply);
  void ResetDataRequestQueue(void);
  BOOL HACK_CheckForEmptyQueue(void);
  void RefreshMenu(int iTankLocation);
//	void Receive(int n, char *buf);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSurfDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSurfDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
  CSingleLock * psSurfLock;

// Generated message map functions
protected:
	//{{AFX_MSG(CSurfDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

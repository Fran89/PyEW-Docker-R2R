// Site.h : header file
//
#ifndef SITE_H
# define SITE_H


#include <afxmt.h>
/////////////////////////////////////////////////////////////////////////////
// CSite command target

class CSite : public CObject
{
	DECLARE_DYNCREATE(CSite)

	CSite();           // protected constructor used by dynamic creation

// Attributes
public:
	int		iPin;		// Pin number (or -1)
	CString	cSta;		// Station name
	CString cChn;		// Channel id
	CString cNet;		// Network id
	CString cLoc;		// Location id
	double	dStart;		// Starting time (optional)
	double	dStop;		// Stopping time (optional)
  CSite& operator=( const CSite& csIn );
  bool operator==( const CSite& csIn ) const;


// Operations
public:
	CSite(CString sta, CString chn, CString net, CString loc);
	CSite(const CSite &SiteIn);
	void ResetParams(CString sta, CString chn, CString net,  CString loc, 
                   int iPin, double tStart, double tEnd);
	virtual ~CSite();


};

/////////////////////////////////////////////////////////////////////////////
#endif /* ifndef SITE_H */

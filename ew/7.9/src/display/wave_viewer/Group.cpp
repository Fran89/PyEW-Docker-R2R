// Group.cpp : implementation file
//

#include "stdafx.h"
#include "surf.h"
#include "Group.h"
#include "site.h"
#include "comfile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroup

IMPLEMENT_DYNCREATE(CGroup, CCmdTarget)

CGroup::CGroup()
{
}

CGroup::~CGroup()
{
	int i;

	for(i=0; i<arrSite.GetSize(); i++)
		delete arrSite.GetAt(i);
	arrSite.RemoveAll();
}

CGroup::CGroup(CString sGroupName)
{
  cName = sGroupName;
}


CGroup::CGroup(CComfile *cf) {
	CString sta;
	CString chn;
	CString net;
	CString loc;
	CSite *site;
	int nc;

	cName = cf->Token();
	TRACE("Defining group %s\n", (const char *)cName);
	while((nc = cf->Read()) >= 0) {
		if(nc < 2)
			break;
		sta = cf->Token();
		if(cf->Is("end"))
			break;
		chn = cf->Token();
		net = cf->Token();

    /* add support for possibly null loc's */
		loc = cf->Token();
    if(loc.GetLength() <= 0 || loc[0] == 0x00)
      loc = STRING__NO_LOC;

		site = new CSite(sta, chn, net, loc);
		arrSite.Add(site);
	}
}

BEGIN_MESSAGE_MAP(CGroup, CCmdTarget)
	//{{AFX_MSG_MAP(CGroup)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroup message handlers

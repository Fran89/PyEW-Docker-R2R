// Catalog.cpp : implementation file
//

#include "stdafx.h"
#include "Catalog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCatalog
//
// For now simple encapsulates array of quake objects, but it should be
// easy to generalize

IMPLEMENT_DYNCREATE(CCatalog, CCmdTarget)

CCatalog::CCatalog()
{
}

CCatalog::~CCatalog()
{
	int i;

	for(i=0; i<arrCat.GetSize(); i++)
		delete arrCat.GetAt(i);
	arrCat.RemoveAll();
}

int CCatalog::Add(CObject *q)
{
	return arrCat.Add(q);
}

CObject *CCatalog::GetAt(int index)
{
	return arrCat.GetAt(index);
}

int CCatalog::GetSize()
{
	return arrCat.GetSize();
}

void CCatalog::SetAt(int index, CObject *q)
{
	arrCat.SetAt(index, q);
}

BEGIN_MESSAGE_MAP(CCatalog, CCmdTarget)
	//{{AFX_MSG_MAP(CCatalog)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCatalog message handlers

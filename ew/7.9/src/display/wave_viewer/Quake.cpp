// Quake.cpp : implementation file
//

#include "stdafx.h"
#include "Quake.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQuake

IMPLEMENT_DYNCREATE(CQuake, CCmdTarget)

CQuake::CQuake()
{
}

CQuake::~CQuake()
{
	int i;

	for(i=0; i<arrPhase.GetSize(); i++)
		delete arrPhase.GetAt(i);
	arrPhase.RemoveAll();
}

int CQuake::Add(CObject *q)
{
	return arrPhase.Add(q);
}

CObject *CQuake::GetAt(int index)
{
	return arrPhase.GetAt(index);
}

int CQuake::GetSize()
{
	return arrPhase.GetSize();
}

void CQuake::SetAt(int index, CObject *q)
{
	arrPhase.SetAt(index, q);
}


BEGIN_MESSAGE_MAP(CQuake, CCmdTarget)
	//{{AFX_MSG_MAP(CQuake)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQuake message handlers

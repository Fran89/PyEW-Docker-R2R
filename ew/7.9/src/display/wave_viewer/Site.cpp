// Site.cpp : implementation file
//

#include "stdafx.h"
#include "surf.h"
#include "Site.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSite

IMPLEMENT_DYNCREATE(CSite, CCmdTarget)

CSite::CSite()
{
}

CSite::CSite(CString sta, CString chn, CString net, CString loc) {
	iPin = -1;
	cSta = sta;
	cChn = chn;
	cNet = net;
  cLoc = loc;
	dStart = 0.0;
	dStop = 0.0;
}

CSite::CSite(const CSite &SiteIn)
{
  *this=SiteIn;
}

CSite::~CSite()
{
}


CSite& CSite::operator=( const CSite& csIn )
{
  this->cSta   = csIn.cSta;
  this->cChn   = csIn.cChn;
  this->cNet   = csIn.cNet;
  this->cLoc   = csIn.cLoc;
  this->dStart = csIn.dStart;
  this->dStop  = csIn.dStop;
  this->iPin   = csIn.iPin;
  return(*this);
}

bool CSite::operator==( const CSite& csIn ) const
{
  if(this->cSta == csIn.cSta)
    return(this->cChn == csIn.cChn && this->cNet == csIn.cNet
           && this->cLoc == csIn.cLoc);
  else
    return(FALSE);
}
 

void CSite::ResetParams(CString sta, CString chn, CString net, CString loc,
                        int iPin, double tStart, double tEnd)
{
  this->cSta   = sta;
  this->cChn   = chn;
  this->cNet   = net;
  this->cLoc   = loc;
  this->dStart = tStart;
  this->dStop  = tEnd;
  this->iPin   = iPin;
}




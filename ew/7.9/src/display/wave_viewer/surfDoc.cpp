// surfDoc.cpp : implementation of the CSurfDoc class
//

#include "stdafx.h"
#include "surf.h"
#include "comfile.h"
#include "date.h"
#include "quake.h"
#include "phase.h"
#include "surfDoc.h"
#include "surfView.h"
#include "wave.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CSurfApp *pApp;

/////////////////////////////////////////////////////////////////////////////
// CSurfDoc

IMPLEMENT_DYNCREATE(CSurfDoc, CWaveDoc)

BEGIN_MESSAGE_MAP(CSurfDoc, CWaveDoc)
	//{{AFX_MSG_MAP(CSurfDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSurfDoc construction/destruction

CSurfDoc::CSurfDoc()
{
	pApp = (CSurfApp *)AfxGetApp();
	iQuake = -2;
  bApplyGroupFilter = FALSE;
  psSurfLock = new CSingleLock(&(m_mutex));

}

CSurfDoc::~CSurfDoc()
{
  delete(psSurfLock);
}

BOOL CSurfDoc::Com(CComfile *cf) 
{
	if(cf->Is("max_trace")) 
  {
		nMaxTrace = (int)cf->Long();
		return TRUE;
	}

	return CWaveDoc::Com(cf);
}


void CSurfDoc::ApplyGroupFilter(CObArray * pSiteArray, BOOL bApplyFilter)
{
  this->bApplyGroupFilter = bApplyFilter;
  if(bApplyFilter)
    this->parrGroupSite = pSiteArray;
  pWave->GetMenu();
  // DK Cleanup  This is very ugly.  What we need to do
  // is setup the callback function (write now in CWaveDoc)
  // so that when it gets the menu reply, it filters it with
  // the group site list.  The problem is that we are trying
  // to keep <group> knowledge within the CSurfDoc class.
  // We fixed this predicament by overriding CWaveDoc::GetMenu()
  // in CSurfDoc  DK 08/29/01

}


void CSurfDoc::RefreshMenu(int iTankLocation)
{
  iRefreshMenuOnly = iTankLocation;
  pWave->GetMenu();
}


void CSurfDoc::HandleWSMenuReply(CString sReply)
{
  if(this->iRefreshMenuOnly)
  {
    CWaveDoc::HandleWSMenuReply(sReply);
    UpdateAllViews(NULL,iRefreshMenuOnly,NULL);

	// reset RefreshMenuOnly
	iRefreshMenuOnly = FALSE;
    //return;
  }
  else
  {
    psSurfLock->Lock();
    CWaveDoc::HandleWSMenuReply(sReply);
    // Filter against the group sitelist
    int iNumElements = arrTrace.GetSize();
    int i,j;
    CTrace * pTrace;
    CSite Site;
    
    if(bApplyGroupFilter && parrGroupSite && parrGroupSite->GetSize() > 0)
    {
      for(j=0; j < parrGroupSite->GetSize(); j++)
      {
        for(i=0; i < iNumElements; i++)
        {
          pTrace = (CTrace *)arrTrace[i];
          pTrace->GetSite(&Site);
          if(Site == *(CSite *)(parrGroupSite->GetAt(j)))
          {
            // If we found this group site in the array,
            // then add it to the end of the array, after [iNumElements]
            Open(Site);
            break;
          }
        }
      }
      for(i=iNumElements - 1; i >= 0; i--)
      {
        pTrace = (CTrace *)arrTrace[i];
        delete(pTrace);
        arrTrace.RemoveAt(i);
      }
    }
    if(arrTrace.GetSize() > 0)
    {
      
      pApp->pView->GetDisplay()->nlSetTraceArray(&arrTrace);
    }
    else
    {
      if(bApplyGroupFilter)
      {
        AfxMessageBox("Waveserver contains no data from the specified Group!");
        bApplyGroupFilter = FALSE;
        pWave->GetMenu();
      }
      pApp->pView->GetDisplay()->nlSetTraceArray(NULL);
    }
    psSurfLock->Unlock();
  }  // end else (iRefreshMenuOnly)

  // DavidK 20020115
  // Added call to this location due to lag problems on slow connections
  // when doing menu refreshes.
  // When we try to go to the newest data in the tank, a series of
  // events happens:
  // 1) We call CWave::GetMenu, which in turn clears the request queue.
  // 2) While we are making a menu request, and waiting for the menu
  //    reply, channels are re-requesting data, based upon the 
  //    old tank time (we haven't gotten the new one yet).
  // 3) We get the menu reply back from the wave_server, and
  //    adjust the display, and now the traces make new requests,
  //    that unfortunately get appended to the back of the 
  //    request queue behind the newly made requests for old data (from #2)
  // If we're grabbing a menu, then something major must
  // have happened, so abandon all existing requests.
  ResetDataRequestQueue();
  UnblockWaitingTraces();

}

BOOL CSurfDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	TRACE("CSurfDoc::OnNewDocument\n");
	pApp->pDoc = this;
	return TRUE;
}

//void CSurfDoc::Poll()
//{
//	TRACE("CSurfDoc::Poll\n");
//	CWaveDoc::Poll();
//}

/////////////////////////////////////////////////////////////////////////////
// CSurfDoc serialization
double CSurfDoc::TimeCrack(CComfile *cf) {

  /* DavidK Y2K change.  Declaring yr,mo,da at begining of function instead
     of on fly, due to use of if statement and sub-blocks */
  UINT yr,mo,da;
  /* End Davidk Y2k change. */

	CString date = cf->Token();
	CString time = cf->Token();
	CString zone = cf->Token();
  /* Pre Davidk Y2K proposed changes 

	UINT yr = (UINT)atoi((const char *)date.Mid(0,2)) + 1900;
	UINT mo = (UINT)atoi((const char *)date.Mid(2,2));
	UINT da = (UINT)atoi((const char *)date.Mid(4,2));

  End Pre Davidk Y2K proposed changes */

  /* Davidk Y2K proposed changes */

  if(date.GetLength() == 8)
  {
    /* Y2K Date Format */
    yr = (UINT)atoi((const char *)date.Mid(0,4));
    mo = (UINT)atoi((const char *)date.Mid(4,2));
    da = (UINT)atoi((const char *)date.Mid(6,2));
  }
  else if(date.GetLength() == 6)
  {
    /* Pre Y2K Date Format */
	yr = (UINT)atoi((const char *)date.Mid(0,2)) + 1900;
	mo = (UINT)atoi((const char *)date.Mid(2,2));
	da = (UINT)atoi((const char *)date.Mid(4,2));
  }
  else
  {
    /* Wake me up before you Go-Go Format (Error) */
    return(0.0);
  }

  /* End Davidk Y2K proposed changes */

	UINT hr = (UINT)atoi((const char *)time.Mid(0,2));
	UINT mn = (UINT)atoi((const char *)time.Mid(3,2));
	double sc =     atof((const char *)time.Mid(6,5));

	CDate cd(yr, mo, da, hr, mn, sc);
	CString cd18 = cd.Date18();
	return cd.Time();
}

void CSurfDoc::ResetDataRequestQueue(void)
{
  if(this->pWave)
    pWave->ClearRequestQueue();
}

BOOL CSurfDoc::HACK_CheckForEmptyQueue(void)
{
  if(this->pWave)
  {
    if(!this->pWave->GetRequestQueueSize())
    {
      UnblockWaitingTraces();
      return(TRUE);
    }
    else
    {
      return(FALSE);
    }
  }
  else
  {
    return(FALSE);
  }


}

void CSurfDoc::Serialize(CArchive& ar)
{
	CComfile cf;
	CString com;
	CString ctmp;
	CString csize;
	int nc;
	int nevt;
	CQuake *q;
	CPhase *p;

	if (ar.IsStoring())
	{
	}
	else
	{
		// Restore from event trigger file
		nevt = 0;
		if(!cf.Archive(&ar)) {
			AfxMessageBox("Archive error");
			AfxAbort();
		}
		while((nc = cf.Read()) >= 0) {
			com = cf.Token();
			if(cf.Is("EVENT")) {
				cf.Token();		// Skip "DETECTED"
				q = new CQuake();
				q->nType = QUAKE_TRIGGER;
				q->dTime = TimeCrack(&cf);
				cf.Read();		// Skip blank line
				cf.Read();		// Skip column headings
				cf.Read();		// Skip stupid dashes
				while((nc = cf.Read()) >= 2) {
					p = new CPhase();
					p->nType = PHASE_TRIGGER;
					p->cSite = cf.Token();
					if(p->cSite == CString("")) {
						delete p;
						break;
					}
/* Proposed DK Changes */
          p->cComp = cf.Token();
          p->cNet = cf.Token();
/*  End Proposed DK Changes */

/* The SCN trigger messages will not have a Loc
   token (the SCNL trig messages will), 
   but we don't know how to differentiate
   between a csize and a cLoc
   DK 06/03/2004
   *********************/
          p->cLoc = cf.Token();

/* Replaced By Proposed DK Changes */
//				ctmp = cf.Token();
//				p->cNet  = ctmp.Left(2);
//				p->cComp = ctmp.Right(3);
/* End Replaced By Proposed DK Changes */

					csize = cf.Token();	// B or N for big or normal, ignored for now
					p->dTime = TimeCrack(&cf);
					q->Add(p);
				}
				cCat.Add(q);
				nevt++;
			}
		}
		iQuake = -1;
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSurfDoc diagnostics

#ifdef _DEBUG
void CSurfDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSurfDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSurfDoc commands

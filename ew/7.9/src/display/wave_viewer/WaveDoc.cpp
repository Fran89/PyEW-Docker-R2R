// WaveDoc.cpp : implementation file
//

#include "stdafx.h"
#include "WaveDoc.h"
#include "site.h"
#include <math.h>
#include "surf.h"
#include "wave.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CString cSend;;
static CString cReceive;
static CString cError;
static char Txt[120];
static int nReceived;	// Characters received

// Define the Wave Server reply flags
#define SCNL_FLAGS_GOOD_REPLY 1
#define SCNL_FLAGS_GAP_REPLY  2
#define SCNL_FLAGS_BAD_REPLY  3
#define SCNL_FLAGS_LEFT_OF_TANK_REPLY  4

// Define the default cache size
#define DEFAULT_CACHE_SIZE 24000


// Define constants for dealing with wave_serverV types
#define TOKENS_PER_LINE 8

/////////////////////////////////////////////////////////////////////////////
// CWaveDoc

IMPLEMENT_DYNCREATE(CWaveDoc, CDocument)


/* Very lovely constructor method
*********************************************************************/
CWaveDoc::CWaveDoc()
{
	TRACE("CWaveDoc constructor\n");
	iMsg = 0;
  // define default cache size
	nCache = DEFAULT_CACHE_SIZE;

  // define mutex for trace related operations
  psLock = new CSingleLock(&(m_mutex));

  // indicate that we want a full menu grab to be
  // done the first time around.
  iRefreshMenuOnly = FALSE;

  iWaveServerType = WAVE_SERVER_TYPE_UNDEFINED;
  bWaveServerTypeDefined = false;

}


/* Very lovely destructor method
*********************************************************************/
CWaveDoc::~CWaveDoc()
{
  // Get rid of the mutex lock.
  delete(psLock);
	CleanupTraceArray();	// Clean up trace list
}


/* Very lovely faux SetTitle method
*********************************************************************/
void CWaveDoc::SetTitle(LPCTSTR lpszTitle)
{
  // We don't want the default MFC SetTitle() calls
  // to run, so we overrode the base class with
  // this lovely function.  That way the document
  // title is only set by us, and not the framework.
}


/* Method to get config file information
*********************************************************************/
BOOL CWaveDoc::Com(CComfile *cf) 
{

  CString cServerName;
  char szTitle[100];

  // handle gulp command
  // this is the max length (in sec) of a req we will pass to a server
  // format:   gulp <DECIMAL NUMBER IN SECONDS>
 	if(cf->Is("gulp")) 
  {
		dGulp = cf->Double();
		return TRUE;
	}

  // handle server command
  // this is name (optional), ip address, and port info for our wave_server.
  // we should probably have a check here, to make sure we do not
  // have multiple "server" commands in the config file.  Oh well.
  // format:   server [<SERVER_NAME>] <SERVER IP ADDR AAA.BBB.CCC.DDD> \
  //           <SERVER TCP PORT#>
  if(cf->Is("server")) 
  {
		cServer = cf->Token();
    // check to see if this is the name or IP addr
    if(cServer[0] < '0' || cServer[0] > '9')
    {
      // name:  store it, and grab the next token, which
      //  is the IP addr
      cServerName = cServer;
      cServer = cf->Token();
    }
    else
    {
      // parse out the last 1/2 of the IP addr.
      // we need this to print in our abbreviated title
      // since we don't have a nickname
      char * szIP;
      szIP = strchr(cServer,'.');
      szIP = strchr(szIP+1,'.');
      szIP++;
      cServerName = szIP;
    }
    // grab the TCP port#
    nPort = (int)cf->Long();

    // now formulate our title.  We set it up in two parts:
    // 1) an abbreviated part that will definitely be printed
    //    out in the ICON application summary toolbar at the
    //    bottom of the screen.
    // 2) a full unabridged title that will be printed in the
    //    title bar(coincidence?) at the top of our app window.
    // cServerName and nPort make up the abbreviated title.
    // the full title is the abbreviated plus the rest.
    sprintf(szTitle,"%s: %d -  Server %s : %d",cServerName,nPort,cServer,nPort);

    // Set the title.  Be sure to call the base class function, instead of
    // our fake SetTitle() function that we use to fool the framework!
    this->CDocument::SetTitle(szTitle);

    // Create a new CWave class to handle interactions with
    // the wave_server.
    pWave = new CWave(&cServer,nPort,this);

    // Order up a menu from the new CWave class.
    pWave->GetMenu();
    
    // done!!!
		return TRUE;
	}

  // handle cache command
  // this is the amount of cache (number of samples) that we will
  // store locally for each wave_server channel
  // format:   cache <INTEGER NUMBER OF SAMPLES>
	else if(cf->Is("cache")) 
  {
		nCache = cf->Long();
		return TRUE;
	}
	else if(cf->Is("Queue")) 
  {
		if(pWave)
    {
      pWave->SetRequestQueueLen(cf->Long());
  		return TRUE;
    }
    else
      return FALSE;
	}

  // done.  return false, indicating that we did not handle the command
	return FALSE;
} // CWaveDoc::Com()


/* Default useless method to handle a new document. 
*********************************************************************/
BOOL CWaveDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	TRACE("CWaveDoc::OnNewDocument\n");
	return TRUE;
}

/* Method to create a new CTrace object to track a
   wave_server channel, and append it to the list
   of traces.
*********************************************************************/
void CWaveDoc::Open(CSite & Site) 
{
	CTrace *tr = new CTrace(Site, nCache, this);
	arrTrace.Add(tr);
}

/* Method to clear our trace array.  It destroys
   and then removes from the array, each object in the array.
*********************************************************************/
void CWaveDoc::CleanupTraceArray(void) 
{
	int i;

	for(i=0; i<arrTrace.GetSize(); i++)
		delete arrTrace.GetAt(i);
	arrTrace.RemoveAll();
}

#define WS_REPLY_TOKENIZERS " "

/* Method to handle a reply to a menu request from a wave_server via CWave.
*********************************************************************/
void CWaveDoc::HandleWSMenuReply(CString sReply)
{

  //DK CLEANUP  Hey were screwing with the Trace Array here, we
  // need a mutex lock on the trace array before this code executes.
  // I don't see a lock in this function or in some of the code that
  // calls it.  This could be a serious problem!!!!  11/02/01

	CSite site;
  char *szToken, *szTokenString;
  int iToken;
  CString cMenuSta, cMenuChn, cMenuNet, cMenuLoc;
  int iMenuPin;
  double dMenuT1, dMenuT2;

  // Copy the replystring to our own freshly allocated char string.
  szTokenString = (char *)malloc(sReply.GetLength()+1);
  strcpy(szTokenString,(LPCTSTR)(sReply));

  // Get The first token
  szToken=strtok(szTokenString,WS_REPLY_TOKENIZERS);

  // Once we're ready, and before we handle the reply, we need
  // to figure out what to do with it.
  // If we're just doing a refresh, then we can simply update
  // the times in the current array.  If we are doing a full 
  // replace, we must get rid of the old trace array first.
  if(!iRefreshMenuOnly)
  {
    // First closeout the old Trace Array
    CleanupTraceArray();
  }

  iToken = 0;
  // Now process the new menu
  for(int i=0; szToken != NULL; i++)
  {
    // We are expecting 8 tokens per wave_server channel:
    // <PIN#> <STA> <CHAN> <NET> <LOC> <START> <END> <DATATYPE> 
    // PRE SCNLL:
    // We are expecting 7 tokens per line (per wave_server channel):
    // <PIN#> <STA> <CHAN> <NET> <START> <END> <DATATYPE> 
    
    switch(iToken) 
    {
    case 0:		// Pin number
      iMenuPin = atoi(szToken);
      break;
    case 1:		// Station name
      cMenuSta = CString(szToken);
      break;
    case 2:		// Channel
      cMenuChn = CString(szToken);
      break;
    case 3:		// Network
      cMenuNet = CString(szToken);
      break;
    case 4:		// Loc
      if(!bWaveServerTypeDefined)
      {
        /* loc code should be alphanumberic or --.
           Start time should be a float  XXX.XX
           Look for a decimal point to indicate a float */
        if(strchr(szToken, '.'))
        {
          iWaveServerType = WAVE_SERVER_TYPE_SCN;
        }
        else
        {
          iWaveServerType = WAVE_SERVER_TYPE_SCNL;
        }
        bWaveServerTypeDefined = TRUE;
      }
      if(iWaveServerType ==  WAVE_SERVER_TYPE_SCNL)
      {
        cMenuLoc = CString(szToken);
        break;
      }
      else
      {
        cMenuLoc = CString(STRING__NO_LOC);
      }
      /* don't break */
      iToken++;
    case 5:		// Starting time
      dMenuT1 = atof(szToken);
      break;
    case 6:		// Ending time
      dMenuT2 = atof(szToken);
      break;
    case 7:		// Data type (ignored for now)
      // set the site params based on our newly read values
      site.ResetParams(cMenuSta, cMenuChn, cMenuNet, cMenuLoc, iMenuPin,dMenuT1,dMenuT2);

      // If we are doing a refresh only, then attempt to locate the
      // channel in the current list, and update the update the menu times.
      // If we can't find the channel, append it to the list.
      if(iRefreshMenuOnly)
      {
        CTrace * pTrace = LocateTraceForSite(&site);
        if(pTrace)
          pTrace->UpdateMenuBounds(dMenuT1, dMenuT2);
        else
          Open(site);
      }
      // If we are doing a full replace, instead of a refresh, 
      // the just add the new channel to the array.
      else
      {
        if(arrTrace.GetSize() < this->nMaxTrace)
          Open(site);
      }
      iToken = -1;
      break;

    default:
      break;
    }
    // grab the next token
    szToken = strtok(NULL, WS_REPLY_TOKENIZERS);
    iToken++;
  }  // end for loop until we get a Null token


  // release our locally allocated buffer
  free(szTokenString);

  // Done
}  // end CWaveDoc::HandleWSMenuReply()


/* Handle the wave_server reply flags for a reply to a GETSCNL request
*********************************************************************/
int CWaveDoc::HandleReplyFlags(CString cScnFlags, CTrace * pTrace)
{
  // Check out the flags, return the appropriate code based on the
  // usability of the data.
  // F:  everything's good
  if(cScnFlags == CString("F")) 
  {
    return(SCNL_FLAGS_GOOD_REPLY);
  }
  // FL: we're asking for data that's too old.
  else if(cScnFlags == CString("FL")) 
  {
    // Missed tank on left (don't block)
    return(SCNL_FLAGS_LEFT_OF_TANK_REPLY);
  }

  // FR: we're asking for data that's too new.  Try back later, but
  // be careful not to harp on the wave_server
  else if(cScnFlags == CString("FR")) 
  {
    pTrace->Block(TRACE_BLOCK_RIGHT_OF_TANK);	// Missed tank on right
    return(SCNL_FLAGS_BAD_REPLY);
  }
  // FG: we're asking for data that's in a gap.  Will never be available
  else if(cScnFlags == CString("FG")) 
  {
    return(SCNL_FLAGS_GAP_REPLY);
  }
  // FN:  wave_server can't find the tank.  THERE's TROUBLE IN RIVER CITY!!
  else if(cScnFlags == CString("FN")) 
  {
    // Tank doesn't exist.  Uh
    // what to do now?
    // Act like we missed left
    pTrace->Block(TRACE_BLOCK_LEFT_OF_TANK);
    return(SCNL_FLAGS_BAD_REPLY);
  }
  // FC:  wave_server says the tank is corrupt.  THERE's TROUBLE IN RIVER CITY!!
  else if(cScnFlags == CString("FC")) 
  {
    // Tank is corrupt.  Uh
    // what to do now?
    // Act like we missed left
    pTrace->Block(TRACE_BLOCK_LEFT_OF_TANK);
    return(SCNL_FLAGS_BAD_REPLY);
  }
  // FU:  Yeah, FU too!  THERE's BIG TROUBLE IN RIVER CITY!!
  else if(cScnFlags == CString("FU")) 
  {
    // Uh.. Uh...   Uh
    // what to do now?
    // Act like we missed left
    pTrace->Block(TRACE_BLOCK_LEFT_OF_TANK);
    return(SCNL_FLAGS_BAD_REPLY);
  }
  else
  {
    // Some unrecognized flag.  This can't be good.
    // Uh.. Uh...   Uh
    // what to do now?
    // Act like we missed left
    pTrace->Block(TRACE_BLOCK_LEFT_OF_TANK);
    return(SCNL_FLAGS_BAD_REPLY);
  }  /* end cScnFlags check */
}  /* end HandleReplyFlags() */


#define MAXBUF 10000

/* Method that searches the CTrace array for a given channel.
*********************************************************************/
CTrace * CWaveDoc::LocateTraceForSite(CSite * pSite)
{
  CSite Site;
  // We're doing a stupid serially sequential search.
  // Oh... in a perfect world.....
  for(int i=0;i < arrTrace.GetSize(); i++)
  {
    ((CTrace *)(arrTrace.GetAt(i)))->GetSite(&Site);
    if(Site == *pSite)
    {
      return((CTrace *)(arrTrace.GetAt(i)));
    }
  }
  // If we went through the entire trace array and didn't find
  // a match for this channel, then return NULL, indicating NOT FOUND.
  return(NULL);
}


/* Method that parses a wave_server reply to a GETSCNL request.  Parse
   the reply and convert it from a string to a C array of datapoints.
*********************************************************************/
void CWaveDoc::HandleWSSCNLReply(CString sReply, CSite * pSite)
{
  CTrace * pTr;
  char *szToken, *szTokenString;
  int nScnSamp = 0, iScnPin;
  CString cScnChn, cScnNet, cScnSta, cScnFlags, cScnLoc;
  double dScnStart, dScnStep = 0.0;
  int ScnBuf[MAXBUF];
  int iFlag;
  int i;

  // We're gonna put our hands all over the trace array, so we need
  // to make sure it is locked first.
  TRACE("%u CWaveDoc::HandleWSSCNLReply  Waiting For Lock!\n",time(NULL));
  psLock->Lock();
  TRACE("%u CWaveDoc::HandleWSSCNLReply   Got Lock!\n",time(NULL));

  // First, look up the site in the array.
  pTr=LocateTraceForSite(pSite);

  // If we didn't find it, then something is hosed!  Abort!
  if(!pTr)
  {
    TRACE("HandleWSSCNLReply: Site we retrieved data for is suddenly gone!\n");
    goto End;
  }


  // Get The first token
  // Allocate our own local char array, so that we can use strtok()
  szTokenString = (char *)malloc(sReply.GetLength()+1);
  strcpy(szTokenString,(LPCTSTR)(sReply));

  // Get The first token
  szToken=strtok(szTokenString,WS_REPLY_TOKENIZERS);

  // nScnSamp is the counter for the number of samples we got.
  nScnSamp = 0;

  // Parse the header.  We are expecting 8 tokens in the header,
  // in the order listed below.  After that we are expecting 
  // samples galore.
  for(i=0; szToken != NULL; i++)
  {
    switch(i)
    {
    case 0:	// Pin number
      iScnPin = atoi(szToken);
      break;
    case 1:	// Station name
      cScnSta = CString(szToken);
      break;
    case 2:	// Channel
      cScnChn = CString(szToken);
      break;
    case 3:	// Network id
      cScnNet = CString(szToken);
      break;
    case 4:	// Loc id
      if(iWaveServerType == WAVE_SERVER_TYPE_SCNL)
      {
        cScnLoc = CString(szToken);
        break;
      }
      else
      {
        cScnLoc = STRING__NO_LOC;
        i++;
      }
    case 5:	// Flags
      cScnFlags = CString(szToken);
      break;
    case 6:	// Data type (irrelevant for ascii)
      break;
    case 7:	// Starting time (seconds past 1970, Microsoft style)
      dScnStart = atof(szToken);
      break;
    case 8:	// Sampling rate (hz), initially this was spec'ed as interval
      dScnStep = 1.0 / atof(szToken);
      break;
    default:	// Samples
      
      /******** BEGIN FUNCTIONAL CHANGE DK 20000525**********/
      if(nScnSamp < MAXBUF)
      {
        // Set the current sample equal to the value in szToken
        ScnBuf[nScnSamp] = atol(szToken);
        
        // Check to see if the new value is zero
        if(ScnBuf[nScnSamp] == 0)
        {
          // If the new value is zero, then check to see if szToken
          // was not 0, but our special fill value.
          if(strcmp(szToken,FILL_VALUE) == 0)
          {
            // This is a fill value, convert it to 
            //  FILL_NUMBER for storage.
            ScnBuf[nScnSamp] = FILL_NUMBER;
          }
        }
        else if(ScnBuf[nScnSamp] == FILL_NUMBER)
        {
        /* If the data value that came in is somehow
        equal to our magic FILL_NUMBER, then decrease 
        the vaule by one.  
        WARNING!!!!!  We are actually corrupting data here, 
        but we believe that it is OK, because: it should 
        happen rarely, we are altering the value by only 1 
        point (< .001%), and this data is for human viewing 
        only, not for computational analysis.

        *****************************************************/
          (ScnBuf[nScnSamp])--;
        /*****************************************************
        Unfortunately, when we did this, we were not aware
        that certain networks are running channels with 
        DC Offsets in the millions.  So the Offset may be 
        1 million, but the variance is only 512.  So if you
        are viewing a channel with a DC Offset near the FILL_NUMBER,
        with a very small variance, you may find problems with
        wave_viewer.  Realistically, the difference should be 
        undetectable to the human eye, or your data is so
        screwed up that you deserve what you get. 
        (end self-righteous tyrade)
        *****************************************************/
        }
        nScnSamp++;  // Increase the sample count
      }  // end if(nScnSamp < MAXBUF)
      /********   END FUNCTIONAL CHANGE DK 20000525**********/
    }  /* end switch token num */
    szToken = strtok(NULL, WS_REPLY_TOKENIZERS);
  }  /* end for each token till done */

  // Handle the reply flag
  iFlag=HandleReplyFlags(cScnFlags, pTr);

  if(iFlag == SCNL_FLAGS_GOOD_REPLY)
  {
    // if the request was valid, and there should have been data, but
    // there wasnt.  Block!
    if(nScnSamp < 1) 
    {
      pTr->Block(TRACE_BLOCK_NO_DATA_AVAILABLE);	// No data avaible, try later
    }

    // Update the CTrace local cache with the new data.
    if(pTr->Append(dScnStart, dScnStep, nScnSamp, (long *)ScnBuf))
    {
      // There were new useful samples in the reply, note that
      // trace has been updated.
      SetTraceHasBeenUpdated(TRUE);
    }

  }
  else if(iFlag == SCNL_FLAGS_GAP_REPLY)
  {
    // Our entire request fell into a gap.  Unfortunately wave_server doesn't
    // tell us what our request was, only that it started at time T and fell
    // completely into a gap.
    //
    // HAH!  Lucky us!  We managed to slip the request (as a CSite object)
    // to CWave, and it passed it back to us, so we can get the starting
    // and ending time from the CSite, and we can formulate FILL_NUMBER data 
    // to fillthe request within the local cache, so that we don't keep  
    // asking wave_server for it.
    int iNumSteps = (int)((pSite->dStop - pSite->dStart)/dScnStep);
    for(int i=0; i < iNumSteps; i++)
      ScnBuf[i] = FILL_NUMBER;
    nScnSamp = iNumSteps;
    if(pTr->Append(pSite->dStart, dScnStep, nScnSamp, (long *)ScnBuf))
    {
      SetTraceHasBeenUpdated(TRUE);
    }
  }
  else if(iFlag == SCNL_FLAGS_LEFT_OF_TANK_REPLY)
  {
    // Send back fill, so we won't ask any more, cause it ain't coming
    int iNumSteps = (int)((pSite->dStop - pSite->dStart)/dScnStep);
    for(int i=0; i < iNumSteps; i++)
      ScnBuf[i] = FILL_NUMBER;
    nScnSamp = iNumSteps;
    if(pTr->Append(pSite->dStart, dScnStep, nScnSamp, (long *)ScnBuf))
    {
      SetTraceHasBeenUpdated(TRUE);
    }

  }
  else
  {
    // This is either SCNL_FLAGS_BAD_REPLY or something else we don't
    // know of.  Either way, it's bad and we're done.
    // The appropriate blocking was already done by HandleReplyFlags()
  }
  // release are locally allocated char array
  free(szTokenString);
End:
  // release the lock on the all important mutex
  psLock->Unlock();
  TRACE("%u CWaveDoc::HandleWSSCNLReply   Released Lock!\n",time(NULL));
}


/* Braindead method that reports the status of the bTraceWasUpdated flag
********************************************************************/
BOOL CWaveDoc::TraceHasBeenUpdated(void)
{
  return(bTraceWasUpdated);
}

/* Braindead method that allows other classes, like CSurfView to update
   the bTraceWasUpdated flag.  The only access really neccessary if 
   for CSurfView to be able to reset the flag to FALSE after it orders
   a screen redraw.
********************************************************************/
void CWaveDoc::SetTraceHasBeenUpdated(BOOL bUpdated)
{
  bTraceWasUpdated = bUpdated;
}


/* Method that allows individual CTrace objects to request channel data
   via our CWaveDoc class.
********************************************************************/
void CWaveDoc::MakeTraceRequest(double dStart, double dEnd, const CSite * pSiteIn)
{

  CSite Site;
  // Debug statement showing initial request params, before we disect it
  TRACE("Initial Trace Req (%s,%s,%s,%s) [%.2f-%.2f]\n",
        pSiteIn->cSta, pSiteIn->cChn, pSiteIn->cNet, pSiteIn->cLoc,
        pSiteIn->dStart, dEnd-dStart);

  // Sanity check.
  if(dStart > dEnd)
    TRACE("dStart < dEnd\n");

  // We are required to break up the requests that we submit to CWave 
  // into GULP sized chunks
  for(double dTemp = dStart; dEnd - dTemp > this->dGulp; dTemp+=this->dGulp)
  {
    // Grab the channel info, and set the start and end times of the request.
    Site = *pSiteIn;
    Site.dStart = dTemp;
    Site.dStop  = dTemp+dGulp;
    // Request data via CWave
    pWave->GetSCNLTraceData(&Site,dTemp,dTemp+dGulp);
    // If we've already generated 8 GULP sized requests from 
    // the original, then quit.  That's enough requests for now
    // from one channel.  8 is kinda magic (read ARBITRARY).
    if(dTemp == dStart + 8*dGulp)
      return;
  }

  // Grab the channel info, and set the start and end times of the request.
  Site = *pSiteIn;
  Site.dStart = dTemp;
  Site.dStop  = dEnd;

  // Request data via CWave
  pWave->GetSCNLTraceData(&Site,dTemp,dEnd);

  return;
}  // end CWaveDoc::MakeTraceRequest()


/* Method that unblocks all trace channels that are blocked only
   because they are waiting for the queue to clear, not because of
   any data irregualarity problems.
   This method is designed to be called by CWave when it empties the
   queue.  This way, all the channel CTraces make a bunch of requests.
   Those requests are all queued and filled serially by CWave.  When
   CWave has processed the entire queue, it calls this method which 
   free's the CTrace objects up to request more data.
********************************************************************/
void CWaveDoc::UnblockWaitingTraces(void)
{
  for(int i=0; i < arrTrace.GetSize(); i++)
  {
    // Only unblock if the trace is waiting for queue to clear.
    // Don't unblock if the block is due to a bad request or bad data.
    if(((CTrace *)arrTrace[i])->GetBlockCode() == TRACE_BLOCK_WAITING_FOR_QUEUE)
      ((CTrace *)arrTrace[i])->RemoveBlock();
  }
} // end CWaveDoc::UnblockWaitingTraces()



BEGIN_MESSAGE_MAP(CWaveDoc, CDocument)
	//{{AFX_MSG_MAP(CWaveDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveDoc diagnostics

#ifdef _DEBUG
void CWaveDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWaveDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveDoc serialization

void CWaveDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWaveDoc commands

// Trace.cpp : implementation file
//

#include "stdafx.h"
#include "surf.h"
#include "Trace.h"
#include "WaveDoc.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CTrace

IMPLEMENT_DYNCREATE(CTrace, CCmdTarget)


/* Useless Default Constructor for CTrace
*********************************************************************/
CTrace::CTrace()
{
	nFlag = TRACE_BOGUS;
}


/* Useful Constructor for CTrace
*********************************************************************/
CTrace::CTrace(CSite & Site, int nBuf, CWaveDoc * pWaveDoc)
{
	long *buf;
	int i;

  // Set the Site
	this->Site=Site;

  // Set the State
	nFlag = TRACE_INITIALIZE;

  // Initialize attributes
	iBlock = 0;
  bBlockStateChanged = FALSE;
	dStep = 0.01;
  dStart = dStop = 0;
	dTotal = 0.0;
	this->nBuf = nBuf;
 	dTotal = dStep * nBuf;

  // Allocate a local cache for trace
	hBuf = GlobalAlloc(GMEM_MOVEABLE, nBuf*sizeof(long));


  // Initialize the local cache
	buf = (long *)GlobalLock(hBuf);
	for(i=0; i<nBuf; i++)
		buf[i] = UNTOUCHED_FILL_NUMBER;
	GlobalUnlock(hBuf);

  // Set the linkback to our Wave Document (there's only one)
  this->pWaveDoc = pWaveDoc;

  // Reset state to Active
 	nFlag = TRACE_ACTIVE;

  // Initialize trace search and retrieval variables
  bCheckAgain = TRUE;
  bNewTraceRecvd = FALSE;
}  // end CTrace::CTrace()


/* Default Destructor
*********************************************************************/
CTrace::~CTrace()
{
	GlobalFree(hBuf);
}


/* Method to calculate local cache index from time.  Given
   a time, this method calculates where in the local cache that data
   would be.
   Returns -1 if the requested time is not within the local cache, 
   otherwise returns the index associated with the given time.  
   *Note*:  Just because an index is returned, does not mean that a
   valid datapoint exists within the local cache at that index.  
   (Meaning the function tells you where the data would be if it were
    there.  It does not indicate whether the data is or is not there.)
*********************************************************************/
long CTrace::Index(double t) 
{

  // We're not active, so there is no data
	if(nFlag != TRACE_ACTIVE)
		return -1;

  // If the time is less than the start time or more than the end time 
  // of the cache, return -1, indicating "not in local cache".
	if(t < this->dStart || t >= this->dStart + this->dTotal - dStep * 0.5)
		return -1;

  // Added 0.5 to the position total before truncating
  // it into a long.  The problem here is sloppy timestamps.
  // You can get a timestamp that is off by .001/samplerate,
  // and without the 0.5, it will be truncated to the prior
  // index, instead of the current index where it belongs.
  //     DavidK 080701
	return (long)((fmod(t, dTotal) / dStep)+0.5);
}  // end CTrace::Index()


/* Method calculates the time for a datapoint at index "i" in the
   local cache.
   This function is the inverse of Index().
   Returns 0 if the index or Trace is invalid, otherwise returns
   the time as seconds since 1970.
*********************************************************************/
double CTrace::T(long i) 
{
	double t;

  // Trace is bogus
	if(nFlag != TRACE_ACTIVE)
		return 0.0;

  // Index is invalid
	if(i < 0 || i >= nBuf)	// Check for programmer error
		return 0.0;

  // Get the start time for the local cache
	long i0 = Index(dStart);

  // Calculate the time of interest based on start time 
  // and index position
	if(i >= i0)
		t = dStart + dStep * (i - i0);
	else
		t = dStart + dStep * (i + nBuf - i0);
	return t;
}  // end CTrace::T()


/* Method to add trace data from a wave_server to the local cache.
   Returns the number of new samples added to the local cache.  Fill
   values are not counted in the number of new samples.
*********************************************************************/
int CTrace::Append(double t, double step, int n, long *in) 
{
	int i;
	double t1, t2;
	double tend;

  // hopefully this should only happen for the first trace
  // request, which is the first time we see the true step size
  if(step && step != this->dStep)
  {
    if(step > 0.00001)
      dStep = step;
    else
      TRACE("Bad step in CTrace:: Append (%.4f)\n",step);
    dTotal = dStep * nBuf;
  }

	tend = t + step * (n-1);
	t1 = t;
	t2 = tend;

  // Check the tank bounds and reset if neccessary
  if(!CheckAndSetCacheBoundaries(t1,t2))
    return(0);

	// Transfer new data to trace buffer
	long *buf = (long *)GlobalLock(hBuf);
	int j = Index(t1);
  int iNumNew = 0;

  // Loop through the new data, comparing it with
  // the old data.  Keep track of how many new 
  // useful data samples we got
	for(i=0; i<n; i++) 
  {
    if(buf[j] != in[i])
      iNumNew++;
		buf[j++] = in[i];
		if(j >= nBuf)
			j = 0;
	}
  // release the local cache buffer
	GlobalUnlock(hBuf);

  // If we got any new useful data samples, then
  // record that we got new trace, so that it will
  // be drawn to the screen.
  if(iNumNew)
  {
    // if we have received trace to be drawn from a
    // previous call, then just add what we've got to
    // the existing list.  (Sometimes, more than one
    // one trace request can be appended in between
    // drawing calls, and we don't want to overwrite
    // the data listed as NEW by the last call.)
    if(this->bNewTraceRecvd)
    {
      if(t < this->dNewTraceStart)
        this->dNewTraceStart = t;
      if((t + step * n) > this->dNewTraceEnd)
        this->dNewTraceEnd = (t + step * n);
    }
    else
    {
      this->dNewTraceStart = t;
      this->dNewTraceEnd = (t + step * n);
      this->bNewTraceRecvd = TRUE;
    }
  }

  // move the dStop time if we've added data after 
  // the previous dStop
  if(t2 > this->dStop)
    this->dStop = t2;
  return(iNumNew); 
}  // end CTrace::Append()


/* Method to set a block condition on the Trace object.  When the 
   CTrace object is blocked, it cannot request data from a 
   wave_server.  A Trace can be blocked for several reasons, most of
   which are a result of a problem retrieving data for that CTrace
   from a wave_server.  Traces are also blocked because they have
   already made a set of requests to a wave_server, and they are
   waiting for those existing requests to be filled.  (You don't want
   to ask for the same data twice.)
*********************************************************************/
void CTrace::Block(int iBlockCode)
{
  iBlock = iBlockCode;

  // bBlockStateChanged is used by CBand when determining whether to
  // redraw the block code portion of the trace header.
  bBlockStateChanged = TRUE;
}


/* Simple method to return the object's current block code.
*********************************************************************/
int CTrace::GetBlockCode(void)
{
  return(iBlock);
}


/* Method to remove the current block on the CTrace.
*********************************************************************/
void CTrace::RemoveBlock(void)
{
  iBlock = 0;
  bBlockStateChanged = TRUE;
}


/* Method to check (and if neccessary adapt) the current local cache
   against a given time window.  The method is used to do all of the
   house cleaning neccessary when shifting the cache, when the cache
   should now hold a slighty different time window of data than before.
   Example:  The local cache goes from time "0 - 10".  A time window of
   "9 - 11" is passed as input to this method.  The method shifts the
   local cache to "1 - 11", and reinitializes the cache data that now
   runs from "10 - 11".
   dStart must be <= dEnd, and dEnd - dStart should not be greater 
   than the size of the local cache.
*********************************************************************/
BOOL CTrace::CheckAndSetCacheBoundaries(double dStart, double dEnd)
{
  int i1,i2;
  int i;
  long * buf;
  double dStartTemp;

  // Sanity Check
  if(dStart > dEnd)
    return(FALSE);

  // Get the index of the start of the new time window
  i1=Index(dStart);
  if(i1 == -1)
  {
    // The index is bogus, we have to do some shifting to
    // bring the new start time into the local cache
    if(Site.dStart)
    {
      // Fix any rounding errors by bringing the time
      // we were passed into step with the start of the tank.
      // If we have to change the start, then make sure we
      // make the new "synched" start less than the old.
      int nSteps = (int)((dStart - Site.dStart)/dStep);
      dStartTemp = Site.dStart + nSteps * dStep;
      if(dStartTemp > dStart)
        dStartTemp -= dStep;
    }
    else
    {
      dStartTemp = dStart;
    }

    
    double dStartOld = this->dStart;
    // dStartTemp holds what will be the new starting time of
    // the local cache.
    // dStartOld holds the previous starting time.

    // What we need to do now, is determine what data is now
    // invalid after the shift.

    // The local cache is a circular memory buffer.  It runs
    // from time X to time Y.  The position of a time in the
    // buffer is calculated as time since 1970 modulus the
    // buffer size.  The absolute offset since 1970 is given
    // by the starttime of the buffer.  This way, as the starttime
    // is shifted, all of the existing data is valid in its
    // current position.  It is only invalidated when it becomes
    // less than the start time, or greater than the 
    // (start time + buffer len (in sec)).
    this->dStart = dStartTemp;
    // adjust dStop if neccessary
    if(this->dStart + this->dTotal < this->dStop)
      this->dStop = this->dStart + this->dTotal;

    // check to see if there is any useful data at all.
    // since the index request for the new start came back
    // as -1, we know that 
    // (dStartNew < dStartOld || dStartNew > dStartOld+BufferLen)
    if(this->dStart > dStartOld || this->dStart <= dStartOld - dTotal)
    {
      // the old and the new local cache's will not overlap at all
      // no worries about trying to save data.  Blow it all away!

      // clear all trace
      buf = (long *)GlobalLock(hBuf);
      for(i=0; i<nBuf; i++)
        buf[i] = UNTOUCHED_FILL_NUMBER;
      GlobalUnlock(hBuf);
    }
    else
    {  // dStart is before the old dStart, but we still have
       // some useful samples left
      buf = (long *)GlobalLock(hBuf);

      // reinitialize the datapoints in the local cache between 
      // dStart and dStartOld since that area does not overlap
      for(i=Index(dStart); i!=Index(dStartOld); i++)
      {
        buf[i] = UNTOUCHED_FILL_NUMBER;
        if(i==nBuf)
          i=0;
      }
      GlobalUnlock(hBuf);
    }
    // now get a vaild Index for dStart  (this better be valid)
    i1=Index(dStart);
  }

  // check the end of the given time window to see if it is 
  // within local cache.
  i2=Index(dEnd);

  if(i2 == -1)
  {
    // oops it's (the end of the time window) not within the 
    // local cache.  Repeat the slide process based on the end time.
    if(dEnd - dStart > (this->nBuf * this->dStep - 1))
    {
      // oh crap!  The screen time is longer than the local cache.
      // this isn't supported, issue a warning!!!
      TRACE("WARNING!!! the display time is longer than the cache!\n"
            "  Wave_viewer will not function properly in this manner.\n"
            "  Please extend the cache (in the config file) and restart,\n"
            "  or shrink the amount of time shown on the display.\n");
      return(FALSE);
    }
    else
    {
      if(dEnd > this->dStart + (this->nBuf-1) * this->dStep)
      {
        // We need to slide the cache, so that it will
        // incorporate the new end that we are asking for.
        /* Steps:
        1)  Figure out where the new start time is, based
        on subtracting the buffer time from the new end time.
        2)  Clear the buffer from the old start time to the new start
        time.
        3)  Set the new start time.
        4)  Done.
        *******************************************/

        // 1)  Figure out where the new start time is, based
        //     on subtracting the buffer time from the new end time.
        double dOldStart = this->dStart;
        double dNewStart = dEnd - (this->nBuf * this->dStep - 1 );

        buf = (long *)GlobalLock(hBuf);
        
        if(dNewStart - dOldStart >= this->nBuf * this->dStep)
        {
          // 2)  Clear the buffer from the old start time to the new start
          //     time.
          for(int iCtr=0; iCtr < this->nBuf; iCtr++)
          {
            buf[iCtr] = UNTOUCHED_FILL_NUMBER;
          }
        }
        else
        {
          int i=Index(dOldStart);
          int iNew = Index(dNewStart);
          buf[i] = UNTOUCHED_FILL_NUMBER;
          i++;
          if(i >= this->nBuf)
            i = 0;
          // 2)  Clear the buffer from the old start time to the new start
          //     time.
          for(; i !=iNew; i++)
          {
            /* DavidK Change 20020115 
            if(i >= this->nBuf)
              i=0;
            buf[i] = UNTOUCHED_FILL_NUMBER;
            *  CHANGED TO */
            if(i >= this->nBuf)
            {
              i=-1;
            }
            else
            {
              buf[i] = UNTOUCHED_FILL_NUMBER;
            }
            /* END DavidK Change 20020115
               Change made to fix an infinite loop problem
               when iNew == 0.
             **********************************************/
          }
        }

        // 3)  Set the new start time.
        this->dStart = dNewStart;

        // shift dStop if neccessary
        if(this->dStart + this->dTotal < this->dStop)
          this->dStop = this->dStart + this->dTotal;

        // 4)  Done.

        // recalculate i1 and i2 for kicks!
        i1=Index(this->dStart);
        i2=Index(dEnd);
        if(i2 == -1)
        {
          TRACE("ERROR Adjusting End Index: CTrace::CheckAndSetCacheBoundaries(%.2f dStart, %.2f dEnd)\n",
            dStart, dEnd);
        }
      }  /* end if dEnd > this->dStart + bufferlen */
      else
      {
        // ERROR:  We know from the bogus index,  
        // that (dEnd < this->dStart || dEnd > this->dStart + bufferlen)
        // so this means that (dEnd < this->dStart).  This is clearly an
        // error and it should have been handled by earlier code.
        // I don't think this code patch ever gets executed, but it is
        // more Debug-only info, so it can't hurt.
        TRACE("ERROR:  CTrace::CheckAndSetCacheBoundaries(%.2f dStart, %.2f dEnd)\n",
          dStart, dEnd);
      }
      
    }  // end if dEnd - dStart > buffer time
    GlobalUnlock(hBuf);
  }  // end if i2 == -1

  // Now for the acid test.  After all our posturing, hootin' and a haullerin'
  // make sure that the indexes come up valid.
  if(Index(dStart) == -1)
    return(FALSE);
  if(Index(dEnd) == -1)
    return(FALSE);

  return(TRUE);
}  // end CTrace::CheckAndSetCacheBoundaries()


/* Method with two purposes.
   1) Tell CTrace what data it should keep in its local cache.
   2) Give CTrace the opportunity to scan it's local cache
      and request missing data from the wave_server.
      (assuming that the CTrace object is not in a Blocking state).
*********************************************************************/
BOOL CTrace::CheckForNeededTrace(double dStart, double dEnd)
{
  int i1,i2;
  int i;
  long * buf;
  BOOL bRetCode;

  // I'm in a blocked state.  Abort!
  if(this->iBlock)
    return(FALSE);

  // Nothing new since the last time we checked (and found nothing).  Abort!
  if(!bCheckAgain)
    return(FALSE);

  // Whacked out window times.  Abort!
  if(dStart > dEnd)
  {
    TRACE("ERROR ERROR ERROR:  CTrace::CheckForNeededTrace(%.2f dStart,"
          " %.2f dEnd)\n",
          dStart, dEnd);
    return(FALSE);
  }


  // check the local cache versus the new time window we were passed.
  // adjust if neccessary.
  if(!CheckAndSetCacheBoundaries(dStart,dEnd))
    return(FALSE);

  // grab the indexes for the time window.
  i1 = Index(dStart);
  i2 = Index(dEnd);

  buf = (long *)GlobalLock(hBuf);
  int iGapStart = -1;
  BOOL bInGap = FALSE;

  // search through the given time window (within the local cache)
  // and request data for the first gap we see.
  for(i=i1; i!=i2; i++)
  {
    if(bInGap)
    {
      // we're in a gap, and we found a valid data point.  Gap over,
      // make request!
      if(buf[i] != UNTOUCHED_FILL_NUMBER)
      {
        pWaveDoc->MakeTraceRequest(T(iGapStart) - 2 * this->dStep,T(i),&(this->Site));
        bRetCode = TRUE;
        goto CheckForNeededTrace_End;
      }
    }
    else 
    {
      // we're not in a gap, and we found an unreported data point.
      // we found the start of a gap (in the local cache)
      if(buf[i] == UNTOUCHED_FILL_NUMBER)
      {
        iGapStart = i;
        bInGap = TRUE;
      }
    }
    if(i == nBuf - 1)
      i=-1;
  }
  // We finished the end of the gap search loop.  However, we may
  // have been in a (endless) gap when we stopped searching, so we
  // must do gap processing at the end of the loop.
  if(bInGap)
  {
    // We ended in a gap.  Make a request for data from iGapStart
    // until the end (i2).
    GlobalUnlock(hBuf);
    pWaveDoc->MakeTraceRequest(T(iGapStart),T(i2),&(this->Site));
    bRetCode = TRUE;
    goto CheckForNeededTrace_End;
  }
  else
  {
    // We didn't find squat!  Note it!
    bRetCode = FALSE;
  }

CheckForNeededTrace_End:
  if(bRetCode)
  {
    // We found something and made a request.  We don't want to 
    // research again until we get our current request processed.
    // So block ourselves.  The request processor code will unblock
    // us when it finishes processing all of the current requests.
    this->iBlock = TRACE_BLOCK_WAITING_FOR_QUEUE;
    // Note that we changed the block state.  This was enabled for
    // debug, but in production we don't draw a block color for 
    // Queue Waiting.
    bBlockStateChanged = TRUE;
  }
  else
  {
    // We didn't find anything, so don't bother checking again until
    // something happens.  If something happens somebody else will wake
    // us up and reset this bCheckAgain flag.
    bCheckAgain = FALSE;
  }
    
  // Free Willy!  or atleast release the local cache lock.
  GlobalUnlock(hBuf);

  return(bRetCode);
}  // end CTrace::CheckForNeededTrace()


/* Method to return the time at the left of the wave_server 
   tank (oldest data).
   (based upon the last menu we got!)
*********************************************************************/
double CTrace::GetLeftOfTank(void)
{
  return(Site.dStart);
}


/* Method to return the time at the right of the wave_server 
   tank (newest data).
   (based upon the last menu we got!)
*********************************************************************/
double CTrace::GetRightOfTank(void)
{
  return(Site.dStop);
}


/* Method to return the sample rate of the current channel.
   We even try to estimate one if we have not yet retrieved data
   for this channel.  (THIS MAY NOT BE A GOOD THING!)
*********************************************************************/
double CTrace::GetSampleRate(void)
{
  // if dStep is valid, just return 1/dStep
  if(dStep >= 0.001 && dStep <= 100.00)
    return(1.0/dStep);
  // otherwise, get fancy
  else
  {
    // try and calculate dStep based on start and end time.
    double dStepEst;
    dStepEst = fmod(Site.dStart,1.00) - fmod(Site.dStop,1.00);
    if(dStepEst >= 0.001)
      // Ah hah, a good calculation
      return(1.0/dStep);
    else if(dStepEst == 0.0)
      // Ah hah, 1hz data (or greater)
      return(1.0);
    else
      // No clue!!
      return(0);
  }
}  // end CTrace::GetSampleRate()


/* Method to return a COPY of the CSite for this channel.
*********************************************************************/
void CTrace::GetSite(CSite * pSite)
{
  *pSite = Site;
}  // end CTrace::GetSite()


/* Method to update the wave_server tank boundaries for this CTrace.
*********************************************************************/
void CTrace::UpdateMenuBounds(double dMenuStart, double dMenuEnd)
{
  this->Site.dStart = dMenuStart;
  this->Site.dStop  = dMenuEnd;
}  // end CTrace::UpdateMenuBounds()


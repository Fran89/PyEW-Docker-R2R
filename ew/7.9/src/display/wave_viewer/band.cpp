// band.cpp : implementation file
//

#include "stdafx.h"
#include "band.h"
#include "surf.h"
#include "display.h"
#include "trace.h"
#include <math.h>
#include <limits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBand

IMPLEMENT_DYNCREATE(CBand, CCmdTarget)


/* Default constructor.  Create the Band
   but label it useless(BAND_BOGUS), since it has no
   trace.  
*************************************************/
CBand::CBand()
{
  this->pTrace = NULL;
  this->nState = BAND_BOGUS;
}


/* Useful CBand constructor.  Create the Band passing
   it a CTrace object.  Set state to INIT.  Initialize
   some member variables.
*************************************************/
CBand::CBand(CTrace * pTrace)
{
	this->pTrace = pTrace;
  nState = BAND_INIT;
	nScale = INT_MAX;
	nBias = 0;
	dTrigger = 0.0;
}


/* Default CBand destructor.
*************************************************/
CBand::~CBand()
{
}


/* Useful CBand copy constructor in the form of an '='
   operator.
   Copy our Trace object, along with band state
   and manipulation/filtering info.
*************************************************/
CBand& CBand::operator=( const CBand& cbIn )
{
	this->pTrace = cbIn.pTrace;
	nState = cbIn.nState;
	nScale = cbIn.nScale;
	nBias = cbIn.nBias;
	dTrigger = cbIn.dTrigger;
  return(*this);
}


/* Method to unblock the trace for a given Band.
   See CTrace::RemoveBlock() for a description
   of blocking/unblocking CTrace objects.
*************************************************/
void CBand::UnblockTrace()
{
  if(pTrace)
    this->pTrace->RemoveBlock();
}


/* Method to calculate the scale and gain of a
   trace object.  The gain is always calculated
   and applied to this object's filtering properties.
   Scale (gain) is only applied under certain circumstances.

   The bias (DC Offset) is calculated based on an
   average of the data points within the current
   CDisplay time window.

   The Scale is calculated based upon the variance of
   datapoints with the current CDisplay time window.
   There is a set of rules within this function that
   determines if and how the current variance should
   be used to calculate the scale.  If there is little
   data available for the calculation, then the scale
   is made to be a large multipe of the variance ( 8 * stddev).
   If there is a mid amount of data, then the scale is
   set to (4 * stddev), and if there is already a lot
   of data, then the current variance isn't applied to
   the Band's Scale.  (It is assumed that once there
   is a fair amount of data, that the user will have
   set the Scale to the value they desire, and there
   is no reason for the program to reset it.
*************************************************/
void CBand::Calibrate(CDisplay *dsp) 
{
	CTrace *tr;
	long i;
	long ibuf,ibuf2;
	long nbuf;
	double sum;
	double dev;
	long *buf;
  double dCalcStart,dCalcStop;
  int  nTempScale, nTempBias;
  // DK 20000525 Added nfill to take into account filled samples
  int nfill;

  if(!pTrace)
    return;

	tr = this->pTrace;

  // Calculate the start of the trace window that we will use
  //   for calibrations
  if(tr->dStart > dsp->GetStartTime())
    dCalcStart = tr->dStart;
  else 
    dCalcStart = dsp ->GetStartTime();

  // Calculate the end of the trace window that we will use
  //   for calibrations
  if(tr->dStop < dsp->GetEndTime())
    dCalcStop = tr->dStop;
  else 
    dCalcStop = dsp ->GetEndTime();

  // Sanity check
  if(dCalcStop <= dCalcStart)
  {
    // Sanity check failed, set the window to be whatever
    // trace we currently have in local cache
    dCalcStart = tr->dStart;
    dCalcStop  = tr->dStop;
  }

  // Figure out where our window is in the local cache
	ibuf = tr->Index(dCalcStart);
	ibuf2 = tr->Index(dCalcStop);
  nbuf = ibuf2 - ibuf;
  if(nbuf < 0)
    nbuf += tr->nBuf;

	sum = 0.0;

  // Get a lock on the local cache
	buf = (long *)GlobalLock(tr->hBuf);
  // DK 20000525 Added nfill to take into account filled samples
  nfill = 0;

  // Calculate Bias
  for(i=0; i<nbuf; i++) 
  {
    // DK 20000525 Don't count fill values
    if(buf[(ibuf+i)%tr->nBuf] == FILL_NUMBER || 
       buf[(ibuf+i)%tr->nBuf] == UNTOUCHED_FILL_NUMBER)
    {
      nfill++;
    }
    else
    {
      sum += buf[(ibuf+i)%tr->nBuf];  // Sum all valid data points
    }
  }
  
  // DK 20000525 Added nfill to take into account filled samples
  nTempBias = (long)(sum/(nbuf-nfill));
  sum = 0.0;
  
  // Calculate variance (stddev)
  for(i=0; i<nbuf; i++) 
  {
    // DK 20000525 Don't count fill values
    if(buf[(ibuf+i)%tr->nBuf] != FILL_NUMBER && 
       buf[(ibuf+i)%tr->nBuf] != UNTOUCHED_FILL_NUMBER)
    {
      dev = buf[(ibuf+i)%tr->nBuf] - nTempBias;
      sum += dev * dev;
    }
  }

  // Done with buffer, release lock
	GlobalUnlock(tr->hBuf);

  // Calculate 
  // DK 20000525 Added nfill to take into account filled samples
	dev = sqrt(sum / (nbuf-nfill+1));
	if(dev < 10.0)
		dev = 10.0;

  // Set scale to 4 std-deviations
  nTempScale = (int)(dev * 4);
  if((nbuf-nfill+1) < 30) 
  {
    //We have very few data points, so set scale to 16 sd
    nTempScale *= 4;
  }

  // check for overflow.... not sure what the deal is with these 4 std deviations.....
  // anyway doing a bitshift on a negative number is not gonna turn it into 0
  if(nTempScale < 0)
    nTempScale = 2 << 29;

  // round Scale down to nearest power of 2
  int iCtr=0;
  for(;nTempScale;nTempScale>>=1)
    iCtr++;
  nTempScale = 1 << iCtr;

  // If the band is active, only update the Bias.
  // Otherwise update both the Scale and the Bias
  if(this->nState == BAND_ACTIVE)
  {
    this->nBias = nTempBias;
  }
  else
  {
    // Don't update the Scale, if it has been set by the
    // user in the config file.
    if(!dsp->GetbHumanScale())
      this->nScale = nTempScale;
    this->nBias = nTempBias;
  }

  // Debug statement
	TRACE("Calibrate: ibuf=%d, nbuf=%d, nBias=%d, nScale=%d\n",
		ibuf, nbuf, this->nBias, this->nScale);
} // end CBand::Calibrate()


/* Method to draw frame and tick marks for the band.
*************************************************/
void CBand::Frame(CDC *pdc, CDisplay *dsp, RECT *rect) {
	double t;
	int it;
	int ntick;
	int x;

	// Draw top and bottom boundaries
	pdc->MoveTo(0, rect->top);
	pdc->LineTo(rect->right, rect->top);
	pdc->MoveTo(0, rect->bottom);
	pdc->LineTo(rect->right, rect->bottom);

	// Generate tick marks
	double t1 = dsp->GetStartTime() - fmod(dsp->GetStartTime(), 60.0);
	double t2 = dsp->GetEndTime();

  // Display is F'd up.  Quit & Go Home!
	if(t2-t1 < 0.0)
		return;

  // Loop through time on the display at 1 second intervals.
  // Draw the appropriate Tick Mark each second.
	for(t=t1; t<=t2; t+=1.0) 
  {
		if(t < dsp->GetStartTime())
			continue;
		it = (int)(fmod(t, 60.0) + 0.5);
		ntick = 5;
		if(it%10 == 0)
			ntick = 10;
		if(it%60 == 0)
			ntick = 20;
		x = dsp->GetHeaderWidth() + (int)((t - dsp->GetStartTime()) * dsp->GetPixelsPerSecond());
		pdc->MoveTo(x, rect->top);
		pdc->LineTo(x, rect->top+ntick);
		pdc->MoveTo(x, rect->bottom);
		pdc->LineTo(x, rect->bottom-ntick);
	}
}


/* Method to draw the header for the band.
*************************************************/
void CBand::Head(CDC *pdc, CDisplay *dsp, RECT *rect) 
{
	CTrace *tr;
	char txt[40];
  int vsize;

  // If there's no trace then Quit & Go Home!
  if(!pTrace)
    return;
  else
    tr = pTrace;

  // Set the colors so that they match system colors
  DWORD dColor = GetSysColor(COLOR_WINDOW);
  pdc->SetBkColor(dColor);
  dColor = GetSysColor(COLOR_WINDOWTEXT);
  pdc->SetTextColor(dColor);

  // Calculate the vertical size for this band, so 
  // we know how many lines of text we can write in.
  vsize=rect->bottom - rect->top;
  // Calculate the vertical space neccessary for each line.
	CSize sz = pdc->GetTextExtent("XXXXXX");
	int inc = sz.cy + 2;

  // Set the starting point for text output
	int x = 5;
	int y = rect->top+5;

  // Write out the Net.Station Codes for this channel
  sprintf(txt, "%s . %s", tr->Site.cSta, tr->Site.cNet);  
	pdc->TextOut(x, y, (const char *)txt);
	y += inc;

  // If there's room, write out the Channel Code for this channel
  if (vsize > (y + inc) - rect->top)
  {
    // Write out the Chan.Loc Codes for this channel
    sprintf(txt, "%s . %s", tr->Site.cChn, tr->Site.cLoc);  
	  pdc->TextOut(x, y, (const char *)txt);
	  y += inc;
  }

  // If there's room, write out the Scale for this channel
  if (vsize > (y + inc) - rect->top)
  {
    sprintf(txt, "+/- %6d",nScale);
    pdc->TextOut(x, y, txt);
    y += inc;
  }

  // If there's room, write out the Bias for this channel
  if (vsize > (y + inc) - rect->top)
  {
    if(nBias >= 20000 || nBias <= (-20000))
      sprintf(txt, "Bias: %4dk", nBias/1000);
    else
      sprintf(txt, "Bias: %5d", nBias);
    pdc->TextOut(x, y, txt);
    y += inc;
  }

  // If there's room, write out the Bias for this channel
  if (vsize > (y + inc) - rect->top)
  {
    sprintf(txt, "Hz: %-6d", (int)(1.0/(tr->dStep)));
    pdc->TextOut(x, y, txt);
    y += inc;
  }

  

	// Draw '+' and '-' boxes for individual channel scale adjustment
	sz = pdc->GetTextExtent("+");
	int nx = sz.cx + 4;
	int ny = sz.cy + 4;
	x = dsp->GetHeaderWidth() - nx;
	y = rect->top;
	pdc->MoveTo(x, rect->top);
	pdc->LineTo(x, rect->bottom);
	pdc->MoveTo(x, rect->top+ny);
	pdc->LineTo(x+nx, rect->top+ny);
	pdc->MoveTo(x, rect->bottom-ny);
	pdc->LineTo(x+nx, rect->bottom-ny);
	sz = pdc->GetTextExtent("-");
	pdc->TextOut(x+2, rect->top+2, "+");
	pdc->TextOut(x+nx/2-sz.cx/2, rect->bottom-ny/2-sz.cy/2, "-");

  // Done!!
}  // End CBand::Head()


/* Method to draw the "block" section of the header.  The "block"
   is drawn various colors to indicate some sort of error or block
   on the channel, that is preventing data acquisition.
   RED:     !!!!Inidicates serious error with the tank!!!!!
   BLUE:    Indicates that data was requested to the right of the tank
   GREEN:   Indicates that no data was available for the channel,
             or an error occurred while retrieving data.
   PURPLE:  Unknown error state!
   The only color that should be seen during normal operation (other
   than clear, is BLUE.  Any other color probably indicates an error
   with the tank within the wave_server.)
*************************************************/
void CBand::DrawBlock(CDC * pDC, CDisplay *pDisplay, RECT *pRect)
{
	CSize sz = pDC->GetTextExtent("+");
	int nx = sz.cx + 4;
	int ny = sz.cy + 4;
	int x = pDisplay->GetHeaderWidth() - nx;

  RECT r;
  r.left = x+1;
  r.right = x+nx-1;
  r.top = pRect->top+ny+1;
  r.bottom = pRect->bottom-ny;
  COLORREF crColor;

  switch(this->pTrace->iBlock)
  {
  case TRACE_BLOCK_UNBLOCK:
  case TRACE_BLOCK_WAITING_FOR_QUEUE:
    crColor = GetSysColor(COLOR_WINDOW);
    break;
  case TRACE_BLOCK_LEFT_OF_TANK:
    crColor = 0x000000FF;  // Red
    break;
  case TRACE_BLOCK_RIGHT_OF_TANK:
    crColor = 0x00FF0000;  // Blue
    break;
  case TRACE_BLOCK_NO_DATA_AVAILABLE:
  case TRACE_BLOCK_ERROR_GETTING_DATA:
    crColor = 0x0000FF00;  // Green
    break;
  default:
    crColor = 0x00FF00FF;  // Purple
    break;
  }

  pDC->FillSolidRect(&r, crColor);
} // end CBand::DrawBlock()


/* Method to draw the band, including, header, frame, trace, and "block state".
*************************************************/
BOOL CBand::DrawBand(CDC *pDC, CDisplay *pDisplay, RECT *pRect,
                     double tstart, double tstop, BOOL bDrawHeader,
                     BOOL bForceRedraw)
{
  // Draw the frame and header if requested to do so
  if(bDrawHeader)
  {
 		Frame(pDC, pDisplay, pRect);
		Head(pDC, pDisplay, pRect);
  }

  // Draw the block state if applicable
  if(this->pTrace)
  {
    if(this->pTrace->bBlockStateChanged)
    {
      DrawBlock(pDC, pDisplay, pRect);
      this->pTrace->bBlockStateChanged = FALSE;
    }
  }

  // Draw the trace
  return(DrawTrace(pDC, pDisplay, pRect, tstart, tstop, bForceRedraw));
}  // end CBand::DrawBand()


/* Method to draw the trace for the band.
   Returns TRUE if calibration performed.
************************************************************************/
BOOL CBand::DrawTrace(CDC *pdc, CDisplay *dsp, RECT *rect,
				  double tstart, double tstop, BOOL bForceRedraw) 
{
	long j;
	long ibuf, ibuf2;
	long nbuf;
	int x;
	int y;
	int ytop;
	int y0;
	int ybot;
	long *buf;
	CTrace *tr;
	double t;
	double t1;
	double t2;
	BOOL bRetCode = FALSE;
  int ystart, FDPx, FDPy;
  BOOL bLastPointOnDisplay;

  // DK 20000525  Added to prevent the drawing of fill values.
	BOOL bLastDatapointIsValid = TRUE;  

	tr = this->pTrace;

  // Check the Band state
  // If Bogus, then nothing for us to do.  Go Home!
  if(this->nState == BAND_BOGUS)
    return(FALSE);

  // If !Active, then we may have to do some setup
  if(this->nState != BAND_ACTIVE)
  {
    if(this->nState == BAND_INIT)
    {
      // If were are in INIT state, and we finally got
      // some trace, then calibrate the channel and move on
      // to setup.
      if(tr->dStart < tr->dStop)
      {
        Calibrate(dsp);
        this->nState = BAND_SETUP;
        bRetCode = TRUE;
      }
    }
    else  // BAND_SETUP
    {
      // If we are in SETUP and we have received more than 60
      // seconds worth of trace (or atleast trace over a 60 
      // second time interval) then do a final calibration and
      // move to ACTIVE.
      if(tr->dStart + 60 > tr->dStop)
      {
        this->nState = BAND_ACTIVE;
        bForceRedraw = TRUE;
        Calibrate(dsp);
        bRetCode = TRUE;
      }
    }
  }

  // If we are not forced to redraw, then only
  // draw if we have received new trace in since the
  // last time we were called.
  if(!bForceRedraw)
  {
    if(tr->bNewTraceRecvd)
    {
      // only draw the new portion
      tstart = tr->dNewTraceStart;
      tstop  = tr->dNewTraceEnd;
    }
    else
    {
      return(FALSE);
    }
  }

  // Calculate vertical values for Top, Bottom, and middle.
  ytop = rect->top;
	ybot = rect->bottom;
	y0 = (ytop + ybot) / 2;

  // We are given a start and an endtime, but we don't trust them,
  // they will only provide for the drawing of our own little segment,
  // but we also need to connect that segment to the rest of the record.
  // So try to grab an extra sample on both sides of our segment.
  if(tstart == 0)
    t1 = tstart;
  else
    t1 = tstart - tr->dStep;

	t2 = tstop + (tr->dStep);

  // Make sure our request fits inside the display, give or take a sample. 
  if(t1 <= (tstart - tr->dStep))
		t1 = tstart - 0.99 * tr->dStep;
	if(t2 >= (tstop + tr->dStep))
		t2 = tstop + 0.99 * tr->dStep;

  // We can't draw what we don't have, so use the trace limits as hard limits.
	if(t1 < tr->dStart)
		t1 = tr->dStart;
	if(t2 > tr->dStop)
		t2 = tr->dStop;


  // After applying constraints, our stop time is prior to our start.  Abort!
  if(t2 < t1)
		return bRetCode;

  // Get the local cache index for the start and end times
	ibuf = tr->Index(t1);
  ibuf2 = tr->Index(t2);

  // The number of buffers to check is essentially 
  // index(finish) - index(start) + 1, because we
  // must handle both the start and the finish, as
  // well as all of the buffers in between.
  nbuf = ibuf2 - ibuf + 1;
  if(nbuf < 0)
    nbuf += tr->nBuf;

  // Sanity check
	if(nbuf > tr->nBuf) 
  {
		TRACE("CBand::Trace: nbuf = %d, reset to %d\n", nbuf, tr->nBuf);
		nbuf = tr->nBuf;
	}

  // Get the time of the starting buffer 
	t = tr->T(ibuf);

  // Lock the local cache
	buf = (long *)GlobalLock(tr->hBuf);

  // Loop through the datapoints to draw
  for(j=0; j<nbuf; j++) 
	{
    // Calculate the screen coordinates of the current datapoint
		x = dsp->GetHeaderWidth() + (int)((t - dsp->GetStartTime()) * dsp->GetPixelsPerSecond());
		y = (int)(y0 - (y0 - ytop) * (buf[ibuf]-nBias)/nScale);
		if(y < ytop)
			y = ytop;
		if(y > ybot)
			y = ybot;

    /* What follows is a lot chicken scratch to figure out what we should
       draw (a point, a line, or a nothing), based on the validity and
       location of the last datapoint and the current datapoint.
    **********************************************************************/
    // If this is the first sample in this group
		if(j == 0) 
		{
        bLastDatapointIsValid = FALSE;
        bLastPointOnDisplay   = FALSE;
    }
    /******** BEGIN FUNCTIONAL CHANGE DK 20000525**********/
    
    /* Change so that fill values would not be drawn as valid
    data points.  Added a variable bLastDatapointIsValid,
    that tracks whether the last data point was valid data
    or a fill value.  If the last point was valid, then it
    draws a line from the last point to the current point.
    If the last point was a fill value it does not draw
    a line between the fill value and the current data point.
    If the current data point is a fill value, then it sets
    bLastDatapointIsValid = FALSE.
    **********************************************************/
    if(buf[ibuf] == FILL_NUMBER || buf[ibuf] == UNTOUCHED_FILL_NUMBER)
    {
      // This is a fill value, so record that it is not valid.
      bLastDatapointIsValid = FALSE;
    }
    else  // the current datapoint is valid
    {
      if(x < dsp->GetHeaderWidth())  // if the current point is left of screen
      {
        // record vitals
        FDPx = x;
        FDPy = y;
        bLastPointOnDisplay = FALSE;
        bLastDatapointIsValid = TRUE;
      }
      else  // current point is on the screen
      {
        if(bLastDatapointIsValid)
        {
          if(!bLastPointOnDisplay)
          {
            // interpolate samples to calc the value at start of display
            ystart = FDPy + ((y - FDPy)*(dsp->GetHeaderWidth() - FDPx)/(x - FDPx));
            pdc->MoveTo(dsp->GetHeaderWidth(), ystart);

            bLastPointOnDisplay = TRUE;
          }
          // here's the meat
          pdc->LineTo(x, y);
          //TRACE("DT: buf(%d), [%d,%d]\n",ibuf,x,y); 
        }  
        else  // !bLastDataPointIsValid
        {
					pdc->MoveTo(x, y);
          bLastDatapointIsValid = TRUE;
          if(!bLastPointOnDisplay)
            bLastPointOnDisplay = TRUE;
        }
      }  // end else (current point is on screen)
    }  // end else (current point is valid)

		ibuf++;
		if(ibuf >= tr->nBuf)
			ibuf = 0;
		t += tr->dStep;
  }   // End for j < nBuf

  // Release the global buffer
	GlobalUnlock(tr->hBuf);

	// Plot trigger times as vertical red bars
	if(dTrigger > t1 && dTrigger < t2) 
  {
		x = dsp->GetHeaderWidth() + 
        (int)((dTrigger - dsp->GetHeaderWidth()) * dsp->GetPixelsPerSecond());
		CPen *penNew = new CPen();
		penNew->CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
		CPen *penOld = pdc->SelectObject(penNew);
		pdc->MoveTo(x, ytop);
		pdc->LineTo(x, ybot);
		pdc->SelectObject(penOld);
		delete penNew;
	}

	// Plot menu bounds as blue bars (start of tank)
	if(tr->Site.dStart > dsp->GetStartTime() && tr->Site.dStart < dsp->GetEndTime()) 
  {
		x = dsp->GetHeaderWidth() + 
        (int)((tr->Site.dStart - dsp->GetStartTime()) * dsp->GetPixelsPerSecond());
		CPen *penNew = new CPen();
		penNew->CreatePen(PS_SOLID, 0, RGB(0, 0, 255));
		CPen *penOld = pdc->SelectObject(penNew);
		pdc->MoveTo(x, ytop);
		pdc->LineTo(x, ybot);
		pdc->SelectObject(penOld);
		delete penNew;
	}

	// Plot menu bounds as blue bars (end of tank)
	if(tr->Site.dStop > dsp->GetStartTime() && tr->Site.dStop < dsp->GetEndTime()) {
		x = dsp->GetHeaderWidth() + (int)((tr->Site.dStop - dsp->GetStartTime()) * dsp->GetPixelsPerSecond());
		CPen *penNew = new CPen();
		penNew->CreatePen(PS_SOLID, 0, RGB(0, 0, 255));
		CPen *penOld = pdc->SelectObject(penNew);
		pdc->MoveTo(x, ytop);
		pdc->LineTo(x, ybot);
		pdc->SelectObject(penOld);
		delete penNew;
	}

  // reset the NewTraceRecvd flag, that indicates that trace has
  // been received since the last drawing of the channel.
  tr->bNewTraceRecvd = FALSE;
  return(bRetCode);
}  // end CBand::DrawTrace()


/* Method to tell the CTrace object what data it should
   keep in the local cache.
************************************************************************/
BOOL CBand::CheckForNeededTrace(double dStart, double dEnd)
{
  if(pTrace)
    return(this->pTrace->CheckForNeededTrace(dStart,dEnd));
  else
    return(FALSE);
}

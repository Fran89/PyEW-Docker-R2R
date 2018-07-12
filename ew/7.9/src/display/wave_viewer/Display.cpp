// Display.cpp : implementation file
//

#include "stdafx.h"
#include "surf.h"
#include "Display.h"
#include "surfview.h"
#include "surfdoc.h"  // include for access to m_mutex

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DISPLAY_DRAWING_DELAY_FUDGE_FACTOR 10
/////////////////////////////////////////////////////////////////////////////
// CDisplay

IMPLEMENT_DYNCREATE(CDisplay, CCmdTarget)

CDisplay::CDisplay()
{
	dStart = 0.0;
  dDuration   = 100.0;
	//nHeaderWidth = 75;  // DK 2004/06/03 changed from 60
	nHeaderWidth = 100;  // DK 2006/10/26 changed from 75
  rDisplay.left = 0;
  rDisplay.right = 1028;
  rDisplay.top = 0;
  rDisplay.bottom = 700;
	dPixsPerSec = (rDisplay.right-nHeaderWidth)/dDuration;

	nBandHeight = 100;
	iBand1 = 0;
	iNumBands = 0;
  pView = NULL;
  parrTrace = NULL;
  //Initialize the Band Array

  bSomethingHappened = TRUE;
  bHumanScale = FALSE;
  dDelayFudgeFactor = DISPLAY_DRAWING_DELAY_FUDGE_FACTOR;
}

CDisplay::CDisplay(CSurfView * pView)
{
	dStart = 0.0;
  dDuration   = 24.0;
	//nHeaderWidth = 75;  // DK 2004/06/03 changed from 60
	nHeaderWidth = 100;  // DK 2006/10/26 changed from 75
  if(!pView)
    TRACE("Bad View in call to CDisplay Constructor!!!!\n");


  // Set some defaults, since we can't call GetClientRect() yet
  rDisplay.left = 0;
  rDisplay.right = 1028;
  rDisplay.top = 0;
  rDisplay.bottom = 700;

	dPixsPerSec = (rDisplay.right-nHeaderWidth)/dDuration;

	nBandHeight = 100;
	iBand1 = 0;
	iNumBands = 0;
  parrTrace = NULL;
  bHumanScale = FALSE;
  dDelayFudgeFactor = DISPLAY_DRAWING_DELAY_FUDGE_FACTOR;

  //Band Array Initialization is done in nlSetTraceArray()

  bSomethingHappened = TRUE;
  this->pView = pView;
  psLock = NULL;
}

CDisplay::~CDisplay()
{
  delete(psLock);
}


void CDisplay::ChangeDisplaySize(void)
{
  pView->GetClientRect(&this->rDisplay);
  if(iNumBands)
    nBandHeight = (rDisplay.bottom-rDisplay.top)/iNumBands;
  if(dDuration)
  dPixsPerSec = (rDisplay.right - rDisplay.left - nHeaderWidth)/dDuration;
}

void CDisplay::nlInvalidateAllBands(void)
{
  CBand * pBand;
  int i;

  // Clear out the old bands
  for(i=0;i < arrBand.GetSize(); i++)
  {
    pBand = (CBand *)arrBand.GetAt(i);
    delete(pBand);
  }
  arrBand.RemoveAll();

  if(!parrTrace)
    return;  // Nothing to do, go home early

  // Build the new ones
  for(i=0;i < iNumBands; i++)
  {
    if(parrTrace)
    {
      pBand = new CBand((CTrace *)parrTrace->GetAt(iBand1+i));  
      if(iBand1+i >= arrBand.GetSize()) // DK CLEANUP
        TRACE("InvalidateAllBands:Something!\n");
      if(this->bHumanScale)
        pBand->nScale = this->iScale;
      pBand->Calibrate(this);
    }
    else
    {
      pBand = new CBand;
    }
    arrBand.Add(pBand);
  }

  if(pView)
  {
    CDC * pDC = pView->GetDC();
    pView->Invalidate(TRUE);
    nlDrawDisplay(pDC,TRUE);
    pView->ReleaseDC(pDC);
    SomethingHappened();
  }
}

double CDisplay::GetStartTime()  // Get the starttime of the display (seconds since 1970)
{
  return(this->dStart);
}


double CDisplay::GetEndTime()    // Get the endtime   of the display (seconds since 1970)
{
  return(this->dStart + this->dDuration);
}

void CDisplay::SetStartTime(double dStartTime) // Set the starttime of the display (seconds since 1970)
{
  psLock->Lock();
  TRACE("Locked by SetStartTime\n");
  nlSetStartTime(dStartTime);
  psLock->Unlock();
  TRACE("UnLocked by SetStartTime\n");
}

void CDisplay::nlSetStartTime(double dStartTime) // Set the starttime of the display (seconds since 1970)
{
  if(dStartTime >= 0)
  {
    this->dStart = dStartTime;
  }
  else if(parrTrace)
  {
    if(dStartTime == DISPLAY_TIME_TANK_START)
    {
      if(iBand1 >= parrTrace->GetSize()) 
        TRACE("Something 1!\n");
      this->dStart = ((CTrace *)(parrTrace->GetAt(this->iBand1)))->GetLeftOfTank();
    }
    else if(dStartTime == DISPLAY_TIME_TANK_END)
    {
      if(iBand1 >= parrTrace->GetSize()) 
        TRACE("Something 2!\n");
      this->dStart = ((CTrace *)(parrTrace->GetAt(this->iBand1)))->GetRightOfTank()
                      - this->dDuration
                      - dDelayFudgeFactor;  // DK 011602 making variable 
      double dSampRate;
      if(dSampRate = ((CTrace *)(parrTrace->GetAt(this->iBand1)))->GetSampleRate())
        this->dStart += 1.0/dSampRate;
    }
    else
    {
      // we're not expecting anything else.  Ignore
      return;
    }
  }
  nlResetBandBlocks();
  SomethingHappened();
  nlReDrawAllBands();  // This should probably just be a draw!! DK CLEANUP
}  // end SetStartTime()

void CDisplay::SetEndTime(double dEndTime)     // Set the endtime of the display (seconds since 1970)
{
  TRACE("Locked by SetEndTime\n");
  psLock->Lock();

  if(dEndTime > this->dStart)
  {
    this->dDuration = dEndTime - this->dStart;
    SomethingHappened();
    nlReDrawAllBands();
  }
  TRACE("UnLocked by SetEndTime\n");
  psLock->Unlock();
}

void CDisplay::SetDuration(double dDuration)   // Set the duration of the display (seconds)
{
  psLock->Lock();
  TRACE("Locked by SetDuration\n");
  if(dDuration > 0)
  {
    this->dDuration = dDuration;
    SomethingHappened();
    nlReDrawAllBands();
  }
  psLock->Unlock();
  TRACE("UnLocked by SetDuration\n");
}

int  CDisplay::GetBandHeight(void)  // Get the height of each band in Pixels.
{
  return(this->nBandHeight);
}

void CDisplay::SetBandHeight(int nBandHeight)  // Set the height of each band in Pixels.
{
  if(nBandHeight > 10)
  {
    if(psLock)
    {
      psLock->Lock();
  TRACE("Locked by SetBandHeight\n");
    }
    this->nBandHeight = nBandHeight;
    nlSetNumberOfBands((rDisplay.bottom - rDisplay.top)/nBandHeight);
    if(psLock)
    {
      psLock->Unlock();
  TRACE("UnLocked by SetBandHeight\n");
    }
  }
}

int  CDisplay::GetHeaderWidth(void)    // Get the Width of the header for the bands
{
  return(this->nHeaderWidth);
}

void CDisplay::nlSetHeaderWidth(int nHeaderWidth) // Set the Width of the header for the bands
{
  if(nHeaderWidth > 10)
  {
    this->nHeaderWidth = nHeaderWidth;
    nlReDrawAllBands();
  }
}

double CDisplay::GetPixelsPerSecond() // Get the pixels/time ratio for the display
{
  return(this->dPixsPerSec);
}

void   CDisplay::DrawDisplay(CDC *pdc, BOOL bForceRedraw)
{
  // DK Cleanup
  //if(bForceRedraw)
    //Sleep(50);
  if(!parrTrace)
    return;
  psDDLock->Lock();
  TRACE("Locked by DrawDisplay\n");
  nlDrawDisplay(pdc, bForceRedraw);
  psDDLock->Unlock();
  TRACE("UnLocked by DrawDisplay\n");
}

void   CDisplay::nlReDrawAllBands()
{
  if(pView)
  {
    CDC * pDC = pView->GetDC();
    pView->Invalidate(TRUE);
    nlDrawDisplay(pDC, TRUE);
    pView->ReleaseDC(pDC);
  }
}

void   CDisplay::nlReDrawBand(int iBand)
{
  if(pView)
  {
    RECT r;
    CDC * pDC = pView->GetDC();
    GetBandRect(iBand,&r);
    pView->InvalidateRect(&r,TRUE);
    ((CBand *)arrBand.GetAt(iBand))->DrawBand(pDC, this, &r, this->dStart, this->dDuration, TRUE);
    pView->ReleaseDC(pDC);
  }
}

void CDisplay::nlDrawDisplay(CDC *pDC, BOOL bForceRedraw)
{
  RECT rTemp=rDisplay;
  CBand * band;

 //  pView->GetClientRect(&this->rDisplay);

 	pDC->MoveTo(nHeaderWidth-1, 0);
	pDC->LineTo(nHeaderWidth-1, rTemp.bottom);
  if(iNumBands != arrBand.GetSize())
    return;

  if(!parrTrace)
    return;  //Nothing to draw, take the rest of the day off.
  
	for(int i=0; i<iNumBands; i++) 
  {
		//TRACE("Surf::OnDraw: Band = %d\n", i);
      if(i >= arrBand.GetSize()) // DK CLEANUP
        TRACE("Something!\n");
		band = (CBand *)arrBand.GetAt(i);
    rTemp.top = i * nBandHeight;
    rTemp.bottom = (i+1) * nBandHeight;
    if(band->DrawBand(pDC, this, &rTemp, dStart, dStart+dDuration, 
                      TRUE, bForceRedraw))
    {
      // We had a recalibration, so we need to clear this band and redraw
      // it to make sure that we got rid of any old datapoints drawn on the
      // screen based on the old calibration.
      if(pView)
      {
        pView->InvalidateRect(&rTemp);  // Erase the previous lines
        // Now draw the new ones (again)
        band->DrawBand(pDC, this, &rTemp, dStart, dStart+dDuration, TRUE); 
      }
    }
	}
  this->SomethingHappened();
}

void   CDisplay::SomethingHappened(void)
{
  bSomethingHappened = TRUE;
  if(parrTrace)
    for(int i=0; i< parrTrace->GetSize(); i++)
    {
      ((CTrace *)parrTrace->GetAt(i))->bCheckAgain = TRUE;
    }
}

BOOL   CDisplay::Yes_SomethingDidHappen(void)
{
  return(bSomethingHappened);
}

double   CDisplay::CalcShiftStartTime(double dNumSeconds)
{
  // round time to the nearest pixel  
  if(dNumSeconds > 0)
    return(((int)(dNumSeconds * dPixsPerSec + 0.5))/dPixsPerSec);
  else if(dNumSeconds < 0)
    return(((int)(dNumSeconds * dPixsPerSec - 0.5))/dPixsPerSec);
  else  //dNumSeconds == 0
    return(0.0);
}

double CDisplay::ShiftStartTime(double dNumSeconds)
{
  double t1,t2;
  CBand * pBand;
  RECT rTemp = rDisplay;
  double dTempNumSeconds;
    
  // DK Cleanup   doesn't this function need a lock!!?!!
  //dTempNumSeconds = dNumSeconds;

  // round time to the nearest pixel  
  if(dNumSeconds > 0)
    dTempNumSeconds = ((int)(dNumSeconds * dPixsPerSec + 0.5))/dPixsPerSec;
  else if(dNumSeconds < 0)
    dTempNumSeconds = ((int)(dNumSeconds * dPixsPerSec - 0.5))/dPixsPerSec;
  else  //dNumSeconds == 0
    return(0.0);

  if(dNumSeconds != dTempNumSeconds)
  {
    TRACE("We got trouble in river city!\n");
  }


  this->dStart += dTempNumSeconds;
  
  if(pView)
  {
    CDC * pDC = pView->GetDC();
    
    if(dNumSeconds > 0)
    {
      t1 = dStart + dDuration - dTempNumSeconds;
      t2 = dStart + dDuration;
    }
    else
    {
      t1 = dStart;
      t2 = dStart  - dTempNumSeconds;
    }
    
    for(int i=0; i<iNumBands; i++) 
    {
      if(i >= arrBand.GetSize()) // DK CLEANUP
        TRACE("Something!\n");
      pBand = (CBand *)arrBand.GetAt(i);
      rTemp.top = i * nBandHeight;
      rTemp.bottom = (i+1) * nBandHeight;
      if(pBand->DrawBand(pDC, this, &rTemp, t1, t2, FALSE))
      {
        // We had a recalibration, so we need to clear this band and redraw
        // it to make sure that we got rid of any old datapoints drawn on the
        // screen based on the old calibration.
        if(pView)
        {
          pView->InvalidateRect(&rTemp);  // Erase the previous lines
          // Now draw the new ones (again)
          pBand->DrawBand(pDC, this, &rTemp, t1, t2, FALSE); 
        }
      }
    }
    pView->ReleaseDC(pDC);
  }

  SomethingHappened();
//  TRACE("SST: Shifting %.2f secs  (%d)pixels\n",
//        dTempNumSeconds, (int)(dPixsPerSec * dTempNumSeconds)); 

  return(dTempNumSeconds);
}


int    CDisplay::ScrollBands(int iNumBands)
{
  int iBandsScrolled = iNumBands;
  int iCopyBand1, iCopyBand2;
  int i;

  psLock->Lock();
  TRACE("Locked by ScrollBands\n");
  if(this->iBand1 + iBandsScrolled + this->iNumBands > parrTrace->GetSize())
  {
    iBandsScrolled = parrTrace->GetSize() - this->iBand1 - this->iNumBands;
  }
  else if(this->iBand1 + iBandsScrolled < 0)
  {
    iBandsScrolled = 0 - this->iBand1;
  }

  if(iBandsScrolled > this->iNumBands || iBandsScrolled < 0 - this->iNumBands)
  {
    // We are scrolling completely off the page.
    // Set the new starting Band
    this->iBand1 = iBand1 + iBandsScrolled;

    // For all the remaining bands: reinitialize them!
    for(i=0; i < this->iNumBands; i++)
      nlInvalidateBand(i);
  }
  else  // We are scrolling less than a page, so take care of the overlap
  {
    if(iBandsScrolled > 0)  //Scrolling Down
    {
      iCopyBand1=iBandsScrolled;  // the first band I will copy from
      iCopyBand2=this->iNumBands - 1;  // the last band I will copy from
      // Copy the bands that were on the screen and still will be after the scroll
      for(i=iCopyBand1; i<=iCopyBand2; i++)
      {
      if(i >= arrBand.GetSize()) // DK CLEANUP
        TRACE("Something!\n");

      if(i-iBandsScrolled >= arrBand.GetSize()) // DK CLEANUP
        TRACE("Something!\n");
      if(i >= arrBand.GetSize()) // DK CLEANUP
        TRACE("Something!\n");
        *(CBand *)(arrBand.GetAt(i-iBandsScrolled)) = *(CBand *)(arrBand.GetAt(i));
      }
      
      // Set the new starting Band
      this->iBand1 = iBand1 + iBandsScrolled;
      
      // For all the remaining bands: reinitialize them!
      for(i=iCopyBand2-iBandsScrolled+1; i < this->iNumBands; i++)
        nlInvalidateBand(i);
    }
    else  // Scrolling Up  (iBandsScrolled < 0 )
    {
      iCopyBand1 = 0;  // the last band I will copy from
      iCopyBand2 = iBandsScrolled + this->iNumBands - 1;  // the first band I will copy from
      for(i=iCopyBand2; i>=iCopyBand1; i--)
      {
        *(CBand *)(arrBand.GetAt(i-iBandsScrolled)) = *(CBand *)(arrBand.GetAt(i));
      if(i >= arrBand.GetSize()) // DK CLEANUP
        TRACE("Something!\n");
      }
      
      // Set the new starting Band
      this->iBand1 = iBand1 + iBandsScrolled;
      
      // For all the remaining bands: reinitialize them!
      for(i=0; i < 0-iBandsScrolled; i++)
        nlInvalidateBand(i);
    }
  }  // end else (if not full page scroll)
    
  nlReDrawAllBands();
  psLock->Unlock();
  TRACE("UnLocked by ScrollBands\n");
  return(iBandsScrolled);
}

int    CDisplay::GetTopBand(void)
{
  return(iBand1);
}

int    CDisplay::GetNumberOfBands(void)
{
  return(this->iNumBands);
}

BOOL   CDisplay::GetInfoForBand(int iBand, int * piTrace, int * pnScale, int * pnBias)
{
  CBand * pBand;

  if(iBand >= this->iNumBands)
    return(FALSE);

  pBand = (CBand *)arrBand.GetAt(iBand);
  *piTrace = iBand + this->iBand1;
  *pnScale = pBand->nScale;
  *pnBias  = pBand->nBias;
  return(TRUE);
}

void CDisplay::GetBandRect(int iBand, RECT * pRect)
{
  if(iBand >= iNumBands)
  {
    memset(pRect,0,sizeof(RECT));
    return;
  }

  pRect->left  = rDisplay.left;
  pRect->right = rDisplay.right;
  pRect->top   = rDisplay.top + iBand * nBandHeight;
  pRect->bottom= rDisplay.top + (iBand+1) * nBandHeight;

}

void CDisplay::nlInvalidateBand(int iBand)
{
  RECT rTemp=rDisplay;
  CBand * pBand;


  pBand = (CBand *)arrBand.GetAt(iBand);
  delete(pBand);
  if(parrTrace)
  {
    pBand = new CBand((CTrace *)parrTrace->GetAt(iBand1 + iBand));
    if(this->bHumanScale)
      pBand->nScale = this->iScale;
    pBand->Calibrate(this);
  }
  else
  {
    pBand = new CBand;
  }

  arrBand.SetAt(iBand,pBand);

  if(pView)
  {
    CDC * pDC = pView->GetDC();
    pBand = (CBand *)arrBand.GetAt(iBand);
    rTemp.top = iBand * nBandHeight;
    rTemp.bottom = (iBand+1) * nBandHeight;
    pBand->DrawBand(pDC, this, &rTemp, dStart, dStart+dDuration, TRUE);
    pView->ReleaseDC(pDC);
    SomethingHappened();
  }
}

void   CDisplay::AdjustGain(int iBand, float dGainMultiplier)
{

  if(dGainMultiplier <= 0)
    return;

  if(iBand < -1 || iBand >= this->iNumBands)
    return;

  TRACE("Locked by AdjustGain\n");
  psLock->Lock();

  SomethingHappened();

  if(iBand == -1)
  {
    for(int i=0; i<this->iNumBands; i++)
    {
      ((CBand *)arrBand.GetAt(i))->nScale = 
          (int) ( ((CBand *)arrBand.GetAt(i))->nScale/dGainMultiplier + 0.5);
      // DK Cleanup  nScale should not be public
      if(((CBand *)arrBand.GetAt(i))->nScale <= 0 )
        ((CBand *)arrBand.GetAt(i))->nScale = 1;
    }
      nlReDrawAllBands();
  }
  else
  {
      ((CBand *)arrBand.GetAt(iBand))->nScale = 
          (int) ( ((CBand *)arrBand.GetAt(iBand))->nScale/dGainMultiplier + 0.5);
      if(((CBand *)arrBand.GetAt(iBand))->nScale <= 0 )
        ((CBand *)arrBand.GetAt(iBand))->nScale = 1;
      nlReDrawBand(iBand);
  }
  psLock->Unlock();
  TRACE("UnLocked by AdjustGain\n");
}  // end AdjustGain()


int    CDisplay::AdjustNumberOfBands(int nAdjustment)
{
  int iBandAdjustment;
  int iNewNumBands = this->iNumBands + nAdjustment;
  int nFinalAdjustment;
  int iOldNumBands;
  int i;
  int iScrollAdjustment;

  psLock->Lock();
  TRACE("Locked by AdjustNumberOfBands\n");

  SomethingHappened();

  if(iNewNumBands <= MAX_NUM_BANDS &&  iNewNumBands > 0)
  {
    if(!parrTrace)
    {
      SetNumberOfBands_internal(iNewNumBands);
      nlInvalidateAllBands();
      nFinalAdjustment = nAdjustment;
      goto AdjustNumberOfBands_End;
    }
    else  // we actually have trace, so this is meaningful
    {
      if(nAdjustment > 0)
      {
        if(this->iNumBands == parrTrace->GetSize())
        {
          nFinalAdjustment = 0;
          goto AdjustNumberOfBands_End;
        }
        
        // Check to see if they are requesting more bands than 
        // we have traces, or if we need to scroll band1
        // so that we don't run off the end of the trace array
        if(iNewNumBands > parrTrace->GetSize())
        {
          iBandAdjustment = parrTrace->GetSize() - this->iNumBands;
          iScrollAdjustment = this->iBand1;
          this->iBand1 = 0;
          iNewNumBands = parrTrace->GetSize();
        }
        else if(iNewNumBands > parrTrace->GetSize() - iBand1)
        {
          iBandAdjustment = nAdjustment;
          iScrollAdjustment = iNewNumBands - (parrTrace->GetSize() - iBand1);
          this->iBand1 -= iScrollAdjustment;
        }
        else
        {
          iBandAdjustment = nAdjustment;
          iScrollAdjustment = 0;
        }
        
        iOldNumBands = this->iNumBands;
        SetNumberOfBands_internal(iNewNumBands);
        
        // Add the neccessary bands
        for(i=iOldNumBands; i < iNewNumBands; i++)
        {
          arrBand.Add(new CBand());
        }
        
        // Copy the scrolled bands to new band location
        if(iScrollAdjustment)
        {
          for(i=iOldNumBands - 1; i>=0; i--)
          {
            *(CBand *)(arrBand.GetAt(i+1)) = *(CBand *)(arrBand.GetAt(i));
          }
        }
        
        // Setup the completely new bands
        for(i=0; i < iScrollAdjustment; i++)
        {
          nlInvalidateBand(i);
        }
        for(i=iOldNumBands + iScrollAdjustment; i < iNewNumBands; i++)
        {
          nlInvalidateBand(i);
        }
        nFinalAdjustment = iBandAdjustment;
        goto AdjustNumberOfBands_End;
      }
      else  // if nAdjustment > 0
      {
        int iNewNumBands = this->iNumBands + nAdjustment;
        int i;

        for(i=this->iNumBands - 1; i >= iNewNumBands; i--)
        {
          delete((CBand *)(arrBand.GetAt(i)));
          arrBand.RemoveAt(i);
        }
        SetNumberOfBands_internal(iNewNumBands);
        nFinalAdjustment = nAdjustment;
        goto AdjustNumberOfBands_End;
      }
    }
  }
  else
  {
    nFinalAdjustment = 0;
    goto AdjustNumberOfBands_End;
  }

AdjustNumberOfBands_End:
  if(nFinalAdjustment)
  {
    this->nBandHeight = (this->rDisplay.bottom - this->rDisplay.top) /
                         this->iNumBands;
  }

  psLock->Unlock();
  TRACE("UnLocked by AdjustNumberOfBands\n");
  return(nFinalAdjustment);
}


void   CDisplay::SetScale(int iScale)
{
  this->iScale = iScale;
}

void   CDisplay::SetbHumanScale(void)
{
  this->bHumanScale = TRUE;
}

BOOL   CDisplay::GetbHumanScale(void)
{
  return(this->bHumanScale);
}


void CDisplay::SetNumberOfBands_internal(int iNumBands)
{
  int iNumTraces;

  this->iNumBands = iNumBands;

  if(!pView)
    return;

  if(parrTrace)
    iNumTraces = parrTrace->GetSize();
  else
    iNumTraces = 0;

  if(iNumBands >= iNumTraces || iNumBands <= 0 || iNumTraces <= 0) 
  {
    pView->ShowScrollBar(SB_VERT, FALSE);	// Disappear the vertical scroll bar
  } 
  else 
  {
    SCROLLINFO scInfo;
    pView->GetScrollInfo(SB_VERT,&scInfo);
    scInfo.nPage = iNumBands;
    pView->SetScrollInfo(SB_VERT,&scInfo);
    pView->SetScrollRange(SB_VERT, 0, iNumTraces -  1);  // DK Cleanup - iNumBands
    pView->SetScrollPos(SB_VERT, this->iBand1, TRUE);
    pView->ShowScrollBar(SB_VERT, TRUE);	// Appear the vertical scroll bar
  }

}
int    CDisplay::nlSetNumberOfBands(int iNumBands)
{
  int iNewNumBands;

  if(iNumBands <= MAX_NUM_BANDS &&
     iNumBands > 0)
  {
    SomethingHappened();
    iNewNumBands = iNumBands;
    if(parrTrace)
    {
      // Check to see if we need to adjust band1
      // so that we don't run off the end of the trace array
      if(parrTrace->GetSize() - iNewNumBands< iBand1)
      {
        if(parrTrace->GetSize() - iNewNumBands < 0)
        {
          iBand1 = 0;
          iNewNumBands = parrTrace->GetSize();
        }
        else
        {
          iBand1 = parrTrace->GetSize() - iNewNumBands;
        }
      }
    }
    else
    {
      //iNewNumBands = 0;
      //this->iBand1 = 0;
    }

    SetNumberOfBands_internal(iNewNumBands);
    if(this->iNumBands)
      this->nBandHeight = (this->rDisplay.bottom - this->rDisplay.top) / this->iNumBands;
    nlInvalidateAllBands();
    return(this->iNumBands);
  }
  return(this->iNumBands);
}

void   CDisplay::SetScreenDelay(double dDelayTime)
{
  dDelayFudgeFactor = dDelayTime;
}

void   CDisplay::nlSetTraceArray(CObArray * parrTrace)
{
  if(!psLock)
  {
    psLock = new CSingleLock(&(pView->GetDocument()->m_mutex));
    psDDLock = new CSingleLock(&(pView->GetDocument()->m_mutex));
  }
  this->parrTrace = parrTrace;
  if(parrTrace)
    if(parrTrace->GetSize()  <= 0)
      TRACE("TROUBLE!\n");  // DK Cleanup
    else

  TRACE("parrTrace set to %u\n",parrTrace);
  nlSetNumberOfBands(this->iNumBands);
  TRACE("NumberOfBands  set to %u\n",this->iNumBands);

  if(parrTrace)
    TRACE("Number of Traces %d\n", parrTrace->GetSize());

  if(parrTrace)
   if(!this->dStart)
    nlSetStartTime(DISPLAY_TIME_TANK_END);
}

void    CDisplay::RecalibrateBand(int iBand)
{
  psLock->Lock();
  TRACE("Locked by RecalibrateBand\n");
  if(iBand >= 0 && iBand < MAX_NUM_BANDS)
  {
    ((CBand *)arrBand.GetAt(iBand))->Calibrate(this);
  }
  else if(iBand == -1)
  {
    for(int i=0; i < arrBand.GetSize(); i++)
      ((CBand *)arrBand.GetAt(i))->Calibrate(this);
  }
  psLock->Unlock();
  TRACE("UnLocked by RecalibrateBand\n");
}

void    CDisplay::AdjustDuration(float dTimeMultiple)
{
  if(dTimeMultiple > 0)
  {
  TRACE("Locked by AdjustDuration\n");
    psLock->Lock();
    this->dDuration *= dTimeMultiple;
    this->dPixsPerSec = (rDisplay.right - (rDisplay.left + nHeaderWidth))/dDuration;
    SomethingHappened();
    nlReDrawAllBands();
    psLock->Unlock();
  TRACE("UnLocked by AdjustDuration\n");
  }
}

void CDisplay::ResetBandBlocks()
{
  if(psLock)
  {
    psLock->Lock();
    TRACE("Locked by ResetBandBlocks\n");
  }

  nlResetBandBlocks();

  if(psLock)
  {
    psLock->Unlock();
    TRACE("UnLocked by ResetBandBlocks\n");
  }
}


void CDisplay::nlResetBandBlocks()
{
  if(!parrTrace)
    return;  // Nothing to do, go home early
  for(int iBand=0; iBand < this->iNumBands; iBand++)
  {
    ((CBand *)arrBand.GetAt(iBand))->UnblockTrace();
  }
}

BOOL CDisplay::CheckForNeededTrace(void)
{
  if(bSomethingHappened && parrTrace)
  {
    psLock->Lock();
    TRACE("Locked by CheckForNeededTrace\n");
    //TRACE("%u CDisplay::CheckForNeededTrace\n",time(NULL));
    BOOL bFound = FALSE;
    TRACE("Block States: [");
    for(int iBand=0; iBand < arrBand.GetSize(); iBand++)
    {
      TRACE(" %d ",((CBand *)arrBand.GetAt(iBand))->pTrace->GetBlockCode());
      // We'll use bitwise, since it will work just as well
      bFound |=((CBand *)arrBand.GetAt(iBand))->
                 CheckForNeededTrace(dStart,dStart+dDuration+
                                     DISPLAY_DRAWING_DELAY_FUDGE_FACTOR);
    }
    TRACE("]\n");
    bSomethingHappened = FALSE;
    psLock->Unlock();
    TRACE("UnLocked by CheckForNeededTrace\n");
    return(bFound);
  }
  else
  {
    return(FALSE);
  }
}

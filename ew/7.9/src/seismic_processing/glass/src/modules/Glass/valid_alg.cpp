#include <IGlint.h>
#include <Debug.h>
#include <opcalc.h>

bool AlgValidateOrigin(ORIGIN * pOrigin, IGlint * pGlint, int nCut)
{

  PhaseType  ptPhase;
  PhaseClass pcPhase;
  size_t iPickRef=0;
  PICK * pck;
  int iPCtr = 0;
  int iPCICtr = 0;
  int iPAllCtr = 0;

  static int MIN_QLTY_CLOSEIN_ARR = 4;
  static int MIN_QLTY_ARR = nCut;
  static int MIN_P_ARR = nCut * 2;

  /***************************************************
   ***************************************************
   THIS FUNCTION IS MEANT TO BE A METHOD OF TUNING GLASS
   FOR YOUR DATA NETWORK.  THIS FUNCTION IS SUPPOSED TO
   BE A MECHANISM FOR AN INSTITUTION TO DO SECONDARY
   FILTERING, BASED ON THE SPECIFICS OF ITS DATA NETWORK,
   WITHIN THE SCOPE OF THE GLASS ASSOCIATOR.
   THE SOURCE FOR THIS FUNCTION WILL BE NETWORK SPECIFIC.
   DK 2004/11/30
   ***************************************************
   ***************************************************/


  // Ensure nCut-1 quality P phases in western hemisphere, and polar regions
  //  Where Western Hemisphere is the following box -45 x -180  90 x 50
  if(//Central Western Hemisphere
     (pOrigin->dLat > -45.0 && pOrigin->dLat < 85.0 &&
      pOrigin->dLon > -140.0 && pOrigin->dLon < -40.0)
     ||
     //Widespread NorthWest Quadrant
     (pOrigin->dLat > 0.0 && pOrigin->dLat < 85.0 &&
      pOrigin->dLon > -170.0 && pOrigin->dLon < -40.0)
     ||  
     // Polar Region
     (pOrigin->dLat > 70)
    )
  {
    // Check for increased number of P Phases in Intermountain West 
    // Death Zone (North America).  We get a lot of picks out of
    // IW/UU/MB, that are either noise picks or are picks for small
    // local quakes, that are too small to associate into events 
    // that we can locate, but profide enough fodder for Glass
    // to associate them into spurious events around the globe with
    // other noise picks.
    //  IW Death Zone is the following box  5 -150 x  85 -70
    // Exempt stations in the IW/UU/MB network, otherwise we'll
    // never locate any genuine earthquakes there.
    if(pOrigin->dLat > 5.0 && pOrigin->dLat < 85.0 &&
       pOrigin->dLon > -160.0 && pOrigin->dLon < -70.0  // DK Changed -150 to -160
       &&
       !(pOrigin->dLat > 37.5 && pOrigin->dLat < 46.0 &&
         pOrigin->dLon > -113.0 && pOrigin->dLon < -104.0)
      )
    {
      iPickRef=0;
      iPCtr = 0;
      iPCICtr = 0;
      while(pck = pGlint->getPicksFromOrigin(pOrigin, &iPickRef))
      {
        if(!pck->bTTTIsValid)
          continue;
        ptPhase = GetPhaseType(pck->ttt.szPhase);
        pcPhase = GetPhaseClass(ptPhase);
        if(pcPhase == PHASECLASS_P)
        {
          if(pck->tRes < 2.0 && pck->tRes > -2.0)
          {
            // if not from any of those networks
            if(strcmp(pck->sNet, "IW") &&
              strcmp(pck->sNet, "UU") &&
              strcmp(pck->sNet, "MB"))
            {
              if(pck->dDelta < 10.0)
                iPCICtr++;
              iPCtr++;
              if(iPCtr == MIN_QLTY_ARR || iPCICtr == MIN_QLTY_CLOSEIN_ARR)
              {
                pGlint->endPickList(&iPickRef);
                break;
              }
            }
          }
          iPAllCtr++;
        }  // if a P phase
      }  // end while not at end of picklist
      if((iPCtr < MIN_QLTY_ARR) && (iPCICtr < MIN_QLTY_CLOSEIN_ARR) && (iPAllCtr < MIN_P_ARR)) 
      {
        pGlint->endPickList(&iPickRef);
        CDebug::Log(DEBUG_MINOR_WARNING,"Origin %s/%d in North America, but too few non-IW Good P picks(%d)/(%d).  Invalidating!\n",
          pOrigin->idOrigin, pOrigin->iOrigin, iPCtr, iPCICtr);
        return false;
      }
    }    // end if in North America and not in IW zone
    else if(pOrigin->dLat > 37.5 && pOrigin->dLat < 46.0 &&
            pOrigin->dLon > -113.0 && pOrigin->dLon < -104.0)  // in IW zone

    {
      iPickRef=0;
      iPCtr = 0;
      iPCICtr = 0;
      while(pck = pGlint->getPicksFromOrigin(pOrigin, &iPickRef))
      {
        if(!pck->bTTTIsValid)
          continue;
        ptPhase = GetPhaseType(pck->ttt.szPhase);
        pcPhase = GetPhaseClass(ptPhase);
        if(pcPhase == PHASECLASS_P)
        {
          if(pck->tRes < 2.0 && pck->tRes > -2.0)
          {
            if(pck->dDelta < 3.0)
              iPCICtr++;
            iPCtr++;
            if(iPCtr == MIN_QLTY_ARR || iPCICtr == MIN_QLTY_CLOSEIN_ARR)
            {
              pGlint->endPickList(&iPickRef);
              break;
            }
          }
          iPAllCtr++;
        }  // if a P phase
      }  // end while not at end of picklist
      if((iPCtr < MIN_QLTY_ARR) && (iPCICtr < MIN_QLTY_CLOSEIN_ARR) && (iPAllCtr < MIN_P_ARR)) 
      {
        pGlint->endPickList(&iPickRef);
        CDebug::Log(DEBUG_MINOR_WARNING,"Origin %s/%d in IW zone, but too few Good P picks(%d)/(%d).  Invalidating!\n",
          pOrigin->idOrigin, pOrigin->iOrigin, iPCtr, iPCICtr);
        return false;
      }
    }   // else(in North America)
    else 
    {   // outside North America, but in the western hemisphere/polar area)
      iPickRef=0;
      iPCtr = 0;
      iPCICtr = 0;
      while(pck = pGlint->getPicksFromOrigin(pOrigin, &iPickRef))
      {
        if(!pck->bTTTIsValid)
          continue;
        ptPhase = GetPhaseType(pck->ttt.szPhase);
        pcPhase = GetPhaseClass(ptPhase);
        if(pcPhase == PHASECLASS_P)
        {
          if(pck->tRes < 2.0 && pck->tRes > -2.0)
          {
            if(pck->dDelta < 10.0)
              iPCICtr++;
            iPCtr++;
            if(iPCtr == MIN_QLTY_ARR || iPCICtr == MIN_QLTY_CLOSEIN_ARR)
            {
              pGlint->endPickList(&iPickRef);
              break;
            }
          }
          iPAllCtr++;
        }  // if a P phase
      }  // end while not at end of picklist
      if((iPCtr < MIN_QLTY_ARR) && (iPCICtr < MIN_QLTY_CLOSEIN_ARR) && (iPAllCtr < MIN_P_ARR)) 
      {
        pGlint->endPickList(&iPickRef);
        CDebug::Log(DEBUG_MINOR_WARNING,"Origin %s/%d in Western Hemisphere, but too few Good P picks(%d)/(%d).  Invalidating!\n",
          pOrigin->idOrigin, pOrigin->iOrigin, iPCtr, iPCICtr);
        return false;
      }
    }   // else(in North America)
  }      // if in Western Hemisphere
  else
  {
    iPickRef=0;
    iPCtr = 0;
    while(pck = pGlint->getPicksFromOrigin(pOrigin, &iPickRef))
    {
      if(!pck->bTTTIsValid)
        continue;
      ptPhase = GetPhaseType(pck->ttt.szPhase);
      pcPhase = GetPhaseClass(ptPhase);
      if(pcPhase == PHASECLASS_P)
      {
        if(pck->tRes < 2.0 && pck->tRes > -2.0)
        {
          iPCtr++;
          if(iPCtr == (nCut/2+1))
          {
            pGlint->endPickList(&iPickRef);
            break;
          }
        }
      }
    }
    if((iPCtr <= (nCut/2)) && (iPAllCtr < MIN_P_ARR)) 
    {
      CDebug::Log(DEBUG_MINOR_WARNING,"Origin %s/%d outside Western Hemisphere, but too few Good P picks(%d)/ need (%d).  Invalidating!\n",
                  pOrigin->idOrigin, pOrigin->iOrigin, iPCtr, nCut/2+1);
      return false;
    }
  }

  CDebug::Log(DEBUG_MINOR_INFO,"Origin %s/%d validating.\n",
              pOrigin->idOrigin, pOrigin->iOrigin);
  return(true);

}  // end AlgValidateOrigin()

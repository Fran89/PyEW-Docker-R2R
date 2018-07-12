/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT.
 *
 *    $Id: Refocus.cpp 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2007/12/27 18:05:23  paulf
 *     sync from hydra_proj circa Oct 2007
 *
 *     Revision 1.7  2006/04/27 20:16:01  davidk
 *     downgraded "can't find origin" message to MINOR_WARNING from MINOR_ERROR.
 *
 *     Revision 1.6  2005/10/17 06:36:01  davidk
 *     Added code to stop the focus mechanism from fighting with the locator.
 *     The focus now attempts at most one refocus per change in origin-pick
 *     association
 *
 *     Revision 1.5  2005/08/29 21:13:02  davidk
 *     Added CVS revision header.
 *     Removed extra triggered logging. (the idea being that if something ever went
 *     wrong and the logging triggered, that the logging tidalwave would be far worse
 *     than the problem it would be logging about).
 *
 *
 ***********************************************************************/
#include <IGlint.h>
#include <ITravelTime.h>
#include "Refocus.h"
#include "RefocusMod.h"

//extern "C" {
//#include "utility.h"
//}
#include <float.h>

static int __cdecl CompareTTEByTravelTime(const void *elem1, const void *elem2 );

#define SECONDS_SEARCH_WINDOW  60.0

// constant for minimum amount of improvement required to "re-focus" the origin
// 1.05 = 5% better
#define FOCUS_MINIMUM_IMPROVEMENT_RATIO 1.05  

extern CMod *pMod;

//---------------------------------------------------------------------------------------Zerg
// Calculate the Zerg function of x such that Zerg(0) = 1.0,
// zerg(-w) = zerg(w) = 0.0. dZerg/dX(0) = dZerg/dX(1) = 0.0.
// Zerg is symetric about 0.0, and is roughly gaussian with a
// half-width of 1. For x < -1 and x > 1, Zerg is defined as 0.
// Zerg is usually called as Zerg(x=y/w), where y is a value,
// such as a pick residual, and w is the half width
// of the distrubution desired.
double Zerg(double x) {
  if(x <= -1.0)
    return 0.0;
  if(x > 1.0)
    return 0.0;
  if(x > 0.001)
    return 2.0*x*x*x - 3.0*x*x + 1;
  if(x < -0.001)
    return -2.0*x*x*x - 3.0*x*x + 1;
  return 1.0;
}

//---------------------------------------------------------------------------------------Refocus
Refocus::Refocus() {
  pGlint = 0;
  nPck = 0;

  bDeepFocus = false;
}  // end Refocus()

//---------------------------------------------------------------------------------------~Refocus
Refocus::~Refocus() {
}  // end ~Refocus()

//---------------------------------------------------------------------------------------Generate
void Refocus::Focus() 
{
  
  

# define DEFAULT_DEPTH_RANGE_MAX 16
# define DEFAULT_DEPTH_RANGE_MIN  0
# define DEFAULT_DEPTH_MULTIPLICATION_FACTOR 1.075;

# define DEPTH_PHASE_MULTIPLIER 0.5
# define DEPTH_MAXIMUM 800.0
  int i;

  // this function attempts to determine the optimal point in a time/depth grid, where
  // the origin should be located

  // grab a pointer to the traveltime calculator
  ITravelTime *trv = pMod->pTrv;

  //  A couple of temporary traveltime entries and pointers
  TTEntry tte1,tte2, tte;
  TTEntry *ptte1, *ptte2, *ptte;

  static TTEntry * tteArray = NULL;

  // A pick pointer
  PICK *pck;

  double d;
  double tRes, tRes1, tRes2, tResMin;
  double t;
  double z;

  /* used vars */
  double tmin,tmax;
  //double tmesh = 2.0;    // Time resolution of focus mesh
  // DK 112204 changed to 0.5 seconds to improve granularity problems
  // that we believe are preventing us from getting good focus values
  // DK 112204, changed to 1.0, that seemed solid enough given the
  //            current focus mechanism, and would cut our # of loops in half.
  double tmesh = 5.0;    // Time resolution of focus mesh
  double zmesh = 4.0;  // Depth resolution of focus mesh
  double zmin = 0.0;
  double zmax;
  double tWindowHalfWidth;
  double tResWindowHalfWidth = 5.0;    // Half-width of the zerg distribution

  int nt, nz, maxt, maxz;

  static double *FocusTable = NULL; 
  static int    *PFocusTable = NULL; 

  int it;
  int iz;

  double tOrg = pOrg->dT;
  double dLatOrg = pOrg->dLat;
  double dLonOrg = pOrg->dLon;
  double dZOrg   = pOrg->dZ;
  double focorg  = 0.0;

  double dMult, dLastMult, dNextMult;

  double dDistKM, dAzm;
  double wzerg = 5.0;
  int    ipt;
  static bool  bInitialized = false;

  int iGoodPhaseTypes;
  int iCurrent;
  double tLast,tNext;

  int bPPhase, bLastPPhase, bNextPPhase;

  int    iNumberOfPhaseTypes = GetNumberOfPhases();

  // sanity check, lets not get in a tug of war with the locator
  // only run once since the inputs have changed
  if(!(pOrg->dFocusedT == 0.0 && pOrg->dFocusedZ == 0.0))
  {
    pMod->bFocus = false;  
    CDebug::Log(DEBUG_MAJOR_INFO,
                "Refocus(%s): Not running again because associations unchanged.\n",
                pOrg->idOrigin);
    return;
  }

  
  // If the origin is at less than 100km depth, then limit our search to
  // approximately 0-200km depth and -20 - +20 seconds
  if(pOrg->dZ < 100.0)
  {
    zmax = 200.0;
    tWindowHalfWidth = SECONDS_SEARCH_WINDOW / 3.0;  // changed to 3 (20sec vs 15)
  }
  else   // otherwise search the whole 0-800 km spectrum.(+/- 60 sec)
  {
    zmax = DEPTH_MAXIMUM;
    tWindowHalfWidth = SECONDS_SEARCH_WINDOW;
  }

  // set the boundaries for the search window based on the window half-width
  tmin = 0.0 - tWindowHalfWidth;
  tmax = tWindowHalfWidth;

  // validate traveltime pointer
  if(!trv)
    return;

  // calculate the number of time(x) iterations
    nt = (int)((tmax - tmin)/tmesh + 0.1) + 1;

  // calculate the number of depth(y) iterations
    nz = (int)((zmax - zmin)/zmesh + 0.1) + 1;


  if(!bInitialized)
  {
    maxt = (int)((SECONDS_SEARCH_WINDOW - (0.0 - SECONDS_SEARCH_WINDOW))/tmesh + 0.1) + 1; 
    maxz = (int)((DEPTH_MAXIMUM - 0)/zmesh + 0.1) + 1;

    FocusTable = (double *) calloc(maxt*maxz,sizeof(double));
    if(!FocusTable)
    {
      CDebug::Log(DEBUG_MAJOR_ERROR,"Focus():  Could not allocate %d bytes for FocusTable.\n",
                  maxt*maxz*sizeof(double));
      return;
    }
    PFocusTable = (int *) calloc(maxt*maxz,sizeof(int));
    if(!PFocusTable)
    {
      CDebug::Log(DEBUG_MAJOR_ERROR,"Focus():  Could not allocate %d bytes for PFocusTable.\n",
                  maxt*maxz*sizeof(int));
      return;
    }

   // allocate the TTEntry array
    tteArray = (TTEntry *)malloc(sizeof(TTEntry) * iNumberOfPhaseTypes);

    if(!tteArray)
    {
      CDebug::Log(DEBUG_MAJOR_ERROR,"Focus():  Could not allocate %d bytes for tteArray.\n",
                  sizeof(TTEntry) * iNumberOfPhaseTypes);
      return;
    }

    bInitialized = true;
  }  // if(!bInitialized)
  else
  {
    memset(FocusTable, 0, nt*nz*sizeof(double));
    memset(PFocusTable, 0, nt*nz*sizeof(int));
  }
  

  // Calculate the focus-quality value of the current origin time/depth
  // this will be our baseline value.
  // for each pick...

  bool bFocOrgImproved;
  for(i=0; i<nPck; i++) 
  {
    bFocOrgImproved = false;
    // shortcut
    pck = pPck[i];

    // if it belongs to the current origin, use the existing dist/azm info
    if(pck->iOrigin == pOrg->iOrigin)
      d = pck->dDelta;
    else  // otherwise calc dist/azm
       geo_to_km_deg(dLatOrg, dLonOrg, pck->dLat, pck->dLon, &dDistKM, &d, &dAzm);
    
    // if dist > 150, then we don't want it.
    if(d > 150.0)  // DK CLEANUP changed from 75.0
      continue;


    // t is the target traveltime
    t = pck->dT - (tOrg);

    // tRes is the residual of the best phase
    tRes = wzerg;

    /* for each pick, calculate the zerg of the closest phase */
    ptte1 = trv->TBestByClass(dZOrg,d,t, &tte1, PHASECLASS_P);
    ptte2 = trv->TBestByPhase( dZOrg,d,t, &tte2, PHASE_pP);
    if(!ptte2)
    {
      if(ptte1)      tRes = ptte1->dTPhase - t;
    }
    else if(!ptte1)
    {
      if(ptte2)      tRes = ptte2->dTPhase - t;
    }
    else 
    {
      if(ptte1->dTPhase > t)       tRes1 = ptte1->dTPhase - t;
      else                         tRes1 = t - ptte1->dTPhase;

      if(ptte2->dTPhase > t)       tRes2 = ptte2->dTPhase - t;
      else                         tRes2 = t - ptte2->dTPhase;
      
      if(tRes1 < tRes2)
      {
        tRes = tRes1;
        dMult = 0.5;
      }
      else                         
      {
        tRes = tRes2;
        dMult = DEPTH_PHASE_MULTIPLIER;
      }
    }

    /* make sure tRes is a positive number.  we're just trying to determine
       if it's within a window, so we don't care about it's direction */
    if(tRes < 0.0)
      tRes = 0.0 - tRes;

    // if we got a residual within the allowable range, then
    // add this to the overall focus value for this time/depth
    if(tRes < wzerg)
    {
      bFocOrgImproved = true;
      focorg+= 1; // + Zerg(tRes/wzerg) * dMult;
    }

    // for each depth iteration
    for(iz=0; iz<nz; iz++)
    {
      // calc depth for this iteration
      z = zmin + zmesh*iz;

      if((z > 50.0) && (iz % 5))
        continue;  // limit to ever 5th step (20km) for depths > 50km


      // target traveltime  - we base off origin time.  
      //    This is not a perfect method, ideally we would grab the closest
      //    phase to each time iteration, but we only expect one
      //    phase per phase type in our traveltime tables, even though
      //    that might not be the case in terms of tau(p) paths
      t = pck->dT - tOrg;

      // reinit the tteArray
      // DK 112204  we shouldn't need to memset it each time, as long
      //            as we set the appropriate fields before reading
      //   memset(tteArray,0,sizeof(TTEntry) * iNumberOfPhaseTypes);

      iGoodPhaseTypes = 0;
      // for each phasetype, calculate the best possible traveltime.
      for(ipt=0; ipt < iNumberOfPhaseTypes; ipt++)
      {
        // only use P phases and pP
        //    maybe sometime we could expand this to include all depth phases
        if(Phases[ipt].iClass == PHASECLASS_P || 
           Phases[ipt].iNum  == PHASE_pP)
        {
          ptte = trv->TBestByPhase( z,d,t, &tte, Phases[ipt].iNum);

          // only mark the phase as good IF we got a valid result
          // back, and the residual for that result is within the tmin/tmax window.
          if(ptte)
          {
            tRes = t - ptte->dTPhase;
            if(tRes < tmax && tRes > tmin)
            {
              memcpy(&tteArray[iGoodPhaseTypes], ptte, sizeof(tte));
              iGoodPhaseTypes++;
            }
          }
        }
      }  /* end for each phase type */

      /* already determined iGoodPhaseTypes, and consolidated array, above
      iGoodPhaseTypes = iNumberOfPhaseTypes;

      // remove the non-qualifying phase-type entries from the list
      for(ipt=0;ipt < iGoodPhaseTypes; ipt++)
      {
        if(!tteArray[ipt].szPhase)
        {
          iGoodPhaseTypes --;
          memcpy(&tteArray[ipt], &tteArray[iGoodPhaseTypes], sizeof(TTEntry));
          ipt--;
          continue;
        }
      }
      **********************************************/
     
      // make sure we have atleast one potential phase at this depth
      if(iGoodPhaseTypes <= 0)
      {
        if((z-dZOrg < FLT_EPSILON) && (dZOrg-z < FLT_EPSILON) && bFocOrgImproved)
        {
            CDebug::Log(DEBUG_MINOR_ERROR, "WARNING: Current Orig Equiv remains at F=%.0f PF=%d, even though actual origin increaesed.  No phases for pick at this depth!\n",
                        FocusTable[iz*nt+ nt/2],  PFocusTable[iz*nt+ nt/2]);
        }
        continue;  // nothing at this depth, go on to the next depth iteration
      }

      // sort the list by traveltime
      qsort(tteArray, iGoodPhaseTypes, sizeof(TTEntry), CompareTTEByTravelTime);

      iCurrent = 0;
      tLast = -999999999999.0;  // a low number that is too low to be used
      dLastMult = 0.0;
      bLastPPhase = 0;

      tNext = tteArray[iCurrent].dTPhase;  // the first/earliest-arriving valid phase entry
      if(GetPhaseType(tteArray[iCurrent].szPhase) == PHASE_pP)
      {
        dNextMult = DEPTH_PHASE_MULTIPLIER;
        bNextPPhase = 0;
      }
      else
      {
        dNextMult = 1.0;
        bNextPPhase = 1;
      }

      // tTravMin is the target traveltime for the earliest possible origin!
      double tTravMin = pck->dT - (tOrg + tmin);

      // for each time iteration
      for(it=nt-1; it>=0; it--)
      {
        // calc the target pick traveltime where it*tmesh is our x-coordinate on the focus display
        t = tTravMin - (it*tmesh);

        // while the target time is greater than the next pick
        while(t > tNext)
        {
          // move forward one phase in the list
          tLast = tNext;
          dLastMult = dNextMult;
          bLastPPhase = bNextPPhase;
          if(iCurrent < (iGoodPhaseTypes - 1))
          {
            tNext = tteArray[++iCurrent].dTPhase;
            if(GetPhaseType(tteArray[iCurrent].szPhase) == PHASE_pP)
            {
              dNextMult = DEPTH_PHASE_MULTIPLIER;
              bNextPPhase = 0;
            }
            else
            {
              dNextMult = 1.0;
              bNextPPhase = 1;
            }
          }
          else  // end of the list
          {
            tNext = 999999999999.0;  // use a high number that is too high to be used
            dNextMult = 0.0;
            bNextPPhase = 0;
          }
        }  // end while t > tNext

        tRes1 = t - tLast;  // residual of the last arrival prior to the target time
        tRes2 = tNext - t;  // residual of the first arrival after the target time

        // get the residual of the closest phase
        if(tRes1 < tRes2)
        {
          tResMin = tRes1;
          dMult = dLastMult;
          bPPhase = bLastPPhase;
        }
        else
        {
          tResMin = tRes2;
          dMult = dNextMult;
          bPPhase = bNextPPhase;
        }

        // if the residual is within range, add it's Zerg() to
        // the current focus table entry.
        if(tResMin < wzerg)
        {
          FocusTable[iz*nt+it]+= 1; // + Zerg(tResMin/wzerg) * dMult;
          PFocusTable[iz*nt+it]+= bPPhase; // + Zerg(tResMin/wzerg) * dMult;
        }
      }  // end for each time iteration
    }    // end for each depth iteration
  }      // end for each pick

  double focmax = 0.0;
  int itfocus=-1;
  int izfocus=-1;
  for(it=0; it<nt; it++) 
  {
    for(iz=0; iz<nz; iz++) 
    {
      if(PFocusTable[iz*nt+it] >= (FocusTable[iz*nt+it] * 0.5)) // ensure that atleast 1/2 the phases are P
                                                              // otherwise we may end up converting a quake
                                                              // to mostly pP phases, and the validator 
                                                              // will throw it out for being jibberish!
                                                              // DK 021805
      {
        /* Try to favor the default depth.  The theory is that if we don't get
           either a shapely enough curve indicative of a particular depth, or 
           enough depth phases to indicate a particular depth, then we should
           use the default depth.  So we mulitply the Focus value by the 
           DEFAULT_DEPTH_MULTIPLICATION_FACTOR to give the default depth a 
           slight advantage in refocusing.
           The reason we're doing this is that we see glass deliver a lot of
           artificially deep earthquakes because of an errant pick or two, when
           most earthquakes that do not create depth phases are fairly close
           to the surface (< 35km ) */
        if(iz <= (DEFAULT_DEPTH_RANGE_MAX / zmesh))  /* depth < 16 km */
          FocusTable[iz*nt+it] *= DEFAULT_DEPTH_MULTIPLICATION_FACTOR; 

        if(FocusTable[iz*nt+it] > focmax) {
          focmax = FocusTable[iz*nt+it];
          itfocus = it;
          izfocus = iz;
        }
      }
    }
  }
  /******************
  // cleanup
  if(FocusTable)
  {
    free(FocusTable);
    FocusTable = NULL;
  }

  if(tteArray)
  {
    free(tteArray);
    tteArray = NULL;
  }
  ******************/

  tRes = (tmin + itfocus*tmesh);

  /* apply the multiplication factor */
  if(dZOrg < DEFAULT_DEPTH_RANGE_MAX)
      focorg *= DEFAULT_DEPTH_MULTIPLICATION_FACTOR;

  if(focmax > (focorg * FOCUS_MINIMUM_IMPROVEMENT_RATIO))
  {
    pMod->bFocus = true;
    pOrg->dT += tRes;
    pOrg->dZ =  (zmin + izfocus*zmesh);
    pOrg->dFocusedT = pOrg->dT;
    pOrg->dFocusedZ = pOrg->dZ;
  }
  else
  {
    if(focorg > (focmax+1))
    {
      for(it=0; it<nt; it++) 
      {
        for(iz=0; iz<nz; iz++) 
        {
          // calc depth for this iteration
          z = zmin + zmesh*iz;
          if(!((z > 50.0) && (iz % 5)))
            CDebug::Log(DEBUG_MAJOR_INFO, "x%3.0f/%2d\t", FocusTable[iz*nt+it],  PFocusTable[iz*nt+it]);
        }
        CDebug::Log(DEBUG_MAJOR_INFO, "x\n");
      }
      CDebug::Log(DEBUG_MAJOR_INFO, "x\n");
    }
    pMod->bFocus = false;
    // still record the focused info
    pOrg->dFocusedT = pOrg->dT + tRes;
    pOrg->dFocusedZ = (zmin + izfocus*zmesh);
  }

  pMod->dT = pOrg->dFocusedT;
  pMod->dZ = pOrg->dFocusedZ;

  CDebug::Log(DEBUG_MAJOR_INFO,
              "Refocus(%s): Cur:(%.0f)-%.2f New:(%.0f, %.2f)-%.2f bF %1d\n",
              pOrg->idOrigin, dZOrg, focorg, (zmin + izfocus*zmesh), tRes, focmax, (int)pMod->bFocus);
              
}  // end Focus()

//---------------------------------------------------------------------------------------Quake
void Refocus::Quake(char *ent) {
  PICK *pck;

  if(!pGlint)
    return;

  if(!(pOrg = pGlint->getOrigin(ent)))
  {
    // We could not find the Origin in glint!  This is bad, because we should only be called
    // for active quakes.  Issue Error Message!
    CDebug::Log(DEBUG_MINOR_WARNING,"Quake():  Could not find Origin(%s) in Glint\n", ent);
    return;
  }

  nPck = 0;
  size_t iPickRef=0;
  // DK trying deep focus 013004
  double t1,t2;
  t1 = pOrg->dT;
  t2 = t1 + 1680.0;

  // grab picks for time range.
  // If only want to consider picks already attached to the 
  // solution, use getPicksForOrigin() instead
  while(pck = pGlint->getPicksForTimeRange(t1,t2,&iPickRef)) 
  {
    
    if(nPck >= MAXPCK)
    {
      pGlint->endPickList(&iPickRef);
      break;
    }
    pPck[nPck] = pck;
    nPck++;
  }

  Focus();
}  // end Quake()



static int __cdecl CompareTTEByTravelTime(const void *elem1, const void *elem2 )
{
  TTEntry * p1 = (TTEntry *)elem1;
  TTEntry * p2 = (TTEntry *)elem2;

  if(p1->dTPhase < p2->dTPhase)
    return(-1);
  else if(p1->dTPhase == p2->dTPhase) 
    return(0);
  else
    return(1);
}  // end CompareTTEByTravelTime()

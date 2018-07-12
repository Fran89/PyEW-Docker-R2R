#include <math.h>
#include <Debug.h>
#include <string.h>


# define _OPCALC_SO_EXPORT  __declspec( dllexport)

#include <opcalc.h>
#include <opcalc_const.h>

extern "C" {
void __cdecl qsortd (double *base, unsigned num);
}


int OPCalc_ProcessOriginSimple(ORIGIN * pOrg, PICK ** pPick, int nPck);


// Lib Global variables - File Scope
static ITravelTime * pTT = 0x00;      // file-scope master traveltime pointer

//---------------------------------------------------------------------------------------Zerg
// Calculate the OPCalc_BellCurve function of x.
// The height of the curve ranges from 0 to 1.
// The curve is centered at 0, with a range from -1 to 1,
// such that OPCalc_BellCurve(0) = 1 and OPCalc_BellCurve(-1) = OPCalc_BellCurve(1) = 0.
// OPCalc_BellCurve is usually called as OPCalc_BellCurve(x/w), 
// where x is the deviation from the mean, and w is the half-width
// of the distrubution desired.
// OPCalc_BellCurve(-w) = OPCalc_BellCurve(w) = 0.0. 
// dBellCurve/dX(0) = dBellCurve/dX(1) = 0.0.
// OPCalc_BellCurve is symetric about 0.0, and is roughly gaussian with a
// half-width of 1. For x < -1 and x > 1, OPCalc_BellCurve is defined as 0.
// In Carl terms, OPCalc_BellCurve is defined as Zerg.
double OPCalc_BellCurve(double xx) 
{
	double x = xx;
	if(x < 0.0)
		x = -x;
	if(x > 1.0)
		return 0.0;
	return 2.0*x*x*x - 3.0*x*x + 1.0;
}


int __cdecl ComparePickAffinityAsc(const void * pvoid1, const void * pvoid2)
{
  PICK * p1, *p2;

  p1 = *(PICK **) pvoid1;
  p2 = *(PICK **) pvoid2;

  if(p1->dAff < p2->dAff)
    return(-1);
  if(p1->dAff > p2->dAff)
    return(1);
  else
    return(0);
}

int __cdecl ComparePickResidDesc(const void * pvoid1, const void * pvoid2)
{
  PICK * p1, *p2;

  p1 = *(PICK **) pvoid1;
  p2 = *(PICK **) pvoid2;

  if(fabs(p1->tRes) > fabs(p2->tRes))
    return(-1);
  if(fabs(p1->tRes) < fabs(p2->tRes))
    return(1);
  else
    return(0);
}
  
int __cdecl ComparePickiPickAsc(const void * pvoid1, const void * pvoid2)
{
  PICK * p1, *p2;

  p1 = *(PICK **) pvoid1;
  p2 = *(PICK **) pvoid2;

  if(p1->iPick < p2->iPick)
    return(-1);
  if(p1->iPick > p2->iPick)
    return(1);
  else
    return(0);
}
  

int OPCalc_ProcessOrigin(ORIGIN * pOrg, PICK ** pPick, int nPck) 
{

  double dFullAff;
  int    rc;
  int    i;
  PICK * pck;

  /* process the origin with the full complement of picks 
   *******************************************************/
  if(rc=OPCalc_ProcessOriginSimple(pOrg, pPick, nPck))
    return(rc);

  dFullAff = pOrg->dAff;

  // log the results
  CDebug::Log(DEBUG_MINOR_INFO,"OPC_PO: Full Origin: %.2f/%.2f/%.2f - %.2f  Aff: %.2f  AfGp: %.2f AfNeq: %.2f  RMS: %.2f Gap: %.2f\n",
              pOrg->dLat, pOrg->dLon, pOrg->dZ, pOrg->dT, pOrg->dAff, pOrg->dAffGap, pOrg->dAffNumArr, pOrg->dRms, pOrg->dGap,
              pOrg->dRms, pOrg->nEq);

  for(i=0; i<nPck; i++)
  {
		pck = pPick[i];
    CDebug::Log(DEBUG_MINOR_INFO,
                "%3d) %-4s %-6s %5.1f %3.0f %3.0f %5.2f %6.1f %4.1f %3.1f/%3.1f/%3.1f  %s\n",
			          i, pck->sSite, pck->sPhase,
			          pck->dDelta, pck->dAzm, pck->dToa * RAD2DEG, pck->tRes, pck->dT - pOrg->dT, 
                pck->dAff, pck->dAffDis, pck->dAffRes, pck->dAffPPD, pck->sTag
               );  
  }

  // sort the picks by resid
  qsort(pPick, nPck, sizeof(PICK *), ComparePickResidDesc);

  /* when trimming picks, don't trim anything that is either:
     1) outside of the worst 20% in terms of residual
     2) inside 1 std. deviation (assuming normal distribution with stddev = 3.33 sec) 
     We Make three BIG assumptions here:
     a) That the ORIGIN is an accurate approximation of the Event origin
     b) That the pick residual distribution is NORMAL.
     c) That the stddev for such a distribution is 3.33 seconds.
   **************************************************************/
#define OPCALC_MAX_RESID_TRIM_RATIO .20
#define OPCALC_PICK_RESID_DIST_STD_DEV_SEC 3.33

  // trim out the worst picks (by res)
  for(i=0; i<(nPck*OPCALC_MAX_RESID_TRIM_RATIO); i++)
  {
		pck = pPick[i];
    CDebug::Log(DEBUG_MINOR_INFO, "Pick filter: %d res: %.2f aff: %.2f  (%.2f ~ %.2f)\n",
                i, pck->tRes, pck->dAff,
                fabs(pck->tRes), OPCALC_PICK_RESID_DIST_STD_DEV_SEC);
    if(fabs(pck->tRes) < OPCALC_PICK_RESID_DIST_STD_DEV_SEC)
      break;
  }

  /* process the origin with the full complement of picks 
   *******************************************************/
  if(i<nPck)
  {
    OPCalc_ProcessOriginSimple(pOrg, &pPick[i], nPck-i);

    // log the results
    CDebug::Log(DEBUG_MINOR_INFO,"OPC_PO: Trimmed Origin: %.2f/%.2f/%.2f - %.2f  Aff: %.2f  AfGp: %.2f AfNeq: %.2f  RMS: %.2f Gap: %.2f\n",
                pOrg->dLat, pOrg->dLon, pOrg->dZ, pOrg->dT, pOrg->dAff, pOrg->dAffGap, pOrg->dAffNumArr, pOrg->dRms, pOrg->dGap,
                pOrg->dRms, pOrg->nEq);

    for(; i<nPck; i++)
    {
		  pck = pPick[i];
      CDebug::Log(DEBUG_MINOR_INFO,
                  "%3d) %-4s %-6s %5.1f %3.0f %3.0f %5.2f %6.1f %4.1f %3.1f/%3.1f/%3.1f  %s\n",
			            i, pck->sSite, pck->sPhase,
			            pck->dDelta, pck->dAzm, pck->dToa * RAD2DEG, pck->tRes, pck->dT - pOrg->dT, 
                  pck->dAff, pck->dAffDis, pck->dAffRes, pck->dAffPPD, pck->sTag
                 );  
    }

    // if the full was better, then reprocess the parameters using the FULL 
    if(dFullAff > pOrg->dAff)
    {
      OPCalc_ProcessOriginSimple(pOrg, pPick, nPck);
    }
  }

  // sort the picks by resid
  qsort(pPick, nPck, sizeof(PICK *), ComparePickiPickAsc);

  return(0);

}
int OPCalc_ProcessOriginSimple(ORIGIN * pOrg, PICK ** pPick, int nPck) 
{

  double pPickDistArray[OPCALC_MAX_PICKS];
  double pPickAzmArray[OPCALC_MAX_PICKS];

  // Initialize origin vars
	int nEq = 0;
  int i;

	double sum = 0.0;
  double dWeight = 0.0;
  PICK * pck;
  int iRetCode;
  double dGap;

  PhaseType  ptPhase;
  PhaseClass pcPhase;

  // Set default RMS to absurd level.  Shouldn't this be an affinity setting.
  pOrg->dRms = 10000.0;


//  CDebug::Log(DEBUG_MAJOR_INFO,"Locate(I): Org(%6.3f,%8.3f,%5.1f - %.2f)\n",
//              pOrg->dLat, pOrg->dLon, pOrg->dZ, pOrg->dT);

	for(i=0; i<nPck; i++) 
  {
		pck = pPick[i];
    iRetCode = OPCalc_CalculateOriginPickParams(pck,pOrg);
    if(iRetCode == -1)
    {
      CDebug::Log(DEBUG_MINOR_ERROR,"ProcessOrigin(): OPCalc_CalculateOriginPickParams  failed with "
                                    "terminal error.  See logfile.  Returning!\n");
      return(-1);
    }
    else if(iRetCode == 1)
    {
      // could not find TravelTimeTable entry for pOrg,pck
      CDebug::Log(DEBUG_MINOR_INFO,"ProcessOrigin(): OPCalc_CalculateOriginPickParams() could not "
                                    "find TTT entry for Org,Pick!\n");
      continue;
    }

    // accumulate statistics
    sum += pck->tRes*pck->tRes*pck->ttt.dLocWeight;

    dWeight += pck->ttt.dLocWeight;

    // copy the delta/azimuth of the pick to proper arrays for
    // further calculations
    pPickDistArray[nEq] = pck->dDelta;

    // DK 021105  Change gap calculation to only use P-Class phases
    // that way if we pull in random S, PPP, and SKiKP phases,
    // they won't affect the gap "quality" of the solution.
    ptPhase = GetPhaseType(pck->ttt.szPhase);
    pcPhase = GetPhaseClass(ptPhase);
    if(( (pcPhase == PHASECLASS_P) && (pck->dDelta < OPCALC_MAX_BELIEVABLE_DISTANCE_FOR_P_PHASE )) || 
       (ptPhase == PHASE_PKPdf)
      )
      pPickAzmArray[nEq] = pck->dAzm;
    else
      pPickAzmArray[nEq] = -1.0;


    // increment the number of picks used to locate the origin
    nEq++;
    
    // keep the arrays from being overrun
    if(nEq > OPCALC_MAX_PICKS)
      break;
  }  // end for each pick in the Pick Array

  // if we didn't use atleast one pick.  Give up and go home.
  if(nEq < 1)
	  return(1);


  // Record the number of equations used to calc the location
  pOrg->nEq = nEq;

  // Record the residual standard deviation
  pOrg->dRms = sqrt(sum/dWeight);

  /* Calculate Pick Azimuth statistics for the Origin (Gap)
   ****************************************************/
  // Sort the pick-azimuth array
  qsortd(pPickAzmArray, nEq);

  // If the Pick wasn't a P-class Pick, then we gave it a negative azm,
  // so igonre all of the negative azm picks (which are all at the beginning
  // since the array has been sorted
 	for(i=0; i<nEq-2; i++) 
  {
    if(pPickAzmArray[i] >= 0.0)
      break;
  }

  CDebug::Log(DEBUG_MAJOR_INFO,
              "ProcessOrigin(): Calculating Gap for Org %d(%5.1f, %6.1f, %3.0f) based on %d valid picks.\n ",
              pOrg->iOrigin, pOrg->dLat, pOrg->dLon, pOrg->dZ, nEq - i);
  // Calculate the gap for the last-to-first wrap
 	dGap = pPickAzmArray[i] + 360.0 - pPickAzmArray[nEq-1];
 	for(; i<nEq-1; i++) 
  {
    // for each gap between two picks, record only if largest so far
    if(pPickAzmArray[i+1] - pPickAzmArray[i] > dGap)
      dGap = pPickAzmArray[i+1] - pPickAzmArray[i];
 	}
 	pOrg->dGap = dGap;

  /* Calculate Pick Distance statistics for the Origin
   ****************************************************/
  // Sort the pick-distance array
  qsortd(pPickDistArray, nEq);

  // Calculate the minimum station distance for the origin
	pOrg->dDelMin = pPickDistArray[0];

  // Calculate the maximum station distance for the origin
	pOrg->dDelMax = pPickDistArray[nEq-1];

  // Calculate the median station distance for the origin
	pOrg->dDelMod = 0.5*(pPickDistArray[nEq/2] + pPickDistArray[(nEq-1)/2]);


  /* Calculate the Affinity for the Origin (and Picks) 
   ****************************************************/
  iRetCode = OPCalc_CalcOriginAffinity(pOrg, pPick, nPck);
  if(iRetCode)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,
                "Error: ProcessOrigin(): OPCalc_CalcOriginAffinity() return (%d)\n",
                iRetCode);
    return(iRetCode);
  }
     
  return(0);
}  // End Process Location()


int OPCalc_CalcOriginAffinity(ORIGIN * pOrg, PICK ** ppPick, int nPck) 
{
//	PICK *pck;
	int i;
  int iRetCode;
  double dAffinitySum = 0.0;  // Sum of usable-pick affinities
  int    iAffinityNum = 0;    // Number of usable-picks

  if(nPck < 1)
    return(-1);

  // Log the calculated values
	CDebug::Log(DEBUG_MAJOR_INFO,"Affinity(): dDelMin:%.2f dDelMod:%.2f dDelMax:%.2f\n",
		pOrg->dDelMin, pOrg->dDelMod, pOrg->dDelMax);

	// Gap affinity
		// DK CHANGE 011504.  Changing the range of the
    //                    dAffGap statistic
    //         pOrg->dAffGap = 1.0+OPCalc_BellCurve(pOrg->dGap/360.0);
    // dAffGap range is 0.0 - 2.0
  pOrg->dAffGap = 4 * OPCalc_BellCurve(pOrg->dGap/360.0); 
	if(pOrg->nEq < OPCALC_nMinStatPickCount && pOrg->dAffGap < 1.0)
		pOrg->dAffGap = 1.0;

//    pOrg->dAffGap = 3 * OPCalc_BellCurve(pOrg->dGap/360.0); 

  if(pOrg->dAffGap > 2.0)
    pOrg->dAffGap = 2.0;
  else if(pOrg->dAffGap < 0.5 && pOrg->dAffGap > 0.1)
    pOrg->dAffGap = 0.5;  // don't let AffGap drop below 0.5 unless 
                          //the Gap is AWFUL!!!!



	// nEq affinity (macho statistic)
	if(pOrg->nEq < OPCALC_nMinStatPickCount)
		pOrg->dAffNumArr = 1.0;
	else
		pOrg->dAffNumArr = log10((double)(pOrg->nEq));

	for(i=0; i<nPck; i++) 
  {
    iRetCode = OPCalc_CalcPickAffinity(pOrg, ppPick[i]);
	  CDebug::Log(DEBUG_MINOR_INFO,"Affinity(): Pck[%d] Delta:%.2f Aff:%.2f\n", i, ppPick[i]->dDelta, ppPick[i]->dAff);

    if(iRetCode < 0)
    {
   	  CDebug::Log(DEBUG_MINOR_ERROR,
                  "OPCalc_CalcOriginAffinity(): Fatal Error(%d) calculating"
                  " affinity for pick[%d]\n",
                  iRetCode,i);
      return(-1);
    }
    if(iRetCode > 0)
      continue;

    dAffinitySum+=ppPick[i]->dAff;
    iAffinityNum++;
  }

  if(iAffinityNum > 0)
  {
    pOrg->dAff = dAffinitySum / iAffinityNum;

    /* now lets do something more advanced.  
       We are trying to calculate the "quality" of this origin.  
       We're not trying to recalculate where the origin should be,
       only trying to caculate a "quality" number.
       Under the current basic calculations, we can run into problems
       where a couple of random picks can have residuals close enough in 
       to associate, but far enough out to lower the quality of the origin.
       A problem is, that as soon as you remove a pick from the origin 
       calculation, you have to do a lot of quality recalculation.  Removing 
       one pick potentially affects both AffGap affinity component as well
       as the AffNumArr.
       What we'd like is to find some sort of peak quality value for the Origin,
       (a reverse saddle).  We'll try to get there by taking the full result
       (all-picks), and remove the picks with the lowest affinity until we hit 
       a point where the origin affinity starts to decrease.  
       Unfortunately this can't all be done in the OPCalc routines, because the
       Gap is currently calculated in the Glock module ONLY.
       DK 0920'06
      ************************************************************************/

    for(i=0; i<nPck; i++) 
    {
    }



  }
  else
  {
    pOrg->dAff = 0;
  }
  return(0);
}  // end OPCalc_CalcOriginAffinity()


int OPCalc_CalcPickAffinity(ORIGIN * pOrg, PICK * pPick) 
{

  double dDistRatio;  // ratio between distance of current pick and median distance
  double dMedianDistEst;

  int iRetCode;

	// Do full calculations for picks not already associated with the
  // origin.  (distance, azimuth, traveltime, TakeOffAngle)
	if(pPick->iState == GLINT_STATE_WAIF) 
  {
    iRetCode = OPCalc_CalculateOriginPickParams(pPick,pOrg);
    if(iRetCode)
      return(iRetCode);
	}

	// Residual affinity
  // Range   0.0 - 2.0  where 0.0 is no residual and 2.0 is 
  //         any absolute residual >= 10 seconds (OPCALC_dResidualCutOff)
	pPick->dAffRes = 2.0 * OPCalc_BellCurve(pPick->tRes/(pPick->ttt.dResidWidth));

	// Distance affinity
  if(OPCALC_fAffinityStatistics & AFFINITY_USE_DISTANCE)
  {

    // hack in a calculation that increases the value used for the median distance if the number of 
    // phases is large.
    if(pOrg->nPh > (pOrg->dDelMod*2.0))
      dMedianDistEst = (double) (pOrg->nPh/2);
    else
      dMedianDistEst = pOrg->dDelMod;
    
    // Calculate the distance of this pick relative
    // to the distance of median pick used in the solution
    if(pOrg->nEq < OPCALC_nMinStatPickCount) 
    {
        dDistRatio = pPick->dDelta / (OPCALC_dCutoffMultiple*dMedianDistEst * 1.5);
    } 
    else 
    {
        dDistRatio = pPick->dDelta / (OPCALC_dCutoffMultiple*dMedianDistEst);
    }
    // Distance Affinity
    //       Penalize picks (based upon distance) when
    //       they are more than dCutoffMultiple times the 
    //       median pick distance from the Origin.
    // Range   0.0 - 2.0  where 0.0 is beyond the Modal distance cutoff and 2.0 is 
    //         a pick at the hypocenter.
    pPick->dAffDis = 2 * OPCalc_BellCurve(dDistRatio);

    if(pOrg->nEq < 4)
      pPick->dAffDis = 1.0;
  }
  else
  {
    pPick->dAffDis = 1.0;
  }

  if(OPCALC_fAffinityStatistics & AFFINITY_USE_PPD)
  {
    // Pick Probability Affinity
    // Calculate a Pick Probability Affinity statistic.
    // Range   0.5 - 5.0 (highest observed value is 3.0)
    // where the value is ((log 2 (X+1)) + 1) / 2
    // where X is the number of times that a phase was picked
    // within the Pick's travel time entry per 10000 picks.
    pPick->dAffPPD = pPick->ttt.dAssocStrength / 2.0;
    if(pPick->dAffPPD > 2.0)
      pPick->dAffPPD = 2.0;
  }
  else
  {
    pPick->dAffPPD = 1.0;
  }

	// Composite affinity statistic
	pPick->dAff = pOrg->dAffGap * pOrg->dAffNumArr * pPick->dAffRes * pPick->dAffDis * pPick->dAffPPD;

  return(0);
}  // end CalcPickAffinity()


void OPCalc_SetTravelTimePointer(ITravelTime * pTravTime)
{
  pTT = pTravTime;
}


int OPCalc_CalculateOriginPickParams(PICK * pPick, ORIGIN * pOrg)
{
  TTEntry   ttt;
  TTEntry * pttt;
  double    dDistKM;

  if(!(pTT && pPick && pOrg))
  {
		CDebug::Log(DEBUG_MINOR_ERROR,"OPCalc_CalculateOriginPickParams(): Null Pointer: "
                                  "pPick(%u) pOrg(%u) pTT(%u)\n", 
                pPick, pOrg, pTT);
    return(-1);
  }

  if(pPick->dLat < -90.0 || pPick->dLat > 90.0 || pPick->dLon < -180.0 || pPick->dLon > 180.0)
    return(-1);

	geo_to_km_deg(pOrg->dLat, pOrg->dLon, pPick->dLat, pPick->dLon, 
                &dDistKM, &pPick->dDelta, &pPick->dAzm);
	pttt = pTT->TBest(pOrg->dZ, pPick->dDelta, pPick->dT - pOrg->dT, &ttt);
	if(!pttt) 
  {
    pPick->bTTTIsValid = false;
    pPick->tRes = 9999.9;
    pPick->ttt.szPhase = Phases[PHASE_Unknown].szName;

    return(1);
  }

	pPick->dTrav = pttt->dTPhase;
	pPick->dToa = pttt->dTOA;
	pPick->tRes = (pPick->dT - pOrg->dT) - pttt->dTPhase;
  strcpy(pPick->sPhase, pttt->szPhase);
  memcpy(&pPick->ttt, pttt, sizeof(TTEntry));
  pPick->bTTTIsValid = true;
  CDebug::Log(DEBUG_MINOR_INFO,"Iterate(): Eq[%d] d:%.2f z:%.2f tobs:%.2f azm:%.2f ttt:%d\n", 
              pOrg->nEq, pPick->dDelta, pOrg->dZ, pPick->dT - pOrg->dT, pPick->dAzm, pttt);

  return(0);
}  // end OPCalc_CalculateOriginPickParams()



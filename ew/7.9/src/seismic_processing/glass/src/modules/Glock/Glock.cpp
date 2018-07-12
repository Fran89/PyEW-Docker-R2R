/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: Glock.cpp 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/12/27 18:05:23  paulf
 *     sync from hydra_proj circa Oct 2007
 *
 *     Revision 1.6  2006/09/30 05:34:51  davidk
 *     All origin-pick relation calculations were moved to OPCalc(Gap, etc).  Now Glock
 *     calls OPCalc to do the calculation of Origin-Pick derived parameters, and
 *     calls Solve to invert the pick data into a better origin solution.
 *     So now it acts as more of a delegatory shell, instead of containing a bunch
 *     of calculations.
 *
 *     Revision 1.5  2006/05/04 23:16:25  davidk
 *     Moved pMod and parameter initialization from static tasks to constructor tasks,
 *     the way they should be.
 *
 *     Revision 1.4  2006/01/30 23:16:22  davidk
 *     Added int type-cast to get rid of compiler warning.
 *
 *     Revision 1.3  2005/10/17 06:34:31  davidk
 *     Modified the way phase weighting is handled in the locator.  The person who
 *     originally put it in, did not understand how the least square solver worked and
 *     wrongly applied the weighting values.
 *     Now weighting is done in integral increments ranging from 0 to 4.
 *     Currently any phase with weight below (GLOCK_WEIGHT_GRANULARITY/2)
 *     is not passed to the solver.
 *
 *     Revision 1.2  2005/10/13 22:21:58  davidk
 *     Fixed a bug where the locator was performing multiple relocations with the same
 *     starting location, instead of refining the location by using the results of the
 *     previous location as the starting point for the next.
 *     Made an aesthetic change to the calculation of the depth coefficient.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.8  2005/02/28 23:42:11  davidk
 *     Downgraded a debug message.
 *
 *     Revision 1.7  2005/02/15 20:02:46  davidk
 *     Modified the gap calculation (for the origin) in the locator, so that it is limited
 *     to P-class (within 110 degrees distance) and PKPdf phases.
 *     This way some flukily associated S or ancillary phase doesn't affect the
 *     quality of the gap metric.
 *
 *     Revision 1.6  2004/12/02 17:51:20  davidk
 *     Incorporated phase-based LocWeight values, that cause different phases
 *     to contribute different amounts to affecting the location. Utilized LocWeight
 *     in weight values submittted to the Solver, and in determining RMS, so
 *     that picks that aren't weighted in the solution to not adversly skew
 *     the RMS value even if they have a high residual value. (This is particularly
 *     important with phases like P'P', that have huge residual values, but should
 *     be associated with the origin, yet not contribute to it's location.
 *
 *     Revision 1.5  2004/11/30 02:48:52  davidk
 *     Found bug where input params for the dtdz vertical slowness parameter
 *     were being input in degrees (same unit as dtdx).  Converted the output
 *     from degrees to km, before using it to adjust the origin depth.
 *
 *     Inverted the dtdz coefficient so that it was negative instead, the same way
 *     the other coefficients were.
 *
 *     Revision 1.4  2004/11/01 06:33:21  davidk
 *     Modified to work with new hydra traveltime library.
 *     Cleaned up struct ITravelTime references.
 *
 *     Revision 1.3  2004/09/16 01:09:38  davidk
 *     Added a configable variable for the number of Iterations that the Locator-Solver runs
 *     each time an origin is modified.  Initialized it to the previously hardcoded value of 3-net-1.
 *
 *     Cleaned up logging messages.
 *     Downgraded some debugging messages.
 *
 *     Revision 1.2  2004/04/01 22:09:18  davidk
 *     v1.11 update
 *     Now utilizes OPCalc routines for calculating Origin/Pick association params.
 *
 *     Revision 1.3  2003/11/07 22:42:29  davidk
 *     Added RCS Header.
 *     Added function Locate_InitOrigin() that initializes a blank
 *     origin Structure for use with the locator.
 *     Added CompareOrigins() function for comparing the "quality"
 *     of two origin estimates for the same quake.
 *     Used function in code that formerly compared origins based on
 *     RMS values.
 *
 *     Revision 1.3  2003/11/07 22:38:59  davidk
 *     Added RCS Header.
 *     Added two member functions: Locate_InitOrigin() and CompareOrigins().
 *
 *
 **********************************************************/

// glock.cpp

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <Debug.h>
#include <ISolve.h>
#include <opcalc.h>
#include "glock.h"
#include "GlockMod.h"
//#include "date.h"
//#include "str.h"
extern "C" {
#include "utility.h"
}


extern "C" {
void __cdecl qsortd (double *base, unsigned num);
}


#define ITER_SUCCESS	0
#define	ITER_NODATA		1
#define	ITER_DEPTH		2

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994

// Carl 120303
static double pPickDistArray[OPCALC_MAX_PICKS];
static double pPickAzmArray[OPCALC_MAX_PICKS];

/*
static int PickAzmComp(const void *az1, const void *az2) {
	double azm1;
	double azm2;
	memmove(&azm1, az1, sizeof(double));
	memmove(&azm2, az2, sizeof(double));
	if(azm1 < azm2)
		return -1;
	if(azm1 > azm2)
		return 1;
	return 0;
}

// End Carl 120303

static int PickDistComp(const void *elem1, const void *elem2) {
	PICK *pck1;
	PICK *pck2;
	memmove(&pck1, elem1, sizeof(PICK *));
	memmove(&pck2, elem2, sizeof(PICK *));
	CDebug::Log(DEBUG_MINOR_INFO,"Comp() %.2f %.2f\n", pck1->dDelta, pck2->dDelta);
	if(pck1->dDelta < pck2->dDelta)
		return -1;
	if(pck1->dDelta > pck2->dDelta)
		return 1;
	return 0;
}
***************************************************/

//---------------------------------------------------------------------------------------CWave
CGlock::CGlock(CMod * IN_pMod) 
{
	for(int i=0; i<4; i++)
		bFree[i] = true;

	this->pMod = IN_pMod;
	CDebug::SetModuleName("CGlock");
	pGlnt = pMod->pGlint;
	pTT = pMod->pTT;
	OPCalc_SetTravelTimePointer(pTT);
	pSlv = pMod->pSolve;

}

//---------------------------------------------------------------------------------------~CWave
CGlock::~CGlock() {
}

//---------------------------------------------------------------------------------------Locate
// Locate earthquake with constraints. E.G. set tnez to "TZ" to allow free
// origin time and depth, tnez set to "TNE" is a fixed depth solution.
// T = Time, N = North, E = East, Z = Depth
int CGlock::Locate(char *ent, char *mask) {
	int i;
	int res;

  // Carl 120303  - reversed each one of the true/false definitions
	for(i=0; i<4; i++)
		bFree[i] = false;
	for(i=0; i<(int)strlen(mask); i++) {
		switch(mask[i]) {
		case 't':
		case 'T':
			bFree[0] = true;
			break;
		case 'n':
		case 'N':
			bFree[1] = true;
			break;
		case 'e':
		case 'E':
			bFree[2] = true;
			break;
		case 'z':
		case 'Z':
			bFree[3] = true;
			break;
		default:
			return 7001;
		}
	}
	res = Locate(ent);
	for(i=0; i<4; i++)
		bFree[i] = true;
	return res;
}

//---------------------------------------------------------------------------------------Locate
// Locate earthquake (retrieve data from glint, update event in glint)
int CGlock::Locate(char *ent) {
	PICK *pck;
	double torg;
	double dis;
	double azm;
  double dDistKM;
	int res;
	bool fix;
	bool btemp;

	pOrg = pGlnt->getOrigin(ent);
	if(!pOrg)
		return 1;
  if(pOrg->nPh < 4)  // we need atleast 4 phases for hypocenter processing 
    return(1);

	torg = pOrg->dT;
  nPck = 0;
  size_t iPickRef=0;
	while(pck = pGlnt->getPicksFromOrigin(pOrg, &iPickRef)) 
  {
    // DK 011404  record the min and max pick id's used in this
    //            location for debugging purposes
    if(nPck == 0)
    {
      strcpy(pOrg->idMinPick, pck->idPick);
      strcpy(pOrg->idMaxPick, pck->idPick);
    }
    else
    {
      if(strcmp(pOrg->idMinPick,pck->idPick) > 0)
        strcpy(pOrg->idMinPick, pck->idPick);
      else if(strcmp(pOrg->idMaxPick,pck->idPick) < 0)
        strcpy(pOrg->idMaxPick, pck->idPick);
    }

		geo_to_km_deg(pOrg->dLat, pOrg->dLon, pck->dLat, pck->dLon, &dDistKM, &dis, &azm);
//		CDebug::Log(DEBUG_MINOR_INFO,"Locate(): PCK:%s t:%.2f lat:%.4f lon:%.4f elev:%.4f dis:%.2f azm:%.2f\n",
//			pck->idPick, pck->dT-torg, pck->dLat, pck->dLon, pck->dElev, dis, azm);
		pPick[nPck++] = pck;
	}

  // DK Added 081303
  ORIGIN oBest, oCurrent, oNext;
  memcpy(&oCurrent,pOrg,sizeof(ORIGIN));
  Locate_InitOrigin(&oNext, pOrg->idOrigin);
  Locate_InitOrigin(&oBest, pOrg->idOrigin);

	fix = false;
  /* Note we must loop through two times before we start looping
     through locator iterations.  Something about the first time
     through calcs residuals for the existing origin, and the
     second time loads the solver's matrix.  So the 3rd Iterate()
     call is the first one that results in an actual relocation.
   **************************************************************/
	for(nIter=0; nIter<(pMod->iNumLocatorIterations+2); nIter++) {
		if(fix) {
			btemp = bFree[3];
			bFree[3] = false;
    }  // end if(fix)
    res = Iterate(true, &oCurrent, &oNext);
	  CDebug::Log(DEBUG_MINOR_INFO,"Locate(): Trying location: %.4f %.4f %.4f - %.2f (%.f/%.1f-%3d)\n", 
                oCurrent.dLat, oCurrent.dLon, oCurrent.dZ, 
                oCurrent.dT, oCurrent.dAff, oCurrent.dRms, oCurrent.nEq);
		if(fix) {
			fix = false;
			bFree[3] = btemp;
    }  // end if(fix)
    if(CompareOrigins(&oBest, &oCurrent) > 0)
      memcpy(&oBest,&oCurrent, sizeof(ORIGIN));

		switch(res) {
		case ITER_SUCCESS:
			break;
		case ITER_NODATA:
			pOrg->nEq = 0;
			pOrg->dRms = 0;
			return 0;
			break;
		case ITER_DEPTH:
			fix = true;
			break;
		}
    // make the next origin the current one and reprocess
    memcpy(&oCurrent,&oNext, sizeof(ORIGIN));
  }  // end for 3 iterations
  /* process the last origin */
  res = Iterate(false, &oCurrent, &oNext);
	  CDebug::Log(DEBUG_MINOR_INFO,"Locate(): Trying location: %.4f %.4f %.4f - %.2f (%.1f-%3d)\n", 
                oCurrent.dLat, oCurrent.dLon, oCurrent.dZ, 
                oCurrent.dT, oCurrent.dRms, oCurrent.nEq);
    if(CompareOrigins(&oBest, &oCurrent) > 0)
      memcpy(&oBest,&oCurrent, sizeof(ORIGIN));

  /* reset the originpick information to the best. */
  res = Iterate(false, &oBest, &oNext);

  if(oBest.nPh > 4)
    memcpy(pOrg,&oBest, sizeof(ORIGIN));
  else
  {
	  CDebug::Log(DEBUG_MAJOR_INFO,"Locate(): Unable to generate location using "
                                 "atleast 4 phases for Origin %s nPh:%d nEq:%d\n",
                oBest.idOrigin, oBest.nPh, oBest.nEq);
    return(1);
  }

	CDebug::Log(DEBUG_MINOR_INFO,"Locate(): Chose location : %.4f %.4f %.4f - %.2f (%.1f-%3d)\n", 
              oBest.dLat, oBest.dLon, oBest.dZ, 
              oBest.dT, oBest.dRms, oBest.nEq);
	return 0;
} // end Locate()

//---------------------------------------------------------------------------------------Iterate
int CGlock::Iterate(bool solve, ORIGIN * poCurrent, ORIGIN * poNext) 
{
	/*
  PICK *pck;
	TTT *ttt;
	double tres;
	double coef[4];
	int j;
	double tobs;
	double tcal;
	double d;
	double azm;
	double toa;
	double z;
	double sum;
	double torg;
	double lat;
	double lon;
	double dep;
  */

  int iRetCode;

	int i,j;
	double *stp;
	int nvar;
	int ivar;
	
	nvar = 0;
	for(i=0; i<4; i++)
		if(bFree[i]) nvar++;
	if(nvar < 1)
		solve = false;
	if(solve)
		pSlv->Init(nvar);

  iRetCode = ProcessLocation(solve, poCurrent, pPick, nPck);

	nIter++;
	if(poCurrent->nEq < 4)
		return ITER_SUCCESS;
	if(!solve)
		return ITER_SUCCESS;

	// Iterate solution
	stp = pSlv->Solve();
	j = 0;

  memcpy(poNext,poCurrent,sizeof(ORIGIN));
  poNext->nEq=0;
  poNext->dRms=10000.0;

	for(ivar=0; ivar<4; ivar++) {
		if(bFree[ivar]) {
			switch(ivar) {
			case 0:
				poNext->dT	+= stp[j++];
				break;
			case 1:
				poNext->dLat	+= stp[j++];
				break;
			case 2:
				poNext->dLon	+= stp[j++];
				break;
			case 3:
				poNext->dZ	+= stp[j++] * DEG2KM;
				break;
			}
		}
	}
	if(poNext->dZ < 0.0)
		return ITER_DEPTH;
	if(poNext->dZ > 800.0)
		return ITER_DEPTH;

  // DK 061903
  while(poNext->dLon > 180.0)
  {
    CDebug::Log(DEBUG_MAJOR_INFO,"Locate(): adjusting origin(%.2f,%6.2f,%7.2f,%3.0f) original(%.2f,%6.2f,%7.2f,%3.0f)\n",
                poNext->dT,poNext->dLat,poNext->dLon,poNext->dZ, poCurrent->dT,poCurrent->dLat,poCurrent->dLon,poCurrent->dZ);
    poNext->dLon-=360.0;
  }
  while(poNext->dLon < -180.0)
  {
    CDebug::Log(DEBUG_MAJOR_INFO,"Locate(): adjusting origin(%.2f,%6.2f,%7.2f,%3.0f) original(%.2f,%6.2f,%7.2f,%3.0f)\n",
                poNext->dT,poNext->dLat,poNext->dLon,poNext->dZ, poCurrent->dT,poCurrent->dLat,poCurrent->dLon,poCurrent->dZ);
    poNext->dLon+=360.0;
  }
  while(poNext->dLat > 90.0)
  {
    CDebug::Log(DEBUG_MAJOR_INFO,"Locate(): adjusting origin(%.2f,%6.2f,%7.2f,%3.0f) original(%.2f,%6.2f,%7.2f,%3.0f)\n",
                poNext->dT,poNext->dLat,poNext->dLon,poNext->dZ, poCurrent->dT,poCurrent->dLat,poCurrent->dLon,poCurrent->dZ);
    poNext->dLat=180.0 - poNext->dLat;
  }
  while(poNext->dLat < -90.0)
  {
    CDebug::Log(DEBUG_MAJOR_INFO,"Locate(): adjusting origin(%.2f,%6.2f,%7.2f,%3.0f) original(%.2f,%6.2f,%7.2f,%3.0f)\n",
                poNext->dT,poNext->dLat,poNext->dLon,poNext->dZ, poCurrent->dT,poCurrent->dLat,poCurrent->dLon,poCurrent->dZ);
    poNext->dLat=-180.0 - poNext->dLat;
  }

	// Update solution

  CDebug::Log(DEBUG_MINOR_INFO,"Locate(%s,%d): Solve moved origin(%.2f,%6.2f,%7.2f,%3.0f) original(%.2f,%6.2f,%7.2f,%3.0f)\n",
              poCurrent->idOrigin,poCurrent->iOrigin, poNext->dT,poNext->dLat,poNext->dLon,poNext->dZ, poCurrent->dT,poCurrent->dLat,poCurrent->dLon,poCurrent->dZ);
	return ITER_SUCCESS;

}  // end Iterate()

//---------------------------------------------------------------------------------------Affinity
// Calculates affinity factors for each associated phase. For now it is just the ratio
// of a stations distance to N times the modal distance, where N is to be determined.

#define GLOCK_WEIGHT_GRANULARITY  0.25

int CGlock::AddPickCoefficientsForSolve(PICK * pPick, const ORIGIN * pOrg)
{
  int j = 0;
  double coef[4];
  int i;

  if(bFree[0]) 
    coef[j++] = 1.0;
  if(bFree[1]) 
    coef[j++] = -cos(DEG2RAD*pPick->dAzm)*pPick->ttt.dtdx;
  if(bFree[2]) 
    coef[j++] = -sin(DEG2RAD*pPick->dAzm)*cos(DEG2RAD*pOrg->dLat) * pPick->ttt.dtdx;
  if(bFree[3]) 
    coef[j++] = -pPick->ttt.dtdz; 

  i = (int)((pPick->ttt.dLocWeight + (GLOCK_WEIGHT_GRANULARITY/2)) / GLOCK_WEIGHT_GRANULARITY);
  if(pSlv)
  {
    for(j=0; j < i; j++)
      pSlv->Equation(coef, pPick->tRes);
    return(0);
  }
  else
  {
    return(-1);
  }
}  // end AddPickCoefficientsForSolve()

// Function called by Iterate to calculate Origin and Origin/Link 
// params, and to suggest better locations.
int CGlock::ProcessLocation(bool solve, ORIGIN * pOrg, PICK ** pPick, int nPck) 
{

  // Initialize origin vars
	int rc = 0;
  int i;


  rc = OPCalc_ProcessOrigin(pOrg, pPick, nPck);
  if(rc)
    return(rc);



//  CDebug::Log(DEBUG_MAJOR_INFO,"Locate(I): Org(%6.3f,%8.3f,%5.1f - %.2f)\n",
//              pOrg->dLat, pOrg->dLon, pOrg->dZ, pOrg->dT);

	for(i=0; i<nPck; i++) 
  {
    // calculate solve coefficients if desired
    if(solve)
      AddPickCoefficientsForSolve(pPick[i], pOrg);
    
  }  // end for each pick in the Pick Array

  return(0);
}  // End Process Location()


// Function to initialize the comparison parameters for
// relative solution quality.
void  CGlock::Locate_InitOrigin(ORIGIN * pOrigin, const char * idOrigin)
{
  // set the comparison criteria to something really high/bad
  memset(pOrigin,0,sizeof(ORIGIN));
  pOrigin->dRms = 10000.0;
  strncpy(pOrigin->idOrigin, idOrigin, sizeof(pOrigin->idOrigin));
  pOrigin->idOrigin[sizeof(pOrigin->idOrigin)-1] = 0x00;
}


// Standard compare function.  returns -1 if Origin1 is "better"
// than Origin2, 1 if vice-versa, and 0 if they are of equal
// quality.
int CGlock::CompareOrigins(ORIGIN * po1, ORIGIN * po2)
{
  CDebug::Log(DEBUG_MINOR_INFO,
              "Comparing Origins:\n"
              "1) %.2f/%.2f/%.2f - %.2f  Aff: %.2f  AfGp: %.2f AfNeq: %.2f  RMS: %.2f Gap: %.2f\n"
              "2) %.2f/%.2f/%.2f - %.2f  Aff: %.2f  AfGp: %.2f AfNeq: %.2f  RMS: %.2f Gap: %.2f\n",
              po1->dLat, po1->dLon, po1->dZ, po1->dT, po1->dAff, po1->dAffGap, po1->dAffNumArr, po1->dRms, po1->dGap,
              po2->dLat, po2->dLon, po2->dZ, po2->dT, po2->dAff, po2->dAffGap, po2->dAffNumArr, po2->dRms, po2->dGap
             );

  if(po1->dAff > po2->dAff)
    return(-1);
  if(po1->dAff < po2->dAff)
    return(1);
  //else
  return(0);
}  // end CompareOrigins()



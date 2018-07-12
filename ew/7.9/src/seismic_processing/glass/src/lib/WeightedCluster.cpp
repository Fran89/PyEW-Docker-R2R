/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: WeightedCluster.cpp 2174 2006-05-22 15:15:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/22 15:15:05  paulf
 *     upgrade with hydra glass
 *
 *     Revision 1.2  2005/12/15 17:05:58  davidk
 *     completed initial run-testing and debugging.
 *
 *     Revision 1.1  2005/12/08 17:57:58  davidk
 *     Added WeightedCluster which is a replacement for Rank that includes
 *     station quality weighting.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:24  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2005/04/18 18:50:26  davidk
 *     Part of the integration of glass algorithms into hydra.
 *     Copied file out of glass/src/modules/glass into hydra libsrc_cpp.
 *     Cleaned up unneccesary includes.
 *
 *     Revision 1.2  2004/11/01 18:03:11  davidk
 *     rewrote the carlsortd() routine  in a feeble attempt to improve performance.
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:22  michelle
 *     New Hydra Import
 *
 *     Revision 1.3  2003/11/07 22:35:56  davidk
 *     Added RCS Header.
 *     Added function carlsortd(), and included it in lieu of
 *     qsortd() in the Cluster() function, because Carl's mutated
 *     insertion sort of part of the list is more efficient than
 *     a qsort of the entire list.
 *
 *
 **********************************************************/

//Rank.cpp: Calculate the ith largest number from list provided
//#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <WeightedCluster.hpp>
#include <string.h>

#define DSORT_BUFFER_SIZE 1000

typedef struct _ClusterDistanceStruct
{
  double dDist;
  double dWeight;
} ClusterDistanceStruct;

static double ClusterForPoint(PointStruct * pPoint, double dReqWeight, 
                              int iNumIntersections, IntersectStruct *IntersectArray);

static int carlsort(ClusterDistanceStruct * Array, int nArraySize, double dReqWeight);

static 	ClusterDistanceStruct cdsArray[DSORT_BUFFER_SIZE];  // used in main CWeightedCluster::Cluster function

//---------------------------------------------------------------------------------------CEntity
CWeightedCluster::CWeightedCluster() {
}

//---------------------------------------------------------------------------------------~CEntity
CWeightedCluster::~CWeightedCluster() {
}

//---------------------------------------------------------------------------------------Cluster
// Cluster() selects the best point from an array of points based on 
// residual distance.
// Best is determined by the proximity of the "irank"th closest point 
// to the point being evaluated.  So if there are 10 points in the
// array, and irank = 4, for each point, the 4th closest point to
// it will be calculated, and it's distance stored in "a".  The point
// with the smallest associated "a" is returned.
double CWeightedCluster::Cluster(double dReqWeight, int iNumIntersections, 
                                 IntersectStruct *IntersectArray, 
                                 PointStruct **ppBestPoint)
{
	int ibest = -1;
	double abest = 1.0e32;
	int i,j;
	double a;

  *ppBestPoint = NULL;

  // STEP 1: Insure a solution is possible.  If the point of interest
  //         is greater than the number of points in the array, then
  //         there is no solution.  (There are 4 points in the array,
  //         and we're evaluating the 5th closest point.)
  //         If not, return error.
	if(dReqWeight > iNumIntersections * STA_WEIGHT_MAX) 
		return abest;

  // STEP 2: For each point in the array, calculate the distance
  //         of the "irank"th closest point to it.
  //         Save the distance/point num of the best point.
	for(i=0; i<iNumIntersections; i++) 
  {
	  for(j=0; j<IntersectArray[i].iNumPts; j++) 
    {
      // STEP 2.1: 
      //       Calculate the "irank"th closest point to the given one.
      a = ClusterForPoint(&IntersectArray[i].pt[j], dReqWeight, iNumIntersections, IntersectArray);
      // STEP 2.2: 
      //       If the current point is the best so far, save it and it's
      //       associated distance.
      if(a < abest) 
      { 
        abest = a;
        *ppBestPoint = &IntersectArray[i].pt[j];
      }
    }  // end for each point in this intersection 
	}


  // STEP 3: Done.  Return the best point and it's associated residual
  //         distance, as discovered in the for() loop above.
	return abest;
}


// CLEANUP : WARNING this code is no longer threadsafe

static 	double xd,yd,zd;
static  double x,y,z;
static  double dDist;

//---------------------------------------------------------------------------------------Cluster
// Calculate the distance beteween a given point, and all the points in an array.
// Return the distance of the "irank"th closest point from the given one.
// We don't care which point the "irank"th closest one is, just it's distance.
static double ClusterForPoint(PointStruct * pPoint, double dReqWeight, int iNumIntersections, IntersectStruct *IntersectArray)
{


  // Vector distances of one point in the array from a given point.
  

	int i,j,k;

  // STEP 1: Calculate the number of points to process.  Try to process
  // all, but do not overflow the storage array.
  // (set safe nXYZ based on nxyz and cdsArray array-size)
  int nXYZ = (iNumIntersections > DSORT_BUFFER_SIZE) ? DSORT_BUFFER_SIZE : iNumIntersections;

  x = pPoint->x;
  y = pPoint->y;
  z = pPoint->z;

  k=0;

  // STEP 3: For each point in the array, calculate the distance between that 
  //         point and the given one.  Store the distance in the "cdsArray" 
  //         distance array.
  for(i=0; i < nXYZ; i++)
  {
    if(!IntersectArray[i].iNumPts)  // no points in this intersection, then ignore this intersection.
      continue;
    cdsArray[k].dDist = 1.0e32;
    cdsArray[k].dWeight = IntersectArray[i].dWeight;
    for(j=0; j < IntersectArray[i].iNumPts; j++)
    {
      xd = x - IntersectArray[i].pt[j].x;
      yd = y - IntersectArray[i].pt[j].y;
      zd = z - IntersectArray[i].pt[j].z;
      dDist = xd*xd + yd*yd + zd*zd;      
      if(dDist < cdsArray[k].dDist)
        cdsArray[k].dDist = dDist;
    }
    k++;
  }

  // STEP 4: Sort the array in ascending order.
  i=carlsort(cdsArray, k, dReqWeight);

  // STEP 5: Return the "irank"th closest distance.
  if(i > 0)
	  return sqrt(cdsArray[i-1].dDist);
  else
    return(1.0e32);
}  /* end ClusterForPoint() */




static ClusterDistanceStruct * pInsertion;
static ClusterDistanceStruct *pEnd, *pCurr, *pBest;
static double   dBest, dAccumulatedWeight;
static ClusterDistanceStruct  cdsTemp;

// this function sorts an array of nArraySize,
// returning a buffer with the smallest nCutOff
static int carlsort(ClusterDistanceStruct * Array, int nArraySize, double dReqWeight)
{

	dAccumulatedWeight = 0.0;

  pEnd  = Array + nArraySize;

  // for each item in the array.  Check to see if the item is
  // is one of the nCutoff smallest of the items evaluated
  // so far.
	for(pInsertion=Array; (dAccumulatedWeight < dReqWeight) &&(pInsertion < pEnd); pInsertion++)
	{
	
    pBest   = pInsertion;
    dBest   = pInsertion->dDist;

    for(pCurr = pInsertion + 1; pCurr < pEnd; pCurr++)
    {
      if(pCurr->dDist < dBest)
      {
        pBest = pCurr;
        dBest = pCurr->dDist;
      }
    }
    memcpy(&cdsTemp,   pInsertion, sizeof(cdsTemp));
    memcpy(pInsertion, pBest,      sizeof(cdsTemp));
    memcpy(pBest,      &cdsTemp,   sizeof(cdsTemp));

    dAccumulatedWeight += pInsertion->dWeight;
  }
  if(dAccumulatedWeight < dReqWeight)
    return(0 -(pInsertion-Array));
  else
    return(pInsertion - Array);
}  // end carlsort()

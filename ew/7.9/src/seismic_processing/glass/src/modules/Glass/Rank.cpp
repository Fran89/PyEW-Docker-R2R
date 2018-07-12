/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: Rank.cpp 2176 2006-05-22 16:04:15Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/05/22 16:01:25  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
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
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "rank.h"
extern "C" {
#include "utility.h"
}

//---------------------------------------------------------------------------------------CEntity
CRank::CRank() {
}

//---------------------------------------------------------------------------------------~CEntity
CRank::~CRank() {
}

//---------------------------------------------------------------------------------------Bundle
// For each point in the array of vectors xyz (dimension 3*xyz), construct a list of
// the angular distance between each and all the others. Out of these nxyz lists,
// choose the one such the the magnitude of the irank largest is a minimum, and return
// that index. This is a kind of angular clustering analysis.
// In the following implementation it is assumed that all vectors are of the same
// magnitude, that is their heads fall on a sphere, although it is not required that
// they be unit vectors.
int CRank::Bundle(int irank, int nxyz, double *xyz, double *best) {
	int isort;
	int jsort;
	int nsort = 0;
	int ibest = -1;
	double dot;
	double dbest = 0.0;
	double dsort[100];
	int i;
	int j;

//	TRACE("Bundle irank:%d nxyz:%d\n", irank, nxyz);
	if(irank > nxyz) {
		*best = 0.0;
		return -1;
	}
	for(i=0; i<nxyz; i++) {
		nsort = 0;
		for(j=0; j<nxyz; j++) {
			dot = xyz[3*i  ]*xyz[3*j  ]
				+ xyz[3*i+1]*xyz[3*j+1]
				+ xyz[3*i+2]*xyz[3*j+2];
		//	TRACE("  dot[%d] = %.2f\n", j, sqrt(dot));
			jsort = nsort;
			for(isort=0; isort<nsort; isort++) {
				if(dot > dsort[isort]) {
					jsort = isort;
					break;
				}
			}
			for(isort=nsort; isort>jsort; isort--)
				dsort[isort] = dsort[isort-1];
			dsort[jsort] = dot;
			nsort++;
			if(nsort > irank)
				nsort = irank;
		}
	//	for(j=0; j<nsort; j++)
	//		TRACE("  dsort[%d] %.2f\n", j, sqrt(dsort[j]));
		if(dsort[nsort-1] > dbest) {
			dbest = dsort[nsort-1];
			ibest = i;
		}
	//	TRACE("  ibest:%d dbest:%.2f\n", ibest, sqrt(dbest));
	}
	*best = dbest;
	return ibest;
}

//---------------------------------------------------------------------------------------Cluster
double CRank::Cluster(int irank, int nxyz, double *xyz) {
	double abest = 1.0e32;
	int i;
	double a;

	if(irank < nxyz)
		return abest;
	for(i=0; i<nxyz; i++) {
		a = Cluster(irank, nxyz, xyz, xyz[3*i], xyz[3*i+1], xyz[3*i+2]);
		if(a < abest)
			abest = a;
	}
	return abest;
}

//---------------------------------------------------------------------------------------Cluster
// Cluster() selects the best point from an array of points based on 
// residual distance.
// Best is determined by the proximity of the "irank"th closest point 
// to the point being evaluated.  So if there are 10 points in the
// array, and irank = 4, for each point, the 4th closest point to
// it will be calculated, and it's distance stored in "a".  The point
// with the smallest associated "a" is returned.
double CRank::Cluster(int irank, int nxyz, double *xyz, int *ixyz) {
	int ibest = -1;
	double abest = 1.0e32;
	int i;
	double a;

  // STEP 1: Insure a solution is possible.  If the point of interest
  //         is greater than the number of points in the array, then
  //         there is no solution.  (There are 4 points in the array,
  //         and we're evaluating the 5th closest point.)
  //         If not, return error.
	if(irank > nxyz) {
		*ixyz = -1;
		return abest;
	}

  // STEP 2: For each point in the array, calculate the distance
  //         of the "irank"th closest point to it.
  //         Save the distance/point num of the best point.
	for(i=0; i<nxyz; i++) {
    // STEP 2.1: 
    //       Calculate the "irank"th closest point to the given one.
		a = Cluster(irank, nxyz, xyz, xyz[3*i], xyz[3*i+1], xyz[3*i+2]);
    // STEP 2.2: 
    //       If the current point is the best so far, save it and it's
    //       associated distance.
		if(a < abest) {
			abest = a;
			ibest = i;
		}
	}


  // STEP 3: Done.  Return the best point and it's associated residual
  //         distance, as discovered in the for() loop above.
	*ixyz = ibest;
	return abest;
}

extern "C" {
void __cdecl qsortd (double *base, unsigned num);
}

static int dComp(const void *elem1, const void *elem2 )
{
  double d1 = *((double *) elem1);
  double d2 = *((double *) elem2);

  if(d1<d2)
    return(-1);
  else if(d1>d2)
    return(1);
  else
    return(0);
}

// DK 080503  Increased the size of dsort from 100 to 1000.
//            100 seemed small in the case of noisy stations
//            or aftershocks.
//            dsort is used to sort 
static 	double dsort[1000];  // used in main CRank::Cluster function
// CLEANUP : WARNING this code is no longer threadsafe


static void carlsortd(double * dArray, int nArraySize, int nCutoff);

//---------------------------------------------------------------------------------------Cluster
// Calculate the distance beteween a given point, and all the points in an array.
// Return the distance of the "irank"th closest point from the given one.
// We don't care which point the "irank"th closest one is, just it's distance.
double CRank::Cluster(int irank, int nxyz, double *xyz,
			   double x, double y, double z) {


  // Vector distances of one point in the array from a given point.
	double xd;
	double yd;
	double zd;


  // STEP 1: Calculate the number of points to process.  Try to process
  // all, but do not overflow the storage array.
  // (set safe nXYZ based on nxyz and dsort array-size)
  int nXYZ = (nxyz > sizeof(dsort)/sizeof(double)) ? (sizeof(dsort)/sizeof(double)) : nxyz;

	int i;

  // STEP 2: Check irank to see if it's reasonable.  We can't return the 
  //         distance of the 5th closest point if we only have 4 to work with.
	if(irank > nXYZ || irank <= 0)
		return 1.0e32;


  // STEP 3: For each point in the array, calculate the distance between that 
  //         point and the given one.  Store the distance in the "dsort" 
  //         distance array.
  for(i=0; i < nXYZ; i++)
  {
		xd = x - xyz[3*i];
		yd = y - xyz[3*i+1];
		zd = z - xyz[3*i+2];
		dsort[i] = xd*xd + yd*yd + zd*zd;
  }

  // STEP 4: Sort the array in ascending order.
  carlsortd(dsort, nXYZ, irank);

  // STEP 5: Return the "irank"th closest distance.
	return sqrt(dsort[irank-1]);

/***************************************
    OLD FUNCTION

  int isort;
	int jsort;
	int nsort = 0;
	int ibest = -1;
	double r2;
	double xd;
	double yd;
	double zd;
	int i;

	nsort = 0;
	for(i=0; i<nxyz; i++) {
		xd = x - xyz[3*i];
		yd = y - xyz[3*i+1];
		zd = z - xyz[3*i+2];
		r2 = xd*xd + yd*yd + zd*zd;
		jsort = nsort;
		for(isort=0; isort<nsort; isort++) {
			if(r2 < dsort[isort]) {
				jsort = isort;
				break;
			}
		}
		for(isort=nsort; isort>jsort; isort--)
			dsort[isort] = dsort[isort-1];
		dsort[jsort] = r2;
		nsort++;
		if(nsort > irank)
			nsort = irank;
	}

//	for(i=0; i<nsort; i++) 
//		TRACE("dsort[%d] = %.2f\n", i, dsort[i]);
	return sqrt(dsort[irank-1]);

  
    OLD FUNCTION
***************************************/


}



static int iCurrentSortedItem;
static 	int iInsertionPoint;
static 	int nSortedItems = 0;
static 	double dResSquared;
static 	int iCurrentIndex;

static double * pInsertion;
static double * pStop, *pEnd, *pCurr;
static double   dBest, *pBest;
static double   dTemp;

// this function sorts an array of nArraySize,
// returning a buffer with the smallest nCutOff
static void carlsortd(double * dArray, int nArraySize, int nCutoff)
{

	nSortedItems = 0;

  pStop = dArray + nCutoff;
  pEnd  = dArray + nArraySize;

  // for each item in the array.  Check to see if the item is
  // is one of the nCutoff smallest of the items evaluated
  // so far.
	for(pInsertion=dArray; pInsertion < pStop; pInsertion++)
	{
	
    pBest = pInsertion;
    dBest = *pInsertion;

    for(pCurr = pInsertion + 1; pCurr < pEnd; pCurr++)
    {
      if(*pCurr < dBest)
      {
        pBest = pCurr;
        dBest = *pCurr;
      }
    }
    dTemp       = *pInsertion;
    *pInsertion = dBest;
    *pBest      = dTemp;
  }
}  // end carlsortd()

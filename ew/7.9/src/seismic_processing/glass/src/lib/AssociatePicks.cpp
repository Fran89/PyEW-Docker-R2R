
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <ITravelTime.h>
#include <IGlint.h>
//#include "date.h"
#include "Sphere.h"
#include <Rank.h>
#include <WeightedCluster.hpp>

// Max time window for P arrival for primary association
#define MAX_P_ASSOCIATE_TIME 840.0  

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994
#define MAXPCK 1000

static int nXYZ;


static IntersectStruct dXYZPointArray[MAXPCK];
static double dXYZ[6*MAXPCK];

double AssociatePicks(double torg, double depth, double *lat, double *lon, 
                      PICK ** Pck, int nPck, ITravelTime	* pTT, double dNCutWeight,
                      bool bUseStaQual)
{
	TTEntry ttt, *pttt;
	PICK *pck;
	CSphere shell;
	CSphere sph1;
	CSphere sph2;
	double x;
	double y;
	double z;
	double xy;
	double d;
	double r;
	double t;
	double r2;
  double dCutWeight;
	int i;
	int n;

	double re = 6371.0;  // radius of the earth in km
	double rs = re - depth;  // radius of the current depth shell in km
	double rlat;  // latitude (in radians)
	double rlon;  // longitude (in radians)


  // STEP 1: Associate() works by finding the intersections of spheres.  Set
  //         the first sphere to be the given depth shell.  That way we
  //         won't locate any hypocenters that aren't at the given depth.

  // DK CLEANUP  Why is the sphere centered at 0.1 instead of 0.0 ???
//	shell.Set(0.1, 0.1, 0.1, rs);  // set the first sphere to be the depth
	shell.Set(0.0, 0.0, 0.0, rs);  // set the first sphere to be the depth
                                 // shell, centered at(NEAR) the earth's 
                                 // center, with a radius = (re - depth)
  
	nXYZ = 0;                // reset the number of intersections to 0

  // STEP 2: Process each pick in the Pck array.
  //         attempting to calculate it's intersection with
  //         the given pick and the depth shell.
	for(i=0; i<nPck; i++) 
  {
		pck = Pck[i];

    // STEP 2.1: 
    //       Calculate parameters for the current pick:
    //         rlat:  latitude of pick (in radians)
    //         rlon:  longitude of pick (in radians)
    //         x,y,z: xyz coordiantes of pick (in km's) from
    //                earth's center
    //         t:     deltaT between trial origin-time and pick-time
		rlat = DEG2RAD*pck->dLat;
		rlon = DEG2RAD*pck->dLon;
		z = re*sin(rlat);
		xy = re*cos(rlat);
		x = xy*cos(rlon);
		y = xy*sin(rlon);
		t = pck->dT - torg;

    // STEP 2.2: 
    //       Ignore picks that aren't within P travel time of origin
		if(t < 1.0)
			continue;
		if(t > MAX_P_ASSOCIATE_TIME)
			continue;
      //		Debug("Before pTT->D() %d t:%.2f depth:%.2f\n", pTT, t, depth);


    // STEP 2.3: 
    //       Obtain the most likely traveltime record based upon
    //       P-phase type(Table 0), deltaT, and depth.
		pttt = pTT->DBestByClass(depth,t,180.0/* deg */,&ttt,PHASECLASS_P);

    // STEP 2.4: 
    //       If there was no suitable matching traveltime record,
    //       ignore the current phase.  Continue to the next.
		if(!pttt)
			continue;


    // STEP 2.5: 
    //       Set the radial distance from the obtained traveltime record.
		d = pttt->dDPhase;

    // STEP 2.6: 
    //       Calculate the radius(km) of the sphere "r" for the
    //       current pick.
    //       Calculated by finding the third side of a triangle
    //       defined by three endpoints:
    //             m) earth's center
    //             l) pick location
    //             n) a spot rs(km) from earth's center
    //       and angle (lmn) = d(radial distance)
    //       (*Note: ml = re, mn = rs)

    // r2 = r**2
		r2 = rs*rs + re*re - 2*rs*re*cos(DEG2RAD*d);
		if(r2 < 0.0)
    {
      // CHANGE TO MINOR ERROR!!!  DK 073103  - this shouldn't be possible
      /* CDebug::Log(DEBUG_MINOR_ERROR,"CGlass::Associate():  "
                                    "Invalid calculation(1): re=%.0f rs=%.0f"
                                    "depth=%.0f t=%.2f d=%.0f r2=%.3f\n",
                  re,rs,depth,t,d,r2); */
			continue;
    }
		r = sqrt(r2);

    // STEP 2.7: 
    //       If this is the given pick (the first pick)
    //       then record the properties of it's sphere (center=x,y,z radius=r)
    if(!i)
    {
			sph1.Set(x, y, z, r);
      if(bUseStaQual)
        dCutWeight = dNCutWeight - pck->dStaQual;
    }
    // STEP 2.8: 
    //       If this is not the given pick (the first pick)
    //       then attempt to calculate the intersection
    //       between three spheres:
    //         the sphere around this pick:      (center=x,y,z radius=r)
    //         the sphere around the given pick: sph1
    //         the sphere for the depth shell:   shell
    else  //if(i) 
    {
			sph2.Set(x, y, z, r);
			n = shell.Intersect(&sph1, &sph2, &dXYZ[nXYZ*3]);
      if(bUseStaQual)
      {
        dXYZPointArray[i].iNumPts = n;
        dXYZPointArray[i].dWeight = pck->dStaQual;
        if(n>0)
        {
          /* this is cheating, but it's QUICK */
          memcpy(&dXYZPointArray[i].pt[0], &dXYZ[nXYZ], sizeof(dXYZPointArray[i].pt[0]));
        }
        if(n>1)
        {
          /* this is cheating, but it's QUICK */
          memcpy(&dXYZPointArray[i].pt[1], &dXYZ[nXYZ+3], sizeof(dXYZPointArray[i].pt[1]));
        }
      }
      nXYZ+=n;
    }

	}// end for each pick in Pck[]

  // STEP 3: If we didn't find atleast one (1) intersection
  //         point, quit and return false (huge distance)

  if(nXYZ == 0)
  {
		return 1.0e32;
  }

  //else


	double best;
	PointStruct *pBest;
  int iBest;
	PointStruct Best;

  // STEP 4: Go through the list of pick intersection points
  //         (aka list of potential origins)
  //         and attempt to pick the best one.
  //         (the one whose nth residual(km) is the smallest,
  //          where n is defined by nCut).
  if(bUseStaQual)
  {
	  best = CWeightedCluster::Cluster(dCutWeight, nXYZ, dXYZPointArray,&pBest);
  }
  else
  {
	  best = CRank::Cluster((int)dNCutWeight, nXYZ, dXYZ, &iBest);
    if(iBest < 0)
    {
      pBest = NULL;
    }
    else
    {
      Best.x = dXYZ[iBest*3+0];
      Best.y = dXYZ[iBest*3+1];
      Best.z = dXYZ[iBest*3+2];
      pBest = &Best;
    }
  }
  // STEP 5: If we didn't find atleast one (1) acceptable origin,
  //         quit and return false (huge distance)
  //         (*Note: nCut and dCut define standards as to how many
  //                 picks must intersect within a given tolerance(km)
  //                 in order to generate an acceptable location )
	if(pBest == NULL) {
		*lat = 0.0;
		*lon = 0.0;
		return 1.0e32;
	}


  // STEP 6: Calculate origin lat and lon from the x,y,z parameters stored
  //         in the point(dXYZPointArray) array.
  //         Copy the lat and lon out into the caller's variables.
	x = pBest->x;
	y = pBest->y;
	z = pBest->z;
	r = sqrt(x*x + y*y + z*z);
	xy = sqrt(x*x + y*y);
	*lat = RAD2DEG*atan2(z, xy);
	*lon = RAD2DEG*atan2(y, x);

/*********************************
  DK CLEANUP    {possible algorithm change}.
  The original glass was done with a single P travel
  time that was a conglomeration of varying P phases.
  Using the new traveltime library, there is now the
  potential to have multiple different P phases that
  could associate.  How do you tell which one to use.
  For now, we use the furthest reaching P phase for
  a given traveltime/depth.  (I believe this is close
  to how the origin P-conglomerate table was built.)
  Other possibilities, are to take the closest P phase
  to a guessed distance, or to iterate through all
  possible P phases.
 ************************************/

  // STEP 7: Return the distance of the nCut closest pick intersection(from the given pick)
  //         If nCut = 4, then return the residual distance(km) of the 4th closest
  //         pick intersection to the origin.
  //         This is the value returned by Cluster(), which ranks the potential
  //         origins discovered in the for() loop above.
	return best;
}


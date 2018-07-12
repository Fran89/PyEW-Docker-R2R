/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: trav.cpp 2176 2006-05-22 16:04:15Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/05/22 16:01:25  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.2  2004/04/01 21:52:43  davidk
 *     v1.11 update
 *     Added PPD to traveltime tables and associated code
 *
 *     Revision 1.6  2003/11/07 19:12:41  davidk
 *     Added RCS comment header.
 *
 *     Revision 1.4  2003/08/11 23:05:32  davidk
 *     build11 fixed bug that looked up the phase number in T(), it was using the distance
 *     array, not the time array.
 *
 *     Revision 1.3  2003/08/11 20:24:05  davidk
 *     Fixed bug in T(), where dDelT was being used instead of dDelD,
 *     possibly causing array/buffer overflows.
 *
 *     Revision 1.2  2003/08/07 22:24:09  davidk
 *     patched for build 9.
 *
 *     Revision 1.2  2003/08/07 22:20:12  davidk
 *     patched for v0.0 build 9
 *
 *
 ***********************************************************/
// Trav.cpp : implementation file
//
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "trav.h"
#include "terra.h"
#include "ray.h"
#include "str.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994


/***********************************************************
 * DK 080703
 *  This class uses a set of travel time tables.
 *    In fact, each class uses 8 tables (implemented as 4 arrays):
 *      1)  iT[D][Z]  table of phaseIDs by Angle(D), Depth(Z)
 *      2)  dT[D][Z]  table of values by  Angle(D), Depth(Z)
 *            value 1:  Time(T) for phase 
 *            value 2:  Take Off Angle(TOA) for phase
 *      3)  iD[T][Z]  table of phaseIDs by Time(T), Depth(Z)
 *      4)  dD[T][Z]  table of values by Time(T), Depth(Z)
 *            value 1:  Angle(D)(radians) for phase
 *            value 2:  Take Off Angle(TOA) for phase
 *
 *   If this class is not CPU Critical, it might be useful
 *   to make it much more readable, by creating two lookup
 *   tables instead of four arrays, and using a single struct
 *   with PhaseID, TOA, and Angle(D) or Time(T)         
 *
 *   I believe this class, and especially the D() function 
 *   are CPU critical, and thus they should be kept optimized
 *   as much as possible.  I am leaving the code alone, but
 *   adding extra comments.
 *
 ***********************************************************/

/////////////////////////////////////////////////////////////////////////////
// CTrav

CTrav::CTrav()
{
	dT = NULL;
	iT = NULL;
	dD = NULL;
	iD = NULL;
	pTerra = NULL;
	bCreate = FALSE;
	nPhase = 0;
	nD = 0;
	nT = 0;
	nZ = 0;
	strcpy(cTrv, "Not Saved");
  DebugOutputStruct dos = {1,0,0,0,0};
  CDebug::SetLevelParams(DEBUG_MINOR_INFO,&dos);
}

CTrav::~CTrav()
{
	if(dT != NULL)	delete [] dT;
	if(iT != NULL)	delete [] iT;
	if(dD != NULL)	delete [] dD;
	if(iD != NULL)	delete [] iD;
}

void CTrav::Init(CTerra *terra) {
	pTerra = terra;
	bCreate = TRUE;
}

//------------------------------------------------------------------------------RangeD
// Specify distance range in degress
void CTrav::RangeD(double dmin, double dinc, double dmax) {
	dMinD = dmin;
	dDelD = dinc;
	dMaxD = dmax;
	nD = (int)((dMaxD - dMinD)/dDelD + 0.5);
	dDelD = (dMaxD - dMinD)/nD;
	nD++;
}

//------------------------------------------------------------------------------RangeT
// Specify depth range in kilometers
void CTrav::RangeZ(double zmin, double zinc, double zmax) {
	dMinZ = zmin;
	dDelZ = zinc;
	dMaxZ = zmax;
	nZ = (int)((dMaxZ - dMinZ)/dDelZ + 0.5);
	if(nZ < 1) {
		dDelZ = 0.0;
		nZ = 1;
	} else {
		dDelZ = (dMaxZ - dMinZ)/nZ;
		nZ++;
	}
}

//------------------------------------------------------------------------------Phase
// Append phase to phase list
void CTrav::Phase(char *phs) {
	if(nPhase < MAXPHS)
		cPhase[nPhase++] = CStr(phs);
}

//------------------------------------------------------------------------------Load
// Load travel-time table from disk
void CTrav::Load(char *file) {
	FILE *fttt;
	char cphs[16*MAXPHS];
	int ndz;
	int ndt;
	int magic;
	int i;

	strcpy(cTrv, file);
  if(!(fttt = fopen(file, "rb")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTrav::Load(): ERROR!  Could not open travel-time table file (%s)\n",
                file);
		return;
  }
	fread(&magic, sizeof(magic), 1, fttt);
	if(magic != 777) {
//		sprintf(txt, "Invalid magic number <%d> in file <%s>", magic, file);
//		AfxMessageBox(txt);
    // DK CLEAUP!!!
		ExitProcess(-1);
	}
	fread(&nPhase, sizeof(nPhase), 1, fttt);
	fread(&nD, sizeof(nD), 1, fttt);
	fread(&nT, sizeof(nT), 1, fttt);
	fread(&nZ, sizeof(nZ), 1, fttt);
	fread(&dMinT, sizeof(dMinT), 1, fttt);
	fread(&dMaxT, sizeof(dMaxT), 1, fttt);
	fread(&dMinD, sizeof(dMinD), 1, fttt);
	fread(&dMaxD, sizeof(dMaxD), 1, fttt);
	fread(&dMinZ, sizeof(dMinZ), 1, fttt);
	fread(&dMaxZ, sizeof(dMaxZ), 1, fttt);
	dDelD = (dMaxD - dMinD)/(nD-1);
	dDelT = (dMaxT - dMinT)/(nT-1);
	dDelZ = (dMaxZ - dMinZ)/(nZ-1);
	fread(cphs, 16*nPhase, 1, fttt);
	for(i=0; i<nPhase; i++) {
		cPhase[i] = CStr(&cphs[16*i]);
	}
	ndz = nD*nZ;
	ndt = nT*nZ;

  dT = new double[2*ndz];
	iT = new int[ndz];
	dD = new double[2*ndt];
	iD = new int[ndt];
  dPPD = new float[ndz];
	fread(iT, sizeof(iT[0]),   ndz, fttt);
	fread(dT, sizeof(dT[0]), 2*ndz, fttt);
	fread(dPPD, sizeof(dPPD[0]), ndz, fttt);
	fread(iD, sizeof(iD[0]),   ndt, fttt);
	fread(dD, sizeof(dD[0]), 2*ndt, fttt);
	fclose(fttt);
  fttt = NULL;
}


//------------------------------------------------------------------------------D & T
// static variables for use in D() and T()  Made static for improved performance
static 	int iDepthRow;
static 	int iDistCol;
static 	int iTableIndex;
static 	int it;
static 	double dTime;

static 	int iz;
static 	int itz;
static 	int itz2;
static 	int iMax;
static 	int index00;
static 	int index01;
static 	int index10;
static 	int index11;

static 	int iq;

static 	double d00;
static 	double d01;
static 	double d10;
static 	double d11;
static 	double s;
static 	double q;
static 	double sm;
static 	double qm;
static 	double q00;
static 	double q01;
static 	double q10;
static 	double q11;
static 	double qmax;
static 	double toa00;
static 	double toa01;
static 	double toa10;
static 	double toa11;
static 	double d;

//------------------------------------------------------------------------------T
// Interpolate first arrival travel time for given distance (degrees)
// Bilinear interpolation
double CTrav::T(double dDist, double z) 
{

  // Step 1:  T() is a mirror of the D() function.
  //          T() looks up the time of a phase from a table
  //          based on it's Depth and Angular Distance;
  //          D() looks up the Angular Distance of a phase from a table
  //          based on it's Depth and Time.
  //          Please see the comments in the D() function for a description
  //          of how both functions work.

  iPhase = -1;
	if(dDist < dMinD || dDist > dMaxD)
		return -1.0;
	if(z < dMinZ || z > dMaxZ)
		return -2.0;   //DK C original code returns -1.0 here

  // DK Change bug in v0b9  (use deltaD not delta T)
  //iDistCol = (int)((dDist - dMinD)/dDelT);
  iDistCol = (int)((dDist - dMinD)/dDelD);
	iDepthRow = (int)((z - dMinZ)/dDelZ);
	iTableIndex = iDepthRow*nD+iDistCol;
	itz2 = 2*iTableIndex;
  iMax = (nD*nZ - 1) * 2;  // DK 080703  Set this to the last
                           //            index in the array.
  index00 = itz2;
  index01=itz2+2;
  index10=itz2 + 2*nD;
  index11=itz2 + 2*nD + 2;

  /* DK 080703
     If z = dMaxZ or dDist = dMaxD, then index01, index10, 
     and index11 can all overflow the array, even
     though they won'dDist be used in the calculation.
     Set any overflowing indexes to iMax.  Their value
     is not an issue as long as iDistCol's legal, because
     their data will not be used in the calculation
   ***********************************************/
  if(index01 > iMax) index01= iMax;
  if(index10 > iMax) index10= iMax;
  if(index11 > iMax) index11= iMax;

	d00 = dT[index00];
	d01 = dT[index01];
	d10 = dT[index10];
	d11 = dT[index11];

  if(d00 < 0.0)	return -3.0;  //DKC all -1 in original call
	if(d01 < 0.0)	return -4.0;  //DKC all -1 in original call
	if(d10 < 0.0)	return -5.0;  //DKC all -1 in original call
	if(d11 < 0.0)	return -6.0;  //DKC all -1 in original call

  // DK Change bug in v0b9  (use deltaD not delta T)
  // s = (dDist - dMinD - iDistCol*dDelT)/dDelT;
  s = (dDist - dMinD - iDistCol*dDelD)/dDelD;
	q = (z - dMinZ - iDepthRow*dDelZ)/dDelZ;
	sm = 1.0 - s;
	qm = 1.0 - q;
	q00 = sm * qm;
	q01 = s * qm;
	q10 = sm * q;
	q11 = s * q;
	iq = iTableIndex;
	qmax = q00;

	if(q01 > qmax) {
		qmax = q01;
		iq = iTableIndex+1;
	}
	if(q10 > qmax) {
		qmax = q10;
		iq = iTableIndex + nD;
	}
	if(q11 > qmax) {
		qmax = q11;
		iq = iTableIndex + nD + 1;
	}
	iPhase = iT[iq];  // DK 081103 Change from iD to iT v0.0b11

  dTime = d00 * q00 + d01 * q01 + d10 * q10 + d11 * q11;

  toa00 = dT[index00 + 1];
	toa01 = dT[index01 + 1];
	toa10 = dT[index10 + 1];
	toa11 = dT[index11 + 1];

	dToa = toa00 * q00 + toa01 * q01 + toa10 * q10 + toa11 * q11;
  //CDebug::Log(DEBUG_MINOR_INFO,"CTrav::T(%.3f,%.2f) = %.2f\n",dDist,z,dTime);
	return dTime;
}  // end T()



//------------------------------------------------------------------------------T
// Interpolate first arrival travel time for given distance (degrees)
// Bilinear interpolation
double CTrav::PPD(double dDist, double z) 
{

  // Step 1:  T() is a mirror of the D() function.
  //          T() looks up the time of a phase from a table
  //          based on it's Depth and Angular Distance;
  //          D() looks up the Angular Distance of a phase from a table
  //          based on it's Depth and Time.
  //          Please see the comments in the D() function for a description
  //          of how both functions work.

  iPhase = -1;
	if(dDist < dMinD || dDist > dMaxD)
		return -1.0;
	if(z < dMinZ || z > dMaxZ)
		return -2.0;   //DK C original code returns -1.0 here

  // DK Change bug in v0b9  (use deltaD not delta T)
  //iDistCol = (int)((dDist - dMinD)/dDelT);
  iDistCol = (int)((dDist - dMinD)/dDelD);
	iDepthRow = (int)((z - dMinZ)/dDelZ);
	iTableIndex = iDepthRow*nD+iDistCol;
	itz2 = iTableIndex;
  iMax = (nD*nZ - 1);  // DK 080703  Set this to the last
                           //            index in the array.
  index00 = itz2;
  index01=itz2+1;
  index10=itz2 + nD;
  index11=itz2 + nD+1;

  /* DK 080703
     If z = dMaxZ or dDist = dMaxD, then index01, index10, 
     and index11 can all overflow the array, even
     though they won'dDist be used in the calculation.
     Set any overflowing indexes to iMax.  Their value
     is not an issue as long as iDistCol's legal, because
     their data will not be used in the calculation
   ***********************************************/
  if(index01 > iMax) index01= iMax;
  if(index10 > iMax) index10= iMax;
  if(index11 > iMax) index11= iMax;

	d00 = dPPD[index00];
	d01 = dPPD[index01];
	d10 = dPPD[index10];
	d11 = dPPD[index11];

  if(d00 < 0.0)	return -3.0;  //DKC all -1 in original call
	if(d01 < 0.0)	return -4.0;  //DKC all -1 in original call
	if(d10 < 0.0)	return -5.0;  //DKC all -1 in original call
	if(d11 < 0.0)	return -6.0;  //DKC all -1 in original call

  // DK Change bug in v0b9  (use deltaD not delta T)
  // s = (dDist - dMinD - iDistCol*dDelT)/dDelT;
  s = (dDist - dMinD - iDistCol*dDelD)/dDelD;
	q = (z - dMinZ - iDepthRow*dDelZ)/dDelZ;
	sm = 1.0 - s;
	qm = 1.0 - q;
	q00 = sm * qm;
	q01 = s * qm;
	q10 = sm * q;
	q11 = s * q;

	iq = iTableIndex;
	qmax = q00;

	if(q01 > qmax) {
		qmax = q01;
		iq = iTableIndex+1;
	}
	if(q10 > qmax) {
		qmax = q10;
		iq = iTableIndex + nD;
	}
	if(q11 > qmax) {
		qmax = q11;
		iq = iTableIndex + nD + 1;
	}
	iPhase = iT[iq];  // DK 081103 Change from iD to iT v0.0b11

  dTime = d00 * q00 + d01 * q01 + d10 * q10 + d11 * q11;

	return dTime;
}  // end PPD()


//------------------------------------------------------------------------------D
// Interpolate distance for first arrival at a given travel time
double CTrav::D(double t, double z) {
	iPhase = -1;

  // Step 1:  Insure that the given Time/Depth is within the 
  //          scope of this traveltime table.  If not, return error.
	if(t < dMinT || t > dMaxT)
		return -1.0;
	if(z < dMinZ || z > dMaxZ)
		return -2.0;

  // Step 2:  Calculate the following:
  //       it:   Column index based on given time(t)
  //       iz:   Row index based on given depth(z)
  //       itz:  Base table index (rownum * rowwidth + colnum)
  //       itz2: Actual table index for data array
  //       iMax: Maximum index for this traveltime table
  it = (int)((t - dMinT)/dDelT);
	iz = (int)((z - dMinZ)/dDelZ);
	itz = iz*nT+it;
	itz2 = 2*itz;
  iMax = (nT*nZ - 1) * 2;  // DK 080703  Set this to the last
                           //            index in the array.

  
  // Step 2:  Calculate the indexes of the 4 entries that
  //          could affect the answer.  We know that the
  //          time of the phase lies between the time of it and it+1,
  //          and that the depth of the phase lies between the
  //          depth of iz and iz+1, so calculate the indexes for
  //          the four resulting entries:
  //           (it,iz), (it+1,iz), (it,iz+1), (it+1,iz+1)
  index00 = itz2;
  index01=itz2+2;
  index10=itz2 + 2*nT;
  index11=itz2 + 2*nT + 2;

  // Step 3:  Bounds check the table/array.
  /* If z = dMaxZ or t = dMaxT, then index01, index10, 
     and index11 can all overflow the table(array), even
     though they won't be used in the calculation.
     Set any overflowing indexes to iMax.  Their value
     is not an issue as long as it's legal, because
     their data will not be used in the calculation
   ***********************************************/
  if(index01 > iMax) index01= iMax;
  if(index10 > iMax) index10= iMax;
  if(index11 > iMax) index11= iMax;


  // Step 4:  Extract the relevant values from the table
	d00 = dD[index00];
	d01 = dD[index01];
	d10 = dD[index10];
	d11 = dD[index11];

  // Step 5:  Return error if any of the values are bad!
  if(d00 < 0.0)	return -3.0;
	if(d01 < 0.0)	return -4.0;
	if(d10 < 0.0)	return -5.0;
	if(d11 < 0.0)	return -6.0;

  // Step 6:  Calculate the following params:
  //       s:   The linear contribution of the first time entry
  //       q:   The linear contribution of the first depth entry
  //       sm:  The linear contribution of the second time entry (0-s)
  //       qm:   The linear contribution of the second depth entry (0-q)

  s = (t - dMinT - it*dDelT)/dDelT;
	q = (z - dMinZ - iz*dDelZ)/dDelZ;
	sm = 1.0 - s;
	qm = 1.0 - q;

  // Step 7:  Calculate the contribution percentage of the four
  //          extracted data values(00,01,10,11), based on the 
  //          linear interpolation attributes calculated above (s,q,sm,qm)
	q00 = sm * qm;
	q01 = s * qm;
	q10 = sm * q;
	q11 = s * q;


  // Step 8:  Determine which data value has the highest contribution.
  //          Use the phase name of that value as the name of the 
  //          interpolated phase.
	iq = itz;
	qmax = q00;

	if(q01 > qmax) {
		qmax = q01;
		iq = itz+1;
	}
	if(q10 > qmax) {
		qmax = q10;
		iq = itz + nT;
	}
	if(q11 > qmax) {
		qmax = q11;
		iq = itz + nT + 1;
	}
	iPhase = iD[iq];


  // Note:  Each element in the table has two entries: Angular Distance(D) and 
  //        Take of Angle(TOA)

  // Step 9:  Use linear interpolation to calculate
  //          the angular distance(D) of the requested phase.
  d = d00 * q00 + d01 * q01 + d10 * q10 + d11 * q11;

  toa00 = dD[index00 + 1];
	toa01 = dD[index01 + 1];
	toa10 = dD[index10 + 1];
	toa11 = dD[index11 + 1];

  // Step 10: Use linear interpolation to calculate
  //          the takeoff angle(TOA) of the requested phase.
	dToa = toa00 * q00 + toa01 * q01 + toa10 * q10 + toa11 * q11;

  // Step 11: Return the calculated distance.
  return d;
}  // end D()



////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
//   THE REMAINING FUNCTIONS IN THIS FILE ARE NOT USED BY THE CURRENT VERSION OF
//   GLASS!
//                  - DK 080703
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------------List
// Produce printable listing of travel time table
void CTrav::List(char *file) {
	int nper = 5;
	int npage = (nZ-1)/nper + 1;
	int iz1, iz2;
	int ipage;
	int iz;
	int id;
	double d;
	double z;
	double t;
	int mn;
	double toa;
	int idz;
	int iph;
	int itoa;
	CStr tmp;
	char *s;

	FILE *flst;
  
	if(!(flst = fopen(file, "wt")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTrav::List(): ERROR!  Could not open printable travel time table output file (%s)\n",
                file);
		return;
  }

	tmp.Empty();
	tmp.Cat("Table dump for file:");
	tmp.Cat(cTrv);
	tmp.Cat("\n\n");
	s = tmp.GetBuffer();
	fwrite(s, strlen(s), 1, flst);

	tmp.Empty();
	tmp.Cat("Delta range: nD:");
	tmp.Cat(nD, 3);
	tmp.Cat(" from ");
	tmp.Cat(dMinD, 6, 2);
	tmp.Cat(" to ");
	tmp.Cat(dMaxD, 6, 2);
	tmp.Cat(" by ");
	tmp.Cat(dDelD, 6, 2);
	tmp.Cat("\n");
	s = tmp.GetBuffer();
	fwrite(s, strlen(s), 1, flst);

	tmp.Empty();
	tmp.Cat("Time  range: nT:");
	tmp.Cat(nD, 3);
	tmp.Cat(" from ");
	tmp.Cat(dMinT, 6, 2);
	tmp.Cat(" to ");
	tmp.Cat(dMaxT, 6, 2);
	tmp.Cat(" by ");
	tmp.Cat(dDelT, 6, 2);
	tmp.Cat("\n");
	s = tmp.GetBuffer();
	fwrite(s, strlen(s), 1, flst);

	tmp.Empty();
	tmp.Cat("Depth range: nD:");
	tmp.Cat(nZ, 3);
	tmp.Cat(" from ");
	tmp.Cat(dMinZ, 6, 2);
	tmp.Cat(" to ");
	tmp.Cat(dMaxZ, 6, 2);
	tmp.Cat(" by ");
	tmp.Cat(dDelZ, 6, 2);
	tmp.Cat("\n");
	s = tmp.GetBuffer();
	fwrite(s, strlen(s), 1, flst);

	fwrite("Phases...\n", 10, 1, flst);
	for(iph=0; iph<nPhase; iph++) {
		tmp.Empty();
		tmp.Cat("    Ph[");
		tmp.Cat(iph);
		tmp.Cat("]: ");
		tmp.Cat(cPhase[iph].GetBuffer());
		tmp.Cat("\n");
		s = tmp.GetBuffer();
		fwrite(s, strlen(s), 1, flst);
	}
	for(ipage=0; ipage<npage; ipage++) {
		iz1 = ipage*nper;
		iz2 = iz1+nper;
		if(iz2 > nZ)
			iz2 = nZ;
		fwrite("\n\nDelta ", 8, 1, flst);
		for(iz=iz1; iz<iz2; iz++) {
			tmp.Empty();
			tmp.Cat("        z=");
			tmp.Cat(dMinZ+iz*dDelZ, 5, 1);
			tmp.Cat(" ");
			s = tmp.GetBuffer();
			fwrite(s, strlen(s), 1, flst);
		}
		fwrite("\n", 1, 1, flst);
		for(id=0; id<nD; id++) {
			d = dMinD + id*dDelD;
			tmp.Empty();
			tmp.Cat(d, 5, 1);
			tmp.Cat(" ");
			s = tmp.GetBuffer();
			fwrite(s, strlen(s), 1, flst);
			for(iz=iz1; iz<iz2; iz++) {
				idz = iz*nD+id;
				z = dMinZ + iz*dDelZ;
				t = dT[2*idz];
				if(t < 0.0) {
					fwrite("           NADA ", 16, 1, flst);
					continue;
				}
				if(t < dMinT)
					dMinT = t;
				if(t > dMaxT)
					dMaxT = t;
				toa = dT[2*idz+1];
				mn = (int)(t/60.0);
				iph = iT[idz];
				itoa = (int)(toa+0.5);
				tmp.Empty();
				tmp.Cat(iph, 2);
				tmp.Cat(":");
				tmp.Cat(mn, 2);
				tmp.Cat("-");
				tmp.Cat(t-60.0*mn, 5, 2);
				tmp.Cat(itoa, 4);
				s = tmp.GetBuffer();
				fwrite(s, strlen(s), 1, flst);
			}
			fwrite("\n", 1, 1, flst);
		}
	}
	fclose(flst);
  flst=NULL;
}  // end List()



//------------------------------------------------------------------------------Generate
void CTrav::Generate() {
	CRay ray[MAXPHS];
	double rcvr = pTerra->dRadius;
	double rsrc;
	double vsrc;
	double t;
	double d;
	double p;
	double z;
	double toa;
	double d1, d2;
	int id;
	int iz;
	int idz;
	int idz2;
	int itz;
	int iph;
	ixT = 10;
	iyT = 100;
	int mn;

	bCreate = FALSE;

	nD = (int)((dMaxD - dMinD)/dDelD) + 1;
	dT = new double[2*nD*nZ];
	iT = new int[nD*nZ];
	for(iz=0; iz<nZ; iz++) {
		for(id=0; id<nD; id++) {
			idz = iz*nD + id;
			iT[idz] = -1;
			dT[2*idz] = -10.0;
			dT[2*idz+1] = 0.0;
		}
	}
	dMinT = 10000.0;
	dMaxT = -10000.0;
	double p0, p1, p2, pinc, tmp;
	double t1, t2;
	double pmin = 0.01;
	int istate;
	int np, ip;
	double plst, dlst, tlst;

	// Initialize ray object array
	for(iph=0; iph<nPhase; iph++)
		ray[iph].Attach(pTerra);

	// Calulate bounds for reduced travel-time plot
	double tred;

	hTst = 100;
	wTst = 300;
	double tredmin =  10000.0;
	double tredmax = -10000.0;
	double tmin =  10000.0;
	double tmax = -10000.0;
	double tint;
	double pint;
	for(int pass=0; pass<2; pass++) {
		for(iph=0; iph<nPhase; iph++) {
			ray[iph].setPhase(cPhase[iph].GetBuffer());
			for(iz=0; iz<nZ; iz++) {
				z = dMinZ + iz*dDelZ;
				rsrc = pTerra->dRadius - z;
				if(z < 0.01)
					z = 0.01;
				ray[iph].setGeocentric(0.0, 0.0, rsrc);
				ray[iph].Init();
				if(cPhase[iph] == "Pdiff" || cPhase[iph] == "SDiff") {
					for(d=dMinD; d<=dMaxD; d+=dDelD) {
						t = ray[iph].Travel(DEG2RAD*d, rcvr, &p);
						if(t < 0)
							continue;
						switch(pass) {
						case 0:
							if(t < 0.0)
								continue;
							if(t < tmin)
								tmin = t;
							if(t > tmax)
								tmax = t;
							break;
						case 1:
							tint = tmin + (tmax-tmin)*(d-dMinD)/(dMaxD-dMinD);
							tred = t - tint;
							if(tred < tredmin)
								tredmin = tred;
							if(tred > tredmax)
								tredmax = tred;
							break;
						}
					}
					continue;
				}
				p1 = ray[iph].dP1;
				p2 = ray[iph].dP2;
				if(p2 < p1) {
					tmp = p2;
					p2 = p1;
					p1 = tmp;
				}
				np = 1000;
				pinc = (p2-p1)/np;
				for(ip=0; ip<=np; ip++) {
					p = p1 + ip*pinc;
					d = RAD2DEG*ray[iph].D(p, rcvr);
					if(d < dMinD || d > dMaxD)
						continue;
					t = ray[iph].T(p, rcvr);
					if(t < 0.0)
						continue;
					switch(pass) {
					case 0:
						if(t < tmin) {
							tmin = t;
						}
						if(t > tmax) {
							tmax = t;
						}
						break;
					case 1:
						tint = tmin + (tmax-tmin)*(d-dMinD)/(dMaxD-dMinD);
						tred = t - tint;
						if(tred < tredmin) {
							tredmin = tred;
						}
						if(tred > tredmax) {
							tredmax = tred;
						}
						break;
					}
				}
			}
		}
		switch(pass) {
		case 0:
			break;
		case 1:
			break;
		}
	}

	// Generate travel time interpolation tables
	int id1, id2;
	double frac;
	char phase[32];
	for(iph=0; iph<nPhase; iph++) {
		ray[iph].setPhase(cPhase[iph].GetBuffer());
		strcpy(phase, cPhase[iph].GetBuffer());
		for(iz=0; iz<nZ; iz++) {
			z = dMinZ + iz*dDelZ;
			rsrc = pTerra->dRadius - z;
			if(z < 0.01)
				z = 0.01;
			if(phase[0] == 'S' || phase[0] == 's')
				vsrc = pTerra->S(rsrc);
			else
				vsrc = pTerra->P(rsrc);
			ray[iph].setGeocentric(0.0, 0.0, rsrc);
			ray[iph].Init();
			if(cPhase[iph] == "Pdiff" || cPhase[iph] == "Sdiff") {
				for(id=0; id<nD; id++) {
					d = dMinD + id*dDelD;
					t = ray[iph].Travel(DEG2RAD*d, rcvr, &p);
					if(t < 0.0)
						continue;
					tint = tmin + (tmax-tmin)*(d-dMinD)/(dMaxD-dMinD);
					tred = t - tint;
					mn = (int)(t/60.0);
					idz = iz*nD+id;
					idz2 = 2*idz;
					if(iT[idz] < 0 || iT[idz] >= 0 && dT[idz2] > t) {
						toa = RAD2DEG*asin(p*vsrc/rsrc);
						iT[idz] = iph;
						dT[idz2] = t;
						dT[idz2+1] = toa;
					}
				}
				continue;
			}
			p1 = ray[iph].dP1;
			p2 = ray[iph].dP2;
			if(p2 < p1) {
				tmp = p2;
				p2 = p1;
				p1 = tmp;
			}
			p0 = p1;
			np = 10000;
			pinc = (p2-p1)/np;
			istate = 0;
			for(ip=0; ip<=np; ip++) {
				p = p0 + ip*pinc;
				d = RAD2DEG*ray[iph].D(p, rcvr);
				if(d < 0.0)
					continue;
				t = ray[iph].T(p, rcvr);
				if(t < 0.0)
					continue;
				if(istate == 0) {
					dlst = d;
					plst = p;
					tlst = t;
					istate = 1;
					continue;
				}
				d1 = dlst;
				d2 = d;
				t1 = tlst;
				t2 = t;
				p1 = plst;
				p2 = p;
				if(d2 < d1) {
					tmp = p2;
					p2 = p1;
					p1 = tmp;
					tmp = d2;
					d2 = d1;
					d1 = tmp;
					tmp = t2;
					t2 = t1;
					t1 = tmp;
				}
				if(d1 > dMaxD || d2 < dMinD)
					continue;
				tint = tmin + (tmax-tmin)*(d-dMinD)/(dMaxD-dMinD);
				tred = t - tint;

				// Interpolate
				id1 = (int)((d1-dMinD)/dDelD) + 1;
				id2 = (int)((d2-dMinD)/dDelD);
				if(id1 < 2)
					id1 = 0;
				if(id2 >= nD)
					id2 = nD - 1;
				if(id2 < id1)
					continue;
				for(id=id1; id<=id2; id++) {
					frac = (dMinD+id*dDelD-d1)/(d2-d1);
					tint = t1 + frac*(t2-t1);
					pint = p1 + frac*(p2-p1);
					toa = RAD2DEG*asin(pint*vsrc/rsrc);
					if(cPhase[iph] == "Pup"
					|| cPhase[iph] == "Sup"
					|| cPhase[iph] == "pP")
						toa = 180.0 - toa;
					idz = iz*nD+id;
					idz2 = 2*idz;
					// This little bit of nasty code is aimed at suppressing the
					// back branch of PKP, specifically the PKPbc branch when
					// calculating the PKPab branch. Further nastiness will be
					// required if it is desired to calculate the PKPbc branch =)
					if(iT[idz] > -1) {
						if(cPhase[iph] == "PKPab") {
							if(dT[idz2] > tint) {
								continue;
							}
						} else {
							if(dT[idz2] < tint) {
								continue;
							}
						}
					}
					iT[idz] = iph;
					dT[idz2] = tint;
					dT[idz2+1] = toa;
				}
				// Recycle for next interfacl
				plst = p;
				dlst = d;
				tlst = t;
			}
		}
	}

	// Calculate time range from travel time tables
	for(id=0; id<nD; id++) {
		for(iz=0; iz<nZ; iz++) {
			idz = iz*nD+id;
			z = dMinZ + iz*dDelZ;
			d = dMinD + id*dDelD;
			t = dT[2*idz];
			if(t < 0.0) {
				continue;
			}
			if(t < dMinT)
				dMinT = t;
			if(t > dMaxT)
				dMaxT = t;
			toa = dT[2*idz+1];
			mn = (int)(t/60.0);
			iph = iT[idz];
		}
	}

	// Generate Distance table from travel time table
	// It might be a good idea to construct this table using splines
	// if accuracy proves insufficient
	int it;
	int it1;
	int it2;
	double dint;
	nT = nD;
	iD = new int[nT*nZ];
	dD = new double[2*nT*nZ];
	dDelT = (dMaxT - dMinT)/(nT - 1);
	for(iz=0; iz<nZ; iz++) {
		for(it=0; it<nT; it++) {
			itz = iz*nT + it;
			iD[itz] = -1;
			dD[2*itz] = -10.0;
			dD[2*itz+1] = 0.0;
		}
	}
	double toa1;
	double toa2 = 0.0;
	for(iz=0; iz<nZ; iz++) {
		z = dMinZ + iz*dDelZ;
		t2 = -10.0;
		for(id=0; id<nD; id++) {
			idz = iz*nD + id;
			t1 = t2;
			t2 = dT[2*idz];
			d1 = d2;
			d2 = dMinD + id*dDelD;
			toa1 = toa2;
			toa2 = dT[2*idz+1];
			if(t1 < 0.0 || t2 < 0.0)
				continue;
			it1 = (int)((t1-dMinT)/dDelT);
			it2 = (int)((t2-dMinT)/dDelT);
			if(it1 < 1)
				it1 = 0;
			if(it2 >= nT)
				it2 = nT - 1;
			for(it=it1; it<=it2; it++) {
				itz = iz*nT + it;
				frac = (dMinT+it*dDelT-t1)/(t2-t1);
				dint = d1 + frac*(d2-d1);
				if(dint < 0.0)
					dint = 0.0;
				dD[2*itz] = dint;
				dD[2*itz + 1] = toa1 + frac*(toa2-toa1);
				if(frac < 0.5)
					iD[itz] = iT[idz-1];
				else
					iD[itz] = iT[idz];
			}
		}
	}
}  // end Generate()

//------------------------------------------------------------------------------Save
// Write travel-time table to disk
void CTrav::Save(char *file) {
	FILE *fttt;
	int magic = 777;
	char cphs[16*MAXPHS];
	int i;

	strcpy(cTrv, file);
	for(i=0; i<16*MAXPHS; i++)
		cphs[i] = ' ';
	for(i=0; i<nPhase; i++)
		strcpy(&cphs[16*i], cPhase[i].GetBuffer());

  if(!(fttt = fopen(file, "wb")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTrav::Save(): ERROR!  Could not open travel-time table output file (%s)\n",
                file);
		return;
  }

	fwrite(&magic, sizeof(magic), 1, fttt);
	fwrite(&nPhase, sizeof(nPhase), 1, fttt);
	fwrite(&nD, sizeof(nD), 1, fttt);
	fwrite(&nT, sizeof(nT), 1, fttt);
	fwrite(&nZ, sizeof(nZ), 1, fttt);
	fwrite(&dMinT, sizeof(dMinT), 1, fttt);
	fwrite(&dMaxT, sizeof(dMaxT), 1, fttt);
	fwrite(&dMinD, sizeof(dMinD), 1, fttt);
	fwrite(&dMaxD, sizeof(dMaxD), 1, fttt);
	fwrite(&dMinZ, sizeof(dMinZ), 1, fttt);
	fwrite(&dMaxZ, sizeof(dMaxZ), 1, fttt);
	fwrite(cphs, 16*nPhase, 1, fttt);
	int ndz = nD*nZ;
	fwrite(iT, sizeof(iT[0]),   ndz, fttt);
	fwrite(dT, sizeof(dT[0]), 2*ndz, fttt);
	int ntz = nT*nZ;
	fwrite(iD, sizeof(iD[0]),   ntz, fttt);
	fwrite(dD, sizeof(dD[0]), 2*ntz, fttt);
	fclose(fttt);
  fttt = NULL;
}  // end Save()




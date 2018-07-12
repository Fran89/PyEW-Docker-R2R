#include <windows.h>
#include <math.h>
#include "trav.h"
#include "ttt.h"
extern "C" {
#include "utility.h"
}

#define DEGRAD  57.29577951308

CTTT::CTTT() {
	nTrv = 0;
}

CTTT::~CTTT() {
	for(int i=0; i<nTrv; i++)
		delete pTrv[i];
}

//---------------------------------------------------------------------Load
bool CTTT::Load(char *file) {
	pTrv[nTrv] = new CTrav();
	pTrv[nTrv]->Load(file);
	nTrv++;
	return true;
}

//---------------------------------------------------------------------T
// Interpolate travel time for given distance (degrees) and depth (km)
double CTTT::T(double d, double z) {
	int itrv;
	CTrav *trv;
	double t;
	for(itrv=0; itrv<nTrv; itrv++) {
		trv = pTrv[itrv];
		if(z < trv->dMinZ || z > trv->dMaxZ)
			continue;
		if(d < trv->dMinD || d > trv->dMaxD)
			continue;
		t = trv->T(d, z);
		if(trv->iPhase < 0)
			continue;
		dToa = trv->dToa;
		strcpy(sPhase, trv->cPhase[trv->iPhase].GetBuffer());
		iTrv = itrv;
		iPhs = trv->iPhase;
		return t;
	}
	return -10.0;
}

//---------------------------------------------------------------------T
// Interpolate pick probability for given distance (degrees) and depth (km)
double CTTT::PPD(double d, double z) {
	int itrv;
	CTrav *trv;
	double dPP;
	for(itrv=0; itrv<nTrv; itrv++) {
		trv = pTrv[itrv];
		if(z < trv->dMinZ || z > trv->dMaxZ)
			continue;
		if(d < trv->dMinD || d > trv->dMaxD)
			continue;
		dPP = trv->PPD(d, z);
		if(trv->iPhase < 0)
			continue;
		return dPP;
	}
	return 0.0;
}


//---------------------------------------------------------------------D
// Calculate distance (degrees) for given travel time and depth (km)
double CTTT::D(double t, double z) {
	int itrv;
	CTrav *trv;
	char *s;
	int it;
	double d;
	for(itrv=0; itrv<nTrv; itrv++) {
//		Debug("Trv[%d]\n", itrv);
		trv = pTrv[itrv];
		if(z < trv->dMinZ || z > trv->dMaxZ)
			continue;
		if(t < trv->dMinT || t > trv->dMaxT)
			continue;
		d = trv->D(t, z);
//		Debug("CC d:%.2f\n", d);
		if(trv->iPhase < 0)
			continue;
		dToa = trv->dToa;
		it = trv->iPhase;
//		Debug("DD itrv:%d\n", it);
		s = trv->cPhase[it].GetBuffer();
//		Debug("EE s:%d\n", s);
//		Debug("FF s:%s\n", s);
		strcpy(sPhase, s);
		iTrv = itrv;
		iPhs = trv->iPhase;
//		Debug("d:%.2f\n", d);
		return d;
	}
//	Debug("Failed\n");
	return -10.0;
}

//---------------------------------------------------------------------Toa
// Return take of angle of last successful D() or T() call
double CTTT::Toa() {
	return dToa;
}

//---------------------------------------------------------------------Phase
// Return phase of last succesfull D() or T() call
CStr CTTT::Phase() {
	return CStr(sPhase);
}

// DK 061003  This function takes the place of CStr Phase()
// It is more efficient, as it doesn't have to construct
// a CStr object.
// It was created to make the CTravelTime::D() routine more
// efficient.
char * CTTT::Phase(char * szPhaseBuffer, int iBufferLen)
{
  if(!szPhaseBuffer)
  {
    return(NULL);
  }
  else
  {
    strncpy(szPhaseBuffer,sPhase,iBufferLen);
    szPhaseBuffer[iBufferLen-1]=0x00;
    return(szPhaseBuffer);
  }
}

//---------------------------------------------------------------------PhaseIx
// Return phase associated with provied phase index as returned by PhaseIx()
int CTTT::PhaseIx() {
	return 10*iTrv + iPhs;
}

//---------------------------------------------------------------------Phase(ix)
// Return phase of given phase index (10*itrv + iphs)
CStr CTTT::Phase(int ix) {
	int itrv = ix/10;
	int iphs = ix - 10*itrv;
	if(itrv < 0 || itrv >= nTrv)
		return CStr("Nada");
	if(iphs < 0 || iphs >= pTrv[itrv]->nPhase)
		return CStr("Nada");
	return pTrv[itrv]->cPhase[iphs];
}

//---------------------------------------------------------------------------------------DMin
double CTTT::DMin() {
	CTrav *trv;
	double d;
	for(int itrv=0; itrv<nTrv; itrv++) {
		trv = pTrv[itrv];
		if(!itrv) {
			d = trv->dMinD;
			continue;
		}
		if(d > trv->dMinD)
			d = trv->dMinD;
	}
	return d;
}

//---------------------------------------------------------------------------------------DMax
double CTTT::DMax() {
	CTrav *trv;
	double d;
	for(int itrv=0; itrv<nTrv; itrv++) {
		trv = pTrv[itrv];
		if(!itrv) {
			d = trv->dMaxD;
			continue;
		}
		if(d < trv->dMaxD)
			d = trv->dMaxD;
	}
	return d;
}

//---------------------------------------------------------------------------------------TMin
double CTTT::TMin() {
	CTrav *trv;
	double t;
	for(int itrv=0; itrv<nTrv; itrv++) {
		trv = pTrv[itrv];
		if(!itrv) {
			t = trv->dMinT;
			continue;
		}
		if(t > trv->dMinT)
			t = trv->dMinT;
	}
	return t;
}

//---------------------------------------------------------------------------------------TMax
double CTTT::TMax() {
	CTrav *trv;
	double t;
	for(int itrv=0; itrv<nTrv; itrv++) {
		trv = pTrv[itrv];
		if(!itrv) {
			t = trv->dMaxT;
			continue;
		}
		if(t < trv->dMinT)
			t = trv->dMinT;
	}
	return t;
}

//---------------------------------------------------------------------------------------ZMin
double CTTT::ZMin() {
	CTrav *trv;
	double z;
	for(int itrv=0; itrv<nTrv; itrv++) {
		trv = pTrv[itrv];
		if(!itrv) {
			z = trv->dMinZ;
			continue;
		}
		if(z > trv->dMinZ)
			z = trv->dMinZ;
	}
	return z;
}

//---------------------------------------------------------------------------------------ZMax
double CTTT::ZMax() {
	CTrav *trv;
	double z;
	for(int itrv=0; itrv<nTrv; itrv++) {
		trv = pTrv[itrv];
		if(!itrv) {
			z = trv->dMaxZ;
			continue;
		}
		if(z < trv->dMinZ)
			z = trv->dMinZ;
	}
	return z;
}



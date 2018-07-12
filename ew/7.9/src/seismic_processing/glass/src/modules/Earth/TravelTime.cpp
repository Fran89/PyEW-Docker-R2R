/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: TravelTime.cpp 2176 2006-05-22 16:04:15Z paulf $
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
 *     Revision 1.3  2003/11/07 19:13:23  davidk
 *     Added RCS comment header.
 *     Changed two comments.
 *     No functional changes in this rev.
 *
 *
 **********************************************************/

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "TravelTime.h"
#include "terra.h"
#include "EarthMod.h"
#include "ttt.h"
#include "geo.h"
#include "str.h"
#include <IGlint.h>
extern "C" {
#include "utility.h"
}

#include <Debug.h>

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994

TTT tmpTTT;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CTravelTime::CTravelTime() {
	pMod = 0;
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CTravelTime::~CTravelTime() {
}

//---------------------------------------------------------------------------------------Init
void CTravelTime::Init(CMod *mod) {
	pMod = mod;
}

//---------------------------------------------------------------------------------------Action
bool CTravelTime::Action(IMessage *msg) {
	char *file;
	char elf[128];
	int res;

	if(msg->Is("ITravelTime")) {
		msg->setPtr("Interface", this);
		return true;
	}

	if(msg->Is("Ellipticity")) {
		file = msg->getStr("File");
		if(!file) {
			msg->setInt("Res", 1);
			return true;
		}
		strcpy(elf, file);
		if(Ellipticity(elf))
			res = 0;
		else
			res = 1;
		msg->setInt("Res", res);
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------------Ellipticity
bool CTravelTime::Ellipticity(char *elf) {
	FILE *fin;
	FILE *fout;
	int k;
	char ph[16];
	int iph = 0;
	int state = 1;
	int knt = 0;
	int kix = 0;
	char card[120];
	int nc;
	int i;
	int j;

  static char * szEllipticityOutputFilename = "elf.d";
	
	if(!(fin = fopen(elf, "r")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::Ellipticity(): ERROR!  Could not open ellip input file (%s)\n",
                elf);
		return false;
  }
	if(!(fout = fopen(szEllipticityOutputFilename, "wt")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::Ellipticity(): ERROR!  Could not open ellip output file (%s)\n",
                szEllipticityOutputFilename);
		return false;
  }

	nc = 0;
	while(true) {
		k = fgetc(fin);
		if(k == EOF) {
			fclose(fin);
      fin=NULL;
			fclose(fout);
      fout=NULL;
			return true;
		}
		card[nc++] = k;
		card[nc] = 0;
		if(knt%80 == 79) {
			j = 0;
			for(i=0; i<nc; i++) {
				if(card[i] != ' ')
					j = i;
			}
			card[j] = 0;
			fwrite(card, 1, strlen(card), fout);
			fwrite("\n", 1, 1, fout);
			nc = 0;
		}
		switch(state) {
		case 0:
			if(k == 'p' || k == 'P' || k == 's' || k == 'S') {
				state = 1;
				iph = 1;
				ph[0] = k;
				kix = knt;
				break;
			}
			break;
		case 1:
			if(k == ' ') {
				ph[iph] = 0;
				state = 0;
				break;
			}
			ph[iph] = k;
			if(iph < 15)
				iph++;
			break;
		}
		knt++;
	}
}


//---------------------------------------------------------------------------------------Delta
// Calculate the distance from point 1 to point 2 in degrees
double CTravelTime::Delta(double lat1, double lon1, double lat2, double lon2) {
	CGeo geo1;
	CGeo geo2;

	geo1.setGeographic(lat1, lon1, pMod->pTerra->dRadius);
	geo2.setGeographic(lat2, lon2, pMod->pTerra->dRadius);
	return( RAD2DEG*geo1.Delta(&geo2));
}

//---------------------------------------------------------------------------------------Azimuth
// Calculate azimuth from point 1 to point 2 in degrees
double CTravelTime::Azimuth(double lat1, double lon1, double lat2, double lon2) {
	CGeo geo1;
	CGeo geo2;

	geo1.setGeographic(lat1, lon1, pMod->pTerra->dRadius);
	geo2.setGeographic(lat2, lon2, pMod->pTerra->dRadius);
	return(RAD2DEG*geo1.Azimuth(&geo2));
}

//---------------------------------------------------------------------------------------Best
// Determine the best fit to a given travel time and distance, and return an appropriate
// travel time descriptiong
TTT *CTravelTime::Best(double d, double t, double z) {
	int ittt;
	int jttt;
	double tcal;
	double ares;
	double best = 100000.0;

	jttt = -1;
	for(ittt=0; ittt<pMod->nTTT; ittt++) {
		tcal = pMod->pTTT[ittt]->T(d, z);
		if(tcal < 0.0)
			continue;
		ares = t - tcal;
		if(ares < 0.0)
			ares = -ares;
		if(ares < best) {
			best = ares;
			jttt = ittt;
		}
	}
	if(jttt < 0)
		return 0;	// No branches at that distance
	return T(jttt, d, z);
}

//---------------------------------------------------------------------------------------T
// Return travel time record and travel-time for a given branch, distance, and depth
TTT *CTravelTime::T(int ittt, double d, double z) {
	CStr phs;
	double r;
	double vel;
	double p;

	if(ittt < 0 || ittt >= pMod->nTTT)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::T() ERROR! attempted to reference "
                                  " ttt(%d) of (%d).\n",
                ittt, pMod->nTTT);
		return 0;
  }
	tmpTTT.iRevision = TTT_REVISION;
	tmpTTT.iTTT = ittt;
	tmpTTT.dD = d;
	tmpTTT.dZ = z;
	tmpTTT.dT = pMod->pTTT[ittt]->T(d, z);
	tmpTTT.dPPD = pMod->pTTT[ittt]->PPD(d, z);
	if(tmpTTT.dT < 0.0)
		return 0;
	pMod->pTTT[ittt]->Phase(tmpTTT.sPhs, sizeof(tmpTTT.sPhs));   // removed use of CStr
	//strcpy(tmpTTT.sPhs, phs.GetBuffer());
	tmpTTT.dToa = pMod->pTTT[ittt]->Toa();
	r = pMod->pTerra->dRadius - z;
	if(phs[0] == 'S' || phs[0] == 's')
		vel = pMod->pTerra->S(r);
	else
		vel = pMod->pTerra->P(r);
	p = DEG2RAD*r*sin(DEG2RAD*tmpTTT.dToa)/vel;
	tmpTTT.dTdD = p;
	tmpTTT.dTdZ = -RAD2DEG*p/r/tan(DEG2RAD*tmpTTT.dToa);
	return &tmpTTT;
}

//---------------------------------------------------------------------------------------D
// Return travel time record and depth for a given branch, travel-time, and depth
TTT *CTravelTime::D(int ittt, double t, double z) {
	static char szPhase[32];
  static int iPhaseSize = sizeof(szPhase);
	double r;
	double vel;
	double p;

//	DebugOn();
//	Debug("D entered\n");
	if(ittt < 0 || ittt >= pMod->nTTT) 
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::D() ERROR! attempted to reference "
                                  " ttt(%d) of (%d).\n",
                ittt, pMod->nTTT);
		return 0;
  }
	tmpTTT.iRevision = TTT_REVISION;
	tmpTTT.iTTT = ittt;
	tmpTTT.dT = t;
	tmpTTT.dZ = z;
	strcpy(tmpTTT.sPhs, PHASE_UNKNOWN_STR);
//	Debug("A t:%.2f z:%.2f\n", t, z);
	tmpTTT.dD = pMod->pTTT[ittt]->D(t, z);
//	Debug("B d:%.2f\n", tmpTTT.dD);
	if(tmpTTT.dD < 0.0)
		return 0;
	if(!pMod->pTTT[ittt]->Phase(szPhase,iPhaseSize))
    return NULL;
	strcpy(tmpTTT.sPhs, szPhase);
	tmpTTT.dToa = pMod->pTTT[ittt]->Toa();
//	Debug("D\n");
	r = pMod->pTerra->dRadius - z;
	if(szPhase[0] == 'S' || szPhase[0] == 's')
		vel = pMod->pTerra->S(r);
	else
		vel = pMod->pTerra->P(r);
	p = DEG2RAD*r*sin(DEG2RAD*tmpTTT.dToa)/vel;
	tmpTTT.dTdD = p;
	tmpTTT.dTdZ = -RAD2DEG*p/r/tan(DEG2RAD*tmpTTT.dToa);
//	Debug("Z\n");
	return &tmpTTT;
}

//---------------------------------------------------------------------------------------Tall
// Calculate travel-time and ancillary parameters for all phases arriving at a given distance
// specified in degrees. Source depth is in km, receiver assumed to be at surface.
int CTravelTime::Tall(double d, double z, TTT *ttt) {
	int ittt;
	double tcal;
	double toa;
	double vel;
	int nttt;
	CStr phs;
	double r;
	double p;

	if(!pMod)
		return 0;
	nttt = 0;
	for(ittt=0; ittt<pMod->nTTT; ittt++) {
		tcal = pMod->pTTT[ittt]->T(d, z);
		if(tcal < 0.0)
			continue;
		ttt[nttt].iRevision = TTT_REVISION;
		ttt[nttt].iTTT = ittt;
		phs = pMod->pTTT[ittt]->Phase(ttt[nttt].sPhs,sizeof(ttt[nttt].sPhs)); // removed use of CStr
		//strcpy(ttt[nttt].sPhs, phs.GetBuffer());
		ttt[nttt].dD = d;
		ttt[nttt].dZ = z;
		ttt[nttt].dT = tcal;
		toa = pMod->pTTT[ittt]->Toa();
		ttt[nttt].dToa = toa;
		r = pMod->pTerra->dRadius - z;
		if(phs[0] == 'S' || phs[0] == 's')
			vel = pMod->pTerra->S(r);
		else
			vel = pMod->pTerra->P(r);
		p = DEG2RAD*r*sin(DEG2RAD*ttt[nttt].dToa)/vel;
		ttt[nttt].dTdD = p;
		ttt[nttt].dTdZ = -RAD2DEG*p/r/tan(DEG2RAD*toa);
		nttt++;
	}
	return nttt;
}

//---------------------------------------------------------------------------------------Dall
// Calculate travel-time and ancillary parameters for all phases arriving at a given distance
// specified in degrees. Source depth is in km, receiver assumed to be at surface.
int CTravelTime::Dall(double t, double z, TTT *ttt) {
	return 0;
}

//---------------------------------------------------------------------------------------NTTT
int CTravelTime::NTTT() {
	return pMod->nTTT;
}

//---------------------------------------------------------------------------------------DMin
double CTravelTime::DMin(int ittt) {
	return 0.0;
	if(ittt < 0 || ittt >= pMod->nTTT)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::DMin() ERROR! attempted to reference "
                                  " ttt(%d) of (%d).\n",
                ittt, pMod->nTTT);
		return 0.0;
  }
	return pMod->pTTT[ittt]->DMin();
}


//---------------------------------------------------------------------------------------DMax
double CTravelTime::DMax(int ittt) {
	if(ittt < 0 || ittt >= pMod->nTTT)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::DMax() ERROR! attempted to reference "
                                  " ttt(%d) of (%d).\n",
                ittt, pMod->nTTT);
		return 0.0;
  }
	return pMod->pTTT[ittt]->DMax();
}

//---------------------------------------------------------------------------------------TMin
double CTravelTime::TMin(int ittt) {
	if(ittt < 0 || ittt >= pMod->nTTT)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::TMin() ERROR! attempted to reference "
                                  " ttt(%d) of (%d).\n",
                ittt, pMod->nTTT);
		return 0.0;
  }
	return pMod->pTTT[ittt]->TMin();
}

//---------------------------------------------------------------------------------------TMax
double CTravelTime::TMax(int ittt) {
	if(ittt < 0 || ittt >= pMod->nTTT)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::TMax() ERROR! attempted to reference "
                                  " ttt(%d) of (%d).\n",
                ittt, pMod->nTTT);
		return 0.0;
  }
	return pMod->pTTT[ittt]->TMax();
}

//---------------------------------------------------------------------------------------TMin
double CTravelTime::ZMin(int ittt) {
	if(ittt < 0 || ittt >= pMod->nTTT)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::ZMin() ERROR! attempted to reference "
                                  " ttt(%d) of (%d).\n",
                ittt, pMod->nTTT);
		return 0.0;
  }
	return pMod->pTTT[ittt]->ZMin();
}

//---------------------------------------------------------------------------------------TMax
double CTravelTime::ZMax(int ittt) {
	if(ittt < 0 || ittt >= pMod->nTTT)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CTravelTime::ZMax() ERROR! attempted to reference "
                                  " ttt(%d) of (%d).\n",
                ittt, pMod->nTTT);
		return 0.0;
  }
	return pMod->pTTT[ittt]->ZMax();
}


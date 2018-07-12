// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
#include <stdio.h>
extern "C" {
#include "utility.h"
}

#include <Debug.h>
#include "SummaryMod.h"
#include "SummaryScroll.h"
#include <Date.h>

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}


CMod *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pScroll = 0;
	pGlint = 0;
	idEntity[0] = 0;
	pMod = this;
  CDebug::SetModuleName("Summary");
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	if(pScroll)
		delete pScroll;
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
	IMessage *m;
	void *ptr;
	char *str;
	HINSTANCE hinst;
	int res;

	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("Initialize")) {
		ptr = msg->getPtr("hInst");
		memmove(&hinst, ptr, sizeof(hinst));
		pScroll = new CScroll();
		pScroll->Init(hinst, "Summary");

		// Get glint interface
		m = CreateMessage("IGlint");
		Dispatch(m);
		pGlint = (IGlint *)m->getPtr("IGlint");
		m->Release();
		if(!pGlint) {
			msg->setInt("Res", 2);
			return true;
		}
		pScroll->pGlint = pGlint;

		pScroll->Refresh();
		return true;
	}

	if(msg->Is("Grok")) {
		str = msg->getStr("Quake");
		if(!str)
			return true;
		strcpy(idEntity, str);
		pScroll->Quake(idEntity);
	//	SumList(msg);
		return true;
	}

	if(msg->Is("SumList")) {
		res = SumList(msg);
		msg->setInt("Res", res);
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------------SumList
// Create summary listing for single quake
int CMod::SumList(IMessage *msg) {
	IMessage *m;
	ORIGIN *org;
	PICK *pck;
	double dres;
	char file[128];
	char sfer[128];
	int code;
	char *str;
	CDate *dt;
	int i;

	if(!pGlint)
		return 105;
	str = msg->getStr("Quake");
	if(str)
		strcpy(idEntity, str);
	if(!idEntity[0])
		return 110;
	sprintf(file, "%s.lst", idEntity);
	FILE *f = fopen(file, "wt");
	if(!f)
		return 120;

	// Summary information
	org = pGlint->getOrigin(idEntity);
	if(!org)
		return 130;
	fprintf(f, " Seq Origin Time            Latitude  Longitude  Depth Neq   RMS\n");
	dt = new CDate(org->dT);
	fprintf(f, "%4d %s %9.4f %10.4f %6.2f%4d%6.2f\n", org->iOrigin,
		dt->Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
		org->nEq, org->dRms);
	delete dt;

	m = CreateMessage("FlinnEngdahl");
	m->setDbl("Lat", org->dLat);
	m->setDbl("Lon", org->dLon);
	if(Dispatch(m)) {
		code = m->getInt("Code");
		str = m->getStr("Region");
		if(str)
			sprintf(sfer, "%d:%s", code, str);
	}
	m->Release();
	fprintf(f, "     Delta(%.1f %.1f %.1f)  Flynn-Engdahl:%s\n\n",
		org->dDelMin, org->dDelMod, org->dDelMax, sfer);

	// Phase listing
	fprintf(f, "nPh Site  Comp Net  Phase    Delta Azm Toa   Res   Aff\n");
	nPhs = 0;
  size_t iPickRef = 0;
	while(pck = pGlint->getPicksFromOrigin(org, &iPickRef))
  {
    if(!pck->bTTTIsValid)
       continue;
		if(nPhs < MAXPHS)
    {
      pGlint->endPickList(&iPickRef);
			break;
    }
		Phs[nPhs++].pPck = pck;
	}
	for(i=0; i<nPhs; i++) {
		pck = Phs[i].pPck;
		dres = pck->dT - org->dT - pck->dTrav;
		fprintf(f, "%3d %-5s%-4s%-3s%-3s%-8s %5.1f %3.0f %3.0f %5.2f%6.1f\n",
			i, pck->sSite, pck->sComp, pck->sNet, pck->sLoc, pck->sPhase,
			pck->dDelta, pck->dAzm, pck->dToa, dres, pck->dAff);
	}

	fclose(f);

	return 0;
}



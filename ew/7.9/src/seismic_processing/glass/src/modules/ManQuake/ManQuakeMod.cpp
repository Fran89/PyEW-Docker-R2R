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
#include "ManQuakeMod.h"
#include "ManQuakeScroll.h"
#include <Date.h>

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

#include <opcalc.h>

CMod *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pScroll = 0;
	pGlint = 0;
	idEntity[0] = 0;
	pMod = this;
  CDebug::SetModuleName("ManQuake");
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
	HINSTANCE hinst;

	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("Initialize")) {
		ptr = msg->getPtr("hInst");
		memmove(&hinst, ptr, sizeof(hinst));
		pScroll = new CScroll();
		pScroll->Init(hinst, "ManQuake");

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

    ITravelTime * pTT;
		m = CreateMessage("ITravelTime");
		Dispatch(m);
		pTT = (ITravelTime *)m->getPtr("Interface");
		m->Release();
		if(!pTT) {
			msg->setInt("Res", 1);
			return true;
		}
    OPCalc_SetTravelTimePointer(pTT);

		pScroll->Refresh();
		return true;
	}

	return false;
}



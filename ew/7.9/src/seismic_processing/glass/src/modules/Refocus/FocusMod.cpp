// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
//#include <windows.h>
extern "C" {
#include "utility.h"
}

#include <ITravelTime.h>
#include "FocusMod.h"
#include "Refocus.h"

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

CMod *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pMod = this;
	pRefocus = 0;
	pTrv = 0;
  CDebug::SetModuleName(this->sName);
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	if(!pRefocus)
		delete pRefocus;
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
	IMessage *m;
	ITravelTime *trv;
	void *ptr;
	char *str;
	char ent[24];

	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("Initialize")) {
		ptr = msg->getPtr("hInst");
		memmove(&hinst, ptr, sizeof(hinst));
		pRefocus = new CCanvas();

		// Get glint interface
		m = CreateMessage("IGlint");
		Dispatch(m);
		pRefocus->pGlint = (IGlint *)m->getPtr("IGlint");
		if(!pRefocus->pGlint) {
			msg->setInt("Res", 2);
			return true;
		}
		m->Release();

		// Get traveltime interface
		m = CreateMessage("ITravelTime");
		Dispatch(m);
		trv = (ITravelTime *)m->getPtr("Interface");
		m->Release();
		if(!trv) {
			msg->setInt("Res", 1);
			return true;
		}
		pTrv = trv;

		pRefocus->Refresh();
		return true;
	}

	if(msg->Is("Focus")) {
		str = msg->getStr("Quake");
		strcpy(ent, str);
		pRefocus->Quake(ent);
		if(bFocus) {
			msg->setDbl("T", dT);
			msg->setDbl("Z", dZ);
			msg->setInt("Res", 0);
		} else {
			msg->setInt("Res", 100);
		}
		return true;
	}

	return false;
}  // end Action()

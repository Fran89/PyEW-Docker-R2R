// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
extern "C" {
#include "utility.h"
}

#include "FocusGUIMod.h"
#include "canvas.h"
#include <ITravelTime.h>

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

CMod *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pMod = this;
	pCan = 0;
	pTrv = 0;
  CDebug::SetModuleName(this->sName);
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	if(!pCan)
		delete pCan;
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
	IMessage *m;
	HINSTANCE hinst;
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
		pCan = new CCanvas();
		pCan->Init(hinst, "FocusGUI");

		// Get glint interface
		m = CreateMessage("IGlint");
		Dispatch(m);
		pCan->pGlint = (IGlint *)m->getPtr("IGlint");
		if(!pCan->pGlint) {
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

		pCan->Refresh();
		return true;
	}

	if(msg->Is("Grok")) {
		str = msg->getStr("Quake");
		strcpy(ent, str);
		pCan->Quake(ent);
		return true;
	}

	return false;
}

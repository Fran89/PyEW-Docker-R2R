// Mod.cpp

#include <windows.h>
extern "C" {
#include "utility.h"
}

#include "ResidualsMod.h"
#include "canvas.h"

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

CModule *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pMod = this;
	pCan = 0;
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	if(pCan)
		delete pCan;
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
	IMessage *m;
	HINSTANCE hinst;
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
		pCan->Init(hinst, "Residuals");

		// Get glint interface
		m = CreateMessage("IGlint");
		Dispatch(m);
		pCan->pGlint = (IGlint *)m->getPtr("IGlint");
		if(!pCan->pGlint) {
			msg->setInt("Res", 2);
			return true;
		}
		m->Release();

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

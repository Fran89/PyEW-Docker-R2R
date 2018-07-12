// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
#include <Debug.h>
extern "C" {
#include "utility.h"
}

#include "SolveMod.h"
#include "solve.h"

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

CModule *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pMod = this;
  CDebug::SetModuleName("Solve");
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
	IMessage *m;
	CSolve *solve;
	ISolve *slv;

	if(msg->Is("ShutDown")) {
		return true;
	}

	// Retrieve ISolve interface, must be released by requestor
	if(msg->Is("ISolve")) {
		solve = new CSolve();
		msg->setPtr("ISolve", solve);
		return true;
	}

	if(msg->Is("SolveTest")) {
		m = CreateMessage("ISolve");
		Dispatch(m);
		slv = (ISolve *)m->getPtr("ISolve");
		if(!slv) {
			msg->setInt("Res", 1);
			return true;
		}
		m->Release();
		slv->Test();
		slv->Release();
		msg->setInt("Res", 0);
		return true;
	}

	return false;
}

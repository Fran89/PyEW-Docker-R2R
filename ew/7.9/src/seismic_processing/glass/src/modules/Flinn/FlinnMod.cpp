// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
extern "C" {
#include "utility.h"
}

#include "FlinnMod.h"
#include "Flinn.h"

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

CModule *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pMod = this;
	pFlinn = 0;
  CDebug::SetModuleName(this->sName);
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	if(pFlinn)
		delete pFlinn;
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
	CFlinn *flinn;
	char *str;
	char dir[128];
	double lat;
	double lon;
	int code;
	int ireg;
	char sreg[128];

	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("FlinnEngdahlLoad")) {
		str = msg->getStr("Dir");
		if(!str)
			return true;
		strcpy(dir, str);
		flinn = new CFlinn();
		if(!flinn->Load(dir)) {
			delete flinn;
			return true;
		}
		pFlinn = flinn;
		return true;
	}

	if(msg->Is("FlinnEngdahl")) {
		if(!pFlinn) {
			msg->setInt("Res", 1000);
			return true;
		}
		lat = msg->getDbl("Lat");
		lon = msg->getDbl("Lon");
		code = pFlinn->Code(lat, lon);
		str = pFlinn->Region1(code, &ireg);
		msg->setInt("Code", ireg);
		if(str) {
			strcpy(sreg, str);
			msg->setStr("Area", sreg);
		}
		str = pFlinn->Region2(code);
		if(str) {
			strcpy(sreg, str);
			msg->setStr("Region", sreg);
		}
		msg->setInt("Res", 0);
		return true;
	}

	return false;
}

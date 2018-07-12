// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
extern "C" {
#include "utility.h"
}

#include "CatalogMod.h"
#include "CatalogScroll.h"

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

CMod *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pScroll = 0;
	pMod = this;
  CDebug::SetModuleName("Catalog");
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
	char ent[18];
	HINSTANCE hinst;

	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("Initialize")) {
		ptr = msg->getPtr("hInst");
		memmove(&hinst, ptr, sizeof(hinst));
		pScroll = new CScroll();
		pScroll->Init(hinst, "Catalog");

		// Get glint interface
		m = CreateMessage("IGlint");
		Dispatch(m);
		pScroll->pGlint = (IGlint *)m->getPtr("IGlint");
		if(!pScroll->pGlint) {
			msg->setInt("Res", 2);
			return true;
		}
		m->Release();

		pScroll->Refresh();
		return true;
	}

	if(msg->Is("Grok")) {
		str = msg->getStr("Quake");
		strcpy(ent, str);
		pScroll->Quake(ent);
		return true;
	}

	if(msg->Is("LOG_ORIGINS")) 
  {
    if(pScroll)
    {
      pScroll->SetOriginLogging(true);
      return true;
    }
  }

	if(msg->Is("OriginDel")) {
		str = msg->getStr("idOrigin");
		strcpy(ent, str);
		pScroll->Quake(ent);
		return true;
	}

	if(msg->Is("CatalogDebugLevel")) 
  {
    DebugOutputStruct dosDebug;
    int iLevel;
    memset(&dosDebug, 0, sizeof(dosDebug));
    
    iLevel = msg->getInt("Level");
    if(iLevel < DEBUG_MAX_LEVEL || iLevel > DEBUG_MIN_LEVEL)
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "Error parsing config file line: %s %d...\n", 
                  "CatalogDebugLevel", iLevel);
      return(true);
    }
    dosDebug.bOTF = msg->getInt("OTF");
    dosDebug.bOTD = msg->getInt("OTD");
    dosDebug.bOTE = msg->getInt("OTE");
    dosDebug.bOTS = msg->getInt("OTS");
    dosDebug.bOSM = msg->getInt("OSM");
    CDebug::SetLevelParams(iLevel, &dosDebug);
    return(true);
	}

	return false;
}


// Mod.cpp

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>

#include "GlintMod.h"
#include "glint.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pGlint = 0;
  CDebug::SetModuleName("Glint");
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {

	if(msg->Is("ShutDown")) {
		if(pGlint) {
			delete pGlint;
			pGlint = 0;
		}
		return true;
	}

	if(!pGlint) {
		pGlint = new CGlint();
	}

	// Process request for glint interface
	if(msg->Is("IGlint")) {
		msg->setPtr("IGlint", pGlint);
		return true;
	}

	if(msg->Is("GlintDebugLevel")) 
  {
    DebugOutputStruct dosDebug;
    int iLevel;
    memset(&dosDebug, 0, sizeof(dosDebug));
    
    iLevel = msg->getInt("Level");
    if(iLevel < DEBUG_MAX_LEVEL || iLevel > DEBUG_MIN_LEVEL)
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "Error parsing config file line: %s %d...\n", 
                  "GlintDebugLevel", iLevel);
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

  CDebug::Log(DEBUG_MINOR_INFO, "Glint ignored nexus message\n");
	return false;
}

// Mod.cpp

#include <windows.h>
extern "C" {
#include "utility.h"
}

#include "GlockMod.h"
#include "ISolve.h"
#include "glock.h"

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

CMod *pMod;

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pTT = 0;
	pGlint = 0;
	pSolve = 0;
	pTT = 0;
	pMod = this;
  iNumLocatorIterations = 1;  
  pGlock = NULL;
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	if(pSolve)
		pSolve->Release();
  if(pGlock)
    delete(pGlock);
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
	IMessage *m;
	char *str;
	char ent[32];
	char msk[16];
	int res;

	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("Initialize")) {
		// Get glint interface
		m = CreateMessage("ITravelTime");
		Dispatch(m);
		pTT = (ITravelTime *)m->getPtr("Interface");
		m->Release();
		if(!pTT) {
			msg->setInt("Res", 1);
			return true;
		}

		m = CreateMessage("IGlint");
		Dispatch(m);
		pGlint = (IGlint *)m->getPtr("IGlint");
		m->Release();
		if(!pGlint) {
			msg->setInt("Res", 2);
			return true;
		}

		m = CreateMessage("ISolve");
		Dispatch(m);
		pSolve = (ISolve *)m->getPtr("ISolve");
		m->Release();
		if(!pSolve) {
			msg->setInt("Res", 3);
			return true;
		}

    pGlock = new CGlock(pMod);

		return true;
	}

	if(msg->Is("Locate")) {
		if(!pGlint) {
			msg->setInt("Res", 1);
			return true;
		}
		str = msg->getStr("Quake");
		if(!str) {
			msg->setInt("Res", 2);
			return true;
		}
		strcpy(ent, str);
		str = msg->getStr("Mask");

		if(!str) {
			res = Locate(ent, 0);
		} else {
			strcpy(msk, str);
			res = Locate(ent, msk);
		}

		msg->setInt("Res", res);
		return true;
	}
	if(msg->Is("NumLocatorIterations")) 
  {
    iNumLocatorIterations = msg->getInt("Num");
    return true;
  }
	if(msg->Is("LocatorDebugLevel")) 
  {
    DebugOutputStruct dosDebug;
    int iLevel;
    memset(&dosDebug, 0, sizeof(dosDebug));
    
    iLevel = msg->getInt("Level");
    if(iLevel < DEBUG_MAX_LEVEL || iLevel > DEBUG_MIN_LEVEL)
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "Error parsing config file line: %s %d...\n", 
                  "LocatorDebugLevel", iLevel);
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

//---------------------------------------------------------------------------------------Locate
int CMod::Locate(char *ent, char *mask) {
	int res;

  if(!pGlock)
  {
    CDebug::Log(DEBUG_MAJOR_ERROR, "Error locator class unavailable.\n");
    return(1);
  }

	if(mask)
		res = pGlock->Locate(ent, mask);
	else
		res = pGlock->Locate(ent);
	return res;
}

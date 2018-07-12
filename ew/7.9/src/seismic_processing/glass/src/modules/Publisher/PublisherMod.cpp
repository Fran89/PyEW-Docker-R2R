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
#include "PublisherMod.h"
#include "PublisherScroll.h"
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
  CDebug::SetModuleName("Publisher");
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	if(pScroll)
		delete pScroll;
}

//---------------------------------------------------------------------------------------Poll
bool CMod::Poll() 
{
  Process();
  if(pScroll->tListBuilt < tLastChange)
  {
    pScroll->iSelectedPub = iLastOriginChanged;
    pScroll->UpdateList();
  }
  return(true);
}


//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
  IMessage *m;
  void *ptr;
  char *str;
  char ent[128];
  HINSTANCE hinst;
  char file[128];
  
  if(msg->Is("ShutDown")) {
    return true;
  }
  
  if(msg->Is("Initialize")) {
    ptr = msg->getPtr("hInst");
    memmove(&hinst, ptr, sizeof(hinst));
    pScroll = new CScroll();
    pScroll->Init(hinst, "Publisher");
    
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

    // init the publish code
    init(this);

		pScroll->Refresh();
		return true;
  }

	if(msg->Is("OriginAdd")) {
		str = msg->getStr("idOrigin");
		strcpy(ent, str);
    OriginChanged(ent, ORIGIN_CREATED);
		
    if(pScroll->tListBuilt < tLastChange)
      pScroll->UpdateList();
		return true;
	}

	if(msg->Is("OriginMod")) {
		str = msg->getStr("idOrigin");
		strcpy(ent, str);
    OriginChanged(ent, ORIGIN_RELOCATED);
		return true;
	}

	if(msg->Is("OriginDel")) {
		str = msg->getStr("idOrigin");
		strcpy(ent, str);
    CDebug::Log(DEBUG_MAJOR_INFO, "Received Del message for Origin(%s)\n",
                ent);
    OriginChanged(ent, ORIGIN_DELETED);
		return true;
	}

	if(msg->Is("OriginLink")) {
		str = msg->getStr("idOrigin");
		strcpy(ent, str);
    OriginChanged(ent, ORIGIN_PHASE_LINKED);
		return true;
	}
  
  if(msg->Is("OriginUnLink")) {
    str = msg->getStr("idOrigin");
    strcpy(ent, str);
    OriginChanged(ent, ORIGIN_PHASE_UNLINKED);
    return true;
  }
  
	if(msg->Is("PubAlgParams")) {
		str = msg->getStr("File");
		if(!str) {
			msg->setInt("Res", 1);
			return true;
		}
		strcpy(file, str);
		if(!AlgReadParams(file)) {
			msg->setInt("Res", 2);
			return true;
		}
		msg->setInt("Res", 0);
		return true;
	}

	if(msg->Is("PublisherDebugLevel")) 
  {
    DebugOutputStruct dosDebug;
    int iLevel;
    memset(&dosDebug, 0, sizeof(dosDebug));
    
    iLevel = msg->getInt("Level");
    if(iLevel < DEBUG_MAX_LEVEL || iLevel > DEBUG_MIN_LEVEL)
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "Error parsing config file line: %s %d...\n", 
                  "PublisherDebugLevel", iLevel);
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


void CMod::Publish(char * idOrigin, int iVersion)
{
	IMessage *m = CreateMessage("PublishOrigin");
	m->setStr("idOrigin",idOrigin);
	m->setInt("version",iVersion);
	pMod->Dispatch(m);
	m->Release();
}

void CMod::PrunePick(char * idOrigin, char * idPick)
{
	IMessage *m = CreateMessage("PrunePick");
	m->setStr("idOrigin",idOrigin);
	m->setStr("idPick",idPick);
	pMod->Dispatch(m);
	m->Release();
}

void CMod::Retract(char * idOrigin, int iOrigin)
{
	IMessage *m = CreateMessage("DeletePublishedOrigin");
	m->setStr("idOrigin",idOrigin);
	m->setInt("iOrigin",iOrigin);
	pMod->Dispatch(m);
	m->Release();
}
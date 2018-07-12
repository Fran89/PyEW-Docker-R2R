// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
extern "C" {
#include "utility.h"
}

#include "NetworkMod.h"
#include "station.h"

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() 
{
	pStaList = new CStation();
  CDebug::SetModuleName("Network");
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	delete pStaList;
}

//---------------------------------------------------------------------------------------Action
// All inter-module messaging comes through this single interface all. Always return
// "true" if the module type is of interest, even if a specific message is ignored. The
// reason for this is that in a future release of the "nexus" element, learning will
// occur such that modules that ignore a message will no longer receive that message.
bool CMod::Action(IMessage *msg) {
	char *file;

  SCNL Name;
  const STATION * pStation;

	// The "ShutDown" message is broadcast when the system is shutting down. Messages
	// can still be sent to other modules, and may be received from other modules,
	// but inevitably -- the end is nigh. Put things in order.
	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("NetLoad")) {
		file = msg->getStr("File");
		if(!file) {
			msg->setInt("Res", 1001);
			return true;
		}
		if(!pStaList->LoadHypoEllipse(file)) {
			msg->setInt("Res", 1002);
			return true;
		}
		msg->setInt("Res", 0);
		return true;
	}

	if(msg->Is("HypoLoad")) {
		file = msg->getStr("File");
		if(!file) {
			msg->setInt("Res", 1001);
			return true;
		}
		if(!pStaList->LoadHypoInverse(file)) {
			msg->setInt("Res", 1002);
			return true;
		}
		msg->setInt("Res", 0);
		return true;
	}

	if(msg->Is("NetFind")) {
		strcpy(Name.szSta, msg->getStr("Sta"));
		strcpy(Name.szComp, msg->getStr("Comp"));
		strcpy(Name.szNet, msg->getStr("Net"));
		strcpy(Name.szLoc, msg->getStr("Loc"));
		if(!(pStation = pStaList->Get(&Name))) 
		{
			msg->setInt("Res", 1003);
			return true;
		}
		else
		{
			msg->setDbl("Lat", pStation->dLat);
			msg->setDbl("Lon", pStation->dLon);
			msg->setDbl("Elev", pStation->dElev);
			msg->setDbl("Qual", pStation->dQual);
			/* removed use of sDesc field as it wasn't being used by anything. 
      szDesc = pStation->sDesc;
			if(szDesc)
			  msg->setStr("Desc", szDesc);
			else
              msg->setStr("Desc", "");
      *********************************/
			msg->setInt("Res", 0);
		}
		return true;
	}

	if(msg->Is("NetGet")) {
      STATION * StationList;
      int       nLen;
      pStaList->GetList(&StationList, &nLen);
			msg->setInt("pStart", (int)StationList);
			msg->setInt("iLen", nLen);
			msg->setInt("Res", 0);
  		return true;
	}

	return false;
}

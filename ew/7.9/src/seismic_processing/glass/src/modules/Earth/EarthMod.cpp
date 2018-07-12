// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
extern "C" {
#include "utility.h"
}

/*
#include "comfile.h"
#include "terra.h"
#include "TravelTime.h"
#include "ttt.h"
#include "str.h"
*/
#include "EarthMod.h"

#define DEG2RAD 57.29577951308

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() 
{
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() 
{
}

//---------------------------------------------------------------------------------------Action
// All inter-module messaging comes through this single interface all. Always return
// "true" if the module type is of interest, even if a specific message is ignored. The
// reason for this is that in a future release of the "nexus" element, learning will
// occur such that modules that ignore a message will no longer receive that message.
bool CMod::Action(IMessage *msg) 
{
  /*
	CTTT *pt;
	double d;
	double z;
	double t;
	double toa;
	CStr phs;
	int i;
  */
	char *file;
	int res;

	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("TTInit")) {
		file = msg->getStr("File");
    CDebug::Log(DEBUG_MINOR_WARNING,"Got TTInit message!\n");
		if(!file) {
			msg->setInt("Res", 1001);
			return true;
		}
    CDebug::Log(DEBUG_MINOR_WARNING,"Calling Init!\n");
		res = Init(file);
    CDebug::Log(DEBUG_MINOR_WARNING,"Returned from Init!\n");
		msg->setInt("Res", res);
		return true;
	}

	if(msg->Is("ITravelTime")) {
		msg->setPtr("Interface", &List);
		return true;
	}

	return false;
}

int CMod::Init(char *file) {
	/*CComFile cf;
	CStr cmd;
	CStr cstr;
	int nc;
	int nroot;
	char root[128];
	char ttt[128];
	char model[128];
	int i;
	int n;
	int ntrv;
	CTTT *pttt;
  */
  int rc;

  
  CDebug::Log(DEBUG_MINOR_WARNING,"Init()  Calling List.Load(%s)!\n", file);

  rc = List.Load(file);
  CDebug::Log(DEBUG_MINOR_WARNING,"Init()  List.Load() returned(%d)!\n",rc);

  if(rc <= 0)
  {
    CDebug::Log(DEBUG_MAJOR_ERROR, 
                "Init() could not load traveltime tables from file(%s).  Error(%d)\n",
                file, rc);
    exit(-1);
  }

	return 0;
}


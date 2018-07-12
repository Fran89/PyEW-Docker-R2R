// nexus.cpp
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "nexus.h"
#include "module.h"
#include "message.h"
#include "status.h"
extern "C" {
#include "utility.h"
}
#include <Debug.h>


static char szTrafficFileName[256];
#define szTRAFFIC_BASE_FILENAME "nexus_traffic_log.txt"
extern CDebug Debug;
//---------------------------------------------------------------------------------------CNexus
CNexus::CNexus() {
	bMonitor = true;	// Write all message traffic to traffic.txt if true;
  bMonitor = false;   // DK CLEANUP
	hInst = 0;
	pStat = 0;
	nPoll = 0;
	FILE *fdbg;
//	DebugOn();
	if(bMonitor) {
    char * szEW_LOG = getenv("EW_LOG");
    if(!szEW_LOG)
      szEW_LOG = "";
    _snprintf(szTrafficFileName,sizeof(szTrafficFileName),"%s%s",szEW_LOG,szTRAFFIC_BASE_FILENAME);
    szTrafficFileName[sizeof(szTrafficFileName) - 1] = 0x00;
    if(!(fdbg = fopen(szTrafficFileName, "w")))
    {
      CDebug::Log(DEBUG_MINOR_ERROR,"Could not open Nexus Traffic file (%s)\n",
                  szTrafficFileName);
    }
		if(fdbg)
    {
			fclose(fdbg);
      fdbg = NULL;
    }
	}
}

//---------------------------------------------------------------------------------------~CNexus
CNexus::~CNexus() {
	IModule *mod;
	if(pStat)
		delete pStat;
	int n = arrMod.GetSize();
	for(int i=0; i<n; i++) {
		mod = (IModule *)arrMod.GetAt(i);
		mod->Release();
	}
}

//---------------------------------------------------------------------------------------Add
// Add module to nexus. Module must extend the class IModule.
// The module name is specified for debugging purpuses
// Initially put all modules in poll response list. They are removed from the
// list the first time they return false (default if not overriden in module
// subclass.
bool CNexus::Add(IModule *mod) {
	if(nPoll < 100)
		pPoll[nPoll++] = mod;
	arrMod.Add(mod);
	return true;
}

//---------------------------------------------------------------------------------------Load
// Load module in COM form from named dll file
IModule *CNexus::Load(char *name) {
	char modnam[128];
	char txt[128];

	Debug.Log(DEBUG_MAJOR_INFO,"Before LoadLibrary...\n");
	HINSTANCE hModule = ::LoadLibrary(name);
	if(hModule == NULL) {
		Debug.Log(DEBUG_MAJOR_ERROR,"Module <%s> could not be loaded\n", name);
    exit(-1);
		return 0;
	}

	Debug.Log(DEBUG_MAJOR_INFO,"Before GetProcAddress...\n");
	FARPROC CreateModule = (FARPROC)::GetProcAddress(hModule, "CreateModule");
	if(CreateModule == NULL) {
		Debug.Log(DEBUG_MAJOR_ERROR,"Could not find CreateModule in module <%s>\n", name);
    exit(-1);
		return 0;
	}

	unsigned int i;
	int n = 0;
	// Strip out module name from library path name
	for(i=0; i<strlen(name)-4; i++) {
		if(name[i] == '/' || name[i] == '\\') {
			n = 0;
			continue;
		}
	//	if(name[i] == '.')
	//		break;
		modnam[n++] = name[i];
	}
	modnam[n] = 0;
	Debug.Log(DEBUG_MAJOR_INFO,"Before CreateModule <%x>...\n", CreateModule);
	IModule *mod = (IModule *)CreateModule();
	if(!mod) {
		Debug.Log(DEBUG_MAJOR_ERROR,"Could not create module <%s>\n", modnam);
    exit(-1);
		return 0;
	}
	Debug.Log(DEBUG_MAJOR_INFO,"Setting module name to <%s>\n", modnam);
	mod->setName(modnam);
	Debug.Log(DEBUG_MAJOR_INFO,"Load complete\n");

	if(bMonitor) {
		sprintf(txt, "Module <%s> loaded.", modnam);
		Dump(txt);
	}
	Add(mod);
	return mod;
}

//---------------------------------------------------------------------------------------Idle
// Called whenever message queue is empty to implement pseudo-multitasking. At least
// permit access to visualization interfaces during processing of long data inputs.
// Modules are removed from the polling queue whenever they return false, which is
// the default if the modulebase class Poll() method is not overriden when subclassed.
bool CNexus::Idle() {
	int i;
	bool b = false;
	IModule *mod;

	for(i=0; i<nPoll; i++) {
		mod = pPoll[i];
		if(mod->Poll()) {
			b = true;
		} else {
			pPoll[i] = pPoll[nPoll-1];
			nPoll--;
		}
	}
	return b;
}

//---------------------------------------------------------------------------------------Dispatch
// Distribute message to all attached module until one of them returns true;
bool CNexus::Dispatch(IMessage *msg) {
	int i;
	IModule *mod;
	char txt[256];

	msg->Dump(txt, sizeof(txt));
	Debug.Log(DEBUG_MINOR_INFO,"MSG:%s\n", txt);
	if(msg->Is("DISPLAY")) {
		if(hInst) {
			pStat = new CStatus();
			pStat->Init(hInst, "Nexus");
		}
		return true;
	}
	for(i=0; i<arrMod.GetSize(); i++) {
		mod = (IModule *)arrMod.GetAt(i);
		if(pStat) {
			msg->Dump(txt, sizeof(txt));
			pStat->Status(i, mod->getName(), txt);
		}
    CDebug::Log(DEBUG_MINOR_INFO, "DISPATCH %s >>>>>> %s\n", msg->getCode(), mod->getName());
		if(mod->Action(msg)) {
			if(bMonitor)
				Dump(' ', mod->getName(), msg);
			if(pStat) {
				msg->Dump(txt, sizeof(txt));
				pStat->Set(txt);
			}
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------------------Broadcast
// Distribute message to all attached module. Returns true if messasge processed by
// at least one module.
bool CNexus::Broadcast(IMessage *msg) {
	int i;
	IModule *mod;
	char txt[256];

	if(msg->Is("DISPLAY")) {
		if(hInst) {
			pStat = new CStatus();
			pStat->Init(hInst, "Nexus");
		}
		return true;
	}

	bool b = false;
	for(i=0; i<arrMod.GetSize(); i++) {
		mod = (IModule *)arrMod.GetAt(i);
		if(pStat) {
			msg->Dump(txt, sizeof(txt));
      pStat->Status(i, mod->getName(), txt);
		}
    CDebug::Log(DEBUG_MINOR_INFO, "BROADCAST %s >>>>>> %s\n", msg->getCode(), mod->getName());
		if(mod->Action(msg)) 
    {
			b = true;
			if(bMonitor)
				Dump(' ', mod->getName(), msg);
			if(pStat) {
				msg->Dump(txt, sizeof(txt));
				pStat->Set(txt);
			}
			continue;
		}
	}
	return b;
}

//---------------------------------------------------------------------------------------Dump
void CNexus::Dump(char *txt) {
	FILE *fdbg;

	
	if(bMonitor) {
    if(!(fdbg = fopen(szTrafficFileName, "a")))
    {
      CDebug::Log(DEBUG_MINOR_ERROR,"Could not open Nexus Traffic file (%s)\n",
        szTrafficFileName);
      return;
    }
    fprintf(fdbg, "%s\n", txt);
    fclose(fdbg);
    fdbg = NULL;
  }
}

//---------------------------------------------------------------------------------------Dump
void CNexus::Dump(char code, char *modnam, IMessage *msg) {

  if(bMonitor)
  {
  	char txt[256];
	  char dbg[300];
	  msg->Dump(txt, sizeof(txt));
	  sprintf(dbg, "%c %-12s %s", code, modnam, txt);
	  Dump(dbg);
  }
}

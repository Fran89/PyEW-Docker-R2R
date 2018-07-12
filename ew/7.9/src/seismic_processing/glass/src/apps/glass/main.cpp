/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: main.cpp 6858 2016-10-28 17:47:35Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/05/22 16:04:15  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.2  2004/09/16 01:21:05  davidk
 *     Changed exit call to return.  (main procedure, same function, just looks better.)
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:22  michelle
 *     New Hydra Import
 *
 *     Revision 1.2  2003/11/07 19:07:28  davidk
 *     removed Sleep() call from main loop.
 *
 *
 ****************************************************************/
// hello.cpp : Defines the entry point for the application.
//
//#define _USE_SPEECH_

#include <windows.h>
#include <stdlib.h>
#include "message.h"
#include "Imodule.h"
#include "nexus.h"
#include "comfile.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
};

extern "C" {
int get_prog_name2(char *szFullName, char * szProgName, int iProgNameBufferLen);
}

#ifdef _USE_SPEECH_
#include <sapi.h>
#include <stdio.h>
#include <string.h>
#include <atlbase.h>
#include "sphelper.h"
CComPtr<ISpRecoContext> cpRecoCtxt;
CComPtr<ISpRecoGrammar> cpGrammar;
CComPtr<ISpVoice> cpVoice;
void Say(char *phrase) {
	wchar_t cw[128];
	int i;
	int n = strlen(phrase);
	for(i=0; i<n; i++)
		cw[i] = phrase[i];
	cw[n] = 0;
	cpVoice->Speak(cw, SPF_ASYNC, NULL);
}
#endif


#define MODE_DISPATCH	1
#define MODE_BROADCAST	2

static int iMode = MODE_DISPATCH;

CMessage MessageFactory;

CDebug Debug;

void DebugOn()
{
  // this is a DEFAULT type function that
  // attempts to set Debug levels with a hatchet!
  // skip level 0, we always want those
  DebugOutputStruct dos;

  memset(&dos,0,sizeof(dos));
  dos.bOTD=1;
  dos.bOTE=1;
  dos.bOTF=1;

  for(int i=0; i <= DEBUG_MAX_LEVEL; i++)
  {
    CDebug::SetLevelParams(i,&dos);
  }
}

void DebugOff()
{
  // this is a DEFAULT type function that
  // attempts to set Debug levels with a hatchet!
  // skip level 0, we always want those
  DebugOutputStruct dos;

  memset(&dos,0,sizeof(dos));

  for(int i=1; i <= DEBUG_MAX_LEVEL; i++)
  {
    CDebug::SetLevelParams(i,&dos);
  }

}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char sin[256] = "c:/siam/prg/siam/siam.ini";
	CNexus Nexus;
	IMessage *msg;
	IModule *mod;
	MSG Msg;
	CStr cmd;
	CStr crd;
	CStr modnam;
	CStr arg;
	CStr val;
	CComFile cf;
	int nc;
  char szCmdLine[1024];
  char szProgramName[40];
  char * szCmd;

#ifdef _USE_SPEECH_
	HRESULT hr;
	hr = ::CoInitialize(NULL);
	if(FAILED(hr)) return 0;
	hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
	if(FAILED(hr))	return 0;
	Say("The rain in spain falls mainly on the plain");

#endif

  strncpy(szCmdLine, GetCommandLine(), sizeof(szCmdLine));
  szCmdLine[sizeof(szCmdLine)-1]=0x00;
  szCmd=strtok(szCmdLine," ");
  if(!szCmd)
    szCmd="GLASS";
  else
    get_prog_name2(szCmd,szProgramName,sizeof(szProgramName));

  CDebug::SetProgramName(szProgramName);
  CDebug::SetModuleName("main()");

  Nexus.hInst = hInstance;
	if(lpCmdLine)
		strcpy(sin, lpCmdLine);

  /* DK   Use CDebug::Log() for debug information 
   * FILE *fdbg = fopen("CDebug::txt", "w");
	 * if(fdbg)
	 *   fclose(fdbg);
   ********************************************************/

	CDebug::Log(DEBUG_MAJOR_INFO, "CmdLine <%s>\n", lpCmdLine);
	if(!cf.Open(sin)) {
		CDebug::Log(DEBUG_MAJOR_ERROR,"Cannot open initialization file <%s>\n", sin);
    return(-1);
	}
	while(true) {
		nc = cf.Read();
		if(nc < 0) {
			cf.Close();
			break;
		}
		if(nc < 1)
			continue;
		cmd = cf.Token();
		if(cmd.GetLength() < 1)
			continue;
		if(cf.Is("#"))
			continue;
		if(cf.Is("mod")) {
			modnam = cf.Token();
			mod = Nexus.Load(modnam.GetBuffer());
			if(mod)
				mod->Initialize(&Nexus, &MessageFactory);
			continue;
		}
		if(cf.Is("$")) {
			arg = cf.Token();
			val = cf.Token();
			cf.Arg(arg.GetBuffer(), val.GetBuffer());
			continue;
		}
		if(cf.Is("DEBUG_ON")) {
			DebugOn();
			continue;
		}
		if(cf.Is("DEBUG_OFF")) {
			DebugOff();
			continue;
		}
		if(cf.Is("BROADCAST")) {
			iMode = MODE_BROADCAST;
			continue;
		}
		if(cf.Is("DISPATCH")) {
			iMode = MODE_DISPATCH;
			continue;

		}
		if(cf.Is("TERMINATE")) {
			msg = MessageFactory.CreateMessage("ShutDown");
			if(Nexus.bMonitor)
				Nexus.Dump('B', "System", msg);
			Nexus.Broadcast(msg);
			msg->Release();
		//	ExitProcess(0);
			PostQuitMessage(0);
		}
		crd = cf.Card();
		msg = MessageFactory.CreateMessage(crd.GetBuffer());
		if(msg->Is("Initialize")) {
			msg->setPtr("hInst", &hInstance);
		}
		if(msg->Is("CommandLine")) {
			if(lpCmdLine)
				msg->setStr("Cmd", lpCmdLine);
		}
		switch(iMode) {
		case MODE_DISPATCH:
			if(Nexus.bMonitor)
				Nexus.Dump('D', "System", msg);
			Nexus.Dispatch(msg);
			break;
		case MODE_BROADCAST:
			if(Nexus.bMonitor)
				Nexus.Dump('B', "System", msg);
			Nexus.Broadcast(msg);
			break;
		}
		msg->Release();
	}
	cf.Close();

	while(true) {
		if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
			if(Msg.message == WM_QUIT) {
				CDebug::Log(DEBUG_MAJOR_INFO,"Message Loop : WM_QUIT received, terminating.\n");
				return 0;
			}
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		} else {
			Nexus.Idle();
		}
	}
//	int idTimer = SetTimer(0, 777, 1000, NULL);
//	while(GetMessage(&Msg, NULL, 0, 0)) {
//		TranslateMessage(&Msg);
//		DispatchMessage(&Msg);
//	}
//	KillTimer(0, idTimer);

#ifdef _USE_SPEECH_
//	::CoUninitialize();
#endif

	return 0;
}




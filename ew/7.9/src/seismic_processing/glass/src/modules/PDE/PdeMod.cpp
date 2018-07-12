// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
#include <stdio.h>
extern "C" {
#include "utility.h"
}

#include "PDEMod.h"
#include "str.h"
#include "date.h"

#define STATE_IDLE	0
#define	STATE_EW	1
#define STATE_PDE	2
#define STATE_NEIC	3

//---------------------------------------------------------------------------------------*/

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	iState = STATE_IDLE;
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
}

//---------------------------------------------------------------------------------------Action
// All inter-module messaging comes through this single interface all. Always return
// "true" if the module type is of interest, even if a specific message is ignored. The
// reason for this is that in a future release of the "nexus" element, learning will
// occur such that modules that ignore a message will no longer receive that message.
bool CMod::Action(IMessage *msg) {
	char *file;

	// The "ShutDown" message is broadcast when the system is shutting down. Messages
	// can still be sent to other modules, and may be received from other modules,
	// but inevitably -- the end is nigh. Put things in order.
	if(msg->Is("ShutDown")) {
		return true;
	}

	if(msg->Is("PDE")) {
		file = msg->getStr("File");
		if(!file)
			return false;
		return PDE(file);
	}

	if(msg->Is("EW")) {
		file = msg->getStr("File");
		if(!file)
			return false;
		return EW(file);
	}

	if(msg->Is("NEIC")) {
		file = msg->getStr("File");
		if(!file) {
			msg->setInt("Res", 1);
			return true;
		}
		if(!NEIC(file)) {
			msg->setInt("Res", 2);
			return true;
		}
		msg->setInt("Res", 0);
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------------Poll
bool CMod::Poll() {
	switch(iState) {
	case STATE_IDLE:
		break;
	case STATE_PDE:
		PDE();
		break;
	case STATE_EW:
		EW();
		break;
	case STATE_NEIC:
		NEIC();
		break;
	}
	return true;
}
/*
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 PDE-Q 2002   11 02 185041.92  63.52 -148.12  10 2.90 MLPMR   .... .......      
 PDE-Q 2002   11 02 195353.78  -5.16  151.48  33 4.70 mb GS   .... .......      
 PDE-Q 2002   11 02 195743.63  18.42  -67.42  20 2.90 MDRSPR 
 */   
//---------------------------------------------------------------------------------------NEIC
// Read PDE and PDE-Q catalog formation
bool CMod::NEIC(char *file) {
	if(iState != STATE_IDLE)
  {
		fclose(fFile);
    fFile=NULL;
  }
	iState = STATE_IDLE;
	
	if(!(fFile = fopen(file, "rt")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CMod::NEIC():  ERROR! Could not open input file (%s) for pick list!\n",
                file);
		return false;
  }
	iState = STATE_NEIC;
	nHyp = 0;
	while(true) {
		NEIC();
		if(iState != STATE_NEIC)
			break;
	}
	return true;
}
void CMod::NEIC() {
	IMessage *m;
	char txt[1024];
	CStr hyp;
	CDate dt;
	double ss;
	double lat;
	double lon;
	double z;
	int n;
	int i;

	txt[0] = 0;
  if(!fFile)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CMod::NEIC():  ERROR! fFile pointer not initialized.  Returning!\n");
		return;
  }
	if(!fgets(txt, sizeof(txt), fFile)) {
		iState = STATE_IDLE;
		fclose(fFile);
    fFile=NULL;
	}
	n = strlen(txt);
	if(n < 10)
		return;
	for(i=0; i<n; i++)
		if(txt[i] == 10 || txt[i] == 13)
			txt[i] = 0;
	hyp = CStr(txt);
	iYr = hyp.Long(4, 7);
	iMo = hyp.Long(5);
	iDa = hyp.Long(3);
	iHr = hyp.Long(3);
	iMn = hyp.Long(2);
	ss  = hyp.Double(5);
	lat = hyp.Double(7);
	lon = hyp.Double(8);
	z = hyp.Double(8);
	CDebug::Log(DEBUG_MINOR_INFO,"NEIC():n:%d yr:%d mo:%d da:%d hr:%d mn:%d ss:%.2f lat:%.2f lon:%.2f z:%.2f\n",
		n, iYr, iMo, iDa, iHr, iMn, ss, lat, lon, z);
	dt = CDate(iYr, iMo, iDa, iHr, iMn, ss);
	m = CreateMessage("QUAKE2K");
	m->setStr("Logo", "10.53.13");
	m->setInt("Seq", nHyp);
	m->setDbl("T", dt.Time());
	m->setDbl("Lat", lat);
	m->setDbl("Lon", lon);
	m->setDbl("Z", z);
	Broadcast(m);
	m->Release();
	nHyp++;
	return;
}

//---------------------------------------------------------------------------------------PDE
bool CMod::PDE(char *file) {
	if(iState != STATE_IDLE)
  {
		fclose(fFile);
    fFile=NULL;
  }
	iState = STATE_IDLE;
	
	if(!(fFile = fopen(file, "rt")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CMod::PDE():  ERROR! Could not open input file (%s)!\n",
                file);
		return false;
  }
	iState = STATE_PDE;
	return true;
}
void CMod::PDE() {
	IMessage *m;
	CDate dt;
	CStr hyp;
	CStr pck;
	char *str;
	char phase[12];
	char phs[12];
	char site[8];
	char txt[1024];
	char msg[1024];
	double ss;
	double lat;
	double lon;
	double z;
	int nhyp = 1000;
	int npck = 300;

	int n;
	int i;
  if(!fFile)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CMod::PDE():  ERROR! fFile pointer not initialized.  Returning!\n");
		return;
  }
	if(!fgets(txt, sizeof(txt), fFile)) {
		iState = STATE_IDLE;
		fclose(fFile);
    fFile=NULL;
	}
	n = strlen(txt);
	for(i=0; i<n; i++)
		if(txt[i] == 10 || txt[i] == 13)
			txt[i] = 0;
	if(txt[0] == 'H' && txt[1] == 'Y') {
		hyp = CStr(txt);
		iYr = hyp.Long(4, 2);
		iMo = hyp.Long(2);
		iDa = hyp.Long(2);
		iHr = hyp.Long(2, 11);
		iMn = hyp.Long(2);
		ss = hyp.Double(5);
		lat = hyp.Double(6,21);
		if(txt[27] == 'S')
			lat = -lat;
		lon = hyp.Double(7,29);
		if(txt[36] == 'W')
			lon = -lon;
		z = hyp.Double(5, 37);
		nhyp++;
		sprintf(msg, "  3 53%6d%4d%02d%02d%02d%02d%5.2f %8.4f %9.4f %5.2f",
			nhyp, iYr, iMo, iDa, iHr, iMn, ss, lat, lon, z);
		dt = CDate(iYr, iMo, iDa, iHr, iMn, ss);
		m = CreateMessage("QUAKE2K");
		m->setStr("Logo", "10.53.13");
		m->setPtr("Msg", msg);
		m->setInt("Seq", nhyp);
		m->setDbl("T", dt.Time());
		m->setDbl("Lat", lat);
		m->setDbl("Lon", lon);
		m->setDbl("Z", z);
		Broadcast(m);
		m->Release();
//			con->Log(msg);
//			logo.type = 101;
//			logo.mod = 53;
//			logo.instid = 13;
//			ringOut->Put(&logo, msg);
		return;
	}
	if(txt[0] == 'P' && txt[1] == ' ') {
		pck = CStr(txt);
		pck.String(2,0);
		str = pck.String();
		strcpy(site, str);
		str = pck.String();
		strcpy(phase, str);
		strcpy(phs, str);
		phs[2] = 0;
		iHr = pck.Long(2, 15);
		iMn = pck.Long(2);
		ss = pck.Double(5);
		npck++;
		sprintf(msg, " 10 53 13 %4d %-5s        %-2s%4d%02d%02d%02d%02d%5.2f %s",
			npck, site, phs, iYr, iMo, iDa, iHr, iMn, ss, phase);
	//	TRACE("PDE:%s\n", msg);
		dt = CDate(iYr, iMo, iDa, iHr, iMn, ss);
		m = CreateMessage("PICK2K");
		m->setStr("Logo", "10.53.13");
		m->setPtr("Msg", msg);
		m->setInt("Seq", npck);
		m->setStr("S", site);
		m->setStr("Phs", phs);
		m->setDbl("T", dt.Time());
		Broadcast(m);
		m->Release();
//			con->Log(msg);
//			logo.type = 10;
//			logo.mod = 53;
//			logo.instid = 13;
//			ringOut->Put(&logo, msg);
		return;
	}
	if(txt[0] == 'S' && txt[1] == ' ') {
		pck = CStr(txt);
		pck.String(7, 0);
		for(i=0; i<3; i++) {
			str = pck.String(8, 7+18*i);
			if(!str)
				break;
			strcpy(phase, str);
			if(phase[0] == 'D')
				continue;
			if(phase[0] == ' ' || phase[0] == 0)
				break;
			strcpy(phs, str);
			phs[2] = 0;
			iHr = pck.Long(2);
			iMn = pck.Long(2);
			ss = pck.Double(5);
			npck++;
			sprintf(msg, " 10 53 13 %4d %-5s        %-2s%4d%02d%02d%02d%02d%5.2f %s",
				npck, site, phs, iYr, iMo, iDa, iHr, iMn, ss, phase);
		//	TRACE("PDE:%s\n", msg);
			dt = CDate(iYr, iMo, iDa, iHr, iMn, ss);
			m = CreateMessage("PICK2K");
			m->setStr("Logo", "10.53.13");
			m->setPtr("Msg", msg);
			m->setInt("Seq", npck);
			m->setStr("S", site);
			m->setStr("Phs", phs);
			m->setDbl("T", dt.Time());
			Broadcast(m);
			m->Release();
//				con->Log(msg);
//				logo.type = 10;
//				logo.mod = 53;
//				logo.instid = 13;
//				ringOut->Put(&logo, msg);
		}
		return;
	}
}

//---------------------------------------------------------------------------------------PDE
bool CMod::EW(char *file) {
	if(iState != STATE_IDLE)
  {
		fclose(fFile);
    fFile=NULL;
  }
	iState = STATE_IDLE;
	
	if(!(fFile = fopen(file, "rt")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CMod::EW():  ERROR! Could not open input file (%s)!\n",
                file);
		return false;
  }
	iState = STATE_EW;
	return true;
}

void CMod::EW() {
	IMessage *m;
	char *str;
	char txt[1024];
	char logo[32];
	char site[32];
	char comp[32];
	CDate dt;
	CStr pck;
	int n;
	int i;
	int type;
	int mod;
	int inst;
	int seq;
	double sec;

  if(!fFile)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CMod::EW():  ERROR! fFile pointer not initialized.  Returning!\n");
		return;
  }
	if(!fgets(txt, sizeof(txt), fFile)) {
		iState = STATE_IDLE;
		fclose(fFile);
    fFile=NULL;
		return;
	}
	n = strlen(txt);
	for(i=0; i<n; i++)
		if(txt[i] == 10 || txt[i] == 13)
			txt[i] = 0;
	pck = CStr(txt);
	type = pck.Long(3);
	mod  = pck.Long(3);
	inst = pck.Long(3);
	sprintf(logo, "%d.%d.%d", type, mod, inst);
	seq = pck.Long(5);
	str = pck.String(5, 15);
	strcpy(site, str);
	str = pck.String(5, 20);
	strcpy(comp, str);
	iYr = pck.Long(4, 30);
	iMo = pck.Long(2);
	iDa = pck.Long(2);
	iHr = pck.Long(2);
	iMn = pck.Long(2);
	sec = pck.Double(5);
	dt = CDate(iYr, iMo, iDa, iHr, iMn, sec);

	m = CreateMessage("PICK2K");
	m->setStr("Logo", logo);
	m->setInt("Seq", seq);
	m->setStr("S", site);
	m->setStr("Phs", "P");
	m->setDbl("T", dt.Time());
	Broadcast(m);
	m->Release();
	iState = STATE_EW;
}
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: GlassMod.cpp 2176 2006-05-22 16:04:15Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/05/22 16:01:25  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.4  2006/01/24 00:05:01  davidk
 *     reverting to NON-DEBUG version.  no functional changes.
 *
 *     Revision 1.2  2005/08/15 22:43:24  davidk
 *     Downgraded a warning about Channel not found, as it was filling up the logfile,
 *     since under the current configuration, we removed noisy stations from the
 *     glass logfile.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.9  2005/06/16 20:17:13  davidk
 *     removed unreferenced variable x from Action() method.
 *
 *     Revision 1.8  2005/04/18 20:26:23  davidk
 *     Part of the integration of glass algorithms into hydra.
 *     Removed support for "Sphere" message, which tested the sphere code.
 *     Sphere code is well tested and now resides in hydra libsrc.
 *
 *     Revision 1.7  2005/02/28 17:51:17  davidk
 *     Added support for PrunePick message, that other modules can send to
 *     glass to get glass to prune a single pick from a given origin.
 *     Added comments to miscalaneous code.
 *
 *     Revision 1.6  2004/10/29 16:11:31  davidk
 *     Moved the glass version string to GlassMod.h.  Updated to v1.14, built 10/29/04.
 *
 *     Revision 1.5  2004/09/16 01:31:27  davidk
 *     Updated version # and date.
 *     v1.13 reflects cleanup of error reporting and logging.
 *     Includes cleanup of focus modules.
 *
 *     Revision 1.4  2004/09/16 00:57:09  davidk
 *     Added command for setting debug levels for this module from the glass.d
 *     config file.
 *     Cleaned up logging messages.
 *     Downgraded some debugging messages.
 *
 *     Revision 1.3  2004/08/06 01:25:45  davidk
 *     Update to v1.12, with new GUI look, and hardcoded publishing module that
 *     replaces globalproc.
 *
 *     Revision 1.2  2004/04/01 22:05:43  davidk
 *     v1.11 update
 *     Added internal state-based processing.
 *
 *     Revision 1.6  2003/11/07 22:32:00  davidk
 *     Added RCS header.
 *     Upped version number to 1.0 build 8 (20031107).
 *     Removed refernce to ReportInterval config file param, as
 *     it is no longer relevant.
 *
 *
 **********************************************************/

// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
#include "ITravelTime.h"
#include "str.h"
//#include "sphere.h"
#include "monitor.h"
extern "C" {
#include "utility.h"
}

#include "GlassMod.h"
#include "glass.h"

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
	pGlass = new CGlass();
	pGlass->pMod = this;
	bSpockReport = false;
	pMon = 0;
  CDebug::SetModuleName("Glass");
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	CDebug::Log(DEBUG_FUNCTION_TRACE, "CMod::~Cmod() entered\n");
	if(pMon) {
		pMon->CatOut();
		delete pMon;
	}
	delete pGlass;
	CDebug::Log(DEBUG_FUNCTION_TRACE, "CMod::~Cmod() finished\n");
}

//---------------------------------------------------------------------------------------Action
// All inter-module messaging comes through this single interface all. Always return
// "true" if the module type is of interest, even if a specific message is ignored. The
// reason for this is that in a future release of the "nexus" element, learning will
// occur such that modules that ignore a message will no longer receive that message.
bool CMod::Action(IMessage *msg) {
	HINSTANCE hinst;
	char file[128];
	void *ptr;
	IMessage *m;
	char *s;
	char *s2;
  /*
	CSphere sph;
	CSphere sph1;
	CSphere sph2;
	double x[6];
  */

	ITravelTime *ptt;

  // "Initialize"
	if(msg->Is("Initialize")) 
  {
		ptr = msg->getPtr("hInst");
		memmove(&hinst, ptr, sizeof(hinst));
		hInst = hinst;

		// Get traveltime interface
		m = CreateMessage("ITravelTime");
		Dispatch(m);
		ptt = (ITravelTime *)m->getPtr("Interface");
		m->Release();
		if(!ptt) {
			msg->setInt("Res", 1);
			return true;
		}
		pGlass->SetTravelTimePointer(ptt);

		// Get glint interface
		m = CreateMessage("IGlint");
		Dispatch(m);
		pGlass->pGlint = (IGlint *)m->getPtr("IGlint");
		if(!pGlass->pGlint) {
			msg->setInt("Res", 2);
			return true;
		}
		m->Release();

		// Get report interface
		m = CreateMessage("IReport");
		Dispatch(m);
		pReport = (IReport *)m->getPtr("Interface");
		if(!pReport) {
			m->Release();
			bSpockReport = false;
			return true;
		}
		m->Release();
		return true;
	}

  // "ShutDown"
	if(msg->Is("ShutDown")) {
		return true;
	}

  // "GlassParams"
	if(msg->Is("GlassParams")) {
		s = msg->getStr("File");
		if(!s) {
			msg->setInt("Res", 1);
			return true;
		}
		strcpy(file, s);
		if(!pGlass->Params(file)) {
			msg->setInt("Res", 2);
			return true;
		}
		msg->setInt("Res", 0);
		return true;
	}

  // "GlassNoAssociate"
	// Turn off association, only assign to known locations
	if(msg->Is("GlassNoAssociate")) {
		pGlass->bAssociate = false;
		return true;
	}

  // "GlassNoLocate"
	// Turn off location capability, calculate residuals and rms, but
	// do not adjust solution from known (specified) location
	if(msg->Is("GlassNoLocate")) {
		pGlass->bLocate = false;
		return true;
	}

  // "GlassMonitor"
	if(msg->Is("GlassMonitor")) {
		hinst = hInst;
		pMon = new CMonitor();
		pMon->pGlass = pGlass;
		pMon->Init(hinst, "Glass Status  -- " GLASS_VERSION_STRING);
		pGlass->pMon = pMon;
		return true;
	}

  // "SpockReport"
	if(msg->Is("SpockReport")) {
		bSpockReport = true;
		return true;
	}

// DK 1.0 PULL  	if(msg->Is("ReportInterval")) 
// DK 1.0 PULL    {
// DK 1.0 PULL  		CDebug::Log(DEBUG_MAJOR_INFO, "ReportInterval\n");
// DK 1.0 PULL  				if(pGlass)
// DK 1.0 PULL        pGlass->iReportInterval = msg->getInt("Interval");
// DK 1.0 PULL  		CDebug::Log(DEBUG_MAJOR_INFO, "  iReportInterval=%d\n", pGlass->iReportInterval);
// DK 1.0 PULL  		return true;
// DK 1.0 PULL  	}

	CStr clogo;
	char logo[16];
	char sta[16];
	char comp[8];
	char net[8];
	char loc[8];
	char phs[8];
	double t;
	double z;
	int iseq;
	double lat;
	double lon;
	double elev;


  // "QUAKE2K"
	if(msg->Is("QUAKE2K")) {
		t = msg->getDbl("T");
		lat = msg->getDbl("Lat");
		lon = msg->getDbl("Lon");
		z = msg->getDbl("Z");
		pGlass->Origin(t, lat, lon, z);
		return true;
	}

  // "PICK2K"
	if(msg->Is("PICK2K")) {
		s = msg->getStr("Logo");
		if(s)
			strcpy(logo, s);
		else
			return true;
		iseq = msg->getInt("Seq");
		s = msg->getStr("S");
		if(s)
			strcpy(sta, s);
		else
			return true;
		s = msg->getStr("C");
		if(s)
			strcpy(comp, s);
		else
			strcpy(comp, SCNL_UNKNOWN_STR);
		s = msg->getStr("N");
		if(s)
			strcpy(net, s);
		else
			strcpy(net, SCNL_UNKNOWN_STR);
		s = msg->getStr("L");
		if(s)
			strcpy(loc, s);
		else
			strcpy(loc, SCNL_UNKNOWN_STR);
		s = msg->getStr("Phs");
		if(s)
			strcpy(phs, s);
		else
			strcpy(phs, PHASE_UNKNOWN_STR);
		t = msg->getDbl("T");
		m = CreateMessage("NetFind");
		m->setStr("Sta", sta);
		m->setStr("Comp", comp);
		m->setStr("Net", net);
		m->setStr("Loc", loc);
		if(Dispatch(m)) {
			if(m->getInt("Res")) {
				m->Release();
        CDebug::Log(DEBUG_MINOR_INFO,"Channel (%s.%s.%s.%s) not found.\n",sta,comp,net,loc);
				return true;
			}
			lat = m->getDbl("Lat");
			lon = m->getDbl("Lon");
			elev = m->getDbl("Elev");
			pGlass->Pick(sta, comp, net, loc, phs, t, lat, lon, elev,
				logo, iseq);
		}
		m->Release();
		return true;
	}

  // "sphere"
	/*if(msg->Is("sphere")) {
		sph.Set(0.0, 0.0, 0.0, 100.0);
		sph1.Set(50.0, 0.0, 0.0, 70.0);
		sph2.Set(0.0, 50.0, 0.0, 80.0);
		sph.Intersect(&sph1, &sph2, x);
		return true;
	}
  */

  // "GlassDebugLevel"
	if(msg->Is("GlassDebugLevel")) 
  {
    DebugOutputStruct dosDebug;
    int iLevel;
    memset(&dosDebug, 0, sizeof(dosDebug));
    
    iLevel = msg->getInt("Level");
    if(iLevel < DEBUG_MAX_LEVEL || iLevel > DEBUG_MIN_LEVEL)
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "Error parsing config file line: %s %d...\n", 
                  "GlassDebugLevel", iLevel);
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

  // "PrunePick"
	if(msg->Is("PrunePick")) 
  {
		s = msg->getStr("idOrigin");
		if(!s) {
			msg->setInt("Res", 1);
			return true;
		}
		s2 = msg->getStr("idPick");
		if(!s2) {
			msg->setInt("Res", 1);
			return true;
		}
		if(!pGlass->PrunePick(s,s2)) {
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
	pGlass->Poll();
	return true;
}


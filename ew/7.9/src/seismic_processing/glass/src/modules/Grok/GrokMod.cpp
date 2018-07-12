// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
#include <windows.h>
extern "C" {
#include "utility.h"
}

#include "GrokMod.h"
#include "mapwin.h"
#include "ohia.h"
//#include "visualpick.h"
#include "pvisualpicklist.h"
#include "IGlint.h"
#include <Debug.h>

CMod *pMod;

extern "C"   __declspec(dllexport) IModule * CreateModule() {
  IModule *mod = (IModule *)new CMod();
  return mod;
}

//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
  pWin = 0;
  pGlint = 0;
  bShowPicks = false;
  CDebug::SetModuleName("Grok");
}

//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
  IMessage *m;
  HINSTANCE hinst;
  ORIGIN *org;
  PICK *pck;
  COhia *ohia;
  void *ptr;
  char *str;
  char base[256];
  char ent[32];
  int res;
  static CVisualPickList * pcvpList=NULL;

  static time_t tLastUpdate = 0;
  time_t tNow;

  PhaseType ptPhase;

#define THRESHOLD_TIME 10

//# define TIME_BETWEEN_REDRAWS 60
//  static time_t tLastRedraw;
//  time_t tNow;

  if(msg->Is("ShutDown")) {
    return true;
  }

  if(msg->Is("Initialize")) {
    ptr = msg->getPtr("hInst");
    memmove(&hinst, ptr, sizeof(hinst));
    pWin = new CMapWin();
    pWin->Init(hinst);
    // Get glint interface
    m = CreateMessage("IGlint");
    Dispatch(m);
    pGlint = (IGlint *)m->getPtr("IGlint");
    m->Release();
    return true;
  }

  if(msg->Is("PICK2K")) {
    char *s;
    char sta[20],comp[20],net[20],loc[20];
    double t=0.0,lat=0.0,lon=0.0;

    if(bShowPicks)
    {
      if(!pcvpList)
      {
        pcvpList = new CVisualPickList();
        if(pWin)
          pWin->Entity(new CpVisualPickList(pcvpList));
      }
    }

    //iseq = msg->getInt("Seq");
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
    t = msg->getDbl("T");
//    TRACE("Grock 1 Calling NetFind\n");
    m = CreateMessage("NetFind");
    m->setStr("Sta", sta);
    m->setStr("Comp", comp);
    m->setStr("Net", net);
    m->setStr("Loc", loc);
    if(Dispatch(m)) 
    {
      if(m->getInt("Res")) 
      {
        m->Release();
        return true;
      }
      lat = m->getDbl("Lat");
      lon = m->getDbl("Lon");
      //elev = m->getDbl("Elev");

    }
    else 
    {
//      TRACE("Grock 1 Exception\n");
    }
//    TRACE("Grock 1 End NetFind\n");
    m->Release();


    if(bShowPicks)
    {
      CVisualPick * pvPick = new CVisualPick(t,lat,lon);
      pcvpList->AddPick(pvPick);
      if(time(&tNow) - tLastUpdate > THRESHOLD_TIME)
      {
        tLastUpdate = tNow;
        pWin->Refresh();
      }
    }

//    TRACE("Grock 1 End - Handle Pick\n");
  }

  
  if(msg->Is("World")) {
    if(!pWin) {
      msg->setInt("Res", 1);
      return true;
    }
    str = msg->getStr("Base");
    if(!str) {
      msg->setInt("Res", 2);
      return false;
    }
    strcpy(base, str);
    res = pWin->Load(base);
    pWin->Refresh();
    msg->setInt("Res", res);
    return true;
  }

  if(msg->Is("GrokTest")) {
    ohia = new COhia();
    ohia->Color(0, 255, 0);
    ohia->Ray(0.0, 0.0);
    ohia->Ray(10.0, -13.5);
    ohia->Ray(-5.0, 5.0);
    ohia->Ray(7.0, 2.3);
    ohia->Ray(-1.3, 12.5);
    pWin->Entity(ohia);
    pWin->Refresh();
    return true;
  }

  if(msg->Is("Grok")) {
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
    org = pGlint->getOrigin(ent);
    if(!org) {
      msg->setInt("Res", 3);
      return true;
    }
    ohia = new COhia();
    ohia->Color(255, 0, 0);
    ohia->Ray(org->dLat, org->dLon);

    size_t iPickRef=0;
    while(pck = pGlint->getPicksFromOrigin(org, &iPickRef)) 
    {
      if(!pck->bTTTIsValid)
        continue;
      ptPhase = GetPhaseType(pck->ttt.szPhase);
      // DK CLEANUP the pick doesn't have any traveltime info
      // and no traveltime table.
      ohia->Ray(pck->dLat, pck->dLon, ptPhase);
    }
    pWin->Purge();
    pWin->Entity(ohia);
    if(bShowPicks)
      if(pcvpList)
        pWin->Entity(new CpVisualPickList(pcvpList));
    pWin->Refresh();
    msg->setInt("Res", 0);
    return true;
  }

  if(msg->Is("SHOW_PICKS")) {
    bShowPicks = true;
    return true;
  }

  return false;
}

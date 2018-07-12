/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: EarthwormMod.cpp 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/12/27 18:05:23  paulf
 *     sync from hydra_proj circa Oct 2007
 *
 *     Revision 1.7  2006/05/04 17:16:19  davidk
 *     removed unneccessary windows.h include, as it was causing winsock issues.
 *
 *     Revision 1.6  2006/01/24 00:05:01  davidk
 *     reverting to NON-DEBUG version.  no functional changes.
 *
 *     Revision 1.3  2006/01/19 21:03:39  davidk
 *     Added additional error messaging based on possibly return codes from Ring.Get().
 *
 *     Revision 1.2  2005/12/08 18:08:04  davidk
 *     Changed code to match change to PICK struct dQuality - > dPickQual
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.11  2004/11/02 19:16:13  davidk
 *     removed some superfluous Loc assign statements that were accidentally added
 *     in the last revision.
 *
 *     Revision 1.10  2004/11/02 18:32:40  davidk
 *     Updated info logging.
 *     Added a summary log statement for each origin publication.
 *     Downgraded the logging of the full origin from DEBUG_MAJOR_INFO to
 *     DEBUG_MINOR_INFO when an origin is published.
 *     Full global loc messages were cluttering up the logfiles during busy times.
 *
 *     Revision 1.9  2004/11/01 06:29:12  davidk
 *     Added check to make sure that each pick has valid origin-pick traveltime info before
 *     including it in the list for the given Origin.
 *
 *     Revision 1.8  2004/10/29 15:28:58  davidk
 *     Modified the "Delete Origin" code to use the numeric portion of the glass originid,
 *     instead of the entire string.  This way the originid matches what is used in the
 *     "New/Modified Origin" code.
 *     Fixes a bug where glevt2ora was blowing up during deletion, because it couldn't
 *     match the glass eventid(originid).
 *
 *     Revision 1.7  2004/09/16 00:31:56  davidk
 *     Added command for setting debug levels for this module from the glass.d
 *     config file.
 *     Cleaned up logging messages.
 *     Downgraded debugging messages so they will not normally get logged to file.
 *
 *     Revision 1.6  2004/08/24 15:54:41  davidk
 *     Changed loc message output, so that the numerical portion of the
 *     idOrigin field (generated from the unique_origin_sequence file), is
 *     used as the source eventid, instead of the iOrigin field.
 *     Fixes a bug where the eventid wasn't unique after simple glass restarts.
 *
 *     Revision 1.5  2004/08/06 00:16:47  davidk
 *     Added code to allow glass to produce global_loc and delete_global_loc
 *     EW messages.
 *
 *     Revision 1.4  2004/07/15 17:15:24  davidk
 *     Psycho picker fix.
 *     Ensure that the date/time string in the pick message is long enough to parse.
 *     Throw it away if it's not, otherwise Glass will crash while trying to parse the pick.
 *
 *     Revision 1.3  2004/04/09 22:50:23  davidk
 *     Fixed thr_ret "fun" issue, so that it works with the current earthworm header files.
 *
 *     Revision 1.2  2004/04/01 22:01:22  davidk
 *     v1.11 update
 *     Enabled unlink messages.
 *
 *     Revision 1.3  2003/11/07 19:16:53  davidk
 *     Changed the method of Polling for the Earthworm module.
 *     Modified it to use the standard Nexus polling method, instead
 *     of the funky Timer based back-door polling method that Carl
 *     was using.
 *
 *     Added a Sleep() per Carl, to have the module (and thus the
 *     program) sleep, whenever there are no picks available to process.
 *     This effectively prevents glass from pegging the CPU during idle
 *     times.
 *
 *     Added error reporting for EW MessageType retrieval calls.
 *
 *     Commented out "FULL" quake message type, added as part of v1.04.
 *
 *
 **********************************************************/

// Mod.cpp

// This file is a template used to create module for integration into the HSDI control
// system (CS).

// Generally, windows.h and util.h need only be included if TRACE functionality is desired.
//#include <windows.h>
#include <time.h>
#include <stdio.h>
/* removed special thr_ret definition, no longer needed */
extern "C" {
#include "earthworm.h"
#include "utility.h"
#include <global_loc_rw.h>
}
#include "EarthwormMod.h"
#include "ring.h"
#include "str.h"
#include "date.h"
#include "comfile.h"
#include "transport.h"
#include "platform.h"
#include <Debug.h>

extern "C"   __declspec(dllexport) IModule * CreateModule() {
	IModule *mod = (IModule *)new CMod();
	return mod;
}


static int iMsgsProcessed; // Added to count the number of picks entering the system 
                           // during a processing cycle. DK 103103

static char szPickLogfileName[256];
#define szPICK_LOGFILE_BASE_NAME "glass_pick_log.txt"
#define GLOBAL_LOC_MAX_STR_SIZE 65536


// Function for issuing status messages via an earthworm ring (statmgr)
/* We offer this function up to GlobalDebug, so that GlobalDebug()
   can call it to issue status messages via earthworm
 **********************************************************************/
static int IssueStatus(int iType, const char * szOutput);

/**************************************************************************
CMod *pMod;
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime) {
	if(pMod)
		pMod->Poll(dwTime);
}
**************************************************************************/


static CMod * pLocalMod = NULL;



//---------------------------------------------------------------------------------------Mod
// Standard constructor
CMod::CMod() {
// DK	pMod = this;
	pRingIn = 0;
	pRingOut = 0;
	cInst = 0;
	cMod = 0;
	iPulse = 10;
	tPulse = 0.0;
	bFlush = true;
	bLogFile = false;
	cTypePick2K = 0;
	cTypePickGlobal = 0;
	cTypeQuake = 0;
	cTypeLink = 0;
	cTypeUnLink = 0;
	cTypeLocGlobal = 0;
	cTypeDelLocGlobal = 0;
	cTypeHeartBeat = 0;
	cTypeError = 0;
  bLogPicks = false;
  CDebug::SetModuleName("Earthworm");

  // Set the function GlobalDebug uses to issue
  //  earthworm status messages to "IssueStatus"
  GlobalDebug::SetStatusFunction(IssueStatus);

  // Set the local(static) pointer used by "IssueStatus"
  //  to the current object, so that it can reference the
  // objects earthworm type info and functions.
  pLocalMod = this;

  pGlint = NULL;
  pLocBuffer = NULL;
}



//---------------------------------------------------------------------------------------~Mod
// Standard destructor
CMod::~CMod() {
	KillTimer(0, iTimer);
	if(pRingIn)
		delete pRingIn;
	if(pRingOut)
		delete pRingOut;
  if(pLocBuffer)
  {
    free(pLocBuffer);
    pLocBuffer = NULL;
  }
}

//---------------------------------------------------------------------------------------Action
bool CMod::Action(IMessage *msg) {
	CDate dt;
	static char mess[256];
	char sTag[32];
  char * sTmpTag;
	int ktag[10];
	int i;
	char *str;
	int key;
	int iorg;
  char * idOrigin;
	MSG_LOGO logo;
	IMessage *m;
  char    * szIDPick;
  char    * szIDOrigin;
  PICK    * pPick;
  ORIGIN  * pOrg;

	CDebug::Log(DEBUG_MINOR_INFO,"EarthWorm <%s>\n", msg->getCode());

  /****************************
   *      Initialize          *
   ****************************/
	if(msg->Is("Initialize")) 
  {
    int bExit = false;

		if(!cInst)
			cInst = GetLocalInst();
		if(!cTypeQuake)
		{
			cTypeQuake = GetType("TYPE_QUAKE2K");
			if(cTypeQuake== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_QUAKE2K\n");
        bExit = true;
      }
		}
		if(!cTypeLink)
		{
			cTypeLink = GetType("TYPE_LINK");
			if(cTypeLink== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_LINK\n");
        bExit = true;
      }
		}
    if(!cTypeUnLink)
      cTypeUnLink = GetType("TYPE_UNLINK");
			if(cTypeUnLink== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_UNLINK\n");
        bExit = true;
      }

		if(!cTypePickGlobal)
		{
			cTypePickGlobal = GetType("TYPE_PICK_GLOBAL");
			if(cTypePickGlobal== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_PICK_GLOBAL\n");
        bExit = true;
      }
		}

		if(!cTypePick2K)
		{
			cTypePick2K = GetType("TYPE_PICK2K");
			if(cTypePick2K== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_PICK2K\n");
        bExit = true;
      }
		}
		if(!cTypeHeartBeat)
		{
			cTypeHeartBeat = GetType("TYPE_HEARTBEAT");
			if(cTypeHeartBeat== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_HEARTBEAT\n");
        bExit = true;
      }
		}

		if(!cTypeError)
		{
			cTypeError = GetType("TYPE_ERROR");
			if(cTypeError== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_ERROR\n");
        bExit = true;
      }
		}

		if(!cTypeLocGlobal)
		{
			cTypeLocGlobal = GetType("TYPE_LOC_GLOBAL");
			if(cTypeLocGlobal== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_LOC_GLOBAL\n");
        bExit = true;
      }
		}
		if(!cTypeDelLocGlobal)
		{
			cTypeDelLocGlobal = GetType("TYPE_DEL_LOC_GLOBAL");
			if(cTypeDelLocGlobal== -1)			
      {
				CDebug::Log(DEBUG_MAJOR_ERROR,
				  "Could not get EW Message type for TYPE_DEL_LOC_GLOBAL\n");
        bExit = true;
      }
		}
		iPid = getpid();
// DK Remove back door Poll(time) call		iTimer = SetTimer(0, 0, 1000, TimerProc);


		// Get glint interface
		m = CreateMessage("IGlint");
		Dispatch(m);
		pGlint = (IGlint *)m->getPtr("IGlint");
		if(!pGlint) 
    {
			msg->setInt("Res", 2);
			CDebug::Log(DEBUG_MAJOR_ERROR,"EarthwormMod.Initialize() ERROR: "
				                          "Can't get GLINT ptr.\n");
      bExit = true;
		}
		m->Release();

    pLocBuffer = (char *) calloc(1, GLOBAL_LOC_MAXBUFSIZE+1);
    if(!pLocBuffer)
    {
			CDebug::Log(DEBUG_MAJOR_ERROR,"EarthwormMod.Initialize() ERROR: "
 			                              "Can't allocate (%d) bytes for pLocBuffer.\n", 
                  GLOBAL_LOC_MAXBUFSIZE+1);
      bExit = true;
    }
    if(bExit)
      exit(-1);

		return true;
	}

  /****************************
   *      NoFlushInput        *
   ****************************/
	if(msg->Is("NoFlushInput")) {
		bFlush = false;
		return true;
	}

  /****************************
   *      LogPicks            *
   ****************************/
	if(msg->Is("LogPicks")) 
  {
		SetPickLogging(true);
		return true;
	}
  // IssueStatus message is not currently used
	/* if(msg->Is("IssueStatus")) 
   *{
   * int iType = msg->getInt("Type");
	 *	str = msg->getStr("Msg");
   *  IssueStatus(iType,str);
 	 *	return true;
	 *}
   ******************/

  /****************************
   *  DeletePublishedOrigin   *
   ****************************/
	if(msg->Is("DeletePublishedOrigin")) 
  {
    if(!pRingOut)
			return true;
		logo.type = cTypeDelLocGlobal;
		logo.mod = cMod;
		logo.instid = cInst;
    int iOrigin = msg->getInt("iOrigin");
    idOrigin = msg->getStr("idOrigin");

    char * sz0 = strchr(idOrigin, '0');
    int iIDOrigin;
    if(sz0)
      iIDOrigin = atoi(sz0);
    else
      iIDOrigin = -1;

    sprintf(pLocBuffer, "%d %d\n",
            iOrigin, iIDOrigin);

    CDebug::Log(DEBUG_MAJOR_INFO, "DelOrgToRing logo:%d.%d.%d msg:%s\n",
                logo.type, logo.mod, logo.instid, pLocBuffer);
    
    pRingOut->Put(&logo, pLocBuffer);

	  return true;
  }  /* end if "DeletePublishedOrigin" */



  /****************************
   *      PublishOrigin       *
   ****************************/
	if(msg->Is("PublishOrigin")) 
  {
    if(!pRingOut)
			return true;
		logo.type = cTypeLocGlobal;
		logo.mod = cMod;
		logo.instid = cInst;
    idOrigin = msg->getStr("idOrigin");

    if((pOrg = pGlint->getOrigin(idOrigin))==NULL)
    {
    		CDebug::Log(DEBUG_MINOR_ERROR,"Action():PublishOrigin: Could not obtain "
          "Glint origin info for id(%s)\n", idOrigin);
        return(true);
    }

    char * sz0 = strchr(idOrigin, '0');
    int iIDOrigin;
    if(sz0)
      iIDOrigin = atoi(sz0);
    else
      iIDOrigin = -1;

    memset(&Loc, 0, sizeof(Loc));
    InitGlobalLoc(&Loc);

    Loc.logo = logo;
    Loc.event_id = iIDOrigin;
    Loc.origin_id = msg->getInt("version");
    Loc.tOrigin = pOrg->dT;
    Loc.lat     = pOrg->dLat;
    Loc.lon     = pOrg->dLon;
    Loc.depth   = pOrg->dZ;
    Loc.gap     = pOrg->dGap;
    Loc.dmin    = pOrg->dDelMin;
    Loc.rms     = pOrg->dRms;
    Loc.pick_count = pOrg->nEq;

    size_t iPickRef=0;
    GLOBAL_PHSLINE_STRUCT * pPhsOut;
    while(pPick = pGlint->getPicksFromOrigin(pOrg,&iPickRef)) 
    {
      if(!pPick->bTTTIsValid)
        continue;
      pPhsOut = &Loc.phases[Loc.nphs];
      InitGlobalPhaseLine(pPhsOut);

      // parse the sTag phase-value for logo/sequence info 
      strncpy(sTag, pPick->sTag, sizeof(sTag));
      sTag[sizeof(sTag)-1] = 0x00;
      memset(ktag,0,sizeof(ktag));
      sTmpTag = strtok(sTag,".\n");
      for(i=0; i<4; i++)
      {
        if(!sTmpTag)
          break;
        
        ktag[i]=atoi(sTmpTag);
        
        sTmpTag = strtok(NULL,".\n");
      }
      // ktag  1=Logo.Source 2=Logo.inst 3=Logo.Sequence


      pPhsOut->sequence    = ktag[3];
      pPhsOut->logo.type   = ktag[0];
      pPhsOut->logo.mod    = ktag[1];
      pPhsOut->logo.instid = ktag[2];
      strncpy(pPhsOut->station, pPick->sSite, sizeof(pPhsOut->station)-1);
      strncpy(pPhsOut->channel, pPick->sComp, sizeof(pPhsOut->channel)-1);
      strncpy(pPhsOut->network, pPick->sNet, sizeof(pPhsOut->network)-1);
      strncpy(pPhsOut->location, pPick->sLoc, sizeof(pPhsOut->location)-1);
      pPhsOut->tPhase = pPick->dT;
      strncpy(pPhsOut->phase_name, pPick->sPhase, sizeof(pPhsOut->phase_name)-1);
      pPhsOut->quality = pPick->dPickQual;

      Loc.nphs++;
    }  /* end for each phase in Glint for this origin*/

    WriteLocToBuffer(&Loc, pLocBuffer, GLOBAL_LOC_MAXBUFSIZE); 

    CDebug::Log(DEBUG_MAJOR_INFO, "OrgToRing logo:%d.%d.%d Event:%d:%d %.0f-(%.2f/%.2f/%.0f) rms:%.2f  picks: %3d\n",
                logo.type, logo.mod, logo.instid, 
                Loc.event_id, Loc.origin_id, Loc.tOrigin, Loc.lat, Loc.lon, Loc.depth,
                Loc.rms, Loc.pick_count);
    CDebug::Log(DEBUG_MINOR_INFO, "OrgToRing logo:%d.%d.%d msg:%s\n",
                logo.type, logo.mod, logo.instid, pLocBuffer);
    
    pRingOut->Put(&logo, pLocBuffer);

	  return true;
  }  /* end if "PublishOrigin" */
	
  /****************************
   *      AssLink             *
   ****************************/
	if(msg->Is("AssLink")) {
		if(!pRingOut)
			return true;
		
		szIDPick = msg->getStr("idPick");
		if(!szIDPick)
		{
			CDebug::Log(DEBUG_MINOR_ERROR,"Action():AssLink: Could not "
				"obtain idPick from message.\n");
			return true;
		}
		
		pPick = pGlint->getPickFromidPick(szIDPick);
		if(!pPick)
		{
			CDebug::Log(DEBUG_MINOR_ERROR,"Action():AssLink: Could not obtain "
				"pick for id(%s)\n", szIDPick);
			return(true);
		}
		
		logo.type = cTypeLink;
		logo.mod = cMod;
		logo.instid = cInst;
		
		strncpy(sTag, pPick->sTag, sizeof(sTag));
		sTag[sizeof(sTag)-1] = 0x00;
		memset(ktag,0,sizeof(ktag));
		sTmpTag = strtok(sTag,".\n");
    for(i=0; i<4; i++)
    {
      if(!sTmpTag)
        break;
      
      ktag[i]=atoi(sTmpTag);
      
      sTmpTag = strtok(NULL,".\n");
    }
    if((pOrg = pGlint->getOriginForPick(pPick))==NULL)
    {
		CDebug::Log(DEBUG_MINOR_ERROR,"Action():AssLink: Could not obtain "
			"origin for pick id(%s)\n", szIDPick);
        return(true);
    }
    iorg = atoi(&(pOrg->idOrigin[8]));

    _snprintf(mess, sizeof(mess) - 1,
              "%u %u %d %d %s %.2f %s %s %s %s %.2f %.2f %.2f %.2f %.4f %.4f %.2f %s \n",
              ktag[2], ktag[1], ktag[3], iorg, pPick->sPhase, pPick->dT, 
              pPick->sSite,pPick->sComp,pPick->sNet,pPick->sLoc,
              (pPick->dT - pOrg->dT) - pPick->dTrav, //res
              pPick->dToa, pPick->dAff, pPick->dAzm,
              pPick->dLat, pPick->dLon, pPick->dElev, pPick->idPick);      
    mess[sizeof(mess)-1] = 0x00;
    
    
		CDebug::Log(DEBUG_MAJOR_INFO, "LnkToRing logo:%d.%d.%d msg:%s",
			logo.type, logo.mod, logo.instid, mess);
		pRingOut->Put(&logo, mess);
		return true;
	}

  /****************************
   *      AssUnLink           *
   ****************************/
	if(msg->Is("AssUnLink")) {
		if(!pRingOut)
			return true;
		
		szIDPick = msg->getStr("idPick");
		if(!szIDPick)
		{
			CDebug::Log(DEBUG_MINOR_ERROR,"Action():AssUnLink: Could not "
				"obtain idPick from message.\n");
			return true;
		}
		
		pPick = pGlint->getPickFromidPick(szIDPick);
		if(!pPick)
		{
			CDebug::Log(DEBUG_MINOR_ERROR,"Action():AssUnLink: Could not obtain "
				"pick for id(%s)\n", szIDPick);
			return(true);
		}
		
		logo.type = cTypeUnLink;
		logo.mod = cMod;
		logo.instid = cInst;
		
		strncpy(sTag, pPick->sTag, sizeof(sTag));
		sTag[sizeof(sTag)-1] = 0x00;
		memset(ktag,0,sizeof(ktag));
		sTmpTag = strtok(sTag,".\n");
    for(i=0; i<4; i++)
    {
      if(!sTmpTag)
        break;
      
      ktag[i]=atoi(sTmpTag);
      
      sTmpTag = strtok(NULL,".\n");
    }

		szIDOrigin = msg->getStr("idOrigin");
		if(!szIDOrigin)
		{
			CDebug::Log(DEBUG_MINOR_ERROR,"Action():AssUnLink: Could not "
				"obtain idOrigin from message.\n");
			return true;
		}

    iorg = atoi(&(szIDOrigin[8]));

    _snprintf(mess, sizeof(mess) - 1,
              "%03u %03u %d %d %s %.2f %s %s %s %s %.4f %.4f %.2f %s \n",
              ktag[2], ktag[1], ktag[3], iorg, pPick->sPhase, pPick->dT, 
              pPick->sSite,pPick->sComp,pPick->sNet,pPick->sLoc,
              pPick->dLat, pPick->dLon, pPick->dElev, pPick->idPick);      
    mess[sizeof(mess)-1] = 0x00;
    
    
		CDebug::Log(DEBUG_MAJOR_INFO, "UnLnkToRing logo:%d.%d.%d msg:%s",
			logo.type, logo.mod, logo.instid, mess);
		pRingOut->Put(&logo, mess);
		return true;
	}
  
  
  /****************************
   *      GlobalPick          *
   ****************************/
  if(msg->Is("GlobalPick")) {
		str = msg->getStr("Msg");
		if(!str)
			return true;
		strcpy(mess, str);
		Decode(cTypePickGlobal, 0, 0, mess);
		return true;
	}

  /****************************
   *      ModuleId            *
   ****************************/
	if(msg->Is("ModuleId")) {
		CDebug::Log(DEBUG_MAJOR_INFO, "ModuleId...\n");
		str = msg->getStr("Module");
		if(!str)
			return true;
		CDebug::Log(DEBUG_MAJOR_INFO, "  Module <%s>\n", str);
		cMod = GetModId(str);
		CDebug::Log(DEBUG_MAJOR_INFO, "  cMod=%d\n", cMod);
		return true;
	}

  /****************************
   *      HeartBeat           *
   ****************************/
	if(msg->Is("HeartBeat")) {
		CDebug::Log(DEBUG_MAJOR_INFO, "HeartBeat\n");
		iPulse = msg->getInt("Interval");
		CDebug::Log(DEBUG_MAJOR_INFO, "  iPulse=%d\n", iPulse);
		return true;
	}

  /****************************
   *      LogFile             *
   ****************************/
	if(msg->Is("LogFile")) {
		CDebug::Log(DEBUG_MAJOR_INFO, "LogFile\n");
		bLogFile = true;
		return true;
	}

  /****************************
   *      MsgSize             *
   ****************************/
	if(msg->Is("MsgSize")) {
		CDebug::Log(DEBUG_MAJOR_INFO, "MsgSize\n");
		return true;
	}

  /****************************
   *      MaxMessage          *
   ****************************/
	if(msg->Is("MaxMessage")) {
		CDebug::Log(DEBUG_MAJOR_INFO, "MaxMessage\n");
		return true;
	}

  /****************************
   *      ShutDown            *
   ****************************/
	if(msg->Is("ShutDown")) {
		return true;
	}

  /****************************
   *      RingIn              *
   ****************************/
	if(msg->Is("RingIn")) {
		str = msg->getStr("Name");
		if(str) {
			CDebug::Log(DEBUG_MAJOR_INFO, "RingIn Name:%s\n", str);
			key = GetKey(str);
		} else {
			key = 1005;
		}
    if(key == -1)
    {
      CDebug::Log(DEBUG_MAJOR_ERROR,"RingIn Failed to obtain Ring SharedMemory "
                                    "Key (i.e. 1005) for RingIn(%s)\n",
                  str);
      exit(-1);
    }
		pRingIn = new CRing();
		if(!pRingIn) {
			CDebug::Log(DEBUG_MAJOR_ERROR, " ** RingIn Cannot create new ring\n");
      exit(-1);
			return true;
		}
		pRingIn->Logo(0, 0, 0);
		if(!pRingIn->Attach(key)) {
			CDebug::Log(DEBUG_MAJOR_ERROR, " ** RingIn Cannot attach to PICK_RING\n");
      exit(-1);
			return true;
		}
//		Poll(0);
		return true;
	}

  /****************************
   *      RingOut             *
   ****************************/
	if(msg->Is("RingOut")) {
		str = msg->getStr("Name");
		if(str) {
			key = GetKey(str);
		} else {
			key = 1005;
		}
    if(key == -1)
    {
      CDebug::Log(DEBUG_MAJOR_ERROR,"RingOut Failed to obtain Ring SharedMemory "
                                    "Key (i.e. 1005) for RingOut(%s)\n",
                  str);
      exit(-1);
    }
		pRingOut = new CRing();
		if(!pRingOut) {
			CDebug::Log(DEBUG_MAJOR_ERROR, " ** RingOut Cannot create new ring\n");
      exit(-1);
			return true;
		}
		if(!pRingOut->Attach(key)) {
			CDebug::Log(DEBUG_MAJOR_ERROR, " ** RingOut Cannot attach to RING(%d)\n",key);
      exit(-1);
			return true;
		}
//		Poll(0);
		return true;
	}

  /****************************
   *      LOG_PICKS           *
   ****************************/
	if(msg->Is("LOG_PICKS")) 
  {
    SetPickLogging(true);
    return true;
  }

  /****************************
   *      EarthwormDebugLevel *
   ****************************/
	if(msg->Is("EarthwormDebugLevel")) 
  {
    DebugOutputStruct dosDebug;
    int iLevel;
    memset(&dosDebug, 0, sizeof(dosDebug));
    
    iLevel = msg->getInt("Level");
    if(iLevel < DEBUG_MAX_LEVEL || iLevel > DEBUG_MIN_LEVEL)
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "Error parsing config file line: %s %d...\n", 
                  "EarthwormDebugLevel", iLevel);
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

static double tThreshold = 1.0;
//---------------------------------------------------------------------------------------Poll
//void CMod::Poll(long time) {
bool CMod::Poll() {
	IMessage *m;
	MSG_LOGO logo; 
	char msg[MAXMSG];
	int res;
	int i;
	int j;
	int n;
	char c;
	double t;
	long theart;

  double tStart;
  double tNow;
	iMsgsProcessed = 0;

	j = 0;
	if(!pRingIn)
  {
   	CDebug::Log(DEBUG_MAJOR_ERROR, "Poll(): ERROR! pRingIn is NULL! Cannot get EW input data.\n");
    exit(-1);  // should this be there.
		return(true);
  }
	if(bFlush) {
		while(res = pRingIn->Get()) {
			if(res == TERMINATE) {
				m = CreateMessage("TERMINATE");
				Broadcast(m);
				m->Release();
				break;
			}
		}
		bFlush = false;
	}
  hrtime_ew(&tStart);
	while(res = pRingIn->Get()) {
		switch(res) {
		case GET_NOTRACK:
      break;
		case GET_MISS_LAPPED:
 		  CDebug::Log(DEBUG_MINOR_ERROR, "ERROR:  Missed messages in EW Ring.  Ring Lapped! Falling Behind!\n");
		case GET_MISS_SEQGAP:
      if(res == GET_MISS_SEQGAP)
      {
        if(pRingIn->cSeq == 0)
   		    CDebug::Log(DEBUG_MAJOR_WARNING, "WARNING:  Module %d / Inst %d appears to have restarted. Seq Gap! \n");
        else
   		    CDebug::Log(DEBUG_MINOR_ERROR, "WARNING:  Missed message from Module %d / Inst %d.  Seq(%d) Gap!\n", pRingIn->cSeq);
      }
		case GET_OK:
			n = pRingIn->nMsg;
			pRingIn->cMsg[n] = 0;
			for(i=0; i<=n; i++) {
				c = pRingIn->cMsg[i];
				if(c == '\n' || c == '\r')
					c = 0;
				msg[i] = c;
			}
 		  CDebug::Log(DEBUG_MINOR_INFO, "msg <%s>\n", msg);
			Decode(pRingIn->inLogo.type, pRingIn->inLogo.mod, pRingIn->inLogo.instid, msg);
			break;
		case TERMINATE:
			m = CreateMessage("ShutDown");
			Broadcast(m);
			m->Release();
			PostQuitMessage(0);
			break;
    default:
 		  CDebug::Log(DEBUG_MINOR_ERROR, "ERROR:  Unexpected return code(%d) from pRingIn->Get()!\n", res);

		}
    //  Don't let processing go (much) beyond tThreshold seconds
    //  DK 061903
		if(hrtime_ew(&tNow) - tStart > tThreshold)
			break;
	}
	if(!iMsgsProcessed)
      Sleep(200);  // DK 103003 moved sleep from main() to here, per Carl

	if(!pRingOut)
		return(true);
	t = Secs();
	if(t - tPulse > iPulse) {
		tPulse = t;
		theart = CRing::Time();
		sprintf(msg, "%ld %ld\n\0", theart, iPid);
		logo.type = cTypeHeartBeat;
		logo.mod = cMod;
		logo.instid = cInst;
		pRingOut->Put(&logo, msg);
	}
  return(true);

}

//---------------------------------------------------------------------------------------Decode
void CMod::Decode(int type, int mod, int src, char *msg) {
	IMessage *m = 0x00;
	CStr cmsg = msg;
	char logo[64];
	int yr, mo, da, hh, mm;
	double ss;
	CDate date;
	char sta[16];
	char net[16];
	char comp[16];
	char loc[16];
	char phs[16];
	int seq;
	char *s;
	CStr str;
	int n;

	str.Cat(type);
	str.Cat(".");
	str.Cat(mod);
	str.Cat(".");
	str.Cat(src);
	strcpy(logo, str.GetBuffer());

//	switch(type) {
//	case cTypeHeartBeat:
	if(type == cTypeHeartBeat) {
		m = CreateMessage("HEARTBEAT");
		m->setStr("Logo", logo);
		m->setPtr("Msg", msg);
	} else
	if(type == cTypePick2K) {
    LogPick(msg);
//		m = CreateMessage("PICK2K");
//		m->setStr("Logo", logo);
//		m->setPtr("Msg", msg);
		seq = cmsg.Long(5, 9);
		s = cmsg.String();
		if(s &&  (strlen(s) > 0) && (s[0] != SCNL_UNKNOWN_CHAR) && (s[0]!= '?'))
			strcpy(sta, s);
		else
			strcpy(sta, SCNL_UNKNOWN_STR);
		s = cmsg.String(3, 22);
		if(s &&  (strlen(s) > 0) && (s[0] != SCNL_UNKNOWN_CHAR) && (s[0]!= '?'))
			strcpy(comp, s);
		else
			strcpy(comp, SCNL_UNKNOWN_STR);
		s = cmsg.String(2, 28);
		if(s &&  (strlen(s) > 0) && (s[0] != SCNL_UNKNOWN_CHAR) && (s[0]!= '?'))
			strcpy(net, s);
		else
			strcpy(net, SCNL_UNKNOWN_STR);
    // DK CLEANUP - add Loc Support  - Note this is dependent upon the EW Pick2K format
		//s = cmsg.String(2, 28);
		//if(s && strlen(s) > 0)
		//	strcpy(net, s);
		//else
			strcpy(loc, SCNL_UNKNOWN_STR);
		yr = cmsg.Long(4, 30);
		mo = cmsg.Long(2);
		da = cmsg.Long(2);
		hh = cmsg.Long(2);
		mm = cmsg.Long(2);
		ss = cmsg.Double(5);
		date = CDate(yr, mo, da, hh, mm, ss);
		m = CreateMessage("PICK2K");
		m->setStr("Logo", logo);
		m->setPtr("Msg", msg);
		s = cmsg.String();
		if(s)
			strcpy(phs, s);
		else
			strcpy(phs, PHASE_UNKNOWN_STR);
		m->setInt("Seq", seq);
		if(sta[0] != SCNL_UNKNOWN_CHAR)
			m->setStr("S", sta);
		if(comp[0] != SCNL_UNKNOWN_CHAR)
			m->setStr("C", comp);
		if(net[0] != SCNL_UNKNOWN_CHAR)
			m->setStr("N", net);
		if(loc[0] != SCNL_UNKNOWN_CHAR)
			m->setStr("L", loc);
		m->setStr("Phs", phs);
		m->setDbl("T", date.Time());
	} else
	if(type == cTypePickGlobal) {
    LogPick(msg);
		m = CreateMessage("PICK2K");
		if(!m)
		{
      CDebug::Log(DEBUG_MAJOR_ERROR, "Could not Create PICK2K internal Message\n");
      exit(-1);
		}
		m->setStr("Logo", logo);
		m->setPtr("Msg", msg);
		cmsg.String();			// Author
		seq = cmsg.Long();		// Sequence number
		cmsg.String();			// Version
		s = cmsg.String();		// Station
		if(s &&  (strlen(s) > 0) && (s[0] != SCNL_UNKNOWN_CHAR) && (s[0]!= '?'))
		  strcpy(sta, s);
    else
		  strcpy(sta, SCNL_UNKNOWN_STR);
		s = cmsg.String();		// Comp
		if(s &&  (strlen(s) > 0) && (s[0] != SCNL_UNKNOWN_CHAR) && (s[0]!= '?'))
		  strcpy(comp, s);
    else
		  strcpy(comp, SCNL_UNKNOWN_STR);
		s = cmsg.String();		// Net
		if(s &&  (strlen(s) > 0) && (s[0] != SCNL_UNKNOWN_CHAR) && (s[0]!= '?'))
		  strcpy(net, s);
    else
		  strcpy(net, SCNL_UNKNOWN_STR);
		s = cmsg.String();		// Location
		if(s &&  (strlen(s) > 0) && (s[0] != SCNL_UNKNOWN_CHAR) && (s[0]!= '?'))
		  strcpy(loc, s);
    else
		  strcpy(loc, SCNL_UNKNOWN_STR);
		str = cmsg.String();	// yyyymmddhhmmss.sss
		n = str.GetLength() - 12;

    /* ensure that the length is valid */
    if(n < 0)
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "Bad pick message(%s), logo(%s) could not parse!  Ignoring!\n",
                  msg, logo);
      return;
    }
		yr = str.Long(4);
		mo = str.Long(2);
		da = str.Long(2);
		hh = str.Long(2);
		mm = str.Long(2);
		ss = str.Double(n);
		date = CDate(yr, mo, da, hh, mm, ss);
		s = cmsg.String();
		strcpy(phs, s);
		m->setInt("Seq", seq);
		if(sta[0] != SCNL_UNKNOWN_CHAR)
			m->setStr("S", sta);
		if(comp[0] != SCNL_UNKNOWN_CHAR)
			m->setStr("C", comp);
		if(net[0] != SCNL_UNKNOWN_CHAR)
			m->setStr("N", net);
		if(loc[0] != SCNL_UNKNOWN_CHAR)
			m->setStr("L", loc);
		m->setStr("Phs", phs);
		m->setDbl("T", date.Time());
	} 
  else 
  {
    // ignore message

    // DK 072803  Currently, there is no need to handle anything other
    //  than pick and possibly quake messages.  No point to propogating
    //  all the other messages from the ring through Nexus.  It just adds
    //  unneccessary processing, and provides further opportunities for 
    //  problems to arise.  
    //  This behavior can be changed at a later time if needed.
    /*
	   *  m = CreateMessage("UnknownType");
	   *  m->setStr("Logo", logo);
	   *  m->setStr("Msg", msg);
     *******************************************/
		return;
	}

	Broadcast(m);
	m->Release();
}

int CMod::GetKey(char *ring) {
	return GetPar("Ring", ring);
}

int CMod::GetLocalInst() {
	char sin[128];
	char *inst = getenv("EW_INSTALLATION");
	if(!inst)
		return -1;
	strcpy(sin, inst);
	return GetPar("Installation", sin);
}

int CMod::GetType(char *msgtyp) {
	return GetPar("Message", msgtyp);
}

int CMod::GetModId(char *modnam) {
	return GetPar("Module", modnam);
}

int CMod::GetPar(char *group, char *name) {
	char root[256];
	char path[256];
	char *str;
	int n;
	int ifile;
	CComFile cf;
	CStr tok;
	int ival;
	int nc;

	str = getenv("EW_PARAMS");
	if(str) {
		strcpy(root, str);
		n = strlen(root);
		if(root[n-1] != '\\' && root[n-1] != '/')
			strcat(root, "\\");
	} else {
		root[0] = 0;
	}

	for(ifile=0; ifile<2; ifile++) {
		strcpy(path, root);
		switch(ifile) {
		case 0:
			strcat(path, "earthworm.d");
			break;
		case 1:
			strcat(path, "earthworm_global.d");
			break;
		}
		if(!cf.Open(path)) {
			CDebug::Log(DEBUG_MAJOR_ERROR, "Parameter file <%s> not found\n", path);
      exit(-1);
			return -1;
		}
		while(true) {
			nc = cf.Read();
			if(nc < 0) {
				cf.Close();
				break;
			}
			if(nc < 2)
				continue;
			tok = cf.Token();
			if(cf.Is("#"))
				continue;
			if(cf.Is(group)) {
				tok = cf.Token();
				if(cf.Is(name)) {
					ival = cf.Long();
					cf.Close();
					return ival;
				}
			}
		}
		cf.Close();
	}
	
	return -1;
}


/**************************************************
    CMod::SetPickLogging(int bLog) 


     params:
      bLog - flag indicating whether or not Picks should
             be logged to disk.

     return value:
         1 - disk logging enabled
         0 - disk logging disabled
 ***************************************************/
int CMod::SetPickLogging(int bLog) 
{
  FILE * f;

  this->bLogPicks = false;

  if(bLog)
  {
    char * szEW_LOG = getenv("EW_LOG");
    if(!szEW_LOG)
      szEW_LOG = "";
    _snprintf(szPickLogfileName, sizeof(szPickLogfileName), 
              "%s%s", szEW_LOG, szPICK_LOGFILE_BASE_NAME);
    szPickLogfileName[sizeof(szPickLogfileName)-1] = 0x00;

    if(f = fopen(szPickLogfileName, "a"))
    {
      time_t tNow;
      time(&tNow);
      fprintf(f,"\n\n"
                "########################################\n"
                "### STARTING AT %s"
                "########################################\n\n",
              ctime(&tNow));
      fclose(f);
      f = NULL;
      this->bLogPicks = true;
    }
    else
    {
      CDebug::Log(DEBUG_MINOR_ERROR, 
                  "CScroll:SetPickLogging(): ERROR: Unable to open Picks log file (%s).\n",
                  szPickLogfileName);
    }
  }
  return(this->bLogPicks);
}


int  CMod::LogPick(char * txt)
{
  int rc = false;
  FILE * f;
 
  iMsgsProcessed++;  // Added to count the number of picks entering the system 
                     // during a processing cycle. DK 103103

  if(bLogPicks)
  {
    if(f = fopen(szPickLogfileName, "a"))
    {
      fprintf(f, "%s\n", txt);
      fclose(f);
      f = NULL;
      rc = true;
    }
    else
    {
      CDebug::Log(DEBUG_MINOR_ERROR,"CMod::Decode(): ERROR!  Could not open pick log file (%s)\n",
                  szPickLogfileName);
    }
  }

  return(rc);
}  // end CMod::LogPick()



// Function for issuing status messages via an earthworm ring (statmgr)
/* We offer this function up to GlobalDebug, so that GlobalDebug()
   can call it to issue status messages via earthworm
 **********************************************************************/
static int IssueStatus(int iType, const char * szOutput)
{
  MSG_LOGO logo;

  // NOTE: This function makes use of a static variable
  //       pLocalMod, which is set in the (Earthworm)Mod
  //       constructor.  
  //       It is a hack to get at the Mod functions, without
  //       having to declare an interface that GlobalDebug and
  //       (Earthworm)Mod know about, and then pAing GlobalDebug
  //       a pointer to the (Earthworm)Mod class.
  //       This way we just pass a function pointer(C-style) to 
  //       GlobalDebug, and I don't have to spend the time defining
  //       an interface.  (Please have mercy on my soul.)
  //       DK 20030623
	logo.type = pLocalMod->cTypeError;
	logo.mod = pLocalMod->cMod;
	logo.instid = pLocalMod->cInst;
  if(!pLocalMod->pRingOut)
  {
    GlobalDebug::DebugVA("et", "ERROR! EarthwormMod: IssueStatus()  Output EW Ring is Null! "
                                "Could not write status to ring:\n\t\t(%d:%s)", 
				                 iType, szOutput );
    return(CRING_NOT_ATTACHED);
  }
  return(pLocalMod->pRingOut->PutStatus(&logo, pLocalMod->cTypeError, iType, szOutput));
}  // end IssueStatus()


/****************************************************************
* int IssueStatusMessage(int iType, const char * szOutput)
* {
*  if(pLocalMod)
*  {
*    IMessage * m;
*    m = pLocalMod->CreateMessage("IssueStatus");
*    m->setInt("Type",iType);
*    m->setStr("Msg", szOutput);
*    pLocalMod->Dispatch(m);
*    m->Release();
*  }
*  return(0);
* }
****************************************************************/

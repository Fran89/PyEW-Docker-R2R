/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: glass.cpp 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/12/27 18:05:23  paulf
 *     sync from hydra_proj circa Oct 2007
 *
 *     Revision 1.6  2006/09/11 15:40:52  davidk
 *     Added/Fixed debugging for cases where the nucleator generates a location
 *     that doesn't generate nCut number of phase associations.
 *
 *     Revision 1.5  2006/09/05 15:20:30  davidk
 *     added logging changes to log problematic nucleator results.
 *     (may include a bug that causes excessive logging).
 *
 *     Revision 1.4  2006/08/26 06:16:18  davidk
 *     modified code to track the number of picks associted during the initial post-nucleation
 *     association phase.  If the number is below the minimum expected, the a log
 *     message is issued.
 *     This was done because Glass *APPEARS* to be producing and then
 *     squashing a bunch of bogus events, that should never have been produced
 *     in the first place.
 *
 *     Revision 1.3  2006/05/04 23:26:01  davidk
 *     Changed abest from some really big number, to something smaller but larger
 *     than the cutoff.  Ideally it would be set to the cutoff or just over the cutoff
 *     because there's no point (other than seeing if things are working) to setting
 *     the value bigger than the cutoff, because values bigger than the cutoff will
 *     never be used.
 *
 *     Revision 1.2  2005/10/17 06:29:46  davidk
 *     Added code to stop the focus mechanism from fighting with the locator.
 *     The focus now attempts at most one refocus per change in origin-pick
 *     association
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.14  2005/04/18 20:55:42  davidk
 *     Part of the integration of glass algorithms into hydra.
 *     Extracted the nucleating code from the auxilliary Associate()
 *     function (and this source file) into a hydra library function AssociatePicks().
 *
 *     Revision 1.13  2005/02/28 18:14:37  davidk
 *      Added code to filter out spurious associated phases at time of publication.
 *     Added function PrunePick() so the publisher can communicate to the associator
 *     that a particular pick should be pruned from an origin.
 *     Added a MarkOriginAsScavenged flag to UnassociatePickWOrigin()
 *     in order to control the amount of origin rehashing that gets done.  Now
 *     the origin no longer gets scheduled for rehash when pruned by the publisher.
 *
 *     Revision 1.12  2005/02/15 19:33:28  davidk
 *     Added additional debugging information.
 *
 *     Revision 1.11  2004/12/02 20:57:03  davidk
 *     Moved temporary hack validation code out of ValidateOrigin() and into
 *     an external  AlgValidateOrigin() function, in order to separate core glass
 *     code from network specific corrections/filtering.
 *
 *     Revision 1.10  2004/12/01 06:16:52  davidk
 *     refining temporary filtering criteria in ValidateOrigin()
 *
 *     Revision 1.9  2004/12/01 02:35:31  davidk
 *     Added temporary validation constraints to ValidateOrigin() function,
 *     that places events in NorthAmerica and the Western Hemisphere,
 *     under more stringent quality constraints.
 *
 *     Revision 1.8  2004/11/23 00:50:49  davidk
 *     Fixed bug in Focus() where code was not checking the return code from the
 *     focus message, and was always reporting that the origin was refocussed,
 *     causing further processing to occur.
 *     Code now reports the appropriate return code from the focus message, so
 *     that further processing only occurs if the message indicated that refocussing
 *     occurred.
 *
 *     Revision 1.7  2004/11/02 19:11:32  davidk
 *     Modified UnPutOrigin() to BROADCAST "OriginDel" messages, so that
 *     they can be received by multiple modules, instead of DISPATCHing them,
 *     where they are only picked up by one module.
 *
 *     Revision 1.6  2004/11/01 18:01:18  davidk
 *     Changed the number of picks required to go through association from 4 to a
 *     number derived from the nCut parameter.
 *     A weenie attempt to improve performance.
 *
 *     Revision 1.5  2004/11/01 06:31:31  davidk
 *     Modified to work with new hydra traveltime library.
 *
 *     Revision 1.4  2004/09/16 00:53:25  davidk
 *     Added glass_assoc.d config param  NumLocatorIterations.
 *     (param gets converted to glass internal msg, and sent to locator).
 *     Moved all old code to the bottom of the file.
 *
 *     Revision 1.3  2004/08/06 00:34:16  davidk
 *     Modified PutOrigin() function so that it no longer outputs "AssOrigin"
 *     messages each time a Pick is associated with an origin, but instead puts
 *     out "OriginAdd" and "OriginMod" messages when a new origin is created
 *     or updated respectively.
 *
 *     Added UnPutOrigin() function that creates an "OriginDel" message.
 *
 *     Modified PutLink() and PutUnLink() functions  to create "OriginLink"
 *     and "OriginUnLink" intra-glass messages, instead of "AssLink" and
 *     "AssUnLink" messages.  (Used to sound a lot like a conga-line).
 *
 *     Added call to UnPutOrigin() in DeleteOrigin().  UnPutOrigin() is
 *     called after the picks are stripped from an Origin, but prior
 *     to the actual deletion of the origin from Glint.
 *
 *     Changed the format of the Pick string that is printed to the
 *     GUI status tool.
 *
 *     Changed the handling of the return code from the Glock locator.
 *     Now the number of location-calculations is only increment when
 *     Glock actually RELOCATES the origin.
 *
 *     Added code to increment the Quake counter when a new origin
 *     is created.
 *
 *     Revision 1.2  2004/04/01 22:05:43  davidk
 *     v1.11 update
 *     Added internal state-based processing.
 *
 *     Revision 1.3  2003/11/07 22:27:54  davidk
 *     Added RCS header.
 *     Removed algorithmic changes (such as periodic event processing)
 *     that had been added as part of v1.0, but were deemed undesirable.
 *     Improved the way that glass updates the "Monitor" during times
 *     of inactivity.
 *
 *
 **********************************************************/

// glass.cpp

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <ITravelTime.h>
#include "glass.h"
#include "spock.h"
#include "monitor.h"
#include "GlassMod.h"
#include "IGlint.h"
#include "date.h"
#include "str.h"
#include <Debug.h>
#include <opcalc.h>
#include <AssociatePicks.hpp>

extern "C" {
#include "utility.h"
}

static int nPck;
static int nXYZ;
static PICK *Pck[MAXPCK];
static double dXYZ[6*MAXPCK];

static double dTest;		// Temporary for testing

//---------------------------------------------------------------------------------------CGlass
CGlass::CGlass() {
	nAss = 0;
	pMod = 0;
	pMon = 0;
	pTT = 0;
	dTest = 0.0;
	nCut = 5;
	dCut = 50.0;	// Cluster parameter (km)
	strcpy(sPick, "");
	strcpy(sDisp, "");
	nPick = 0;
	nAssoc = 0;
	nQuake = 0;
	iLapse = 0;
	tDone = Secs();
//	dXYZ = new double[6*MAXPCK];
	bAssociate = true;
	bLocate = true;

	// Adjustable tuning parameters
	nCut = 5;					// Number of partial locations in starting cluster
	dCut = 50.0;					// Cluster distance threshold (km)
	dTimeBefore = -120.0;		// Time previous to pick for nucleation consideration
	dTimeAfter = 30.0;			// Time after pick for nucleaation consideration
	dTimeOrigin = -300.0;		// Time before pick containing trail origins
	dTimeStep = 10.0;			// Nucleation time mesh (seconds)
	nShell = 0;					// Number of nucleation shells
	dShell[nShell++] = 0.5;		// Nucleation shell depths (km)
	dShell[nShell++] = 50.0;
	dShell[nShell++] = 100.0;
	dShell[nShell++] = 200.0;
	dShell[nShell++] = 400.0;
	dShell[nShell++] = 800.0;
	memset(Mon,0,sizeof(Mon));  // DK CHANGE 060303  initializing Mon
  // DK 1.0 PULL iReportInterval = ORIGIN_EXTPROC_INTERVAL;

  InitGlassStateData();
}  // end CGlass()

//---------------------------------------------------------------------------------------~CGlass()
CGlass::~CGlass() {
}

//---------------------------------------------------------------------------------------Params
// Read parameter file
bool CGlass::Params(char *file) {
	CComFile cf;
	CStr cmd;
	int nc;
	bool b = true;
// DK REMOVE	double prob2;

  //DebugBreak();

	if(!cf.Open(file))
  {
    CDebug::Log(DEBUG_MAJOR_ERROR,"CGlass::Params():  Unable to open config file(%s).\n",
                file);
    exit(-1);
		return false;
  }
	while(true) {
		nc = cf.Read();
		if(nc < 0) {
			break;
		}
		if(nc < 1)
			continue;
    CDebug::Log(DEBUG_MAJOR_INFO, "Params() New command <%s>\n", cf.Card().GetBuffer());

		cmd = cf.Token();
		if(cmd.GetLength() < 1)
			continue;
		if(cf.Is("#"))
			continue;
		if(cf.Is("Cut")) {
			nCut = cf.Long();
			dCut = cf.Double();
			continue;
		}
		if(cf.Is("TimeRange")) {
			dTimeBefore = cf.Double();
			dTimeAfter = cf.Double();
			dTimeOrigin = cf.Double();
			continue;
		}
		if(cf.Is("TimeStep")) {
			dTimeStep = cf.Double();
			continue;
		}
		if(cf.Is("Shell")) {
			if(b) {
				b = false;
				nShell = 0;
			}
			dShell[nShell++] = cf.Double();
			continue;
		}
		if(cf.Is("Tran")) 
    {
      ConfigureTransitionRule(&cf);
      continue;
		}
		if(cf.Is("NumLocatorIterations")) 
    {
      IMessage *m;

      m = pMod->CreateMessage("NumLocatorIterations");
      m->setInt("Num",cf.Long());
      pMod->Dispatch(m);
      m->Release();
      continue;
		}
  }   // end while(true)
	cf.Close();
	return true;
}  // end Params()



//---------------------------------------------------------------------------------------PutOrigin
void CGlass::PutOrigin(char *idorg, bool bNew)
{
  IMessage *m;
  if(bNew)
  {
  	m = pMod->CreateMessage("OriginAdd");
	  m->setStr("idOrigin",idorg);
  }
  else
  {
  	m = pMod->CreateMessage("OriginMod");
	  m->setStr("idOrigin",idorg);
  }
	pMod->Dispatch(m);
	m->Release();
}

//---------------------------------------------------------------------------------------PutOrigin
void CGlass::UnPutOrigin(char *idorg)
{
	IMessage *m = pMod->CreateMessage("OriginDel");
	m->setStr("idOrigin",idorg);
	pMod->Broadcast(m);
	m->Release();
}

//---------------------------------------------------------------------------------------PutLink
void CGlass::PutLink(char *idorg, char *idpck) 
{
	IMessage *m = pMod->CreateMessage("OriginLink");
	m->setStr("idPick",idpck);
	m->setStr("idOrigin",idorg);
	pMod->Dispatch(m);
	m->Release();
}

//---------------------------------------------------------------------------------------PutLink
void CGlass::PutUnLink(char *idorg, char *idpck) 
{
	IMessage *m = pMod->CreateMessage("OriginUnLink");
	m->setStr("idPick",idpck);
	m->setStr("idOrigin",idorg);
	pMod->Dispatch(m);
	m->Release();
}

//---------------------------------------------------------------------------------------Poll
void CGlass::Poll() {
	if(!pMon)
		return;
	int lapse = (int)(Secs() - tDone);
	if(lapse != iLapse) {
		iLapse = lapse;
// DK PERF		pMon->Refresh();
		pMon->UpdateStatus();
	}		
}

int CGlass::DeleteOrigin(char * idOrigin)
{
  ORIGIN * pOrigin;
  PICK * pPick;

  if(!(pOrigin = pGlint->getOrigin(idOrigin)))
  {
    CDebug::Log(DEBUG_MAJOR_INFO, "Origin %s not found.  Ignoring delete request.\n", idOrigin);
    return(false);
  }

  // check origin parameters to ensure it is still viable
  if(pOrigin->nPh > 0)
  {
    // Remove Phases
    size_t iPickRef=0;
    while(pPick = pGlint->getPicksFromOrigin(pOrigin,&iPickRef))
    {
      UnAssociatePickWOrigin(pOrigin->idOrigin,pPick,false);
    }
  }

   CDebug::Log(DEBUG_MINOR_WARNING, "Deleting origin %s\n", idOrigin);
   UnPutOrigin(idOrigin);

   if(pGlint->deleteOrigin(idOrigin))
     return(true);
   else
     return(false);
}  // end DeleteOrigin()

bool CGlass::ValidateOrigin(char * idOrigin)
{
  ORIGIN * pOrigin;

  PhaseType  ptPhase;
  PhaseClass pcPhase;

  CDebug::Log(DEBUG_MINOR_INFO,"Origin %s: Attempting to Validate!\n",
              idOrigin);

  if(!(pOrigin = pGlint->getOrigin(idOrigin)))
  {
    CDebug::Log(DEBUG_MINOR_WARNING,"Origin %s: can't retrieve origin info!\n",
                idOrigin);
    return(false);
  }

  // check origin parameters to ensure it is still viable
  if(pOrigin->nPh < nCut || pOrigin->nEq < nCut)
  {
    CDebug::Log(DEBUG_MAJOR_INFO,"Origin %s/%d, too few picks(%d/%d).  Invalidating!\n",
                pOrigin->idOrigin, pOrigin->iOrigin, pOrigin->nPh,pOrigin->nEq);
    return(false);
  }

  // DK 020604  Check to make sure we still have the requisite
  //            number of Primary phases (Pup, P, PDif)
  size_t iPickRef=0;
  PICK * pck;
  int iPCtr = 0;
  while(pck = pGlint->getPicksFromOrigin(pOrigin, &iPickRef))
  {
    if(!pck->bTTTIsValid)
      continue;
    ptPhase = GetPhaseType(pck->ttt.szPhase);
    pcPhase = GetPhaseClass(ptPhase);
    if(pcPhase == PHASECLASS_P)
    {
      iPCtr++;
      if(iPCtr == nCut)
      {
        pGlint->endPickList(&iPickRef);
        break;
      }
    }
  }
  if(iPCtr < (nCut - 1))   // -1 is for leniency (sp?)
  {
    CDebug::Log(DEBUG_MAJOR_INFO,"Origin %s/%d, too few P picks(%d).  Invalidating!\n",
                pOrigin->idOrigin, pOrigin->iOrigin, iPCtr);
    return false;
  }
  // End DK 020604  Check for number of P Phases

  return(AlgValidateOrigin(pOrigin, pGlint, nCut));
}  // end ValidateOrigin()



//---------------------------------------------------------------------------------------Origin
// Load origin, assumes coordinates are geographic
void CGlass::Origin(double t, double lat, double lon, double depth) {
	ORIGIN org;
	CDate dt(t);
	CDebug::Log(DEBUG_MINOR_INFO, "Origin: %s %.4f %.4f %.2f\n", dt.Date20().GetBuffer(), lat, lon, depth);
	if(!pGlint)
		return;
	org.iVersion = GLINT_VERSION;
	org.dT = t;
	org.dLat = lat;
	org.dLon = lon;
	org.dZ = depth;
	org.nEq = 0;
	org.dRms = 0.0;
	org.dDelMin = 0.0;
	org.dDelMod = 0.5;
	org.dDelMax = 1.0;
	sprintf(org.sTag, "0.0.0.0");
	pGlint->putOrigin(&org);
	Grok(org.idOrigin);
	dTest = t;
}

//---------------------------------------------------------------------------------------Pick
void CGlass::Pick(char *sta, char *comp, char *net, char * loc, char *phase,
		double t, double lat, double lon, double elev,
		char *logo, int iseq)
{

  /*********************************************************
   * This is the entry point for triggered glass association.
   * This routine is called everytime a new "pick" enters
   * the system.
   *********************************************************/

	char scn[32];
	PICK pck;
	double tpick;
//	double t1;
//	double t2;
	double a;
	double b;
//	bool bres;

  // STEP 1: Record the pick's attributes, and save the pick in GLINT
	if(!pGlint)
		return;
  // DK CLEANUP - this is BAD!! nScheduledActions = 0;   // CARL 120303
	MON *mon = &Mon[0];
	MON *avg = &Mon[1];
	tpick = Secs();

  memset(&pck,0,sizeof(pck));
	pck.iVersion = GLINT_VERSION;
	pck.dT = t;
	pck.dLat = lat;
	pck.dLon = lon;
	pck.dElev = elev;
	pck.dAff = 0.0;
	sprintf(pck.sTag, "%s.%d", logo, iseq);
	strncpy(pck.sPhase, phase, sizeof(pck.sPhase)-1);
	strncpy(pck.sSite, sta, sizeof(pck.sSite)-1);
	strncpy(pck.sComp, comp, sizeof(pck.sComp)-1);
	strncpy(pck.sNet, net, sizeof(pck.sNet)-1);
	strncpy(pck.sLoc, loc, sizeof(pck.sLoc)-1);

	if(pMon) {
    _snprintf(scn, sizeof(scn), "%s.%s.%s.%s",sta,comp,net,loc);
    scn[sizeof(scn)-1] = 0x00;
    CDate dt(t);

    _snprintf(sPick, sizeof(sPick), "%8d: %-14s %-1s %02d:%02d:%05.2f %4d/%02d/%02d", 
              iseq, scn, phase, dt.Hour(), dt.Minute(), dt.Secnds(), 
              dt.Year(), dt.Month(), dt.Day());
    sPick[sizeof(sPick)-1] = 0x00;
	}
	if(!nPick) {
		tDone = tpick;
		CatOut();
	}
	mon->tIdle = tpick - tDone;
	mon->tAsn = 0.0;
	mon->tAss = 0.0;
	mon->tLoc = 0.0;
	pGlint->putPick(&pck);

  /**********   removed // CARL 120303
  // STEP 2: Attempt to associate the Pick with one of the existing
  //         quakes.
	t1 = Secs();
	bres = Assign(pck.idPick);
	t2 = Secs();
	mon->tAsn = t2 - t1 - mon->tLoc;

  // STEP 2.1: If we were able to associate the pick,
  //           we're done.  Update the perf. counters
	if(bres) 
  {
    // NOW SET IN AssociatePick()  nAssoc++;
		strcpy(sDisp, "       Assigned");
		goto pau;
	}

  // STEP 3: If desired, try to associate the pick into a new quake.
	if(bAssociate) 
  {
    // STEP 3.1: Try to associate a new quake, from the
    //           the new pick and the existing unassociated
    //           picks.
		bres = false;
		t1 = Secs();
		bres = Associate(pck.idPick);
		t2 = Secs();
		mon->tAss = t2 - t1 - mon->tLoc;

    // STEP 3.2: If we were able to associate the pick,
    //           we're done.  Update the perf. counters
		if(bres) {
			nQuake++;
			strcpy(sDisp, "       New Event");
			goto pau;
		}
	}

  // STEP 4:   No further processing to do.  Declare the
  //           pick an orphin.  Update perf counters.
	strcpy(sDisp, "       Hapless waif");

pau:

   end // CARL 120303  ***************/

	// Schedule for assignment:
  //  attempting to associate a Pick with one of 
  //  the existing quakes.
  RegisterUnassociatedPick(pck.idPick);

  // Process defered processing
	Action();

  // wrapup
	tDone = Secs();
	mon->tOther = tDone - tpick - mon->tAsn - mon->tAss - mon->tLoc;
	if(nPick)
		a = 0.95;
	else
		a = 0.0;
	b = 1.0 - a;
	avg->tIdle  = a*avg->tIdle  + b*mon->tIdle;
	avg->tAsn   = a*avg->tAsn   + b*mon->tAsn;
	avg->tAss   = a*avg->tAss   + b*mon->tAss;
	avg->tLoc   = a*avg->tLoc   + b*mon->tLoc;
	avg->tOther = a*avg->tOther + b*mon->tOther;
	nPick++;
	if(pMon)
		pMon->Refresh();
//	Debug("--------------------\n");
}

#define szPARAMLOG_BASE_FILENAME "glass_params_log.txt"
static char szGlassParamLogFilename[256];

//---------------------------------------------------------------------------------------CatOut
// Initialize catalog.txt with detection parameters
// Overwrites previous catalog.txt
void CGlass::CatOut() 
{
  FILE *f;
  
  
  char * szEW_LOG = getenv("EW_LOG");
  if(!szEW_LOG)
    szEW_LOG = "";

  // set the ParamLogfile name
  _snprintf(szGlassParamLogFilename,sizeof(szGlassParamLogFilename),"%s%s",szEW_LOG,szPARAMLOG_BASE_FILENAME);
  szGlassParamLogFilename[sizeof(szGlassParamLogFilename) - 1] = 0x00;

  if(!(f = fopen(szGlassParamLogFilename, "w")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CGlass::CatOut():  ERROR! Could not open param log file <%s>.\n",
      szGlassParamLogFilename);
    return;
  }
  
	fprintf(f, "Association parameters...\n");
	fprintf(f,   "%6d nCut           Minimum cluster size to nucleate new event\n", nCut);
	fprintf(f, "%6.1f dCut           Cluster distance (km)\n", dCut);
	fprintf(f, "%6.0f dTimeBefore    Time previous to current pick to consider\n", dTimeBefore);
	fprintf(f, "%6.0f dTimeAfter     Time after current pick to consider\n", dTimeAfter);
	fprintf(f, "%6.0f dTimeOrigin    Time before previous pick for trial loccations\n", dTimeOrigin);
	fprintf(f, "%6.0f dTimeStep      Trial location origin time granularity\n", dTimeStep);
	for(int i=0; i<nShell; i++) {
		fprintf(f, "%6.0f		Shell[%d]\n", dShell[i], i);
	}
	fprintf(f, "\n");
	fclose(f);
  f = NULL;
}

//---------------------------------------------------------------------------------------Grok
// Create grok message
void CGlass::Grok(char *idorg) {
	IMessage *m;
	m = pMod->CreateMessage("Grok");
	m->setStr("Quake", idorg);
	pMod->Broadcast(m);
	m->Release();
}


/*********  // CARL 120303 REPLACE  function Assign
//---------------------------------------------------------------------------------------Assign
// Scan quake list and try to assocate pick with existing origin
bool CGlass::Assign(char *idpick) {
	ORIGIN *org;
	PICK *pck;
	TTT *ttt;
  TTT tttLocal;
	char idorg[32]="";
	double ares;
	double bres;
	double d;
	double az;
	double t1;
	double t2;
	double t;
	double z;

  // Parameters of the best fitting origin
	double bT;
	double bZ;
	double bD;

  // STEP 0: Insure we have access to the traveltime tables.
	if(!pTT)
		return false;

  // STEP 1:   Obtain the full information for the pick
  //           from GLINT.
	pck = pGlint->getPickFromidPick(idpick);
	if(!pck)
		return false;

  // STEP 2: Select a time range (t1 - t2), within which
  //         we will search for origins that the pick could
  //         be associated with.
	t2 = pck->dT;
	t1 = t2 - 1680;	// 28 minutes
	bres = 100000.0;

  // STEP 3: For each origin within our time range
  //         attempt to associate the pick with the origin.
  //         Find the origin (if any exists) with the best
  //         residual fit.
	while(org = pGlint->getOrigin(t1, t2)) 
  {
    // STEP 3.1: 
    //         Calculate the deltaT(sec)
    //                       deltaZ(km) - depth
    //                   and deltaD(deg) - angle (pick/earth center/hypocenter)
    //         between the pick and the hypocenter
		t = pck->dT - org->dT;
		z = org->dZ;
		d = pTT->Delta(org->dLat, org->dLon, pck->dLat, pck->dLon);
		az = pTT->Azimuth(org->dLat, org->dLon, pck->dLat, pck->dLon);

    // STEP 3.2: 
    //         Obtain the most likely phase type, based upon
    //         angle, time, and depth.
		ttt = pTT->Best(d, t, z);

    // STEP 3.3: 
    //         If there wasn't a likely phase for this origin,
    //         go on to the next origin.
		if(!ttt)
			continue;
    else
      memcpy(&tttLocal,ttt,sizeof(tttLocal));

    // STEP 3.4: 
    //         Calculate the residual as the ABSOLUTE differnce 
    //         between the traveltimetable value for the chosen
    //         phase and the actual traveltime.
		ares = pck->dT - org->dT - tttLocal.dT;
		if(ares < 0.0)
			ares = -ares;

    // STEP 3.5: 
    //         If the residual for the current origin is
    //         within an acceptable range and better
    //         than previous origins, then record
    //         the origin, residual, and phase name
    if(ares < GLASS_RESIDUAL_THRESHOLD && ares < bres) 
    {
      strncpy(pck->sPhase, tttLocal.sPhs, sizeof(pck->sPhase));
      pck->sPhase[sizeof(pck->sPhase)-1] = 0x00;

			bres = ares;
			bT = t;
			bZ = z;
			bD = d;
			strncpy(idorg, org->idOrigin, sizeof(idorg)-1);
      idorg[sizeof(idorg)-1] = 0x00;
		}
       //		CDebug::Log(DEBUG_MINOR_INFO,"    iTTT:%d sPhs:%s az:%.2f"
       //                                " dD:%.2f dT:%.2f dZ:%.2f dToa:%.2f"
       //                                " dTdD:%.2f dTdZ:%.2f\n",
       //			          ttt->iTTT, ttt->sPhs, az, ttt->dD, ttt->dT, ttt->dZ, 
       //               ttt->dToa, ttt->dTdD, ttt->dTdZ);
  }  // end while(there are origins the meet the given time criteria)

  // STEP 4: If we found an acceptable origin, officially associate
  //         the pick with the origin.  
	if(bres < GLASS_RESIDUAL_THRESHOLD)
  {
    BOOL bRetCode;

    	ttt = pTT->Best(bD, bT, bZ);
    // STEP 3.3: 
    //         If there isn't a likely phase, then something is fishy!
		if(!ttt)
    {
	    CDebug::Log(DEBUG_MINOR_ERROR, 
                    "Inconsistency in the traveltime code.  Asked for "
                    "traveltime record for Origin match that was rated"
                    "the best, and now no record is available.  Think "
                    "memory corruption!\n Params(d t z) (%.2f %.2f %.2f\n",
                  bD,bT,bZ);
      return(false);
    }
    else
      memcpy(&tttLocal,ttt,sizeof(tttLocal));

	  CDebug::Log(DEBUG_MINOR_INFO, "Pick:%s associated with Origin:%s\n", pck->idPick, idorg);

// STEP 4.1:
//       Perform the association.
//         Return true, indicating that the pick was associated.
    if(bRetCode=AssociatePickWOrigin(idorg,pck,bLocate,&tttLocal))
    {
// DK 1.0 PULL        if(org=pGlint->getOrigin(idorg))
// DK 1.0 PULL        {
// DK 1.0 PULL          // STEP 4.1.1:
// DK 1.0 PULL          //       Determine if Extended Processing should be done at this time
// DK 1.0 PULL          //       to the affected origin .
// DK 1.0 PULL          if(org->nPh > org->nNextExtProc)
// DK 1.0 PULL          {
// DK 1.0 PULL            PerformExtendedProcessing(org);
// DK 1.0 PULL            org->nNextExtProc = org->nPh + iReportInterval;
// DK 1.0 PULL          }
// DK 1.0 PULL        }
// DK 1.0 PULL        else
// DK 1.0 PULL        {
// DK 1.0 PULL  	      CDebug::Log(DEBUG_MINOR_ERROR, "Assign(): Could not obtain origin "
// DK 1.0 PULL                                         "from Glint for idorigin:%s\n", idorg);
// DK 1.0 PULL        }
      return(true);
    }  // end if AssociatePickWOrigin()
    // STEP 4.2:
    //       If something went wrong with the association,
    //         return false, indicating that the pick was not associated.
    else
    {
      return(false);
    }
  }
  // STEP 5: If we didn't find an acceptable origin, return false,
  //         indicating that the pick was NOT associated.
  else  // if bres < GLASS_RESIDUAL_THRESHOLD
  {
		return false;
  }
}  // end Assign()

*********  END // CARL 120303 REPLACE  function Assign   */


// CARL 120303 REPLACEMENT  function Assign
//---------------------------------------------------------------------------------------Assign
// Scan quake list and try to assocate pick with existing origin
// Action returns
//		0: Pick not assigned
//		1: Pick assigned
int CGlass::Assign(char *idpick) {
	ORIGIN *org;
	PICK *pck;
	PICK CurrentPick,BestPick;
	char idorg[18]="";
	double affmax;
	double t1;
	double t2;
  bool bRetCode;
  int iRetCode;

  // STEP 0: Ensure we have access to the traveltime tables.
	if(!pTT)
		return false;

  // STEP 1:   Obtain the full information for the pick
  //           from GLINT.
	pck = pGlint->getPickFromidPick(idpick);
	if(!pck)
		return false;

  // STEP 1.1:   Ensure that the pick is a WAIF.  This is
  //             a check to make sure we don't have a bug
  //             in the system that is causing us to try
  //             to re-associate already associated picks.
  if(pck->iState != GLINT_STATE_WAIF)
  {
    // STEP 1.1.1:   Pick's not a waif.  Complain. Return failure.
    CDebug::Log(DEBUG_MINOR_ERROR, 
                "Assign(): Error: Pick (%s/%s) is not a waif! (%s/%s/%s/%s - %.2f)\n"
                "iPick %d, iOrigin %d\n",
                idpick, pck->idPick, pck->sSite, pck->sComp, pck->sNet, pck->sLoc,
                pck->dT, pck->iPick, pck->iOrigin);
   return(0);
  }


  // STEP 2: Select a time range (t1 - t2), within which
  //         we will search for origins that the pick could
  //         be associated with.
	t2 = pck->dT;
	t1 = t2 - OPCALC_dSecondaryAssociationPrePickTime;

  // Initialize the max affinity to the highest possible
  //  non-associable value.  (Instead of initializing to
  //  0, which would cause us to do affmax update, even
  //  if the affinity wasn't high enough to associate)
	affmax = OPCALC_dAffMinAssocValue * 0.99;

  // STEP 3: For each origin within our time range
  //         attempt to associate the pick with the origin.
  //         Find the origin (if any exists) with the best
  //         residual fit.
	while(org = pGlint->getOrigin(t1, t2)) 
  {

    // STEP 3.1: 
    //         Copy the pick's params to a temp structure (CurrentPick)
		memmove(&CurrentPick, pck, sizeof(CurrentPick));

    // STEP 3.2: 
    //         Obtain the most likely phase-type (based upon angle, time, and depth), 
    //         and calculate the affinity of this pick as that phase-type, to the
    //         current origin. 
		if(iRetCode = OPCalc_CalcPickAffinity(org, &CurrentPick))
      if(iRetCode < 0)
      {
        CDebug::Log(DEBUG_MINOR_ERROR,"OPCalc_CalcPickAffinity() failed with code %d\n",
                    iRetCode);
        return(0);
      }
      else
      {
        continue;
      }

    // STEP 3.3: 
    //         If the current origin is the best match (so far), save it's ID.
    //         and the origin-pick params (stored in Pick structure)
		if(CurrentPick.dAff > affmax) 
    {
			affmax = CurrentPick.dAff;
			strcpy(idorg, org->idOrigin);
      memcpy(&BestPick, &CurrentPick, sizeof(CurrentPick));
		}
       //		CDebug::Log(DEBUG_MINOR_INFO,"    iTTT:%d sPhs:%s az:%.2f"
       //                                " dD:%.2f dT:%.2f dZ:%.2f dToa:%.2f"
       //                                " dTdD:%.2f dTdZ:%.2f\n",
       //			          ttt->iTTT, ttt->sPhs, az, ttt->dD, ttt->dT, ttt->dZ, 
       //               ttt->dToa, ttt->dTdD, ttt->dTdZ);
  }  // end while(each eligible origin in list)

  // STEP 4: If we found an acceptable origin, officially associate
  //         the pick with the origin.  
	if(affmax >= OPCALC_dAffMinAssocValue)
  {
    CDebug::Log(DEBUG_MINOR_INFO, "Assigning Pick(%s) to Origin:%s\n", pck->idPick, idorg);

    // STEP 4.1:
    //       Save the Origin-Pick params from BestPick to pck
    memcpy(pck,&BestPick,sizeof(BestPick));

    // STEP 4.2:
    //       Perform the association.
    if(bRetCode=AssociatePickWOrigin(idorg,pck))
    {
      // STEP 4.2.1:
      //       Success, Pick Associated.
      //       Return 1, indicating that the pick was associated.
      return(1);
    }
  }
  return(0);  // failed to associate.
}  // end Assign()


//------------------------------------------------------------------------AssociatePickWOrigin
// Associate a pick with a hypocenter
bool CGlass::AssociatePickWOrigin(char *idorg, PICK * pck) 
{
  // Formally associate a Pick with an Origin 
  // Assumes that the preliminary Origin-Pick parameters 
  // have been calculated (TOA, Phase Type, Residual, TTT)
  // and stored in pck.

  // STEP 0:   Validate the input params.
  //           Retrieve the origin from glint, based on the given ID

  if(!(idorg && pck))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"AssociatePickWOrigin(): Bad input params: "
                                   "idorg(%u), pck(%u)\n",
                                   idorg, pck);
    return(false);
  }
  ORIGIN * pOrg = pGlint->getOrigin(idorg);

  if(!pOrg)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"AssociatePickWOrigin(): Could "
                                   "not retrieve origin for id(%s)\n",
                idorg);
    return(false);
  }


  // STEP 1:   Check to make sure the association doesn't violate any rules

  // STEP 1.1: Make sure there aren't any matching picks already associated with
  //           this event.
  if(!HandleMatchingPicks(pOrg, pck))
    return(false);

  // STEP 2:   origin-pick params are ALREADY set in the Pick

  // STEP 3:   Record the association in glint
	if(!pGlint->OriginPick(idorg, pck->idPick))
    return(false);

  // STEP 4:   Update the performance counter.
  nAssoc++;

  // STEP 5:   Change the state of the pick to ASSOC
  pck->iState = GLINT_STATE_ASSOC;

  // STEP 6:   Export the new origin-pick-link
	PutLink(idorg, pck->idPick);

  // reinitialized stored focus location if any
  pOrg->dFocusedT = 0.0;
  pOrg->dFocusedZ = 0.0;

  // STEP 7:   Done.  Return true.
	return(true);
}  // end AssociatePickWOrigin()



//------------------------------------------------------------------------AssociatePickWOrigin
// Associate a pick with a hypocenter
bool CGlass::UnAssociatePickWOrigin(char *idorg, PICK * pck, bool bMarkOriginAsScavenged) 
{
  // Formally unassociate a Pick with an Origin 

  // STEP 0:   Validate the input params.
  //           Retrieve the origin from glint, based on the given ID

  if(!(idorg && pck))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"UnAssociatePickWOrigin(): Bad input params: "
                                   "idorg(%u), pck(%u)\n",
                                   idorg, pck);
    return(false);
  }
  ORIGIN * pOrg = pGlint->getOrigin(idorg);

  if(!pOrg)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"UnAssociatePickWOrigin(): Could "
                                   "not retrieve origin for id(%s)\n",
                idorg);
    return(false);
  }


  // STEP 1:   Record the association in glint
	pGlint->UnOriginPick(idorg, pck->idPick);

  // STEP 2:   Update the performance counter.
  nAssoc--;

  // STEP 3:   Change the state of the pick to WAIF
  pck->iState = GLINT_STATE_WAIF;

  // STEP 4:   Export the new un-origin-pick-link
	PutUnLink(idorg, pck->idPick);

  // STEP 5:   If desired, mark the origin as scaveneged to give it a chance
  //           to clean itself up.
  if(bMarkOriginAsScavenged)
    MarkOriginAsScavenged(pOrg->idOrigin);


  // STEP 6:   Done.  Return true.
	return(true);
}  // end UnAssociatePickWOrigin()


//---------------------------------------------------------------------------------------Waif
// Scan unassociated picks for inclusion in recently modified hypocenter
//  returns:
//		0: No changes
//		N > 0: Waifs associated, need to relocate
int CGlass::Waif(char *idorg) {
	ORIGIN *org;
	PICK *pck;
	double t1;
	double t2;
	int n=0;  // Initialize "number of picks associated" to 0
  //TTEntry tttLocal;

  int iRetCode;
  // CARL 120303 remove
	// TTT *ttt;
	// double tres;
	// double t;
	// double z;
	// double d;

  // CARL 120303 
  PICK CurrentPick;
  //double aff;

  // STEP 0: Validate parameters
  // If we don't have access to traveltime tables, abort.
	if(!(pTT && idorg))
  {
    CDebug::Log(DEBUG_MINOR_ERROR, "Waif(): pTT(%u) and idorg(%u) are both "
                                   "needed as input params. Returning error!\n",
                pTT, idorg);
    return NULL;
  }

  // STEP 1:   Retrieve full information for the Origin from GLINT.
  //           Abort if Origin is not found.
	org = pGlint->getOrigin(idorg);
	if(!org)
		return 0;

  // STEP 2:   Calculate a time range (t1 - t2), for which to look
  //           for picks that might be associatable with the Origin.
  t1 = org->dT;
	t2 = t1 + OPCALC_dSecondaryAssociationPrePickTime;  // 28 minutes

  // STEP 3:   For each unassociated pick within the timerange, attempt
  //           to associate the pick.
  size_t iPickRef=0;
	while(pck = pGlint->getWaifsForTimeRange(t1, t2, &iPickRef)) 
  {
    if(pck->iState != GLINT_STATE_WAIF)
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "Pick %s retrieved in getWaifsXXX() call, but is not waif(%d)\n",
                  pck->idPick, pck->iState);
      continue;
    }
		memmove(&CurrentPick, pck, sizeof(PICK));
    // STEP 3.1:   For each unassociated pick within the timerange, attempt
    //           to associate the pick.
		iRetCode = OPCalc_CalcPickAffinity(org, &CurrentPick);
    // STEP 3.2:   Ensure we were successfully able to calc an affinity

    if(iRetCode != 0)
    {
      if(iRetCode < 0)
      {
        CDebug::Log(DEBUG_MINOR_ERROR,"Waif(): Error during OPCalc_CalcPickAffinity(%s)\n",
                    CurrentPick.idPick);
        return(0);
      }
      continue;
    }

    // STEP 3.2:   If the pick has a high enough affinity, associate
    //             it with the Origin.
    if(CurrentPick.dAff >= OPCALC_dAffMinAssocValue) 
    {
      memcpy(pck,&CurrentPick, sizeof(CurrentPick));
      if(AssociatePickWOrigin(idorg,pck))
      {
        n++;
      }
    }
  }  // end while(each waif in time range)
	return n;
}  // end Waif()


//---------------------------------------------------------------------------------------Locate
// Locate.   (Actual Location done by 'glock' module)
//  returns
//		0: Location not changed
//		1: Location changed
//    2: Location unresolvable
int CGlass::Locate(char *idorg, char *mask) 
{
	double t = Secs();
	IMessage *m;
  int res;

  ORIGIN * pOrg = pGlint->getOrigin(idorg);
  if(!pOrg)
    return(0);

  CDebug::Log(DEBUG_MINOR_INFO, "Locate: starting Origin(%.2f/%.2f/%.0f - %.2f  %d,%d\n",
              pOrg->dLat, pOrg->dLon, pOrg->dZ, pOrg->dT, pOrg->nPh, pOrg->nEq);

  // Send out a Locate Message (for GLOCK)
	m = pMod->CreateMessage("Locate");
	m->setStr("Quake", idorg);
	if(mask)
		m->setStr("Mask", mask);
	pMod->Dispatch(m);

  // Get the result
  res = m->getInt("Res");

  // Release the message
	m->Release();

//  if(res== 0 || res == 1)
    if(res== 0)
	  pOrg->nLocate++;

  // DK REMOVED  - Locator will calculate Affinity during sanity check
	// Affinity(org);
	Mon[0].tLoc += Secs() - t;

  CDebug::Log(DEBUG_MINOR_INFO, "Locate: ending Origin(%.2f/%.2f/%.0f - %.2f  %d,%d\n",
              pOrg->dLat, pOrg->dLon, pOrg->dZ, pOrg->dT, pOrg->nPh, pOrg->nEq);

  // This was an Algorithmic change added by DK in v1.0 and subsequently pulled
  // DK 1.0 PULL  // Trim picks with outlying residuals.
  // DK 1.0 PULL  TrimOutliers(pOrg);

  // Export the Origin
	PutOrigin(idorg,false);
  return(1);
}  // end Locate()


int CGlass::UpdateGUI(char * idOrigin)
{
  // Update the GUI
	Grok(idOrigin);
  return(0);
}  // UpdateGUI()

//---------------------------------------------------------------------------------------Focus
// Calculate new origin from refocussing algoritm
// Action returns
//		0: No change
//		1: New location assisgned
int CGlass::Focus(char *idorg) {
	IMessage *m;
	m = pMod->CreateMessage("Focus");
	m->setStr("Quake", idorg);
	pMod->Dispatch(m);

  // Get the result
  int res = m->getInt("Res");

  // Release the message
	m->Release();

  if(res == 0)
	  return 1;
  else
    return 0;
}

//---------------------------------------------------------------------------------------Prune
// Action returns
//		0: No change
//		1: Picks disassociated
int CGlass::Prune(char *idorg) 
{
	PICK *pPick;
	ORIGIN *pOrg;
	int res;

	pOrg = pGlint->getOrigin(idorg);
	if(!pOrg)
		return 0;
	res = 0;
  size_t iPickRef=0;
  while(pPick = pGlint->getPicksFromOrigin(pOrg,&iPickRef))
  {
    if(pPick->bTTTIsValid && pPick->dAff >= (OPCALC_dAffMinAssocValue - OPCALC_dAffinitySlop))
      continue;

    // Deassociate if affinity too low
	  CDebug::Log(DEBUG_MINOR_INFO,
		            "Pick(%d   %s - %s - %.0f, %.1f %3.0f) (%4.1f=%4.1f*%4.1f*%4.1f*%4.1f) - %4.1f being pruned from Origin(%s)\n",
		            pPick->idPick, pPick->sSite, pPick->sPhase, pPick->dDelta, pPick->tRes, pPick->dAzm, 
                pPick->dAff, pPick->dAffRes, pPick->dAffDis, pOrg->dAffGap, pOrg->dAffNumArr, pOrg->dAff, idorg );
	  
	  UnAssociatePickWOrigin(pOrg->idOrigin,pPick,false);
	  res = 1;
  }
	return res;
}

//---------------------------------------------------------------------------------------Prune
// Action returns
//		0: No change
//		1: Picks disassociated
int CGlass::PrunePick(char *idOrigin, char * idPick) 
{
  PICK *pPick;
  ORIGIN *pOrg;
  
  pOrg = pGlint->getOrigin(idOrigin);
  if(!pOrg)
    return(0);
  
  pPick = pGlint->getPickFromidPick(idPick);
  if(!pOrg)
    return(0);
  
  UnAssociatePickWOrigin(pOrg->idOrigin, pPick, false);
  return(1);
}

//---------------------------------------------------------------------------------------Scavenge
// Action returns
//		0: No change
//		1: Picks scavenged and added to primary
int CGlass::Scavenge(char *idorg) {
	PICK CurrentPick;
	PICK *pPick;
	ORIGIN *pOrg;
	ORIGIN *pOrg2;
	double t1;
	double t2;
  int iNumScavenged = 0;

	pOrg = pGlint->getOrigin(idorg);
	if(!pOrg)
		return 0;
	t1 = pOrg->dT - 10.0;
	t2 = pOrg->dT + 2000.0;
  size_t iPickRef=0;
	while(pPick = pGlint->getPicksForTimeRange(t1, t2, &iPickRef)) 
  {
		// Already associated
		if(pPick->iState == GLINT_STATE_ASSOC && pPick->iOrigin == pOrg->iOrigin)
			continue;
		// Caculate affinity for new association
		memmove(&CurrentPick, pPick, sizeof(PICK));

		CurrentPick.iState = GLINT_STATE_WAIF;
		OPCalc_CalcPickAffinity(pOrg, &CurrentPick);
		if(pPick->iState == GLINT_STATE_WAIF) 
    {
			if(CurrentPick.dAff > OPCALC_dAffMinAssocValue) 
      {
        //Associate  (try to associate, if error, then oh-well)
        AssociatePickWOrigin(pOrg->idOrigin,pPick);
			}
		}
    else
    {
      // Belongs to another quake
      // Check to see if the existing origin/pick affinity
      // is better than ours.
      if(CurrentPick.dAff > pPick->dAff)
      {

        CDebug::Log(DEBUG_MAJOR_INFO,
                    "Pick(%d   %s - %s - %.0f, %.1f) being scavenged from Origin(%s)\n",
                    pPick->idPick, pPick->sSite, pPick->sPhase, pPick->dDelta, pPick->tRes, idorg);
	  
        
        // Unassociate the pick with the other origin
        if(pOrg2 = pGlint->getOriginForPick(pPick))
        {
          UnAssociatePickWOrigin(pOrg2->idOrigin,pPick,true);
        }
        else
          CDebug::Log(DEBUG_MINOR_ERROR,
                      "Pick(%s) supposively associated with Origin(%d), but origin "
                      "could not be retrieved.\n",  pPick->idPick, pPick->iOrigin);

        
        // Associate the pick with the current origin.
        memcpy(pPick,&CurrentPick,sizeof(CurrentPick));
        if(AssociatePickWOrigin(pOrg->idOrigin,pPick))
          iNumScavenged++;
      }
    }
  }  // end while(each pick in range)
  if(iNumScavenged > 0)
    return(1);
  else
	  return 0;
}  // end Scavenge()


// Primary associate, collect all waif that are possible candidates for inclusion with
// pick provided and attempt to create a new origin.
//  returns
//		0: No change
//		1: New quake nucleated
int CGlass::Associate(char *idpck) {
	ORIGIN org;
	CSpock *spock;
	PICK *pck;		// Primary pick

  // STEP 1:   Obtain the full information for the pick
  //           from GLINT.
	pck = pGlint->getPickFromidPick(idpck);
	if(!pck)
		return false;
//	Debug("Glint succeeded\n");

  // STEP 2: Select a time range (t1 - t2), within which
  //         we will search for picks that could
  //         be associated with the current pick.
	double t1 = pck->dT + dTimeBefore;
	double t2 = pck->dT + dTimeAfter;
	Pck[0] = pck;
	nPck = 1;

  // STEP 3: For each pick within our time range
  //         copy the pick into our private Pick array.
  //         Don't exceed MAXPCK picks.
  size_t iPickRef=0;
	while(pck = pGlint->getWaifsForTimeRange(t1, t2, &iPickRef)) {
//		Debug("Waif[%d] %s\n", nPck, pck->idPick);
		if(pck->iPick == Pck[0]->iPick)
			continue;
		Pck[nPck] = pck;
		if(nPck >= MAXPCK)
    {
      pGlint->endPickList(&iPickRef);
			break;
    }
		nPck++;
  }  // end while picks

  // STEP 4: We can't associate with less than
  //         four picks.  If we have less than
  //         four, we're done.  Quit and go home.
  //         Quit and go home if we don't have 
  //         a number approaching nCut, since it
  //         will be cut later, because of lack of
  //         nCut points, even if it associates well.
	if(nPck < (nCut / 1.25))
		return false;

	double a;
	double z;
	double t;
	double lat;
	double lon;
	double abest = dCut * 2;
	double t0;
	double lat0;
	double lon0;
	double z0;
	int i;
  int nAssociatedPicks;

  // STEP 5: Only use Primary phases (P,Pup,Pdiff) for association
  //         of new origins.

  // STEP 6: Select a time range (t1 - t2), within which
  //         we think an Origin could have occurred, assuming
  //         the given phase is a P-ish arrival.
	t2 = Pck[0]->dT - 0.9;
	t1 = t2 + dTimeOrigin;

  
  // STEP 7: Attempt to associate the picks in the array at
  //         different time/depth points.  Find the best
  //         origin, based on a residual-value.  Record the parameters 
  //         of the best trial origin.
  //
  //         For each depth shell that we are configured to use,
  //         iterate through possible origin times.
	for(i=0; i<nShell; i++) 
  {
		z = dShell[i];

    // STEP 7.1: 
    //       For each timestep within our (t1-t2) timerange.  Attempt to
    //       associate the picks in the array into an origin, at 
    //       the given depth(z).
		for(t=t1; t<t2; t+=dTimeStep) 
    {

      // STEP 7.1.1: 
      //     Attempt to associate the picks in the array into an origin,
      //     recording a residual value for the origin.
      //     (See Associate() for a description of how the 
      //      residual-value is calculated.)
			a = Associate(t, z, &lat, &lon);


      // STEP 7.1.2: 
      //     If the residual value of the current origin, is the
      //     smallest so far, save the parameters of the current
      //     origin: time, lat, lon, and depth(z)
			if(a < abest) 
      {
        CDebug::Log(DEBUG_MINOR_INFO, "Nucleation:  a=%.2f, t=%.1f, lat=%.0f, lon = %.0f, z=%.1f\n", a, t, lat, lon, z);

				abest = a;
				t0 = t;
				lat0 = lat;
				lon0 = lon;
				z0 = z;
			}
    }  // end for each timestep
  }    // end for each depth shell


  // STEP 8: Record the best origin location, even if it doesn't
  //         have the required nucleation distance.
  //
	dAss[nAss%MAXASS] = abest;
	nAss++;


  // STEP 9: If our origin meets the required distance nucleation
  //         criteria, process it.
  //
	if(abest < dCut) 
  {
    // STEP 9.1: 
    //       Record the new origin in GLINT.
		org.iVersion = GLINT_VERSION;
		org.dT = t0;
		org.dLat = lat0;
		org.dLon = lon0;
		org.dZ = z0;
		org.nEq = 0;
    org.nPh = 0;
		org.dRms = 0.0;
		org.dDelMin = 0.0;
		org.dDelMod = 0.5;
		org.dDelMax = 1.0;
    org.dGap = 360.0;
    org.dAffGap = org.dAffNumArr = 1.0;
    
// DK 1.0 PULL      org.nNextExtProc = iReportInterval;
		CDate dt(t0);
		CDebug::Log(DEBUG_MINOR_INFO, "Origin: %s %.4f %.4f %.2f (%.4f)\n",
			dt.Date20().GetBuffer(), lat0, lon0, z0, abest);
		sprintf(org.sTag, "0.0.0.0");
		pGlint->putOrigin(&org);
		CDebug::Log(DEBUG_MINOR_INFO, "Nucleated %s: %d lat:%.4f lon:%.4f z:%.4f\n", org.idOrigin, org.iOrigin, org.dLat, org.dLon, org.dZ);

    // STEP 9.2: 
    //       Export the new origin to Earthworm or some other seismic processing system.
    PutOrigin(org.idOrigin,true);
    nQuake++;

    // STEP 9.2.1:
    //       Associate the starting pick with the Origin.  (FORCE THIS ASSOCIATION)
    //       It can always be pruned later.
    pck = pGlint->getPickFromidPick(idpck);
    if(!AssociatePickWOrigin(org.idOrigin, pck))
      return(false);
    nAssociatedPicks=1;


    // STEP 9.3: 
    //       Attempt to associate any/all of the unassociated picks with
    //       the new origin.
		nAssociatedPicks+=Waif(org.idOrigin);

    if(nAssociatedPicks < (nCut-1))
    {
      time_t tOrigin;
      double  dLat, dLon;
      tOrigin = (time_t)org.dT;
      /**********************************************************
       * NOTE:  This condition is not NECCESSARILY an error.  It most commonly
                occurs because:
                   The hypocenter is close to a group of channels, 
                   each (or several) channel generates 2 intersections
                   instead of 1, and the nucleation logic currently counts
                   intersections instead of channels
        *******************************************************************/
      CDebug::Log(DEBUG_MAJOR_INFO, "Nucleated %s: %d lat:%.4f lon:%.4f z:%.4f, t:%.4f - %s"
                                     " but only able to associate %d phases with hypocenter! Something's wrong!\n",
                org.idOrigin, org.iOrigin, org.dLat, org.dLon, org.dZ, org.dT, ctime(&tOrigin),
                nAssociatedPicks);
      CDebug::Log(DEBUG_MAJOR_INFO, "Nucleated for pick %s.%s.%s.%s - %.0f\n",  
                  pck->sSite, pck->sComp, pck->sNet, pck->sLoc, pck->dT);
      AssociatePicks(t0, z0, &dLat, &dLon, Pck, nPck, pTT,nCut, false, true);
      CDebug::Log(DEBUG_MAJOR_INFO, "Resulting in origin lat:%.4f lon:%.4f z:%.4f, t:%.4f\n",
                dLat, dLon, t0, z0);

    }

    // STEP 9.4:
    //       Write Associator debug info to the log file if desired.
    //       (We will need to rerun the secondary associate on 
    //        the origin to get all the parameters for logging)
		if(pMod->bSpockReport) {
			spock = new CSpock();
			spock->Init(this, pMod->pReport);
      Associate(t0, z0, &lat, &lon);
			spock->Report(t0, lat0, lon0, z0, nXYZ, dXYZ);
			delete spock;
		}

    // STEP 9.5: 
    //       Locate the new origin.  (Attempt to improve the location
    //         through iterative hypocenter adjustments)
    //Locate(org.idOrigin, 0);
    // DK  No need to call Locate, state-logic will handle the transition
    // LocateOrigin(org.idOrigin);


    // STEP 9.6: 
    //       Done.  Return true indicating that we succesfully associated
    //       a new quake around the given phase.
  	return true;
	}
               //	Debug("Association failed\n");

  // STEP 10:Done.  Return false indicating that we were not able
  //         to associate the given phase into a new acceptable hypocenter.
  return false;
}  //end Associate(idPck)

//---------------------------------------------------------------------------------------Associate
// Auxilliary association (called once for each origin time and depth shell
// This function must be called by the primary Associate() function, because
//   the primary fills in the Pck[] array with picks eligible to be used in 
//   the auxilliary.
double CGlass::Associate(double torg, double depth, double *lat, double *lon) 
{
  return(AssociatePicks(torg, depth, lat, lon, Pck, nPck, pTT,nCut));
}

//------------------------------------------------------------------------HandleMatchingPicks
// Scan origin for any picks matching the given one (Phase and SCNL are the same)
// Handle any unassociation deemed neccessary.
// Return true if the given pick should be associated with Origin, or
// false if it should not.
bool CGlass::HandleMatchingPicks(ORIGIN * pOrg, PICK * pPick)
{
    PICK * pAltPick;
    // We should do a check here.  We should make sure that we are not associating
    // a new pick with a Phase that is already associated for an existing channel
    // If we already have a pick from XYZ that is associated with quake as P phase,
    // then we should not associate another XYZ pick with the quake as a P phase.
    // There should be only one P phase per channel per quake.
    // To be fancy, we could check to see if the residual for the new pick is better
    // than the residual for the old pick.
    // To be really fancy, we could try to do a distance/tt fit based upon the time
    // between one pick and the next.
    //       Retrieve the selected phasetype for the pick from the traveltime
    //       record.

  if(!(pOrg && pPick))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"HandleMatchingPicks(): Bad input params: "
                                   "pOrg(%u), pPick(%u),\n",
                pOrg, pPick);
    return(false);
  }
    
  size_t iPickRef=0;
	while(pAltPick = pGlint->getPicksFromOrigin(pOrg,&iPickRef)) 
  {
    // If the channel's don't match (SCNL), we're OK.  Continue
    if(pGlint->ComparePickChannels(pAltPick,pPick))
      continue;

    // If the phase type's don't match (P vs. PP), we're OK.  Continue
    if(strcmp(pAltPick->sPhase,pPick->sPhase))
      continue;

    // Now we're in trouble.  We have two matching phases
    //    example:   AAA EHZ US "P" and AAA EHZ US "P"

    // Calculate the affinity for the new pick (pPick)
    // ASSUME: we assume that the
    // affinity for the new pick already calculated prior to
    // AssociatePickWOrigin() call.

    // Calculate the residual for the old pick (pAltPick)
    // Affinity for the old pick already calculated during last
    // location, or when it was associated with the origin.

    if(pAltPick->dAff >= pPick->dAff)
    {
      // We can't improve on the affinity of the old pick.
      // Leave well enough alone.  Don't associate this
      // pick with the origin, because there's already 
      // a matching phase and the new one is no better.
      if(pAltPick->dT == pPick->dT)
      {
        CDebug::Log(DEBUG_MINOR_INFO,
                    "HandleMatchingPicks(): DUPLICATE PICKS found.  \n"
                    "%s:%.2f (old %s) (new %s)\n",
                    pAltPick->sSite, pAltPick->dT, pAltPick->sTag,
                    pPick->sTag);
      }
      CDebug::Log(DEBUG_MINOR_INFO,
                    "HandleMatchingPicks(): duplicate found.  \n"
                    "Keeping old (%s:%s (old %.2f-%5.2f) (new %.2f-%5.2f)\n",
                  pAltPick->sSite, pAltPick->sPhase, 
                  pAltPick->dT, pAltPick->dAff,
                  pPick->dT, pPick->dAff);
      pGlint->endPickList(&iPickRef);
      return(false);
    }
    else
    {
      // Now we're in trouble.  The new phase is a better fit
      // than the old phase.  Disassociate the old phase! 
      CDebug::Log(DEBUG_MAJOR_INFO,
                    "HandleMatchingPicks(): duplicate found.  \n"
                    "Using new   (%s:%s (old %.2f-%5.2f) (new %.2f-%5.2f)\n",
                  pAltPick->sSite, pAltPick->sPhase, 
                  pAltPick->dT, pAltPick->dAff,
                  pPick->dT, pPick->dAff);
	    UnAssociatePickWOrigin(pOrg->idOrigin, pAltPick, true);
      pGlint->endPickList(&iPickRef);
      break;  // there should be at most one matching pick.
    }
  }  // end while picklist
  return(true);
}  // end HandleMatchingPicks()


bool CGlass::SetTravelTimePointer(ITravelTime * ptt)
{
  if(ptt)
  {
    pTT = ptt;
    OPCalc_SetTravelTimePointer(pTT);
    return(true);
  }
  else
  {
    return(false);
  }
}


/************************** DK REMOVE from v1.0
int CGlass::PerformExtendedProcessing(ORIGIN * pOrg)
{
  int iNumPhases;

  CDebug::Log(DEBUG_MAJOR_INFO, "Attempting to scavenge picks for origin:%s\n",
              pOrg->idOrigin);

  // Attempt to scavenge picks from smaller quakes.
  iNumPhases = Scavenge(pOrg);

  CDebug::Log(DEBUG_MAJOR_INFO, "Scavenged %d picks for origin %s!\n",
              iNumPhases, pOrg->idOrigin);

  PutOrigin(pOrg->idOrigin, true);

  return(0);
}


// Remove picks with high residuals
bool CGlass::TrimOutliers(ORIGIN * pOrg)
{
  double tRes;
  PICK * pPick;

  // STEP 1: Insure that we have a valid origin pointer
  if(!pOrg)
    return(false);

  // STEP 2: For each pick associated with the given origin
  //         check to make sure the residual(s) is within an
  //         acceptable range.
  int iPickRef=0;
  while(pPick = pGlint->getPicksFromOrigin(pOrg,&iPickRef))
  {
    // STEP 2.1:   
    //         Calculate the residual(sec) between the traveltime for the
    //         selected phase and the actual deltaT
		tRes = pPick->dT - pOrg->dT - pPick->dTrav;
		if(tRes > GLASS_RESIDUAL_THRESHOLD || tRes < (0 - GLASS_RESIDUAL_THRESHOLD)) 
		{
      CDebug::Log(DEBUG_MAJOR_INFO, "Triming Pick %s from Origin %s because of residual(%s:%.2f)\n",
                  pPick->idPick, pOrg->idOrigin, pPick->sPhase, tRes);
      pGlint->UnOriginPick(pOrg->idOrigin, pPick->idPick);
    }
  }  // end while picks
  return(true);
}


//---------------------------------------------------------------------------------------Scavenge
// Scan unassociated picks for inclusion in recently modified hypocenter
int CGlass::Scavenge(ORIGIN * pOrg) {
	PICK *pck;
	TTT *ttt;
  ORIGIN * pAltOrigin;
	double tres;
	double t1;
	double t2;
	double t;
	double z;
	double d;
  TTT tttLocal;
	int n=0;  // Initialize "number of picks associated" to 0

  // If we don't have access to traveltime tables, abort.
	if(!pTT)
		return 0;

  // STEP 1:   Calculate a time range (t1 - t2), for which to look
  //           for picks that might be associatable with the Origin.
	t1 = pOrg->dT;
	t2 = t1 + 1680.0;

  // STEP 2:   For each pick within the timerange, attempt
  //           to associate the pick(assuming it doesn't already
  //           belong to the current event or a larger one.
  int iPickRef=0;
	while(pck = pGlint->getPicksForTimeRange(t1, t2, &iPickRef)) 
  {
    // Don't try to steal picks from ourself.
		if(pck->iOrigin == pOrg->iOrigin)
			continue;

    // STEP 2.1:
    //        Make sure we're allowed to use this pick
    //        (It's unassociated, or associated with a smaller
    //         quake. )
    if(pck->iState == GLINT_STATE_ASSOC)
    {
      pAltOrigin = pGlint->getOriginForPick(pck);
      if(pAltOrigin  && pAltOrigin->nPh > pOrg->nPh)
      {
        // we don't have precedence to steal this phase from 
        // the other quake.  Continue to the next phase.
        continue;
      }
    }
    else
    {
      pAltOrigin = NULL;
    }
    // STEP 2.2:   
    //         Calculate the following params for use in fitting
    //         the pick to the closest theoretical phase via
    //         the traveltime tables:
    //             t:  deltaT between origin time and pick time
    //             z:  depth of the origin(km)
    //             d:  radial distance(rad) between origin and pick
		t = pck->dT - pOrg->dT;
		z = pOrg->dZ;
		d = pTT->Delta(pOrg->dLat, pOrg->dLon, pck->dLat, pck->dLon);


    // STEP 2.3:   
    //         Retrieve the most likely phase type from the traveltime tables.
		ttt = pTT->Best(d, t, z);


    // STEP 2.4:   
    //         If there is no likely phase for this pick, continue on 
    //         to the next pick.
		if(!ttt)
			continue;
    else
      memcpy(&tttLocal,ttt,sizeof(tttLocal));


    // STEP 2.5:   
    //         Calculate the residual(sec) between the traveltime for the
    //         selected phase and the actual deltaT
		tres = pck->dT - pOrg->dT - tttLocal.dT;


    // STEP 2.6:   
    //         If the residual is within the acceptable 
    //         threshold (+/- GLASS_RESIDUAL_THRESHOLD),
    //         associate the pick with the Origin.
		if(tres > (0 - GLASS_RESIDUAL_THRESHOLD) && tres < GLASS_RESIDUAL_THRESHOLD) 
		{
      CDebug::Log(DEBUG_MAJOR_INFO, "Pick:%s(%s) associated with Origin:%s "
                                    "(scavenged from Origin %s)\n", 
                                    pck->idPick, pck->sSite, pOrg->idOrigin, 
                                    pAltOrigin ? pAltOrigin->idOrigin: "<NULL>");
      
      // STEP 2.6.1:   
      //       Unassociate the pick from any existing origin
      //       if it is already associated.
      if(pAltOrigin)
      {
        pGlint->UnOriginPick(pAltOrigin->idOrigin, pck->idPick);
      }

      // STEP 2.6.2:   
      //       Formally associate the waif pick w/origin
      if(!AssociatePickWOrigin(pOrg->idOrigin,pck))
        CDebug::Log(DEBUG_MAJOR_INFO, "AssociatePickWOrigin() failed for (%s,%s)\n",
                    pOrg->idOrigin, pck->idPick);


      // STEP 2.6.3:   
      //       Update the return value counter
      n++;
		}
  }   // end while picks

  // STEP 3:   If we matched up atleast one pick with the modified
  //           origin, then re-locate the origin.
  if(n)
  {
    if(bLocate)
      Locate(pOrg->idOrigin, NULL);
    else
      Locate(pOrg->idOrigin, "TNEZ");
  }
	CDebug::Log(DEBUG_MAJOR_INFO, "Scavenge n=%d\n", n);


  // STEP 5:   Return the number of waif picks associated with the Origin.
	return n;
}  // end Scavenge(idOrg)
********************* END DK PULL from v1.0 ***************************************/


void Debug( char *flag, char *szFormat, ... )
{
	va_list ap;
  int     rc;

	va_start(ap,szFormat);
  rc = CDebug::LogVA(DEBUG_MAJOR_INFO, szFormat, ap);
  va_end(ap);
}

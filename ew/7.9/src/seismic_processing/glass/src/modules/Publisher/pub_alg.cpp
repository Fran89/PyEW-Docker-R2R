/******************************************************************
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pub_alg.cpp 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/24 15:04:30  paulf
 *     first inclusion
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.6  2005/02/28 18:25:45  davidk
 *     Added code to filter out spurious associated phases at time of publication.
 *     Added functions AlgPrepOriginForPublication() and ShouldPhaseBePruned()
 *     to prune out the spurious phases.  Code calls PrunePick() which eventually
 *     communicates to the associator module that the pick should be pruned
 *     from the origin.
 *
 *     Revision 1.5  2004/09/16 01:17:09  davidk
 *     Cleaned up logging messages.
 *     Downgraded some debugging messages.
 *     Set module to exit on all MAJOR_ERRORs.
 *
 *     Revision 1.4  2004/08/20 16:44:34  davidk
 *     Modified algorithm to read parameters from a config file instead of being hard coded.
 *
 *     Revision 1.3  2004/08/12 22:04:11  davidk
 *     Added an explicit double-to-int caste before printf'ing a value.
 *
 *     Revision 1.2  2004/08/06 15:00:52  davidk
 *     Changed the oldestEventToPublish to 7 days instead of 31.
 *
 *     Revision 1.1  2004/08/06 00:06:55  davidk
 *     Code that implements algorithm and logic for when to publish events.
 *     (Initial logic is hardcoded).
 *
 *
 *
 *******************************************************************/


/***************************************************************

Rules Section

MinNumPhases  10

# Event Death occurs after final Force tCreatedDelta
# has passed.

# OldestEventToPublish (in days)
OldestEventToPublish 7

xNumChanges   MinTimeBetweenPublications  PubDelay
   1                              600       10 
  10                              240       10
  20                              120        0
  40                                0        0

Force
tCreatedDelta         PubDelay  Min Phases
 1200                       0    20    # Report for MS, HMB 2/20/03
 2400                       0    20    # Report for MS, HMB 2/20/03
 3600                       0    20    # Report for MS, HMB 2/20/03
 4800                       0    20    # Report for MS, HMB 2/20/03
***************************************************************/

#include <stdlib.h>
#include <publish.h>
#include <debug.h>
#include <comfile.h>
#include <phase.h>
#include <PublisherMod.h>

typedef struct _ForcePubStruct
{
  time_t tDelta;
  time_t tDelay;
  int    iMinNumPhases;
} ForcePubStruct;

typedef struct _ChangePubStruct
{
  int    iNumChanges;
  time_t tDelay;
  time_t tMinPubInterval;
} ChangePubStruct;

static ForcePubStruct * FPSArray;
static ChangePubStruct * CPSArray;
static int iNumForcedPublications;
static int iNumChangedPublications;
static double dOldestEventToPublish;
static int iMinNumPhases;



//--------------------------------------------------------------------------------AlgInit
// Initialize algorithm parameters
int AlgInit()
{
  FPSArray = (ForcePubStruct *) calloc(1, nQuakeListSize * sizeof(ForcePubStruct));
  if(!FPSArray) 
    return(-1);

  iNumForcedPublications = 0;

  CPSArray = (ChangePubStruct *) calloc(1, nQuakeListSize * sizeof(ChangePubStruct));
  if(!CPSArray) 
    return(-1);

  iNumChangedPublications = 0;

  iMinNumPhases = 10;
  dOldestEventToPublish = 7.0; /* days */

  CDebug::Log(DEBUG_MAJOR_INFO, "Publishing Algorithm 08/18/2004 in effect.\n");


    return(0);
}  /* end AlgInit() */

//--------------------------------------------------------------------------------AlgReadParams
// Read parameter file
bool AlgReadParams(char * szFileName)
{
  CComFile cf;
	CStr cmd;
	int nc;
	bool b = true;
	int i;

	if(!cf.Open(szFileName))
  {
    CDebug::Log(DEBUG_MAJOR_ERROR,"Publisher::AlgReadParams():  Unable to open config file(%s).\n",
                szFileName);
    exit(-1);
		return false;
  }
	while(1) 
  {
		nc = cf.Read();

    /* break on error or EOF */
		if(nc < 0) 
			break;

    /* skip blank lines */
    if(nc < 1)
			continue;

    CDebug::Log(DEBUG_MAJOR_INFO, "AlgReadParams() New command <%s>\n", cf.Card().GetBuffer());

		cmd = cf.Token();

    /* skip strange blank tokens */
		if(cmd.GetLength() < 1)
			continue;

    /* skip comment lines starting with #*/
		if(cf.CurrentToken()[0] == '#')
			continue;

		if(cf.Is("ForcePub")) 
    {
      if(iNumForcedPublications >= nQuakeListSize)
      {
        CDebug::Log(DEBUG_MINOR_ERROR, "Too many ForcePub lines in Publisher config file. %d max!\n",
                    nQuakeListSize);
        continue;
      }
      
      FPSArray[iNumForcedPublications].tDelta = cf.Long();
      FPSArray[iNumForcedPublications].tDelay = cf.Long();
      FPSArray[iNumForcedPublications].iMinNumPhases = cf.Long();

      iNumForcedPublications++;
    }
		if(cf.Is("ChangePub")) 
    {
      if(iNumChangedPublications >= nQuakeListSize)
      {
        CDebug::Log(DEBUG_MINOR_ERROR, "Too many ChangePub lines in Publisher config file. %d max!\n",
                    nQuakeListSize);
        continue;
      }
      
      CPSArray[iNumChangedPublications].iNumChanges = cf.Long();
      CPSArray[iNumChangedPublications].tMinPubInterval = cf.Long();
      CPSArray[iNumChangedPublications].tDelay = cf.Long();

      iNumChangedPublications++;
    }
		if(cf.Is("MinNumPhases")) {
			iMinNumPhases = cf.Long();
			continue;
		}
		if(cf.Is("OldestEventToPublish")) {
			dOldestEventToPublish = cf.Double();
			continue;
		}
  }   // end while(true)
	cf.Close();

  CDebug::Log(DEBUG_MAJOR_INFO,
              "AlgReadParams():  Config file loaded!\n  PARAMS\n");
  CDebug::Log(DEBUG_MAJOR_INFO,
              "Force Publications:\n");
  for(i=0; i < iNumForcedPublications; i++)
    CDebug::Log(DEBUG_MAJOR_INFO,
                "\t %9d %9d %9d\n",
                FPSArray[i].tDelta,
                FPSArray[i].tDelay,
                FPSArray[i].iMinNumPhases);
  CDebug::Log(DEBUG_MAJOR_INFO,
              "\nChange Publications:\n");
  for(i=0; i < iNumChangedPublications; i++)
    CDebug::Log(DEBUG_MAJOR_INFO,
                "\t %9d %9d %9d\n",
                CPSArray[i].iNumChanges,
                CPSArray[i].tMinPubInterval,
                CPSArray[i].tDelay);

  CDebug::Log(DEBUG_MAJOR_INFO,
              "\nMin Phases to Publish: %d\n", iMinNumPhases);
  CDebug::Log(DEBUG_MAJOR_INFO,
              "Oldest Event to Publish: %.2f\n", dOldestEventToPublish);

	return true;
}  // end AlgReadParams()

int ShouldPhaseBePruned(PICK * pPick, int iPhaseCount, PhaseType ptPhaseType)
{
  PhaseClass pcPhase = GetPhaseClass(ptPhaseType);

  // all P-phases are good
  if(pcPhase == PHASECLASS_P)
    return(0);

  // all close in S-phases are good
  if(pcPhase == PHASECLASS_S && pPick->dDelta < 10.0)
    return(0);

  // all P-diff picks > 110.0 are bogus 
  if(ptPhaseType == PHASE_Pdiff && pPick->dDelta > 110.0)
    return(1);

  // require that all other picks come in sets of atleast three
  if(iPhaseCount < 3)
    return(1);

  return(0);
}


/* functions implemented by the publishing algorithm */
int AlgPrepOriginForPublication(PubQuake * pOrigin, CMod * IN_pMod)
{
  int * pPhaseCount;
	PICK *pPick;
	ORIGIN *pOrg;
	int res      = 0;
  size_t iPickRef = 0;
  int iNumPhaseTypes;
  PhaseType ptPhaseType;
  
  iNumPhaseTypes = GetNumberOfPhases();

  if(!(pPhaseCount = (int *) calloc(1, sizeof(int) * iNumPhaseTypes)))
  {
    CDebug::Log(DEBUG_MAJOR_ERROR,
                "AlgPrepOriginForPublication(): could not allocate %d bytes for PhaseCount array.  "
                "Returning Error!\n",
                sizeof(int) * iNumPhaseTypes);
    return(-1);
  }

	pOrg = pGlint->getOrigin(pOrigin->idOrigin);
  if(!pOrg)
  {
    CDebug::Log(DEBUG_MINOR_WARNING,
                "APOFP(): Origin (%s) not found in Glint.  Either Origin Deleted or ERROR!\n",
                pOrigin->idOrigin);
    res = 1;
    goto Cleanup;
  }

  while(pPick = pGlint->getPicksFromOrigin(pOrg,&iPickRef))
  {
    ptPhaseType = GetPhaseType(pPick->sPhase);
    if(ptPhaseType >= iNumPhaseTypes)
    {
	    CDebug::Log(DEBUG_MINOR_ERROR,
		              "APOFP(): Pick(%s) - illegal type (%d)\n",
		              pPick->idPick, ptPhaseType);
      continue;
    }
    pPhaseCount[ptPhaseType]++;
  }

  iPickRef=0;

  while(pPick = pGlint->getPicksFromOrigin(pOrg,&iPickRef))
  {
    ptPhaseType = GetPhaseType(pPick->sPhase);
    if(ShouldPhaseBePruned(pPick, pPhaseCount[ptPhaseType], ptPhaseType))
    {
	    CDebug::Log(DEBUG_MINOR_WARNING,
		              "APOFP(): Removing pick %s(%s %s %s %s) - %s - %t.\n",
		              pPick->idPick, pPick->sSite, pPick->sComp, pPick->sNet, pPick->sLoc,
                  pPick->sPhase, pPick->dT);
      IN_pMod->PrunePick(pOrg->idOrigin, pPick->idPick);
    }
  }

Cleanup:
  if(pPhaseCount)
  {
    free(pPhaseCount);
    pPhaseCount = NULL;
  }
  return(res);

}  // end AlgPrepOriginForPublication()



/* return codes:
     0 : SUCCESS, origin schedule updated
     1 : SUCCESS, origin schedule not-updated
    -1 : FAILURE
 **********************************************************/
int AlgOriginChanged(PubQuake * pOrigin, OriginChangeType ectChangeType)
{
  time_t tNow;
  time_t tNextDelay, tNextInterval, tNext;
  int i;
  

  time(&tNow);

  /* ignore dead Origins */
  if(pOrigin->bDead)
    return(1);

  /* Retrieve the origin params from Glint */
  ORIGIN * pOrg;
  pOrg = pGlint->getOrigin(pOrigin->idOrigin);
  if(!pOrg)
  {
    CDebug::Log(DEBUG_MINOR_WARNING,
                "Origin (%s) not found in Glint.  Either Origin Deleted or ERROR!\n",
                pOrigin->idOrigin);
    return(1);
  }

  /* ignore any origins older than OldestEventToPublish */
  if(dOldestEventToPublish)
  {
    if((tNow - pOrg->dT) > (dOldestEventToPublish * 3600 * 24))
    {
      /* This quake's too old.  Terminate it with extreme prejudice */
      CDebug::Log(DEBUG_MINOR_WARNING, 
                  "AlgOriginChanged():  Marking quake(%s/%d) as dead, Quake is too old(%d secs/ %d cutoff)!\n",
                  pOrigin->idOrigin, pOrigin->iOrigin,
                  (int)(tNow-pOrg->dT), (int)(dOldestEventToPublish * 3600 * 24));
      pOrigin->bDead = true;
      return(0);
    }
  }

  /* ignore any origins without the minimum number of phases */
  if (pOrg->nPh < iMinNumPhases)
    return(1);

  /* ignore any origins without the minimum number of changes */
  if (pOrigin->iNumChanges < CPSArray[0].iNumChanges)
    return(1);

  /* search backwards through the list, and find the most
     beneficial rule. */
  for(i=iNumChangedPublications-1; i >=0; i--)
  {
    if(pOrigin->iNumChanges > CPSArray[i].iNumChanges)
    {
      tNextDelay = tNow + CPSArray[i].tDelay;
      tNextInterval = pOrigin->tLastPub + CPSArray[i].tMinPubInterval;
      tNext = (tNextDelay > tNextInterval) ? tNextDelay : tNextInterval;
      if(!(pOrigin->tNextPub > 0 && pOrigin->tNextPub < tNext))
      {
        pOrigin->tNextPub = tNext;
        return(0);
      }
      else
      {
        return(1);
      }
    }
  }

  // we should never get here.  
  return(-1);
}

/* return codes:
     0 : SUCCESS, origin schedule updated
     1 : SUCCESS, origin schedule not-updated
    -1 : FAILURE
 **********************************************************/
int AlgOriginPublished(PubQuake * pOrigin)
{
  int i;
  time_t tNow;
  time_t tDelta;
  ORIGIN * pGlintOrigin;
  int      bKeepOriginAlive = false;
  time_t tNextPublish;

  time(&tNow);

  tDelta = tNow - pOrigin->tDeclare;

  pGlintOrigin = pGlint->getOrigin(pOrigin->idOrigin);
  if(!pGlintOrigin)
    return(-1);
  
  for(i=0; i < iNumForcedPublications; i++)
  {
    if(tDelta < FPSArray[i].tDelta)
    {
      bKeepOriginAlive = true;
      if(pGlintOrigin->nPh >= FPSArray[i].iMinNumPhases)
      {
        tNextPublish = pOrigin->tDeclare + FPSArray[i].tDelta
                       + FPSArray[i].tDelay;
        if(tNextPublish < pOrigin->tNextPub || pOrigin->tNextPub <= 0)
        {
          pOrigin->tNextPub = tNextPublish;
          return(0);
        }
        else
        {
          return(1);
        }
      }  /* end if this origin has enough phases to meet the current force rule. */
    }    /* end if the current force rule applies */
  }  /* end for each force rule in the list */

  if(!bKeepOriginAlive)
  {
    CDebug::Log(DEBUG_MAJOR_INFO, 
                "AlgOriginPublished():  Marking quake(%s/%d) as dead,"
                " Quake is older than last required publication. Age:%d secs.\n",
                pOrigin->idOrigin, pOrigin->iOrigin, tDelta
                );
    pOrigin->bDead = true;
    return(0);
  }
  else
  {
    return(1);
  }
}   /* end AlgOriginPublished() */


int AlgExamineOrigin(PubQuake * pOrigin)
{
  time_t tNow;

  time(&tNow);

  if((pOrigin->tNextPub <= 0) &&
     (tNow - pOrigin->tDeclare > FPSArray[iNumForcedPublications - 1].tDelta)
    )
  {
    CDebug::Log(DEBUG_MAJOR_INFO, 
                "AlgExamineOrigin():  Marking quake(%s/%d) as dead,"
                " Quake is older(%d) than last required publication(%d.\n",
                pOrigin->idOrigin, pOrigin->iOrigin, (int)(tNow - pOrigin->tDeclare), 
                FPSArray[iNumForcedPublications - 1].tDelta
                );
    pOrigin->bDead = true;
    return(0);
  }
  else
  {
    return(1);
  }
}  /* end AlgExamineOrigin() */





/******************************************************************
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: publish.cpp 3212 2007-12-27 18:05:23Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2007/12/27 18:05:23  paulf
 *     sync from hydra_proj circa Oct 2007
 *
 *     Revision 1.3  2006/09/30 05:41:52  davidk
 *     Added dZ(event depth) to the quake info tracked by publisher.
 *
 *     Revision 1.2  2006/04/22 01:14:54  davidk
 *     Downgraded the severity on "unknown idOrigin" error in the publisher,
 *     as it is not a serious error as once thought, and sitescope is sending it out
 *     as fatal in v1.43.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.5  2005/02/28 18:27:22  davidk
 *     Added code to filter out spurious associated phases at time of publication.
 *     Modified call to AlgPrepOriginForPublication() to include a pointer to the
 *     publishing module, so that the Prep funciton can call the module's
 *     PrunePick() which eventually communicates to the associator module
 *     that the pick should be pruned
 *     from the origin.
 *
 *     Revision 1.4  2005/02/15 21:55:59  davidk
 *     Added Lat/Lon/Time to the PubQuake struct that describes quakes
 *     elligible to be published, in order to debug the publishing mechanism.
 *     Now when a quake is deleted, a record of it is still kept in the publishing
 *     display.
 *
 *     Revision 1.3  2004/11/02 19:07:26  davidk
 *     Added code to mark any quakes as dead that appear to be deleted(no info in glint)
 *     when they attempt to publish.
 *     Addresses a problem where because of a glass::delete() bug, the publisher was
 *     filling up the log file with <can't publish deleted quake> messages.
 *     Once the quake is gone, it's not coming back, so this is just a redundant step
 *     to delete (deleted) quakes.  An error is still issued, but this keeps the
 *     log file from becoming unmanagable.
 *
 *     Revision 1.2  2004/09/16 01:16:05  davidk
 *     Cleaned up logging messages.
 *     Downgraded some debugging messages.
 *     Set module to exit on all MAJOR_ERRORs.
 *
 *     Revision 1.1  2004/08/06 00:07:51  davidk
 *     Code that controls the publishing mechanism.  Handles Origin change notifications,
 *     and tracks publication.
 *
 *
 *
 *******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include "publish.h"
#include <PublisherMod.h>
#include <windows.h>

/* global vars */
PubQuake * QList;
int iOldestQuake;
int iYoungestQuake;
int nQuakeListSize;
time_t tLastChange;
IGlint *pGlint;
int iLastOriginChanged;

static CMod * pMod;
static int bInitialized=false;

/* funcitons implemented by publish.c */

int init(CMod * IN_pMod)
{
  nQuakeListSize = 100;

  QList = (PubQuake *)calloc(1, nQuakeListSize * sizeof(PubQuake));
  if(!QList)
  {
    CDebug::Log(DEBUG_MAJOR_ERROR, "Publisher:  Could not allocate %d bytes for %d Quake List elements.\n",
                nQuakeListSize * sizeof(PubQuake), nQuakeListSize);
    exit(-1);
    return(-1);
  }

  iOldestQuake  = 0;
  iYoungestQuake = -1;

  pMod = IN_pMod;
  pGlint = pMod->pGlint;

  bInitialized = true;
  tLastChange = 0;
  iLastOriginChanged = -1;

  AlgInit();

  return(0);
}

int MakeSpaceInQList()
{
  int iMove;

  /* if there are some dead quakes we can throw off the
     end of the list, then do it.
     Otherwise throw the oldest live quake off the end of 
     the list.
   *****************************************************/
  if(iOldestQuake > 0)
   iMove = iOldestQuake - 0;
  else
  {
   iMove = 1;
   iOldestQuake++;
  }

  memmove(&QList[0], &QList[iOldestQuake], 
          (nQuakeListSize - iOldestQuake) * sizeof(PubQuake));

  /* decrement the youngest by number of quakes dropped (moved) */
  iYoungestQuake -= iMove;

  /* set the oldest back to 0 */
  iOldestQuake = 0;

  /* in case we deleted a live quake, and there are for some reason
     dead quakes behind it, move the OldestQuake marker, so that
     they will be deleted the next time through
   *****************************************************/
  for(int i=0; i <= iYoungestQuake; i++)
  {
    if(QList[i].bDead)
      iOldestQuake++;
    else 
      break;
  }

  return(0);
}  /* end MakeSpaceInQList() */


int GetIndexOfNewQuake()
{
  if(iYoungestQuake == (nQuakeListSize - 1))
    MakeSpaceInQList();

  iYoungestQuake++;
  memset(&QList[iYoungestQuake], 0, sizeof(PubQuake));
  return(iYoungestQuake);
}  /* end GetIndexOfNewQuake() */


int GetIndexOfOrigin(char * idOrigin)
{
  int i;
  for(i = iYoungestQuake; i >= iOldestQuake; i--)
  {
    if(strcmp(QList[i].idOrigin, idOrigin) == 0)
      return(i);
  }
  return(-1);
}  /* end GetIndexOfNewQuake() */


int OriginChanged(char * idOrigin, OriginChangeType ectChangeType)
{
  int iQuake;
  ORIGIN * pOrg;

  time(&tLastChange);
  iQuake = GetIndexOfOrigin(idOrigin);
  if(iQuake < 0)
  {
    /* Unknown Quake */
    if(ectChangeType == ORIGIN_CREATED)
    {
      /* new quake - create an entry for it*/
      iQuake = GetIndexOfNewQuake();
      if(iQuake < 0)
      {
        CDebug::Log(DEBUG_MINOR_ERROR, "GetIndexOfNewQuake() returned error(%d).  Serious! Aborting!\n",
                    iQuake);
        return(-1);
      }
      /* Create the Quake */
      memset(&QList[iQuake], 0, sizeof(QList[iQuake]));
      strncpy(QList[iQuake].idOrigin, idOrigin, sizeof(QList[iQuake].idOrigin)-1);
      time(&QList[iQuake].tDeclare);

      /* Get the Origin info from Glint */
      if((pOrg = pGlint->getOrigin(idOrigin))==NULL)
      {
        CDebug::Log(DEBUG_MINOR_ERROR,"OriginChanged(): Could not obtain "
                    "origin for id(%s)\n", idOrigin);
        return(1);
      }
      
      QList[iQuake].iOrigin = pOrg->iOrigin;
      QList[iQuake].dLat = (float)pOrg->dLat;
      QList[iQuake].dLon = (float)pOrg->dLon;
      QList[iQuake].tOrigin = (float)pOrg->dT;
      QList[iQuake].dZ = (float)pOrg->dZ;

      AlgOriginChanged(&QList[iQuake], ectChangeType);
      time(&tLastChange);
      iLastOriginChanged = iQuake;

    }
    else
    {
      CDebug::Log(DEBUG_MINOR_WARNING, "OriginChanged(): Unknown idOrigin (%d)(%d).\n",
                  idOrigin, (int)ectChangeType);
      return(1);
    }
  }  /* end if quake not found */
  else
  {
    time(&tLastChange);
    iLastOriginChanged = iQuake;
    QList[iQuake].iNumChanges++;

    // ignore dead quakes
    if(QList[iQuake].bDead)
      return(0);

    if(ectChangeType == ORIGIN_DELETED)
    {
      CDebug::Log(DEBUG_MAJOR_INFO, 
                  "OriginChanged():  Marking quake(%s/%d/%d) as dead, got Delete Quake message.\n",
                  idOrigin, iQuake, QList[iQuake].iOrigin);
      QList[iQuake].bDead = true;
      if(QList[iQuake].bPublished)
        pMod->Retract(QList[iQuake].idOrigin, QList[iQuake].iOrigin);
    }
    else
    {
      /* Get the Origin info from Glint */
      if((pOrg = pGlint->getOrigin(idOrigin))==NULL)
      {
        CDebug::Log(DEBUG_MINOR_ERROR,"OriginChanged(): Could not obtain "
                    "origin for id(%s)\n", idOrigin);
        return(1);
      }
      
      QList[iQuake].dLat = (float)pOrg->dLat;
      QList[iQuake].dLon = (float)pOrg->dLon;
      QList[iQuake].tOrigin = (float)pOrg->dT;
      AlgOriginChanged(&QList[iQuake], ectChangeType);
    }
  }

  return(0);
}  /* end OriginChanged() */

  /* this is where we go through the list of events, and determine if any 
     need to be published 
   ***********************************************************************/
int Process()
{
  int i;
  time_t tNow;

  if(!bInitialized)
    return(0);


  time(&tNow);

  for(i=iOldestQuake; i <= iYoungestQuake; i++)
  {
    // Ignore dead quakes
    if(QList[i].bDead)
      continue;

    if(QList[i].tNextPub > 0 &&  QList[i].tNextPub < tNow)
    {
      PublishOrigin(&QList[i]);
      time(&tLastChange);
      iLastOriginChanged = i;
    }
    if(AlgExamineOrigin(&QList[i]) == 0)
    {
      time(&tLastChange);
      iLastOriginChanged = i;
    }
  }
  return(0);
}

int PublishOrigin(PubQuake * pOrigin)
{
  time_t tNow;

  AlgPrepOriginForPublication(pOrigin,pMod);

  time(&tNow);
  {
    ORIGIN * pOrg;
    pOrg = pGlint->getOrigin(pOrigin->idOrigin);
    if(pOrg)
      CDebug::Log(DEBUG_MINOR_INFO, 
                  "Publishing Event %d with %d phases at time %s\n",
                  pOrg->iOrigin, pOrg->nPh, ctime(&tNow));
    else
    {
      CDebug::Log(DEBUG_MINOR_ERROR, 
                  "Attempting to publish (dead?(%d)) Origin(%s)  at time %s. Ignoring pub request.\n"
                  "  Marking quake as dead!\n",
                  pOrigin->bDead, pOrigin->idOrigin, ctime(&tNow));
      OriginChanged(pOrigin->idOrigin, ORIGIN_DELETED);
      return(1);
    }
  }

  pMod->Publish(pOrigin->idOrigin, pOrigin->iNumPub+1);

  /* reset the PubQuake params */
  pOrigin->tLastPub = tNow;
  pOrigin->tNextPub = 0;
  pOrigin->iNumPub++;
  pOrigin->iNumChanges = 0;
  if(!pOrigin->bPublished)
    pOrigin->bPublished = true;

  AlgOriginPublished(pOrigin);
  return(0);
}





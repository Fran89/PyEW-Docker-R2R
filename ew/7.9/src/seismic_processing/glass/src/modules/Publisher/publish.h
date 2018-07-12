/******************************************************************
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: publish.h 3212 2007-12-27 18:05:23Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2007/12/27 18:05:23  paulf
 *     sync from hydra_proj circa Oct 2007
 *
 *     Revision 1.2  2006/09/30 05:41:34  davidk
 *     Added dZ(event depth) to the quake info tracked by publisher.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.4  2005/02/28 18:28:00  davidk
 *     Added code to filter out spurious associated phases at time of publication.
 *     Modified call to AlgPrepOriginForPublication() to include a pointer to the
 *     publishing module, so that the Prep funciton can call the module's
 *     PrunePick() which eventually communicates to the associator module
 *     that the pick should be pruned
 *     from the origin.
 *
 *     Revision 1.3  2005/02/15 21:31:10  davidk
 *     Added Lat/Lon/Time to the PubQuake struct that describes quakes
 *     elligible to be published, in order to debug the publishing mechanism.
 *     Now when a quake is deleted, a record of it is still kept in the publishing
 *     display.
 *
 *     Revision 1.2  2004/08/20 16:43:47  davidk
 *     Added AlgReadParams() function.
 *
 *     Revision 1.1  2004/08/06 00:07:51  davidk
 *     Code that controls the publishing mechanism.  Handles Origin change notifications,
 *     and tracks publication.
 *
 *
 *
 *******************************************************************/

#ifndef PUBLISH_H
# define PUBLISH_H

#include <time.h>
#include <IGlint.h>

typedef enum _OriginChangeType
{
  ORIGIN_CREATED,
  ORIGIN_DELETED,
  ORIGIN_PHASE_LINKED,
  ORIGIN_PHASE_UNLINKED,
  ORIGIN_RELOCATED
}  OriginChangeType;


typedef struct _PubQuake
{
   char       idOrigin[32];
   int        iOrigin;      /* this is somewhat redundant, but we
                               need it for deletions */
   int        bPublished;
   int        bDead;
   time_t     tDeclare;
   time_t     tLastPub;
   time_t     tNextPub;
   int        iNumChanges;  /* since last publication */
   int        iNumPub;      /* # of times published */
   float      dLat;
   float      dLon;
   float      dZ;
   double     tOrigin;
} PubQuake;

class CMod;


/* functions implemented by the publishing algorithm */
int AlgInit();
int AlgPrepOriginForPublication(PubQuake * pOrigin, CMod * IN_pMod);
int AlgOriginChanged(PubQuake * pOrigin, OriginChangeType ectChangeType);
int AlgOriginPublished(PubQuake * pOrigin);
int AlgExamineOrigin(PubQuake * pOrigin);
bool AlgReadParams(char * szFileName);

/* funcitons implemented by publish.c */
int init(CMod * pMod);
int OriginChanged(char * idOrigin, OriginChangeType ectChangeType);
int Process();
int PublishOrigin(PubQuake *pOrigin);

extern PubQuake * QList;
extern int iOldestQuake;
extern int iYoungestQuake;
extern int nQuakeListSize;
extern time_t tLastChange;
extern IGlint *pGlint;
extern int iLastOriginChanged;
#endif /* PUBLISH_H */

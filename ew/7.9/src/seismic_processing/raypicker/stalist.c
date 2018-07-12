/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: stalist.c,v 1.5 2010/08/31 18:37:44 davidk Exp $
 *
 *    Revision history:
 *     $Log: stalist.c,v $
 *     Revision 1.5  2010/08/31 18:37:44  davidk
 *     Issue an error message (via a RING, destined for statmgr), whenever the
 *     picker station-list shrinks by more than 25%.
 *     Done to add end monitoring of picker station-list size since it's automatically
 *     updated without any human review.
 *
 *     Revision 1.4  2009/02/16 17:26:57  mark
 *     Bug fixing (need qsort before adding new channels)
 *
 *     Revision 1.3  2009/02/13 20:40:36  mark
 *     Added SCNL parsing library
 *
 *     Revision 1.2  2008/06/02 20:12:51  mark
 *     Bug fixing; minor tweaks
 *
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 */

#include "raypicker.h"
#include "watchdog_client.h"
#include "earthworm_defs.h"
#include "compare.h"
#include "pick_channel_info.h"
/*#include "hydra_utils.h"*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
   char sta[TRACE2_STA_LEN+1];
   char comp[TRACE2_CHAN_LEN+1];
   char net[TRACE2_NET_LEN+1];
   char loc[TRACE2_LOC_LEN+1];
} SCNL;

typedef struct
{
   SCNL * List;
   int    Count;
   int    numAlloced;
} SCNLList;

/* extern func proto */
int ParsePickerList(char *szFile, SCNL *pSCNL, int SCNLSize, int *pNumSCNL);

extern EWParameters    EwParameters;  /* structure containing ew-type parameters */

/**
  * Reads in the station list from $(EW_PARAMS)\stalist.picker.  Copies all the SCNLs to a new SCNL list.
  *
  * Entry: hNewSCNL - pointer to where new SCNL list pointer will live
  * Exit:  hNewSCNL - pointer to newly-allocated SCNL list.  The caller is responsible for freeing this!
  *        pSCNLSize - pointer to number of SCNL entries in *hNewSCNL
  * Returns: EW_FAILURE if something goes wrong, else EW_SUCCESS
  */
int ReadStationList(char *szSCNLFile, RaypickerSCNL **hNewSCNL, int *pSCNLSize)
{
    SCNL *pJustSCNL;
    int allocSCNL = 5000;
    int i, retval;

    do
    {
        pJustSCNL = (SCNL *)calloc(allocSCNL, sizeof(SCNL));
        retval = ParsePickerList(szSCNLFile, pJustSCNL, allocSCNL, pSCNLSize);
        if (retval == EW_FAILURE)
        {
            free(pJustSCNL);
            return EW_FAILURE;
        }
        else if (retval == EW_WARNING)
        {
            // Not enough memory allocated.  Try doubling it...
            free(pJustSCNL);
            allocSCNL *= 2;
        }
    } while (retval == EW_WARNING);

    // Copy the "standard" SCNL structs to our raypicker-exclusive versions
    *hNewSCNL = (RaypickerSCNL *)calloc((*pSCNLSize) + 1, sizeof(RaypickerSCNL));
    for (i = 0; i < *pSCNLSize; i++)
    {
        strcpy(((*hNewSCNL)[i]).sta, pJustSCNL[i].sta);
        strcpy(((*hNewSCNL)[i]).chan, pJustSCNL[i].comp);
        strcpy(((*hNewSCNL)[i]).net, pJustSCNL[i].net);
        strcpy(((*hNewSCNL)[i]).loc, pJustSCNL[i].loc);
    }

    free(pJustSCNL);
    return retval;
}

/**
  * Compares two station lists for differences.
  *
  * Entry: pNewSCNL - pointer to the new SCNL list
  *        NewSCNLSize - number of entries in pNewSCNL
  *        pOldSCNL - pointer to the old SCNL list
  *        OldSCNLSize - number of entries in pOldSCNL
  * Returns: EW_FAILURE if something goes wrong,
  *          EW_WARNING if there are no differences between the lists,
  *          EW_SUCCESS if the lists differ
  */
int CompareStationLists(RaypickerSCNL *pNewSCNL, int NewSCNLSize, RaypickerSCNL *pOldSCNL, int OldSCNLSize)
{
    RaypickerSCNL *thisSCNL;
    int i, retval = EW_WARNING;

    // Handle the obvious case first - if the sizes of the lists aren't identical, they can't
    // very well be the same, now can they?
    if (NewSCNLSize != OldSCNLSize)
    {
        reportError(WD_INFO, 0, "Sizes of SCNL lists differ (%d in new, %d in old)\n",
                    NewSCNLSize, OldSCNLSize);
    if(NewSCNLSize < 0.75 * OldSCNLSize)
    {
      char szError[1024];
      reportError(WD_FATAL_ERROR, 0, "New Picker Station List is MUCH SMALLER than old list!!!  THIS COULD BE FATAL FOR EARTHQUAKE MONITORING!!!"
        "(%d in new, %d in old)!\n",
        NewSCNLSize, OldSCNLSize);
      sprintf(szError,
        "New Picker Station List is MUCH SMALLER than old list!!!  THIS COULD BE FATAL FOR EARTHQUAKE MONITORING!!!"
        "(%d in new, %d in old)!\n",
        NewSCNLSize, OldSCNLSize);
      WriteError(EwParameters, params, myPid, szError);
    }
    return EW_SUCCESS;
    }

    // Now, do a compare of each station.
    for (i = 0; i < NewSCNLSize; i++)
    {
        // We shouldn't have to worry about locking mutexes, since we're the only thread capable of
        // re-ordering this list...
        thisSCNL = (RaypickerSCNL *)bsearch(&(pNewSCNL[i]), pOldSCNL, OldSCNLSize, sizeof(RaypickerSCNL), CompareSCNLs);
        if (thisSCNL == NULL)
        {
            // Found one in the new list that's not in the old.  Return now.
            reportError(WD_INFO, 0, "Found SCNL <%s><%s><%s><%s> in new list but not in old\n",
                        pNewSCNL[i].sta, pNewSCNL[i].chan, pNewSCNL[i].net, pNewSCNL[i].loc);
            return EW_SUCCESS;
        }
    }

    // If we get this far, there's no differences between the lists.
    return retval;
}

/**
  * Copies the station list to its new home.  Also sorts the new list for use with bsearch.
  * NOTE: New SCNL entries are initialized with new memory (pchanInfo, rawBuffer, etc.)
  *       Old SCNL entries only copy the pointers to channel infos (and associated buffers and filters) to the
  *         new list.  Do not free these pointers from the old list!
  *
  * Entry: pNewSCNL - pointer to the new SCNL list
  *        NewSCNLSize - number of entries in pNewSCNL
  *        pOldSCNL - pointer to the old SCNL list
  *        OldSCNLSize - number of entries in pOldSCNL
  * Exit:  pNewSCNL - contains copied data from pOldSCNL
  * Returns: EW_FAILURE if something goes wrong, otherwise EW_SUCCESS
  */
int CopyStationList(RaypickerSCNL *pNewSCNL, int NewSCNLSize, RaypickerSCNL *pOldSCNL, int OldSCNLSize, double MaxSampleRate)
{
    RaypickerSCNL *thisSCNL;
    int i, numNew, numMoved, numDel;

    // Sort out the new list before monkeying around with it.
    qsort(pNewSCNL, NewSCNLSize, sizeof(RaypickerSCNL), CompareSCNLs);

    // Step through each entry in the new list.
    numNew = numMoved = numDel = 0;
    for (i = 0; i < NewSCNLSize; i++)
    {
        // Find this entry in the old list.
        // We shouldn't have to worry about locking mutexes, since we're the only thread capable of
        // re-ordering this list...
        thisSCNL = (RaypickerSCNL *)bsearch(&(pNewSCNL[i]), pOldSCNL, OldSCNLSize, sizeof(RaypickerSCNL), CompareSCNLs);
        if (thisSCNL == NULL)
        {
            // Found one in the new list that's not in the old list.  Allocate and initialize this one.
            reportError(WD_DEBUG, 0, "Found new SCNL: <%s><%s><%s><%s>\n", pNewSCNL[i].sta,
                        pNewSCNL[i].chan, pNewSCNL[i].net, pNewSCNL[i].loc);

            if ((pNewSCNL[i].pchanInfo = (PICK_CHANNEL_INFO *)malloc(sizeof(PICK_CHANNEL_INFO))) == NULL)
            {
                reportError(WD_FATAL_ERROR, MEMALLOC, "raypicker: cannot allocate space for PICK_CHANNEL_INFO for <%s:%s:%s:%s>\n",
                            pNewSCNL[i].sta, pNewSCNL[i].chan, pNewSCNL[i].net, pNewSCNL[i].loc);
                return EW_FAILURE;
            }
            if (InitChannelInfo(pNewSCNL[i].pchanInfo, MaxSampleRate) != EW_SUCCESS)
            {
                reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: InitChannelInfo unsuccessful for <%s:%s;%s;%s>\n",
                            pNewSCNL[i].sta, pNewSCNL[i].chan, pNewSCNL[i].net, pNewSCNL[i].loc);
                return EW_FAILURE;
            }
            numNew++;
        }
        else
        {
            // We already know about this entry.  Copy all its data to the new entry.
            // (This just copies pointers, not actual data...)
#ifdef _WINNT
            memcpy_s(&(pNewSCNL[i]), sizeof(RaypickerSCNL), thisSCNL, sizeof(RaypickerSCNL));
#else
            memcpy(&(pNewSCNL[i]), thisSCNL, sizeof(RaypickerSCNL));
#endif
            reportError(WD_DEBUG, 0, "Moved station <%s><%s><%s><%s>, oldBase 0x%08X, newBase 0x%08X, oldBuf 0x%08X, newBuf 0x%08X\n",
                    thisSCNL->sta, thisSCNL->chan, thisSCNL->net, thisSCNL->loc,
                    thisSCNL, &(pNewSCNL[i]),
                    thisSCNL->pchanInfo->rawBuffer, pNewSCNL[i].pchanInfo->rawBuffer);
            reportError(WD_DEBUG, 0, "\tOld sampling rate %f; new rate %f\n",
                    thisSCNL->pchanInfo->sampleRate, pNewSCNL[i].pchanInfo->sampleRate);
            numMoved++;
        }
    }

    // On to the next bit - see if there's any in the old list that aren't in the new list.
    for (i = 0; i < OldSCNLSize; i++)
    {
        thisSCNL = (RaypickerSCNL *)bsearch(&(pOldSCNL[i]), pNewSCNL, NewSCNLSize, sizeof(RaypickerSCNL), CompareSCNLs);
        if (thisSCNL == NULL)
        {
            // Not much we can do now, since other threads could be accessing this memory.
            // Mark this SCNL for deletion once the swap is made.
            reportError(WD_DEBUG, 0, "Found SCNL to delete: <%s><%s><%s><%s>\n", pOldSCNL[i].sta,
                        pOldSCNL[i].chan, pOldSCNL[i].net, pOldSCNL[i].loc);
            pOldSCNL[i].bDeleteMe = 1;
            numDel++;
        }
    }
    reportError(WD_INFO, 0, "Found %d new stations; moved %d existing stations; marked %d stations for deletion\n",
                numNew, numMoved, numDel);

    // Sort out the new list to make future bsearches possible.
    qsort(pNewSCNL, NewSCNLSize, sizeof(RaypickerSCNL), CompareSCNLs);

    // All is well...
    return EW_SUCCESS;
}

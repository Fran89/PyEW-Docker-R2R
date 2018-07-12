/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: scnl_list_utils.c,v 1.2 2005/12/16 19:14:10 davidk Exp $
 *
 *    Revision history:
 *     $Log: scnl_list_utils.c,v $
 *     Revision 1.2  2005/12/16 19:14:10  davidk
 *     swapped ioc_defs.h include for hydra_defs.h.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:24  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.6  2004/09/22 16:14:15  michelle
 *     moved DoSCNLsMatch from scnl_match.c into here so all functions are in one place
 *
 *     Revision 1.5  2004/09/22 01:02:21  davidk
 *     Added qsort() type function Compare_SCNLs().
 *     This function incorporates support  for '*' wildcards for S, C, N, or L.
 *     It sorts via SNCL order.
 *     It equates a blank Loc code to "--".
 *
 *     Modified FindSCNLInList() to utilize Compare_SCNLs()
 *     instead of DoSCNLsMatchExact(), thus making it work with wildcards.
 *     Modified DeleteSCNLsFromList() to use FindSCNLInList() instead
 *     of FindSCNLInListWithWildCard().
 *
 *     Revision 1.4  2004/09/21 23:23:24  michelle
 *     added a return for delete and find functions of WARNING_NOT_FOUND
 *     and returns MODULE_RET_ERROR (or -1) for true errors
 *
 *     Revision 1.3  2004/09/20 20:19:28  davidk
 *     Addid strib() function for stripping blanks out of SCNL strings.
 *     Reorganized #include statements.
 *
 *     Revision 1.2  2004/09/07 23:00:48  michelle
 *     move #include scnl_match.h to my .h
 *
 *     Revision 1.1  2004/09/04 01:34:07  michelle
 *     utility functions to support a list of SCNL structs (originally buit to support AddSCNL, DelSCNL passport )
 *
 *     Revision 1.2  2004/04/14 21:58:38  davidk
 *     Added a newline char at the end of the file to get rid of warning msgs.
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:19  michelle
 *     New Hydra Import
 *
 *     Revision 1.1  2003/06/26 13:45:31  dhanych
 *     Initial revision
 *
 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include <watchdog_client.h>
#include <hydra_defs.h>
#include "scnl_list_utils.h"
//#include <scnl_match.h>

#define cWILDCARD '*'
#define szNO_LOC_CODE "--"
int Compare_SCNLs(void * p1, void * p2);


/***
 * Checks if SCNLs match exactly (no wild cards taken into account)
 * for example * BHZ * == * BHZ * returns 1 for a match
 * but * BHZ * == * BHZ IU returns zero as the netword codes are not the same characters
 * like wise ACSO BHZ UI == ACSO HHZ UI returns 0 for NO match
 * returns 1 if sncls are an exact match
 * returns 0 if sncls are NOT an exact match
 * returns -1 if invalid parameters
 */

int DoSCNLsMatchExact( SCNL *p_scnlA, SCNL *p_scnlB)
{
    if (p_scnlA == NULL || p_scnlB == NULL )
    {
        // invalid input params
        return MODULE_RET_ERROR;
    }

   if (   p_scnlA->sta == NULL
       || p_scnlA->comp == NULL
       || p_scnlA->net == NULL
       || p_scnlA->loc == NULL
       || p_scnlB->sta == NULL
       || p_scnlB->comp == NULL
       || p_scnlB->net == NULL
       || p_scnlB->loc == NULL
      )
   {
      //invalid params, return error
       reportError(WD_WARNING_ERROR, SYSERR, "invalid NULL input params.\n");
      return MODULE_RET_ERROR;
   }

   if ( strcmp(p_scnlA->sta, p_scnlB->sta) == 0 &&
        strcmp(p_scnlA->comp, p_scnlB->comp) == 0 &&
        strcmp(p_scnlA->net, p_scnlB->net) == 0 &&
        strcmp(p_scnlA->loc, p_scnlB->loc) == 0 )
   {
       // exact match, return true
       return 1;
   }

   // NO match, return false
   return 0;
}

int DoSCNLsMatchWithWildCard( SCNL *p_scnlA, SCNL *p_scnlB, const char * p_wildCard)
{
    if (p_scnlA == NULL || p_scnlB == NULL || p_wildCard == NULL)
    {
        // invalid input params
        reportError(WD_WARNING_ERROR, SYSERR, "invalid NULL input params.\n");
        return MODULE_RET_ERROR;
    }

    return DoSCNLsMatch(p_scnlA->sta, p_scnlA->comp, p_scnlA->net, p_scnlA->loc,
                        p_scnlB->sta, p_scnlB->comp, p_scnlB->net, p_scnlB->loc,
                        p_wildCard);
}

/**
 *  copied from scnl_match to have all the functions in one place
 *  see header file for return codes
 */
int DoSCNLsMatch( const char * p_staA
                , const char * p_chaA
                , const char * p_netA
                , const char * p_locA
                , const char * p_staB
                , const char * p_chaB
                , const char * p_netB
                , const char * p_locB
                , const char * p_wild
                )
{
   if (   p_staA == NULL
       || p_chaA == NULL
       || p_netA == NULL
       || p_locA == NULL
       || p_staB == NULL
       || p_chaB == NULL
       || p_netB == NULL
       || p_locB == NULL
      )
   {
      return -1;
   }

   if (    strcmp( p_staA , p_staB ) == 0
       || (    p_wild != NULL
           && (   strcmp( p_staA , p_wild ) == 0
               || strcmp( p_staB , p_wild ) == 0
              )
          )
      )
   {
      if (    strcmp( p_chaA , p_chaB ) == 0
          || (    p_wild != NULL
              && (   strcmp( p_chaA , p_wild ) == 0
                  || strcmp( p_chaB , p_wild ) == 0
                 )
             )
         )
      {
         if (    strcmp( p_netA , p_netB ) == 0
             || (    p_wild != NULL
                 && (   strcmp( p_netA , p_wild ) == 0
                     || strcmp( p_netB , p_wild ) == 0
                    )
                )
            )
         {
            if (    strcmp( p_locA , p_locB ) == 0
                || (    p_wild != NULL
                    && (   strcmp( p_locA , p_wild ) == 0
                        || strcmp( p_locB , p_wild ) == 0
                       )
                   )
               )
            {
               return 1; //match
            }
         }
      }
   }
   return 0; // no match
}

/**
 * returns array index of SCNL's position in the list if it is found on exact match
 * returns WARNING_NOT_FOUND if exact match not found
 * returns MODULE_RET_ERROR if error encountered
 */
int FindSCNLInList(SCNLList *pList, SCNL *p_scnl)
{
    int indexInList = WARNING_NOT_FOUND;
    int i = 0;

    if (pList == NULL || p_scnl == NULL )
    {
        // invalid input params
        reportError(WD_WARNING_ERROR, SYSERR, "invalid NULL input params.\n");
        return MODULE_RET_ERROR;
    }

    for(i=0; i < pList->Count; i++)
    {

        if(!Compare_SCNLs((void *)p_scnl, (void *)&pList->List[i]) )
        {
                indexInList = i;
                break;
        }

    } // end for loop

    return indexInList;
}

/**
 * returns the first found array index of SCNL's position in the list
 *         if it is found on wild card match
 * returns MODULE_RET_ERROR -1  if error encountered
 * returns WARNING_NOT_FOUND (-2) if no wild card match found
 */
int FindSCNLInListWithWildCard(SCNLList *pList, int startAt, SCNL *p_scnl, const char *p_wildCard)
{
    int indexInList = WARNING_NOT_FOUND;
    int i = 0;

    if (pList == NULL || p_scnl == NULL || p_wildCard == NULL)
    {
        // invalid input params
        reportError(WD_WARNING_ERROR, SYSERR, "invalid NULL input params.\n");
        return MODULE_RET_ERROR;
    }

    for(i = startAt; i < pList->Count; i++)
    {

        if(DoSCNLsMatchWithWildCard(p_scnl, &pList->List[i], p_wildCard) == 1)
        {
                indexInList = i;
                reportError(WD_DEBUG, WD_DEBUG, "FindSCNLInListWithWildCard found at %d.\n", indexInList);
                break;
        }

    } // end for loop

    return indexInList;
}

/**
 * delete SCNL at array index scnlIndex
 */
int DeleteSCNLFromListByIndex(SCNLList *pList, int scnlIndex)
{
    int retIndex = WARNING_NOT_FOUND;
    int i = 0;

    if (pList == NULL )
    {
        // invalid input params
        reportError(WD_WARNING_ERROR, SYSERR, "invalid NULL input params.\n");
        return MODULE_RET_ERROR;
    }

    //make sure scnlIndex is a valid arry index, if so then delete
    if(scnlIndex > -1  && scnlIndex < pList->Count)
    {
        for (i=scnlIndex; i < (pList->Count - 1); i++)
        {
                //remove this SCNL from the list by
                //shifting everything after it back one spot in the array
                strcpy(pList->List[i].sta, pList->List[i+1].sta);
                strcpy(pList->List[i].comp, pList->List[i+1].comp);
                strcpy(pList->List[i].net, pList->List[i+1].net);
                strcpy(pList->List[i].loc, pList->List[i+1].loc);
        }

        // empty out the last element in the list
        strcpy(pList->List[pList->Count].sta, "");
        strcpy(pList->List[pList->Count].comp, "");
        strcpy(pList->List[pList->Count].net, "");
        strcpy(pList->List[pList->Count].loc, "");

        //decrement list count
        pList->Count--;
        retIndex = scnlIndex;

    } // end if indexInList

    return retIndex;
}

/**
 * delete a SCNL from list on an exact match
 * also shifts all following SCNLs in the array back one array index spot
 * this does not change the allocated size of the SCNL array,
 * but it does change the count or number of SCNLs in the array if a match is found
 * returns the array index that the SCNL was found at and thus removed from if success
 * returns -1 if error
 */
int DeleteSCNLFromList(SCNLList * pList, SCNL *p_scnl)
{
    int indexInList = WARNING_NOT_FOUND;

    if (pList == NULL || p_scnl == NULL )
    {
        // invalid input params
        reportError(WD_WARNING_ERROR, SYSERR, "invalid NULL input params.\n");
        return MODULE_RET_ERROR;
    }

    // see if the SCNL is in my list first
    indexInList = FindSCNLInList(pList, p_scnl);

    // if it is in my list then delete it
    if(indexInList > -1  && indexInList < pList->Count)
    {
        indexInList = DeleteSCNLFromListByIndex(pList, indexInList);
    }

    return indexInList;
}

/**
 * delete SCNL(s) from list based on wild card match
 * p_wildcard is ignored for now, and assumed to be '*'
 */
int DeleteSCNLsFromList(SCNLList * pList, SCNL *p_scnl, const char *p_wildCard)
{
    int i = 0;
    int numDeletedSCNLs = 0;

    if (pList == NULL || p_scnl == NULL || p_wildCard == NULL)
    {
        // invalid input params
        reportError(WD_WARNING_ERROR, SYSERR, "invalid NULL input params.\n");
        return MODULE_RET_ERROR;
    }

    //Look thru entire list for wild card matches
    while (i > -1 && i < pList->Count)
    {
        i = FindSCNLInList(pList, p_scnl);
        reportError(WD_DEBUG, WD_DEBUG, "found scnl in list at index %d. \n", i);

        // if match found, delete that match
        if(i > -1 && i < pList->Count)
        {
            i = DeleteSCNLFromListByIndex(pList, i);
            reportError(WD_DEBUG, WD_DEBUG, "deleted scnl from list at index %d. \n", i);
            numDeletedSCNLs++;
        }
    }

    if(numDeletedSCNLs > 0)
        return numDeletedSCNLs;
    else
      return WARNING_NOT_FOUND;
}

int ExtendSCNLList(int *pNumAlloced, SCNL ** pCurrentList)
{
  SCNL *new_list;
  int new_alloc;

  if (pNumAlloced == NULL || pCurrentList == NULL)
    return MODULE_RET_ERROR;

  new_alloc = *pNumAlloced + SCNLLIST_INCREMENT_AMOUNT;
  new_list = (SCNL*) malloc(new_alloc * (sizeof(SCNL)) );
  if (new_list == NULL)
  {
    reportError (WD_FATAL_ERROR, MEMALLOC, "scnl_list_utils: Unable to extend SCNL list.\n");
        return MODULE_RET_ERROR;
  }

  if (*pCurrentList != NULL)
  {
    memcpy(new_list, *pCurrentList, sizeof(SCNL) * (*pNumAlloced));
    free(*pCurrentList);
  }

  *pCurrentList = new_list;
  *pNumAlloced = new_alloc;
  return *pNumAlloced;
}


/**
 * add to end of array
 * will only add if exact match not already in the list
 */
int AddSCNLToList(SCNLList * pList, SCNL *pNewSCNL)
{
    int isInList = -1;

    if (pList == NULL || pNewSCNL == NULL )
    {
        // invalid input params
        return MODULE_RET_ERROR;
    }

    isInList = FindSCNLInList(pList, pNewSCNL);
    // if this exact SCNL already exists in the list then don't add it
    // just return with current count unchanged
    if(isInList > -1)
        return pList->Count;

    // its not already in the list so make sure there is room to add it
    if (pList->numAlloced <= pList->Count)
    {
        if (ExtendSCNLList(&pList->numAlloced, &pList->List) == MODULE_RET_ERROR)
            return MODULE_RET_ERROR;
    }

    //add to list
    memcpy(&(pList->List[pList->Count]), pNewSCNL, sizeof(SCNL));
    pList->Count++;

    return(pList->Count);
}


/*
 * Strip trailing blanks and newlines from string.
 */
int strib( char *string )
{
  int i, length;

  length = strlen( string );
  if ( length )
  {
    for ( i = length-1; i >= 0; i-- )
    {
      if ( string[i] == ' ' || string[i] == '\n' )
    string[i] = '\0';
      else
    return ( i+1 );
    }
  }
  else
    return length;
  return ( i+1 );
}



int Compare_SCNLs(void * p1, void * p2)
{
  SCNL * pA;
  SCNL * pB;
  char   szLocA[TRACE_LOC_LEN+1];
  char   szLocB[TRACE_LOC_LEN+1];
  int    rc;

  pA = (SCNL *) p1;
  pB = (SCNL *) p2;


  /* Check inputs, return if error */
  if(   pA == NULL || pB == NULL
     || pA->sta == NULL || pA->comp == NULL || pA->net == NULL || pA->loc == NULL
     || pB->sta == NULL || pB->comp == NULL || pB->net == NULL || pB->loc == NULL
    )
  {
    return(0);
  }


  /* Check Sta code */
  if(pA->sta[0] != cWILDCARD  && pB->sta[0] != cWILDCARD
     && (rc = strcmp(pA->sta, pB->sta))
    )
  {
    /* sta codes don't match, return */
    return rc;
  }

  /* Check Net code */
  if(pA->net[0] != cWILDCARD  && pB->net[0] != cWILDCARD
     && (rc = strcmp(pA->net, pB->net))
    )
  {
    /* comp codes don't match, return */
    return rc;
  }

  /* Check Comp code */
  if(pA->comp[0] != cWILDCARD  && pB->comp[0] != cWILDCARD
     && (rc = strcmp(pA->comp, pB->comp))
    )
  {
    /* comp codes don't match, return */
    return rc;
  }

  /* Check Loc code */
  if(pA->loc[0] != cWILDCARD  && pB->loc[0] != cWILDCARD)
  {
    if(pA->loc[0] == 0x00)
      strcpy(szLocA, szNO_LOC_CODE);
    else
    {
      strncpy(szLocA, pA->loc, sizeof(szLocA));
      szLocA[sizeof(szLocA)-1] = 0x00;
    }

    if(pB->loc[0] == 0x00)
      strcpy(szLocB, szNO_LOC_CODE);
    else
    {
      strncpy(szLocB, pB->loc, sizeof(szLocB));
      szLocA[sizeof(szLocB)-1] = 0x00;
    }

    return(strcmp(szLocA, szLocB));
  }

  return(0);
}  /* end Compare_SCNLs() */



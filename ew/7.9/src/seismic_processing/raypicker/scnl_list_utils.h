/******************
 * Functions to compare, find, add, delete and grown an array of SCNL structs
 * built originally to support passports and the concept of maintaining
 * two lists of SCNLs, one a list of SCNLs to use, the other a list
 * of SCNLs not to use
 */

#ifndef SCNL_LIST_UTILS_H
#define SCNL_LIST_UTILS_H

#include <trace_buf.h>

#define SCNLLIST_INCREMENT_AMOUNT 50
#define WARNING_NOT_FOUND -2
#define NULL_LOC_CODE "--"

static const char WILD_CARD_STR[] = "*"; 

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

/************** Comparison/Find Functions *****************/
/***
 * Checks if SCNLs match exactly (no wild cards taken into account)
 * for example * BHZ * == * BHZ * returns 1 for a match
 * but * BHZ * == * BHZ IU returns zero as the netword codes are not the same characters
 * like wise ACSO BHZ UI == ACSO HHZ UI returns 0 for NO match
 * returns 1 if sncls are an exact match
 * returns 0 if sncls are NOT an exact match
 * returns -1 if invalid parameters
 */
int DoSCNLsMatchExact( SCNL *p_scnlA, SCNL *p_scnlB);

/**
 * Checks if SCNLs match using the wild card character
 * for example using a wild card of "*"
 * "* BHZ *" == "* BHZ *" returns 1 for a match
 * likewise * BHZ * == * BHZ IU returns 1 for a match
 * but ACSO BHZ UI == ACSO HHZ UI returns 0 for NO match
 * returns 1 if sncls are an exact match
 * returns 0 if sncls are NOT an exact match
 * returns MODULE_RET_ERROR (-1) if invalid parameters or error
 */
int DoSCNLsMatchWithWildCard( SCNL *p_scnlA, SCNL *p_scnlB, const char * p_wildCard);

/*
 * All parameters, except p_wild are required.
 * 
 * If p_wild is the wildcard string, if NULL then no wildcards are allowed
 * 
 * @return 1 matched
 *         0 not matched
 *        -1 bad parameter
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
                );

/**
 * returns array index of SCNL's position in the list if it is found on exact match
 * returns WARNING_NOT_FOUND (-2) if exact match not found
 * returns MODULE_RET_ERROR (-1) if an error was encountered
 */
int FindSCNLInList(SCNLList *pList, SCNL *p_scnl);

/**
 * returns the first found array index of SCNL's position in the list 
 *         if it is found on wild card match
 * returns WARNING_NOT_FOUND (-2) if wild card match not found
 * returns MODULE_RET_ERROR (-1) if an error was encountered        
 */
int FindSCNLInListWithWildCard(SCNLList *pList, int startAt, SCNL *p_scnl, const char *p_wildCard);


/****************** List Manipulation Functions ****************/

/**
 * Add the SCNL to the list - add at the end of the array
 * will extend the allocated size fo the array if there is not enough room to add it
 * thus may change allocated size of list in addition to count
 * will not add the SCNL to the list if there is an EXACT match
 * returns array index at where SCNL was added which is equal to pList->Count
 * returns -1 if error
 */
int AddSCNLToList(SCNLList * pList, SCNL *pNewSCNL);

/**
 * grow the array of SCNL structs 
 * changes num alloced in the list
 * returns num alloced if successful
 * returns -1 if error
 */
int ExtendSCNLList(int *pNumAlloced, SCNL ** pCurrentList);

/**
 * delete a SCNL from list on an exact match
 * also shifts all following SCNLs in the array back one array index spot
 * this does not change the allocated size of the SCNL array, 
 * but it does change the count or number of SCNLs in the array if a match is found
 * returns the array index that the SCNL was found at and thus removed from if success
 * returns MODULE_RET_ERROR (-1) if error
 * returns WARNING_NOT_FOUND (-2) if not found in list
 */
int DeleteSCNLFromList(SCNLList * pList, SCNL *pNewSCNL);

/**
 * delete all wild card matching SCNL(s) from list 
 * also shifts all following SCNLs in the array back one array index spot
 * this does not change the allocated size of the SCNL array, 
 * but it does change the count or number of SCNLs in the array if a match is found
 * returns the number of SCNLs deleted from the list if successful
 * returns MODULE_RET_ERROR (-1) if error
 * returns WARNING_NOT_FOUND (-2) if not found in list
 */
int DeleteSCNLsFromList(SCNLList * pList, SCNL *pNewSCNL, const char *p_wildCard);

/**
 * Delete the SCNL at the specified array index
 * and shift all following SCNLs back one index in the array
 * returns the array index at which the SCNL was removed from the list if successful
 * returns MODULE_RET_ERROR (-1) if error
 * returns WARNING_NOT_FOUND (-2) if not found in list
 */
int DeleteSCNLFromListByIndex(SCNLList *pList, int scnlIndex);


/**
 * Strip trailing blanks and newlines from string.
 * Useful for stripping blanks from SCNL strings read from fixed-len fields.
 */
int strib( char *string );  /* strip trailing blanks / \n */


#endif


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: index.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/18 22:30:20  lombard
 *     Modified for location code
 *
 *     Revision 1.2  2000/07/08 19:01:07  lombard
 *     Numerous bug fies from Chris Wood
 *
 *     Revision 1.1  2000/02/14 19:58:27  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include "wave_serverV.h"

/********* The Index Manipulation Routines ***********************************/

/*
Oh, how to implement the index: 
	The index is a malloc'd array of DATA_CHUNK entries. The active chunks are kept on a 
	linked list of these entries. The list head - pointer to first element of the index -
	is kept in TANK.IndxStart. I suppose the unused elements should be kept in a free list.
	But for openers a brute search is done for likks with -1's in the offset field, meaning
	it's not used. 
		An unused entry also has 0's for time, -1 for offset, and NULL for the 'next'
	 pointer. This is used by the client server thread as a safety thing to avoid dealing
	with a dead index entry.
	This is to provide hooks for the client server threads to easily see when an index
	entry is not in use. The motivation is simpler code. The price is more cpu usage.
		Convention: an empty index consists of TANK.IndxStart pointing to an entry in
	the chunk array. That element has start and stop times set to zero, and a null 'next'
	pointer.
*/
	
/*********************************************************************************************************/
void IndexInit( TANK* t)
/* Obsolete
*/
{
   return;
}
/*********************************************************************************************************/
int IndexAdd( TANK* t, double tS, double tE, long int offst)
/* Add a new index entry

  Return Values:
         0  Success
         1  Overwrote Index
         -1 Unrecoverable Error
         All Other Values Undefined
*/
{
   DATA_CHUNK* new;
   int AALStatus;
   new=AddALink(t,&AALStatus); /* clips an new element to end of index list */
   if( AALStatus < 0) 
     return(-1); /* oh crap */
   new->tStart = tS;
   new->tEnd   = tE;
   new->offset = offst;
   return(AALStatus);
}

/*********************************************************************************************************/
DATA_CHUNK* AddALink( TANK* t, int *pStatus )
/* Helper routine for IndexAdd above:
   return pointer to next DATA_CHUNK after the EOL

   *pStatus(Status) values:
         0  Success
         1  Overwrote Index
         -1 Unrecoverable Error with t->chunkIndex
         -2 Unrecoverable Error with t->indxStart,Finish
         All Other Values Undefined
*/
{
   unsigned int index;
   DATA_CHUNK* new;

   if(t->indxFinish == (unsigned int)(t->indxMaxChnks -1))
   {
     /* We are at the end of the ChunkArray, 
        go back to the beginning    */
     index = 0;
   }
   else
   {
     /* Move to the next index */
     index=(t->indxFinish) + 1;
   }
   /* if we have wrapped on top of the start of the index */
   if(index == t->indxStart)
   {

     /*  No more indexes.  We assume that the newer data is 
         more valuable than the older data, so let us overwrite 
         the oldest index. 
     */

     /* Move indxStart to the next entry, before writing indxFinish
        to the current entry  */
     (t->indxStart)+=1;

     /* if we passed the end of the list, go back to the beginning */
     if(t->indxStart >= (unsigned long )(t->indxMaxChnks))
     {
       t->indxStart=0;
     }
     *pStatus=1;
   }
   else /* if(index == t->indxStart) 
           (overwriting an index) */ 
   {
     *pStatus=0;
   }

   /* We've found a home. */
   t->indxFinish=index;
   new=&((t->chunkIndex)[index]);
   if(!new)
   {
     logit("t","t->chunkIndex corrupted for tank %s\n",
           t->tankName);
     *pStatus=-1;
   }
   if(t->indxStart >= (unsigned int)(t->indxMaxChnks) ||
      t->indxFinish >= (unsigned int)(t->indxMaxChnks) )
   {
     logit("t","t->indxStart=%d or indxFinish=%d corrupted for tank %s\n",
           t->indxStart,t->indxFinish);
      *pStatus=-2;
   }

   return(new);
}
   
/*********************************************************************************************************/
void            IndexDel( TANK* t, DATA_CHUNK* victim )
/* excise the specified element from the index of the named tank
   which is a fancy way of saying move the start of the Tank index
   forward an entry.  
   Be cautious of a list where the start and finish are the same entry,
   in such a case, to delete the node, we need to invalidate the node
   somehow.
*/
{

  if(t->indxStart == t->indxFinish)
  {
    /* This is a special case, where there is only one
       element in the list.  I don't know when this would
       occur, but we will assume it can.  What we need to
       do is to invalidate the single element in the list,
       so that when IndexAdd() is called, it can check for
       an empty list, by checking to see if the list is 1
       element long, and the element is invalid.
    */
    /* Invalidate the element, by setting it's start and stop times to 0,
       and it's file offset to -1
    */
    victim->tStart=0.0;
    victim->tEnd=0.0;
    victim->offset=-1;
  }
  else  /* (more than one element in list) perform a normal deletion */
  {
    if(t->indxStart == (unsigned int)((t->indxMaxChnks)-1))
    {
      t->indxStart=0;
    }
    else /* Not at end of chunk array */
    {
      t->indxStart=(t->indxStart)+1;
    }
  }
  return;
}

/*********************************************************************************************************/
DATA_CHUNK* IndexNext( TANK* t, DATA_CHUNK* thisOne)
/* Obsolete */
{
 return(0);
}   

/*********************************************************************************************************/
DATA_CHUNK*     IndexOldest( TANK* t )
/* Return a pointer to the oldest segment in the index
   (aka first element)
*/
{ 
   return(&((t->chunkIndex)[t->indxStart]));
}
/*********************************************************************************************************/
DATA_CHUNK*     IndexYoungest( TANK* t )
/* Return a pointer to the most recent segment in the index
   (aka last element)
*/
{ 
  return(&((t->chunkIndex)[t->indxFinish]));
}

/**********************************************************************************************************/
void LogTank( TANK* tptr )
/* log tank structure entries to log file for debugging */
{
   DATA_CHUNK* ptr;
   unsigned int i;

   logit (""," Start of Tank Structure dump\n");
   logit (""," tankName:    %s\n", tptr->tankName);
   logit (""," tfp:         %ld\n", tptr->tfp);
   logit (""," ifp:         %ld\n", tptr->ifp);
   logit (""," logo:        %d %d %d \n", tptr->logo.instid, tptr->logo.mod, tptr->logo.type);
   logit (""," pin:         %d\n", tptr->pin);
   logit (""," sta:         %s\n", tptr->sta);
   logit (""," net:         %s\n", tptr->net);
   logit (""," chan:         %s\n", tptr->chan);
   logit (""," loc:         %s\n", tptr->loc);
   logit (""," datatype:         %s\n", tptr->datatype);
   logit (""," samprate:         %f\n", tptr->samprate);
   logit (""," tankSize:    %ld\n", tptr->tankSize);
   logit (""," indxMaxChnks: %ld\n", tptr->indxMaxChnks);
   logit (""," recSize:     %ld\n", tptr->recSize);
   logit (""," nRec:        %ld\n", tptr->nRec);
   logit (""," inPtOffset: %ld\n", tptr->inPtOffset);
   logit (""," firstPass:   %d\n", tptr->firstPass);
   logit (""," firstWrite:  %d\n", tptr->firstWrite);
   logit (""," chunkIndex:  %lld\n",(long long)tptr->chunkIndex);
   logit (""," indxStart:   %ld\n",(long)tptr->indxStart);
   logit (""," The Index:\n");
   ptr=tptr->chunkIndex;
   for(i=tptr->indxStart; i!=tptr->indxFinish; i++)
   {
     if(i == (unsigned int)(tptr->indxMaxChnks))
     {
       i=0;
     }
     logit(""," startT: %f, endT: %f, offset: %ld\n", ptr[i].tStart,
           ptr[i].tEnd, ptr[i].offset);  
   }
   logit(""," startT: %f, endT: %f, offset: %ld\n", ptr[i].tStart,
         ptr[i].tEnd, ptr[i].offset);  
   logit(""," End of Tank dump\n\n");
}

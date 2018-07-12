/*
 * compare.c
 * Same as compare.c, Revision 1.2 in src/seismic_processing/pick_ew
 *
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 */

#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include "pick_FP.h"


     /*************************************************************
      *                       CompareSCNL()                       *
      *                                                           *
      *  This function is passed to qsort() and bsearch().        *
      *  We use qsort() to sort the station list by SCNL numbers, *
      *  and we use bsearch to look up an SCNL in the list.       *
      *************************************************************/

int CompareSCNL( const void *s1, const void *s2 )
{
   int rc;
   STATION *t1 = (STATION *) s1;
   STATION *t2 = (STATION *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->chan, t2->chan );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->net,  t2->net );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->loc,  t2->loc );
   return rc;
}

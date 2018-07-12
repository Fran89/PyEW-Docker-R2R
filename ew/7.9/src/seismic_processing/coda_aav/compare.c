/* THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU CHECKED IT OUT!
 *
 *  $Id: compare.c 490 2009-11-09 19:16:15Z dietz $
 * 
 *  Revision history:
 *   $Log$
 *   Revision 1.1  2009/11/09 19:15:28  dietz
 *   Initial version, may still contain bugs and debugging statements
 *
 */

#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include "coda_aav.h"

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

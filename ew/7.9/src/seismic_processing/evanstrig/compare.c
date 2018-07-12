
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: compare.c 1507 2004-05-21 22:32:23Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/21 22:32:23  dietz
 *     added location code; inputs TYPE_TRACEBUF2, outputs TYPE_LPTRIG_SCNL
 *
 *     Revision 1.1  2000/02/14 17:17:36  lucky
 *     Initial revision
 *
 *
 */

#include <string.h>
#include "evanstrig.h"


     /*************************************************************
      *                       CompareSCNLs()                      *
      *                                                           *
      *  This function is passed to qsort() and bsearch().        *
      *  We use qsort() to sort the station list by SCNL numbers, *
      *  and we use bsearch to look up an SCNL in the list.       *
      *************************************************************/

int CompareSCNLs( const void *s1, const void *s2 )
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


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: compare.c,v 1.1 2000/02/14 19:06:49 lucky Exp $
 *
 *    Revision history:
 *     $Log: compare.c,v $
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include "pick_ew.h"


     /*************************************************************
      *                       CompareSCNs()                       *
      *                                                           *
      *  This function is passed to qsort() and bsearch().        *
      *  We use qsort() to sort the station list by SCN numbers,  *
      *  and we use bsearch to look up an SCN in the list.        *
      *************************************************************/

int CompareSCNs( const void *s1, const void *s2 )
{
   int rc;
   STATION *t1 = (STATION *) s1;
   STATION *t2 = (STATION *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->chan, t2->chan );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->net,  t2->net );
   return rc;
}

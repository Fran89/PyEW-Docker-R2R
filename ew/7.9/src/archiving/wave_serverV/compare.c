
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: compare.c 1577 2004-06-10 23:14:10Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/06/10 23:14:10  lombard
 *     Fixed logging of invalid packets
 *     Fixed compareSCNL function.
 *
 *     Revision 1.1  2000/02/14 19:58:27  lucky
 *     Initial revision
 *
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <earthworm.h>
#include <transport.h>
#include "wave_serverV.h"

     /*************************************************************
      *                       CompareSCNLs()                      *
      *                                                           *
      *  This function is passed to qsort() and bsearch().        *
      *  We use qsort() to sort the tank list by SCNL numbers,    *
      *  and we use bsearch to look up an SCNL in the list.       *
      *************************************************************/

int CompareTankSCNLs( const void *s1, const void *s2 )
{
   int rc;
   TANK *t1 = (TANK *) s1;
   TANK *t2 = (TANK *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->chan, t2->chan );
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->net,  t2->net );
   if (rc != 0 ) return(rc);
   rc = strcmp( t1->loc, t2->loc );
   return(rc);
}

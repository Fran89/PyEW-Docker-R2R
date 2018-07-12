
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: cmprscn.c 1458 2004-05-11 17:49:07Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * cmprscn.c: CompareSCNs
 *              1) Compare alphabetical order of SCN's
 *		2) Passed to qsort() to sort the station list
 *		3) Passed to bsearch() to look up an SCN in the list
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: compareSCNs						*/
/*									*/
/*	Inputs:		Pointers to two station structures		*/
/*									*/
/*	Outputs:	Relative order of SCNs				*/
/*									*/
/*	Returns:	0 if SCN's are identical			*/
/*		       +1 if first SCN comes after second SCN		*/
/*		       -1 if second SCN comes after first SCN		*/


/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <string.h>	/* strcmp					*/

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit					*/

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/


int CompareSCNs( const void *s1, const void *s2 )
{
   int rc;
   STATION *t1 = (STATION *) s1;
   STATION *t2 = (STATION *) s2;

   rc = strcmp( t1->staCode, t2->staCode );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->compCode, t2->compCode );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->netCode,  t2->netCode );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->locCode,  t2->locCode );
   return rc;
}

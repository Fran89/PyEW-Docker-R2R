
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: findsta.c 1458 2004-05-11 17:49:07Z lombard $
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
 * findsta.c: Find a station in a station list.
 *              1) Search the sorted station list using bsearch 
 *                   for a station with the given station, component, 
 *                   and network codes.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: FindStation						*/
/*									*/
/*	Inputs:		Pointer to a string(station code)		*/
/*			Pointer to a string(component code)		*/
/*			Pointer to a string(network code)		*/
/*			Pointer to the World structure			*/
/*									*/
/*	Outputs:	Pointer to the station of interest or not found	*/
/*									*/
/*	Returns:	Valid pointer to a station structure on success	*/
/*			NULL pointer on failure				*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <string.h>	/* strcmp					*/
#include <stdlib.h>	/* bsearch					*/

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: FindStation						*/
STATION* FindStation( char* staCode, char* compCode, char* netCode,
		      char* locCode, NETWORK* csuNet )
{
  STATION       key;            /* Key for searching			*/
  STATION*	retSta = NULL;	/* Return value for this function.	*/

  /* Point the key items toward that for which we are searching		*/
  strcpy( key.staCode, staCode );
  strcpy( key.compCode, compCode );
  strcpy( key.netCode, netCode );
  strcpy( key.locCode, locCode );
  
  /* Find it */
  retSta = (STATION *) bsearch(&key, csuNet->stations, 
			       csuNet->nSta,
			       sizeof(STATION), CompareSCNs );
  
  return ( retSta );
}

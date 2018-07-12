
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: findsta.c 1532 2004-05-31 17:57:19Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/31 17:57:19  lombard
 *     Minor bug fixes
 *
 *     Revision 1.2  2004/05/05 23:54:04  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
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

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: FindStation                                           */
/*                                                                      */
/*      Inputs:         Pointer to a string(station code)               */
/*                      Pointer to a string(component code)             */
/*                      Pointer to a string(network code)               */
/*                      Pointer to the World structure                  */
/*                                                                      */
/*      Outputs:        Pointer to the station of interest or not found */
/*                                                                      */
/*      Returns:        Valid pointer to a station structure on success */
/*                      NULL pointer on failure                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <string.h>     /* strcmp                                       */
#include <stdlib.h>     /* bsearch                                      */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: FindStation                                           */
STATION* FindStation( char* staCode, char* compCode, char* netCode,
		      char* locCode, WORLD* cstWorld )
{
  STATION       key;            /* Key for searching                    */
  STATION*      retSta = NULL;  /* Return value for this function.      */

  /* Point the key items toward that for which we are searching         */
  strcpy( key.staCode, staCode );
  strcpy( key.compCode, compCode );
  strcpy( key.netCode, netCode );
  strcpy( key.locCode, locCode );
  
  /* Find it */
  retSta = (STATION *) bsearch(&key, cstWorld->stations, 
                               cstWorld->nSta,
                               sizeof(STATION), CompareSCNs );
  
  return ( retSta );
}

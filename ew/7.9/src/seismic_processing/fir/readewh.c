
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readewh.c 1652 2004-07-28 22:43:05Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/07/28 22:43:04  lombard
 *     Modified to handle SCNLs and TYPE_TRACEBUF2 (only!) messages.
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * readewh.c: Read the Earthworm parameters.
 *              1) Set members of the FIREWH structure.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ReadEWH                                               */
/*                                                                      */
/*      Inputs:         Pointer to a Fir Earthworm parameter       */
/*                        structure                                     */
/*                                                                      */
/*      Outputs:        Updated input structure(above)                  */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
/*                      failure                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* GetInst, GetLocalInst, GetModId, GetType     */

/*******                                                        *********/
/*      Fir Includes                                               */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ReadEWH                                               */
int ReadEWH( WORLD* pFir )
{

  if ( GetLocalInst( &(pFir->firEWH.myInstId)) != 0 )
  {
    fprintf(stderr, "fir: Error getting myInstId.\n" );
    return EW_FAILURE;
  }
  
  if ( GetModId( pFir->firParam.myModName, &(pFir->firEWH.myModId)) != 0 )
  {
    fprintf( stderr, "fir: Error getting myModId.\n" );
    return EW_FAILURE;
  }

  if ( GetInst( pFir->firParam.readInstName, &(pFir->firEWH.readInstId)) != 0)
  {
    fprintf( stderr, "fir: Error getting readInstId.\n" );
    return EW_FAILURE;
  }
  
  if ( GetModId( pFir->firParam.readModName, &(pFir->firEWH.readModId)) != 0 )
  {
    fprintf( stderr, "fir: Error getting readModId.\n" );
    return EW_FAILURE;
  }

  /* Look up keys to shared memory regions */
  if ((pFir->firEWH.ringInKey = GetKey (pFir->firParam.ringIn)) == -1) 
  {
    fprintf (stderr,
             "fir:  Invalid ring name <%s>; exitting!\n", 
             pFir->firParam.ringIn);
    return EW_FAILURE;
  }

  if ((pFir->firEWH.ringOutKey = GetKey (pFir->firParam.ringOut) ) == -1) 
  {
    fprintf (stderr,
             "fir:  Invalid ring name <%s>; exitting!\n", 
             pFir->firParam.ringOut);
    return EW_FAILURE;
  }

  /* Look up message types of interest */
  if (GetType ("TYPE_HEARTBEAT", &(pFir->firEWH.typeHeartbeat)) != 0) 
  {
    fprintf (stderr, 
             "fir: Invalid message type <TYPE_HEARTBEAT>; exitting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_ERROR", &(pFir->firEWH.typeError)) != 0) 
  {
    fprintf (stderr, 
             "fir: Invalid message type <TYPE_ERROR>; exitting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_TRACEBUF2", &(pFir->firEWH.typeWaveform)) != 0) 
  {
    fprintf (stderr, 
             "fir: Invalid message type <TYPE_TRACEBUF2>; exitting!\n");
    return EW_FAILURE;
  }

  return EW_SUCCESS;

} 


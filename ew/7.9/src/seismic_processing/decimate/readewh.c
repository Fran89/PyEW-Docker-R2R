
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readewh.c 1459 2004-05-11 18:14:18Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/11 18:14:18  dietz
 *     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
 *
 *     Revision 1.2  2002/10/25 17:59:44  dietz
 *     Added support for multiple GetWavesFrom commands
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * readewh.c: Read the Earthworm parameters.
 *              1) Set members of the DCMEWH structure.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ReadEWH                                               */
/*                                                                      */
/*      Inputs:         Pointer to a Decimate Earthworm parameter       */
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
/*      Decimate Includes                                               */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: ReadEWH                                               */
int ReadEWH( WORLD* pDcm )
{
  int i;

  if ( GetLocalInst( &(pDcm->dcmEWH.myInstId)) != 0 )
  {
    logit("e", "decimate: Error getting myInstId for %s.\n", 
          pDcm->dcmEWH.myInstId);
    return EW_FAILURE;
  }
  
  if ( GetModId( pDcm->dcmParam.myModName, &(pDcm->dcmEWH.myModId)) != 0 )
  {
    logit("e", "decimate: Error getting myModId for %s.\n",
           pDcm->dcmParam.myModName );
    return EW_FAILURE;
  }

  for (i=0; i<pDcm->dcmParam.nlogo; i++ ) {
    if ( GetInst( pDcm->dcmParam.readInstName[i], &(pDcm->dcmEWH.readInstId[i])) != 0)
    {
      logit("e", "decimate: Error getting readInstId for %s.\n",
             pDcm->dcmParam.readInstName[i] );
      return EW_FAILURE;
    }
  
    if ( GetModId( pDcm->dcmParam.readModName[i], &(pDcm->dcmEWH.readModId[i])) != 0 )
    {
      logit("e", "decimate: Error getting readModName for %s.\n",
             pDcm->dcmParam.readModName[i] );
      return EW_FAILURE;
    }

    if ( GetType( pDcm->dcmParam.readTypeName[i], &(pDcm->dcmEWH.readMsgType[i])) != 0 )
    {
      logit("e", "decimate: Error getting readMsgType for %s.\n",
             pDcm->dcmParam.readTypeName[i] );
      return EW_FAILURE;
    }
  }

  /* Look up keys to shared memory regions */
  if ((pDcm->dcmEWH.ringInKey = GetKey (pDcm->dcmParam.ringIn)) == -1) 
  {
    logit("e", "decimate:  Invalid input ring name <%s>; exiting!\n", 
           pDcm->dcmParam.ringIn);
    return EW_FAILURE;
  }

  if ((pDcm->dcmEWH.ringOutKey = GetKey (pDcm->dcmParam.ringOut) ) == -1) 
  {
    logit("e", "decimate:  Invalid output ring name <%s>; exiting!\n", 
          pDcm->dcmParam.ringOut);
    return EW_FAILURE;
  }

  /* Look up message types of interest */
  if (GetType ("TYPE_HEARTBEAT", &(pDcm->dcmEWH.typeHeartbeat)) != 0) 
  {
    logit("e", "decimate: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_ERROR", &(pDcm->dcmEWH.typeError)) != 0) 
  {
    logit("e", "decimate: Invalid message type <TYPE_ERROR>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_TRACEBUF", &(pDcm->dcmEWH.typeTrace)) != 0) 
  {
    logit("e", "decimate: Invalid message type <TYPE_TRACEBUF>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_TRACEBUF2", &(pDcm->dcmEWH.typeTrace2)) != 0) 
  {
    logit("e", "decimate: Invalid message type <TYPE_TRACEBUF2>; exiting!\n");
    return EW_FAILURE;
  }

  return EW_SUCCESS;

} 


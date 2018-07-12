
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initpars.c 4882 2012-07-06 19:54:34Z paulf $
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
 * initpars.c: Initialize parameter structures used by Decimate.
 *              1) Initialize members of the DCMEWH structure.
 *              2) Initialize members of the WORLD structure.
 *              3) Allocates memory for the trigger message buffer.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: InitializeParameters                                  */
/*                                                                      */
/*      Inputs:         Pointer to a Decimate network parameter         */
/*                        structure                                     */
/*                                                                      */
/*      Outputs:        Updated inputs(above)                           */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in decimate.h on          */
/*                      failure                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <stdlib.h>     /* malloc                                       */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      Decimate Includes                                            */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: InitializeParameters                                  */
void InitializeParameters( WORLD* pDcm )
{
  int i;

  /*    Initialize members of the DCMEWH structure                      */
  pDcm->dcmEWH.myInstId = 0;
  pDcm->dcmEWH.myModId  = 0;
  for( i=0; i<MAX_LOGO; i++ ) {
     pDcm->dcmEWH.readInstId[i]  = 0;
     pDcm->dcmEWH.readModId[i]   = 0;
     pDcm->dcmEWH.readMsgType[i] = 0;
  }
  pDcm->dcmEWH.typeTrace     = 0;
  pDcm->dcmEWH.typeTrace2    = 0;
  pDcm->dcmEWH.typeError     = 0;
  pDcm->dcmEWH.typeHeartbeat = 0;
  pDcm->dcmEWH.ringInKey     = 0l;
  pDcm->dcmEWH.ringOutKey    = 0l;

  /*    Initialize members of the DCMPARAM structure                    */
  for( i=0; i<MAX_LOGO; i++ ) {
     sprintf( pDcm->dcmParam.readInstName[i], "" );
     sprintf( pDcm->dcmParam.readModName[i],  "" );
     sprintf( pDcm->dcmParam.readTypeName[i], "" );
  }
  pDcm->dcmParam.ringIn[0]      = '\0';
  pDcm->dcmParam.ringOut[0]     = '\0';
  pDcm->dcmParam.nlogo          = 0;
  pDcm->dcmParam.heartbeatInt   = 15;
  pDcm->dcmParam.debug          = 0;
  pDcm->dcmParam.quiet          = 0;
  pDcm->dcmParam.logSwitch      = 1;
  pDcm->dcmParam.minTraceBufLen = 10;
  pDcm->dcmParam.cleanStart     = 1;
  pDcm->dcmParam.testMode       = 0;
  pDcm->dcmParam.maxGap         = 1.5;
  
  pDcm->stations = NULL;
  pDcm->nSta     = 0;
  pDcm->nStage   = 0;

  return;
}

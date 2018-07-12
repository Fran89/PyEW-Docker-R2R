
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initpars.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * initpars.c: Initialize parameter structures used by Fir.
 *              1) Initialize members of the FIREWH structure.
 *              2) Initialize members of the WORLD structure.
 *              3) Allocates memory for the trigger message buffer.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: InitializeParameters                                  */
/*                                                                      */
/*      Inputs:         Pointer to a Fir WORLD parameter                */
/*                        structure                                     */
/*                                                                      */
/*      Outputs:        Updated inputs(above)                           */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in fir.h on          */
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
/*      Fir Includes                                            */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: InitializeParameters                                  */
void InitializeParameters( WORLD* pFir )
{
  /*    Initialize members of the FIREWH structure                      */
  pFir->firEWH.myInstId = 0;
  pFir->firEWH.myModId = 0;
  pFir->firEWH.readInstId = 0;
  pFir->firEWH.readModId = 0;
  pFir->firEWH.typeWaveform = 0;
  pFir->firEWH.typeError = 0;
  pFir->firEWH.typeHeartbeat = 0;
  pFir->firEWH.ringInKey = 0l;
  pFir->firEWH.ringOutKey = 0l;

  /*    Initialize members of the FIRPARAM structure                    */
  sprintf( pFir->firParam.readInstName, "INST_WILDCARD" );
  sprintf( pFir->firParam.readModName, "MOD_WILDCARD" );
  pFir->firParam.ringIn[0] = '\0';
  pFir->firParam.ringOut[0] = '\0';
  pFir->firParam.heartbeatInt = 15;
  pFir->firParam.debug = 0;
  pFir->firParam.logSwitch = 1;
  pFir->firParam.testMode = 0;
  pFir->firParam.maxGap = 1.5;
  
  pFir->FirStatus = 0;
  pFir->stations = NULL;
  pFir->nSta = 0;
  pFir->filter.Length = 0;
  pFir->filter.coef = NULL;
  pFir->pBand = NULL;
  
  return;
}


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initpars.c 2470 2006-10-20 15:44:37Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2006/10/20 15:44:37  paulf
 *     udpated with changes from Utah, STAtime now configurable
 *
 *     Revision 1.4  2005/04/12 22:43:07  dietz
 *     Added optional command "GetWavesFrom <instid> <module_id>"
 *
 *     Revision 1.3  2004/05/05 23:54:04  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.2  2001/05/09 18:13:53  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/*
 * initpars.c: Initialize parameter structures used by CarlStaTrig.
 *              1) Initialize members of the CSTEWH structure.
 *              2) Initialize members of the WORLD structure.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: InitializeParameters                                  */
/*                                                                      */
/*      Inputs:         Pointer to an Earthworm parameter structure     */
/*                      Pointer to a CarlTrig network parameter         */
/*                        structure                                     */
/*                                                                      */
/*      Outputs:        Updated inputs(above)                           */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      Error code as defined in carlstatrig.h on       */
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
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: InitializeParameters                                  */
int InitializeParameters( CSTEWH* cstEwh, WORLD* cstWorld )
{
  int   retVal = 0;     /* Return value for this function               */

  /*    Initialize members of the CSTEWH structure                      */
  cstEwh->myInstId = 0;
  cstEwh->myModId = 0;
  cstEwh->instWild = 0;
  cstEwh->modWild = 0;
  cstEwh->typeCarlStaTrig = 0;
  cstEwh->typeError = 0;
  cstEwh->typeHeartbeat = 0;
  cstEwh->typeWaveform = 0;

  /*    Initialize members of the WORLD structure                       */
  cstWorld->MyPid = getpid();
  if( cstWorld->MyPid == -1 )
  {
    logit("e","carlstatrig: Cannot get pid. Exiting.\n");
    return ERR_INIT;
  }
  cstWorld->cstEWH = cstEwh;
  cstWorld->Ratio = 1.0;
  cstWorld->LTAtime = 8;
  cstWorld->STAtime = 1;
  cstWorld->startUp = 7;
  cstWorld->Quiet = 0.0;
  cstWorld->decimation = 1;
  cstWorld->maxGap = 1;
  cstWorld->stations = NULL;

  /*    Initialize members of the CSTPARAM structure                    */
  cstWorld->cstParam.debug = 0;
  cstWorld->cstParam.ringIn[0] = '\0';
  cstWorld->cstParam.ringOut[0] = '\0';
  cstWorld->cstParam.staFile[0] = '\0';
  cstWorld->cstParam.heartbeatInt = 15;
  cstWorld->cstParam.ringInKey = 0;
  cstWorld->cstParam.ringOutKey = 0;
  cstWorld->cstParam.nGetLogo = 0;
  cstWorld->cstParam.GetLogo = NULL;

  return ( retVal );
}


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initsta.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * initsta.c: Initialize filter buffers for all stations
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: SetStaFilters                                         */
/*                                                                      */
/*      Inputs:         Pointer to World Structure                      */
/*                                                                      */
/*      Outputs:        Updated Station structures                      */
/*                                                                      */
/*      Returns:        UW_SUCCESS on success, else UW_FAILURE          */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      Fir Includes                                                    */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: InitSta                                               */
void InitSta( WORLD *pFir )
{
  int iSta;
  STATION *sta;             /* pointer to array of stations             */
  
  /* Set up filter buffer structures for all stations we know about     */
  for (iSta = 0; iSta < pFir->nSta; iSta++)
  {
    sta = &(pFir->stations[iSta]);
    sta->inBuff.starttime = 0.0;
    sta->inBuff.read = 0;
    sta->inBuff.write = 0;
    sta->inEndtime = 0.0;
    sta->outBuff.starttime = 0.0;
    sta->outBuff.read = 0;
    sta->outBuff.write = 0;
  }
  
  return;
}

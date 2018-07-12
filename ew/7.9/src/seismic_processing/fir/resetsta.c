
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: resetsta.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * resetsta.c: Reset station buffers for one STATION structure
 *              1) Reset station parameters and buffer pointers
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: ResetStation                                          */
/*                                                                      */
/*      Inputs:         Pointer to Station Structure                    */
/*                                                                      */
/*      Outputs:        Updated Station structures                      */
/*                                                                      */
/*      Returns:        nothing                                         */

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

/*      Function: SetStaFilters                                         */
void ResetStation( STATION *sta )
{
  sta->inEndtime = 0.0;
  sta->inBuff.starttime = 0.0;
  sta->inBuff.read = 0;
  sta->inBuff.write = 0;
  sta->outBuff.starttime = 0.0;
  sta->outBuff.read = 0;
  sta->outBuff.write = 0;
    
  return;
}
